#include "gfx.h"

#include <cmath>
#include <vector>

#include "utf8.h"

bool Font::open(SDL_Renderer* r, const std::string& path, int px) {
    r_ = r;
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
    for (auto& [cp, g] : cache_) SDL_DestroyTexture(g.tex);
    cache_.clear();
    for (auto& [str, g] : strings_) SDL_DestroyTexture(g.tex);
    strings_.clear();
    if (font_) TTF_CloseFont(font_);
    font_ = nullptr;
}

SDL_Texture* Font::glyph(uint32_t cp, int* w, int* h) {
    auto it = cache_.find(cp);
    if (it == cache_.end()) {
        Glyph g{nullptr, 0, 0};
        std::string s = utf8::encode(cp);
        if (SDL_Surface* surf = TTF_RenderUTF8_Blended(font_, s.c_str(), SDL_Color{255, 255, 255, 255})) {
            g.tex = SDL_CreateTextureFromSurface(r_, surf);
            g.w = surf->w;
            g.h = surf->h;
            SDL_FreeSurface(surf);
        }
        it = cache_.emplace(cp, g).first;
    }
    *w = it->second.w;
    *h = it->second.h;
    return it->second.tex;
}

SDL_Texture* Font::string(std::string_view s, int* w, int* h) {
    std::string key(s);
    auto it = strings_.find(key);
    if (it == strings_.end()) {
        if (strings_.size() > 512) {  // crude bound; typical screens reuse a few dozen
            for (auto& [str, g] : strings_) SDL_DestroyTexture(g.tex);
            strings_.clear();
        }
        Glyph g{nullptr, 0, 0};
        if (SDL_Surface* surf = TTF_RenderUTF8_Blended(font_, key.c_str(), SDL_Color{255, 255, 255, 255})) {
            g.tex = SDL_CreateTextureFromSurface(r_, surf);
            g.w = surf->w;
            g.h = surf->h;
            SDL_FreeSurface(surf);
        }
        it = strings_.emplace(std::move(key), g).first;
    }
    *w = it->second.w;
    *h = it->second.h;
    return it->second.tex;
}

bool Gfx::init(SDL_Renderer* r, const std::string& fontDir) {
    r_ = r;
    SDL_SetRenderDrawBlendMode(r_, SDL_BLENDMODE_BLEND);
    const std::string mono = fontDir + "DejaVuSansMono.ttf", sans = fontDir + "DejaVuSans.ttf",
                      light = fontDir + "DejaVuSans-ExtraLight.ttf";
    return text_.open(r, mono, 24) && small_.open(r, mono, 18) && large_.open(r, mono, 26) &&
           monoTV_.open(r, mono, 34) && monoTile_.open(r, mono, 30) && light_.open(r, light, 60) &&
           lightMid_.open(r, light, 44) && sans_.open(r, sans, 22) && sansSmall_.open(r, sans, 17);
}

void Gfx::shutdown() {
    for (Font* f : {&text_, &small_, &large_, &monoTV_, &monoTile_, &light_, &lightMid_, &sans_, &sansSmall_})
        f->close();
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

void Gfx::outline(Rect rc, Color c) {
    SDL_SetRenderDrawColor(r_, c.r, c.g, c.b, c.a);
    SDL_Rect sr{rc.x, rc.y, rc.w, rc.h};
    SDL_RenderDrawRect(r_, &sr);
}

void Gfx::fillCircle(float cx, float cy, float radius, Color c) {
    ring(cx, cy, radius, radius, c);
}

void Gfx::ring(float cx, float cy, float radius, float thickness, Color c) {
    const int seg = 48;
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
        for (SDL_FPoint p : {o0, o1, i0, i0, o1, i1}) v.push_back({p, col, {0, 0}});
    }
    SDL_RenderGeometry(r_, nullptr, v.data(), (int)v.size(), nullptr, 0);
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

int Gfx::measure(Font& f, std::string_view s) {
    int w = 0;
    TTF_SizeUTF8(f.ttf(), std::string(s).c_str(), &w, nullptr);
    return w;
}
