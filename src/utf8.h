// Minimal UTF-8 helpers. Text is stored as UTF-8; cursor positions are byte
// offsets that always sit on a codepoint boundary.
#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace utf8 {

inline bool isCont(unsigned char c) { return (c & 0xC0) == 0x80; }

// Byte offset of the codepoint after the one starting at i.
inline size_t next(std::string_view s, size_t i) {
    if (i >= s.size()) return s.size();
    ++i;
    while (i < s.size() && isCont((unsigned char)s[i])) ++i;
    return i;
}

// Byte offset of the codepoint before i.
inline size_t prev(std::string_view s, size_t i) {
    if (i == 0) return 0;
    --i;
    while (i > 0 && isCont((unsigned char)s[i])) --i;
    return i;
}

// Number of codepoints in s[0, bytes).
inline size_t length(std::string_view s, size_t bytes = std::string_view::npos) {
    if (bytes > s.size()) bytes = s.size();
    size_t n = 0;
    for (size_t i = 0; i < bytes; ++i)
        if (!isCont((unsigned char)s[i])) ++n;
    return n;
}

// Byte offset of codepoint index cp (clamped to end).
inline size_t offset(std::string_view s, size_t cp) {
    size_t i = 0;
    while (cp > 0 && i < s.size()) { i = next(s, i); --cp; }
    return i;
}

// Decode the codepoint at i (invalid bytes come back as U+FFFD).
inline uint32_t decode(std::string_view s, size_t i) {
    auto b = [&](size_t k) { return (uint32_t)(unsigned char)s[k]; };
    uint32_t c = b(i);
    size_t n = next(s, i) - i;
    if (c < 0x80) return c;
    if (n == 2 && (c & 0xE0) == 0xC0) return ((c & 0x1F) << 6) | (b(i + 1) & 0x3F);
    if (n == 3 && (c & 0xF0) == 0xE0)
        return ((c & 0x0F) << 12) | ((b(i + 1) & 0x3F) << 6) | (b(i + 2) & 0x3F);
    if (n == 4 && (c & 0xF8) == 0xF0)
        return ((c & 0x07) << 18) | ((b(i + 1) & 0x3F) << 12) | ((b(i + 2) & 0x3F) << 6) |
               (b(i + 3) & 0x3F);
    return 0xFFFD;
}

// Display width in monospace cells: 0 for joiners, variation selectors and
// combining marks; 2 for emoji and wide East Asian characters; else 1.
inline int width(uint32_t c) {
    if (c == 0x200D || (c >= 0xFE00 && c <= 0xFE0F) || (c >= 0x0300 && c <= 0x036F) || (c >= 0x1F3FB && c <= 0x1F3FF))
        return 0;
    if ((c >= 0x1F300 && c <= 0x1F64F) || (c >= 0x1F680 && c <= 0x1F6FF) || (c >= 0x1F900 && c <= 0x1FAFF) ||
        (c >= 0x1F1E6 && c <= 0x1F1FF) || (c >= 0x1F004 && c <= 0x1F251) || c == 0x231A || c == 0x231B ||
        (c >= 0x23E9 && c <= 0x23F3) || c == 0x2B50 || c == 0x2B55 || (c >= 0x1100 && c <= 0x115F) ||
        (c >= 0x2E80 && c <= 0xA4CF) || (c >= 0xAC00 && c <= 0xD7A3) || (c >= 0xF900 && c <= 0xFAFF) ||
        (c >= 0xFF00 && c <= 0xFF60) || (c >= 0xFFE0 && c <= 0xFFE6))
        return 2;
    return 1;
}

inline std::string encode(uint32_t c) {
    std::string out;
    if (c < 0x80) out += (char)c;
    else if (c < 0x800) { out += (char)(0xC0 | (c >> 6)); out += (char)(0x80 | (c & 0x3F)); }
    else if (c < 0x10000) {
        out += (char)(0xE0 | (c >> 12)); out += (char)(0x80 | ((c >> 6) & 0x3F));
        out += (char)(0x80 | (c & 0x3F));
    } else {
        out += (char)(0xF0 | (c >> 18)); out += (char)(0x80 | ((c >> 12) & 0x3F));
        out += (char)(0x80 | ((c >> 6) & 0x3F)); out += (char)(0x80 | (c & 0x3F));
    }
    return out;
}

}  // namespace utf8
