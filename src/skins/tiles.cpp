// Tiles: a Windows 8 Start-screen take on Metro. Coloured background, the
// document on the left, and the character groups as a grid of square live
// tiles on the right: two 2x2 blocks (D-pad and face buttons, each diamond
// turned 45 degrees: up/X top-left, right/A top-right, left/Y bottom-left,
// down/B bottom-right) plus wide tiles for the modes.
#include <algorithm>
#include <cstring>

#include "../skin.h"

namespace {
using namespace skins;

constexpr Color bg{46, 20, 84}, text{255, 255, 255}, soft{255, 255, 255, 170}, idle{72, 44, 112};
constexpr int T = 128, G = 8;  // tile size and gap

Color tileColor(MultiTap::Mode m, Btn b) {
    switch (m) {
        case MultiTap::Mode::Letters:
            switch (b) {
                case DUp: return {0, 138, 0};      case DRight: return {27, 161, 226};
                case DLeft: return {0, 171, 169};  case DDown: return {0, 80, 239};
                case North: return {240, 150, 9};  case East: return {229, 20, 0};
                case West: return {250, 104, 0};   default: return {216, 0, 115};
            }
        case MultiTap::Mode::Symbols:
            switch (b) {
                case DUp: case East: return {0, 138, 0};
                case DRight: case West: return {96, 169, 23};
                case DLeft: case South: return {0, 150, 110};
                default: return {51, 153, 51};
            }
        case MultiTap::Mode::Commands:
            switch (b) {
                case South: return {229, 20, 0};  case West: return {250, 104, 0};
                case East: return {162, 0, 37};   case North: return {216, 0, 115};
                default: return idle;
            }
    }
    return idle;
}

class Tiles : public Skin {
public:
    const char* name() const override { return "tiles"; }

    Rect frame(Gfx& g, const ShellView& v) override {
        g.clear(bg);
        g.draw(g.font("DejaVuSans-ExtraLight.ttf", 64), 96, 30, v.heading, text);
        Font& c = g.font("DejaVuSans-ExtraLight.ttf", 40);
        g.draw(c, Gfx::W - 80 - Gfx::measure(c, v.clock), 44, v.clock, text);
        if (!v.toast.empty()) {
            Font& t = g.font("DejaVuSans.ttf", 20);
            int w = Gfx::measure(t, v.toast) + 28;
            g.fill({Gfx::W - 80 - Gfx::measure(c, v.clock) - 24 - w, 52, w, 36}, {216, 0, 115});
            g.draw(t, Gfx::W - 80 - Gfx::measure(c, v.clock) - 24 - w + 14, 58, v.toast, text);
        }
        return {96, 150, Gfx::W - 96 - 80, Gfx::H - 150 - 36};
    }

    void editor(Gfx& g, Rect a, const EditorView& v) override {
        const MultiTap& tap = *v.tap;
        const int gridW = 4 * T + 3 * G + 40;  // two 2x2 blocks with a gutter
        const int gx = a.x + a.w - gridW;
        const Color mc = modeColor(v.mode());

        // Document column.
        Font& f = g.font("DejaVuSansMono.ttf", 28);
        Rect ta{a.x, a.y, gx - 48 - a.x, 2 * T + G + T + G};
        const int cols = std::max(1, ta.w / f.cellW()), visible = std::max(1, ta.h / f.lineH());
        const auto rows = v.wrap(cols);
        const int top = std::max(0, (int)rows.size() - visible);
        for (int r = top; r < (int)rows.size() && r < top + visible; ++r)
            g.print(f, ta.x, ta.y + (r - top) * f.lineH(), v.rowText(rows[r]), text);
        if (v.empty() && !tap.hasPending())
            g.draw(g.font("DejaVuSans-ExtraLight.ttf", 30), ta.x + f.cellW() + 10, ta.y - 2, "start typing", soft);
        int cx = ta.x + (v.endColumn() % cols) * f.cellW();
        int cy = ta.y + ((int)rows.size() - 1 - top) * f.lineH();
        if (tap.hasPending()) {
            g.fill({cx, cy, f.cellW(), f.lineH()}, mc);
            g.print(f, cx, cy, tap.pendingText(), bg);
        }
        g.fill({cx, cy + f.lineH() - 5, f.cellW(), 5}, mc);
        Font& meta = g.font("DejaVuSans.ttf", 19);
        std::string info = std::to_string(v.lineCount()) + " lines  \xC2\xB7  " + std::to_string(v.chars()) +
                           " characters" + (v.modified ? "  \xC2\xB7  unsaved" : "");
        g.draw(meta, a.x, a.y + a.h - 28, info, soft);

        // Live tiles: [up/X  right/A] / [left/Y  down/B].
        const Btn dpad[4] = {DUp, DRight, DLeft, DDown}, face[4] = {North, East, West, South};
        for (int i = 0; i < 4; ++i) {
            tile(g, v, dpad[i], {gx + (i % 2) * (T + G), a.y + (i / 2) * (T + G), T, T});
            tile(g, v, face[i], {gx + 2 * (T + G) + 40 + (i % 2) * (T + G), a.y + (i / 2) * (T + G), T, T});
        }
        // Mode tiles.
        int my = a.y + 2 * (T + G);
        modeTile(g, {gx, my, 2 * T + G, T}, std::string(btnLabel(ZL)), "symbols", "123",
                 v.mode() == MultiTap::Mode::Symbols, {0, 138, 0});
        modeTile(g, {gx + 2 * (T + G) + 40, my, T, T}, btnLabel(ZR), "edit", "\xE2\x8C\xAB",
                 v.mode() == MultiTap::Mode::Commands, {229, 20, 0});
        modeTile(g, {gx + 3 * (T + G) + 40, my, T, T}, btnLabel(L), "caps", "Aa", tap.uppercase(), {216, 0, 115});
        Font& h = g.font("DejaVuSans.ttf", 19);
        std::string hint = "+ menu    - apps";
        g.draw(h, gx + gridW - Gfx::measure(h, hint), my + T + 14, hint, soft);
    }

    void menu(Gfx& g, const std::string& title, const std::vector<std::string>& items, int sel) override {
        // A charms-style panel from the right edge.
        g.fill({0, 0, Gfx::W, Gfx::H}, {0, 0, 0, 120});
        const int w = 400, x = Gfx::W - w;
        g.fill({x, 0, w, Gfx::H}, {17, 17, 17, 250});
        g.draw(g.font("DejaVuSans-ExtraLight.ttf", 44), x + 40, 60, title, text);
        Font& f = g.font("DejaVuSans-ExtraLight.ttf", 34);
        int y = 160;
        for (int i = 0; i < (int)items.size(); ++i) {
            if (i == sel) g.fill({x, y - 6, w, f.lineH() + 12}, Color{120, 60, 200});
            g.draw(f, x + 40, y, items[i], text);
            y += f.lineH() + 18;
        }
        g.draw(g.font("DejaVuSans.ttf", 19), x + 40, Gfx::H - 70,
               std::string(btnLabel(kConfirm)) + " select    " + btnLabel(kBack) + " back", soft);
    }

private:
    void tile(Gfx& g, const EditorView& v, Btn b, Rect t) {
        const MultiTap& tap = *v.tap;
        const bool held = v.held & b;
        if (held) t = {t.x + 6, t.y + 6, t.w - 12, t.h - 12};  // pressed tiles sink
        Color c = tileColor(v.mode(), b);
        g.fill(t, c);
        Font& lab = g.font("DejaVuSans.ttf", 17);
        int lw = buttonLabel(g, lab, t.x + 10, t.y + t.h - lab.lineH() - 6, b, soft);

        if (v.mode() == MultiTap::Mode::Commands) {
            if (const auto* cmd = commandFor(b)) {
                g.draw(lab, t.x + 10 + lw + 8, t.y + t.h - lab.lineH() - 6, cmd->label, text);
                const char* icon = cmd->kind == TextEvent::Newline ? "\xE2\x86\xB5" : cmd->kind == TextEvent::Backspace
                                       ? "\xE2\x8C\xAB" : cmd->kind == TextEvent::DeleteLine ? "\xE2\x9C\x95" : "\xE2\x90\xA3";
                g.drawCentered(g.font("DejaVuSans.ttf", 46), t.x + t.w / 2, t.y + 22, icon, text);
            }
            return;
        }
        const auto* grp = groupFor(v.mode(), b);
        if (!grp) return;
        int n = (int)std::strlen(grp->chars);
        int cur = tap.hasPending() && tap.pendingGroup() == grp->chars ? tap.pendingIndex() : -1;
        if (cur >= 0) {
            // Live tile: the character being selected, big, over its group.
            g.drawCentered(g.font("DejaVuSans-ExtraLight.ttf", 72), t.x + t.w / 2, t.y + 14,
                           tap.display(grp->chars[cur]), text);
            Font& sf = g.font("DejaVuSansMono.ttf", 15);
            int gx = t.x + t.w - 10 - n * sf.cellW();
            for (int k = 0; k < n; ++k)
                g.print(sf, gx + k * sf.cellW(), t.y + t.h - sf.lineH() - 8, tap.display(grp->chars[k]),
                        k == cur ? text : Color{255, 255, 255, 120});
            return;
        }
        Font& cf = g.font("DejaVuSans-ExtraLight.ttf", n > 5 ? 28 : 40);
        int perLine = n > 5 ? (n + 1) / 2 : n;
        std::string l1 = groupText(tap, grp->chars).substr(0, perLine), l2 = groupText(tap, grp->chars).substr(perLine);
        int y = t.y + (n > 5 ? 14 : 22);
        g.drawCentered(cf, t.x + t.w / 2, y, l1, text);
        if (!l2.empty()) g.drawCentered(cf, t.x + t.w / 2, y + cf.lineH() - 4, l2, text);
    }

    void modeTile(Gfx& g, Rect t, const std::string& btn, const char* what, const char* icon, bool on, Color c) {
        g.fill(t, on ? c : idle);
        Font& lab = g.font("DejaVuSans.ttf", 17);
        g.draw(lab, t.x + 10, t.y + t.h - lab.lineH() - 6, btn + "  " + what, on ? text : soft);
        // Regular weight: the ExtraLight face lacks symbols such as the erase key.
        g.drawCentered(g.font("DejaVuSans.ttf", 40), t.x + t.w / 2, t.y + 24, icon, on ? text : soft);
    }
};
}  // namespace

Skin* makeTiles() { static Tiles s; return &s; }
