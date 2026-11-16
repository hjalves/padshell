// Cross: metro with the tiles arranged like the controller itself, a D-pad
// cross on the left and a face-button diamond on the right, each tile in its
// button's colour. Between them sit the mode tags, the pending character's
// cycle and its commit timer.
#include <array>

#include "family.h"

namespace {
using namespace fam;

const Palette& P = darkPalette();
constexpr int TW = 124, TH = 64, G = 6;  // tile size and gap

class Cross : public Skin {
public:
    const char* name() const override { return "cross"; }

    Rect frame(Gfx& g, const ShellView& v) override { return metroHeader(g, v, P); }

    void editor(Gfx& g, Rect a, const EditorView& v) override {
        const int clusterH = 3 * TH + 2 * G, clusterW = 3 * TW + 2 * G;
        const int cy = a.y + a.h - clusterH;
        Font& f = g.font("DejaVuSansMono.ttf", 32);
        document(g, {a.x, a.y, a.w, cy - 24 - a.y}, v, P, {&f, f.lineH() + 2, 6});
        if (v.empty() && !v.tap->hasPending())
            g.draw(g.font("DejaVuSans-ExtraLight.ttf", 38), a.x + f.cellW() + 14, a.y - 4, "start typing", P.muted);

        cluster(g, v, a.x, cy, {DUp, DLeft, DRight, DDown});
        cluster(g, v, a.x + a.w - clusterW, cy, {North, West, East, South});

        // Centre column.
        const int mx = a.x + clusterW + 40, mw = a.w - 2 * clusterW - 80, cx = mx + mw / 2;
        Font& tf = g.font("DejaVuSansMono-Bold.ttf", 18);
        modeTags(g, cx + 70, cy + 4, 32, tf, v, P);
        if (v.tap->hasPending()) {
            const int n = (int)std::strlen(v.tap->pendingGroup());
            const int cell = std::min(40, mw / n);  // long symbol groups shrink to fit
            const int w = n * cell;
            cycle(g, cx - w / 2, cy + 70 + (40 - cell) / 2, cell, v, P);
            g.fill({cx - w / 2, cy + 118, w, 4}, P.surface);
            g.fill({cx - w / 2, cy + 118, (int)(w * timerLeft(v)), 4}, buttonColor(pendingButton(v)));
        } else {
            Font& h = g.font("DejaVuSans.ttf", 19);
            int y = cy + 62;
            for (auto [b, what] : {std::pair{ZL, "hold: symbols"}, std::pair{ZR, "hold: edit"}, std::pair{L, "caps"}}) {
                std::string s = std::string(btnLabel(b)) + "  " + what;
                g.drawCentered(h, cx, y, s, P.muted);
                y += h.lineH() + 4;
            }
        }
        Font& s = g.font("DejaVuSans.ttf", 17);
        std::string info = std::to_string(v.chars()) + " chars" + (v.modified ? "  \xC2\xB7  unsaved" : "") +
                           "    + menu   - apps";
        g.drawCentered(s, cx, cy + clusterH - s.lineH(), info, P.muted);
    }

    void menu(Gfx& g, const std::string& title, const std::vector<std::string>& items, int sel) override {
        metroMenu(g, P, title, items, sel);
    }

private:
    // Buttons in up, left, right, down order, laid out as a plus.
    void cluster(Gfx& g, const EditorView& v, int x, int y, std::array<Btn, 4> b) {
        tile(g, v, b[0], {x + TW + G, y, TW, TH});
        tile(g, v, b[1], {x, y + TH + G, TW, TH});
        tile(g, v, b[2], {x + 2 * (TW + G), y + TH + G, TW, TH});
        tile(g, v, b[3], {x + TW + G, y + 2 * (TH + G), TW, TH});
    }

    void tile(Gfx& g, const EditorView& v, Btn b, Rect t) {
        const MultiTap& tap = *v.tap;
        const bool commands = v.mode() == MultiTap::Mode::Commands;
        const auto* cmd = commands ? commandFor(b) : nullptr;
        const auto* grp = commands ? nullptr : groupFor(v.mode(), b);
        const bool live = cmd || grp;
        const Color c = live ? buttonColor(b) : P.surface;
        g.fill(t, (v.held & b) ? mix(c, Color{255, 255, 255}, 0.3f) : c);
        const Color fg = live ? P.dark : P.muted;
        // Label in the top-right corner, content on the left.
        if (isDpad(b)) g.arrow(t.x + t.w - 16, t.y + 15, 12, dirOf(b), fg);
        else g.drawCentered(g.font("DejaVuSansMono-Bold.ttf", 15), t.x + t.w - 15, t.y + 5, btnLabel(b), fg);

        if (cmd) {
            Font& cf = g.font("DejaVuSans.ttf", 19);
            g.draw(cf, t.x + 10, t.y + t.h - cf.lineH() - 8, cmd->label, P.dark);
            return;
        }
        if (!grp) return;
        const int n = (int)std::strlen(grp->chars);
        const int cur = tap.hasPending() && tap.pendingGroup() == grp->chars ? tap.pendingIndex() : -1;
        Font& mf = g.font("DejaVuSansMono.ttf", n > 5 ? 19 : 26);
        const int perLine = n > 5 ? (n + 1) / 2 : n, lines = (n + perLine - 1) / perLine;
        const int step = mf.cellW() + 2;
        for (int k = 0; k < n; ++k) {
            int lx = t.x + 10 + (k % perLine) * step;
            int ly = t.y + t.h - 6 - (lines - k / perLine) * (mf.lineH() - 2);
            if (k == cur) g.fill({lx - 2, ly + 2, mf.cellW() + 4, mf.lineH() - 4}, P.dark);
            g.print(mf, lx, ly, tap.display(grp->chars[k]), k == cur ? c : P.dark);
        }
    }
};
}  // namespace

Skin* makeCross() { static Cross s; return &s; }
