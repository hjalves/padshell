#include "editor.h"

#include <algorithm>
#include <cstring>

#include "../shell.h"
#include "../utf8.h"

namespace {
constexpr Color kCursorLetters{255, 121, 198};  // pink
constexpr Color kCursorSymbols{80, 220, 120};   // green: left trigger held
constexpr Color kCursorCommand{240, 80, 80};    // red: right trigger held
const char* const kMenuItems[] = {"Save", "Quit padshell"};
constexpr int kMenuCount = sizeof kMenuItems / sizeof *kMenuItems;
}  // namespace

EditorApp::EditorApp(std::string path) : path_(std::move(path)) {
    buf_.load(path_);  // missing file = start empty
    buf_.moveToEnd();
}

std::string EditorApp::title() const {
    std::string name = path_.substr(path_.find_last_of('/') + 1);
    return "Editor - " + name + (buf_.modified() ? " *" : "");
}

std::vector<std::string> EditorApp::help() const {
    return {
        std::string(btnLabel(ZL)) + " hold: numbers & symbols",
        std::string(btnLabel(ZR)) + " hold: newline, space, delete",
        std::string(btnLabel(L)) + ": uppercase",
        "+: editor menu    -: apps",
    };
}

bool EditorApp::save(Shell* shell) {
    std::vector<TextEvent> ev;
    tap_.flush(ev);
    apply(ev);
    bool ok = buf_.save(path_);
    if (shell) shell->toast(ok ? "Saved" : "Save failed!");
    if (!ok) SDL_Log("save failed: %s", path_.c_str());
    return ok;
}

void EditorApp::onLeave() {
    std::vector<TextEvent> ev;
    tap_.flush(ev);
    apply(ev);
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
    if (menu_) {
        updateMenu(pad, shell);
        return;
    }
    if (pad.hit(Plus)) {
        std::vector<TextEvent> ev;
        tap_.flush(ev);
        apply(ev);
        menu_ = true;
        menuSel_ = 0;
        return;
    }
    std::vector<TextEvent> ev;
    tap_.update(pad, ev);
    apply(ev);
}

void EditorApp::updateMenu(const Gamepad& pad, Shell& shell) {
    if (pad.hit(Plus) || pad.hit(kBack)) { menu_ = false; return; }
    if (pad.rep(DUp)) menuSel_ = (menuSel_ + kMenuCount - 1) % kMenuCount;
    if (pad.rep(DDown)) menuSel_ = (menuSel_ + 1) % kMenuCount;
    if (!pad.hit(kConfirm)) return;
    menu_ = false;
    switch (menuSel_) {
        case 0: save(&shell); break;
        case 1: shell.quit(); break;
    }
}

void EditorApp::render(Gfx& g, Rect area) {
    Font& f = g.text();
    const int statusH = 2 * g.small().lineH() + 14;
    Rect ta{area.x, area.y, area.w, area.h - statusH};

    const auto& lines = buf_.lines();
    const int last = (int)lines.size() - 1;
    const int gutterCols = std::max(3, (int)std::to_string(lines.size()).size());
    const int gutterW = (gutterCols + 2) * f.cellW();
    const int pad = 8;
    const int cols = std::max(1, (ta.w - gutterW - 2 * pad) / f.cellW());
    const int visible = std::max(1, (ta.h - 2 * pad) / f.lineH());

    // Soft-wrap into visual rows. The last line gets room for the cursor cell
    // (where the pending character is previewed).
    struct Row { int line; size_t from, to; bool first; };
    std::vector<Row> rows;
    for (int i = 0; i <= last; ++i) {
        const std::string& s = lines[i];
        int len = (int)utf8::length(s);
        int n = i == last ? len / cols + 1 : std::max(1, (len + cols - 1) / cols);
        for (int k = 0; k < n; ++k)
            rows.push_back({i, utf8::offset(s, (size_t)k * cols), utf8::offset(s, (size_t)(k + 1) * cols), k == 0});
    }
    const int top = std::max(0, (int)rows.size() - visible);  // follow the end

    g.fill({ta.x, ta.y, gutterW, ta.h}, theme::panel);
    const int textX = ta.x + gutterW + pad;
    for (int r = top; r < (int)rows.size() && r < top + visible; ++r) {
        const Row& row = rows[r];
        int y = ta.y + pad + (r - top) * f.lineH();
        if (row.first) {
            std::string num = std::to_string(row.line + 1);
            num.insert(0, gutterCols - num.size(), ' ');
            g.print(f, ta.x + f.cellW(), y, num, row.line == last ? theme::text : theme::dim);
        }
        g.print(f, textX, y, std::string_view(lines[row.line]).substr(row.from, row.to - row.from), theme::text);
    }

    // Underline cursor at the end of the document, coloured by trigger mode,
    // with the character being selected previewed on top of it.
    int endCol = (int)utf8::length(lines[last]);
    int cx = textX + (endCol % cols) * f.cellW();
    int cy = ta.y + pad + ((int)rows.size() - 1 - top) * f.lineH();
    Color cc = tap_.mode() == MultiTap::Mode::Commands ? kCursorCommand
             : tap_.mode() == MultiTap::Mode::Symbols  ? kCursorSymbols
                                                       : kCursorLetters;
    if (tap_.hasPending()) {
        g.fill({cx, cy, f.cellW(), f.lineH()}, {cc.r, cc.g, cc.b, 60});
        g.print(f, cx, cy, tap_.pendingText(), theme::text);
    }
    g.fill({cx, cy + f.lineH() - 4, f.cellW(), 3}, cc);

    renderStatus(g, {area.x, area.y + area.h - statusH, area.w, statusH});
    if (menu_) renderMenu(g, area);
}

void EditorApp::renderStatus(Gfx& g, Rect st) {
    Font& sf = g.small();
    g.fill(st, theme::panelHi);
    int x = st.x + 12, y = st.y + 6;

    // Line 1: the pending character's cycle, or what each button does now.
    if (tap_.hasPending()) {
        x += g.print(sf, x, y, "Selecting: ", theme::dim);
        const char* grp = tap_.pendingGroup();
        int cur = tap_.pendingIndex();
        for (int i = 0; grp[i]; ++i) {
            if (i == cur) g.fill({x - 3, y - 1, sf.cellW() + 6, sf.lineH() + 2}, theme::accent);
            g.print(sf, x, y, tap_.display(grp[i]), i == cur ? theme::bg : theme::text);
            x += 2 * sf.cellW();
        }
    } else if (tap_.mode() == MultiTap::Mode::Commands) {
        for (const auto& c : MultiTap::commands()) {
            x += g.print(sf, x, y, btnLabel(c.button), theme::accent) + sf.cellW();
            x += g.print(sf, x, y, c.label, theme::text) + 3 * sf.cellW();
        }
    } else {
        const auto& groups = tap_.mode() == MultiTap::Mode::Symbols ? MultiTap::symbols() : MultiTap::letters();
        for (const auto& grp : groups) {
            x += g.print(sf, x, y, btnLabel(grp.button), theme::accent) + sf.cellW() / 2;
            std::string chars;
            for (const char* c = grp.chars; *c; ++c) chars += tap_.display(*c);
            x += g.print(sf, x, y, chars, theme::text) + 2 * sf.cellW();
        }
    }

    // Line 2: document info and modes.
    y += sf.lineH() + 2;
    size_t chars = 0;
    for (const auto& l : buf_.lines()) chars += utf8::length(l) + 1;
    std::string info = "Ln " + std::to_string(buf_.lines().size()) + "   " + std::to_string(chars - 1) + " chars";
    g.print(sf, st.x + 12, y, info, theme::dim);

    std::string right;
    if (tap_.uppercase()) right += "UPPERCASE   ";
    if (buf_.modified()) right += "modified   ";
    right += "+: menu";
    g.print(sf, st.x + st.w - 12 - Gfx::width(sf, right), y, right,
            tap_.uppercase() || buf_.modified() ? theme::warn : theme::dim);
}

void EditorApp::renderMenu(Gfx& g, Rect area) {
    g.fill(area, {0, 0, 0, 150});
    Font& f = g.text();
    int rowH = f.lineH() + 16, w = 360, h = kMenuCount * rowH + 70;
    Rect box{area.x + (area.w - w) / 2, area.y + (area.h - h) / 2, w, h};
    g.fill(box, theme::panel);
    g.outline(box, theme::panelHi);
    g.print(g.small(), box.x + 20, box.y + 14, "Editor", theme::dim);
    for (int i = 0; i < kMenuCount; ++i) {
        Rect row{box.x + 10, box.y + 44 + i * rowH, w - 20, rowH};
        if (i == menuSel_) g.fill(row, theme::accent);
        g.print(f, row.x + 14, row.y + 8, kMenuItems[i], i == menuSel_ ? theme::bg : theme::text);
    }
    std::string hint = std::string(btnLabel(kConfirm)) + ": select   " + btnLabel(kBack) + ": cancel";
    g.printCentered(g.small(), box.x + w / 2, box.y + h - 4 - g.small().lineH(), hint, theme::dim);
}
