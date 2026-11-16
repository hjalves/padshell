// Arcade: genuine low resolution. Everything is drawn at 320x180 into a
// texture with the Press Start 2P pixel font (no anti-aliasing) and a
// PICO-8-style palette, then scaled 4x with nearest-neighbour filtering.
#include <algorithm>
#include <cctype>
#include <cstring>

#include "pixel.h"

namespace {
using namespace skins;

constexpr int LW = 320, LH = 180;
constexpr Color black{0, 0, 0}, navy{29, 43, 83}, plum{126, 37, 83}, forest{0, 135, 81};
constexpr Color brown{171, 82, 54}, slate{95, 87, 79}, silver{194, 195, 199}, white{255, 241, 232};
constexpr Color red{255, 0, 77}, orange{255, 163, 0}, yellow{255, 236, 39}, green{0, 228, 54};
constexpr Color blue{41, 173, 255}, lavender{131, 118, 156}, pink{255, 119, 168}, peach{255, 204, 170};

Color modeCol(MultiTap::Mode m) {
    return m == MultiTap::Mode::Symbols ? green : m == MultiTap::Mode::Commands ? red : pink;
}

Color keyColor(MultiTap::Mode m, Btn b) {
    switch (m) {
        case MultiTap::Mode::Letters:
            switch (b) {
                case DLeft: return blue;   case DUp: return forest;  case DDown: return orange;
                case DRight: return lavender; case West: return brown; case North: return plum;
                case South: return red;    default: return pink;
            }
        case MultiTap::Mode::Symbols: return (b == DUp || b == DRight || b == West || b == South) ? forest : Color{0, 160, 100};
        case MultiTap::Mode::Commands:
            switch (b) {
                case South: return red; case West: return orange; case East: return brown; case North: return plum;
                default: return slate;
            }
    }
    return slate;
}

class Arcade : public PixelSkin {
public:
    Arcade() : PixelSkin(LW, LH) {}
    const char* name() const override { return "arcade"; }

    Rect frame(Gfx& g, const ShellView& v) override {
        begin(g);
        g.clear(navy);
        Font& f = font(g);
        shadowText(g, 6, 5, upper(v.heading), yellow);
        std::string right = v.toast.empty() ? v.clock : upper(v.toast);
        shadowText(g, LW - 6 - Gfx::width(f, right), 5, right, v.toast.empty() ? white : orange);
        return {4, 17, LW - 8, LH - 17};
    }

    void editor(Gfx& g, Rect a, const EditorView& v) override {
        const MultiTap& tap = *v.tap;
        Font& f = font(g);
        const int lh = f.lineH() + 2;
        Rect box{a.x, a.y, a.w, 104};
        bevelBox(g, box, black);
        Rect ta{box.x + 4, box.y + 5, box.w - 8, box.h - 8};
        const int cols = std::max(1, ta.w / f.cellW()), visible = std::max(1, ta.h / lh);
        const auto rows = v.wrap(cols);
        const int top = std::max(0, (int)rows.size() - visible);
        for (int r = top; r < (int)rows.size() && r < top + visible; ++r)
            g.print(f, ta.x, ta.y + (r - top) * lh, v.rowText(rows[r]), white);
        if (v.empty() && !tap.hasPending() && blinkOn())
            g.printCentered(f, box.x + box.w / 2, box.y + box.h / 2 - 4, "PRESS A BUTTON!", yellow);

        int cx = ta.x + (v.endColumn() % cols) * f.cellW();
        int cy = ta.y + ((int)rows.size() - 1 - top) * lh;
        Color mc = modeCol(v.mode());
        if (tap.hasPending()) {
            g.fill({cx, cy - 1, f.cellW(), lh}, mc);
            g.print(f, cx, cy, tap.pendingText(), black);
        } else if (blinkOn()) {
            g.fill({cx, cy - 1, f.cellW(), lh}, mc);
        }

        // Status strip.
        int sy = box.y + box.h + 3;
        std::string mode = upper(modeName(v.mode(), tap.uppercase()));
        int x = a.x + 2;
        g.fill({x - 1, sy - 1, Gfx::width(f, mode) + 2, f.lineH() + 1}, mc);
        g.print(f, x, sy, mode, black);
        x += Gfx::width(f, mode) + 8;
        std::string info = "CHR " + pad3(v.chars()) + " LN " + pad3(v.lineCount()) + (v.modified ? " *" : "");
        g.print(f, x, sy, info, peach);
        g.print(f, a.x + a.w - Gfx::width(f, "+MENU") - 2, sy, "+MENU", silver);

        // Buttons.
        const Btn order[8] = {DLeft, DUp, DDown, DRight, West, North, South, East};
        const int bw = 36, bh = 36, by = LH - bh - 3;
        const int span = 8 * bw + 6 * 2 + 10;
        const int x0 = (LW - span) / 2;
        for (int i = 0; i < 8; ++i) key(g, v, order[i], x0 + i * (bw + 2) + (i >= 4 ? 8 : 0), by, bw, bh);
    }

    void menu(Gfx& g, const std::string& title, const std::vector<std::string>& items, int sel) override {
        Font& f = font(g);
        g.fill({0, 0, LW, LH}, {0, 0, 0, 150});
        int w = 180, h = (int)items.size() * 14 + 46;
        Rect box{(LW - w) / 2, (LH - h) / 2, w, h};
        bevelBox(g, box, navy);
        g.printCentered(f, box.x + w / 2, box.y + 7, "- " + upper(title) + " -", yellow);
        int y = box.y + 22;
        for (int i = 0; i < (int)items.size(); ++i) {
            if (i == sel && blinkOn()) g.print(f, box.x + 8, y, ">", yellow);
            g.print(f, box.x + 20, y, upper(items[i]), i == sel ? white : lavender);
            y += 14;
        }
        std::string hint = std::string(btnLabel(kConfirm)) + ":OK " + btnLabel(kBack) + ":BACK";
        g.printCentered(f, box.x + w / 2, box.y + h - 12, hint, silver);
    }

private:

    void shadowText(Gfx& g, int x, int y, const std::string& s, Color c) {
        g.print(font(g), x + 1, y + 1, s, black);
        g.print(font(g), x, y, s, c);
    }

    // A box with a 1px light top-left edge and dark bottom-right edge.
    void bevelBox(Gfx& g, Rect r, Color fill) {
        g.fill(r, white);
        g.fill({r.x + 1, r.y + 1, r.w - 1, r.h - 1}, lavender);
        g.fill({r.x + 1, r.y + 1, r.w - 2, r.h - 2}, fill);
    }

    void key(Gfx& g, const EditorView& v, Btn b, int x, int y, int w, int h) {
        const MultiTap& tap = *v.tap;
        Font& f = font(g);
        const bool held = v.held & b;
        Color c = keyColor(v.mode(), b);
        if (held) y += 1;
        g.fill({x + 1, y + 1, w, h}, black);  // drop shadow
        g.fill({x, y, w, h}, c);
        if (!held) {
            g.fill({x, y, w, 1}, mix(c, white, 0.5f));
            g.fill({x, y, 1, h}, mix(c, white, 0.5f));
            g.fill({x, y + h - 1, w, 1}, mix(c, black, 0.4f));
            g.fill({x + w - 1, y, 1, h}, mix(c, black, 0.4f));
        }
        g.print(f, x + 2, y + 2, keyGlyph(b), withAlpha(white, 200));

        if (v.mode() == MultiTap::Mode::Commands) {
            if (const auto* cmd = commandFor(b)) {
                const char* abbr = cmd->kind == TextEvent::Newline ? "RET" : cmd->kind == TextEvent::Backspace ? "BS"
                                 : cmd->kind == TextEvent::DeleteLine ? "DEL" : "SPC";
                g.printCentered(f, x + w / 2, y + 19, abbr, white);
            }
            return;
        }
        const auto* grp = groupFor(v.mode(), b);
        if (!grp) return;
        int n = (int)std::strlen(grp->chars);
        int cur = tap.hasPending() && tap.pendingGroup() == grp->chars ? tap.pendingIndex() : -1;
        for (int k = 0; k < n; ++k) {
            int lx = x + 2 + (k % 4) * f.cellW(), ly = y + 11 + (k / 4) * 8;  // 8px pixel-font rows
            if (k == cur) g.fill({lx, ly - 1, f.cellW(), 9}, white);
            g.print(f, lx, ly, tap.display(grp->chars[k]), k == cur ? black : white);
        }
    }
};
}  // namespace

Skin* makeArcade() { static Arcade s; return &s; }
