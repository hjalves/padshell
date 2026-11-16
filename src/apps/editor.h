#pragma once
#include <string>
#include <vector>

#include "../app.h"
#include "../multitap.h"
#include "../textbuffer.h"

// Single-file, append-only text editor driven by multi-tap input. Text is
// always added at the end of the document; Plus opens the editor menu (save,
// quit). The document is saved automatically when the editor is left.
class EditorApp : public App {
public:
    explicit EditorApp(std::string path);

    std::string title() const override;
    void onLeave() override;
    void update(const Gamepad& pad, Shell& shell) override;
    void render(Gfx& g, Rect area) override;
    std::vector<std::string> help() const override;

private:
    void apply(const std::vector<TextEvent>& events);
    bool save(Shell* shell);
    void updateMenu(const Gamepad& pad, Shell& shell);
    void renderStatus(Gfx& g, Rect r);
    void renderMenu(Gfx& g, Rect area);

    std::string path_;
    TextBuffer buf_;
    MultiTap tap_;
    bool menu_ = false;
    int menuSel_ = 0;
};
