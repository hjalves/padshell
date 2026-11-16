#include "shell.h"

#include <ctime>

void Shell::add(std::unique_ptr<App> app) {
    apps_.push_back(std::move(app));
    if (apps_.size() == 1) apps_[0]->onEnter();
}

void Shell::toast(std::string msg) {
    toast_ = std::move(msg);
    toastUntil_ = SDL_GetTicks() + 2500;
}

void Shell::quit() {
    if (!apps_.empty()) apps_[current_]->onLeave();
    running_ = false;
}

void Shell::switchTo(int index) {
    if (index == current_) return;
    apps_[current_]->onLeave();
    current_ = index;
    apps_[current_]->onEnter();
}

void Shell::update(const Gamepad& pad) {
    if (pad.hit(Minus)) {
        launcher_ = !launcher_;
        sel_ = current_;
        return;
    }
    if (!launcher_) {
        apps_[current_]->update(pad, *this);
        return;
    }
    // Launcher: apps, then a final "Quit" entry.
    int n = (int)apps_.size() + 1;
    if (pad.rep(DUp)) sel_ = (sel_ + n - 1) % n;
    if (pad.rep(DDown)) sel_ = (sel_ + 1) % n;
    if (pad.hit(kBack)) launcher_ = false;
    if (pad.hit(kConfirm)) {
        launcher_ = false;
        if (sel_ == n - 1) quit();
        else switchTo(sel_);
    }
}

void Shell::render(Gfx& g) {
    g.clear(theme::bg);
    Rect area{0, kBarH, Gfx::W, Gfx::H - kBarH};
    if (!apps_.empty()) apps_[current_]->render(g, area);

    // Top bar.
    g.fill({0, 0, Gfx::W, kBarH}, theme::panelHi);
    int ty = (kBarH - g.small().lineH()) / 2;
    int x = 14 + g.print(g.small(), 14, ty, "padshell", theme::accent);
    if (!apps_.empty()) g.print(g.small(), x + 16, ty, apps_[current_]->title(), theme::text);

    char clock[16];
    std::time_t now = std::time(nullptr);
    std::strftime(clock, sizeof clock, "%H:%M", std::localtime(&now));
    int cw = Gfx::width(g.small(), clock);
    g.print(g.small(), Gfx::W - 14 - cw, ty, clock, theme::text);

    std::string right = SDL_GetTicks() < toastUntil_ ? toast_ : "-: menu";
    g.print(g.small(), Gfx::W - 40 - cw - Gfx::width(g.small(), right), ty, right,
            SDL_GetTicks() < toastUntil_ ? theme::warn : theme::dim);

    if (launcher_) renderLauncher(g);
}

void Shell::renderLauncher(Gfx& g) {
    g.fill({0, kBarH, Gfx::W, Gfx::H - kBarH}, {0, 0, 0, 160});
    Font& f = g.text();
    int n = (int)apps_.size() + 1;
    int rowH = f.lineH() + 16, w = 420, h = n * rowH + 70;
    Rect box{(Gfx::W - w) / 2, (Gfx::H - h) / 2, w, h};
    g.fill(box, theme::panel);
    g.outline(box, theme::panelHi);
    g.print(g.small(), box.x + 20, box.y + 14, "Apps", theme::dim);
    for (int i = 0; i < n; ++i) {
        Rect row{box.x + 10, box.y + 44 + i * rowH, w - 20, rowH};
        if (i == sel_) g.fill(row, theme::accent);
        std::string label = i < (int)apps_.size() ? apps_[i]->title() : "Quit";
        g.print(f, row.x + 14, row.y + 8, label, i == sel_ ? theme::bg : theme::text);
    }
}
