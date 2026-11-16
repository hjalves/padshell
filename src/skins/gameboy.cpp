// Game Boy: the 4-shade green of the original DMG screen. 320x180 pixel art
// with window boxes like a classic handheld RPG, inverse video for pressed
// keys and the letter being picked, and a dot-matrix LCD grid on top.
#include <algorithm>
#include <cstring>

#include "pixel.h"

namespace {
using namespace skins;

constexpr int LW = 320, LH = 180;
constexpr Color c0{15, 56, 15}, c1{48, 98, 48}, c2{139, 172, 15}, c3{155, 188, 15};

class GameBoy : public PixelSkin {
public:
    GameBoy() : PixelSkin(LW, LH) {}
    const char* name() const override { return "gameboy"; }

    Rect frame(Gfx& g, const ShellView& v) override {
        begin(g);
        g.clear(c3);
        Font& f = font(g);
        g.print(f, 6, 4, upper(v.heading), c0);
        std::string right = v.toast.empty() ? v.clock : upper(v.toast);
        g.print(f, LW - 6 - Gfx::width(f, right), 4, right, v.toast.empty() ? c1 : c0);
        return {4, 15, LW - 8, LH - 15};
    }

    void editor(Gfx& g, Rect a, const EditorView& v) override {
        const MultiTap& tap = *v.tap;
        Font& f = font(g);
        const int lh = 10;
        Rect box{a.x, a.y, a.w, 100};
        window(g, box);
        Rect ta{box.x + 6, box.y + 6, box.w - 12, box.h - 10};
        const int cols = std::max(1, ta.w / f.cellW()), visible = std::max(1, ta.h / lh);
        const auto rows = v.wrap(cols);
        const int top = std::max(0, (int)rows.size() - visible);
        for (int r = top; r < (int)rows.size() && r < top + visible; ++r)
            g.print(f, ta.x, ta.y + (r - top) * lh, v.rowText(rows[r]), c0);
        int cx = ta.x + (v.endColumn() % cols) * f.cellW();
        int cy = ta.y + ((int)rows.size() - 1 - top) * lh;
        if (tap.hasPending()) {
            g.fill({cx, cy - 1, f.cellW(), 9}, c0);
            g.print(f, cx, cy, tap.pendingText(), c3);
        } else if (blinkOn()) {
            // Mode shows in the cursor shape: block, half block, or underline.
            int h = v.mode() == MultiTap::Mode::Letters ? 9 : v.mode() == MultiTap::Mode::Symbols ? 5 : 2;
            g.fill({cx, cy + 8 - h, f.cellW(), h}, c0);
        }
        if (v.empty() && !tap.hasPending()) g.print(f, ta.x + 16, ta.y, "PRESS ANY KEY", c1);

        // Status line: mode badge, counters.
        int sy = box.y + box.h + 4;
        std::string mode = upper(modeName(v.mode(), tap.uppercase()));
        g.fill({a.x, sy - 1, Gfx::width(f, mode) + 4, 10}, c0);
        g.print(f, a.x + 2, sy, mode, c3);
        std::string info = "CHR" + pad3(v.chars()) + (v.modified ? " *" : "");
        g.print(f, a.x + Gfx::width(f, mode) + 10, sy, info, c0);
        g.print(f, a.x + a.w - Gfx::width(f, "+MENU"), sy, "+MENU", c1);

        const Btn order[8] = {DLeft, DUp, DDown, DRight, West, North, South, East};
        const int bw = 36, by = LH - 39, x0 = (LW - (8 * bw + 6 * 2 + 10)) / 2;
        for (int i = 0; i < 8; ++i) key(g, v, order[i], {x0 + i * (bw + 2) + (i >= 4 ? 8 : 0), by, bw, 36});
    }

    void menu(Gfx& g, const std::string& title, const std::vector<std::string>& items, int sel) override {
        Font& f = font(g);
        int w = 170, h = (int)items.size() * 14 + 42;
        Rect box{(LW - w) / 2, (LH - h) / 2, w, h};
        window(g, box);
        g.print(f, box.x + 10, box.y + 8, upper(title), c1);
        int y = box.y + 22;
        for (int i = 0; i < (int)items.size(); ++i) {
            if (i == sel) g.print(f, box.x + 8, y, "\xE2\x96\xB6", c0);
            g.print(f, box.x + 20, y, upper(items[i]), c0);
            y += 14;
        }
        std::string hint = std::string(btnLabel(kConfirm)) + ":OK " + btnLabel(kBack) + ":BACK";
        g.print(f, box.x + 10, box.y + h - 12, hint, c1);
    }

protected:
    // Dot-matrix LCD: faint light gaps between the big pixels.
    void afterUpscale(Gfx& g) override {
        const int s = Gfx::W / LW;
        const Color gap{190, 215, 110, 34};
        for (int x = 0; x < Gfx::W; x += s) g.fill({x, 0, 1, Gfx::H}, gap);
        for (int y = 0; y < Gfx::H; y += s) g.fill({0, y, Gfx::W, 1}, gap);
    }

private:
    // RPG-style window: double border with clipped corners.
    void window(Gfx& g, Rect r) {
        g.fill(r, c0);
        g.fill({r.x + 1, r.y + 1, r.w - 2, r.h - 2}, c3);
        g.fill({r.x + 2, r.y + 2, r.w - 4, r.h - 4}, c1);
        g.fill({r.x + 3, r.y + 3, r.w - 6, r.h - 6}, c3);
        for (int cx : {r.x, r.x + r.w - 1})
            for (int cy : {r.y, r.y + r.h - 1}) g.fill({cx, cy, 1, 1}, c3);
    }

    void key(Gfx& g, const EditorView& v, Btn b, Rect r) {
        const MultiTap& tap = *v.tap;
        Font& f = font(g);
        const bool held = v.held & b;
        g.fill(r, c0);  // keys use a single border so 4 characters fit per row
        if (!held) g.fill({r.x + 1, r.y + 1, r.w - 2, r.h - 2}, c3);
        const Color fg = held ? c3 : c0;
        g.print(f, r.x + 2, r.y + 2, keyGlyph(b), held ? c2 : c1);
        if (v.mode() == MultiTap::Mode::Commands) {
            if (const auto* cmd = commandFor(b)) {
                const char* abbr = cmd->kind == TextEvent::Newline ? "RET" : cmd->kind == TextEvent::Backspace ? "BS"
                                 : cmd->kind == TextEvent::DeleteLine ? "DEL" : "SPC";
                g.printCentered(f, r.x + r.w / 2, r.y + 20, abbr, fg);
            }
            return;
        }
        const auto* grp = groupFor(v.mode(), b);
        if (!grp) return;
        int n = (int)std::strlen(grp->chars);
        int cur = tap.hasPending() && tap.pendingGroup() == grp->chars ? tap.pendingIndex() : -1;
        for (int k = 0; k < n; ++k) {
            int lx = r.x + 2 + (k % 4) * f.cellW(), ly = r.y + 11 + (k / 4) * 8;
            if (k == cur) g.fill({lx, ly - 1, f.cellW(), 9}, held ? c3 : c0);
            g.print(f, lx, ly, tap.display(grp->chars[k]), k == cur ? (held ? c0 : c3) : fg);
        }
    }
};
}  // namespace

Skin* makeGameBoy() { static GameBoy s; return &s; }
