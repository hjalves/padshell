#include "multitap.h"

#include <cctype>
#include <cstring>
#include <utility>

// Buttons are named by position. In Xbox terms South = A, West = X, North = Y,
// East = B; on a Switch controller South = B, West = Y, North = X, East = A.
const MultiTap::Groups& MultiTap::letters() {
    static const Groups g{{
        {South, "abc"}, {West, "def"}, {North, "ghi"}, {East, "jkl"},
        {DDown, "mno"}, {DLeft, "pqrs"}, {DUp, "tuv"}, {DRight, "wxyz"},
    }};
    return g;
}

const MultiTap::Groups& MultiTap::symbols() {
    static const Groups g{{
        {South, "123"}, {West, "456"}, {North, "789"}, {East, "0."},
        {DLeft, "([{<\"'"}, {DDown, ".,?!_:;|"}, {DRight, ")]}>\"'"}, {DUp, "-=+*/^~#%@"},
    }};
    return g;
}

const MultiTap::Commands& MultiTap::commands() {
    static const Commands c{{
        {South, TextEvent::Newline, "", "newline"},
        {West, TextEvent::Insert, " ", "space"},
        {East, TextEvent::Backspace, "", "backspace"},
        {North, TextEvent::DeleteLine, "", "delete line"},
    }};
    return c;
}

std::string MultiTap::display(char c) const {
    return std::string(1, upper_ ? (char)std::toupper((unsigned char)c) : c);
}

std::string MultiTap::pendingText() const { return pending_ ? display(pending_) : std::string(); }

int MultiTap::pendingIndex() const {
    if (!pending_ || !group_) return -1;
    return (int)(std::strchr(group_, pending_) - group_);
}

void MultiTap::commit(std::vector<TextEvent>& out, bool shoulderHeld) {
    if (!pending_) return;
    out.push_back({TextEvent::Insert, display(pending_)});
    pending_ = 0;
    group_ = nullptr;
    // Uppercase is one-shot, unless the shoulder is held through the commit.
    if (upper_ && !shoulderHeld) upper_ = false;
}

void MultiTap::press(const Group& g, std::vector<TextEvent>& out, bool shoulderHeld) {
    // Cycling depends on whether the pending character is in this group, not on
    // which button produced it; groups that share characters (" ' .) continue
    // each other's cycle.
    const char* at = pending_ ? std::strchr(g.chars, pending_) : nullptr;
    if (at) {
        size_t i = (size_t)(at - g.chars) + 1;
        pending_ = g.chars[i % std::strlen(g.chars)];
    } else {
        commit(out, shoulderHeld);
        pending_ = g.chars[0];
    }
    group_ = g.chars;
}

void MultiTap::update(const Gamepad& pad, std::vector<TextEvent>& out) {
    const bool shoulder = pad.down(L);

    if (pending_ && pad.ms - lastInput_ >= kCommitMs) commit(out, shoulder);

    if (pad.hit(L)) {
        upper_ = !upper_;
        lastInput_ = pad.ms;  // also restarts the pending character's timer
    }
    if (pad.released & ZL) commit(out, shoulder);
    if (pad.hit(ZR)) commit(out, shoulder);

    mode_ = pad.down(ZR) ? Mode::Commands : pad.down(ZL) ? Mode::Symbols : Mode::Letters;

    if (mode_ == Mode::Commands) {
        for (const Command& c : commands()) {
            if (!pad.hit(c.button)) continue;
            commit(out, shoulder);
            out.push_back({c.kind, c.text});
        }
        // Cursor movement on the D-pad, with key repeat while held.
        static const std::pair<Btn, TextEvent::Kind> moves[] = {
            {DLeft, TextEvent::Left}, {DRight, TextEvent::Right}, {DUp, TextEvent::Up}, {DDown, TextEvent::Down}};
        for (auto [b, kind] : moves) {
            if (!pad.rep(b)) continue;
            commit(out, shoulder);
            out.push_back({kind, {}});
        }
        return;
    }
    for (const Group& g : mode_ == Mode::Symbols ? symbols() : letters()) {
        if (!pad.hit(g.button)) continue;
        press(g, out, shoulder);
        lastInput_ = pad.ms;
    }
}
