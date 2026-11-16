// Gemtext (text/gemini) parsing: one entry per source line.
#pragma once
#include <string>
#include <string_view>
#include <vector>

namespace gem {

struct Line {
    enum Kind { Text, Link, H1, H2, H3, Item, Quote, Pre };
    Kind kind = Text;
    std::string text;  // display text (link: label, or the URL if none)
    std::string url;   // links only, as written (resolve against the page URL)
};

std::vector<Line> parseGemtext(std::string_view body);
// Plain text: every line as Text, no markup.
std::vector<Line> parsePlain(std::string_view body);

}  // namespace gem
