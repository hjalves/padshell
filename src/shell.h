// Owns the apps, draws the top bar and the controller panel, and provides the
// app launcher (press Minus).
#pragma once
#include <memory>
#include <string>
#include <vector>

#include "app.h"

class Shell {
public:
    static constexpr int kBarH = 36;
    static constexpr int kPanelW = 400;  // controller diagram on the right

    void add(std::unique_ptr<App> app);
    void update(const Gamepad& pad);
    void render(Gfx& g);
    void quit();

    bool running() const { return running_; }
    // Short message shown in the top bar for a few seconds.
    void toast(std::string msg);

private:
    void renderLauncher(Gfx& g);
    void renderController(Gfx& g, Rect area);
    void switchTo(int index);

    std::vector<std::unique_ptr<App>> apps_;
    int current_ = 0;
    bool launcher_ = false;
    int sel_ = 0;
    bool running_ = true;
    Gamepad pad_;  // last input, for the controller diagram
    std::string toast_;
    uint32_t toastUntil_ = 0;
};
