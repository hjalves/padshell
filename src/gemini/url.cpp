#include "url.h"

#include <cctype>
#include <cstdlib>

namespace gem {
namespace {
std::string lower(std::string_view s) {
    std::string out(s);
    for (char& c : out) c = (char)std::tolower((unsigned char)c);
    return out;
}

// Splits "scheme:" off if present (scheme must start with a letter and
// precede any '/', '?' or '#').
bool splitScheme(std::string_view s, std::string_view& scheme, std::string_view& rest) {
    for (size_t i = 0; i < s.size(); ++i) {
        char c = s[i];
        if (c == ':') {
            if (i == 0) return false;
            scheme = s.substr(0, i);
            rest = s.substr(i + 1);
            return true;
        }
        bool ok = std::isalpha((unsigned char)c) || (i > 0 && (std::isdigit((unsigned char)c) || c == '+' || c == '-' || c == '.'));
        if (!ok) return false;
    }
    return false;
}
}  // namespace

std::string removeDotSegments(std::string_view in) {
    std::string input(in), out;
    while (!input.empty()) {
        if (input.rfind("../", 0) == 0) input.erase(0, 3);
        else if (input.rfind("./", 0) == 0) input.erase(0, 2);
        else if (input.rfind("/./", 0) == 0) input.replace(0, 3, "/");
        else if (input == "/.") input = "/";
        else if (input.rfind("/../", 0) == 0 || input == "/..") {
            input = input == "/.." ? "/" : input.substr(3);
            size_t slash = out.rfind('/');
            out.erase(slash == std::string::npos ? 0 : slash);
        } else if (input == "." || input == "..") input.clear();
        else {
            size_t start = input[0] == '/' ? 1 : 0;
            size_t next = input.find('/', start);
            if (next == std::string::npos) next = input.size();
            out += input.substr(0, next);
            input.erase(0, next);
        }
    }
    return out;
}

Url Url::parse(std::string_view s) {
    Url u;
    if (size_t hash = s.find('#'); hash != std::string_view::npos) s = s.substr(0, hash);
    std::string_view scheme, rest;
    if (!splitScheme(s, scheme, rest)) return u;
    u.scheme = lower(scheme);
    if (rest.rfind("//", 0) == 0) {
        u.hasAuthority = true;
        rest.remove_prefix(2);
        size_t end = rest.find_first_of("/?");
        std::string_view auth = rest.substr(0, end);
        rest = end == std::string_view::npos ? std::string_view() : rest.substr(end);
        if (size_t at = auth.rfind('@'); at != std::string_view::npos) auth = auth.substr(at + 1);  // drop userinfo
        size_t colon = auth.rfind(':');
        if (!auth.empty() && auth[0] == '[') {  // IPv6 literal
            size_t close = auth.find(']');
            colon = (close != std::string_view::npos && close + 1 < auth.size() && auth[close + 1] == ':') ? close + 1
                                                                                                          : std::string_view::npos;
        }
        if (colon != std::string_view::npos) {
            u.host = lower(auth.substr(0, colon));
            u.port = std::string(auth.substr(colon + 1));
        } else {
            u.host = lower(auth);
        }
    }
    size_t q = rest.find('?');
    u.path = std::string(rest.substr(0, q));
    if (q != std::string_view::npos) {
        u.hasQuery = true;
        u.query = std::string(rest.substr(q + 1));
    }
    if (u.hasAuthority) {
        if (u.path.empty()) u.path = "/";
        u.path = removeDotSegments(u.path);
        if (u.port == "1965" && u.scheme == "gemini") u.port.clear();
    }
    return u;
}

Url Url::resolve(std::string_view ref) const {
    if (size_t hash = ref.find('#'); hash != std::string_view::npos) ref = ref.substr(0, hash);
    std::string_view scheme, rest;
    if (splitScheme(ref, scheme, rest)) return parse(ref);
    if (ref.rfind("//", 0) == 0) return parse(scheme_or(ref));
    Url u = *this;
    if (ref.empty()) return u;
    size_t q = ref.find('?');
    std::string_view refPath = ref.substr(0, q);
    u.hasQuery = q != std::string_view::npos;
    u.query = u.hasQuery ? std::string(ref.substr(q + 1)) : std::string();
    if (refPath.empty()) {
        if (!u.hasQuery) u.hasQuery = hasQuery, u.query = query;
        return u;
    }
    if (refPath[0] == '/') {
        u.path = removeDotSegments(refPath);
    } else {
        std::string base = hasAuthority && path.empty() ? "/" : path;
        size_t slash = base.rfind('/');
        std::string merged = (slash == std::string::npos ? std::string() : base.substr(0, slash + 1)) + std::string(refPath);
        u.path = removeDotSegments(merged);
    }
    return u;
}

std::string Url::scheme_or(std::string_view schemeRelative) const { return scheme + ":" + std::string(schemeRelative); }

std::string Url::str() const {
    std::string s = scheme + ":";
    if (hasAuthority) {
        s += "//" + host;
        if (!port.empty()) s += ":" + port;
    }
    s += path;
    if (hasQuery) s += "?" + query;
    return s;
}

int Url::portNumber() const {
    int p = port.empty() ? 1965 : std::atoi(port.c_str());
    return p > 0 && p < 65536 ? p : 1965;
}

std::string Url::hostPort() const { return host + ":" + std::to_string(portNumber()); }

std::string percentEncode(std::string_view s) {
    static const char* hex = "0123456789ABCDEF";
    std::string out;
    for (unsigned char c : s) {
        if (std::isalnum(c) || c == '-' || c == '.' || c == '_' || c == '~') out += (char)c;
        else {
            out += '%';
            out += hex[c >> 4];
            out += hex[c & 15];
        }
    }
    return out;
}

}  // namespace gem
