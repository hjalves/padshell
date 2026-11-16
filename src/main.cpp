#include <SDL.h>
#include <SDL_ttf.h>

#include <memory>

#include "apps/editor.h"
#include "gfx.h"
#include "pad.h"
#include "platform.h"
#include "shell.h"
#include "skin.h"

int main(int, char**) {
    platform::init();

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0 || TTF_Init() != 0) {
        SDL_Log("init failed: %s", SDL_GetError());
        platform::shutdown();
        return 1;
    }

    SDL_Window* win = SDL_CreateWindow("padshell", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, Gfx::W,
                                       Gfx::H, SDL_WINDOW_RESIZABLE);
    SDL_Renderer* ren = win ? SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC) : nullptr;
    if (!ren && win) ren = SDL_CreateRenderer(win, -1, 0);  // software fallback
    Gfx gfx;
    if (!ren || !gfx.init(ren, platform::assetPath("fonts/"))) {
        SDL_Log("renderer/font setup failed: %s", SDL_GetError());
        return 1;
    }
    // Draw in 1280x720 coordinates regardless of the real window size.
    SDL_RenderSetLogicalSize(ren, Gfx::W, Gfx::H);

    skins::loadSettings(platform::dataDir() + "settings.ini");
    Shell shell;
    shell.add(std::make_unique<EditorApp>(platform::dataDir() + "notes.txt"));

    PadTracker tracker;
    while (shell.running()) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
                case SDL_QUIT: shell.quit(); break;
                case SDL_CONTROLLERDEVICEADDED: platform::onControllerAdded(e.cdevice.which); break;
                case SDL_CONTROLLERDEVICEREMOVED: platform::onControllerRemoved(e.cdevice.which); break;
#ifndef __SWITCH__
                case SDL_KEYDOWN:
                    if (e.key.keysym.sym == SDLK_ESCAPE) shell.quit();
                    break;
#endif
            }
        }
        if (!shell.running()) break;

        uint32_t held;
        float lx, ly, rx, ry;
        platform::samplePad(held, lx, ly, rx, ry);
        shell.update(tracker.update(held, lx, ly, rx, ry, SDL_GetTicks()));

        // Log the real framebuffer size when it changes (e.g. docking).
        static int lastW = 0, lastH = 0;
        int ow = 0, oh = 0;
        SDL_GetRendererOutputSize(ren, &ow, &oh);
        if (ow != lastW || oh != lastH) {
            SDL_Log("render output %dx%d (drawing in %dx%d)", ow, oh, Gfx::W, Gfx::H);
            lastW = ow;
            lastH = oh;
        }

        shell.render(gfx);
        SDL_RenderPresent(ren);
    }

    // Teardown order matters: everything SDL owns is released before
    // SDL_Quit(); closing controllers after it is a use-after-free that can
    // crash the app on exit.
    platform::closeControllers();
    gfx.shutdown();
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    TTF_Quit();
    SDL_Quit();
    platform::shutdown();
    return 0;
}
