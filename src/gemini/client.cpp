#include "client.h"

#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/error.h>
#include <mbedtls/net_sockets.h>
#include <mbedtls/sha256.h>
#include <mbedtls/ssl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <mutex>
#include <sstream>
#include <vector>

namespace gem {
namespace {

constexpr int kTimeoutSec = 15;
std::mutex g_knownHostsMutex;

std::string hex(const unsigned char* p, size_t n) {
    static const char* d = "0123456789abcdef";
    std::string s;
    for (size_t i = 0; i < n; ++i) {
        s += d[p[i] >> 4];
        s += d[p[i] & 15];
    }
    return s;
}

std::string mbedError(int ret) {
    char buf[160];
    mbedtls_strerror(ret, buf, sizeof buf);
    return buf;
}

// The known_hosts file: one "host:port sha256" per line.
std::string knownFingerprint(const std::string& path, const std::string& hostPort) {
    std::lock_guard<std::mutex> lock(g_knownHostsMutex);
    std::ifstream in(path);
    std::string hp, fp;
    while (in >> hp >> fp)
        if (hp == hostPort) return fp;
    return {};
}

void writeFingerprint(const std::string& path, const std::string& hostPort, const std::string& fingerprint) {
    std::lock_guard<std::mutex> lock(g_knownHostsMutex);
    std::vector<std::pair<std::string, std::string>> entries;
    {
        std::ifstream in(path);
        std::string hp, fp;
        while (in >> hp >> fp)
            if (hp != hostPort) entries.emplace_back(hp, fp);
    }
    entries.emplace_back(hostPort, fingerprint);
    std::ofstream out(path, std::ios::trunc);
    for (auto& [hp, fp] : entries) out << hp << ' ' << fp << '\n';
}

int sendCb(void* ctx, const unsigned char* buf, size_t len) {
    int fd = *static_cast<int*>(ctx);
    ssize_t n = ::send(fd, buf, len, 0);
    if (n >= 0) return (int)n;
    if (errno == EAGAIN || errno == EWOULDBLOCK) return MBEDTLS_ERR_SSL_TIMEOUT;
    return MBEDTLS_ERR_NET_SEND_FAILED;
}

int recvCb(void* ctx, unsigned char* buf, size_t len) {
    int fd = *static_cast<int*>(ctx);
    ssize_t n = ::recv(fd, buf, len, 0);
    if (n >= 0) return (int)n;  // 0 = peer closed
    if (errno == EAGAIN || errno == EWOULDBLOCK) return MBEDTLS_ERR_SSL_TIMEOUT;
    return MBEDTLS_ERR_NET_RECV_FAILED;
}

// RAII bundle of everything mbedtls needs for one connection.
struct Tls {
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context drbg;
    mbedtls_ssl_config conf;
    mbedtls_ssl_context ssl;
    Tls() {
        mbedtls_entropy_init(&entropy);
        mbedtls_ctr_drbg_init(&drbg);
        mbedtls_ssl_config_init(&conf);
        mbedtls_ssl_init(&ssl);
    }
    ~Tls() {
        mbedtls_ssl_free(&ssl);
        mbedtls_ssl_config_free(&conf);
        mbedtls_ctr_drbg_free(&drbg);
        mbedtls_entropy_free(&entropy);
    }
};

struct Socket {
    int fd = -1;
    std::atomic<int>* published = nullptr;
    ~Socket() {
        if (published) published->store(-1);
        if (fd >= 0) ::close(fd);
    }
};

bool isCancelled(const std::atomic<bool>* c) { return c && c->load(); }

}  // namespace

void trustCertificate(const std::string& path, const std::string& hostPort, const std::string& fingerprint) {
    writeFingerprint(path, hostPort, fingerprint);
}

Response fetch(const Url& url, const std::string& knownHosts, std::atomic<int>* fdOut, const std::atomic<bool>* cancelled) {
    Response r;
    auto fail = [&](Response::Error e, std::string text) {
        r.error = isCancelled(cancelled) ? Response::Error::Cancelled : e;
        r.errorText = std::move(text);
        return r;
    };

    // Resolve and connect.
    addrinfo hints{}, *res = nullptr;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    std::string port = std::to_string(url.portNumber());
    std::string host = url.host;
    if (host.size() > 2 && host.front() == '[' && host.back() == ']') host = host.substr(1, host.size() - 2);
    if (int e = getaddrinfo(host.c_str(), port.c_str(), &hints, &res); e != 0 || !res)
        return fail(Response::Error::Dns, "Could not find the host " + url.host + ".");
    Socket sock;
    sock.published = fdOut;
    std::string connectError = "Could not connect to " + url.hostPort() + ".";
    for (addrinfo* a = res; a; a = a->ai_next) {
        if (isCancelled(cancelled)) break;
        int fd = ::socket(a->ai_family, a->ai_socktype, a->ai_protocol);
        if (fd < 0) continue;
        timeval tv{kTimeoutSec, 0};
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);
        setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof tv);
        if (fdOut) fdOut->store(fd);
        if (::connect(fd, a->ai_addr, a->ai_addrlen) == 0) {
            sock.fd = fd;
            break;
        }
        if (fdOut) fdOut->store(-1);
        ::close(fd);
    }
    freeaddrinfo(res);
    if (sock.fd < 0) return fail(Response::Error::Connect, connectError);

    // TLS handshake. Certificates are checked by TOFU below, not by a CA.
    Tls tls;
    const char* pers = "padshell-gemini";
    int ret = mbedtls_ctr_drbg_seed(&tls.drbg, mbedtls_entropy_func, &tls.entropy, (const unsigned char*)pers, std::strlen(pers));
    if (ret == 0)
        ret = mbedtls_ssl_config_defaults(&tls.conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
    if (ret != 0) return fail(Response::Error::Tls, "TLS setup failed: " + mbedError(ret));
    mbedtls_ssl_conf_authmode(&tls.conf, MBEDTLS_SSL_VERIFY_NONE);
    mbedtls_ssl_conf_rng(&tls.conf, mbedtls_ctr_drbg_random, &tls.drbg);
    mbedtls_ssl_conf_min_version(&tls.conf, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3);  // TLS 1.2+
    if ((ret = mbedtls_ssl_setup(&tls.ssl, &tls.conf)) != 0 || (ret = mbedtls_ssl_set_hostname(&tls.ssl, host.c_str())) != 0)
        return fail(Response::Error::Tls, "TLS setup failed: " + mbedError(ret));
    mbedtls_ssl_set_bio(&tls.ssl, &sock.fd, sendCb, recvCb, nullptr);
    while ((ret = mbedtls_ssl_handshake(&tls.ssl)) != 0) {
        if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) continue;
        if (ret == MBEDTLS_ERR_SSL_TIMEOUT) return fail(Response::Error::Timeout, "The server did not answer in time.");
        return fail(Response::Error::Tls, "TLS handshake failed: " + mbedError(ret));
    }

    // Trust on first use.
    const mbedtls_x509_crt* cert = mbedtls_ssl_get_peer_cert(&tls.ssl);
    if (!cert) return fail(Response::Error::Tls, "The server sent no certificate.");
    unsigned char digest[32];
    mbedtls_sha256_ret(cert->raw.p, cert->raw.len, digest, 0);
    r.fingerprint = hex(digest, sizeof digest);
    std::string known = knownFingerprint(knownHosts, url.hostPort());
    if (known.empty()) writeFingerprint(knownHosts, url.hostPort(), r.fingerprint);
    else if (known != r.fingerprint)
        return fail(Response::Error::CertChanged, "The certificate of " + url.hostPort() + " has changed since your last visit.");

    // Request: the absolute URL followed by CRLF.
    std::string request = url.str() + "\r\n";
    for (size_t off = 0; off < request.size();) {
        ret = mbedtls_ssl_write(&tls.ssl, (const unsigned char*)request.data() + off, request.size() - off);
        if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) continue;
        if (ret <= 0) return fail(Response::Error::Connect, "Sending the request failed: " + mbedError(ret));
        off += ret;
    }

    // Response: "<status> <meta>\r\n" then the body until the server closes.
    std::string data;
    unsigned char buf[16384];
    for (;;) {
        ret = mbedtls_ssl_read(&tls.ssl, buf, sizeof buf);
        if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) continue;
        if (ret > 0) {
            data.append((const char*)buf, ret);
            if (data.size() > kMaxBody) return fail(Response::Error::TooLarge, "The page is larger than 8 MB.");
            continue;
        }
        if (ret == 0 || ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) break;
        if (ret == MBEDTLS_ERR_NET_RECV_FAILED && !data.empty()) break;  // closed without close_notify
        if (ret == MBEDTLS_ERR_SSL_TIMEOUT) return fail(Response::Error::Timeout, "The server stopped responding.");
        if (data.empty()) return fail(Response::Error::Connect, "Reading the response failed: " + mbedError(ret));
        break;
    }
    mbedtls_ssl_close_notify(&tls.ssl);

    size_t eol = data.find("\r\n");
    if (eol == std::string::npos || eol < 2 || eol > 1026 || !std::isdigit((unsigned char)data[0]) ||
        !std::isdigit((unsigned char)data[1]))
        return fail(Response::Error::BadHeader, "The server sent an invalid response header.");
    r.status = (data[0] - '0') * 10 + (data[1] - '0');
    r.meta = data.substr(2, eol - 2);
    while (!r.meta.empty() && (r.meta[0] == ' ' || r.meta[0] == '\t')) r.meta.erase(0, 1);
    if (r.status / 10 == 2) r.body = data.substr(eol + 2);
    return r;
}

// --- Fetcher ------------------------------------------------------------------

struct Fetcher::Job {
    Url url;
    std::string knownHosts;
    std::atomic<bool> cancelled{false};
    std::atomic<bool> done{false};
    std::atomic<int> fd{-1};
    Response result;
};

namespace {
void* runJob(void* arg) {
    auto* holder = static_cast<std::shared_ptr<Fetcher::Job>*>(arg);
    std::shared_ptr<Fetcher::Job> job = *holder;
    delete holder;
    job->result = fetch(job->url, job->knownHosts, &job->fd, &job->cancelled);
    job->done.store(true);
    return nullptr;
}
}  // namespace

void Fetcher::start(const Url& url, const std::string& knownHostsPath) {
    cancel();
    job_ = std::make_shared<Job>();
    job_->url = url;
    job_->knownHosts = knownHostsPath;
    // A detached thread with a generous stack (TLS handshakes use a lot);
    // it keeps the job alive through its own shared_ptr.
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 512 * 1024);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_t thread;
    auto* holder = new std::shared_ptr<Job>(job_);
    if (pthread_create(&thread, &attr, runJob, holder) != 0) {
        delete holder;
        job_->result.error = Response::Error::Connect;
        job_->result.errorText = "Could not start the network thread.";
        job_->done.store(true);
    }
    pthread_attr_destroy(&attr);
}

std::optional<Response> Fetcher::poll() {
    if (!job_ || !job_->done.load()) return std::nullopt;
    Response r = std::move(job_->result);
    job_.reset();
    return r;
}

void Fetcher::cancel() {
    if (!job_) return;
    job_->cancelled.store(true);
    int fd = job_->fd.load();
    if (fd >= 0) ::shutdown(fd, SHUT_RDWR);  // unblocks recv/connect in the worker
    job_.reset();                             // the worker keeps its own reference
}

}  // namespace gem
