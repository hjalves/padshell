// Building blocks for the ameno skins, after the "ameno" web
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

// Colours of a theme (the font is chosen separately; the skin fills in
// mono/bold before drawing). Button colours are by
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
    {{0, 225, 215}, {255, 214, 0}, {255, 90, 200}, {0, 230, 150}, {80, 170, 255}, {160, 140, 255}}, false};

inline const Palette carbon{{8, 8, 8}, {32, 32, 32}, {57, 57, 57}, {168, 168, 168}, {244, 244, 244}, {8, 8, 8},
    {69, 137, 255}, {66, 190, 101}, {250, 77, 86}, {241, 194, 27},
    {{66, 190, 101}, {250, 77, 86}, {51, 177, 255}, {241, 194, 27}, {255, 131, 43}, {165, 110, 255}, {8, 189, 186}, {255, 126, 182}},
    {{8, 189, 186}, {241, 194, 27}, {255, 126, 182}, {66, 190, 101}, {51, 177, 255}, {165, 110, 255}}, false};

inline const Palette neon{{0, 0, 0}, {22, 12, 30}, {46, 26, 62}, {175, 155, 205}, {255, 255, 255}, {0, 0, 0},
    {255, 0, 170}, {57, 255, 20}, {255, 20, 60}, {255, 240, 0},
    {{57, 255, 20}, {255, 20, 60}, {0, 200, 255}, {255, 240, 0}, {255, 110, 0}, {160, 80, 255}, {0, 255, 200}, {255, 0, 200}},
    {{0, 255, 200}, {255, 240, 0}, {255, 0, 200}, {57, 255, 20}, {0, 200, 255}, {185, 130, 255}}, false};

inline const Palette ember{{12, 6, 4}, {34, 19, 13}, {64, 38, 26}, {195, 165, 145}, {255, 244, 232}, {12, 6, 4},
    {255, 100, 60}, {160, 225, 40}, {240, 40, 70}, {255, 196, 0},
    {{160, 225, 40}, {240, 40, 70}, {40, 185, 230}, {255, 196, 0}, {255, 135, 0}, {225, 110, 215}, {0, 205, 175}, {255, 85, 145}},
    {{255, 170, 60}, {255, 196, 0}, {255, 85, 145}, {160, 225, 40}, {40, 185, 230}, {225, 125, 210}}, false};

inline const Palette day{{248, 246, 240}, {232, 229, 220}, {212, 208, 198}, {98, 94, 88}, {20, 18, 24}, {20, 18, 24},
    {255, 60, 150}, {0, 190, 95}, {255, 75, 90}, {245, 185, 0},
    {{0, 190, 95}, {240, 55, 70}, {0, 150, 240}, {245, 185, 0}, {255, 125, 0}, {140, 95, 255}, {0, 185, 170}, {230, 60, 180}},
    {{0, 125, 115}, {140, 90, 0}, {185, 20, 140}, {0, 130, 55}, {0, 105, 185}, {105, 60, 215}}, true};

// Variations of neon: black backgrounds, electric accents.
// pure black, electric cyan accent with yellow and magenta
inline const Palette cyber{{0, 0, 0}, {10, 20, 28}, {24, 44, 60}, {150, 185, 200}, {240, 255, 255}, {0, 0, 0},
    {0, 235, 255}, {57, 255, 20}, {255, 20, 90}, {255, 235, 0},
    {{57, 255, 20}, {255, 20, 90}, {30, 150, 255}, {255, 235, 0}, {255, 120, 0}, {190, 80, 255}, {0, 255, 210}, {255, 0, 170}},
    {{0, 235, 255}, {255, 235, 0}, {255, 0, 170}, {57, 255, 20}, {30, 160, 255}, {195, 130, 255}}, false};

// violet-black, hot pink accent, softer vaporwave neons
inline const Palette vapor{{6, 0, 12}, {26, 10, 38}, {52, 24, 74}, {185, 160, 215}, {255, 245, 255}, {6, 0, 12},
    {255, 90, 230}, {80, 255, 160}, {255, 60, 120}, {255, 230, 90},
    {{80, 255, 160}, {255, 60, 120}, {80, 200, 255}, {255, 230, 90}, {255, 150, 90}, {180, 110, 255}, {90, 255, 240}, {255, 110, 230}},
    {{90, 255, 240}, {255, 230, 90}, {255, 110, 230}, {80, 255, 160}, {80, 200, 255}, {195, 145, 255}}, false};

// pure black, hot orange accent with magenta and lime
inline const Palette sunset{{0, 0, 0}, {30, 12, 8}, {60, 28, 18}, {205, 165, 150}, {255, 248, 240}, {0, 0, 0},
    {255, 100, 0}, {120, 255, 40}, {255, 30, 80}, {255, 220, 0},
    {{120, 255, 40}, {255, 30, 80}, {0, 190, 255}, {255, 220, 0}, {255, 150, 0}, {200, 90, 255}, {0, 240, 200}, {255, 50, 170}},
    {{255, 170, 0}, {255, 220, 0}, {255, 50, 170}, {120, 255, 40}, {0, 190, 255}, {210, 125, 255}}, false};

// blue-black, electric periwinkle accent with cyan and mint
inline const Palette ice{{0, 2, 10}, {10, 20, 40}, {26, 42, 76}, {160, 178, 212}, {240, 248, 255}, {0, 2, 10},
    {90, 160, 255}, {0, 255, 170}, {255, 60, 110}, {255, 240, 80},
    {{0, 255, 170}, {255, 60, 110}, {40, 175, 255}, {255, 240, 80}, {255, 160, 60}, {150, 120, 255}, {0, 240, 255}, {230, 90, 255}},
    {{0, 240, 255}, {255, 240, 80}, {230, 90, 255}, {0, 255, 170}, {40, 175, 255}, {170, 150, 255}}, false};

// Variations of sunset: near-black backgrounds, hot accents.
// violet-black, violet accent: a dusk sky
inline const Palette dusk{{8, 2, 14}, {30, 12, 40}, {60, 26, 78}, {200, 165, 205}, {255, 244, 250}, {8, 2, 14},
    {200, 110, 255}, {120, 255, 60}, {255, 40, 70}, {255, 210, 40},
    {{120, 255, 60}, {255, 40, 70}, {60, 170, 255}, {255, 210, 40}, {255, 130, 40}, {230, 90, 255}, {40, 235, 210}, {255, 70, 170}},
    {{255, 150, 60}, {255, 210, 40}, {255, 70, 170}, {120, 255, 60}, {60, 170, 255}, {210, 130, 255}}, false};

// warm black, red-orange accent; edit mode is crimson-pink to stay distinct
inline const Palette lava{{8, 2, 0}, {34, 10, 4}, {68, 22, 10}, {210, 160, 140}, {255, 240, 228}, {8, 2, 0},
    {255, 90, 0}, {170, 255, 40}, {255, 0, 110}, {255, 220, 0},
    {{170, 255, 40}, {255, 0, 110}, {0, 180, 255}, {255, 220, 0}, {255, 140, 0}, {190, 90, 255}, {0, 235, 190}, {255, 60, 150}},
    {{255, 130, 40}, {255, 220, 0}, {255, 60, 150}, {170, 255, 40}, {0, 180, 255}, {215, 120, 255}}, false};

// black, amber accent; the caps tag is cyan to stand apart
inline const Palette gold{{4, 3, 0}, {28, 22, 6}, {58, 46, 14}, {205, 185, 140}, {255, 250, 235}, {4, 3, 0},
    {255, 190, 0}, {90, 255, 90}, {255, 40, 80}, {80, 220, 255},
    {{90, 255, 90}, {255, 40, 80}, {40, 170, 255}, {255, 235, 60}, {255, 130, 0}, {190, 110, 255}, {0, 230, 200}, {255, 80, 180}},
    {{255, 190, 0}, {255, 235, 60}, {255, 80, 180}, {90, 255, 90}, {40, 170, 255}, {200, 140, 255}}, false};

// teal-black, orange accent
inline const Palette tropic{{0, 8, 10}, {6, 28, 32}, {14, 54, 60}, {150, 200, 200}, {240, 255, 252}, {0, 8, 10},
    {255, 120, 40}, {60, 255, 120}, {255, 40, 90}, {255, 230, 0},
    {{60, 255, 120}, {255, 40, 90}, {0, 190, 255}, {255, 230, 0}, {255, 150, 40}, {170, 120, 255}, {0, 240, 220}, {255, 90, 190}},
    {{0, 240, 220}, {255, 230, 0}, {255, 90, 190}, {60, 255, 120}, {0, 190, 255}, {185, 145, 255}}, false};

// pink-black, rose accent
inline const Palette rose{{10, 0, 4}, {36, 8, 20}, {70, 18, 40}, {215, 165, 185}, {255, 240, 246}, {10, 0, 4},
    {255, 60, 130}, {110, 255, 70}, {255, 30, 30}, {255, 215, 0},
    {{110, 255, 70}, {255, 30, 30}, {50, 180, 255}, {255, 215, 0}, {255, 140, 40}, {200, 100, 255}, {0, 235, 200}, {255, 90, 200}},
    {{255, 140, 180}, {255, 215, 0}, {255, 90, 200}, {110, 255, 70}, {50, 180, 255}, {210, 130, 255}}, false};

// Lunar: pure black with a white accent (brand, cursor, menu, mode tag),
// pure white text, neutral greys and saturated cool button and text colours
// (all >= 6.4:1 on the black).
inline const Palette moon{{0, 0, 0}, {22, 22, 24}, {46, 46, 50}, {185, 187, 194}, {255, 255, 255}, {0, 0, 0},
    {255, 255, 255}, {40, 240, 135}, {255, 75, 100}, {255, 232, 50},
    {{40, 240, 135}, {255, 75, 100}, {60, 175, 255}, {255, 232, 50}, {255, 145, 30}, {165, 130, 255}, {30, 235, 225}, {255, 85, 225}},
    {{110, 225, 255}, {255, 232, 50}, {255, 85, 225}, {40, 240, 135}, {60, 175, 255}, {175, 145, 255}}, false};

// Dim, for dark rooms: pure black and everything held in a narrow
// 4.8-8:1 contrast band (other themes reach 20:1), warm white text and a
// soft amber accent. Colours keep their hues at reduced brightness.
inline const Palette night{{0, 0, 0}, {14, 12, 11}, {32, 29, 27}, {128, 120, 111}, {170, 158, 146}, {0, 0, 0},
    {191, 113, 45}, {37, 145, 74}, {226, 62, 71}, {152, 131, 36},
    {{36, 142, 72}, {226, 62, 71}, {45, 129, 193}, {142, 123, 33}, {180, 106, 28}, {141, 105, 205}, {24, 139, 126}, {202, 71, 171}},
    {{58, 140, 156}, {149, 129, 35}, {207, 73, 174}, {37, 145, 74}, {46, 132, 197}, {144, 107, 209}}, false};

// Deep space: pure black, white text, a pale blue accent and moon's cool
// colours. Made for the starfield (settings), which works with any theme.
inline const Palette space{{0, 0, 0}, {20, 22, 30}, {44, 48, 64}, {180, 188, 205}, {255, 255, 255}, {0, 0, 0},
    {170, 215, 255}, {40, 240, 135}, {255, 75, 100}, {255, 232, 50},
    {{40, 240, 135}, {255, 75, 100}, {60, 175, 255}, {255, 232, 50}, {255, 145, 30}, {165, 130, 255}, {30, 235, 225}, {255, 85, 225}},
    {{150, 215, 255}, {255, 232, 50}, {255, 85, 225}, {40, 240, 135}, {60, 175, 255}, {175, 145, 255}}, false};

}  // namespace themes

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

// Wrapped document with coloured characters, scrolled so the cursor stays
// visible (`top` is the first visible row, kept between frames), an
// underline cursor in the mode colour, and the pending character inserted at
// the cursor in its button colour.
inline void document(Gfx& g, Rect r, const EditorView& v, const Palette& p, const DocStyle& s, int& top) {
    Font& f = *s.font;
    const int cols = std::max(1, r.w / f.cellW()), visible = std::max(1, r.h / s.lineH);
    const EditorView::Layout L = v.layout(cols);
    const int rows = (int)L.rows.size();
    if (L.cursorRow < top) top = L.cursorRow;
    if (L.cursorRow >= top + visible) top = L.cursorRow - visible + 1;
    top = std::clamp(top, 0, std::max(0, rows - visible));  // no blank space after deletions

    const int dy = (s.lineH - f.lineH()) / 2;
    const bool pending = v.tap->hasPending();
    for (int i = top; i < rows && i < top + visible; ++i) {
        std::string_view t = L.rowText(L.rows[i]);
        int x = r.x, y = r.y + (i - top) * s.lineH;
        int col = 0;
        for (size_t k = 0; k < t.size(); k = utf8::next(t, k), ++col, x += f.cellW()) {
            uint32_t cp = utf8::decode(t, k);
            std::string_view ch = t.substr(k, utf8::next(t, k) - k);
            if (pending && i == L.cursorRow && col == L.cursorX) {
                g.fill({x, y, f.cellW(), s.lineH}, p.surface);
                g.print(f, x, y + dy, ch, ink(buttonColor(pendingButton(v), p), p));
            } else {
                g.print(f, x, y + dy, ch, s.colourChars ? charColor(cp, p) : p.text);
            }
        }
    }
    const int cx = r.x + L.cursorX * f.cellW();
    const int cy = r.y + (L.cursorRow - top) * s.lineH;
    // Solid while a character is pending or ZR (navigation) is held; blinks otherwise.
    if (pending || v.mode() == MultiTap::Mode::Commands || blinkOn())
        g.fill({cx, cy + s.lineH - s.cursorH, f.cellW(), s.cursorH}, modeColor(v.mode(), p));
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
// Returns the width used.
inline int groupsLine(Gfx& g, int x, int y, int size, Font& f, const EditorView& v, const Palette& p) {
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
        const Color bc = buttonColor(b, p);
        const bool held = v.held & b;
        badge(g, x, y, size, b, held ? p.text : bc, held ? p.bg : p.dark, p.bold);
        x += size + size / 4;
        x += g.draw(f, x, y + (size - f.lineH()) / 2, label, ink(bc, p)) + size * 2 / 3;
    }
    if (commands) {  // the D-pad moves the cursor
        for (Btn b : {DLeft, DUp, DDown, DRight}) {
            const bool held = v.held & b;
            badge(g, x, y, size, b, held ? p.text : buttonColor(b, p), held ? p.bg : p.dark, p.bold);
            x += size + size / 8;
        }
        x += size / 8 + g.draw(f, x + size / 8, y + (size - f.lineH()) / 2, "move cursor", p.text);
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

}  // namespace fam
