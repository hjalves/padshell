// Metro: a 10-foot TV layout. Black canvas, large light lowercase heading,
// big monospace text, and the character groups as flat colour tiles in
// controller order, inside TV-safe margins.
#include <algorithm>
#include <cctype>
#include <cstring>

#include "../skin.h"

namespace {
using namespace skins;

constexpr int kMarginX = 64, kMarginY = 36;
constexpr Color bg{0, 0, 0}, text{255, 255, 255}, dim{140, 140, 140}, faint{38, 38, 38};
constexpr Color green{96, 169, 23}, emerald{0, 138, 0}, teal{0, 171, 169}, cyan{27, 161, 226};
constexpr Color cobalt{0, 80, 239}, violet{170, 0, 255}, pink{244, 114, 208}, magenta{216, 0, 115};
constexpr Color crimson{162, 0, 37}, red{229, 20, 0}, orange{250, 104, 0}, amber{240, 163, 10};
constexpr Color olive{109, 135, 100};

constexpr Btn kTileOrder[8] = {DLeft, DUp, DDown, DRight, West, North, South, East};

Color cursorColor(MultiTap::Mode m) {
    return m == MultiTap::Mode::Commands ? red : m == MultiTap::Mode::Symbols ? green : pink;
}

Color tileColor(MultiTap::Mode m, Btn b) {
    switch (m) {
        case MultiTap::Mode::Letters:
            switch (b) {
                case DLeft: return teal;     case DUp: return emerald;
                case DDown: return cobalt;   case DRight: return violet;
                case West: return orange;    case North: return amber;
                case South: return magenta;  default: return cyan;
            }
        case MultiTap::Mode::Symbols:
            switch (b) {
                case DLeft: return emerald;  case DUp: return green;
                case DDown: return teal;     case DRight: return olive;
                case West: return Color{51, 153, 51};  case North: return Color{130, 160, 0};
                case South: return Color{0, 128, 96};  default: return Color{0, 150, 136};
            }
        case MultiTap::Mode::Commands:
            switch (b) {
                case South: return red;      case West: return orange;
                case East: return crimson;   case North: return magenta;
                default: return faint;
            }
    }
    return faint;
}

class Metro : public Skin {
public:
    const char* name() const override { return "metro"; }

    Rect frame(Gfx& g, const ShellView& v) override {
        g.clear(bg);
        const int x = kMarginX, y = kMarginY;
        g.draw(g.sansSmall(), x, y, "PADSHELL", cyan);
        g.draw(g.light(), x - 4, y + 14, v.heading, text);
        int cw = Gfx::measure(g.lightMid(), v.clock);
        g.draw(g.lightMid(), Gfx::W - kMarginX - cw, y + 8, v.clock, text);
        if (!v.toast.empty())
            g.draw(g.sans(), Gfx::W - kMarginX - Gfx::measure(g.sans(), v.toast), y + 62, v.toast, amber);
        return {x, y + 104, Gfx::W - 2 * x, Gfx::H - (y + 104) - kMarginY};
    }

    void editor(Gfx& g, Rect a, const EditorView& v) override {
        const int tileH = 116, chipH = 34;
        Rect strip{a.x, a.y + a.h - tileH, a.w, tileH};
        const int chipY = strip.y - chipH - 18;
        Rect ta{a.x, a.y, a.w, chipY - 20 - a.y};
        const MultiTap& tap = *v.tap;
        const Color mc = cursorColor(v.mode());

        Font& f = g.monoTV();
        const int cols = std::max(1, ta.w / f.cellW());
        const int visible = std::max(1, ta.h / f.lineH());
        const auto rows = v.wrap(cols);
        const int top = std::max(0, (int)rows.size() - visible);
        for (int r = top; r < (int)rows.size() && r < top + visible; ++r)
            g.print(f, ta.x, ta.y + (r - top) * f.lineH(), v.rowText(rows[r]), text);
        if (v.empty() && !tap.hasPending())
            g.draw(g.lightMid(), ta.x + f.cellW() + 12, ta.y - 6, "start typing", Color{70, 70, 70});

        int cx = ta.x + (v.endColumn() % cols) * f.cellW();
        int cy = ta.y + ((int)rows.size() - 1 - top) * f.lineH();
        if (tap.hasPending()) {
            g.fill({cx, cy, f.cellW(), f.lineH()}, mc);
            g.print(f, cx, cy, tap.pendingText(), bg);
        }
        g.fill({cx, cy + f.lineH() - 6, f.cellW(), 6}, mc);

        struct Chip { std::string label; bool on; Color c; };
        const Chip chips[] = {
            {std::string(btnLabel(ZL)) + "  symbols", v.mode() == MultiTap::Mode::Symbols, green},
            {std::string(btnLabel(ZR)) + "  edit", v.mode() == MultiTap::Mode::Commands, red},
            {std::string(btnLabel(L)) + "  shift", tap.uppercase(), pink},
        };
        int x = a.x;
        for (const Chip& c : chips) {
            int w = Gfx::measure(g.sans(), c.label) + 28;
            g.fill({x, chipY, w, chipH}, c.on ? c.c : faint);
            g.draw(g.sans(), x + 14, chipY + (chipH - g.sans().lineH()) / 2, c.label, c.on ? text : dim);
            x += w + 10;
        }
        std::string right = std::string(v.modified ? "unsaved" : "saved") + "      +  menu      -  apps";
        int rw = Gfx::measure(g.sans(), right);
        g.draw(g.sans(), a.x + a.w - rw, chipY + (chipH - g.sans().lineH()) / 2, right, dim);
        if (v.modified) g.fill({a.x + a.w - rw - 22, chipY + chipH / 2 - 5, 10, 10}, amber);

        tiles(g, strip, v);
    }

    void menu(Gfx& g, const std::string& title, const std::vector<std::string>& items, int sel) override {
        g.fill({0, 0, Gfx::W, Gfx::H}, {0, 0, 0, 245});
        int x = kMarginX, y = kMarginY + 64;
        std::string t = title;
        for (char& c : t) c = (char)std::toupper((unsigned char)c);
        g.draw(g.sans(), x, y, t, cyan);
        y += 56;
        for (int i = 0; i < (int)items.size(); ++i) {
            bool on = i == sel;
            if (on) g.fill({x, y + 12, 8, g.lightMid().lineH() - 18}, cyan);
            g.draw(g.lightMid(), x + 28, y, items[i], on ? text : dim);
            y += g.lightMid().lineH() + 10;
        }
        std::string hint = std::string(btnLabel(kConfirm)) + "  select      " + btnLabel(kBack) + "  cancel";
        g.draw(g.sans(), x, Gfx::H - kMarginY - g.sans().lineH(), hint, dim);
    }

private:
    void tiles(Gfx& g, Rect strip, const EditorView& v) {
        const int gap = 8, groupGap = 48;
        const int tileW = (strip.w - 6 * gap - groupGap) / 8;
        const MultiTap& tap = *v.tap;
        Font& mf = g.monoTile();
        for (int i = 0; i < 8; ++i) {
            const Btn b = kTileOrder[i];
            Rect t{strip.x + i * (tileW + gap) + (i >= 4 ? groupGap - gap : 0), strip.y, tileW, strip.h};
            Color c = tileColor(v.mode(), b);
            g.fill(t, (v.held & b) ? mix(c, text, 0.35f) : c);
            buttonLabel(g, g.sansSmall(), t.x + 12, t.y + 8, b, Color{255, 255, 255, 190});

            if (v.mode() == MultiTap::Mode::Commands) {
                const auto* cmd = commandFor(b);
                if (!cmd) continue;
                std::string l1 = cmd->label, l2;
                Font* lf = &g.sans();
                if (Gfx::measure(*lf, l1) > t.w - 24) {
                    if (l1.find(' ') != std::string::npos) {
                        l2 = l1.substr(l1.find(' ') + 1);
                        l1 = l1.substr(0, l1.find(' '));
                    } else {
                        lf = &g.sansSmall();
                    }
                }
                int lh = lf->lineH();
                int y = t.y + t.h - 12 - (l2.empty() ? 1 : 2) * lh;
                g.draw(*lf, t.x + 12, y, l1, text);
                if (!l2.empty()) g.draw(*lf, t.x + 12, y + lh, l2, text);
                continue;
            }
            const auto* grp = groupFor(v.mode(), b);
            if (!grp) continue;
            const int cur = tap.hasPending() && tap.pendingGroup() == grp->chars ? tap.pendingIndex() : -1;
            const int n = (int)std::strlen(grp->chars);
            const int perLine = n > 5 ? (n + 1) / 2 : n;
            const int nLines = (n + perLine - 1) / perLine;
            const int step = mf.cellW() + 3;
            for (int k = 0; k < n; ++k) {
                int lx = t.x + 12 + (k % perLine) * step;
                int ly = t.y + t.h - 10 - (nLines - k / perLine) * mf.lineH();
                if (k == cur) g.fill({lx - 2, ly + 2, mf.cellW() + 4, mf.lineH() - 2}, text);
                g.print(mf, lx, ly, tap.display(grp->chars[k]), k == cur ? c : text);
            }
        }
    }
};
}  // namespace

Skin* makeMetro() { static Metro s; return &s; }
