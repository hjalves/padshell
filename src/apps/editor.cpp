#include "editor.h"

#include <algorithm>
#include <cstdio>

#include "../shell.h"
#include "../utf8.h"

EditorApp::EditorApp(std::string path) : path_(std::move(path)), ime_(std::make_unique<DaisyWheel>()) {
    buf_.load(path_);  // missing file = start empty
}

std::string EditorApp::title() const {
    std::string name = path_.substr(path_.find_last_of('/') + 1);
    return "Editor - " + name + (buf_.modified() ? " *" : "");
}

bool EditorApp::save(Shell* shell) {
    bool ok = buf_.save(path_);
    if (shell) shell->toast(ok ? "Saved" : "Save failed!");
    if (!ok) SDL_Log("save failed: %s", path_.c_str());
    return ok;
}

void EditorApp::onLeave() {
    if (buf_.modified()) save(nullptr);
}

void EditorApp::update(const Gamepad& pad, Shell& shell) {
    if (pad.hit(Plus)) {
        save(&shell);
        return;
    }

    std::vector<TextEvent> events;
    ime_->update(pad, events);
    for (const TextEvent& e : events) {
        switch (e.kind) {
            case TextEvent::Insert: buf_.insert(e.text); break;
            case TextEvent::Backspace: buf_.backspace(); break;
            case TextEvent::Delete: buf_.deleteForward(); break;
            case TextEvent::Newline: buf_.newline(); break;
            case TextEvent::Left: buf_.left(); break;
            case TextEvent::Right: buf_.right(); break;
            case TextEvent::Up: buf_.up(); break;
            case TextEvent::Down: buf_.down(); break;
            case TextEvent::Home: buf_.home(); break;
            case TextEvent::End: buf_.end(); break;
            case TextEvent::PageUp: buf_.pageUp(visibleLines_ - 1); break;
            case TextEvent::PageDown: buf_.pageDown(visibleLines_ - 1); break;
        }
    }
    if (!events.empty()) lastEdit_ = SDL_GetTicks();
}

void EditorApp::render(Gfx& g, Rect area) {
    Font& f = g.text();
    const Font& sf = g.small();
    const int statusH = sf.lineH() + 10;
    Rect textArea{area.x, area.y, area.w - kImeW, area.h - statusH};
    Rect imeArea{area.x + area.w - kImeW, area.y, kImeW, area.h};

    const auto& lines = buf_.lines();
    const int gutterCols = std::max(3, (int)std::to_string(lines.size()).size());
    const int gutterW = (gutterCols + 2) * f.cellW();
    const int pad = 8;
    visibleLines_ = std::max(1, (textArea.h - 2 * pad) / f.lineH());
    const int visibleCols = std::max(1, (textArea.w - gutterW - 2 * pad) / f.cellW());

    // Scroll so the cursor stays on screen.
    const int cl = buf_.line(), cc = (int)buf_.column();
    if (cl < top_) top_ = cl;
    if (cl >= top_ + visibleLines_) top_ = cl - visibleLines_ + 1;
    if (cc < left_) left_ = cc;
    if (cc >= left_ + visibleCols) left_ = cc - visibleCols + 1;

    g.fill({textArea.x, textArea.y, gutterW, textArea.h}, theme::panel);
    for (int row = 0; row < visibleLines_; ++row) {
        int ln = top_ + row;
        if (ln >= (int)lines.size()) break;
        int y = textArea.y + pad + row * f.lineH();
        std::string num = std::to_string(ln + 1);
        num.insert(0, gutterCols - num.size(), ' ');
        g.print(f, textArea.x + f.cellW(), y, num, ln == cl ? theme::text : theme::dim);
        const std::string& s = lines[ln];
        size_t from = utf8::offset(s, left_), to = utf8::offset(s, left_ + visibleCols);
        g.print(f, textArea.x + gutterW + pad, y, std::string_view(s).substr(from, to - from), theme::text);
    }

    // Cursor: solid while typing, blinking when idle.
    uint32_t idle = SDL_GetTicks() - lastEdit_;
    if (idle < 500 || (idle / 530) % 2 == 0) {
        int x = textArea.x + gutterW + pad + (cc - left_) * f.cellW();
        int y = textArea.y + pad + (cl - top_) * f.lineH();
        g.fill({x - 1, y + 2, 3, f.lineH() - 4}, theme::cursor);
    }

    // Status line.
    Rect st{area.x, area.y + area.h - statusH, area.w - kImeW, statusH};
    g.fill(st, theme::panelHi);
    char info[64];
    std::snprintf(info, sizeof info, "Ln %d, Col %d   %zu lines", cl + 1, cc + 1, lines.size());
    g.print(g.small(), st.x + 12, st.y + 5, info, theme::dim);
    const char* hint = buf_.modified() ? "modified  +: save" : "+: save";
    g.print(g.small(), st.x + st.w - 12 - Gfx::width(g.small(), hint), st.y + 5, hint,
            buf_.modified() ? theme::warn : theme::dim);

    ime_->render(g, imeArea);
}
