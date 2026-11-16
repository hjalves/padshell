#include "editor.h"

#include "../shell.h"
#include "../skin.h"

namespace {
const std::vector<std::string> kMenuItems = {"save", "quit padshell"};
}

EditorApp::EditorApp(std::string path) : path_(std::move(path)) {
    buf_.load(path_);  // missing file = start empty
    buf_.moveToEnd();
}

std::string EditorApp::title() const {
    std::string name = path_.substr(path_.find_last_of('/') + 1);
    return "Editor - " + name + (buf_.modified() ? " *" : "");
}

std::string EditorApp::heading() const {
    std::string name = path_.substr(path_.find_last_of('/') + 1);
    return name.substr(0, name.find_last_of('.'));
}

std::vector<std::string> EditorApp::help() const {
    return {
        std::string(btnLabel(ZL)) + " hold: numbers & symbols",
        std::string(btnLabel(ZR)) + " hold: newline, space, delete",
        std::string(btnLabel(L)) + ": uppercase",
        "+: editor menu    -: apps",
    };
}

void EditorApp::flush() {
    std::vector<TextEvent> ev;
    tap_.flush(ev);
    apply(ev);
}

bool EditorApp::save(Shell* shell) {
    flush();
    bool ok = buf_.save(path_);
    if (shell) shell->toast(ok ? "saved" : "save failed!");
    if (!ok) SDL_Log("save failed: %s", path_.c_str());
    return ok;
}

void EditorApp::onLeave() {
    flush();
    if (buf_.modified()) save(nullptr);
}

void EditorApp::apply(const std::vector<TextEvent>& events) {
    for (const TextEvent& e : events) {
        buf_.moveToEnd();  // append-only
        switch (e.kind) {
            case TextEvent::Insert: buf_.insert(e.text); break;
            case TextEvent::Newline: buf_.newline(); break;
            case TextEvent::Backspace: buf_.backspace(); break;
            case TextEvent::DeleteLine: buf_.deleteLastLine(); break;
        }
    }
}

void EditorApp::update(const Gamepad& pad, Shell& shell) {
    held_ = pad.held;
    if (menu_) {
        updateMenu(pad, shell);
        return;
    }
    if (pad.hit(Plus)) {
        flush();
        menu_ = true;
        menuSel_ = 0;
        return;
    }
    std::vector<TextEvent> ev;
    tap_.update(pad, ev);
    apply(ev);
}

void EditorApp::updateMenu(const Gamepad& pad, Shell& shell) {
    const int n = (int)kMenuItems.size();
    if (pad.hit(Plus) || pad.hit(kBack)) { menu_ = false; return; }
    if (pad.rep(DUp)) menuSel_ = (menuSel_ + n - 1) % n;
    if (pad.rep(DDown)) menuSel_ = (menuSel_ + 1) % n;
    if (!pad.hit(kConfirm)) return;
    menu_ = false;
    switch (menuSel_) {
        case 0: save(&shell); break;
        case 1: shell.quit(); break;
    }
}

void EditorApp::render(Gfx& g, Rect area) {
    EditorView v;
    v.lines = &buf_.lines();
    v.modified = buf_.modified();
    v.tap = &tap_;
    v.held = held_;
    Skin& skin = skins::current();
    skin.editor(g, area, v);
    if (menu_) skin.menu(g, "editor", kMenuItems, menuSel_);
}
