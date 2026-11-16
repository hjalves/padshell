// Swiss: International Typographic Style. Off-white paper, heavy black sans,
// a strict grid with ruled cells, one red accent, and the mode set vertically
// in the margin. Metro's flatness in a light theme.
#include <algorithm>
#include <cctype>
#include <cstring>

#include "../skin.h"

namespace {
using namespace skins;

constexpr Color paper{242, 240, 234}, ink{17, 17, 17}, red{226, 35, 26}, grey{125, 123, 118};
const char* kBold = "DejaVuSans-Bold.ttf";

std::string caps(std::string s) {
    for (char& c : s) c = (char)std::toupper((unsigned char)c);
    return s;
}

class Swiss : public Skin {
public:
    const char* name() const override { return "swiss"; }

    Rect frame(Gfx& g, const ShellView& v) override {
        g.clear(paper);
        g.draw(g.font(kBold, 104), 56, 4, v.heading, ink);
        Rect block{Gfx::W - 280, 0, 280, 150};
        g.fill(block, red);
        g.draw(g.font(kBold, 54), block.x + 24, block.y + 70, v.clock, paper);
        if (!v.toast.empty()) {
            Font& t = g.font(kBold, 20);
            g.draw(t, block.x - 24 - Gfx::measure(t, caps(v.toast)), 112, caps(v.toast), red);
        }
        g.fill({56, 150, Gfx::W - 112, 8}, ink);
        return {56, 182, Gfx::W - 112, Gfx::H - 182 - 28};
    }

    void editor(Gfx& g, Rect a, const EditorView& v) override {
        const MultiTap& tap = *v.tap;
        const int gridH = 128, gridY = a.y + a.h - gridH, statusY = gridY - 40;

        // Mode, set vertically in the left margin.
        const char* mode = v.mode() == MultiTap::Mode::Commands  ? "EDIT"
                           : v.mode() == MultiTap::Mode::Symbols ? "SYMBOLS"
                           : tap.uppercase()                     ? "CAPITALS"
                                                                 : "LETTERS";
        g.drawRotated(g.font(kBold, 22), 28, a.y + 150, mode, v.mode() == MultiTap::Mode::Letters && !tap.uppercase() ? grey : red, -90);

        Font& f = g.font("DejaVuSansMono.ttf", 32);
        Rect ta{a.x, a.y, a.w, statusY - 16 - a.y};
        const int cols = std::max(1, ta.w / f.cellW()), visible = std::max(1, ta.h / f.lineH());
        const auto rows = v.wrap(cols);
        const int top = std::max(0, (int)rows.size() - visible);
        for (int r = top; r < (int)rows.size() && r < top + visible; ++r)
            g.print(f, ta.x, ta.y + (r - top) * f.lineH(), v.rowText(rows[r]), ink);
        int cx = ta.x + (v.endColumn() % cols) * f.cellW();
        int cy = ta.y + ((int)rows.size() - 1 - top) * f.lineH();
        if (tap.hasPending()) {
            g.fill({cx, cy, f.cellW(), f.lineH()}, red);
            g.print(f, cx, cy, tap.pendingText(), paper);
        }
        g.fill({cx, cy + f.lineH() - 5, f.cellW(), 5}, red);

        Font& s = g.font(kBold, 17);
        std::string left = std::to_string(v.lineCount()) + " LINES / " + std::to_string(v.chars()) + " CHARACTERS" +
                           (v.modified ? " / UNSAVED" : "");
        g.draw(s, a.x, statusY, left, v.modified ? red : ink);
        std::string right = caps(std::string(btnLabel(ZL)) + " symbols   " + btnLabel(ZR) + " edit   " + btnLabel(L) +
                                 " caps   + menu   - apps");
        g.draw(s, a.x + a.w - Gfx::measure(s, right), statusY, right, grey);

        // The grid: eight ruled cells.
        g.fill({a.x, gridY, a.w, 4}, ink);
        const Btn order[8] = {DLeft, DUp, DDown, DRight, West, North, South, East};
        const int cw = a.w / 8;
        for (int i = 0; i < 8; ++i) {
            const Btn b = order[i];
            Rect c{a.x + i * cw, gridY + 4, cw, gridH - 4};
            const bool held = v.held & b;
            if (held) g.fill(c, ink);
            if (i > 0) g.fill({c.x, c.y, 2, c.h}, ink);
            buttonLabel(g, g.font(kBold, 18), c.x + 14, c.y + 10, b, red);
            const Color fg = held ? paper : ink;
            if (v.mode() == MultiTap::Mode::Commands) {
                if (const auto* cmd = commandFor(b)) {
                    std::string l = caps(cmd->label);
                    size_t sp = l.find(' ');
                    Font& cf = g.font(kBold, 20);
                    if (sp == std::string::npos) g.draw(cf, c.x + 14, c.y + c.h - 36, l, fg);
                    else {
                        g.draw(cf, c.x + 14, c.y + c.h - 60, l.substr(0, sp), fg);
                        g.draw(cf, c.x + 14, c.y + c.h - 36, l.substr(sp + 1), fg);
                    }
                }
                continue;
            }
            const auto* grp = groupFor(v.mode(), b);
            if (!grp) continue;
            int n = (int)std::strlen(grp->chars);
            int cur = tap.hasPending() && tap.pendingGroup() == grp->chars ? tap.pendingIndex() : -1;
            Font& cf = g.font(kBold, n > 5 ? 22 : 34);
            int perLine = n > 5 ? (n + 1) / 2 : n, lines = (n + perLine - 1) / perLine;
            int x = c.x + 14;
            for (int k = 0; k < n; ++k) {
                if (k % perLine == 0) x = c.x + 14;
                int y = c.y + c.h - 10 - (lines - k / perLine) * (cf.lineH() - 2);
                std::string ch = tap.display(grp->chars[k]);
                int w = Gfx::measure(cf, ch);
                if (k == cur) g.fill({x - 2, y + 2, w + 4, cf.lineH() - 4}, red);
                g.draw(cf, x, y, ch, k == cur ? paper : fg);
                x += w + 2;
            }
        }
    }

    void menu(Gfx& g, const std::string& title, const std::vector<std::string>& items, int sel) override {
        g.fill({0, 0, Gfx::W, Gfx::H}, withAlpha(paper, 248));
        g.fill({0, 0, 24, Gfx::H}, red);
        g.draw(g.font(kBold, 22), 80, 70, caps(title), red);
        g.fill({80, 104, Gfx::W - 160, 6}, ink);
        Font& f = g.font(kBold, 64);
        int y = 140;
        for (int i = 0; i < (int)items.size(); ++i) {
            g.draw(f, 80, y, (i == sel ? "\xE2\x86\x92 " : "   ") + items[i], i == sel ? red : ink);
            y += f.lineH();
        }
        g.draw(g.font(kBold, 18), 80, Gfx::H - 60,
               caps(std::string(btnLabel(kConfirm)) + " select    " + btnLabel(kBack) + " back"), grey);
    }
};
}  // namespace

Skin* makeSwiss() { static Swiss s; return &s; }
