// Synthwave: an 80s neon sunset in 320x180 pixel art. Gradient sky, a striped
// sun sinking behind the document, a perspective grid floor scrolling towards
// the viewer, and neon-outlined keys.
#include <algorithm>
#include <cmath>
#include <cstring>

#include "pixel.h"

namespace {
using namespace skins;

constexpr int LW = 320, LH = 180, kHorizon = 120;
constexpr Color night{14, 6, 36}, dusk{88, 20, 110}, glow{255, 80, 140}, floorc{18, 4, 36};
constexpr Color grid{255, 40, 190}, cyan{0, 235, 255}, yellow{255, 228, 70}, pink{255, 60, 170}, white{255, 240, 250};
constexpr Color panel{10, 4, 30, 215};

Color neon(MultiTap::Mode m) {
    return m == MultiTap::Mode::Symbols ? Color{70, 255, 140} : m == MultiTap::Mode::Commands ? Color{255, 60, 60} : pink;
}

class Synthwave : public PixelSkin {
public:
    Synthwave() : PixelSkin(LW, LH) {}
    const char* name() const override { return "synthwave"; }

    Rect frame(Gfx& g, const ShellView& v) override {
        begin(g);
        const float t = SDL_GetTicks() / 1000.0f;
        g.gradient({0, 0, LW, kHorizon}, night, night, dusk, dusk);
        g.gradient({0, kHorizon - 30, LW, 30}, {0, 0, 0, 0}, {0, 0, 0, 0}, withAlpha(glow, 120), withAlpha(glow, 120));

        // Striped sun: rows lerp yellow to pink, with gaps that widen downwards.
        const int sx = LW / 2, sy = kHorizon - 6, r = 46;
        for (int y = -r; y <= 0; ++y) {
            int py = sy + y;
            float k = (float)(y + r) / r;
            if (k > 0.45f && ((py / 2) % (k > 0.8f ? 2 : 3) == 0)) continue;
            int half = (int)std::sqrt((float)(r * r - y * y));
            g.fill({sx - half, py, 2 * half, 1}, mix(yellow, pink, k));
        }

        // Floor with a scrolling perspective grid.
        g.fill({0, kHorizon, LW, LH - kHorizon}, floorc);
        SDL_Renderer* sr = g.sdl();
        SDL_SetRenderDrawColor(sr, grid.r, grid.g, grid.b, 200);
        SDL_RenderDrawLine(sr, 0, kHorizon, LW, kHorizon);
        for (int i = -12; i <= 12; ++i)
            SDL_RenderDrawLine(sr, LW / 2 + i * 10, kHorizon, LW / 2 + i * 70, LH);
        float phase = std::fmod(t * 0.6f, 1.0f);
        for (int k = 0; k < 8; ++k) {
            float d = (k + phase) / 8.0f;  // 0 at horizon .. 1 at bottom
            int y = kHorizon + (int)(d * d * (LH - kHorizon));
            SDL_SetRenderDrawColor(sr, grid.r, grid.g, grid.b, (Uint8)(80 + 150 * d));
            SDL_RenderDrawLine(sr, 0, y, LW, y);
        }

        Font& f = font(g);
        std::string h = upper(v.heading);
        g.print(f, 7, 5, h, pink);
        g.print(f, 6, 4, h, yellow);
        std::string right = v.toast.empty() ? v.clock : upper(v.toast);
        g.print(f, LW - 6 - Gfx::width(f, right), 4, right, v.toast.empty() ? cyan : yellow);
        return {6, 16, LW - 12, LH - 16};
    }

    void editor(Gfx& g, Rect a, const EditorView& v) override {
        const MultiTap& tap = *v.tap;
        Font& f = font(g);
        const int lh = 10;
        Rect box{a.x, a.y, a.w, 90};
        g.fill(box, panel);
        g.outline(box, cyan);
        Rect ta{box.x + 5, box.y + 5, box.w - 10, box.h - 8};
        const int cols = std::max(1, ta.w / f.cellW()), visible = std::max(1, ta.h / lh);
        const auto rows = v.wrap(cols);
        const int top = std::max(0, (int)rows.size() - visible);
        for (int r = top; r < (int)rows.size() && r < top + visible; ++r)
            g.print(f, ta.x, ta.y + (r - top) * lh, v.rowText(rows[r]), white);
        if (v.empty() && !tap.hasPending() && blinkOn())
            g.printCentered(f, box.x + box.w / 2, box.y + 40, "INSERT COIN", yellow);
        int cx = ta.x + (v.endColumn() % cols) * f.cellW();
        int cy = ta.y + ((int)rows.size() - 1 - top) * lh;
        Color mc = neon(v.mode());
        if (tap.hasPending()) {
            g.fill({cx, cy - 1, f.cellW(), 9}, mc);
            g.print(f, cx, cy, tap.pendingText(), night);
        } else if (blinkOn()) {
            g.fill({cx, cy - 1, f.cellW(), 9}, mc);
        }

        int sy = box.y + box.h + 4;
        g.fill({a.x, sy - 2, a.w, 12}, {10, 4, 30, 170});
        std::string mode = upper(modeName(v.mode(), tap.uppercase()));
        g.print(f, a.x + 2, sy, mode, mc);
        g.print(f, a.x + 2 + Gfx::width(f, mode) + 8, sy, "CHR" + pad3(v.chars()) + (v.modified ? "*" : ""), white);
        g.print(f, a.x + a.w - Gfx::width(f, "+MENU") - 2, sy, "+MENU", cyan);

        const Btn order[8] = {DLeft, DUp, DDown, DRight, West, North, South, East};
        const int bw = 36, by = LH - 39, x0 = (LW - (8 * bw + 6 * 2 + 10)) / 2;
        for (int i = 0; i < 8; ++i)
            key(g, v, order[i], {x0 + i * (bw + 2) + (i >= 4 ? 8 : 0), by, bw, 36}, i < 4 ? cyan : pink);
    }

    void menu(Gfx& g, const std::string& title, const std::vector<std::string>& items, int sel) override {
        Font& f = font(g);
        g.fill({0, 0, LW, LH}, {0, 0, 0, 120});
        int w = 176, h = (int)items.size() * 14 + 44;
        Rect box{(LW - w) / 2, (LH - h) / 2, w, h};
        g.fill(box, {10, 4, 30, 240});
        g.outline(box, pink);
        g.outline({box.x + 2, box.y + 2, box.w - 4, box.h - 4}, cyan);
        g.printCentered(f, box.x + w / 2 + 1, box.y + 9, upper(title), pink);
        g.printCentered(f, box.x + w / 2, box.y + 8, upper(title), yellow);
        int y = box.y + 24;
        for (int i = 0; i < (int)items.size(); ++i) {
            if (i == sel && blinkOn()) g.print(f, box.x + 8, y, ">", yellow);
            g.print(f, box.x + 20, y, upper(items[i]), i == sel ? white : Color{150, 110, 190});
            y += 14;
        }
        std::string hint = std::string(btnLabel(kConfirm)) + ":OK " + btnLabel(kBack) + ":BACK";
        g.printCentered(f, box.x + w / 2, box.y + h - 13, hint, cyan);
    }

private:
    void key(Gfx& g, const EditorView& v, Btn b, Rect r, Color edge) {
        const MultiTap& tap = *v.tap;
        Font& f = font(g);
        const bool held = v.held & b;
        g.fill(r, held ? edge : Color{10, 4, 30, 200});
        g.outline(r, edge);
        const Color fg = held ? night : white;
        g.print(f, r.x + 2, r.y + 2, keyGlyph(b), held ? night : edge);
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
            if (k == cur) g.fill({lx, ly - 1, f.cellW(), 9}, yellow);
            g.print(f, lx, ly, tap.display(grp->chars[k]), k == cur ? night : fg);
        }
    }
};
}  // namespace

Skin* makeSynthwave() { static Synthwave s; return &s; }
