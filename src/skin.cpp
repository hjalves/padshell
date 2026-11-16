#include "skin.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>

#include "utf8.h"

// --- EditorView ---------------------------------------------------------------

size_t EditorView::chars() const {
    size_t n = 0;
    for (const auto& l : *lines) n += utf8::length(l) + 1;
    return n - 1;
}

EditorView::Layout EditorView::layout(int cols) const {
    Layout L;
    L.lines = *lines;
    if (tap->hasPending()) {
        std::string& l = L.lines[cursorLine];
        l.insert(utf8::offset(l, cursorCol), tap->pendingText());
    }
    for (int i = 0; i < (int)L.lines.size(); ++i) {
        const int len = (int)utf8::length(L.lines[i]);
        int n = std::max(1, (len + cols - 1) / cols);
        if (i == cursorLine) {
            n = std::max(n, cursorCol / cols + 1);
            L.cursorRow = (int)L.rows.size() + cursorCol / cols;
            L.cursorX = cursorCol % cols;
        }
        const std::string& s = L.lines[i];
        for (int k = 0; k < n; ++k)
            L.rows.push_back({i, utf8::offset(s, (size_t)k * cols), utf8::offset(s, (size_t)(k + 1) * cols), k == 0});
    }
    return L;
}

namespace skins {

// --- helpers ------------------------------------------------------------------

std::string groupText(const MultiTap& tap, const char* chars, const char* sep) {
    std::string out;
    for (const char* c = chars; *c; ++c) {
        if (c != chars) out += sep;
        out += tap.display(*c);
    }
    return out;
}

const MultiTap::Groups* groupsFor(MultiTap::Mode m) {
    if (m == MultiTap::Mode::Commands) return nullptr;
    return m == MultiTap::Mode::Symbols ? &MultiTap::symbols() : &MultiTap::letters();
}

const MultiTap::Group* groupFor(MultiTap::Mode m, Btn b) {
    if (const auto* gs = groupsFor(m))
        for (const auto& gr : *gs)
            if (gr.button == b) return &gr;
    return nullptr;
}

const MultiTap::Command* commandFor(Btn b) {
    for (const auto& c : MultiTap::commands())
        if (c.button == b) return &c;
    return nullptr;
}

bool isDpad(Btn b) { return b == DUp || b == DDown || b == DLeft || b == DRight; }

Dir dirOf(Btn b) {
    return b == DUp ? Dir::Up : b == DDown ? Dir::Down : b == DLeft ? Dir::Left : Dir::Right;
}

int buttonLabel(Gfx& g, Font& f, int x, int y, Btn b, Color c) {
    if (isDpad(b)) {
        float s = f.lineH() * 0.5f;
        g.arrow(x + s / 2, y + f.lineH() / 2.f, s, dirOf(b), c);
        return (int)s;
    }
    return g.draw(f, x, y, btnLabel(b), c);
}

Color modeColor(MultiTap::Mode m) {
    switch (m) {
        case MultiTap::Mode::Symbols: return {96, 200, 60};
        case MultiTap::Mode::Commands: return {235, 50, 40};
        default: return {244, 114, 208};
    }
}

const char* modeName(MultiTap::Mode m, bool upper) {
    switch (m) {
        case MultiTap::Mode::Symbols: return "123";
        case MultiTap::Mode::Commands: return "edit";
        default: return upper ? "ABC" : "abc";
    }
}

}  // namespace skins
