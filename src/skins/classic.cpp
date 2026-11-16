// Classic: a dense desktop-editor look. Top bar, line numbers, a two-line
// status bar, and a controller diagram panel on the right.
#include <algorithm>
#include <cctype>
#include <cstring>

#include "../skin.h"

namespace {
using namespace skins;

constexpr Color bg{24, 26, 33}, panel{34, 37, 46}, panelHi{48, 52, 64};
constexpr Color text{220, 223, 228}, dim{120, 126, 140}, accent{97, 175, 239}, warn{229, 192, 123};
constexpr int kBarH = 36, kPanelW = 400;

class Classic : public Skin {
public:
    const char* name() const override { return "classic"; }

    Rect frame(Gfx& g, const ShellView& v) override {
        g.clear(bg);
        controller(g, {Gfx::W - kPanelW, kBarH, kPanelW, Gfx::H - kBarH}, v);

        g.fill({0, 0, Gfx::W, kBarH}, panelHi);
        Font& sf = g.small();
        int ty = (kBarH - sf.lineH()) / 2;
        int x = 14 + g.print(sf, 14, ty, "padshell", accent);
        g.print(sf, x + 16, ty, v.title, text);
        int cw = Gfx::width(sf, v.clock);
        g.print(sf, Gfx::W - 14 - cw, ty, v.clock, text);
        std::string right = v.toast.empty() ? "-: apps" : v.toast;
        g.print(sf, Gfx::W - 40 - cw - Gfx::width(sf, right), ty, right, v.toast.empty() ? dim : warn);
        return {0, kBarH, Gfx::W - kPanelW, Gfx::H - kBarH};
    }

    void editor(Gfx& g, Rect area, const EditorView& v) override {
        Font& f = g.text();
        const int statusH = 2 * g.small().lineH() + 14;
        Rect ta{area.x, area.y, area.w, area.h - statusH};
        const auto& lines = *v.lines;
        const int last = (int)lines.size() - 1;
        const int gutterCols = std::max(3, (int)std::to_string(lines.size()).size());
        const int gutterW = (gutterCols + 2) * f.cellW(), pad = 8;
        const int cols = std::max(1, (ta.w - gutterW - 2 * pad) / f.cellW());
        const int visible = std::max(1, (ta.h - 2 * pad) / f.lineH());
        const auto rows = v.wrap(cols);
        const int top = std::max(0, (int)rows.size() - visible);

        g.fill({ta.x, ta.y, gutterW, ta.h}, panel);
        const int textX = ta.x + gutterW + pad;
        for (int r = top; r < (int)rows.size() && r < top + visible; ++r) {
            int y = ta.y + pad + (r - top) * f.lineH();
            if (rows[r].first) {
                std::string num = std::to_string(rows[r].line + 1);
                num.insert(0, gutterCols - num.size(), ' ');
                g.print(f, ta.x + f.cellW(), y, num, rows[r].line == last ? text : dim);
            }
            g.print(f, textX, y, v.rowText(rows[r]), text);
        }

        int cx = textX + (v.endColumn() % cols) * f.cellW();
        int cy = ta.y + pad + ((int)rows.size() - 1 - top) * f.lineH();
        Color cc = modeColor(v.mode());
        if (v.tap->hasPending()) {
            g.fill({cx, cy, f.cellW(), f.lineH()}, withAlpha(cc, 60));
            g.print(f, cx, cy, v.tap->pendingText(), text);
        }
        g.fill({cx, cy + f.lineH() - 4, f.cellW(), 3}, cc);

        status(g, {area.x, area.y + area.h - statusH, area.w, statusH}, v);
    }

    void menu(Gfx& g, const std::string& title, const std::vector<std::string>& items, int sel) override {
        g.fill({0, kBarH, Gfx::W, Gfx::H - kBarH}, {0, 0, 0, 150});
        Font& f = g.text();
        int n = (int)items.size(), rowH = f.lineH() + 16, w = 420, h = n * rowH + 74;
        Rect box{(Gfx::W - w) / 2, (Gfx::H - h) / 2, w, h};
        g.fill(box, panel);
        g.outline(box, panelHi);
        std::string t = title;
        if (!t.empty()) t[0] = (char)std::toupper((unsigned char)t[0]);
        g.print(g.small(), box.x + 20, box.y + 14, t, dim);
        for (int i = 0; i < n; ++i) {
            Rect row{box.x + 10, box.y + 44 + i * rowH, w - 20, rowH};
            if (i == sel) g.fill(row, accent);
            std::string label = items[i];
            if (!label.empty()) label[0] = (char)std::toupper((unsigned char)label[0]);
            g.print(f, row.x + 14, row.y + 8, label, i == sel ? bg : text);
        }
        std::string hint = std::string(btnLabel(kConfirm)) + ": select   " + btnLabel(kBack) + ": cancel";
        g.printCentered(g.small(), box.x + w / 2, box.y + h - 8 - g.small().lineH(), hint, dim);
    }

private:
    void status(Gfx& g, Rect st, const EditorView& v) {
        Font& sf = g.small();
        g.fill(st, panelHi);
        int x = st.x + 12, y = st.y + 6;
        const MultiTap& tap = *v.tap;
        if (tap.hasPending()) {
            x += g.print(sf, x, y, "Selecting: ", dim);
            const char* grp = tap.pendingGroup();
            for (int i = 0; grp[i]; ++i) {
                bool cur = i == tap.pendingIndex();
                if (cur) g.fill({x - 3, y - 1, sf.cellW() + 6, sf.lineH() + 2}, accent);
                g.print(sf, x, y, tap.display(grp[i]), cur ? bg : text);
                x += 2 * sf.cellW();
            }
        } else if (v.mode() == MultiTap::Mode::Commands) {
            for (const auto& c : MultiTap::commands()) {
                x += g.print(sf, x, y, btnLabel(c.button), accent) + sf.cellW();
                x += g.print(sf, x, y, c.label, text) + 3 * sf.cellW();
            }
        } else {
            for (const auto& grp : *groupsFor(v.mode())) {
                x += g.print(sf, x, y, btnLabel(grp.button), accent) + sf.cellW() / 2;
                x += g.print(sf, x, y, groupText(tap, grp.chars), text) + 2 * sf.cellW();
            }
        }
        y += sf.lineH() + 2;
        std::string info = "Ln " + std::to_string(v.lineCount()) + "   " + std::to_string(v.chars()) + " chars";
        g.print(sf, st.x + 12, y, info, dim);
        std::string right;
        if (tap.uppercase()) right += "UPPERCASE   ";
        if (v.modified) right += "modified   ";
        right += "+: menu";
        g.print(sf, st.x + st.w - 12 - Gfx::width(sf, right), y, right, tap.uppercase() || v.modified ? warn : dim);
    }

    // A schematic Switch-style controller with the buttons held lit up.
    void controller(Gfx& g, Rect p, const ShellView& v) {
        g.fill(p, panel);
        const uint32_t held = v.pad ? v.pad->held : 0;
        auto col = [&](uint32_t bits) { return (held & bits) ? accent : bg; };
        auto txt = [&](uint32_t bits) { return (held & bits) ? bg : text; };
        Font& sf = g.small();

        struct Sh { Btn b; int x, y, h; };
        for (Sh s : {Sh{ZL, 40, 18, 34}, Sh{L, 40, 58, 28}, Sh{ZR, 250, 18, 34}, Sh{R, 250, 58, 28}}) {
            Rect r{p.x + s.x, p.y + s.y, 110, s.h};
            g.fill(r, col(s.b));
            g.printCentered(sf, r.x + r.w / 2, r.y + (s.h - sf.lineH()) / 2, btnLabel(s.b), txt(s.b));
        }
        Rect body{p.x + 20, p.y + 96, p.w - 40, 236};
        g.fill(body, panelHi);
        const int lx = p.x + 105, rx = p.x + p.w - 105, topY = body.y + 72, botY = body.y + 170;

        auto stick = [&](int cx, int cy, float x, float y, Btn click) {
            g.fillCircle(cx, cy, 34, col(click));
            g.ring(cx, cy, 34, 2, dim);
            g.fillCircle(cx + x * 22, cy + y * 22, 10, text);
        };
        stick(lx, topY, v.pad ? v.pad->lx : 0, v.pad ? v.pad->ly : 0, LStick);
        stick(rx, botY, v.pad ? v.pad->rx : 0, v.pad ? v.pad->ry : 0, RStick);

        struct D { Btn b; int dx, dy; };
        for (D d : {D{DUp, 0, -1}, D{DDown, 0, 1}, D{DLeft, -1, 0}, D{DRight, 1, 0}}) {
            Rect r{lx + d.dx * 32 - 15, botY + d.dy * 32 - 15, 30, 30};
            g.fill(r, col(d.b));
            g.arrow(r.x + 15, r.y + 15, 12, dirOf(d.b), txt(d.b));
        }
        g.fill({lx - 15, botY - 15, 30, 30}, bg);
        for (D d : {D{North, 0, -1}, D{South, 0, 1}, D{West, -1, 0}, D{East, 1, 0}}) {
            int cx = rx + d.dx * 36, cy = topY + d.dy * 36;
            g.fillCircle(cx, cy, 19, col(d.b));
            g.printCentered(sf, cx, cy - sf.lineH() / 2, btnLabel(d.b), txt(d.b));
        }
        for (D d : {D{Minus, -1, 0}, D{Plus, 1, 0}}) {
            int cx = p.x + p.w / 2 + d.dx * 34, cy = body.y + 34;
            g.fillCircle(cx, cy, 13, col(d.b));
            g.printCentered(sf, cx, cy - sf.lineH() / 2, btnLabel(d.b), txt(d.b));
        }
        int y = body.y + body.h + 24;
        for (const std::string& line : v.help) {
            g.print(sf, p.x + 20, y, line, dim);
            y += sf.lineH() + 2;
        }
    }
};
}  // namespace

Skin* makeClassic() { static Classic s; return &s; }
