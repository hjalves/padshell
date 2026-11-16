// Focus: distraction-free writing. Nothing but a quiet top line and a large,
// centred column of text. The status line along the bottom stays muted while
// you write and comes up in full colour when you act (a button held, a
// character pending, a trigger mode). The commit timer is a thin bar under
// the pending character itself.
#include "family.h"

namespace {
using namespace fam;

const Palette& P = darkPalette();
const char* kMono = "DejaVuSansMono.ttf";

class Focus : public Skin {
public:
    const char* name() const override { return "focus"; }

    Rect frame(Gfx& g, const ShellView& v) override {
        g.clear(P.bg);
        Font& f = g.font("DejaVuSans.ttf", 19);
        g.draw(f, 140, 30, v.heading, P.muted);
        std::string right = v.toast.empty() ? v.clock : v.toast;
        g.draw(f, Gfx::W - 140 - Gfx::measure(f, right), 30, right, v.toast.empty() ? P.muted : P.caps);
        return {140, 90, Gfx::W - 280, Gfx::H - 90};
    }

    void editor(Gfx& g, Rect a, const EditorView& v) override {
        Font& f = g.font(kMono, 42);
        const int lh = f.lineH() + 12;
        Rect doc{a.x, a.y, a.w, 7 * lh};
        document(g, doc, v, P, {&f, lh, 5});
        if (v.empty() && !v.tap->hasPending())
            g.draw(g.font("DejaVuSans-ExtraLight.ttf", 40), doc.x + f.cellW() + 16, doc.y + 4, "start typing", P.raised);

        // Timer under the pending character.
        if (float t = timerLeft(v); t > 0) {
            const int cols = std::max(1, doc.w / f.cellW());
            const auto rows = v.wrap(cols);
            const int top = std::max(0, (int)rows.size() - 7);
            int cx = doc.x + (v.endColumn() % cols) * f.cellW();
            int cy = doc.y + ((int)rows.size() - 1 - top) * lh + lh + 4;
            g.fill({cx, cy, (int)(f.cellW() * t), 3}, buttonColor(pendingButton(v)));
        }

        const bool active = v.held || v.tap->hasPending() || v.mode() != MultiTap::Mode::Letters;
        const int y = Gfx::H - 70, s = 28;
        g.fill({a.x, y - 22, a.w, 1}, P.surface);
        if (v.tap->hasPending()) {
            Btn b = pendingButton(v);
            badge(g, a.x, y - 4, 36, b, buttonColor(b), P.dark);
            cycle(g, a.x + 48, y - 4, 36, v, P);
        } else {
            groupsLine(g, a.x, y, s, g.font(kMono, 18), v, P, !active);
        }
        Font& tf = g.font("DejaVuSansMono-Bold.ttf", 16);
        if (active || v.tap->uppercase()) modeTags(g, a.x + a.w, y, s, tf, v, P);
        else g.draw(g.font(kMono, 16), a.x + a.w - 160, y + 5, v.modified ? "unsaved  + menu" : "+ menu", P.muted);
    }

    void menu(Gfx& g, const std::string& title, const std::vector<std::string>& items, int sel) override {
        g.fill({0, 0, Gfx::W, Gfx::H}, withAlpha(P.bg, 252));
        Font& f = g.font(kMono, 34), &fb = g.font("DejaVuSansMono-Bold.ttf", 34);
        const int lh = f.lineH() + 26, w = 520;
        int y = (Gfx::H - (int)items.size() * lh) / 2;
        g.drawCentered(g.font(kMono, 19), Gfx::W / 2, y - 50, title, P.muted);
        for (int i = 0; i < (int)items.size(); ++i) {
            const bool on = i == sel;
            if (on) g.fill({(Gfx::W - w) / 2, y, w, f.lineH() + 14}, P.letters);
            g.drawCentered(on ? fb : f, Gfx::W / 2, y + 7, items[i], on ? P.dark : Color{205, 200, 225});
            y += lh;
        }
    }
};
}  // namespace

Skin* makeFocus() { static Focus s; return &s; }
