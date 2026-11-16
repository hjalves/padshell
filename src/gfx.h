// Thin drawing layer over SDL_Renderer: rectangles, circles and cached
// monospace text. Everything the shell and apps draw goes through here.
#pragma once
#include <SDL.h>
#include <SDL_ttf.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>

struct Color { uint8_t r, g, b, a = 255; };
struct Rect { int x, y, w, h; };

namespace theme {
constexpr Color bg{24, 26, 33}, panel{34, 37, 46}, panelHi{48, 52, 64};
constexpr Color text{220, 223, 228}, dim{120, 126, 140}, accent{97, 175, 239};
constexpr Color warn{229, 192, 123}, cursor{255, 204, 102};
}  // namespace theme

// A font with two texture caches, both rendered white and tinted at draw
// time: per-codepoint glyphs for monospace grids, and whole strings for
// proportional text (cleared when it grows large).
class Font {
public:
    bool open(SDL_Renderer* r, const std::string& path, int px);
    void close();
    int cellW() const { return cellW_; }
    int lineH() const { return lineH_; }
    TTF_Font* ttf() const { return font_; }
    SDL_Texture* glyph(uint32_t cp, int* w, int* h);
    SDL_Texture* string(std::string_view s, int* w, int* h);

private:
    struct Glyph { SDL_Texture* tex; int w, h; };
    SDL_Renderer* r_ = nullptr;
    TTF_Font* font_ = nullptr;
    int cellW_ = 0, lineH_ = 0;
    std::unordered_map<uint32_t, Glyph> cache_;
    std::unordered_map<std::string, Glyph> strings_;
};

class Gfx {
public:
    static constexpr int W = 1280, H = 720;  // Switch native resolution

    // fontDir holds DejaVuSansMono.ttf, DejaVuSans.ttf, DejaVuSans-ExtraLight.ttf.
    bool init(SDL_Renderer* r, const std::string& fontDir);
    void shutdown();

    SDL_Renderer* sdl() const { return r_; }
    Font& text() { return text_; }    // editor body
    Font& small() { return small_; }  // UI chrome
    Font& large() { return large_; }  // diagram labels

    // Metro / TV style.
    Font& monoTV() { return monoTV_; }    // document text
    Font& monoTile() { return monoTile_; }// characters on tiles
    Font& light() { return light_; }      // big light headings
    Font& lightMid() { return lightMid_; }// clock, menu items
    Font& sans() { return sans_; }        // labels
    Font& sansSmall() { return sansSmall_; }

    void clear(Color c);
    void fill(Rect rc, Color c);
    void outline(Rect rc, Color c);
    void fillCircle(float cx, float cy, float radius, Color c);
    void ring(float cx, float cy, float radius, float thickness, Color c);

    // Draws UTF-8 text with its top-left at (x, y). Returns the width drawn.
    int print(Font& f, int x, int y, std::string_view s, Color c);
    // Same, but horizontally centred on cx.
    void printCentered(Font& f, int cx, int y, std::string_view s, Color c);
    static int width(Font& f, std::string_view s);

    // Proportional text (any font). Returns the width drawn.
    int draw(Font& f, int x, int y, std::string_view s, Color c);
    static int measure(Font& f, std::string_view s);

private:
    SDL_Renderer* r_ = nullptr;
    Font text_, small_, large_;
    Font monoTV_, monoTile_, light_, lightMid_, sans_, sansSmall_;
};
