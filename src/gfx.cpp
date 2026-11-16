#include "gfx.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "utf8.h"

// --- Font -------------------------------------------------------------------

bool Font::open(SDL_Renderer* r, const std::string& path, int px, bool solid) {
    r_ = r;
    solid_ = solid;
    font_ = TTF_OpenFont(path.c_str(), px);
    if (!font_) {
        SDL_Log("TTF_OpenFont(%s): %s", path.c_str(), TTF_GetError());
        return false;
    }
    int adv = 0;
    TTF_GlyphMetrics(font_, 'M', nullptr, nullptr, nullptr, nullptr, &adv);
    cellW_ = adv;
    lineH_ = TTF_FontLineSkip(font_);
    return true;
}

void Font::close() {
    for (auto& [cp, t] : glyphs_) SDL_DestroyTexture(t.tex);
    glyphs_.clear();
    for (auto& [s, t] : strings_) SDL_DestroyTexture(t.tex);
    strings_.clear();
    if (font_) TTF_CloseFont(font_);
    font_ = nullptr;
}

SDL_Surface* Font::render(const char* s) {
    SDL_Color white{255, 255, 255, 255};
    return solid_ ? TTF_RenderUTF8_Solid(font_, s, white) : TTF_RenderUTF8_Blended(font_, s, white);
}

SDL_Texture* Font::glyph(uint32_t cp, int* w, int* h) {
    auto it = glyphs_.find(cp);
    if (it == glyphs_.end()) {
        Tex t{nullptr, 0, 0};
        std::string s = utf8::encode(cp);
        if (SDL_Surface* surf = render(s.c_str())) {
            t = {SDL_CreateTextureFromSurface(r_, surf), surf->w, surf->h};
            SDL_FreeSurface(surf);
        }
        it = glyphs_.emplace(cp, t).first;
    }
    *w = it->second.w;
    *h = it->second.h;
    return it->second.tex;
}

SDL_Texture* Font::string(std::string_view s, int* w, int* h) {
    std::string key(s);
    auto it = strings_.find(key);
    if (it == strings_.end()) {
        if (strings_.size() > 512) {  // crude bound; a screen reuses a few dozen
            for (auto& [k, t] : strings_) SDL_DestroyTexture(t.tex);
            strings_.clear();
        }
        Tex t{nullptr, 0, 0};
        if (SDL_Surface* surf = render(key.c_str())) {
            t = {SDL_CreateTextureFromSurface(r_, surf), surf->w, surf->h};
            SDL_FreeSurface(surf);
        }
        it = strings_.emplace(std::move(key), t).first;
    }
    *w = it->second.w;
    *h = it->second.h;
    return it->second.tex;
}

// --- Gfx --------------------------------------------------------------------

bool Gfx::init(SDL_Renderer* r, const std::string& fontDir) {
    r_ = r;
    fontDir_ = fontDir;
    SDL_SetRenderDrawBlendMode(r_, SDL_BLENDMODE_BLEND);
    return text().ttf() != nullptr;  // the fallback font must exist
}

void Gfx::shutdown() { fonts_.clear(); }

Font& Gfx::font(const std::string& file, int px, bool solid) {
    std::string key = file + "@" + std::to_string(px) + (solid ? "s" : "");
    auto it = fonts_.find(key);
    if (it != fonts_.end()) return *it->second;
    auto f = std::make_unique<Font>();
    if (!f->open(r_, fontDir_ + file, px, solid)) f->open(r_, fontDir_ + "DejaVuSansMono.ttf", px, solid);
    return *fonts_.emplace(key, std::move(f)).first->second;
}

void Gfx::clear(Color c) {
    SDL_SetRenderDrawColor(r_, c.r, c.g, c.b, c.a);
    SDL_RenderClear(r_);
}

void Gfx::fill(Rect rc, Color c) {
    SDL_SetRenderDrawColor(r_, c.r, c.g, c.b, c.a);
    SDL_Rect sr{rc.x, rc.y, rc.w, rc.h};
    SDL_RenderFillRect(r_, &sr);
}

void Gfx::outline(Rect rc, Color c, int t) {
    fill({rc.x, rc.y, rc.w, t}, c);
    fill({rc.x, rc.y + rc.h - t, rc.w, t}, c);
    fill({rc.x, rc.y + t, t, rc.h - 2 * t}, c);
    fill({rc.x + rc.w - t, rc.y + t, t, rc.h - 2 * t}, c);
}

void Gfx::fillRounded(Rect rc, int radius, Color c) {
    radius = std::min({radius, rc.w / 2, rc.h / 2});
    if (radius <= 0) { fill(rc, c); return; }
    // Centre column plus two side columns, then a quarter fan in each corner,
    // as one triangle list so translucent fills don't overlap.
    std::vector<SDL_Vertex> v;
    SDL_Color col{c.r, c.g, c.b, c.a};
    auto quad = [&](float x0, float y0, float x1, float y1) {
        SDL_FPoint p[4] = {{x0, y0}, {x1, y0}, {x0, y1}, {x1, y1}};
        for (int i : {0, 1, 2, 2, 1, 3}) v.push_back({p[i], col, {0, 0}});
    };
    const float x = rc.x, y = rc.y, w = rc.w, h = rc.h, r = radius;
    quad(x + r, y, x + w - r, y + h);
    quad(x, y + r, x + r, y + h - r);
    quad(x + w - r, y + r, x + w, y + h - r);
    const int seg = 8;
    const float cx[4] = {x + r, x + w - r, x + w - r, x + r}, cy[4] = {y + r, y + r, y + h - r, y + h - r};
    for (int k = 0; k < 4; ++k) {
        float a0 = (float)M_PI * (1.0f + 0.5f * k);
        for (int i = 0; i < seg; ++i) {
            float a = a0 + (float)M_PI / 2 * i / seg, b = a0 + (float)M_PI / 2 * (i + 1) / seg;
            v.push_back({{cx[k], cy[k]}, col, {0, 0}});
            v.push_back({{cx[k] + r * std::cos(a), cy[k] + r * std::sin(a)}, col, {0, 0}});
            v.push_back({{cx[k] + r * std::cos(b), cy[k] + r * std::sin(b)}, col, {0, 0}});
        }
    }
    SDL_RenderGeometry(r_, nullptr, v.data(), (int)v.size(), nullptr, 0);
}

void Gfx::fillCircle(float cx, float cy, float radius, Color c) { ring(cx, cy, radius, radius, c); }

void Gfx::ring(float cx, float cy, float radius, float thickness, Color c) {
    const int seg = std::clamp((int)(radius * 0.8f), 24, 96);
    float inner = std::max(0.f, radius - thickness);
    SDL_Color col{c.r, c.g, c.b, c.a};
    std::vector<SDL_Vertex> v;
    v.reserve(seg * 6);
    for (int i = 0; i < seg; ++i) {
        float a0 = 2 * (float)M_PI * i / seg, a1 = 2 * (float)M_PI * (i + 1) / seg;
        SDL_FPoint o0{cx + radius * std::cos(a0), cy + radius * std::sin(a0)};
        SDL_FPoint o1{cx + radius * std::cos(a1), cy + radius * std::sin(a1)};
        SDL_FPoint i0{cx + inner * std::cos(a0), cy + inner * std::sin(a0)};
        SDL_FPoint i1{cx + inner * std::cos(a1), cy + inner * std::sin(a1)};
        if (inner <= 0) {
            for (SDL_FPoint p : {SDL_FPoint{cx, cy}, o0, o1}) v.push_back({p, col, {0, 0}});
        } else {
            for (SDL_FPoint p : {o0, o1, i0, i0, o1, i1}) v.push_back({p, col, {0, 0}});
        }
    }
    SDL_RenderGeometry(r_, nullptr, v.data(), (int)v.size(), nullptr, 0);
}

void Gfx::gradient(Rect rc, Color tl, Color tr, Color bl, Color br) {
    auto sc = [](Color c) { return SDL_Color{c.r, c.g, c.b, c.a}; };
    float x0 = rc.x, y0 = rc.y, x1 = rc.x + rc.w, y1 = rc.y + rc.h;
    SDL_Vertex v[4] = {{{x0, y0}, sc(tl), {0, 0}}, {{x1, y0}, sc(tr), {0, 0}},
                       {{x0, y1}, sc(bl), {0, 0}}, {{x1, y1}, sc(br), {0, 0}}};
    int idx[6] = {0, 1, 2, 2, 1, 3};
    SDL_RenderGeometry(r_, nullptr, v, 4, idx, 6);
}

void Gfx::radial(float cx, float cy, float radius, Color inner, Color outer) {
    const int seg = 64;
    SDL_Color ci{inner.r, inner.g, inner.b, inner.a}, co{outer.r, outer.g, outer.b, outer.a};
    std::vector<SDL_Vertex> v;
    v.reserve(seg * 3);
    for (int i = 0; i < seg; ++i) {
        float a0 = 2 * (float)M_PI * i / seg, a1 = 2 * (float)M_PI * (i + 1) / seg;
        v.push_back({{cx, cy}, ci, {0, 0}});
        v.push_back({{cx + radius * std::cos(a0), cy + radius * std::sin(a0)}, co, {0, 0}});
        v.push_back({{cx + radius * std::cos(a1), cy + radius * std::sin(a1)}, co, {0, 0}});
    }
    SDL_RenderGeometry(r_, nullptr, v.data(), (int)v.size(), nullptr, 0);
}

void Gfx::triangle(float x0, float y0, float x1, float y1, float x2, float y2, Color c) {
    SDL_Color col{c.r, c.g, c.b, c.a};
    SDL_Vertex v[3] = {{{x0, y0}, col, {0, 0}}, {{x1, y1}, col, {0, 0}}, {{x2, y2}, col, {0, 0}}};
    SDL_RenderGeometry(r_, nullptr, v, 3, nullptr, 0);
}

void Gfx::arrow(float cx, float cy, float s, Dir d, Color c) {
    float h = s / 2;
    switch (d) {
        case Dir::Up: triangle(cx, cy - h, cx + h, cy + h * 0.6f, cx - h, cy + h * 0.6f, c); break;
        case Dir::Down: triangle(cx, cy + h, cx - h, cy - h * 0.6f, cx + h, cy - h * 0.6f, c); break;
        case Dir::Left: triangle(cx - h, cy, cx + h * 0.6f, cy - h, cx + h * 0.6f, cy + h, c); break;
        case Dir::Right: triangle(cx + h, cy, cx - h * 0.6f, cy + h, cx - h * 0.6f, cy - h, c); break;
    }
}

int Gfx::print(Font& f, int x, int y, std::string_view s, Color c) {
    int x0 = x;
    for (size_t i = 0; i < s.size(); i = utf8::next(s, i)) {
        uint32_t cp = utf8::decode(s, i);
        int w, h;
        if (cp != ' ') {
            if (SDL_Texture* t = f.glyph(cp, &w, &h)) {
                SDL_SetTextureColorMod(t, c.r, c.g, c.b);
                SDL_SetTextureAlphaMod(t, c.a);
                SDL_Rect dst{x, y, w, h};
                SDL_RenderCopy(r_, t, nullptr, &dst);
            }
        }
        x += f.cellW();
    }
    return x - x0;
}

void Gfx::printCentered(Font& f, int cx, int y, std::string_view s, Color c) {
    print(f, cx - width(f, s) / 2, y, s, c);
}

int Gfx::width(Font& f, std::string_view s) { return (int)utf8::length(s) * f.cellW(); }

int Gfx::draw(Font& f, int x, int y, std::string_view s, Color c) {
    if (s.empty()) return 0;
    int w, h;
    SDL_Texture* t = f.string(s, &w, &h);
    if (!t) return 0;
    SDL_SetTextureColorMod(t, c.r, c.g, c.b);
    SDL_SetTextureAlphaMod(t, c.a);
    SDL_Rect dst{x, y, w, h};
    SDL_RenderCopy(r_, t, nullptr, &dst);
    return w;
}

void Gfx::drawCentered(Font& f, int cx, int y, std::string_view s, Color c) {
    draw(f, cx - measure(f, s) / 2, y, s, c);
}

void Gfx::drawRotated(Font& f, int cx, int cy, std::string_view s, Color c, double degrees) {
    if (s.empty()) return;
    int w, h;
    SDL_Texture* t = f.string(s, &w, &h);
    if (!t) return;
    SDL_SetTextureColorMod(t, c.r, c.g, c.b);
    SDL_SetTextureAlphaMod(t, c.a);
    SDL_Rect dst{cx - w / 2, cy - h / 2, w, h};
    SDL_RenderCopyEx(r_, t, nullptr, &dst, degrees, nullptr, SDL_FLIP_NONE);
}

int Gfx::measure(Font& f, std::string_view s) {
    int w = 0;
    TTF_SizeUTF8(f.ttf(), std::string(s).c_str(), &w, nullptr);
    return w;
}
