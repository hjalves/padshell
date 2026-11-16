// Owns the apps and the app launcher (press Minus). All drawing is done by
// the current skin (see skin.h).
#pragma once
#include <memory>
#include <string>
#include <vector>

#include "app.h"

class Shell {
public:
    void add(std::unique_ptr<App> app);
    void update(const Gamepad& pad);
    void render(Gfx& g);
    void quit();

    bool running() const { return running_; }
    // Short message shown by the skin for a few seconds.
    void toast(std::string msg);

private:
    std::vector<std::string> launcherItems() const;
    void switchTo(int index);

    std::vector<std::unique_ptr<App>> apps_;
    int current_ = 0;
    bool launcher_ = false;
    int sel_ = 0;
    bool running_ = true;
    Gamepad pad_;  // last input, for controller diagrams
    std::string toast_;
    uint32_t toastUntil_ = 0;
};
