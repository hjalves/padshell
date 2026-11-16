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
    virtual std::string title() const = 0;    // e.g. "Editor - notes.txt *"
    virtual std::string heading() const = 0;  // short lowercase name, e.g. "notes"
    virtual void onEnter() {}                 // became the foreground app
    virtual void onLeave() {}                 // backgrounded or shell quitting: save state here
    virtual void update(const Gamepad& pad, Shell& shell) = 0;
    // Draw into `area`, normally by handing a view of the app to the skin.
    virtual void render(Gfx& g, Rect area) = 0;
    // Short hints some skins show next to the controller.
    virtual std::vector<std::string> help() const { return {}; }
};
