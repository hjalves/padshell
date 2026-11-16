// Visual style selection, shared by the shell and every app, plus the Metro
// palette. The choice is stored in <dataDir>/settings.ini.
#pragma once
#include <string>
#include <vector>

#include "gfx.h"

namespace ui {

enum class Style { Classic, Metro };

Style style();
void setStyle(Style s);            // also saves the setting
const char* styleName(Style s);
void loadSettings(const std::string& path);

// Metro / TV: black canvas, flat solid tiles, light type, wide safe margins.
namespace metro {
constexpr int kMarginX = 64, kMarginY = 36;  // TV-safe area at 1280x720
constexpr Color bg{0, 0, 0}, text{255, 255, 255}, dim{140, 140, 140}, faint{38, 38, 38};

// Classic Metro accent colours.
constexpr Color lime{164, 196, 0}, green{96, 169, 23}, emerald{0, 138, 0}, teal{0, 171, 169};
constexpr Color cyan{27, 161, 226}, cobalt{0, 80, 239}, indigo{106, 0, 255}, violet{170, 0, 255};
constexpr Color pink{244, 114, 208}, magenta{216, 0, 115}, crimson{162, 0, 37}, red{229, 20, 0};
constexpr Color orange{250, 104, 0}, amber{240, 163, 10}, olive{109, 135, 100}, steel{100, 118, 135};

inline Color mix(Color a, Color b, float t) {
    auto m = [t](uint8_t x, uint8_t y) { return (uint8_t)(x + (y - x) * t); };
    return {m(a.r, b.r), m(a.g, b.g), m(a.b, b.b), a.a};
}
// Full-screen Metro list menu: small caps header, big light items, the
// selected one white with an accent bar.
void drawMenu(Gfx& g, const char* header, const std::vector<std::string>& items, int sel, Color accent);
}  // namespace metro

}  // namespace ui
