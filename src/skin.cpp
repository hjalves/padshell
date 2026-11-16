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

int EditorView::endColumn() const { return (int)utf8::length(lines->back()); }

std::vector<WrapRow> EditorView::wrap(int cols) const {
    const int last = (int)lines->size() - 1;
    std::vector<WrapRow> rows;
    for (int i = 0; i <= last; ++i) {
        const std::string& s = (*lines)[i];
        int len = (int)utf8::length(s);
        int n = i == last ? len / cols + 1 : std::max(1, (len + cols - 1) / cols);
        for (int k = 0; k < n; ++k)
            rows.push_back({i, utf8::offset(s, (size_t)k * cols), utf8::offset(s, (size_t)(k + 1) * cols), k == 0});
    }
    return rows;
}

std::string_view EditorView::rowText(const WrapRow& r) const {
    return std::string_view((*lines)[r.line]).substr(r.from, r.to - r.from);
}

// --- registry -----------------------------------------------------------------

// Defined in src/skins/*.cpp.
Skin* makeMetro();
Skin* makeArcade();
Skin* makeZune();
Skin* makeTiles();
Skin* makeSwiss();
Skin* makeGameBoy();
Skin* makeSynthwave();
Skin* makeXmb();

namespace skins {
namespace {
int g_index = 0;  // metro
std::string g_path;
}  // namespace

const std::vector<Skin*>& all() {
    static const std::vector<Skin*> list = {makeMetro(),  makeArcade(),    makeZune(), makeTiles(),
                                            makeSwiss(),  makeGameBoy(),   makeSynthwave(), makeXmb()};
    return list;
}

Skin& current() { return *all()[g_index]; }
int index() { return g_index; }

void select(int i) {
    const int n = (int)all().size();
    g_index = ((i % n) + n) % n;
    if (g_path.empty()) return;
    if (FILE* f = std::fopen(g_path.c_str(), "w")) {
        std::fprintf(f, "style=%s\n", current().name());
        std::fclose(f);
    }
}

void loadSettings(const std::string& path) {
    g_path = path;
    FILE* f = std::fopen(path.c_str(), "r");
    if (!f) return;  // first run: keep the default
    char line[128];
    while (std::fgets(line, sizeof line, f)) {
        line[std::strcspn(line, "\r\n")] = 0;
        if (std::strncmp(line, "style=", 6) != 0) continue;
        for (size_t i = 0; i < all().size(); ++i)
            if (std::strcmp(line + 6, all()[i]->name()) == 0) g_index = (int)i;
    }
    std::fclose(f);
}

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
