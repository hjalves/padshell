// A single-line text field typed with the multi-tap input method, for the
// browser's address bar and Gemini input prompts. ZR + newline submits; the
// D-pad (with ZR) moves the cursor, up/down jump to the start/end, and
// delete-line clears the field.
#pragma once
#include <string>
#include <string_view>
#include <vector>

#include "multitap.h"
#include "skin.h"
#include "textbuffer.h"

class TextInput {
public:
    void set(std::string_view text) {
        buf_.setText(text);
        buf_.moveToEnd();
        tap_ = MultiTap();
    }
    std::string text() const { return buf_.text(); }

    // Feeds one frame of input; returns true when the user submits.
    bool update(const Gamepad& pad) {
        held_ = pad.held;
        std::vector<TextEvent> ev;
        tap_.update(pad, ev);
        bool submit = false;
        for (const TextEvent& e : ev) {
            switch (e.kind) {
                case TextEvent::Insert: buf_.insert(e.text); break;
                case TextEvent::Newline: submit = true; break;
                case TextEvent::Backspace: buf_.backspace(); break;
                case TextEvent::DeleteLine: buf_.setText(""); break;
                case TextEvent::Left: buf_.left(); break;
                case TextEvent::Right: buf_.right(); break;
                case TextEvent::Up: buf_.home(); break;
                case TextEvent::Down: buf_.end(); break;
            }
        }
        return submit;
    }

    // Commits a character still being picked (e.g. before sending).
    void flush() {
        std::vector<TextEvent> ev;
        tap_.flush(ev);
        for (const TextEvent& e : ev)
            if (e.kind == TextEvent::Insert) buf_.insert(e.text);
    }

    EditorView view() const {
        EditorView v;
        v.lines = &buf_.lines();
        v.tap = &tap_;
        v.held = held_;
        v.cursorLine = buf_.line();
        v.cursorCol = (int)buf_.column();
        return v;
    }

private:
    TextBuffer buf_;
    MultiTap tap_;
    uint32_t held_ = 0;
};
