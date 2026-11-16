// Keys and Light: metro's layout (big light heading, large document, a strip
// of flat tiles in controller order) where every tile keeps its button's own
// colour in all modes, as in ameno. The mode shows in the cursor and tags; a
// bar along the bottom of the pending tile counts down to the commit.
// "keys" is the dark version, "light" a daytime light theme.
#include "family.h"

namespace {
using namespace fam;

constexpr Btn kTileOrder[8] = {DLeft, DUp, DDown, DRight, West, North, South, East};

class Keys : public Skin {
public:
    Keys(const char* name, const Palette& p) : name_(name), p_(p) {}
    const char* name() const override { return name_; }

    Rect frame(Gfx& g, const ShellView& v) override { return metroHeader(g, v, p_); }

    void editor(Gfx& g, Rect a, const EditorView& v) override {
        const int tileH = 116, rowH = 34;
        Rect strip{a.x, a.y + a.h - tileH, a.w, tileH};
        const int rowY = strip.y - rowH - 18;
        Font& f = g.font("DejaVuSansMono.ttf", 34);
        document(g, {a.x, a.y, a.w, rowY - 20 - a.y}, v, p_, {&f, f.lineH() + 2, 6});
        if (v.empty() && !v.tap->hasPending())
            g.draw(g.font("DejaVuSans-ExtraLight.ttf", 40), a.x + f.cellW() + 14, a.y - 4, "start typing", p_.muted);

        Font& s = g.font("DejaVuSans.ttf", 20);
        std::string info = std::to_string(v.lineCount()) + (v.lineCount() == 1 ? " line" : " lines") + "  \xC2\xB7  " +
                           std::to_string(v.chars()) + " chars" + (v.modified ? "  \xC2\xB7  unsaved" : "");
        g.draw(s, a.x, rowY + (rowH - s.lineH()) / 2, info, p_.muted);
        int tw = modeTags(g, a.x + a.w, rowY, rowH, g.font("DejaVuSansMono-Bold.ttf", 18), v, p_);
        std::string hints = std::string(btnLabel(ZL)) + " symbols   " + btnLabel(ZR) + " edit   " + btnLabel(L) +
                            " caps   + menu   - apps";
        g.draw(s, a.x + a.w - tw - 30 - Gfx::measure(s, hints), rowY + (rowH - s.lineH()) / 2, hints, p_.muted);

        tiles(g, strip, v);
    }

    void menu(Gfx& g, const std::string& title, const std::vector<std::string>& items, int sel) override {
        metroMenu(g, p_, title, items, sel);
    }

private:
    void tiles(Gfx& g, Rect strip, const EditorView& v) {
        const int gap = 8, groupGap = 48;
        const int tileW = (strip.w - 6 * gap - groupGap) / 8;
        const MultiTap& tap = *v.tap;
        const bool commands = v.mode() == MultiTap::Mode::Commands;
        const Btn pend = pendingButton(v);
        Font& mf = g.font("DejaVuSansMono.ttf", 30);
        Font& lf = g.font("DejaVuSansMono-Bold.ttf", 17);
        for (int i = 0; i < 8; ++i) {
            const Btn b = kTileOrder[i];
            Rect t{strip.x + i * (tileW + gap) + (i >= 4 ? groupGap - gap : 0), strip.y, tileW, strip.h};
            const auto* cmd = commands ? commandFor(b) : nullptr;
            const auto* grp = commands ? nullptr : groupFor(v.mode(), b);
            const bool live = cmd || grp;
            Color c = live ? buttonColor(b, p_.light) : p_.surface;
            const bool held = v.held & b;
            g.fill(t, held ? mix(c, p_.light ? Color{0, 0, 0} : Color{255, 255, 255}, 0.3f) : c);
            const Color fg = live ? p_.dark : p_.muted;
            if (isDpad(b)) g.arrow(t.x + 20, t.y + 20, 14, dirOf(b), fg);
            else g.draw(lf, t.x + 12, t.y + 8, btnLabel(b), fg);

            if (cmd) {
                std::string l1 = cmd->label, l2;
                Font& cf = g.font("DejaVuSans.ttf", 22);
                if (Gfx::measure(cf, l1) > t.w - 24 && l1.find(' ') != std::string::npos) {
                    l2 = l1.substr(l1.find(' ') + 1);
                    l1 = l1.substr(0, l1.find(' '));
                }
                int y = t.y + t.h - 12 - (l2.empty() ? 1 : 2) * cf.lineH();
                g.draw(cf, t.x + 12, y, l1, p_.dark);
                if (!l2.empty()) g.draw(cf, t.x + 12, y + cf.lineH(), l2, p_.dark);
                continue;
            }
            if (!grp) continue;
            const int n = (int)std::strlen(grp->chars);
            const int cur = b == pend && tap.pendingGroup() == grp->chars ? tap.pendingIndex() : -1;
            const int perLine = n > 5 ? (n + 1) / 2 : n, lines = (n + perLine - 1) / perLine;
            const int step = mf.cellW() + 3;
            for (int k = 0; k < n; ++k) {
                int lx = t.x + 12 + (k % perLine) * step;
                int ly = t.y + t.h - 14 - (lines - k / perLine) * mf.lineH();
                if (k == cur) g.fill({lx - 2, ly + 2, mf.cellW() + 4, mf.lineH() - 2}, p_.dark);
                g.print(mf, lx, ly, tap.display(grp->chars[k]), k == cur ? c : p_.dark);
            }
            if (cur >= 0) g.fill({t.x, t.y + t.h - 6, (int)(t.w * timerLeft(v)), 6}, p_.dark);
        }
    }

    const char* name_;
    const Palette& p_;
};
}  // namespace

Skin* makeKeys() { static Keys s("keys", darkPalette()); return &s; }
Skin* makeLight() { static Keys s("light", lightPalette()); return &s; }
