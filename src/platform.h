// Everything that differs between the Switch and a desktop build lives behind
// this interface (paths, service init, controller mapping).
#pragma once
#include <SDL.h>

#include <string>

#include "pad.h"

namespace platform {

void init();              // romfs, sockets, nxlink stdio on Switch; nothing on desktop
void closeControllers();  // must run before SDL_Quit()
void shutdown();          // after SDL_Quit(): sockets, romfs

std::string assetPath(const std::string& rel);  // read-only bundled files
std::string dataDir();                          // writable, created on demand

// Samples every connected controller (and, on desktop, the keyboard as a
// stand-in pad) and returns which logical buttons are held plus stick axes.
void samplePad(uint32_t& held, float& lx, float& ly, float& rx, float& ry);
void onControllerAdded(int deviceIndex);
void onControllerRemoved(SDL_JoystickID id);

}  // namespace platform
