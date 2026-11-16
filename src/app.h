// The unit of the "OS": every program in the shell (editor, Gemini browser,
// weather...) is an App living in the same process. Homebrew can't really run
// several .nro files at once, so multitasking happens here instead.
#pragma once
#include <string>
#include <vector>

#include "gfx.h"
#include "pad.h"

class Shell;

class App {
public:
    virtual ~App() = default;
    virtual std::string title() const = 0;   // shown in the top bar / launcher
    virtual std::string heading() const { return title(); }  // short, for the Metro header
    virtual void onEnter() {}                // became the foreground app
    virtual void onLeave() {}                // backgrounded or shell quitting: save state here
    virtual void update(const Gamepad& pad, Shell& shell) = 0;
    virtual void render(Gfx& g, Rect area) = 0;
    // Short hints shown under the controller diagram.
    virtual std::vector<std::string> help() const { return {}; }
};
