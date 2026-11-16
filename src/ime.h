// Gamepad input methods. An InputMethod turns Gamepad state into TextEvents; any
// app that needs text (editor, URL bar, city search...) owns one and applies
// the events to whatever it is editing.
#pragma once
#include <array>
#include <string>
#include <vector>

#include "pad.h"

class Gfx;
struct Rect;

struct TextEvent {
    enum Kind { Insert, Backspace, Delete, Newline, Left, Right, Up, Down, Home, End, PageUp, PageDown };
    Kind kind;
    std::string text;  // for Insert
};

class InputMethod {
public:
    virtual ~InputMethod() = default;
    virtual const char* name() const = 0;
    virtual void update(const Gamepad& pad, std::vector<TextEvent>& out) = 0;
    virtual void render(Gfx& g, Rect area) const = 0;
};

// Daisywheel-style input:
//   * Tilt the left stick toward one of 8 petals; each petal holds 4 characters
//     laid out like the face buttons. Press the face button in the same
//     position to type that character.
//   * With the stick centred the face buttons are editing keys:
//     South = space, East = backspace, North = newline, West = delete.
//   * L / R cycle layers (abc / ABC / 123). Hold ZR for one-shot uppercase.
//   * D-pad moves the cursor; hold ZL for Home / End / PageUp / PageDown.
class DaisyWheel : public InputMethod {
public:
    static constexpr int kPetals = 8;
    static constexpr int kPerPetal = 4;  // order: North, East, South, West
    struct Layer { const char* name; const char* chars; };  // 32 chars

    const char* name() const override { return "Daisywheel"; }
    void update(const Gamepad& pad, std::vector<TextEvent>& out) override;
    void render(Gfx& g, Rect area) const override;

    // Petal the stick points at (0 = up, clockwise), or -1 when centred.
    static int petalFor(float x, float y, float deadzone = 0.5f);

    int layer() const { return layer_; }
    int petal() const { return petal_; }
    static const std::array<Layer, 3>& layers();
    std::string charAt(int petal, int slot) const;

private:
    int layer_ = 0;
    int petal_ = -1;
    bool shift_ = false;
};
