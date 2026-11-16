#include "gemtext.h"

namespace gem {
namespace {
std::string_view trimLeft(std::string_view s) {
    while (!s.empty() && (s[0] == ' ' || s[0] == '\t')) s.remove_prefix(1);
    return s;
}
std::string_view trimRight(std::string_view s) {
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r')) s.remove_suffix(1);
    return s;
}

template <class F>
void forEachLine(std::string_view body, F f) {
    size_t start = 0;
    while (start <= body.size()) {
        size_t nl = body.find('\n', start);
        std::string_view line = body.substr(start, nl == std::string_view::npos ? std::string_view::npos : nl - start);
        if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
        if (nl == std::string_view::npos && line.empty() && start == body.size() && start != 0) break;
        f(line);
        if (nl == std::string_view::npos) break;
        start = nl + 1;
    }
}
}  // namespace

std::vector<Line> parseGemtext(std::string_view body) {
    std::vector<Line> out;
    bool pre = false;
    forEachLine(body, [&](std::string_view l) {
        if (l.rfind("```", 0) == 0) {
            pre = !pre;  // the toggle line itself (and its alt text) is not shown
            return;
        }
        Line line;
        if (pre) {
            line.kind = Line::Pre;
            line.text = std::string(l);
        } else if (l.rfind("=>", 0) == 0) {
            std::string_view rest = trimLeft(l.substr(2));
            size_t sp = rest.find_first_of(" \t");
            line.kind = Line::Link;
            line.url = std::string(rest.substr(0, sp));
            std::string_view label = sp == std::string_view::npos ? std::string_view() : trimRight(trimLeft(rest.substr(sp)));
            line.text = label.empty() ? line.url : std::string(label);
            if (line.url.empty()) line.kind = Line::Text;  // "=>" with nothing after it
        } else if (l.rfind("###", 0) == 0) {
            line.kind = Line::H3;
            line.text = std::string(trimRight(trimLeft(l.substr(3))));
        } else if (l.rfind("##", 0) == 0) {
            line.kind = Line::H2;
            line.text = std::string(trimRight(trimLeft(l.substr(2))));
        } else if (l.rfind("#", 0) == 0) {
            line.kind = Line::H1;
            line.text = std::string(trimRight(trimLeft(l.substr(1))));
        } else if (l.rfind("* ", 0) == 0) {
            line.kind = Line::Item;
            line.text = std::string(trimRight(l.substr(2)));
        } else if (l.rfind(">", 0) == 0) {
            line.kind = Line::Quote;
            line.text = std::string(trimRight(trimLeft(l.substr(1))));
        } else {
            line.text = std::string(trimRight(l));
        }
        out.push_back(std::move(line));
    });
    return out;
}

std::vector<Line> parsePlain(std::string_view body) {
    std::vector<Line> out;
    forEachLine(body, [&](std::string_view l) { out.push_back({Line::Text, std::string(trimRight(l)), {}}); });
    return out;
}

}  // namespace gem
