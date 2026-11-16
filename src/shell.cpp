#include "shell.h"

#include <ctime>

#include "ui.h"

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
    // Launcher: apps, then "Style", then "Quit".
    const int n = (int)apps_.size() + 2, styleItem = n - 2, quitItem = n - 1;
    if (pad.rep(DUp)) sel_ = (sel_ + n - 1) % n;
    if (pad.rep(DDown)) sel_ = (sel_ + 1) % n;
    if (pad.hit(kBack)) launcher_ = false;
    if (pad.hit(kConfirm)) {
        if (sel_ == styleItem) {  // stays open so the change is visible
            ui::setStyle(ui::style() == ui::Style::Metro ? ui::Style::Classic : ui::Style::Metro);
            return;
        }
        launcher_ = false;
        if (sel_ == quitItem) quit();
        else switchTo(sel_);
    }
}

std::vector<std::string> Shell::launcherItems() const {
    bool metro = ui::style() == ui::Style::Metro;
    std::vector<std::string> items;
    for (const auto& a : apps_) items.push_back(metro ? a->heading() : a->title());
    items.push_back(std::string(metro ? "style: " : "Style: ") + ui::styleName(ui::style()));
    items.push_back(metro ? "quit" : "Quit");
    return items;
}

void Shell::render(Gfx& g) {
    if (ui::style() == ui::Style::Metro) {
        renderMetro(g);
        return;
    }
    g.clear(theme::bg);
    Rect area{0, kBarH, Gfx::W - kPanelW, Gfx::H - kBarH};
    if (!apps_.empty()) apps_[current_]->render(g, area);
    renderController(g, {Gfx::W - kPanelW, kBarH, kPanelW, Gfx::H - kBarH});

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

    std::string right = SDL_GetTicks() < toastUntil_ ? toast_ : "-: apps";
    g.print(g.small(), Gfx::W - 40 - cw - Gfx::width(g.small(), right), ty, right,
            SDL_GetTicks() < toastUntil_ ? theme::warn : theme::dim);

    if (launcher_) renderLauncher(g);
}

void Shell::renderLauncher(Gfx& g) {
    g.fill({0, kBarH, Gfx::W, Gfx::H - kBarH}, {0, 0, 0, 160});
    Font& f = g.text();
    const std::vector<std::string> items = launcherItems();
    int n = (int)items.size();
    int rowH = f.lineH() + 16, w = 420, h = n * rowH + 70;
    Rect box{(Gfx::W - w) / 2, (Gfx::H - h) / 2, w, h};
    g.fill(box, theme::panel);
    g.outline(box, theme::panelHi);
    g.print(g.small(), box.x + 20, box.y + 14, "Apps", theme::dim);
    for (int i = 0; i < n; ++i) {
        Rect row{box.x + 10, box.y + 44 + i * rowH, w - 20, rowH};
        if (i == sel_) g.fill(row, theme::accent);
        g.print(f, row.x + 14, row.y + 8, items[i], i == sel_ ? theme::bg : theme::text);
    }
}

// A schematic Switch-style controller (sticks offset, D-pad bottom-left) with
// the buttons currently held lit up, plus the foreground app's hints.
void Shell::renderController(Gfx& g, Rect p) {
    g.fill(p, theme::panel);
    auto col = [&](uint32_t bits) { return (pad_.held & bits) ? theme::accent : theme::bg; };
    auto txt = [&](uint32_t bits) { return (pad_.held & bits) ? theme::bg : theme::text; };
    Font& sf = g.small();

    // Shoulders and triggers.
    struct Sh { Btn b; int x, y, h; };
    for (Sh s : {Sh{ZL, 40, 18, 34}, Sh{L, 40, 58, 28}, Sh{ZR, 250, 18, 34}, Sh{R, 250, 58, 28}}) {
        Rect r{p.x + s.x, p.y + s.y, 110, s.h};
        g.fill(r, col(s.b));
        g.printCentered(sf, r.x + r.w / 2, r.y + (s.h - sf.lineH()) / 2, btnLabel(s.b), txt(s.b));
    }

    Rect body{p.x + 20, p.y + 96, p.w - 40, 236};
    g.fill(body, theme::panelHi);
    const int lx = p.x + 105, rx = p.x + p.w - 105, topY = body.y + 72, botY = body.y + 170;

    // Sticks: ring plus a dot showing deflection.
    auto stick = [&](int cx, int cy, float x, float y, Btn click) {
        g.fillCircle(cx, cy, 34, col(click));
        g.ring(cx, cy, 34, 2, theme::dim);
        g.fillCircle(cx + x * 22, cy + y * 22, 10, theme::text);
    };
    stick(lx, topY, pad_.lx, pad_.ly, LStick);
    stick(rx, botY, pad_.rx, pad_.ry, RStick);

    // D-pad.
    struct D { Btn b; int dx, dy; };
    for (D d : {D{DUp, 0, -1}, D{DDown, 0, 1}, D{DLeft, -1, 0}, D{DRight, 1, 0}}) {
        Rect r{lx + d.dx * 32 - 15, botY + d.dy * 32 - 15, 30, 30};
        g.fill(r, col(d.b));
        g.printCentered(sf, r.x + 15, r.y + (30 - sf.lineH()) / 2, btnLabel(d.b), txt(d.b));
    }
    g.fill({lx - 15, botY - 15, 30, 30}, theme::bg);

    // Face buttons, by position.
    for (D d : {D{North, 0, -1}, D{South, 0, 1}, D{West, -1, 0}, D{East, 1, 0}}) {
        int cx = rx + d.dx * 36, cy = topY + d.dy * 36;
        g.fillCircle(cx, cy, 19, col(d.b));
        g.printCentered(sf, cx, cy - sf.lineH() / 2, btnLabel(d.b), txt(d.b));
    }

    // Minus / Plus.
    for (D d : {D{Minus, -1, 0}, D{Plus, 1, 0}}) {
        int cx = p.x + p.w / 2 + d.dx * 34, cy = body.y + 34;
        g.fillCircle(cx, cy, 13, col(d.b));
        g.printCentered(sf, cx, cy - sf.lineH() / 2, btnLabel(d.b), txt(d.b));
    }

    if (apps_.empty()) return;
    int y = body.y + body.h + 24;
    for (const std::string& line : apps_[current_]->help()) {
        g.print(sf, p.x + 20, y, line, theme::dim);
        y += sf.lineH() + 2;
    }
}

// Metro / TV: no chrome, just a small caps app label, a large light heading
// and the clock, inside the TV-safe margins. Apps get everything below.
void Shell::renderMetro(Gfx& g) {
    using namespace ui::metro;
    g.clear(bg);
    const int x = kMarginX, y = kMarginY;
    g.draw(g.sansSmall(), x, y, "PADSHELL", cyan);
    if (!apps_.empty()) g.draw(g.light(), x - 4, y + 14, apps_[current_]->heading(), text);

    char clock[16];
    std::time_t now = std::time(nullptr);
    std::strftime(clock, sizeof clock, "%H:%M", std::localtime(&now));
    int cw = Gfx::measure(g.lightMid(), clock);
    g.draw(g.lightMid(), Gfx::W - kMarginX - cw, y + 8, clock, text);
    if (SDL_GetTicks() < toastUntil_)
        g.draw(g.sans(), Gfx::W - kMarginX - Gfx::measure(g.sans(), toast_), y + 62, toast_, amber);

    Rect area{x, y + 104, Gfx::W - 2 * x, Gfx::H - (y + 104) - kMarginY};
    if (!apps_.empty()) apps_[current_]->render(g, area);
    if (launcher_) drawMenu(g, "APPS", launcherItems(), sel_, cyan);
}
