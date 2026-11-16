#include "editor.h"

#include <algorithm>
#include <cctype>
#include <cstring>

#include "../shell.h"
#include "../ui.h"
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
    lastHeld_ = pad.held;
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

std::vector<EditorApp::Row> EditorApp::wrap(int cols) const {
    const auto& lines = buf_.lines();
    const int last = (int)lines.size() - 1;
    std::vector<Row> rows;
    for (int i = 0; i <= last; ++i) {
        const std::string& s = lines[i];
        int len = (int)utf8::length(s);
        int n = i == last ? len / cols + 1 : std::max(1, (len + cols - 1) / cols);
        for (int k = 0; k < n; ++k)
            rows.push_back({i, utf8::offset(s, (size_t)k * cols), utf8::offset(s, (size_t)(k + 1) * cols), k == 0});
    }
    return rows;
}

void EditorApp::render(Gfx& g, Rect area) {
    if (ui::style() == ui::Style::Metro) renderMetro(g, area);
    else renderClassic(g, area);
}

void EditorApp::renderClassic(Gfx& g, Rect area) {
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

    const std::vector<Row> rows = wrap(cols);
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

// ---------------------------------------------------------------------------
// Metro / TV style

namespace {
using namespace ui::metro;

Color modeColor(MultiTap::Mode m) {
    return m == MultiTap::Mode::Commands ? red : m == MultiTap::Mode::Symbols ? green : pink;
}

// Tile order follows the controller, left to right: D-pad, then face buttons.
constexpr Btn kTileOrder[8] = {DLeft, DUp, DDown, DRight, West, North, South, East};

Color tileColor(MultiTap::Mode m, Btn b) {
    switch (m) {
        case MultiTap::Mode::Letters:
            switch (b) {
                case DLeft: return teal;     case DUp: return emerald;
                case DDown: return cobalt;   case DRight: return violet;
                case West: return orange;    case North: return amber;
                case South: return magenta;  default: return cyan;
            }
        case MultiTap::Mode::Symbols:
            switch (b) {
                case DLeft: return emerald;  case DUp: return green;
                case DDown: return teal;     case DRight: return olive;
                case West: return Color{51, 153, 51};  case North: return Color{130, 160, 0};
                case South: return Color{0, 128, 96};  default: return Color{0, 150, 136};
            }
        case MultiTap::Mode::Commands:
            switch (b) {
                case South: return red;      case West: return orange;
                case East: return crimson;   case North: return magenta;
                default: return faint;
            }
    }
    return faint;
}
}  // namespace

void EditorApp::renderMetro(Gfx& g, Rect a) {
    const int tileH = 116, chipH = 34;
    Rect strip{a.x, a.y + a.h - tileH, a.w, tileH};
    const int chipY = strip.y - chipH - 18;
    Rect ta{a.x, a.y, a.w, chipY - 20 - a.y};
    const Color mc = modeColor(tap_.mode());

    // Document: large monospace, wrapped, scrolled to follow the end.
    Font& f = g.monoTV();
    const int cols = std::max(1, ta.w / f.cellW());
    const int visible = std::max(1, ta.h / f.lineH());
    const auto& lines = buf_.lines();
    const std::vector<Row> rows = wrap(cols);
    const int top = std::max(0, (int)rows.size() - visible);
    for (int r = top; r < (int)rows.size() && r < top + visible; ++r) {
        const Row& row = rows[r];
        g.print(f, ta.x, ta.y + (r - top) * f.lineH(),
                std::string_view(lines[row.line]).substr(row.from, row.to - row.from), text);
    }
    const bool empty = lines.size() == 1 && lines[0].empty();
    if (empty && !tap_.hasPending())
        g.draw(g.lightMid(), ta.x + f.cellW() + 12, ta.y - 6, "start typing", Color{70, 70, 70});

    int endCol = (int)utf8::length(lines.back());
    int cx = ta.x + (endCol % cols) * f.cellW();
    int cy = ta.y + ((int)rows.size() - 1 - top) * f.lineH();
    if (tap_.hasPending()) {
        g.fill({cx, cy, f.cellW(), f.lineH()}, mc);
        g.print(f, cx, cy, tap_.pendingText(), bg);
    }
    g.fill({cx, cy + f.lineH() - 6, f.cellW(), 6}, mc);

    // Chips: trigger / shoulder state on the left, document state on the right.
    struct Chip { std::string label; bool on; Color c; };
    const Chip chips[] = {
        {std::string(btnLabel(ZL)) + "  symbols", tap_.mode() == MultiTap::Mode::Symbols, green},
        {std::string(btnLabel(ZR)) + "  edit", tap_.mode() == MultiTap::Mode::Commands, red},
        {std::string(btnLabel(L)) + "  shift", tap_.uppercase(), pink},
    };
    int x = a.x;
    for (const Chip& c : chips) {
        int w = Gfx::measure(g.sans(), c.label) + 28;
        g.fill({x, chipY, w, chipH}, c.on ? c.c : faint);
        g.draw(g.sans(), x + 14, chipY + (chipH - g.sans().lineH()) / 2, c.label, c.on ? text : dim);
        x += w + 10;
    }
    std::string right = std::string(buf_.modified() ? "unsaved" : "saved") + "      +  menu      -  apps";
    int rw = Gfx::measure(g.sans(), right);
    g.draw(g.sans(), a.x + a.w - rw, chipY + (chipH - g.sans().lineH()) / 2, right, dim);
    if (buf_.modified()) g.fill({a.x + a.w - rw - 22, chipY + chipH / 2 - 5, 10, 10}, amber);

    renderTiles(g, strip);

    if (menu_) {
        std::vector<std::string> items;
        for (const char* m : kMenuItems) {
            std::string s = m;
            for (char& ch : s) ch = (char)std::tolower((unsigned char)ch);
            items.push_back(s);
        }
        drawMenu(g, "EDITOR", items, menuSel_, mc);
    }
}

void EditorApp::renderTiles(Gfx& g, Rect strip) {
    const int gap = 8, groupGap = 48;
    const int tileW = (strip.w - 6 * gap - groupGap) / 8;
    const MultiTap::Mode mode = tap_.mode();
    const auto& groups = mode == MultiTap::Mode::Symbols ? MultiTap::symbols() : MultiTap::letters();
    Font& mf = g.monoTile();

    for (int i = 0; i < 8; ++i) {
        const Btn b = kTileOrder[i];
        Rect t{strip.x + i * (tileW + gap) + (i >= 4 ? groupGap - gap : 0), strip.y, tileW, strip.h};
        Color c = tileColor(mode, b);
        const bool pressed = lastHeld_ & b;
        g.fill(t, pressed ? mix(c, text, 0.35f) : c);
        g.draw(g.sansSmall(), t.x + 12, t.y + 8, btnLabel(b), Color{255, 255, 255, 190});

        if (mode == MultiTap::Mode::Commands) {
            for (const auto& cmd : MultiTap::commands()) {
                if (cmd.button != b) continue;
                // Too wide: break at the space, or fall back to the smaller font.
                std::string l1 = cmd.label, l2;
                Font* lf = &g.sans();
                if (Gfx::measure(*lf, l1) > t.w - 24) {
                    if (l1.find(' ') != std::string::npos) {
                        l2 = l1.substr(l1.find(' ') + 1);
                        l1 = l1.substr(0, l1.find(' '));
                    } else {
                        lf = &g.sansSmall();
                    }
                }
                int lh = lf->lineH();
                int y = t.y + t.h - 12 - (l2.empty() ? 1 : 2) * lh;
                g.draw(*lf, t.x + 12, y, l1, text);
                if (!l2.empty()) g.draw(*lf, t.x + 12, y + lh, l2, text);
            }
            continue;
        }

        const MultiTap::Group* grp = nullptr;
        for (const auto& gr : groups)
            if (gr.button == b) grp = &gr;
        if (!grp) continue;
        const bool pendingHere = tap_.hasPending() && tap_.pendingGroup() == grp->chars;
        const int cur = pendingHere ? tap_.pendingIndex() : -1;
        const int n = (int)std::strlen(grp->chars);
        const int perLine = n > 5 ? (n + 1) / 2 : n;
        const int nLines = (n + perLine - 1) / perLine;
        const int step = mf.cellW() + 3;
        for (int k = 0; k < n; ++k) {
            int lx = t.x + 12 + (k % perLine) * step;
            int ly = t.y + t.h - 10 - (nLines - k / perLine) * mf.lineH();
            if (k == cur) g.fill({lx - 2, ly + 2, mf.cellW() + 4, mf.lineH() - 2}, text);
            g.print(mf, lx, ly, tap_.display(grp->chars[k]), k == cur ? c : text);
        }
    }
}
