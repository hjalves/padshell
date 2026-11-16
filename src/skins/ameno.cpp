// Ameno: an adaptation of the "ameno" TV web app's editor. A slim top bar
// (brand, counts, clock), a large document with characters coloured by kind,
// and a status line of flat button badges with their groups, the pending
// character's cycle, mode tags and a commit timer.
//
// It is the only layout. The colour theme and the font are chosen
// independently (app menu, saved in settings.ini); both tables and the
// settings live at the bottom of this file.
#include <cstdio>
#include <cstring>

#include "family.h"

namespace {
using namespace fam;

constexpr int kM = 64;         // side margin
constexpr int kTopY = 42;      // top bar text row; mirrors the badges row's distance from the bottom
constexpr int kStatusY = 640;  // badges row; timer below it, just inside the TV-safe bottom margin

struct Theme { const char* name; const Palette& palette; };
const Theme kThemes[] = {
    {"sunset", themes::sunset}, {"dusk", themes::dusk},         {"lava", themes::lava},
    {"gold", themes::gold},     {"tropic", themes::tropic},     {"rose", themes::rose},
    {"ember", themes::ember},   {"midnight", themes::midnight}, {"moon", themes::moon},     {"ocean", themes::ocean},
    {"carbon", themes::carbon}, {"neon", themes::neon},         {"cyber", themes::cyber},
    {"vapor", themes::vapor},   {"ice", themes::ice},           {"night", themes::night},
    {"day", themes::day},
};
struct FontPair { const char* id; const char* name; const char* regular; const char* bold; };
const FontPair kFonts[] = {
    {"space-mono", "Space Mono", "SpaceMono-Regular.ttf", "SpaceMono-Bold.ttf"},
    {"jetbrains-mono", "JetBrains Mono", "JetBrainsMono-Regular.ttf", "JetBrainsMono-Bold.ttf"},
    {"ibm-plex-mono", "IBM Plex Mono", "IBMPlexMono-Regular.ttf", "IBMPlexMono-Bold.ttf"},
    {"fira-mono", "Fira Mono", "FiraMono-Regular.ttf", "FiraMono-Bold.ttf"},
    {"dejavu-sans-mono", "DejaVu Sans Mono", "DejaVuSansMono.ttf", "DejaVuSansMono-Bold.ttf"},
    {"courier-prime", "Courier Prime", "CourierPrime-Regular.ttf", "CourierPrime-Bold.ttf"},
};
constexpr int kThemeCount = sizeof kThemes / sizeof *kThemes;
constexpr int kFontCount = sizeof kFonts / sizeof *kFonts;

// Current choices (defaults: sunset, Space Mono) and where they are saved.
int g_theme = 0, g_font = 0;
std::string g_settingsPath;

class Ameno : public Skin {
public:
    const char* name() const override { return "ameno"; }

    Rect frame(Gfx& g, const ShellView& v) override {
        sync();
        g.clear(P.bg);
        Font& brand = g.font(P.bold, 30);
        int x = kM + g.draw(brand, kM, kTopY, "padshell", ink(P.letters, P));
        g.draw(g.font(P.mono, 30), x + 16, kTopY, v.heading, P.muted);
        Font& clock = g.font(P.mono, 30);
        g.draw(clock, Gfx::W - kM - Gfx::measure(clock, v.clock), kTopY, v.clock, P.text);
        if (!v.toast.empty()) {
            Font& t = g.font(P.mono, 15);
            g.draw(t, Gfx::W - kM - Gfx::measure(t, v.toast), kTopY + 44, v.toast, ink(P.caps, P));
        }
        return {kM, kTopY + 44, Gfx::W - 2 * kM, Gfx::H - (kTopY + 44)};
    }

    void editor(Gfx& g, Rect a, const EditorView& v) override {
        sync();
        Font& meta = g.font(P.mono, 17);
        g.draw(meta, kM, kTopY + 44,
               "ln " + std::to_string(v.cursorLine + 1) + ", col " + std::to_string(v.cursorCol + 1) +
                   " \xC2\xB7 " + std::to_string(v.lineCount()) + (v.lineCount() == 1 ? " line" : " lines") +
                   " \xC2\xB7 " + std::to_string(v.chars()) + " chars" + (v.modified ? " \xC2\xB7 unsaved" : ""),
               P.muted);

        Font& f = g.font(P.mono, 38);
        document(g, {kM, kTopY + 97, a.w, 8 * 54}, v, P, {&f, 54, 6}, top_);

        status(g, v);
    }

    void menu(Gfx& g, const std::string& title, const std::vector<std::string>& items, int sel) override {
        sync();
        g.fill({0, kTopY + 80, Gfx::W, kStatusY - 16 - (kTopY + 80)}, P.bg);
        g.draw(g.font(P.mono, 21), kM, kTopY + 107, title, P.muted);
        Font& f = g.font(P.mono, 38), &fb = g.font(P.bold, 38);
        int y = kTopY + 160;
        for (int i = 0; i < (int)items.size(); ++i) {
            bool on = i == sel;
            if (on) g.fill({kM, y, 600, 64}, P.letters);
            g.draw(on ? fb : f, kM + 21, y + (64 - f.lineH()) / 2, items[i], on ? P.dark : mix(P.text, P.muted, 0.3f));
            y += 72;
        }
        // Status line becomes the menu's hints.
        g.fill({0, kStatusY - 16, Gfx::W, Gfx::H - kStatusY + 16}, P.bg);
        Font& sf = g.font(P.mono, 20);
        int x = kM, y0 = kStatusY, s = 30;
        auto hint = [&](std::initializer_list<Btn> bs, const char* what) {
            for (Btn b : bs) {
                badge(g, x, y0, s, b, buttonColor(b, P), P.dark, P.bold);
                x += s + 4;
            }
            x += 4 + g.draw(sf, x + 4, y0 + (s - sf.lineH()) / 2, what, P.text) + 24;
        };
        hint({DUp, DDown}, "navigate");
        hint({kConfirm}, "select");
        hint({kBack}, "cancel");
        tag(g, sf, Gfx::W - kM - 70, y0, s, "menu", P.text, P.bg);
    }

private:
    void status(Gfx& g, const EditorView& v) {
        const int y = kStatusY, s = 30;
        Font& f = g.font(P.mono, 20);
        if (v.tap->hasPending()) {
            Btn b = pendingButton(v);
            badge(g, kM, y - 4, 38, b, buttonColor(b, P), P.dark, P.bold);
            cycle(g, kM + 38 + 12, y - 4, 38, v, P);
        } else {
            groupsLine(g, kM, y, s, f, v, P);
        }
        modeTags(g, Gfx::W - kM, y, s, g.font(P.bold, 18), v, P);

        // Commit timer.
        g.fill({kM, kStatusY + 44, Gfx::W - 2 * kM, 4}, P.surface);
        if (float t = timerLeft(v); t > 0)
            g.fill({kM, kStatusY + 44, (int)((Gfx::W - 2 * kM) * t), 4}, buttonColor(pendingButton(v), P));
    }

    // The current theme's colours with the current font filled in.
    void sync() {
        P = kThemes[g_theme].palette;
        P.mono = kFonts[g_font].regular;
        P.bold = kFonts[g_font].bold;
    }

    Palette P = themes::sunset;
    int top_ = 0;  // first visible document row
};

int wrap(int i, int n) { return ((i % n) + n) % n; }

void save() {
    if (g_settingsPath.empty()) return;
    if (FILE* f = std::fopen(g_settingsPath.c_str(), "w")) {
        std::fprintf(f, "theme=%s\nfont=%s\n", kThemes[g_theme].name, kFonts[g_font].id);
        std::fclose(f);
    }
}

int findTheme(const char* name) {
    for (int i = 0; i < kThemeCount; ++i)
        if (std::strcmp(kThemes[i].name, name) == 0) return i;
    return -1;
}

int findFont(const char* id) {
    for (int i = 0; i < kFontCount; ++i)
        if (std::strcmp(kFonts[i].id, id) == 0) return i;
    return -1;
}
}  // namespace

namespace skins {
Skin& current() {
    static Ameno s;
    return s;
}

int themeCount() { return kThemeCount; }
int themeIndex() { return g_theme; }
std::string themeName(int i) { return kThemes[wrap(i, kThemeCount)].name; }
void selectTheme(int i) { g_theme = wrap(i, kThemeCount); save(); }
int fontCount() { return kFontCount; }
int fontIndex() { return g_font; }
std::string fontName(int i) { return kFonts[wrap(i, kFontCount)].name; }
void selectFont(int i) { g_font = wrap(i, kFontCount); save(); }

void loadSettings(const std::string& path) {
    g_settingsPath = path;
    FILE* f = std::fopen(path.c_str(), "r");
    if (!f) return;  // first run: defaults
    char line[128];
    bool fontSet = false;
    while (std::fgets(line, sizeof line, f)) {
        line[std::strcspn(line, "\r\n")] = 0;
        if (std::strncmp(line, "theme=", 6) == 0) {
            if (int i = findTheme(line + 6); i >= 0) g_theme = i;
        } else if (std::strncmp(line, "font=", 5) == 0) {
            if (int i = findFont(line + 5); i >= 0) { g_font = i; fontSet = true; }
        } else if (std::strncmp(line, "style=", 6) == 0) {
            // Older versions saved one "style" per theme+font, e.g.
            // "ameno-sunset"; "ameno" alone was the midnight theme.
            std::string s = line + 6;
            if (s.rfind("ameno-", 0) == 0) s = s.substr(6);
            if (s == "ameno") s = "midnight";
            if (s == "neon-space") {
                s = "neon";
                if (!fontSet) g_font = findFont("space-mono");
            }
            if (int i = findTheme(s.c_str()); i >= 0) g_theme = i;
        }
    }
    std::fclose(f);
}
}  // namespace skins
