#include "ui.h"

#include <cstdio>
#include <cstring>

#include "pad.h"

namespace ui {
namespace {
Style g_style = Style::Metro;
std::string g_path;

void save() {
    if (g_path.empty()) return;
    if (FILE* f = std::fopen(g_path.c_str(), "w")) {
        std::fprintf(f, "style=%s\n", styleName(g_style));
        std::fclose(f);
    }
}
}  // namespace

Style style() { return g_style; }

void setStyle(Style s) {
    g_style = s;
    save();
}

const char* styleName(Style s) { return s == Style::Metro ? "metro" : "classic"; }

void loadSettings(const std::string& path) {
    g_path = path;
    FILE* f = std::fopen(path.c_str(), "r");
    if (!f) return;  // first run: keep defaults
    char line[128];
    while (std::fgets(line, sizeof line, f)) {
        line[std::strcspn(line, "\r\n")] = 0;
        if (std::strcmp(line, "style=classic") == 0) g_style = Style::Classic;
        if (std::strcmp(line, "style=metro") == 0) g_style = Style::Metro;
    }
    std::fclose(f);
}

namespace metro {
void drawMenu(Gfx& g, const char* header, const std::vector<std::string>& items, int sel, Color accent) {
    g.fill({0, 0, Gfx::W, Gfx::H}, {0, 0, 0, 245});
    int x = kMarginX, y = kMarginY + 64;
    g.draw(g.sans(), x, y, header, accent);
    y += 56;
    for (int i = 0; i < (int)items.size(); ++i) {
        bool on = i == sel;
        if (on) g.fill({x, y + 12, 8, g.lightMid().lineH() - 18}, accent);
        g.draw(g.lightMid(), x + 28, y, items[i], on ? text : dim);
        y += g.lightMid().lineH() + 10;
    }
    std::string hint = std::string(btnLabel(kConfirm)) + "  select      " + btnLabel(kBack) + "  cancel";
    g.draw(g.sans(), x, Gfx::H - kMarginY - g.sans().lineH(), hint, dim);
}
}  // namespace metro

}  // namespace ui
