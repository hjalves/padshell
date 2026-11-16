// Thin drawing layer over SDL_Renderer: shapes, gradients and cached text.
// Skins (src/skins/) draw everything through this.
#pragma once
#include <SDL.h>
#include <SDL_ttf.h>

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

struct Color { uint8_t r, g, b, a = 255; };
struct Rect { int x, y, w, h; };

inline Color withAlpha(Color c, uint8_t a) { return {c.r, c.g, c.b, a}; }
inline Color mix(Color a, Color b, float t) {
    auto m = [t](uint8_t x, uint8_t y) { return (uint8_t)(x + (y - x) * t); };
    return {m(a.r, b.r), m(a.g, b.g), m(a.b, b.b), m(a.a, b.a)};
}

// A font with two texture caches, both rendered white and tinted at draw
// time: per-codepoint glyphs for monospace grids, and whole strings for
// proportional text (cleared when it grows large). "Solid" fonts render
// without anti-aliasing, for pixel fonts.
class Font {
public:
    ~Font() { close(); }
    bool open(SDL_Renderer* r, const std::string& path, int px, bool solid = false);
    void close();
    int cellW() const { return cellW_; }  // advance of 'M'
    int lineH() const { return lineH_; }
    TTF_Font* ttf() const { return font_; }
    SDL_Texture* glyph(uint32_t cp, int* w, int* h);
    SDL_Texture* string(std::string_view s, int* w, int* h);

private:
    struct Tex { SDL_Texture* tex; int w, h; };
    SDL_Surface* render(const char* s);
    SDL_Renderer* r_ = nullptr;
    TTF_Font* font_ = nullptr;
    bool solid_ = false;
    int cellW_ = 0, lineH_ = 0;
    std::unordered_map<uint32_t, Tex> glyphs_;
    std::unordered_map<std::string, Tex> strings_;
};

enum class Dir { Up, Down, Left, Right };

class Gfx {
public:
    static constexpr int W = 1280, H = 720;  // Switch native resolution

    bool init(SDL_Renderer* r, const std::string& fontDir);
    void shutdown();
    SDL_Renderer* sdl() const { return r_; }

    // Any bundled font by file name and pixel size, loaded on first use.
    // Falls back to DejaVu Sans Mono if the file can't be opened.
    Font& font(const std::string& file, int px, bool solid = false);

    // Fonts used by the Classic and Metro skins.
    Font& text() { return font("DejaVuSansMono.ttf", 24); }
    Font& small() { return font("DejaVuSansMono.ttf", 18); }
    Font& monoTV() { return font("DejaVuSansMono.ttf", 34); }
    Font& monoTile() { return font("DejaVuSansMono.ttf", 30); }
    Font& light() { return font("DejaVuSans-ExtraLight.ttf", 60); }
    Font& lightMid() { return font("DejaVuSans-ExtraLight.ttf", 44); }
    Font& sans() { return font("DejaVuSans.ttf", 22); }
    Font& sansSmall() { return font("DejaVuSans.ttf", 17); }

    void clear(Color c);
    void fill(Rect rc, Color c);
    void outline(Rect rc, Color c, int thickness = 1);
    void fillRounded(Rect rc, int radius, Color c);
    void fillCircle(float cx, float cy, float radius, Color c);
    void ring(float cx, float cy, float radius, float thickness, Color c);
    // Corner colours: top-left, top-right, bottom-left, bottom-right.
    void gradient(Rect rc, Color tl, Color tr, Color bl, Color br);
    // Soft disc: `inner` at the centre fading to `outer` at the radius.
    void radial(float cx, float cy, float radius, Color inner, Color outer);
    void triangle(float x0, float y0, float x1, float y1, float x2, float y2, Color c);
    // A solid arrow head of the given size centred on (cx, cy).
    void arrow(float cx, float cy, float size, Dir d, Color c);

    // Monospace grid text: each codepoint advances by cellW. Returns width.
    int print(Font& f, int x, int y, std::string_view s, Color c);
    void printCentered(Font& f, int cx, int y, std::string_view s, Color c);
    static int width(Font& f, std::string_view s);

    // Proportional text. Returns the width drawn.
    int draw(Font& f, int x, int y, std::string_view s, Color c);
    void drawCentered(Font& f, int cx, int y, std::string_view s, Color c);
    void drawRotated(Font& f, int cx, int cy, std::string_view s, Color c, double degrees);
    static int measure(Font& f, std::string_view s);

private:
    SDL_Renderer* r_ = nullptr;
    std::string fontDir_;
    std::map<std::string, std::unique_ptr<Font>> fonts_;
};
