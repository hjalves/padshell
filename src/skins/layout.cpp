// The layout: a slim top bar (brand, counts, clock), a large document with
// characters coloured by kind, and a status line of flat button badges with
// their groups, the pending character's cycle, mode tags and a commit timer;
// plus the Gemini page view.
//
// It is the only layout. The colour theme, the font and the starfield are
// chosen independently (app menu > settings, saved in settings.ini); the
// tables and the settings live at the bottom of this file.
#include <cstdio>
#include <cstring>

#include "../utf8.h"
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
    {"space", themes::space},       {"day", themes::day},
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
bool g_starfield = false;  // moving starfield behind any theme
std::string g_settingsPath;

class Layout : public Skin {
public:
    const char* name() const override { return "layout"; }

    Rect frame(Gfx& g, const ShellView& v) override {
        sync();
        g.clear(P.bg);
        if (g_starfield) starfield(g, P.light);
        Font& brand = g.font(P.bold, 30);
        int x = kM + g.draw(brand, kM, kTopY, "padshell", ink(P.letters, P));
        g.draw(g.font(P.mono, 30), x + 16, kTopY, v.heading, P.muted);
        Font& clock = g.font(P.mono, 30);
        g.draw(clock, Gfx::W - kM - Gfx::measure(clock, v.clock), kTopY, v.clock, P.text);
        toast_ = !v.toast.empty();
        if (toast_) {
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

    // --- Gemini browser ------------------------------------------------------

    void browser(Gfx& g, Rect a, const BrowserView& v) override {
        sync();
        // URL and response status under the top bar.
        Font& meta = g.font(P.mono, 17);
        const int metaY = kTopY + 44;
        int sw = Gfx::measure(meta, v.status);
        if (!toast_)  // a toast from the shell takes this spot while it shows
            g.draw(meta, Gfx::W - kM - sw, metaY, v.status, v.loading ? ink(P.caps, P) : P.muted);
        std::string url = v.url;
        const int maxUrl = std::max(10, (a.w - sw - 40) / std::max(1, meta.cellW()));
        if ((int)utf8::length(url) > maxUrl) url = "\xE2\x80\xA6" + url.substr(utf8::offset(url, utf8::length(url) - maxUrl + 1));
        g.draw(meta, kM, metaY, url, P.muted);

        Rect page{kM, kTopY + 80, a.w, kStatusY - 24 - (kTopY + 80)};
        if (v.input) {
            prompt(g, page, v);
            inputStatus(g, v);
        } else {
            pageView(g, page, v);
            pageStatus(g, v);
        }
    }

private:
    struct PageRow {
        int line;
        int link;  // link index or -1
        gem::Line::Kind kind;
        bool first;
        std::string text;
    };

    // Splits text into rows of at most `cols` cells, breaking at spaces when
    // possible. Emoji count as two cells.
    static std::vector<std::string> wrapCells(const std::string& s, int cols) {
        std::vector<std::string> out;
        if (s.empty()) return {std::string()};
        size_t start = 0;
        while (start < s.size()) {
            int cells = 0;
            size_t i = start, lastSpace = std::string::npos;
            while (i < s.size()) {
                int w = utf8::width(utf8::decode(s, i));
                if (cells + w > cols) break;
                if (s[i] == ' ') lastSpace = i;
                cells += w;
                i = utf8::next(s, i);
            }
            if (i >= s.size()) {
                out.push_back(s.substr(start));
                break;
            }
            size_t cut = (lastSpace != std::string::npos && lastSpace > start) ? lastSpace : i;
            out.push_back(s.substr(start, cut - start));
            start = cut;
            while (start < s.size() && s[start] == ' ') ++start;
        }
        return out;
    }

    static std::string clipCells(const std::string& s, int cols) {
        int cells = 0;
        size_t i = 0;
        while (i < s.size()) {
            int w = utf8::width(utf8::decode(s, i));
            if (cells + w > cols) break;
            cells += w;
            i = utf8::next(s, i);
        }
        return s.substr(0, i);
    }

    static bool external(const std::string& url) {
        size_t colon = url.find(':');
        if (colon == std::string::npos || url.find('/') < colon) return false;  // relative
        std::string scheme = url.substr(0, colon);
        return scheme != "gemini" && scheme != "about";
    }

    void pageView(Gfx& g, Rect r, const BrowserView& v) {
        Font& f = g.font(P.mono, 24);
        Font& fb = g.font(P.bold, 24);
        const int lh = f.lineH() + 6;
        const int cols = std::max(10, (r.w - 16) / f.cellW());
        const int visible = std::max(1, r.h / lh);

        // Lay out the page into rows.
        std::vector<PageRow> rows;
        int link = 0;
        for (int i = 0; v.lines && i < (int)v.lines->size(); ++i) {
            const gem::Line& l = (*v.lines)[i];
            using K = gem::Line::Kind;
            if (l.kind == K::Pre) {
                rows.push_back({i, -1, l.kind, true, clipCells(l.text, cols)});
                continue;
            }
            const int indent = (l.kind == K::Link || l.kind == K::Item || l.kind == K::Quote) ? 2 : 0;
            std::string text = l.text;
            if (l.kind == K::Link && external(l.url)) text += "  \xE2\x86\x97 " + l.url.substr(0, l.url.find(':'));
            auto parts = wrapCells(text, cols - indent);
            for (size_t k = 0; k < parts.size(); ++k)
                rows.push_back({i, l.kind == K::Link ? link : -1, l.kind, k == 0, parts[k]});
            if (l.kind == K::Link) ++link;
        }
        if (v.layout) {
            v.layout->rows = (int)rows.size();
            v.layout->visible = visible;
            v.layout->links.assign(link, {-1, -1});
            for (int k = 0; k < (int)rows.size(); ++k)
                if (rows[k].link >= 0) {
                    auto& lr = v.layout->links[rows[k].link];
                    if (lr.first < 0) lr.first = k;
                    lr.second = k;
                }
        }

        // Draw the visible rows, with a sub-row offset for smooth scrolling.
        const int top = (int)v.scroll;
        const int offset = (int)((v.scroll - top) * lh);
        SDL_Rect clip{r.x - 16, r.y, r.w + 32, r.h};
        SDL_RenderSetClipRect(g.sdl(), &clip);
        const Color linkColour = ink(buttonColor(West, P), P);
        for (int k = top; k < (int)rows.size() && k <= top + visible; ++k) {
            const PageRow& row = rows[k];
            const int y = r.y + (k - top) * lh - offset;
            const int ty = y + (lh - f.lineH()) / 2;
            int x = r.x;
            using K = gem::Line::Kind;
            const bool sel = row.link >= 0 && row.link == v.selected;
            if (sel) {
                g.fill({r.x - 12, y, r.w + 12, lh}, P.surface);
                g.fill({r.x - 12, y, 4, lh}, P.letters);
            }
            switch (row.kind) {
                case K::H1: g.printWide(fb, x, ty, row.text, ink(P.letters, P)); break;
                case K::H2: g.printWide(fb, x, ty, row.text, P.text); break;
                case K::H3: g.printWide(fb, x, ty, row.text, P.chr[0]); break;
                case K::Pre: g.printWide(f, x, ty, row.text, mix(P.text, P.muted, 0.35f)); break;
                case K::Quote:
                    g.fill({x + f.cellW() / 2 - 1, y, 3, lh}, P.raised);
                    g.printWide(f, x + 2 * f.cellW(), ty, row.text, P.muted);
                    break;
                case K::Item:
                    if (row.first) g.printWide(f, x, ty, "\xE2\x80\xA2", ink(P.letters, P));
                    g.printWide(f, x + 2 * f.cellW(), ty, row.text, P.text);
                    break;
                case K::Link:
                    if (row.first) g.printWide(f, x, ty, "\xE2\x86\x92", sel ? P.letters : P.muted);
                    g.printWide(sel ? fb : f, x + 2 * f.cellW(), ty, row.text, sel ? P.text : linkColour);
                    break;
                default: g.printWide(f, x, ty, row.text, P.text); break;
            }
        }
        SDL_RenderSetClipRect(g.sdl(), nullptr);

        // Scrollbar.
        if ((int)rows.size() > visible) {
            const int trackX = r.x + r.w + 8, h = r.h;
            g.fill({trackX, r.y, 3, h}, P.surface);
            int thumbH = std::max(24, h * visible / (int)rows.size());
            int thumbY = r.y + (int)((h - thumbH) * (v.scroll / std::max(1, (int)rows.size() - visible)));
            g.fill({trackX, thumbY, 3, thumbH}, P.muted);
        }
        if (v.loading && (!v.lines || v.lines->empty()))
            g.draw(g.font(P.mono, 24), r.x, r.y, "loading\xE2\x80\xA6", P.muted);
    }

    void pageStatus(Gfx& g, const BrowserView& v) {
        const int y = kStatusY, s = 30;
        Font& f = g.font(P.mono, 20);
        int x = kM;
        auto hint = [&](std::initializer_list<Btn> bs, const char* what) {
            for (Btn b : bs) {
                const bool held = v.held & b;
                badge(g, x, y, s, b, held ? P.text : buttonColor(b, P), held ? P.bg : P.dark, P.bold);
                x += s + 4;
            }
            x += 6 + g.draw(f, x + 6, y + (s - f.lineH()) / 2, what, P.text) + 22;
        };
        hint({DUp, DDown}, "links");
        hint({kConfirm}, "open");
        hint({kBack}, v.loading ? "stop" : "back");
        hint({West}, "address");
        hint({North}, "home");
        Font& tf = g.font(P.bold, 18);
        tag(g, tf, Gfx::W - kM - Gfx::measure(tf, "+ menu") - 21, y, s, "+ menu", P.surface, P.muted);
        g.fill({kM, kStatusY + 44, Gfx::W - 2 * kM, 4}, P.surface);
        if (v.loading) {  // a sweeping bar while waiting for the server
            const int w = (Gfx::W - 2 * kM) / 4;
            const int x0 = kM + (int)((SDL_GetTicks() % 1400) / 1400.0f * (Gfx::W - 2 * kM - w));
            g.fill({x0, kStatusY + 44, w, 4}, P.letters);
        }
    }

    void prompt(Gfx& g, Rect r, const BrowserView& v) {
        Font& pf = g.font(P.mono, 24);
        int y = r.y;
        for (const std::string& line : wrapCells(v.prompt, std::max(10, r.w / pf.cellW()))) {
            g.printWide(pf, r.x, y, line, P.text);
            y += pf.lineH() + 4;
        }
        y += 16;
        // The field, drawn like the editor's document (masked if sensitive).
        Font& f = g.font(P.mono, 34);
        const int lh = f.lineH() + 10;
        g.fill({r.x - 12, y - 6, r.w + 24, lh + 12}, P.surface);
        EditorView field = v.field;
        std::vector<std::string> masked;
        if (v.sensitive) {
            for (const auto& l : *field.lines) {
                std::string m;
                for (size_t n = utf8::length(l); n > 0; --n) m += "\xE2\x80\xA2";
                masked.push_back(m);
            }
            field.lines = &masked;
        }
        int fieldTop = 0;
        fam::document(g, {r.x, y, r.w, lh}, field, P, {&f, lh, 6}, fieldTop);
        y += lh + 28;
        Font& hf = g.font(P.mono, 18);
        std::string hint = std::string(btnLabel(ZR)) + " + " + btnLabel(South) + "  send      + menu (send, cancel)";
        g.draw(hf, r.x, y, hint, P.muted);
    }

    void inputStatus(Gfx& g, const BrowserView& v) {
        const int y = kStatusY, s = 30;
        Font& f = g.font(P.mono, 20);
        const EditorView& e = v.field;
        if (e.tap->hasPending()) {
            Btn b = pendingButton(e);
            badge(g, kM, y - 4, 38, b, buttonColor(b, P), P.dark, P.bold);
            cycle(g, kM + 38 + 12, y - 4, 38, e, P);
        } else {
            groupsLine(g, kM, y, s, f, e, P);
        }
        modeTags(g, Gfx::W - kM, y, s, g.font(P.bold, 18), e, P);
        g.fill({kM, kStatusY + 44, Gfx::W - 2 * kM, 4}, P.surface);
        if (float t = timerLeft(e); t > 0)
            g.fill({kM, kStatusY + 44, (int)((Gfx::W - 2 * kM) * t), 4}, buttonColor(pendingButton(e), P));
    }

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

    // Stars flying slowly towards the viewer from the centre of the screen.
    // Stateless: each star's depth is a function of the clock, wrapping back
    // to the far plane, so it needs no per-frame state. Kept small and dim so
    // the text stays the brightest thing on screen.
    static void starfield(Gfx& g, bool light) {
        constexpr int kStars = 260;
        constexpr float kCycle = 20.0f;  // seconds for a star to travel from far to near
        const float t = SDL_GetTicks() / 1000.0f;
        const float cx = Gfx::W / 2.0f, cy = Gfx::H / 2.0f;
        uint32_t seed = 0x9E3779B9u;
        auto rnd = [&seed] {  // deterministic xorshift, same sky every frame
            seed ^= seed << 13;
            seed ^= seed >> 17;
            seed ^= seed << 5;
            return (seed & 0xFFFFFF) / float(0x1000000);
        };
        auto project = [&](float x, float y, float z, float& sx, float& sy) {
            sx = cx + x / z * cx * 0.5f;
            sy = cy + y / z * cy * 0.5f;
        };
        for (int i = 0; i < kStars; ++i) {
            const float x = rnd() * 2 - 1, y = rnd() * 2 - 1, phase = rnd(), tint = rnd();
            const float z = 0.05f + 0.95f * (1.0f - std::fmod(phase + t / kCycle, 1.0f));  // 1 far .. 0.05 near
            float sx, sy;
            project(x, y, z, sx, sy);
            if (sx < 0 || sy < 0 || sx >= Gfx::W || sy >= Gfx::H) continue;
            const float nearness = (1.0f - z) / 0.95f;  // 0 far .. 1 near
            const int size = nearness > 0.6f ? 3 : nearness > 0.3f ? 2 : 1;
            const uint8_t a = (uint8_t)(90 + 140 * nearness);
            Color c = tint < 0.7f ? Color{225, 232, 255, a} : tint < 0.85f ? Color{170, 215, 255, a}
                                                                           : Color{255, 230, 200, a};
            if (light) c = {60, 64, 80, (uint8_t)(a * 0.7f)};  // dark specks on pale themes
            // Near stars leave a short fading trail back towards the centre.
            if (nearness > 0.45f) {
                for (int k = 1; k <= 3; ++k) {
                    float tx, ty;
                    project(x, y, z + 0.012f * k, tx, ty);
                    g.fill({(int)tx, (int)ty, size - 1 > 0 ? size - 1 : 1, size - 1 > 0 ? size - 1 : 1},
                           withAlpha(c, (uint8_t)(a / (k + 1))));
                }
            }
            g.fill({(int)sx, (int)sy, size, size}, c);
        }
    }

    // The current theme's colours with the current font filled in.
    void sync() {
        P = kThemes[g_theme].palette;
        P.mono = kFonts[g_font].regular;
        P.bold = kFonts[g_font].bold;
    }

    Palette P = themes::sunset;
    int top_ = 0;         // first visible document row
    bool toast_ = false;  // a shell toast is showing in the top-right corner
};

int wrap(int i, int n) { return ((i % n) + n) % n; }

void save() {
    if (g_settingsPath.empty()) return;
    if (FILE* f = std::fopen(g_settingsPath.c_str(), "w")) {
        std::fprintf(f, "theme=%s\nfont=%s\nstarfield=%s\n", kThemes[g_theme].name, kFonts[g_font].id,
                     g_starfield ? "on" : "off");
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
    static Layout s;
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
bool starfield() { return g_starfield; }
void setStarfield(bool on) { g_starfield = on; save(); }

void loadSettings(const std::string& path) {
    g_settingsPath = path;
    FILE* f = std::fopen(path.c_str(), "r");
    if (!f) return;  // first run: defaults
    char line[128];
    while (std::fgets(line, sizeof line, f)) {
        line[std::strcspn(line, "\r\n")] = 0;
        if (std::strncmp(line, "theme=", 6) == 0) {
            if (std::strcmp(line + 6, "starfield") == 0) {  // older: the starfield was a theme
                g_theme = findTheme("space");
                g_starfield = true;
            } else if (int i = findTheme(line + 6); i >= 0) {
                g_theme = i;
            }
        } else if (std::strncmp(line, "starfield=", 10) == 0) {
            g_starfield = std::strcmp(line + 10, "on") == 0;
        } else if (std::strncmp(line, "font=", 5) == 0) {
            if (int i = findFont(line + 5); i >= 0) g_font = i;
        }
    }
    std::fclose(f);
}
}  // namespace skins
