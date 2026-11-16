#include "platform.h"

#include <sys/stat.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <vector>

#ifdef __SWITCH__
#include <switch.h>
#include <unistd.h>
#endif

namespace platform {

namespace {
std::vector<SDL_GameController*> g_pads;
#ifdef __SWITCH__
int g_nxlinkSock = -1;
#endif

void mkdirs(const std::string& path) {
    for (size_t i = 1; i <= path.size(); ++i) {
        if (i == path.size() || path[i] == '/') {
            std::string part = path.substr(0, i);
            if (!part.empty() && part.back() != ':') mkdir(part.c_str(), 0755);
        }
    }
}

float axis(SDL_GameController* c, SDL_GameControllerAxis a) {
    return SDL_GameControllerGetAxis(c, a) / 32767.f;
}
}  // namespace

void init() {
#ifdef __SWITCH__
    romfsInit();
    // Lets `nxlink -s` show printf/SDL_Log output on the PC. Harmless when the
    // app wasn't launched via nxlink (socket connect just fails).
    if (R_SUCCEEDED(socketInitializeDefault())) g_nxlinkSock = nxlinkStdio();
#else
    // Positional face buttons on desktop too, even for Nintendo-layout pads.
    SDL_SetHint(SDL_HINT_GAMECONTROLLER_USE_BUTTON_LABELS, "0");
#endif
}

void shutdown() {
    for (auto* c : g_pads) SDL_GameControllerClose(c);
    g_pads.clear();
#ifdef __SWITCH__
    if (g_nxlinkSock >= 0) close(g_nxlinkSock);
    socketExit();
    romfsExit();
#endif
}

std::string assetPath(const std::string& rel) {
#ifdef __SWITCH__
    return "romfs:/" + rel;
#else
    static std::string base = [] {
        char* p = SDL_GetBasePath();
        std::string s = p ? p : "./";
        SDL_free(p);
        return s + "romfs/";
    }();
    return base + rel;
#endif
}

std::string dataDir() {
#ifdef __SWITCH__
    std::string dir = "sdmc:/switch/padshell/";
#else
    std::string dir;
    if (const char* env = std::getenv("PADSHELL_DATA")) dir = std::string(env) + "/";
    else if (const char* home = std::getenv("HOME")) dir = std::string(home) + "/.local/share/padshell/";
    else dir = "./padshell-data/";
#endif
    mkdirs(dir);
    return dir;
}

void onControllerAdded(int deviceIndex) {
    if (!SDL_IsGameController(deviceIndex)) return;
    if (SDL_GameController* c = SDL_GameControllerOpen(deviceIndex)) {
        SDL_Log("controller: %s", SDL_GameControllerName(c));
        g_pads.push_back(c);
    }
}

void onControllerRemoved(SDL_JoystickID id) {
    auto it = std::find_if(g_pads.begin(), g_pads.end(), [&](SDL_GameController* c) {
        return SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(c)) == id;
    });
    if (it != g_pads.end()) {
        SDL_GameControllerClose(*it);
        g_pads.erase(it);
    }
}

void samplePad(uint32_t& held, float& lx, float& ly, float& rx, float& ry) {
    held = 0;
    lx = ly = rx = ry = 0;

    // SDL's A/B/X/Y are positional on every platform, Xbox-style: A is the
    // bottom button. That includes the Switch port, whose mapping is
    // "a:b1,b:b0,x:b3,y:b2", i.e. SDL A = Nintendo B.
    const Btn aBtn = South, bBtn = East, xBtn = West, yBtn = North;
    struct Map { SDL_GameControllerButton b; uint32_t bit; };
    const Map map[] = {
        {SDL_CONTROLLER_BUTTON_A, aBtn}, {SDL_CONTROLLER_BUTTON_B, bBtn},
        {SDL_CONTROLLER_BUTTON_X, xBtn}, {SDL_CONTROLLER_BUTTON_Y, yBtn},
        {SDL_CONTROLLER_BUTTON_LEFTSHOULDER, L}, {SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, R},
        {SDL_CONTROLLER_BUTTON_START, Plus}, {SDL_CONTROLLER_BUTTON_BACK, Minus},
        {SDL_CONTROLLER_BUTTON_DPAD_UP, DUp}, {SDL_CONTROLLER_BUTTON_DPAD_DOWN, DDown},
        {SDL_CONTROLLER_BUTTON_DPAD_LEFT, DLeft}, {SDL_CONTROLLER_BUTTON_DPAD_RIGHT, DRight},
        {SDL_CONTROLLER_BUTTON_LEFTSTICK, LStick}, {SDL_CONTROLLER_BUTTON_RIGHTSTICK, RStick},
    };

    // Merge all connected pads (e.g. two detached Joy-Cons count as one).
    for (SDL_GameController* c : g_pads) {
        for (const Map& m : map)
            if (SDL_GameControllerGetButton(c, m.b)) held |= m.bit;
        if (axis(c, SDL_CONTROLLER_AXIS_TRIGGERLEFT) > 0.5f) held |= ZL;
        if (axis(c, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) > 0.5f) held |= ZR;
        float x = axis(c, SDL_CONTROLLER_AXIS_LEFTX), y = axis(c, SDL_CONTROLLER_AXIS_LEFTY);
        if (x * x + y * y > lx * lx + ly * ly) { lx = x; ly = y; }
        x = axis(c, SDL_CONTROLLER_AXIS_RIGHTX); y = axis(c, SDL_CONTROLLER_AXIS_RIGHTY);
        if (x * x + y * y > rx * rx + ry * ry) { rx = x; ry = y; }
    }

#ifndef __SWITCH__
    // Keyboard as a fake gamepad, so the input method can be tried without a
    // controller. It deliberately does NOT type letters directly.
    //   WASD = left stick   I/L/K/J = North/East/South/West
    //   arrows = D-pad      Q/E = L/R   Z/C = ZL/ZR   Return = +   Tab = -
    const Uint8* k = SDL_GetKeyboardState(nullptr);
    struct KMap { SDL_Scancode sc; uint32_t bit; };
    const KMap kmap[] = {
        {SDL_SCANCODE_I, North}, {SDL_SCANCODE_L, East}, {SDL_SCANCODE_K, South}, {SDL_SCANCODE_J, West},
        {SDL_SCANCODE_Q, L}, {SDL_SCANCODE_E, R}, {SDL_SCANCODE_Z, ZL}, {SDL_SCANCODE_C, ZR},
        {SDL_SCANCODE_RETURN, Plus}, {SDL_SCANCODE_TAB, Minus},
        {SDL_SCANCODE_UP, DUp}, {SDL_SCANCODE_DOWN, DDown},
        {SDL_SCANCODE_LEFT, DLeft}, {SDL_SCANCODE_RIGHT, DRight},
    };
    for (const KMap& m : kmap)
        if (k[m.sc]) held |= m.bit;
    float kx = (float)(k[SDL_SCANCODE_D] - k[SDL_SCANCODE_A]);
    float ky = (float)(k[SDL_SCANCODE_S] - k[SDL_SCANCODE_W]);
    if (kx != 0 || ky != 0) {
        float n = std::sqrt(kx * kx + ky * ky);
        lx = kx / n;
        ly = ky / n;
    }
#endif
}

}  // namespace platform
