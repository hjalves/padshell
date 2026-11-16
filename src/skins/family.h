// Building blocks shared by the Metro-family skins, after the "ameno" web
// app's design: each button has its own colour, used everywhere it appears
// (badges, labels, the pending character, the cycle), document characters
// are coloured by kind, and a shrinking bar shows when the pending character
// will be committed.
#pragma once
#include <SDL.h>

#include <algorithm>
#include <cctype>
#include <utility>
#include <vector>
#include <cstring>
#include <string>

#include "../skin.h"
#include "../utf8.h"

namespace fam {
using namespace skins;

// A theme: colours plus the monospaced font pair. Button colours are by
// position (Xbox A, B, X, Y = bottom, right, left, top; on a Switch
// controller those positions are B, A, Y, X).
struct Palette {
    Color bg, surface, raised, muted, text, dark;
    // Mode colours: letters, symbols (ZL), commands (ZR), caps tag.
    Color letters, symbols, commands, caps;
    Color btn[8];  // South, East, West, North, D-pad down, left, up, right
    Color chr[6];  // document: capitals, digits, brackets, quotes, operators, other punctuation
    bool light;
    const char* mono = "DejaVuSansMono.ttf";
    const char* bold = "DejaVuSansMono-Bold.ttf";
};

// All dark themes are vivid and high-contrast so they survive cheap TV
// panels: near-black backgrounds, saturated accents (>= 4.9:1 on the
// background, and for dark text on them) and greys at >= 6.8:1. The light
// themes use darkened "ink" versions of the colours for text.
namespace themes {
inline const Palette midnight{{5, 4, 10}, {24, 21, 36}, {48, 43, 72}, {150, 145, 175}, {255, 255, 255}, {5, 4, 10},
    {255, 45, 150}, {0, 230, 110}, {255, 45, 75}, {255, 224, 0},
    {{0, 230, 110}, {255, 45, 75}, {0, 176, 255}, {255, 224, 0}, {255, 140, 0}, {160, 100, 255}, {0, 230, 190}, {255, 60, 220}},
    {{0, 230, 190}, {255, 224, 0}, {255, 60, 220}, {0, 230, 110}, {0, 176, 255}, {170, 120, 255}}, false};

inline const Palette ocean{{2, 10, 22}, {14, 26, 44}, {30, 50, 78}, {140, 168, 196}, {238, 246, 255}, {2, 10, 22},
    {0, 200, 255}, {0, 230, 150}, {255, 70, 95}, {255, 214, 0},
    {{0, 230, 150}, {255, 70, 95}, {60, 140, 255}, {255, 214, 0}, {255, 150, 40}, {150, 120, 255}, {0, 225, 215}, {255, 90, 200}},
    {{0, 225, 215}, {255, 214, 0}, {255, 90, 200}, {0, 230, 150}, {80, 170, 255}, {160, 140, 255}}, false,
    "JetBrainsMono-Regular.ttf", "JetBrainsMono-Bold.ttf"};

inline const Palette carbon{{8, 8, 8}, {32, 32, 32}, {57, 57, 57}, {168, 168, 168}, {244, 244, 244}, {8, 8, 8},
    {69, 137, 255}, {66, 190, 101}, {250, 77, 86}, {241, 194, 27},
    {{66, 190, 101}, {250, 77, 86}, {51, 177, 255}, {241, 194, 27}, {255, 131, 43}, {165, 110, 255}, {8, 189, 186}, {255, 126, 182}},
    {{8, 189, 186}, {241, 194, 27}, {255, 126, 182}, {66, 190, 101}, {51, 177, 255}, {165, 110, 255}}, false,
    "IBMPlexMono-Regular.ttf", "IBMPlexMono-Bold.ttf"};

inline const Palette neon{{0, 0, 0}, {22, 12, 30}, {46, 26, 62}, {175, 155, 205}, {255, 255, 255}, {0, 0, 0},
    {255, 0, 170}, {57, 255, 20}, {255, 20, 60}, {255, 240, 0},
    {{57, 255, 20}, {255, 20, 60}, {0, 200, 255}, {255, 240, 0}, {255, 110, 0}, {160, 80, 255}, {0, 255, 200}, {255, 0, 200}},
    {{0, 255, 200}, {255, 240, 0}, {255, 0, 200}, {57, 255, 20}, {0, 200, 255}, {185, 130, 255}}, false,
    "FiraMono-Regular.ttf", "FiraMono-Bold.ttf"};

inline const Palette ember{{12, 6, 4}, {34, 19, 13}, {64, 38, 26}, {195, 165, 145}, {255, 244, 232}, {12, 6, 4},
    {255, 100, 60}, {160, 225, 40}, {240, 40, 70}, {255, 196, 0},
    {{160, 225, 40}, {240, 40, 70}, {40, 185, 230}, {255, 196, 0}, {255, 135, 0}, {225, 110, 215}, {0, 205, 175}, {255, 85, 145}},
    {{255, 170, 60}, {255, 196, 0}, {255, 85, 145}, {160, 225, 40}, {40, 185, 230}, {225, 125, 210}}, false,
    "SpaceMono-Regular.ttf", "SpaceMono-Bold.ttf"};

inline const Palette day{{248, 246, 240}, {232, 229, 220}, {212, 208, 198}, {98, 94, 88}, {20, 18, 24}, {20, 18, 24},
    {255, 60, 150}, {0, 190, 95}, {255, 75, 90}, {245, 185, 0},
    {{0, 190, 95}, {240, 55, 70}, {0, 150, 240}, {245, 185, 0}, {255, 125, 0}, {140, 95, 255}, {0, 185, 170}, {230, 60, 180}},
    {{0, 125, 115}, {140, 90, 0}, {185, 20, 140}, {0, 130, 55}, {0, 105, 185}, {105, 60, 215}}, true,
    "CourierPrime-Regular.ttf", "CourierPrime-Bold.ttf"};

// Used by the "light" (tile) skin.
inline const Palette daylight{{246, 245, 241}, {232, 230, 224}, {214, 211, 204}, {139, 135, 153}, {27, 26, 34}, {27, 26, 34},
    {224, 53, 138}, {31, 174, 96}, {224, 52, 69}, {214, 160, 0},
    {{31, 174, 96}, {224, 52, 69}, {29, 155, 209}, {214, 160, 0}, {232, 128, 0}, {124, 77, 255}, {15, 170, 145}, {214, 58, 176}},
    {{15, 150, 128}, {190, 140, 0}, {200, 50, 165}, {31, 160, 90}, {29, 140, 200}, {124, 77, 255}}, true};
}  // namespace themes

inline const Palette& darkPalette() { return themes::midnight; }

inline int buttonIndex(Btn b) {
    switch (b) {
        case South: return 0;
        case East: return 1;
        case West: return 2;
        case North: return 3;
        case DDown: return 4;
        case DLeft: return 5;
        case DUp: return 6;
        case DRight: return 7;
        default: return -1;
    }
}

// Identity colour of each button position.
inline Color buttonColor(Btn b, const Palette& p) {
    int i = buttonIndex(b);
    return i < 0 ? p.letters : p.btn[i];
}

// A colour used as text: as-is on dark themes, darkened on light ones.
inline Color ink(Color c, const Palette& p) { return p.light ? mix(c, p.text, 0.5f) : c; }

// Document character colours by kind.
inline Color charColor(uint32_t c, const Palette& p) {
    if (c >= 'a' && c <= 'z') return p.text;
    if (c >= 'A' && c <= 'Z') return p.chr[0];
    if (c >= '0' && c <= '9') return p.chr[1];
    if (c < 128 && std::strchr("([{<)]}>", (int)c)) return p.chr[2];
    if (c == '"' || c == '\'') return p.chr[3];
    if (c < 128 && std::strchr("-=+*/^~#%@", (int)c)) return p.chr[4];
    return p.chr[5];
}

inline Color modeColor(MultiTap::Mode m, const Palette& p) {
    return m == MultiTap::Mode::Symbols ? p.symbols : m == MultiTap::Mode::Commands ? p.commands : p.letters;
}

// The button whose group produced the pending character, or 0.
inline Btn pendingButton(const EditorView& v) {
    if (!v.tap->hasPending()) return (Btn)0;
    for (const auto* gs : {&MultiTap::letters(), &MultiTap::symbols()})
        for (const auto& gr : *gs)
            if (gr.chars == v.tap->pendingGroup()) return gr.button;
    return (Btn)0;
}

// Fraction of the commit delay left for the pending character (1 -> 0).
inline float timerLeft(const EditorView& v) {
    if (!v.tap->hasPending()) return 0;
    float t = (SDL_GetTicks() - v.tap->lastInput()) / (float)MultiTap::kCommitMs;
    return std::clamp(1.0f - t, 0.0f, 1.0f);
}

inline bool blinkOn() { return (SDL_GetTicks() % 1100) < 660; }

// Spec order of the character buttons (A X Y B, then down left up right in
// Xbox terms), used by the status-line layouts.
inline const Btn* specOrder() {
    static const Btn o[8] = {South, West, North, East, DDown, DLeft, DUp, DRight};
    return o;
}

// A flat square badge in `fill`, with the button's label (or a D-pad arrow).
inline void badge(Gfx& g, int x, int y, int size, Btn b, Color fill, Color fg,
                  const char* boldFont = "DejaVuSansMono-Bold.ttf") {
    g.fill({x, y, size, size}, fill);
    if (isDpad(b)) {
        g.arrow(x + size / 2.f, y + size / 2.f, size * 0.42f, dirOf(b), fg);
    } else {
        Font& f = g.font(boldFont, std::max(10, size * 3 / 5));
        g.drawCentered(f, x + size / 2, y + (size - f.lineH()) / 2, btnLabel(b), fg);
    }
}

// A flat tag such as "ABC" or the mode name.
inline int tag(Gfx& g, Font& f, int x, int y, int h, const std::string& s, Color fill, Color fg) {
    int w = Gfx::measure(f, s) + h / 2 + 6;
    g.fill({x, y, w, h}, fill);
    g.draw(f, x + (h / 4) + 3, y + (h - f.lineH()) / 2, s, fg);
    return w;
}

struct DocStyle {
    Font* font;
    int lineH;        // >= font->lineH()
    int cursorH = 6;  // underline thickness
    bool colourChars = true;
};

// Wrapped document following the end, with coloured characters, an underline
// cursor in the mode colour, and the pending character in its button colour.
inline void document(Gfx& g, Rect r, const EditorView& v, const Palette& p, const DocStyle& s) {
    Font& f = *s.font;
    const int cols = std::max(1, r.w / f.cellW()), visible = std::max(1, r.h / s.lineH);
    const auto rows = v.wrap(cols);
    const int top = std::max(0, (int)rows.size() - visible);
    const int dy = (s.lineH - f.lineH()) / 2;
    for (int i = top; i < (int)rows.size() && i < top + visible; ++i) {
        std::string_view t = v.rowText(rows[i]);
        int x = r.x, y = r.y + (i - top) * s.lineH + dy;
        for (size_t k = 0; k < t.size(); k = utf8::next(t, k)) {
            uint32_t cp = utf8::decode(t, k);
            g.print(f, x, y, t.substr(k, utf8::next(t, k) - k), s.colourChars ? charColor(cp, p) : p.text);
            x += f.cellW();
        }
    }
    const int cx = r.x + (v.endColumn() % cols) * f.cellW();
    const int cy = r.y + ((int)rows.size() - 1 - top) * s.lineH;
    const Color mc = modeColor(v.mode(), p);
    if (v.tap->hasPending()) {
        g.fill({cx, cy, f.cellW(), s.lineH}, p.surface);
        g.print(f, cx, cy + dy, v.tap->pendingText(), ink(buttonColor(pendingButton(v), p), p));
        g.fill({cx, cy + s.lineH - s.cursorH, f.cellW(), s.cursorH}, mc);
    } else if (blinkOn()) {
        g.fill({cx, cy + s.lineH - s.cursorH, f.cellW(), s.cursorH}, mc);
    }
}

// The pending character's cycle as cells: current one filled with the button
// colour, the rest in it as text. Returns the width used.
inline int cycle(Gfx& g, int x, int y, int cell, const EditorView& v, const Palette& p) {
    const MultiTap& tap = *v.tap;
    const Color bc = buttonColor(pendingButton(v), p);
    Font& f = g.font(p.mono, cell * 5 / 7);
    Font& fb = g.font(p.bold, cell * 5 / 7);
    const char* grp = tap.pendingGroup();
    int x0 = x;
    for (int i = 0; grp[i]; ++i) {
        bool cur = i == tap.pendingIndex();
        if (cur) g.fill({x, y, cell, cell}, bc);
        g.drawCentered(cur ? fb : f, x + cell / 2, y + (cell - f.lineH()) / 2, tap.display(grp[i]), cur ? p.dark : ink(bc, p));
        x += cell;
    }
    return x - x0;
}

// Badges with their group (or command) in the button colour, in spec order.
// `quiet` draws everything in the muted colour. Returns the width used.
inline int groupsLine(Gfx& g, int x, int y, int size, Font& f, const EditorView& v, const Palette& p,
                      bool quiet = false) {
    const int x0 = x;
    const bool commands = v.mode() == MultiTap::Mode::Commands;
    for (int i = 0; i < 8; ++i) {
        const Btn b = specOrder()[i];
        std::string label;
        if (commands) {
            if (const auto* c = commandFor(b)) label = c->label;
            else continue;
        } else {
            label = groupText(*v.tap, groupFor(v.mode(), b)->chars);
        }
        const Color bc = quiet ? p.raised : buttonColor(b, p);
        const bool held = v.held & b;
        badge(g, x, y, size, b, held ? p.text : bc, quiet ? p.muted : held ? p.bg : p.dark, p.bold);
        x += size + size / 4;
        x += g.draw(f, x, y + (size - f.lineH()) / 2, label, quiet ? p.muted : ink(bc, p)) + size * 2 / 3;
    }
    return x - x0;
}

// "ABC" caps tag and the mode tag. Returns the width used (drawn right-aligned
// so it ends at `right`).
inline int modeTags(Gfx& g, int right, int y, int h, Font& f, const EditorView& v, const Palette& p) {
    const bool caps = v.tap->uppercase();
    const char* name = modeName(v.mode(), false);
    int wm = Gfx::measure(f, name) + h / 2 + 6, wc = Gfx::measure(f, "ABC") + h / 2 + 6;
    int x = right - wm - wc - 10;
    tag(g, f, x, y, h, "ABC", caps ? p.caps : p.surface, caps ? p.dark : p.muted);
    tag(g, f, x + wc + 10, y, h, name, modeColor(v.mode(), p), p.dark);
    return right - x;
}

constexpr int kMarginX = 64, kMarginY = 36;  // TV-safe area at 1280x720

// Metro header: small caps brand, large light heading, clock. Returns the
// area below it.
inline Rect metroHeader(Gfx& g, const ShellView& v, const Palette& p) {
    g.clear(p.bg);
    g.draw(g.font("DejaVuSans.ttf", 17), kMarginX, kMarginY, "PADSHELL", p.letters);
    g.draw(g.font("DejaVuSans-ExtraLight.ttf", 60), kMarginX - 4, kMarginY + 14, v.heading, p.text);
    Font& c = g.font("DejaVuSans-ExtraLight.ttf", 44);
    g.draw(c, Gfx::W - kMarginX - Gfx::measure(c, v.clock), kMarginY + 8, v.clock, p.text);
    if (!v.toast.empty()) {
        Font& t = g.font("DejaVuSans.ttf", 20);
        g.draw(t, Gfx::W - kMarginX - Gfx::measure(t, v.toast), kMarginY + 62, v.toast, p.caps);
    }
    return {kMarginX, kMarginY + 104, Gfx::W - 2 * kMarginX, Gfx::H - (kMarginY + 104) - kMarginY};
}

// Full-screen Metro list; the selection is a filled bar (ameno style).
inline void metroMenu(Gfx& g, const Palette& p, const std::string& title, const std::vector<std::string>& items,
                      int sel) {
    g.fill({0, 0, Gfx::W, Gfx::H}, withAlpha(p.bg, 248));
    std::string t = title;
    for (char& c : t) c = (char)std::toupper((unsigned char)c);
    g.draw(g.font("DejaVuSans.ttf", 22), kMarginX, kMarginY + 64, t, p.muted);
    Font& f = g.font("DejaVuSans-ExtraLight.ttf", 44);
    int y = kMarginY + 116;
    for (int i = 0; i < (int)items.size(); ++i) {
        const bool on = i == sel;
        if (on) g.fill({kMarginX, y, 560, f.lineH() + 10}, p.letters);
        g.draw(f, kMarginX + 24, y + 5, items[i], on ? p.dark : p.text);
        y += f.lineH() + 22;
    }
    Font& h = g.font("DejaVuSans.ttf", 20);
    int x = kMarginX, hy = Gfx::H - kMarginY - 30;
    for (auto [b, what] : {std::pair{kConfirm, "select"}, std::pair{kBack, "cancel"}}) {
        badge(g, x, hy, 30, b, buttonColor(b, p), p.dark, p.bold);
        x += 30 + 10 + g.draw(h, x + 40, hy + (30 - h.lineH()) / 2, what, p.muted) + 30;
    }
}
}  // namespace fam
