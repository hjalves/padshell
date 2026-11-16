// Logical gamepad state, independent of SDL and of the physical controller.
//
// Face buttons are named by *position* (North/East/South/West) rather than by
// label, because Nintendo and Xbox layouts put A/B/X/Y in different places and
// a typing scheme cares about where your thumb goes, not what is printed.
#pragma once
#include <cstdint>

enum Btn : uint32_t {
    North  = 1u << 0,   // Switch: X   Xbox: Y
    East   = 1u << 1,   // Switch: A   Xbox: B
    South  = 1u << 2,   // Switch: B   Xbox: A
    West   = 1u << 3,   // Switch: Y   Xbox: X
    L      = 1u << 4,
    R      = 1u << 5,
    ZL     = 1u << 6,
    ZR     = 1u << 7,
    Plus   = 1u << 8,
    Minus  = 1u << 9,
    DUp    = 1u << 10,
    DDown  = 1u << 11,
    DLeft  = 1u << 12,
    DRight = 1u << 13,
    LStick = 1u << 14,
    RStick = 1u << 15,
};

struct Gamepad {
    uint32_t held = 0;      // currently down
    uint32_t pressed = 0;   // went down this frame
    uint32_t released = 0;  // went up this frame
    uint32_t repeat = 0;    // pressed this frame, or auto-repeating while held
    float lx = 0, ly = 0;   // left stick, -1..1, +y is down (SDL convention)
    float rx = 0, ry = 0;
    uint32_t ms = 0;        // timestamp of this sample

    bool down(Btn b) const { return held & b; }
    bool hit(Btn b) const { return pressed & b; }
    bool rep(Btn b) const { return repeat & b; }
};

// Menus confirm with the A button and cancel with B, wherever the platform
// prints them: A is the right button on Switch, the bottom one on Xbox pads.
#ifdef __SWITCH__
constexpr Btn kConfirm = East, kBack = South;
#else
constexpr Btn kConfirm = South, kBack = East;
#endif

// The label printed on the button in each position, for on-screen hints.
inline const char* btnLabel(Btn b) {
    switch (b) {
#ifdef __SWITCH__
        case North: return "X";
        case East: return "A";
        case South: return "B";
        case West: return "Y";
        case L: return "L";
        case R: return "R";
        case ZL: return "ZL";
        case ZR: return "ZR";
#else
        case North: return "Y";
        case East: return "B";
        case South: return "A";
        case West: return "X";
        case L: return "LB";
        case R: return "RB";
        case ZL: return "LT";
        case ZR: return "RT";
#endif
        case Plus: return "+";
        case Minus: return "-";
        case DUp: return "\u2191";
        case DDown: return "\u2193";
        case DLeft: return "\u2190";
        case DRight: return "\u2192";
        case LStick: return "LS";
        case RStick: return "RS";
    }
    return "?";
}

// Turns raw "which buttons are held" samples into a Gamepad with edge
// detection and key-repeat. Pure logic, so it can be unit tested.
class PadTracker {
public:
    uint32_t repeatDelayMs = 350;
    uint32_t repeatRateMs = 45;

    Gamepad update(uint32_t held, float lx, float ly, float rx, float ry, uint32_t nowMs) {
        Gamepad s;
        s.held = held;
        s.pressed = held & ~prev_;
        s.released = prev_ & ~held;
        s.lx = lx; s.ly = ly; s.rx = rx; s.ry = ry;
        s.ms = nowMs;
        s.repeat = s.pressed;
        for (int i = 0; i < 32; ++i) {
            uint32_t bit = 1u << i;
            if (s.pressed & bit) {
                next_[i] = nowMs + repeatDelayMs;
            } else if ((held & bit) && nowMs >= next_[i]) {
                s.repeat |= bit;
                next_[i] = nowMs + repeatRateMs;
            }
        }
        prev_ = held;
        return s;
    }

private:
    uint32_t prev_ = 0;
    uint32_t next_[32] = {};
};
