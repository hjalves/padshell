// Paper: a typewritten page on a walnut desk. Courier Prime on ruled cream
// paper with a red margin, round typewriter keycaps for the character groups,
// and rubber stamps for modes.
#include <algorithm>
#include <cctype>
#include <cstring>

#include "../skin.h"

namespace {
using namespace skins;

constexpr Color deskTop{70, 50, 36}, deskBottom{38, 26, 18};
constexpr Color paper{250, 246, 234}, ink{38, 38, 52}, inkDim{140, 134, 120}, rule{170, 195, 225, 130};
constexpr Color margin{225, 110, 110}, cream{240, 232, 210}, capInk{24, 24, 26}, chrome{196, 192, 180};
const char* kRegular = "CourierPrime-Regular.ttf";
const char* kBold = "CourierPrime-Bold.ttf";

Color inkFor(MultiTap::Mode m) {
    switch (m) {
        case MultiTap::Mode::Symbols: return {30, 125, 60};
        case MultiTap::Mode::Commands: return {200, 35, 35};
        default: return {195, 45, 130};
    }
}

class Paper : public Skin {
public:
    const char* name() const override { return "paper"; }

    Rect frame(Gfx& g, const ShellView& v) override {
        g.gradient({0, 0, Gfx::W, Gfx::H}, deskTop, deskTop, deskBottom, deskBottom);
        // Wood grain: a few faint horizontal streaks.
        for (int i = 0; i < 18; ++i) g.fill({0, 20 + i * 41, Gfx::W, 2}, {0, 0, 0, (uint8_t)(18 + (i * 7) % 20)});

        sheet_ = {150, 22, 980, 520};
        g.fill({sheet_.x + 8, sheet_.y + 10, sheet_.w, sheet_.h}, {0, 0, 0, 90});
        g.fill(sheet_, paper);
        g.fill({sheet_.x + 92, sheet_.y, 2, sheet_.h}, margin);

        Font& hb = g.font(kBold, 40);
        Font& hs = g.font(kRegular, 22);
        g.draw(hb, sheet_.x + 112, sheet_.y + 22, v.heading, ink);
        std::string right = v.toast.empty() ? v.clock : v.toast;
        g.draw(hs, sheet_.x + sheet_.w - 40 - Gfx::measure(hs, right), sheet_.y + 36, right,
               v.toast.empty() ? inkDim : Color{200, 35, 35});
        g.fill({sheet_.x + 112, sheet_.y + 78, sheet_.w - 152, 2}, ink);
        return {sheet_.x, sheet_.y, sheet_.w, Gfx::H - sheet_.y};
    }

    void editor(Gfx& g, Rect, const EditorView& v) override {
        const MultiTap& tap = *v.tap;
        Font& f = g.font(kRegular, 30);
        const int lh = f.lineH() + 4;
        Rect ta{sheet_.x + 112, sheet_.y + 96, sheet_.w - 152, sheet_.h - 96 - 44};
        const int cols = std::max(1, ta.w / f.cellW());
        const int visible = std::max(1, ta.h / lh);

        for (int i = 0; i < visible; ++i) g.fill({sheet_.x + 20, ta.y + (i + 1) * lh - 4, sheet_.w - 40, 1}, rule);
        const auto rows = v.wrap(cols);
        const int top = std::max(0, (int)rows.size() - visible);
        for (int r = top; r < (int)rows.size() && r < top + visible; ++r)
            g.print(f, ta.x, ta.y + (r - top) * lh, v.rowText(rows[r]), ink);

        int cx = ta.x + (v.endColumn() % cols) * f.cellW();
        int cy = ta.y + ((int)rows.size() - 1 - top) * lh;
        Color mc = inkFor(v.mode());
        if (tap.hasPending()) {
            g.fill({cx - 1, cy, f.cellW() + 2, lh - 4}, withAlpha(mc, 45));
            g.print(f, cx, cy, tap.pendingText(), mc);
        }
        g.fill({cx, cy + lh - 9, f.cellW(), 3}, mc);

        // Footer typed at the bottom of the page.
        Font& small = g.font(kRegular, 20);
        std::string foot = "- " + std::to_string(v.chars()) + " chars \xC2\xB7 " + std::to_string(v.lineCount()) +
                           " lines \xC2\xB7 " + (v.modified ? "unsaved" : "saved") + " -";
        g.drawCentered(small, sheet_.x + sheet_.w / 2, sheet_.y + sheet_.h - 34, foot, inkDim);

        // Rubber stamp for non-default modes.
        const char* stamp = v.mode() == MultiTap::Mode::Commands  ? "EDIT"
                            : v.mode() == MultiTap::Mode::Symbols ? "SYMBOLS"
                            : tap.uppercase()                     ? "CAPITALS"
                                                                  : nullptr;
        if (stamp) {
            Font& sf = g.font(kBold, 34);
            std::string s = std::string("[ ") + stamp + " ]";
            g.drawRotated(sf, sheet_.x + sheet_.w - 170, sheet_.y + 150, s, withAlpha(mc, 190), -12);
        }

        keys(g, v);
    }

    void menu(Gfx& g, const std::string& title, const std::vector<std::string>& items, int sel) override {
        g.fill({0, 0, Gfx::W, Gfx::H}, {0, 0, 0, 120});
        Font& f = g.font(kRegular, 32);
        int lh = f.lineH() + 8, w = 480, h = (int)items.size() * lh + 140;
        Rect card{(Gfx::W - w) / 2, (Gfx::H - h) / 2, w, h};
        g.fill({card.x + 8, card.y + 10, w, h}, {0, 0, 0, 90});
        g.fill(card, Color{253, 250, 240});
        g.fill({card.x, card.y + 66, w, 2}, margin);
        for (int y = card.y + 66 + lh; y < card.y + h - 10; y += lh) g.fill({card.x, y, w, 1}, rule);
        std::string t = title;
        for (char& c : t) c = (char)std::toupper((unsigned char)c);
        g.draw(g.font(kBold, 30), card.x + 30, card.y + 20, t, Color{200, 35, 35});
        int y = card.y + 66 + lh - f.lineH() - 4;
        for (int i = 0; i < (int)items.size(); ++i) {
            g.draw(f, card.x + 30, y, (i == sel ? "> " : "  ") + items[i], i == sel ? ink : inkDim);
            if (i == sel) g.fill({card.x + 30, y + f.lineH() - 2, Gfx::measure(f, "> " + items[i]), 3}, margin);
            y += lh;
        }
        std::string hint = std::string(btnLabel(kConfirm)) + " select   " + btnLabel(kBack) + " cancel";
        g.drawCentered(g.font(kRegular, 20), card.x + w / 2, card.y + h - 34, hint, inkDim);
    }

private:
    void keys(Gfx& g, const EditorView& v) {
        const MultiTap& tap = *v.tap;
        const Btn order[8] = {DLeft, DUp, DDown, DRight, West, North, South, East};
        const int r = 40, cy0 = 612, spacing = 112, clusterGap = 70;
        const int total = 7 * spacing + clusterGap - spacing + spacing;
        const int x0 = (Gfx::W - total) / 2;
        Font& lab = g.font(kBold, 18);
        for (int i = 0; i < 8; ++i) {
            const Btn b = order[i];
            const bool held = v.held & b;
            int cx = x0 + i * spacing + (i >= 4 ? clusterGap : 0), cy = cy0 + (held ? 4 : 0);
            if (!held) g.fillCircle(cx + 2, cy + 6, r + 2, {0, 0, 0, 110});
            g.fillCircle(cx, cy, r + 4, held ? mix(chrome, capInk, 0.3f) : chrome);
            g.fillCircle(cx, cy, r, capInk);

            // Label above the cap's centre.
            if (isDpad(b)) g.arrow(cx, cy - 22, 12, dirOf(b), cream);
            else g.drawCentered(lab, cx, cy - 33, btnLabel(b), cream);

            if (v.mode() == MultiTap::Mode::Commands) {
                if (const auto* c = commandFor(b)) {
                    Font& cf = g.font(kRegular, 15);
                    std::string l = c->label;
                    size_t sp = l.find(' ');
                    if (sp == std::string::npos) g.drawCentered(cf, cx, cy - 4, l, cream);
                    else {
                        g.drawCentered(cf, cx, cy - 10, l.substr(0, sp), cream);
                        g.drawCentered(cf, cx, cy + 6, l.substr(sp + 1), cream);
                    }
                }
                continue;
            }
            const auto* grp = groupFor(v.mode(), b);
            if (!grp) continue;
            int n = (int)std::strlen(grp->chars);
            int cur = tap.hasPending() && tap.pendingGroup() == grp->chars ? tap.pendingIndex() : -1;
            Font& cf = g.font(kBold, n > 5 ? 17 : 22);
            int perLine = n > 5 ? (n + 1) / 2 : n;
            int lines = (n + perLine - 1) / perLine;
            const int step = (int)(cf.lineH() * 0.85f);
            const int top = cy + 6 - (lines * step) / 2 - cf.lineH() / 2 + step / 2;
            for (int k = 0; k < n; ++k) {
                int line = k / perLine, idx = k % perLine;
                int inLine = std::min(perLine, n - line * perLine);
                int cw = cf.cellW() + 1;
                int x = cx - inLine * cw / 2 + idx * cw;
                int y = top + line * step;
                if (k == cur) g.fillRounded({x - 2, y + 3, cf.cellW() + 4, cf.lineH() - 5}, 4, cream);
                g.print(cf, x, y, tap.display(grp->chars[k]), k == cur ? inkFor(v.mode()) : cream);
            }
        }
        // Hints on the desk.
        Font& h = g.font(kRegular, 19);
        std::string left = std::string(btnLabel(ZL)) + " symbols   " + btnLabel(ZR) + " edit   " + btnLabel(L) + " capitals";
        g.draw(h, 40, Gfx::H - 32, left, Color{220, 205, 180, 200});
        std::string right = "+ menu   - apps";
        g.draw(h, Gfx::W - 40 - Gfx::measure(h, right), Gfx::H - 32, right, Color{220, 205, 180, 200});
    }

    Rect sheet_{150, 22, 980, 520};
};
}  // namespace

Skin* makePaper() { static Paper s; return &s; }
