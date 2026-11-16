// Terminal: a green-phosphor CRT. VT323 type, box frame, numbered lines, a
// block cursor, a keymap in [key] notation with inverse video for pressed
// keys, a vim-style mode line, and scanlines + vignette over everything.
#include <algorithm>
#include <cctype>
#include <cstdio>

#include "../skin.h"

namespace {
using namespace skins;

constexpr Color bg{4, 10, 6}, phos{90, 255, 140}, mid{50, 180, 95}, dim{28, 100, 55}, amber{255, 190, 70};
const char* kFont = "VT323.ttf";

const char* keyName(Btn b) {
    switch (b) {
        case DUp: return "^";
        case DDown: return "v";
        case DLeft: return "<";
        case DRight: return ">";
        default: return btnLabel(b);
    }
}

std::string upper(std::string s) {
    for (char& c : s) c = (char)std::toupper((unsigned char)c);
    return s;
}

bool blinkOn() { return (SDL_GetTicks() / 530) % 2 == 0; }

class Terminal : public Skin {
public:
    const char* name() const override { return "terminal"; }

    Rect frame(Gfx& g, const ShellView& v) override {
        g.clear(bg);
        Font& f = g.font(kFont, 36);
        Rect box{24, 20, Gfx::W - 48, Gfx::H - 40};
        g.outline(box, mid, 2);
        g.outline({box.x + 6, box.y + 6, box.w - 12, box.h - 12}, dim, 1);

        // Title set into the top border, like a TUI window.
        std::string title = " padshell :: " + v.heading + " ";
        g.fill({box.x + 30, box.y - 2, g.width(f, title), 8}, bg);
        g.print(f, box.x + 30, box.y - f.lineH() / 2 + 2, title, phos);
        std::string right = v.toast.empty() ? " " + v.clock + " " : " " + upper(v.toast) + " ";
        int rw = Gfx::width(f, right);
        g.fill({box.x + box.w - 30 - rw, box.y - 2, rw, 8}, bg);
        g.print(f, box.x + box.w - 30 - rw, box.y - f.lineH() / 2 + 2, right, v.toast.empty() ? mid : amber);
        return {box.x + 24, box.y + 28, box.w - 48, box.h - 44};
    }

    void editor(Gfx& g, Rect a, const EditorView& v) override {
        Font& f = g.font(kFont, 36);
        const MultiTap& tap = *v.tap;
        const int lh = f.lineH();
        const int modeY = a.y + a.h - lh, keysY = modeY - 2 * lh - 10;
        Rect ta{a.x, a.y, a.w, keysY - 16 - a.y};

        // Document with zero-padded line numbers.
        const int gutter = 5 * f.cellW();
        const int cols = std::max(1, (ta.w - gutter) / f.cellW());
        const int visible = std::max(1, ta.h / lh);
        const auto rows = v.wrap(cols);
        const int top = std::max(0, (int)rows.size() - visible);
        for (int r = top; r < (int)rows.size() && r < top + visible; ++r) {
            int y = ta.y + (r - top) * lh;
            if (rows[r].first) {
                char num[16];
                std::snprintf(num, sizeof num, "%03d", rows[r].line + 1);
                g.print(f, ta.x, y, num, dim);
            }
            g.print(f, ta.x + gutter, y, v.rowText(rows[r]), phos);
        }
        if (v.empty() && !tap.hasPending()) g.print(f, ta.x + gutter + 2 * f.cellW(), ta.y, "_ready.", dim);

        int cx = ta.x + gutter + (v.endColumn() % cols) * f.cellW();
        int cy = ta.y + ((int)rows.size() - 1 - top) * lh;
        Color mc = modeColor(v.mode());
        if (tap.hasPending()) {
            g.fill({cx, cy + 2, f.cellW(), lh - 4}, mc);
            g.print(f, cx, cy, tap.pendingText(), bg);
        } else if (blinkOn()) {
            g.fill({cx, cy + 2, f.cellW(), lh - 4}, mc);
        }

        // Keymap: two rows, D-pad then face buttons.
        g.fill({a.x, keysY - 10, a.w, 1}, dim);
        int x = a.x;
        const Btn order[8] = {DLeft, DUp, DDown, DRight, West, North, South, East};
        for (int i = 0; i < 8; ++i) {
            const Btn b = order[i];
            int col = i % 4, row = i / 4;
            int kx = a.x + col * (a.w / 4), ky = keysY + row * lh;
            bool held = v.held & b;
            std::string key = std::string("[") + keyName(b) + "]";
            int kw = Gfx::width(f, key);
            if (held) g.fill({kx, ky + 3, kw, lh - 6}, phos);
            g.print(f, kx, ky, key, held ? bg : mid);
            x = kx + kw + f.cellW();
            if (v.mode() == MultiTap::Mode::Commands) {
                if (const auto* c = commandFor(b)) g.print(f, x, ky, c->label, phos);
                continue;
            }
            const auto* grp = groupFor(v.mode(), b);
            if (!grp) continue;
            int cur = tap.hasPending() && tap.pendingGroup() == grp->chars ? tap.pendingIndex() : -1;
            for (int k = 0; grp->chars[k]; ++k) {
                if (k == cur) g.fill({x, ky + 3, f.cellW(), lh - 6}, amber);
                g.print(f, x, ky, tap.display(grp->chars[k]), k == cur ? bg : phos);
                x += f.cellW();
            }
        }

        // vim-style mode line.
        const char* mode = v.mode() == MultiTap::Mode::Commands  ? "-- EDIT --"
                           : v.mode() == MultiTap::Mode::Symbols ? "-- SYMBOL --"
                           : tap.uppercase()                     ? "-- INSERT CAPS --"
                                                                 : "-- INSERT --";
        int mw = Gfx::width(f, mode);
        g.fill({a.x, modeY + 3, mw, lh - 6}, mc);
        g.print(f, a.x, modeY, mode, bg);
        char info[96];
        std::snprintf(info, sizeof info, "%dL %zuC%s   +:menu  -:apps", v.lineCount(), v.chars(),
                      v.modified ? " [+]" : "");
        g.print(f, a.x + a.w - Gfx::width(f, info), modeY, info, mid);
    }

    void menu(Gfx& g, const std::string& title, const std::vector<std::string>& items, int sel) override {
        Font& f = g.font(kFont, 40);
        int w = 520, h = (int)items.size() * f.lineH() + 120;
        Rect box{(Gfx::W - w) / 2, (Gfx::H - h) / 2, w, h};
        g.fill({box.x - 12, box.y - 12, box.w + 24, box.h + 24}, withAlpha(bg, 235));
        g.fill(box, bg);
        g.outline(box, phos, 2);
        g.outline({box.x + 6, box.y + 6, box.w - 12, box.h - 12}, mid, 2);
        std::string t = " " + upper(title) + " ";
        int tw = Gfx::width(f, t);
        g.fill({box.x + (w - tw) / 2, box.y - 4, tw, 12}, bg);
        g.print(f, box.x + (w - tw) / 2, box.y - f.lineH() / 2 + 2, t, phos);
        int y = box.y + 36;
        for (int i = 0; i < (int)items.size(); ++i) {
            std::string label = (i == sel ? "> " : "  ") + upper(items[i]);
            if (i == sel) g.fill({box.x + 30, y + 4, w - 60, f.lineH() - 8}, phos);
            g.print(f, box.x + 36, y, label, i == sel ? bg : phos);
            y += f.lineH();
        }
        std::string hint = std::string("[") + btnLabel(kConfirm) + "] OK   [" + btnLabel(kBack) + "] BACK";
        g.printCentered(g.font(kFont, 30), box.x + w / 2, box.y + h - 48, hint, mid);
    }

    void finish(Gfx& g) override {
        for (int y = 0; y < Gfx::H; y += 3) g.fill({0, y, Gfx::W, 1}, {0, 0, 0, 70});
        const Color edge{0, 0, 0, 150}, none{0, 0, 0, 0};
        g.gradient({0, 0, Gfx::W, 90}, edge, edge, none, none);
        g.gradient({0, Gfx::H - 90, Gfx::W, 90}, none, none, edge, edge);
        g.gradient({0, 0, 120, Gfx::H}, edge, none, edge, none);
        g.gradient({Gfx::W - 120, 0, 120, Gfx::H}, none, edge, none, edge);
    }
};
}  // namespace

Skin* makeTerminal() { static Terminal s; return &s; }
