#include "shell.h"

#include <ctime>

#include "skin.h"

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

std::vector<std::string> Shell::launcherItems() const {
    std::vector<std::string> items;
    for (const auto& a : apps_) items.push_back(a->heading());
    items.push_back(std::string("style: ") + skins::current().name());
    items.push_back("quit");
    return items;
}

void Shell::update(const Gamepad& pad) {
    pad_ = pad;
    if (pad.hit(Minus)) {
        launcher_ = !launcher_;
        sel_ = current_;
        return;
    }
    if (!launcher_) {
        apps_[current_]->update(pad, *this);
        return;
    }
    // Launcher: apps, then "style", then "quit".
    const int n = (int)apps_.size() + 2, styleItem = n - 2, quitItem = n - 1;
    if (pad.rep(DUp)) sel_ = (sel_ + n - 1) % n;
    if (pad.rep(DDown)) sel_ = (sel_ + 1) % n;
    if (pad.hit(kBack)) launcher_ = false;
    if (sel_ == styleItem) {  // cycle styles; the menu stays open to preview
        if (pad.hit(kConfirm) || pad.rep(DRight)) skins::select(skins::index() + 1);
        if (pad.rep(DLeft)) skins::select(skins::index() - 1);
        return;
    }
    if (pad.hit(kConfirm)) {
        launcher_ = false;
        if (sel_ == quitItem) quit();
        else switchTo(sel_);
    }
}

void Shell::render(Gfx& g) {
    Skin& skin = skins::current();
    ShellView v;
    if (!apps_.empty()) {
        v.heading = apps_[current_]->heading();
        v.title = apps_[current_]->title();
        v.help = apps_[current_]->help();
    }
    char clock[16];
    std::time_t now = std::time(nullptr);
    std::strftime(clock, sizeof clock, "%H:%M", std::localtime(&now));
    v.clock = clock;
    if (SDL_GetTicks() < toastUntil_) v.toast = toast_;
    v.pad = &pad_;

    Rect area = skin.frame(g, v);
    if (!apps_.empty()) apps_[current_]->render(g, area);
    if (launcher_) skin.menu(g, "apps", launcherItems(), sel_);
    skin.finish(g);
}
