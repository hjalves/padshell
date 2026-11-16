// Drawing for the input methods, kept apart from ime.cpp so the input logic
// can be unit tested without SDL.
#include <cmath>
#include <string>

#include "gfx.h"
#include "ime.h"

void DaisyWheel::render(Gfx& g, Rect a) const {
    g.fill(a, theme::panel);

    const float cx = a.x + a.w / 2.f;
    const float cy = a.y + 30 + 175.f;
    const float petalR = 130, bubbleR = 46, spread = 22;
    Font& f = g.large();

    g.ring(cx, cy, 34, 2, theme::panelHi);
    if (petal_ < 0) g.fillCircle(cx, cy, 30, theme::panelHi);

    for (int p = 0; p < kPetals; ++p) {
        float ang = p * (float)M_PI / 4;
        float px = cx + petalR * std::sin(ang), py = cy - petalR * std::cos(ang);
        bool active = p == petal_;
        g.fillCircle(px, py, bubbleR, active ? theme::accent : theme::panelHi);
        static const int dx[kPerPetal] = {0, 1, 0, -1}, dy[kPerPetal] = {-1, 0, 1, 0};
        for (int s = 0; s < kPerPetal; ++s) {
            std::string ch = charAt(p, s);
            int x = (int)(px + dx[s] * spread) - f.cellW() / 2;
            int y = (int)(py + dy[s] * spread) - f.lineH() / 2;
            g.print(f, x, y, ch, active ? theme::bg : theme::text);
        }
    }

    // Layer tabs.
    int ty = a.y + 420;
    int tabW = 80, gap = 10, total = (int)layers().size() * tabW + ((int)layers().size() - 1) * gap;
    int tx = a.x + (a.w - total) / 2;
    for (int i = 0; i < (int)layers().size(); ++i) {
        Rect tab{tx + i * (tabW + gap), ty, tabW, 30};
        bool on = i == layer_;
        g.fill(tab, on ? theme::accent : theme::panelHi);
        g.printCentered(g.small(), tab.x + tabW / 2, ty + 4, layers()[i].name, on ? theme::bg : theme::dim);
    }
    g.printCentered(g.small(), (int)cx, ty + 38, shift_ ? "SHIFT" : "L / R: layer   ZR: shift",
                    shift_ ? theme::warn : theme::dim);

    // Legend for the centred-stick actions.
    int ly = ty + 76;
    const char* rows[] = {
        "Stick centred:",
        "  South space     East backspace",
        "  North newline   West delete",
        "D-pad: move cursor",
        "ZL + D-pad: home / end / page",
    };
    for (const char* r : rows) {
        g.print(g.small(), a.x + 20, ly, r, theme::dim);
        ly += g.small().lineH();
    }
}
