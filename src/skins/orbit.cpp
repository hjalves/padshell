// Orbit: neon sci-fi. A starfield, a bracketed document panel, and the
// character groups on two rings shaped like the controller (D-pad ring on
// the left, face-button ring on the right) around a mode "core".
#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstring>

#include "../skin.h"

namespace {
using namespace skins;

constexpr Color space{5, 7, 22}, panel{12, 18, 46, 215}, cyan{0, 229, 255}, white{225, 245, 255};
constexpr Color dimc{70, 110, 150}, magenta{255, 60, 200};
const char* kOrb = "Orbitron.ttf";

Color neon(MultiTap::Mode m) {
    switch (m) {
        case MultiTap::Mode::Symbols: return {110, 255, 120};
        case MultiTap::Mode::Commands: return {255, 70, 80};
        default: return {255, 90, 210};
    }
}

std::string upper(std::string s) {
    for (char& c : s) c = (char)std::toupper((unsigned char)c);
    return s;
}

void glow(Gfx& g, float cx, float cy, float r, Color c) {
    g.radial(cx, cy, r + 30, withAlpha(c, 120), withAlpha(c, 0));
}

void brackets(Gfx& g, Rect r, Color c) {
    const int L = 26, t = 3;
    g.fill({r.x, r.y, L, t}, c);                 g.fill({r.x, r.y, t, L}, c);
    g.fill({r.x + r.w - L, r.y, L, t}, c);       g.fill({r.x + r.w - t, r.y, t, L}, c);
    g.fill({r.x, r.y + r.h - t, L, t}, c);       g.fill({r.x, r.y + r.h - L, t, L}, c);
    g.fill({r.x + r.w - L, r.y + r.h - t, L, t}, c); g.fill({r.x + r.w - t, r.y + r.h - L, t, L}, c);
}

class Orbit : public Skin {
public:
    const char* name() const override { return "orbit"; }

    Rect frame(Gfx& g, const ShellView& v) override {
        g.clear(space);
        g.gradient({0, Gfx::H / 2, Gfx::W, Gfx::H / 2}, {0, 0, 0, 0}, {0, 0, 0, 0}, {40, 0, 80, 120},
                   {0, 40, 90, 120});
        // Deterministic twinkling starfield.
        uint32_t s = 12345;
        uint32_t t = SDL_GetTicks();
        for (int i = 0; i < 140; ++i) {
            s = s * 1103515245u + 12345u; int x = (s >> 8) % Gfx::W;
            s = s * 1103515245u + 12345u; int y = (s >> 8) % Gfx::H;
            s = s * 1103515245u + 12345u;
            float tw = 0.5f + 0.5f * std::sin(t / 700.0f + (s % 628) / 100.0f);
            int size = (s >> 20) % 3 == 0 ? 2 : 1;
            g.fill({x, y, size, size}, {200, 220, 255, (uint8_t)(60 + 150 * tw)});
        }

        Font& small = g.font(kOrb, 18);
        Font& big = g.font(kOrb, 30);
        int x = 64 + g.draw(small, 64, 36, "PADSHELL  //  ", cyan);
        g.draw(big, x, 28, upper(v.heading), white);
        std::string right = v.toast.empty() ? v.clock : upper(v.toast);
        g.draw(big, Gfx::W - 64 - Gfx::measure(big, right), 28, right, v.toast.empty() ? white : magenta);
        return {64, 88, Gfx::W - 128, Gfx::H - 88 - 20};
    }

    void editor(Gfx& g, Rect a, const EditorView& v) override {
        const MultiTap& tap = *v.tap;
        const Color mc = neon(v.mode());

        Rect doc{a.x, a.y, a.w, 296};
        g.fill(doc, panel);
        g.outline(doc, withAlpha(cyan, 70), 1);
        brackets(g, doc, cyan);

        Font& f = g.font("DejaVuSansMono.ttf", 30);
        Rect ta{doc.x + 28, doc.y + 20, doc.w - 56, doc.h - 40};
        const int cols = std::max(1, ta.w / f.cellW());
        const int visible = std::max(1, ta.h / f.lineH());
        const auto rows = v.wrap(cols);
        const int top = std::max(0, (int)rows.size() - visible);
        for (int r = top; r < (int)rows.size() && r < top + visible; ++r)
            g.print(f, ta.x, ta.y + (r - top) * f.lineH(), v.rowText(rows[r]), white);
        if (v.empty() && !tap.hasPending())
            g.draw(g.font(kOrb, 22), ta.x + 2 * f.cellW(), ta.y + 4, "AWAITING INPUT", dimc);

        int cx = ta.x + (v.endColumn() % cols) * f.cellW();
        int cy = ta.y + ((int)rows.size() - 1 - top) * f.lineH();
        g.fill({cx - 4, cy - 2, f.cellW() + 8, f.lineH() + 4}, withAlpha(mc, 40));
        if (tap.hasPending()) {
            g.fill({cx, cy, f.cellW(), f.lineH()}, mc);
            g.print(f, cx, cy, tap.pendingText(), space);
        }
        g.fill({cx, cy + f.lineH() - 4, f.cellW(), 4}, mc);

        // Two rings plus the mode core.
        const int ringY = a.y + a.h - 152;
        ring(g, v, 330, ringY, {DUp, DRight, DDown, DLeft}, "D-PAD");
        ring(g, v, Gfx::W - 330, ringY, {North, East, South, West}, "BUTTONS");
        core(g, v, Gfx::W / 2, ringY);
    }

    void menu(Gfx& g, const std::string& title, const std::vector<std::string>& items, int sel) override {
        g.fill({0, 0, Gfx::W, Gfx::H}, {2, 4, 14, 225});
        Font& f = g.font(kOrb, 32);
        int lh = f.lineH() + 18, w = 560, h = (int)items.size() * lh + 140;
        Rect box{(Gfx::W - w) / 2, (Gfx::H - h) / 2, w, h};
        g.fill(box, panel);
        g.outline(box, withAlpha(cyan, 70), 1);
        brackets(g, box, cyan);
        g.draw(g.font(kOrb, 18), box.x + 36, box.y + 26, "//  " + upper(title), cyan);
        int y = box.y + 70;
        for (int i = 0; i < (int)items.size(); ++i) {
            std::string label = upper(items[i]);
            if (i == sel) {
                g.fill({box.x + 24, y - 4, w - 48, f.lineH() + 8}, withAlpha(cyan, 40));
                g.fill({box.x + 24, y - 4, 4, f.lineH() + 8}, cyan);
                label = "[  " + label + "  ]";
            }
            g.drawCentered(f, box.x + w / 2, y, label, i == sel ? white : dimc);
            y += lh;
        }
        std::string hint = std::string(btnLabel(kConfirm)) + "  SELECT      " + btnLabel(kBack) + "  CANCEL";
        g.drawCentered(g.font(kOrb, 16), box.x + w / 2, box.y + h - 40, hint, dimc);
    }

private:
    // Buttons in N, E, S, W order around the ring.
    void ring(Gfx& g, const EditorView& v, int cx, int cy, std::array<Btn, 4> btns, const char* caption) {
        const MultiTap& tap = *v.tap;
        const float R = 98, nr = 44;
        g.ring(cx, cy, R, 2, withAlpha(cyan, 60));
        g.drawCentered(g.font(kOrb, 13), cx, cy - 8, caption, dimc);
        const int dx[4] = {0, 1, 0, -1}, dy[4] = {-1, 0, 1, 0};
        for (int i = 0; i < 4; ++i) {
            const Btn b = btns[i];
            float nx = cx + dx[i] * R, ny = cy + dy[i] * R;
            const bool held = v.held & b;
            const auto* grp = groupFor(v.mode(), b);
            const auto* cmd = v.mode() == MultiTap::Mode::Commands ? commandFor(b) : nullptr;
            const bool live = grp || cmd;
            Color c = live ? neon(v.mode()) : dimc;
            if (held) glow(g, nx, ny, nr, c);
            g.fillCircle(nx, ny, nr, held ? withAlpha(c, 150) : Color{10, 16, 40});
            g.ring(nx, ny, nr, 3, live ? c : withAlpha(dimc, 120));

            // Button label just outside the node, away from the ring centre.
            float lx = nx + dx[i] * (nr + 16), ly = ny + dy[i] * (nr + 16);
            if (dx[i] == 0 && dy[i] == 0) continue;
            if (isDpad(b)) g.arrow(lx, ly, 12, dirOf(b), cyan);
            else g.drawCentered(g.font(kOrb, 16), (int)lx, (int)ly - 10, btnLabel(b), cyan);

            if (cmd) {
                Font& cf = g.font("DejaVuSans.ttf", 14);
                std::string l = cmd->label;
                size_t sp = l.find(' ');
                if (sp == std::string::npos) g.drawCentered(cf, (int)nx, (int)ny - 9, l, white);
                else {
                    g.drawCentered(cf, (int)nx, (int)ny - 17, l.substr(0, sp), white);
                    g.drawCentered(cf, (int)nx, (int)ny - 1, l.substr(sp + 1), white);
                }
                continue;
            }
            if (!grp) continue;
            int n = (int)std::strlen(grp->chars);
            int cur = tap.hasPending() && tap.pendingGroup() == grp->chars ? tap.pendingIndex() : -1;
            Font& cf = g.font("DejaVuSansMono.ttf", n > 5 ? 17 : 22);
            int perLine = n > 5 ? (n + 1) / 2 : n, lines = (n + perLine - 1) / perLine;
            for (int k = 0; k < n; ++k) {
                int line = k / perLine, idx = k % perLine, inLine = std::min(perLine, n - line * perLine);
                int x = (int)nx - inLine * cf.cellW() / 2 + idx * cf.cellW();
                int y = (int)(ny - cf.lineH() / 2.f + (line - (lines - 1) / 2.f) * cf.lineH());
                if (k == cur) g.fill({x - 1, y + 1, cf.cellW() + 2, cf.lineH() - 2}, white);
                g.print(cf, x, y, tap.display(grp->chars[k]), k == cur ? space : white);
            }
        }
    }

    void core(Gfx& g, const EditorView& v, int cx, int cy) {
        const Color mc = neon(v.mode());
        glow(g, cx, cy, 56, mc);
        g.fillCircle(cx, cy, 56, {10, 16, 40});
        g.ring(cx, cy, 56, 3, mc);
        g.drawCentered(g.font(kOrb, 26), cx, cy - 18, upper(modeName(v.mode(), v.tap->uppercase())), mc);

        struct Pill { Btn b; const char* what; bool on; };
        const Pill pills[] = {{ZL, "SYM", v.mode() == MultiTap::Mode::Symbols},
                              {ZR, "EDIT", v.mode() == MultiTap::Mode::Commands},
                              {L, "CAPS", v.tap->uppercase()}};
        Font& pf = g.font(kOrb, 13);
        const int pw = 84, gap = 8;
        int y = cy + 76;
        int x = cx - (3 * pw + 2 * gap) / 2;
        for (const Pill& p : pills) {
            Rect r{x, y, pw, 24};
            g.fillRounded(r, 12, p.on ? withAlpha(mc, 200) : Color{20, 30, 60});
            g.drawCentered(pf, r.x + pw / 2, r.y + 4, std::string(btnLabel(p.b)) + " " + p.what, p.on ? space : dimc);
            x += pw + gap;
        }
        g.drawCentered(g.font(kOrb, 13), cx, cy - 92, v.modified ? "UNSAVED" : "SAVED", v.modified ? magenta : dimc);
        g.drawCentered(g.font(kOrb, 13), cx, cy + 108, "+ MENU    - APPS", dimc);
    }
};
}  // namespace

Skin* makeOrbit() { static Orbit s; return &s; }
