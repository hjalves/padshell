// Multi-tap text entry (phone-keypad style) for a gamepad. See README for the
// full behaviour; in short:
//
//   * Each face button / D-pad direction holds a group of characters. Press it
//     to preview the first one, press again within a second to cycle.
//   * The pending character is committed after 1 s without input, or when a
//     character from another group is selected.
//   * Hold ZL (left trigger) for numbers and symbols; hold ZR (right trigger)
//     for newline / space / backspace / delete-line on the face buttons and
//     cursor movement on the D-pad (auto-repeats). ZR wins over ZL.
//   * L (left shoulder) toggles uppercase for the next committed character.
//
// The logic is pure (time comes from Gamepad::ms) so it can be unit tested.
#pragma once
#include <array>
#include <string>
#include <vector>

#include "pad.h"

struct TextEvent {
    enum Kind { Insert, Newline, Backspace, DeleteLine, Left, Right, Up, Down };
    Kind kind;
    std::string text;  // for Insert
};

class MultiTap {
public:
    static constexpr uint32_t kCommitMs = 1000;

    enum class Mode { Letters, Symbols, Commands };
    struct Group { Btn button; const char* chars; };
    using Groups = std::array<Group, 8>;
    struct Command { Btn button; TextEvent::Kind kind; const char* text; const char* label; };
    using Commands = std::array<Command, 4>;

    static const Groups& letters();
    static const Groups& symbols();
    static const Commands& commands();

    void update(const Gamepad& pad, std::vector<TextEvent>& out);
    // Commits the pending character right away (e.g. before saving).
    void flush(std::vector<TextEvent>& out) { commit(out, false); }

    Mode mode() const { return mode_; }
    bool uppercase() const { return upper_; }
    // When the pending character's one-second commit timer last restarted.
    uint32_t lastInput() const { return lastInput_; }
    bool hasPending() const { return pending_ != 0; }
    // The pending character as it would be committed (case applied).
    std::string pendingText() const;
    // Group the pending character is cycling through, and its position in it.
    const char* pendingGroup() const { return group_; }
    int pendingIndex() const;
    std::string display(char c) const;  // c with the current case applied

private:
    void press(const Group& g, std::vector<TextEvent>& out, bool shoulderHeld);
    void commit(std::vector<TextEvent>& out, bool shoulderHeld);

    Mode mode_ = Mode::Letters;
    char pending_ = 0;            // base (lowercase) character, 0 = none
    const char* group_ = nullptr;
    uint32_t lastInput_ = 0;
    bool upper_ = false;
};
