// Ameno: an adaptation of the "ameno" TV web app's editor. A slim top bar
// (brand, counts, a small controller diagram, clock), a large document with
// characters coloured by kind, and a status line of flat button badges with
// their groups, the pending character's cycle, mode tags and a commit timer.
#include <tuple>

#include "family.h"

namespace {
using namespace fam;

const Palette& P = darkPalette();
const char* kMono = "DejaVuSansMono.ttf";
const char* kBold = "DejaVuSansMono-Bold.ttf";
constexpr int kM = 64;        // side margin
constexpr int kStatusY = 640;  // badges row; timer below it, just inside the TV-safe bottom margin

class Ameno : public Skin {
public:
    const char* name() const override { return "ameno"; }

    Rect frame(Gfx& g, const ShellView& v) override {
        g.clear(P.bg);
        Font& brand = g.font(kBold, 30);
        int x = kM + g.draw(brand, kM, 20, "padshell", P.letters);
        g.draw(g.font(kMono, 30), x + 16, 20, v.heading, P.muted);
        Font& clock = g.font(kMono, 30);
        g.draw(clock, Gfx::W - kM - Gfx::measure(clock, v.clock), 20, v.clock, P.text);
        if (!v.toast.empty()) {
            Font& t = g.font(kMono, 15);
            g.draw(t, Gfx::W - kM - Gfx::measure(t, v.toast), 64, v.toast, P.caps);
        }
        diagram(g, Gfx::W - 227 - 187, 10, v.pad ? v.pad->held : 0);
        return {kM, 64, Gfx::W - 2 * kM, Gfx::H - 64};
    }

    void editor(Gfx& g, Rect a, const EditorView& v) override {
        Font& meta = g.font(kMono, 15);
        g.draw(meta, kM, 64, std::to_string(v.lineCount()) + (v.lineCount() == 1 ? " line" : " lines") +
                                 " \xC2\xB7 " + std::to_string(v.chars()) + " chars" + (v.modified ? " \xC2\xB7 unsaved" : ""),
               P.muted);

        Font& f = g.font(kMono, 38);
        document(g, {kM, 117, a.w, 486}, v, P, {&f, 54, 6});

        status(g, v);
    }

    void menu(Gfx& g, const std::string& title, const std::vector<std::string>& items, int sel) override {
        g.fill({0, 100, Gfx::W, kStatusY - 116}, P.bg);
        g.draw(g.font(kMono, 21), kM, 127, title, P.muted);
        Font& f = g.font(kMono, 38), &fb = g.font(kBold, 38);
        int y = 180;
        for (int i = 0; i < (int)items.size(); ++i) {
            bool on = i == sel;
            if (on) g.fill({kM, y, 600, 64}, P.letters);
            g.draw(on ? fb : f, kM + 21, y + (64 - f.lineH()) / 2, items[i], on ? P.dark : Color{169, 163, 191});
            y += 72;
        }
        // Status line becomes the menu's hints.
        g.fill({0, kStatusY - 16, Gfx::W, Gfx::H - kStatusY + 16}, P.bg);
        Font& sf = g.font(kMono, 20);
        int x = kM, y0 = kStatusY, s = 30;
        auto hint = [&](std::initializer_list<Btn> bs, const char* what) {
            for (Btn b : bs) { badge(g, x, y0, s, b, buttonColor(b), P.dark); x += s + 4; }
            x += 4 + g.draw(sf, x + 4, y0 + (s - sf.lineH()) / 2, what, P.text) + 24;
        };
        hint({DUp, DDown}, "navigate");
        hint({kConfirm}, "select");
        hint({kBack}, "cancel");
        tag(g, sf, Gfx::W - kM - 70, y0, s, "menu", P.text, P.dark);
    }

private:
    void status(Gfx& g, const EditorView& v) {
        const int y = kStatusY, s = 30;
        Font& f = g.font(kMono, 20);
        if (v.tap->hasPending()) {
            Btn b = pendingButton(v);
            badge(g, kM, y - 4, 38, b, buttonColor(b), P.dark);
            cycle(g, kM + 38 + 12, y - 4, 38, v, P);
        } else {
            groupsLine(g, kM, y, s, f, v, P);
        }
        modeTags(g, Gfx::W - kM, y, s, g.font(kBold, 18), v, P);

        // Commit timer.
        g.fill({kM, kStatusY + 44, Gfx::W - 2 * kM, 4}, P.surface);
        if (float t = timerLeft(v); t > 0)
            g.fill({kM, kStatusY + 44, (int)((Gfx::W - 2 * kM) * t), 4}, buttonColor(pendingButton(v)));
    }

    // A small controller diagram; held buttons light up in their colour.
    void diagram(Gfx& g, int x, int y, uint32_t held) {
        auto box = [&](Btn b, int bx, int by, int w, int h, Color on) {
            g.fill({x + bx, y + by, w, h}, (held & b) ? on : P.raised);
        };
        Font& lf = g.font(kBold, 9);
        auto label = [&](Btn b, int bx, int by, int w, int h, Color c) {
            g.drawCentered(lf, x + bx + w / 2, y + by + (h - lf.lineH()) / 2, btnLabel(b), (held & b) ? P.dark : c);
        };
        box(ZL, 9, 0, 37, 12, buttonColor(South)); label(ZL, 9, 0, 37, 12, Color{138, 132, 163});
        box(ZR, 140, 0, 37, 12, buttonColor(East)); label(ZR, 140, 0, 37, 12, Color{138, 132, 163});
        box(L, 0, 16, 56, 9, P.caps);
        box(R, 131, 16, 56, 9, P.text);
        for (auto [b, bx, by] : {std::tuple{DUp, 28, 32}, {DDown, 28, 63}, {DLeft, 13, 47}, {DRight, 43, 47}})
            box(b, bx, by, 15, 15, buttonColor(b));
        g.fill({x + 28, y + 47, 15, 15}, P.raised);
        box(Minus, 75, 51, 15, 8, P.letters);
        box(Plus, 97, 51, 15, 8, P.letters);
        for (auto [b, bx, by] : {std::tuple{North, 151, 39}, {West, 134, 55}, {East, 168, 55}, {South, 151, 71}}) {
            Color c = buttonColor(b);
            g.fillCircle(x + bx + 8, y + by + 3, 8, (held & b) ? c : P.raised);
            g.drawCentered(lf, x + bx + 8, y + by + 3 - lf.lineH() / 2, btnLabel(b), (held & b) ? P.dark : c);
        }
    }

};
}  // namespace

Skin* makeAmeno() { static Ameno s; return &s; }
