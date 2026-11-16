// Base for low-resolution pixel-art skins: everything is drawn into a small
// render-target texture (in its own coordinates, e.g. 320x180) and scaled up
// to 1280x720 with nearest-neighbour filtering in finish().
#pragma once
#include <cctype>
#include <string>

#include "../skin.h"

class PixelSkin : public Skin {
public:
    PixelSkin(int w, int h) : lw_(w), lh_(h) {}
    ~PixelSkin() override {
        if (target_) SDL_DestroyTexture(target_);
    }

    void finish(Gfx& g) override {
        SDL_SetRenderTarget(g.sdl(), nullptr);
        SDL_Rect dst{0, 0, Gfx::W, Gfx::H};
        SDL_RenderCopy(g.sdl(), target_, nullptr, &dst);
        afterUpscale(g);
    }

protected:
    // Call first in frame(): makes the low-res texture the render target.
    void begin(Gfx& g) {
        if (!target_) {
            target_ = SDL_CreateTexture(g.sdl(), SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, lw_, lh_);
            SDL_SetTextureScaleMode(target_, SDL_ScaleModeNearest);
        }
        SDL_SetRenderTarget(g.sdl(), target_);
    }
    // Hook for full-resolution effects drawn over the upscaled image.
    virtual void afterUpscale(Gfx&) {}

    static Font& font(Gfx& g) { return g.font("PressStart2P.ttf", 8, true); }
    static std::string upper(std::string s) {
        for (char& c : s) c = (char)std::toupper((unsigned char)c);
        return s;
    }
    static bool blinkOn() { return (SDL_GetTicks() / 400) % 2 == 0; }
    static std::string pad3(size_t n) {
        std::string s = std::to_string(n > 999 ? 999 : n);
        return std::string(3 - s.size(), '0') + s;
    }
    // Arrow glyphs exist in Press Start 2P.
    static const char* keyGlyph(Btn b) {
        switch (b) {
            case DUp: return "\xE2\x86\x91";
            case DDown: return "\xE2\x86\x93";
            case DLeft: return "\xE2\x86\x90";
            case DRight: return "\xE2\x86\x92";
            default: return btnLabel(b);
        }
    }

    int lw_, lh_;
    SDL_Texture* target_ = nullptr;
};
