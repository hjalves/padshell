#include "textbuffer.h"

#include <algorithm>
#include <cstdio>

#include "utf8.h"

void TextBuffer::insert(std::string_view text) {
    size_t start = 0;
    while (start <= text.size()) {
        size_t nl = text.find('\n', start);
        std::string_view chunk = text.substr(start, nl == std::string_view::npos ? std::string_view::npos : nl - start);
        if (!chunk.empty()) {
            lines_[line_].insert(col_, chunk);
            col_ += chunk.size();
            modified_ = true;
        }
        if (nl == std::string_view::npos) break;
        newline();
        start = nl + 1;
    }
    goalColumn_ = -1;
}

void TextBuffer::newline() {
    std::string rest = lines_[line_].substr(col_);
    lines_[line_].erase(col_);
    lines_.insert(lines_.begin() + line_ + 1, std::move(rest));
    ++line_;
    col_ = 0;
    goalColumn_ = -1;
    modified_ = true;
}

void TextBuffer::backspace() {
    goalColumn_ = -1;
    if (col_ > 0) {
        size_t p = utf8::prev(lines_[line_], col_);
        lines_[line_].erase(p, col_ - p);
        col_ = p;
        modified_ = true;
    } else if (line_ > 0) {
        col_ = lines_[line_ - 1].size();
        lines_[line_ - 1] += lines_[line_];
        lines_.erase(lines_.begin() + line_);
        --line_;
        modified_ = true;
    }
}

void TextBuffer::deleteForward() {
    goalColumn_ = -1;
    std::string& l = lines_[line_];
    if (col_ < l.size()) {
        l.erase(col_, utf8::next(l, col_) - col_);
        modified_ = true;
    } else if (line_ + 1 < (int)lines_.size()) {
        l += lines_[line_ + 1];
        lines_.erase(lines_.begin() + line_ + 1);
        modified_ = true;
    }
}

void TextBuffer::left() {
    goalColumn_ = -1;
    if (col_ > 0) col_ = utf8::prev(lines_[line_], col_);
    else if (line_ > 0) { --line_; col_ = lines_[line_].size(); }
}

void TextBuffer::right() {
    goalColumn_ = -1;
    if (col_ < lines_[line_].size()) col_ = utf8::next(lines_[line_], col_);
    else if (line_ + 1 < (int)lines_.size()) { ++line_; col_ = 0; }
}

void TextBuffer::moveVertical(int delta) {
    if (goalColumn_ < 0) goalColumn_ = (long)column();
    line_ = std::clamp(line_ + delta, 0, (int)lines_.size() - 1);
    col_ = utf8::offset(lines_[line_], (size_t)goalColumn_);
}

void TextBuffer::up() { moveVertical(-1); }
void TextBuffer::down() { moveVertical(1); }
void TextBuffer::pageUp(int n) { moveVertical(-n); }
void TextBuffer::pageDown(int n) { moveVertical(n); }
void TextBuffer::home() { col_ = 0; goalColumn_ = -1; }
void TextBuffer::end() { col_ = lines_[line_].size(); goalColumn_ = -1; }

size_t TextBuffer::column() const { return utf8::length(lines_[line_], col_); }

void TextBuffer::setText(std::string_view text) {
    lines_.assign(1, "");
    line_ = 0;
    col_ = 0;
    std::string clean;
    clean.reserve(text.size());
    for (char c : text)
        if (c != '\r') clean += c;
    insert(clean);
    line_ = 0;
    col_ = 0;
    modified_ = false;
}

std::string TextBuffer::text() const {
    std::string out;
    for (size_t i = 0; i < lines_.size(); ++i) {
        if (i) out += '\n';
        out += lines_[i];
    }
    return out;
}

bool TextBuffer::load(const std::string& path) {
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) return false;
    std::string data;
    char buf[4096];
    size_t n;
    while ((n = std::fread(buf, 1, sizeof buf, f)) > 0) data.append(buf, n);
    std::fclose(f);
    setText(data);
    return true;
}

bool TextBuffer::save(const std::string& path) {
    // Write to a temp file first so a crash mid-write can't eat the old file.
    std::string tmp = path + ".tmp";
    FILE* f = std::fopen(tmp.c_str(), "wb");
    if (!f) return false;
    std::string data = text();
    bool ok = std::fwrite(data.data(), 1, data.size(), f) == data.size();
    ok = (std::fclose(f) == 0) && ok;
    if (!ok) { std::remove(tmp.c_str()); return false; }
    std::remove(path.c_str());  // rename() won't overwrite on every libc
    if (std::rename(tmp.c_str(), path.c_str()) != 0) return false;
    modified_ = false;
    return true;
}
