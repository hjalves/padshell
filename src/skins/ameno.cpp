// Ameno: an adaptation of the "ameno" TV web app's editor. A slim top bar
// (brand, counts, clock), a large document with characters coloured by kind,
// and a status line of flat button badges with their groups, the pending
// character's cycle, mode tags and a commit timer.
//
// The layout is shared by several themes (palette + monospaced font pair),
// each registered as its own style: ameno, ameno-ocean, ameno-carbon, ...
#include "family.h"

namespace {
using namespace fam;

constexpr int kM = 64;         // side margin
constexpr int kTopY = 42;      // top bar text row; mirrors the badges row's distance from the bottom
constexpr int kStatusY = 640;  // badges row; timer below it, just inside the TV-safe bottom margin

class Ameno : public Skin {
public:
    Ameno(const char* name, const Palette& p) : name_(name), P(p) {}
    const char* name() const override { return name_; }

    Rect frame(Gfx& g, const ShellView& v) override {
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
        Font& meta = g.font(P.mono, 17);
        g.draw(meta, kM, kTopY + 44, std::to_string(v.lineCount()) + (v.lineCount() == 1 ? " line" : " lines") +
                                         " \xC2\xB7 " + std::to_string(v.chars()) + " chars" +
                                         (v.modified ? " \xC2\xB7 unsaved" : ""),
               P.muted);

        Font& f = g.font(P.mono, 38);
        document(g, {kM, kTopY + 97, a.w, 8 * 54}, v, P, {&f, 54, 6});

        status(g, v);
    }

    void menu(Gfx& g, const std::string& title, const std::vector<std::string>& items, int sel) override {
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

    const char* name_;
    const Palette& P;
};
}  // namespace

Skin* makeAmeno() { static Ameno s("ameno", themes::midnight); return &s; }
Skin* makeAmenoOcean() { static Ameno s("ameno-ocean", themes::ocean); return &s; }
Skin* makeAmenoCarbon() { static Ameno s("ameno-carbon", themes::carbon); return &s; }
Skin* makeAmenoNeon() { static Ameno s("ameno-neon", themes::neon); return &s; }
Skin* makeAmenoEmber() { static Ameno s("ameno-ember", themes::ember); return &s; }
Skin* makeAmenoDay() { static Ameno s("ameno-day", themes::day); return &s; }
