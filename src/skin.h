// Skins: interchangeable visual designs. The shell and the editor keep all
// state and behaviour; each frame they hand a read-only "view" of it to the
// current skin, which draws everything. One file per skin in src/skins/.
#pragma once
#include <string>
#include <vector>

#include "gfx.h"
#include "multitap.h"
#include "pad.h"

struct ShellView {
    std::string heading;  // short lowercase app name, e.g. "notes"
    std::string title;    // longer app title, e.g. "Editor - notes.txt *"
    std::string clock;    // "HH:MM"
    std::string toast;    // transient message, empty if none
    std::vector<std::string> help;
    const Gamepad* pad = nullptr;  // last input, for controller diagrams
};

struct WrapRow { int line; size_t from, to; bool first; };

struct EditorView {
    const std::vector<std::string>* lines = nullptr;
    bool modified = false;
    const MultiTap* tap = nullptr;
    uint32_t held = 0;  // buttons currently down

    MultiTap::Mode mode() const { return tap->mode(); }
    bool empty() const { return lines->size() == 1 && (*lines)[0].empty(); }
    int lineCount() const { return (int)lines->size(); }
    size_t chars() const;
    // Codepoint column of the end of the document (where the cursor sits).
    int endColumn() const;
    // Visual rows of at most `cols` codepoints. The last line always gets room
    // for the cursor cell (where the pending character is previewed).
    std::vector<WrapRow> wrap(int cols) const;
    std::string_view rowText(const WrapRow& r) const;
};

class Skin {
public:
    virtual ~Skin() = default;
    virtual const char* name() const = 0;
    // Background and shell chrome. Returns the area the app may draw in.
    virtual Rect frame(Gfx& g, const ShellView& v) = 0;
    virtual void editor(Gfx& g, Rect area, const EditorView& v) = 0;
    // A modal list (app launcher, editor menu), drawn over everything.
    virtual void menu(Gfx& g, const std::string& title, const std::vector<std::string>& items, int sel) = 0;
    // Called last each frame (post-processing, upscaling...).
    virtual void finish(Gfx&) {}
};

namespace skins {
const std::vector<Skin*>& all();
Skin& current();
int index();
void select(int i);  // also saves the setting
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
