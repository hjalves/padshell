#include "ime.h"

#include <cctype>
#include <cmath>
#include <cstring>

const std::array<DaisyWheel::Layer, 3>& DaisyWheel::layers() {
    // 8 petals x 4 characters, petals clockwise from the top, each petal in
    // North/East/South/West order. Frequent letters sit on early petals.
    static const std::array<Layer, 3> kLayers{{
        {"abc", "abcdefghijklmnopqrstuvwxyz.,?!'-"},
        {"ABC", "ABCDEFGHIJKLMNOPQRSTUVWXYZ.,?!'-"},
        {"123", "1234567890()[]{}<>/:;\"@#%&*+=_~|"},
    }};
    static_assert(sizeof("abcdefghijklmnopqrstuvwxyz.,?!'-") - 1 == kPetals * kPerPetal);
    static_assert(sizeof("1234567890()[]{}<>/:;\"@#%&*+=_~|") - 1 == kPetals * kPerPetal);
    return kLayers;
}

int DaisyWheel::petalFor(float x, float y, float deadzone) {
    if (x * x + y * y < deadzone * deadzone) return -1;
    // atan2(x, -y): 0 at "up", increasing clockwise (SDL y points down).
    float a = std::atan2(x, -y);
    if (a < 0) a += 2 * (float)M_PI;
    int p = (int)std::lround(a / ((float)M_PI / 4));
    return p % kPetals;
}

std::string DaisyWheel::charAt(int petal, int slot) const {
    char c = layers()[layer_].chars[petal * kPerPetal + slot];
    if (shift_) c = (char)std::toupper((unsigned char)c);
    return std::string(1, c);
}

void DaisyWheel::update(const Gamepad& pad, std::vector<TextEvent>& out) {
    petal_ = petalFor(pad.lx, pad.ly);
    shift_ = pad.down(ZR);

    const int nLayers = (int)layers().size();
    if (pad.hit(R)) layer_ = (layer_ + 1) % nLayers;
    if (pad.hit(L)) layer_ = (layer_ + nLayers - 1) % nLayers;

    static const Btn face[kPerPetal] = {North, East, South, West};
    if (petal_ >= 0) {
        for (int slot = 0; slot < kPerPetal; ++slot)
            if (pad.hit(face[slot])) out.push_back({TextEvent::Insert, charAt(petal_, slot)});
    } else {
        if (pad.rep(South)) out.push_back({TextEvent::Insert, " "});
        if (pad.rep(East)) out.push_back({TextEvent::Backspace, {}});
        if (pad.hit(North)) out.push_back({TextEvent::Newline, {}});
        if (pad.rep(West)) out.push_back({TextEvent::Delete, {}});
    }

    bool zl = pad.down(ZL);
    if (pad.rep(DLeft)) out.push_back({zl ? TextEvent::Home : TextEvent::Left, {}});
    if (pad.rep(DRight)) out.push_back({zl ? TextEvent::End : TextEvent::Right, {}});
    if (pad.rep(DUp)) out.push_back({zl ? TextEvent::PageUp : TextEvent::Up, {}});
    if (pad.rep(DDown)) out.push_back({zl ? TextEvent::PageDown : TextEvent::Down, {}});
}
