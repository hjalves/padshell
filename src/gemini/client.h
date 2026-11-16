// Gemini requests over TLS (mbedtls 2.28), run on a background thread so
// the UI never blocks.
//
// Certificates use trust on first use (TOFU), as is usual for Gemini: the
// SHA-256 of a host's certificate is remembered in a known_hosts file the
// first time it is seen, and a later mismatch is reported instead of
// connecting. CA validation and client certificates are not implemented.
#pragma once
#include <atomic>
#include <memory>
#include <optional>
#include <string>

#include "url.h"

namespace gem {

struct Response {
    enum class Error { None, Dns, Connect, Tls, Timeout, BadHeader, TooLarge, CertChanged, Cancelled };
    Error error = Error::None;
    std::string errorText;    // human-readable, for error pages
    int status = 0;           // two-digit Gemini status
    std::string meta;         // MIME type, prompt, redirect target or error text
    std::string body;         // only for 2x
    std::string fingerprint;  // server certificate SHA-256 (hex)
};

constexpr size_t kMaxBody = 8 * 1024 * 1024;

// Blocking fetch of one URL (no redirects followed).
Response fetch(const Url& url, const std::string& knownHostsPath, std::atomic<int>* fdOut = nullptr,
               const std::atomic<bool>* cancelled = nullptr);

// Replaces the remembered certificate for host:port.
void trustCertificate(const std::string& knownHostsPath, const std::string& hostPort, const std::string& fingerprint);

// Runs requests on one long-lived worker thread, one at a time, so network
// calls never overlap and no thread is created per request. start()
// replaces whatever is queued and abandons the request in flight (its
// result is discarded).
class Fetcher {
public:
    Fetcher();
    ~Fetcher();
    Fetcher(const Fetcher&) = delete;
    Fetcher& operator=(const Fetcher&) = delete;

    void start(const Url& url, const std::string& knownHostsPath);
    bool busy() const;
    std::optional<Response> poll();  // the result, once, when done
    void cancel();

    struct Shared;  // state shared with the worker thread

private:
    std::shared_ptr<Shared> s_;
};

}  // namespace gem
