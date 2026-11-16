// XMB: inspired by the XrossMediaBar. A colour wash that changes with the
// month, translucent ribbons waving across it, a horizontal row of category
// icons (the button groups) that glides so the active one sits in a fixed
// column, and that group's characters as a vertical list beneath it, with
// the earlier items scrolled up above the row. The document sits to the right
// like an item's info panel.
#include <algorithm>
#include <cmath>
#include <cstring>
#include <ctime>
#include <vector>

#include "../skin.h"

namespace {
using namespace skins;

constexpr Color white{255, 255, 255};
constexpr int kRowY = 196, kColX = 300, kSpacing = 150, kItemH = 52;
const char* kSans = "DejaVuSans.ttf";

// Rough PS3-style monthly theme colours.
Color monthColor() {
    static const Color months[12] = {{150, 150, 165}, {190, 160, 40}, {110, 170, 40}, {220, 110, 160},
                                     {40, 150, 70},   {140, 80, 180}, {30, 170, 180}, {40, 120, 210},
                                     {100, 60, 190},  {210, 140, 40}, {150, 90, 50},  {190, 40, 40}};
    std::time_t now = std::time(nullptr);
    return months[std::localtime(&now)->tm_mon];
}

Color scale(Color c, float k) {
    auto s = [k](uint8_t v) { return (uint8_t)std::min(255.f, v * k); };
    return {s(c.r), s(c.g), s(c.b), 255};
}

const char* commandIcon(TextEvent::Kind k) {
    switch (k) {
        case TextEvent::Newline: return "\xE2\x86\xB5";     // ↵
        case TextEvent::Backspace: return "\xE2\x8C\xAB";   // ⌫
        case TextEvent::DeleteLine: return "\xE2\x9C\x95";  // ✕
        default: return "\xE2\x90\xA3";                     // ␣
    }
}

class Xmb : public Skin {
public:
    const char* name() const override { return "xmb"; }

    Rect frame(Gfx& g, const ShellView& v) override {
        const float t = SDL_GetTicks() / 1000.0f;
        const Color base = monthColor();
        g.gradient({0, 0, Gfx::W, Gfx::H / 2}, scale(base, 0.22f), scale(base, 0.32f), scale(base, 0.55f),
                   scale(base, 0.65f));
        g.gradient({0, Gfx::H / 2, Gfx::W, Gfx::H / 2}, scale(base, 0.55f), scale(base, 0.65f), scale(base, 0.3f),
                   scale(base, 0.38f));
        ribbon(g, t, 430, 70, 0.0042f, 0.35f, 46, 46);
        ribbon(g, t + 3.1f, 450, 55, 0.0051f, -0.27f, 30, 38);
        ribbon(g, t + 7.7f, 440, 62, 0.0047f, 0.22f, 3, 120);

        Font& f = g.font(kSans, 22);
        char date[16];
        std::time_t now = std::time(nullptr);
        std::strftime(date, sizeof date, "%d/%m", std::localtime(&now));
        std::string clock = std::string(date) + "   " + v.clock;
        int cw = Gfx::measure(f, clock);
        Rect pill{Gfx::W - 70 - cw - 36, 34, cw + 36, 38};
        g.fillRounded(pill, 19, {0, 0, 0, 50});
        g.draw(f, pill.x + 18, pill.y + 7, clock, white);
        if (!v.toast.empty()) g.draw(f, Gfx::W - 70 - Gfx::measure(f, v.toast), 84, v.toast, white);
        heading_ = v.heading;
        return {0, 0, Gfx::W, Gfx::H};
    }

    void editor(Gfx& g, Rect, const EditorView& v) override {
        const MultiTap& tap = *v.tap;
        const bool commands = v.mode() == MultiTap::Mode::Commands;

        // Row contents for the current mode, in physical left-to-right order.
        std::vector<Btn> row = commands ? std::vector<Btn>{West, North, South, East}
                                        : std::vector<Btn>{DLeft, DUp, DDown, DRight, West, North, South, East};
        // Selection: the pending group, else a held button, else the last one.
        for (size_t i = 0; i < row.size(); ++i) {
            const auto* grp = groupFor(v.mode(), row[i]);
            if ((grp && tap.hasPending() && tap.pendingGroup() == grp->chars) || (v.held & row[i])) sel_ = (int)i;
        }
        sel_ = std::clamp(sel_, 0, (int)row.size() - 1);
        const int cur = tap.hasPending() ? tap.pendingIndex() : 0;
        animate((float)sel_, (float)cur);

        // Category icons.
        for (size_t i = 0; i < row.size(); ++i) {
            float x = kColX + (i - scroll_) * kSpacing;
            if (x < -80 || x > Gfx::W + 80) continue;
            const bool on = (int)i == sel_;
            const Btn b = row[i];
            const uint8_t a = on ? 255 : 120;
            if (on) g.radial(x, kRowY, 70, {255, 255, 255, 60}, {255, 255, 255, 0});
            if (commands) {
                g.drawCentered(g.font(kSans, on ? 52 : 40), (int)x, kRowY - (on ? 34 : 26),
                               commandIcon(commandFor(b)->kind), withAlpha(white, a));
            } else if (isDpad(b)) {
                g.ring(x, kRowY, on ? 34 : 27, 3, withAlpha(white, a));
                g.arrow(x, kRowY, on ? 26 : 20, dirOf(b), withAlpha(white, a));
            } else {
                g.ring(x, kRowY, on ? 34 : 27, 3, withAlpha(white, a));
                Font& lf = g.font(kSans, on ? 32 : 25);
                g.drawCentered(lf, (int)x, kRowY - lf.lineH() / 2, btnLabel(b), withAlpha(white, a));
            }
            if (on) {
                std::string label = commands ? commandFor(b)->label
                                             : groupText(tap, groupFor(v.mode(), b)->chars, " ");
                g.drawCentered(g.font(kSans, 20), (int)x, kRowY + 46, label, white);
            }
        }

        // Vertical list of the selected group's characters.
        if (!commands) {
            const auto* grp = groupFor(v.mode(), row[sel_]);
            const int n = (int)std::strlen(grp->chars);
            const bool live = tap.hasPending() && tap.pendingGroup() == grp->chars;
            Font& big = g.font(kSans, 38);
            Font& small = g.font(kSans, 18);
            for (int k = 0; k < n; ++k) {
                float rel = k - list_;
                float y = rel >= -0.5f ? kRowY + 100 + rel * kItemH : kRowY - 96 + (rel + 1) * kItemH;
                if (y < 70 || y > Gfx::H - 70) continue;
                const bool on = live && k == cur;
                const uint8_t a = on ? 255 : live ? (rel < 0 ? 90 : 160) : 110;
                if (on) g.fillRounded({kColX - 70, (int)y - 6, 230, kItemH}, 10, {255, 255, 255, 40});
                std::string ch = tap.display(grp->chars[k]);
                g.drawCentered(big, kColX - 20, (int)y - 2, ch, withAlpha(white, a));
                g.draw(small, kColX + 24, (int)y + 14, "press " + std::to_string(k + 1) + "\xC3\x97",
                       withAlpha(white, (uint8_t)(a * 0.7f)));
            }
        }

        // Document as the info panel on the right.
        Rect doc{560, 340, Gfx::W - 560 - 70, 270};  // below the icon row, right of the list
        g.draw(g.font(kSans, 26), doc.x, doc.y - 46, heading_, white);
        g.fill({doc.x, doc.y - 8, doc.w, 1}, {255, 255, 255, 90});
        Font& f = g.font("DejaVuSansMono.ttf", 28);
        const int cols = std::max(1, doc.w / f.cellW()), visible = std::max(1, doc.h / f.lineH());
        const auto rows = v.wrap(cols);
        const int top = std::max(0, (int)rows.size() - visible);
        for (int r = top; r < (int)rows.size() && r < top + visible; ++r) {
            int y = doc.y + (r - top) * f.lineH();
            g.print(f, doc.x + 2, y + 2, v.rowText(rows[r]), {0, 0, 0, 70});
            g.print(f, doc.x, y, v.rowText(rows[r]), white);
        }
        int cx = doc.x + (v.endColumn() % cols) * f.cellW();
        int cy = doc.y + ((int)rows.size() - 1 - top) * f.lineH();
        Color mc = modeColor(v.mode());
        if (tap.hasPending()) {
            g.fillRounded({cx - 2, cy, f.cellW() + 4, f.lineH()}, 6, mc);
            g.print(f, cx, cy, tap.pendingText(), {30, 20, 40});
        }
        g.fill({cx, cy + f.lineH() - 4, f.cellW(), 4}, mc);

        // Hints along the bottom, PS3-style.
        Font& h = g.font(kSans, 19);
        int x = 70;
        auto badge = [&](const std::string& key, const std::string& what, bool on) {
            int kw = Gfx::measure(h, key) + 18;
            g.fillRounded({x, Gfx::H - 66, kw, 28}, 14, on ? withAlpha(mc, 230) : Color{255, 255, 255, 50});
            g.draw(h, x + 9, Gfx::H - 63, key, white);
            x += kw + 8;
            x += g.draw(h, x, Gfx::H - 63, what, withAlpha(white, 210)) + 26;
        };
        badge(btnLabel(ZL), "symbols", v.mode() == MultiTap::Mode::Symbols);
        badge(btnLabel(ZR), "edit", commands);
        badge(btnLabel(L), "caps", tap.uppercase());
        badge("+", "menu", false);
        badge("-", "apps", false);
        std::string info = std::to_string(v.chars()) + " characters" + (v.modified ? "  \xC2\xB7  unsaved" : "");
        g.draw(h, Gfx::W - 70 - Gfx::measure(h, info), Gfx::H - 63, info, withAlpha(white, 200));
    }

    // The options panel slides in from the right.
    void menu(Gfx& g, const std::string& title, const std::vector<std::string>& items, int sel) override {
        const int w = 420, x = Gfx::W - w;
        g.gradient({x - 120, 0, 120, Gfx::H}, {0, 0, 0, 0}, {0, 0, 0, 170}, {0, 0, 0, 0}, {0, 0, 0, 170});
        g.fill({x, 0, w, Gfx::H}, {0, 0, 0, 215});
        g.fill({x, 0, 1, Gfx::H}, {255, 255, 255, 60});
        Font& f = g.font(kSans, 28);
        int y = Gfx::H / 2 - (int)items.size() * (f.lineH() + 18) / 2;
        g.draw(g.font(kSans, 18), x + 44, y - 50, title, withAlpha(white, 170));
        for (int i = 0; i < (int)items.size(); ++i) {
            if (i == sel) {
                g.radial(x + w / 2.f, y + f.lineH() / 2.f, w * 0.6f, {255, 255, 255, 50}, {255, 255, 255, 0});
                g.fill({x + 20, y - 6, w - 40, 1}, {255, 255, 255, 120});
                g.fill({x + 20, y + f.lineH() + 6, w - 40, 1}, {255, 255, 255, 120});
            }
            g.draw(f, x + 44, y, items[i], withAlpha(white, i == sel ? 255 : 150));
            y += f.lineH() + 18;
        }
        g.draw(g.font(kSans, 18), x + 44, Gfx::H - 66,
               std::string(btnLabel(kConfirm)) + " select     " + btnLabel(kBack) + " back", withAlpha(white, 170));
    }

private:
    // Glide towards the targets; frame-rate independent.
    void animate(float scrollTarget, float listTarget) {
        uint32_t now = SDL_GetTicks();
        float dt = last_ ? std::min(0.1f, (now - last_) / 1000.0f) : 1.0f;
        last_ = now;
        float k = 1.0f - std::exp(-dt * 14.0f);
        scroll_ += (scrollTarget - scroll_) * k;
        list_ += (listTarget - list_) * k;
    }

    // A soft translucent ribbon: brightest along its centre line.
    void ribbon(Gfx& g, float t, float baseY, float amp, float freq, float speed, float thick, uint8_t alpha) {
        std::vector<SDL_Vertex> v;
        const SDL_Color clear{255, 255, 255, 0}, mid{255, 255, 255, alpha};
        SDL_FPoint pt[3], prev[3];
        for (int x = 0; x <= Gfx::W; x += 16) {
            float fx = (float)x;
            float y = baseY + amp * std::sin(fx * freq + t * speed) + amp * 0.4f * std::sin(fx * freq * 2.3f - t * speed * 0.7f);
            float h = thick * (0.6f + 0.4f * std::sin(fx * 0.004f + t * 0.3f));
            pt[0] = {fx, y - h};
            pt[1] = {fx, y};
            pt[2] = {fx, y + h};
            if (x > 0) {
                const SDL_Color c[3] = {clear, mid, clear};
                for (int s = 0; s < 2; ++s) {  // upper and lower halves
                    SDL_Vertex a{prev[s], c[s], {0, 0}}, b{prev[s + 1], c[s + 1], {0, 0}};
                    SDL_Vertex d{pt[s], c[s], {0, 0}}, e{pt[s + 1], c[s + 1], {0, 0}};
                    v.insert(v.end(), {a, b, d, d, b, e});
                }
            }
            std::copy(pt, pt + 3, prev);
        }
        SDL_RenderGeometry(g.sdl(), nullptr, v.data(), (int)v.size(), nullptr, 0);
    }

    std::string heading_;
    int sel_ = 0;
    float scroll_ = 0, list_ = 0;
    uint32_t last_ = 0;
};
}  // namespace

Skin* makeXmb() { static Xmb s; return &s; }
