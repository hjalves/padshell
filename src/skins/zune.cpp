// Zune: Metro's typographic sibling (Zune HD / Windows Phone hubs). A giant
// cropped heading bleeds off the left edge, the modes are pivot headers, and
// the character groups are pure type in a row, with no boxes.
#include <algorithm>
#include <cctype>
#include <cstring>

#include "../skin.h"

namespace {
using namespace skins;

constexpr Color bg{0, 0, 0}, text{255, 255, 255}, grey{90, 90, 90}, mid{150, 150, 150};
constexpr Color magenta{236, 0, 140}, lime{140, 200, 30}, orange{255, 110, 0};

Color accent(MultiTap::Mode m) {
    return m == MultiTap::Mode::Symbols ? lime : m == MultiTap::Mode::Commands ? orange : magenta;
}

class Zune : public Skin {
public:
    const char* name() const override { return "zune"; }

    Rect frame(Gfx& g, const ShellView& v) override {
        g.clear(bg);
        // The oversized panorama title, cropped by the screen edge.
        g.draw(g.font("DejaVuSans-ExtraLight.ttf", 170), -26, -62, v.heading, Color{255, 255, 255, 46});
        Font& clock = g.font("DejaVuSans-ExtraLight.ttf", 40);
        g.draw(clock, Gfx::W - 64 - Gfx::measure(clock, v.clock), 34, v.clock, text);
        if (!v.toast.empty()) {
            Font& t = g.font("DejaVuSans.ttf", 20);
            g.draw(t, Gfx::W - 64 - Gfx::measure(t, v.toast), 84, v.toast, magenta);
        }
        return {64, 120, Gfx::W - 128, Gfx::H - 120 - 32};
    }

    void editor(Gfx& g, Rect a, const EditorView& v) override {
        const MultiTap& tap = *v.tap;
        const Color ac = accent(v.mode());

        // Pivot headers: the current mode bright, the others faded.
        Font& pivot = g.font("DejaVuSans-ExtraLight.ttf", 46);
        const char* names[3] = {tap.uppercase() ? "LETTERS" : "letters", "symbols", "edit"};
        const MultiTap::Mode modes[3] = {MultiTap::Mode::Letters, MultiTap::Mode::Symbols, MultiTap::Mode::Commands};
        int x = a.x - 4;
        for (int i = 0; i < 3; ++i) {
            bool on = v.mode() == modes[i];
            x += g.draw(pivot, x, a.y, names[i], on ? text : grey) + 34;
        }

        // Document.
        Font& f = g.font("DejaVuSansMono.ttf", 32);
        Rect ta{a.x, a.y + 84, a.w, 300};
        const int cols = std::max(1, ta.w / f.cellW()), visible = std::max(1, ta.h / f.lineH());
        const auto rows = v.wrap(cols);
        const int top = std::max(0, (int)rows.size() - visible);
        for (int r = top; r < (int)rows.size() && r < top + visible; ++r)
            g.print(f, ta.x, ta.y + (r - top) * f.lineH(), v.rowText(rows[r]), text);
        int cx = ta.x + (v.endColumn() % cols) * f.cellW();
        int cy = ta.y + ((int)rows.size() - 1 - top) * f.lineH();
        if (tap.hasPending()) g.print(f, cx, cy, tap.pendingText(), ac);
        g.fill({cx, cy + f.lineH() - 4, f.cellW(), 4}, ac);

        Font& meta = g.font("DejaVuSans.ttf", 19);
        std::string info = std::to_string(v.lineCount()) + " lines   " + std::to_string(v.chars()) + " characters" +
                           (v.modified ? "   unsaved" : "");
        g.draw(meta, a.x, ta.y + ta.h + 8, info, mid);
        std::string hint = std::string(btnLabel(ZL)) + " symbols   " + btnLabel(ZR) + " edit   " + btnLabel(L) +
                           " caps   + menu   - apps";
        g.draw(meta, a.x + a.w - Gfx::measure(meta, hint), ta.y + ta.h + 8, hint, grey);

        // Groups as a row of type: small accent label, big light characters.
        const Btn order[8] = {DLeft, DUp, DDown, DRight, West, North, South, East};
        const int colW = a.w / 8, rowY = a.y + a.h - 104;
        Font& small = g.font("DejaVuSans.ttf", 18);
        for (int i = 0; i < 8; ++i) {
            const Btn b = order[i];
            int gx = a.x + i * colW;
            bool held = v.held & b;
            buttonLabel(g, small, gx, rowY, b, held ? text : ac);
            if (v.mode() == MultiTap::Mode::Commands) {
                if (const auto* c = commandFor(b))
                    g.draw(g.font("DejaVuSans-ExtraLight.ttf", 30), gx, rowY + 34, c->label, held ? ac : text);
                continue;
            }
            const auto* grp = groupFor(v.mode(), b);
            if (!grp) continue;
            int n = (int)std::strlen(grp->chars);
            int cur = tap.hasPending() && tap.pendingGroup() == grp->chars ? tap.pendingIndex() : -1;
            int perLine = n > 5 ? (n + 1) / 2 : n, cx2 = gx;
            // Largest size whose widest line still fits the column.
            int px = 44;
            for (; px > 20; px -= 2) {
                Font& t = g.font("DejaVuSans-ExtraLight.ttf", px);
                std::string line = groupText(tap, grp->chars).substr(0, perLine);
                if (Gfx::measure(t, line) + (perLine - 1) * 6 <= colW - 34) break;
            }
            Font& cf = g.font("DejaVuSans-ExtraLight.ttf", px);
            for (int k = 0; k < n; ++k) {
                if (k % perLine == 0) cx2 = gx;
                int ly = rowY + 30 + (k / perLine) * (cf.lineH() - 6);
                std::string ch = tap.display(grp->chars[k]);
                int w = g.draw(cf, cx2, ly, ch, k == cur ? ac : held ? ac : text);
                if (k == cur) g.fill({cx2, ly + cf.lineH() - 2, w, 3}, ac);
                cx2 += w + (n > 5 ? 4 : 6);
            }
        }
    }

    void menu(Gfx& g, const std::string& title, const std::vector<std::string>& items, int sel) override {
        g.fill({0, 0, Gfx::W, Gfx::H}, {0, 0, 0, 248});
        g.draw(g.font("DejaVuSans-ExtraLight.ttf", 170), -26, -62, title, Color{255, 255, 255, 46});
        Font& f = g.font("DejaVuSans-ExtraLight.ttf", 52);
        int y = 190;
        for (int i = 0; i < (int)items.size(); ++i) {
            g.draw(f, 64, y, items[i], i == sel ? text : grey);
            if (i == sel) g.fillCircle(44, y + f.lineH() / 2.f + 2, 7, magenta);
            y += f.lineH() + 6;
        }
        Font& h = g.font("DejaVuSans.ttf", 19);
        g.draw(h, 64, Gfx::H - 60, std::string(btnLabel(kConfirm)) + " select     " + btnLabel(kBack) + " back", mid);
    }
};
}  // namespace

Skin* makeZune() { static Zune s; return &s; }
