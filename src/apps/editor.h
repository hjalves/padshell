#pragma once
#include <memory>
#include <string>

#include "../app.h"
#include "../ime.h"
#include "../textbuffer.h"

// Single-file text editor. Plus saves; it also autosaves when you leave it.
class EditorApp : public App {
public:
    explicit EditorApp(std::string path);

    std::string title() const override;
    void onLeave() override;
    void update(const Gamepad& pad, Shell& shell) override;
    void render(Gfx& g, Rect area) override;

    static constexpr int kImeW = 420;

private:
    bool save(Shell* shell);

    std::string path_;
    TextBuffer buf_;
    std::unique_ptr<InputMethod> ime_;
    int top_ = 0;      // first visible line
    int left_ = 0;     // first visible column
    int visibleLines_ = 20;
    uint32_t lastEdit_ = 0;  // for cursor blink
};
