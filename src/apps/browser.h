#pragma once
#include <string>
#include <utility>
#include <vector>

#include "../app.h"
#include "../gemini/client.h"
#include "../gemini/gemtext.h"
#include "../skin.h"
#include "../textinput.h"

// A Gemini browser. D-pad up/down moves between links, the sticks scroll,
// A opens, B goes back. Pages asking for input (status 10/11) and the
// address bar use the multi-tap text input. Bookmarks live in
// <dataDir>/bookmarks.gmi and are listed on the home page (about:home).
class BrowserApp : public App {
public:
    explicit BrowserApp(std::string dataDir);

    std::string title() const override { return "Gemini"; }
    std::string heading() const override { return "gemini"; }
    void update(const Gamepad& pad, Shell& shell) override;
    void render(Gfx& g, Rect area) override;

private:
    struct Page {
        std::string url;
        std::string status;  // shown under the top bar
        std::vector<gem::Line> lines;
        float scroll = 0;
        int selected = -1;
    };
    enum class Mode { Page, Input, Address };
    enum class Menu { None, Page, Input };

    // Navigation.
    void navigate(const std::string& target, bool push = true);
    void startFetch(const gem::Url& url);
    void onResponse(gem::Response r);
    void show(Page p);
    void showError(const std::string& title, const std::string& detail, bool retry = true);
    void back();
    void forward();
    void openSelected();

    // Page interaction.
    void moveSelection(int dir);
    void scrollBy(float rows);
    void reveal();
    int linkCount() const;
    const gem::Line* selectedLink() const;

    // Bookmarks and the home page.
    void loadBookmarks();
    void saveBookmarks();
    bool bookmarked(const std::string& url) const;
    void toggleBookmark(Shell& shell);
    std::string homePage() const;

    void openMenu(Menu m);
    void updateMenu(const Gamepad& pad, Shell& shell);
    std::vector<std::string> menuItems() const;

    std::string dataDir_, knownHosts_, bookmarksPath_;
    std::vector<std::pair<std::string, std::string>> bookmarks_;  // url, title

    gem::Fetcher fetcher_;
    std::string loadingUrl_;
    bool pushOnLoad_ = true;
    int redirects_ = 0;
    uint32_t loadStart_ = 0;

    Page page_;
    std::vector<Page> back_, forward_;
    BrowserLayout layout_;
    bool revealPending_ = false;

    Mode mode_ = Mode::Page;
    TextInput input_;
    std::string prompt_, inputUrl_;
    bool sensitive_ = false;

    Menu menu_ = Menu::None;
    int menuSel_ = 0;

    // A refused certificate change, offered as "trust new certificate".
    std::string certHostPort_, certFingerprint_, certUrl_;

    uint32_t held_ = 0, lastMs_ = 0;
};
