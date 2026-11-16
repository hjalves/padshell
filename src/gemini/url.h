// Minimal URL handling for Gemini: parse, resolve relative references
// (RFC 3986 section 5.2) and serialise. Fragments are dropped.
#pragma once
#include <string>
#include <string_view>

namespace gem {

struct Url {
    std::string scheme;  // lowercase, e.g. "gemini", "about"
    std::string host;    // lowercase; empty for non-hierarchical URLs
    std::string port;    // as written; empty = default
    std::string path;    // "/..." for hierarchical URLs, opaque text otherwise
    std::string query;   // without the '?'
    bool hasAuthority = false;
    bool hasQuery = false;

    static Url parse(std::string_view s);
    Url resolve(std::string_view ref) const;
    std::string str() const;
    int portNumber() const;      // default 1965
    std::string hostPort() const;  // "host:port", for known_hosts
    bool valid() const { return !scheme.empty(); }

private:
    std::string scheme_or(std::string_view schemeRelative) const;
};

// Percent-encodes everything except RFC 3986 unreserved characters.
std::string percentEncode(std::string_view s);
// Removes "." and ".." segments (RFC 3986 section 5.2.4).
std::string removeDotSegments(std::string_view path);

}  // namespace gem
