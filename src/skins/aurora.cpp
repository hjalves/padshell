// Aurora: soft and modern. A slowly drifting colour-gradient backdrop with
// frosted-glass cards, rounded everything, Quicksand type, and the character
// groups in a floating dock tinted by mode.
#include <algorithm>
#include <cmath>
#include <cstring>

#include "../skin.h"

namespace {
using namespace skins;

constexpr Color white{255, 255, 255}, deep{40, 26, 80};
const char* kQuick = "Quicksand.ttf";

Color glass(uint8_t a) { return {255, 255, 255, a}; }

Color pastel(MultiTap::Mode m) {
    switch (m) {
        case MultiTap::Mode::Symbols: return {140, 240, 170};
        case MultiTap::Mode::Commands: return {255, 125, 125};
        default: return {255, 150, 225};
    }
}

class Aurora : public Skin {
public:
    const char* name() const override { return "aurora"; }

    Rect frame(Gfx& g, const ShellView& v) override {
        const float t = SDL_GetTicks() / 1000.0f;
        g.gradient({0, 0, Gfx::W, Gfx::H}, {48, 28, 110}, {190, 80, 140}, {18, 92, 140}, {250, 150, 110});
        // Drifting light blobs.
        struct Blob { float x, y, r, sp; Color c; };
        const Blob blobs[] = {{300, 200, 260, 0.13f, {120, 220, 255, 26}},
                              {950, 520, 300, 0.09f, {255, 200, 120, 24}},
                              {760, 140, 200, 0.17f, {255, 120, 220, 22}}};
        for (const Blob& b : blobs) {
            float x = b.x + 60 * std::sin(t * b.sp), y = b.y + 40 * std::cos(t * b.sp * 1.3f);
            g.radial(x, y, b.r, withAlpha(b.c, 70), withAlpha(b.c, 0));
        }

        Rect pill{64, 26, Gfx::W - 128, 64};
        shadow(g, pill, 32);
        g.fillRounded(pill, 32, glass(42));
        Font& small = g.font(kQuick, 20);
        Font& big = g.font(kQuick, 34);
        int x = pill.x + 30 + g.draw(small, pill.x + 30, pill.y + 21, "padshell  \xC2\xB7  ", glass(190));
        g.draw(big, x, pill.y + 11, v.heading, white);
        int cw = Gfx::measure(big, v.clock);
        g.draw(big, pill.x + pill.w - 30 - cw, pill.y + 11, v.clock, white);
        if (!v.toast.empty()) {
            int tw = Gfx::measure(small, v.toast) + 32;
            Rect tp{pill.x + pill.w - 60 - cw - tw, pill.y + 14, tw, 36};
            g.fillRounded(tp, 18, glass(210));
            g.draw(small, tp.x + 16, tp.y + 7, v.toast, deep);
        }
        return {64, 108, Gfx::W - 128, Gfx::H - 108 - 20};
    }

    void editor(Gfx& g, Rect a, const EditorView& v) override {
        const MultiTap& tap = *v.tap;
        const Color mc = pastel(v.mode());

        Rect card{a.x, a.y, a.w, 330};
        shadow(g, card, 28);
        g.fillRounded(card, 28, glass(38));
        Font& f = g.font("DejaVuSansMono.ttf", 30);
        Rect ta{card.x + 36, card.y + 26, card.w - 72, card.h - 52};
        const int cols = std::max(1, ta.w / f.cellW()), visible = std::max(1, ta.h / f.lineH());
        const auto rows = v.wrap(cols);
        const int top = std::max(0, (int)rows.size() - visible);
        for (int r = top; r < (int)rows.size() && r < top + visible; ++r)
            g.print(f, ta.x, ta.y + (r - top) * f.lineH(), v.rowText(rows[r]), white);
        if (v.empty() && !tap.hasPending())
            g.draw(g.font(kQuick, 30), ta.x + f.cellW() + 14, ta.y, "start typing\xE2\x80\xA6", glass(120));

        int cx = ta.x + (v.endColumn() % cols) * f.cellW();
        int cy = ta.y + ((int)rows.size() - 1 - top) * f.lineH();
        if (tap.hasPending()) {
            g.fillRounded({cx - 3, cy, f.cellW() + 6, f.lineH()}, 8, mc);
            g.print(f, cx, cy, tap.pendingText(), deep);
        }
        g.fillRounded({cx, cy + f.lineH() - 5, f.cellW(), 5}, 2, mc);

        // Status pills.
        Font& pf = g.font(kQuick, 19);
        int py = card.y + card.h + 16, x = a.x;
        auto pill = [&](const std::string& s, bool on, Color c) {
            int w = Gfx::measure(pf, s) + 32;
            g.fillRounded({x, py, w, 34}, 17, on ? c : glass(34));
            g.draw(pf, x + 16, py + 6, s, on ? deep : glass(220));
            x += w + 10;
        };
        pill(modeName(v.mode(), tap.uppercase()), true, mc);
        pill(std::string(btnLabel(ZL)) + "  symbols", v.mode() == MultiTap::Mode::Symbols, pastel(MultiTap::Mode::Symbols));
        pill(std::string(btnLabel(ZR)) + "  edit", v.mode() == MultiTap::Mode::Commands, pastel(MultiTap::Mode::Commands));
        pill(std::string(btnLabel(L)) + "  shift", tap.uppercase(), pastel(MultiTap::Mode::Letters));
        std::string right = std::string(v.modified ? "unsaved" : "saved") + "     +  menu     -  apps";
        int rw = Gfx::measure(pf, right);
        g.draw(pf, a.x + a.w - rw, py + 6, right, glass(200));
        if (v.modified) g.fillCircle(a.x + a.w - rw - 16, py + 17, 5, {255, 220, 120});

        dock(g, v, {a.x + 24, py + 50, a.w - 48, a.y + a.h - (py + 50)});
    }

    void menu(Gfx& g, const std::string& title, const std::vector<std::string>& items, int sel) override {
        g.fill({0, 0, Gfx::W, Gfx::H}, {20, 10, 50, 120});
        Font& f = g.font(kQuick, 34);
        int lh = f.lineH() + 14, w = 500, h = (int)items.size() * lh + 130;
        Rect card{(Gfx::W - w) / 2, (Gfx::H - h) / 2, w, h};
        shadow(g, card, 36);
        g.fillRounded(card, 36, glass(70));
        g.draw(g.font(kQuick, 20), card.x + 36, card.y + 26, title, glass(210));
        int y = card.y + 66;
        for (int i = 0; i < (int)items.size(); ++i) {
            if (i == sel) g.fillRounded({card.x + 20, y - 4, w - 40, f.lineH() + 8}, (f.lineH() + 8) / 2, glass(225));
            g.draw(f, card.x + 44, y, items[i], i == sel ? deep : white);
            y += lh;
        }
        std::string hint = std::string(btnLabel(kConfirm)) + "  select     " + btnLabel(kBack) + "  cancel";
        g.drawCentered(g.font(kQuick, 18), card.x + w / 2, card.y + h - 40, hint, glass(200));
    }

private:
    static void shadow(Gfx& g, Rect r, int radius) {
        for (int i = 3; i >= 1; --i)
            g.fillRounded({r.x - i * 3, r.y + 6 - i * 2, r.w + i * 6, r.h + i * 6}, radius + i * 3, {20, 0, 50, 14});
    }

    void dock(Gfx& g, const EditorView& v, Rect d) {
        const MultiTap& tap = *v.tap;
        shadow(g, d, 34);
        g.fillRounded(d, 34, glass(30));
        const Btn order[8] = {DLeft, DUp, DDown, DRight, West, North, South, East};
        const int gap = 14, groupGap = 44, pad = 16;
        const int kw = (d.w - 2 * pad - 6 * gap - groupGap) / 8, kh = d.h - 2 * pad;
        const Color tint = v.mode() == MultiTap::Mode::Letters ? glass(44) : withAlpha(pastel(v.mode()), 70);
        Font& lab = g.font(kQuick, 18);
        Font& mf = g.font("DejaVuSansMono.ttf", 24);
        for (int i = 0; i < 8; ++i) {
            const Btn b = order[i];
            Rect k{d.x + pad + i * (kw + gap) + (i >= 4 ? groupGap - gap : 0), d.y + pad, kw, kh};
            const bool held = v.held & b;
            const auto* cmd = v.mode() == MultiTap::Mode::Commands ? commandFor(b) : nullptr;
            const auto* grp = groupFor(v.mode(), b);
            g.fillRounded(k, 22, held ? glass(150) : (grp || cmd) ? tint : glass(16));
            buttonLabel(g, lab, k.x + 16, k.y + 10, b, glass(200));
            if (cmd) {
                Font& cf = g.font(kQuick, 20);
                std::string l = cmd->label;
                size_t sp = l.find(' ');
                if (sp == std::string::npos) g.draw(cf, k.x + 16, k.y + kh - 34, l, white);
                else {
                    g.draw(cf, k.x + 16, k.y + kh - 56, l.substr(0, sp), white);
                    g.draw(cf, k.x + 16, k.y + kh - 34, l.substr(sp + 1), white);
                }
                continue;
            }
            if (!grp) continue;
            int n = (int)std::strlen(grp->chars);
            int cur = tap.hasPending() && tap.pendingGroup() == grp->chars ? tap.pendingIndex() : -1;
            int perLine = n > 5 ? (n + 1) / 2 : n, lines = (n + perLine - 1) / perLine;
            int step = mf.cellW() + 2;
            for (int c = 0; c < n; ++c) {
                int x = k.x + 16 + (c % perLine) * step;
                int y = k.y + kh - 10 - (lines - c / perLine) * mf.lineH();
                if (c == cur) g.fillRounded({x - 3, y + 1, mf.cellW() + 6, mf.lineH() - 2}, 8, white);
                g.print(mf, x, y, tap.display(grp->chars[c]), c == cur ? deep : white);
            }
        }
    }
};
}  // namespace

Skin* makeAurora() { static Aurora s; return &s; }
