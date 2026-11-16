// Skins: interchangeable visual designs. The shell and the editor keep all
// state and behaviour; each frame they hand a read-only "view" of it to the
// current skin, which draws everything. One file per skin in src/skins/.
#pragma once
#include <string>
#include <vector>

#include "gemini/gemtext.h"
#include "gfx.h"
#include "multitap.h"
#include "pad.h"

struct ShellView {
    std::string heading;  // short lowercase app name, e.g. "notes"
    std::string title;    // longer app title, e.g. "Editor - notes.txt *"
    std::string clock;    // "HH:MM"
    std::string toast;    // transient message, empty if none
    const Gamepad* pad = nullptr;  // last input, for controller diagrams
};

struct WrapRow { int line; size_t from, to; bool first; };

struct EditorView {
    const std::vector<std::string>* lines = nullptr;
    bool modified = false;
    const MultiTap* tap = nullptr;
    uint32_t held = 0;                  // buttons currently down
    int cursorLine = 0, cursorCol = 0;  // cursor position (codepoint column)

    MultiTap::Mode mode() const { return tap->mode(); }
    bool empty() const { return lines->size() == 1 && (*lines)[0].empty(); }
    int lineCount() const { return (int)lines->size(); }
    size_t chars() const;

    // The document as displayed: the pending character (if any) inserted at
    // the cursor, wrapped into rows of at most `cols` codepoints, and where
    // the cursor cell lands. The cursor's line always has room for its cell.
    struct Layout {
        std::vector<std::string> lines;
        std::vector<WrapRow> rows;
        int cursorRow = 0, cursorX = 0;
        std::string_view rowText(const WrapRow& r) const {
            return std::string_view(lines[r.line]).substr(r.from, r.to - r.from);
        }
    };
    Layout layout(int cols) const;
};

// Written by the skin while drawing a page, read back by the browser on the
// next frame to scroll and move between links.
struct BrowserLayout {
    int rows = 0;     // visual rows in the page
    int visible = 1;  // rows that fit on screen
    std::vector<std::pair<int, int>> links;  // first and last row of each link
};

struct BrowserView {
    std::string url;
    std::string status;  // e.g. "20 text/gemini", "loading..."
    bool loading = false;
    const std::vector<gem::Line>* lines = nullptr;
    int selected = -1;   // link index
    float scroll = 0;    // first visible row (fractional for smooth scrolling)
    uint32_t held = 0;
    // Address bar or a Gemini input prompt (status 10/11).
    bool input = false;
    std::string prompt;
    bool sensitive = false;
    EditorView field;
    BrowserLayout* layout = nullptr;  // filled in by the skin
};

class Skin {
public:
    virtual ~Skin() = default;
    virtual const char* name() const = 0;
    // Background and shell chrome. Returns the area the app may draw in.
    virtual Rect frame(Gfx& g, const ShellView& v) = 0;
    virtual void editor(Gfx& g, Rect area, const EditorView& v) = 0;
    virtual void browser(Gfx&, Rect, const BrowserView&) {}
    // A modal list (app launcher, editor menu), drawn over everything.
    virtual void menu(Gfx& g, const std::string& title, const std::vector<std::string>& items, int sel) = 0;
    // Called last each frame (post-processing, upscaling...).
    virtual void finish(Gfx&) {}
};

namespace skins {
Skin& current();

// The colour theme and the font are chosen independently in the app menu and
// saved in settings.ini. Selecting wraps around and saves.
int themeCount();
int themeIndex();
std::string themeName(int i);
void selectTheme(int i);
int fontCount();
int fontIndex();
std::string fontName(int i);
void selectFont(int i);
void loadSettings(const std::string& path);

// --- helpers shared by skins ---
// Characters of a group with the current case applied, joined by `sep`.
std::string groupText(const MultiTap& tap, const char* chars, const char* sep = "");
// The groups for the current mode (nullptr in command mode).
const MultiTap::Groups* groupsFor(MultiTap::Mode m);
const MultiTap::Group* groupFor(MultiTap::Mode m, Btn b);
const MultiTap::Command* commandFor(Btn b);
bool isDpad(Btn b);
Dir dirOf(Btn b);
// Button label: text for face/shoulder buttons, a drawn arrow for the D-pad.
// Returns the width used.
int buttonLabel(Gfx& g, Font& f, int x, int y, Btn b, Color c);
// Spec cursor colours: pink (letters), green (ZL symbols), red (ZR commands).
Color modeColor(MultiTap::Mode m);
const char* modeName(MultiTap::Mode m, bool upper);
}  // namespace skins
