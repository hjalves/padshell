// Editable text: a vector of UTF-8 lines plus a cursor. Deliberately simple;
// swap for a gap buffer / piece table if files ever get large.
#pragma once
#include <string>
#include <string_view>
#include <vector>

class TextBuffer {
public:
    TextBuffer() : lines_{""} {}

    // Inserts text at the cursor; '\n' splits lines.
    void insert(std::string_view text);
    void newline();
    void backspace();
    void deleteForward();

    void left();
    void right();
    void up();
    void down();
    void home();
    void end();
    void pageUp(int lines);
    void pageDown(int lines);

    bool load(const std::string& path);
    bool save(const std::string& path);
    void setText(std::string_view text);
    std::string text() const;

    const std::vector<std::string>& lines() const { return lines_; }
    int line() const { return line_; }
    size_t col() const { return col_; }              // byte offset in current line
    size_t column() const;                           // codepoint index in current line
    bool modified() const { return modified_; }
    void markSaved() { modified_ = false; }

private:
    void moveVertical(int delta);

    std::vector<std::string> lines_;
    int line_ = 0;
    size_t col_ = 0;
    long goalColumn_ = -1;  // codepoint column remembered across up/down
    bool modified_ = false;
};
