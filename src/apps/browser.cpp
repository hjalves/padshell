#include "browser.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>

#include "../shell.h"

namespace {
constexpr size_t kMaxHistory = 50;

std::string lowerMime(const std::string& meta) {
    std::string m = meta.substr(0, meta.find(';'));
    while (!m.empty() && m.back() == ' ') m.pop_back();
    for (char& c : m) c = (char)std::tolower((unsigned char)c);
    return m;
}

std::string statusText(int status, const std::string& meta) {
    std::string s = std::to_string(status);
    if (!meta.empty()) s += " " + meta;
    return s.size() > 60 ? s.substr(0, 57) + "..." : s;
}
}  // namespace

BrowserApp::BrowserApp(std::string dataDir)
    : dataDir_(std::move(dataDir)), knownHosts_(dataDir_ + "known_hosts"), bookmarksPath_(dataDir_ + "bookmarks.gmi") {
    loadBookmarks();
    navigate("about:home", false);
}

// --- bookmarks ------------------------------------------------------------------

void BrowserApp::loadBookmarks() {
    std::ifstream in(bookmarksPath_);
    if (!in) {  // first run: a few starting points
        bookmarks_ = {{"gemini://geminiprotocol.net/", "Project Gemini"},
                      {"gemini://kennedy.gemi.dev/", "Kennedy (search engine)"},
                      {"gemini://bbs.geminispace.org/", "Gemini BBS"}};
        return;
    }
    bookmarks_.clear();
    for (const gem::Line& l : gem::parseGemtext(std::string(std::istreambuf_iterator<char>(in), {})))
        if (l.kind == gem::Line::Link) bookmarks_.emplace_back(l.url, l.text);
}

void BrowserApp::saveBookmarks() {
    std::ofstream out(bookmarksPath_, std::ios::trunc);
    for (auto& [url, title] : bookmarks_) out << "=> " << url << ' ' << title << '\n';
}

bool BrowserApp::bookmarked(const std::string& url) const {
    return std::any_of(bookmarks_.begin(), bookmarks_.end(), [&](auto& b) { return b.first == url; });
}

void BrowserApp::toggleBookmark(Shell& shell) {
    // On the home page the selected bookmark is the target; elsewhere the page.
    std::string url = page_.url, title;
    if (page_.url == "about:home") {
        const gem::Line* l = selectedLink();
        if (!l || !bookmarked(l->url)) return;
        url = l->url;
    }
    auto it = std::find_if(bookmarks_.begin(), bookmarks_.end(), [&](auto& b) { return b.first == url; });
    if (it != bookmarks_.end()) {
        bookmarks_.erase(it);
        shell.toast("bookmark removed");
    } else {
        for (const gem::Line& l : page_.lines)  // the page's first heading makes a good title
            if (l.kind == gem::Line::H1 && !l.text.empty()) {
                title = l.text;
                break;
            }
        bookmarks_.emplace_back(url, title.empty() ? url : title);
        shell.toast("bookmarked");
    }
    saveBookmarks();
    if (page_.url == "about:home") {  // refresh the list in place
        Page p = page_;
        p.lines = gem::parseGemtext(homePage());
        p.selected = std::min(p.selected, std::max(0, (int)bookmarks_.size() - 1));
        page_ = p;
    }
}

std::string BrowserApp::homePage() const {
    std::string s = "# padshell gemini\n\n## Bookmarks\n";
    if (bookmarks_.empty()) s += "No bookmarks yet. Press + on a page to add one.\n";
    for (auto& [url, title] : bookmarks_) s += "=> " + url + " " + title + "\n";
    s += "\n## Controls\n";
    s += std::string("* D-pad up/down: move between links; ") + btnLabel(kConfirm) + ": open; " + btnLabel(kBack) + ": back\n";
    s += "* Sticks: scroll. D-pad left/right: page up/down. L/R: back/forward\n";
    s += std::string("* ") + btnLabel(West) + ": go to an address. " + btnLabel(North) + ": home (this page)\n";
    s += "* +: menu (bookmark, reload, address). -: switch apps\n";
    s += std::string("* When typing: multi-tap as in notes; ") + btnLabel(ZR) + " + " + btnLabel(South) + " sends\n";
    return s;
}

// --- navigation -----------------------------------------------------------------

void BrowserApp::navigate(const std::string& target, bool push) {
    gem::Url base = gem::Url::parse(page_.url.empty() ? "about:home" : page_.url);
    gem::Url url = page_.url.empty() ? gem::Url::parse(target) : base.resolve(target);
    if (!url.valid()) url = gem::Url::parse(target);
    pushOnLoad_ = push;
    redirects_ = 0;
    certHostPort_.clear();
    if (url.scheme == "about") {
        fetcher_.cancel();
        Page p;
        p.url = "about:home";
        p.status = "home";
        p.lines = gem::parseGemtext(homePage());
        show(p);
        return;
    }
    if (url.scheme != "gemini") {
        // Other schemes can't be opened here; say so on a page.
        Page p;
        p.url = url.str();
        p.status = "unsupported";
        p.lines = gem::parseGemtext("# Can't open this link\n\npadshell only speaks Gemini; this is a " + url.scheme +
                                    " link:\n\n```\n" + url.str() + "\n```\n");
        show(p);
        return;
    }
    startFetch(url);
}

void BrowserApp::startFetch(const gem::Url& url) {
    loadingUrl_ = url.str();
    loadStart_ = SDL_GetTicks();
    fetcher_.start(url, knownHosts_);
}

void BrowserApp::onResponse(gem::Response r) {
    using E = gem::Response::Error;
    const gem::Url url = gem::Url::parse(loadingUrl_);
    // One line per response; visible with `nxlink -s` on the Switch.
    SDL_Log("gemini: %s -> %s (%u ms)", loadingUrl_.c_str(),
            r.error != E::None ? r.errorText.c_str() : statusText(r.status, r.meta).c_str(),
            (unsigned)(SDL_GetTicks() - loadStart_));
    if (r.error != E::None) {
        if (r.error == E::Cancelled) return;
        if (r.error == E::CertChanged) {
            certHostPort_ = url.hostPort();
            certFingerprint_ = r.fingerprint;
            certUrl_ = loadingUrl_;
            showError("Certificate changed",
                      r.errorText +
                          "\n\nThis can mean the site renewed its certificate, or that someone is intercepting the "
                          "connection. If you trust it, choose \"trust new certificate\" in the + menu.",
                      false);
            return;
        }
        showError("Could not load the page", r.errorText);
        return;
    }
    const int kind = r.status / 10;
    if (kind == 1) {  // input expected
        mode_ = Mode::Input;
        prompt_ = r.meta.empty() ? "Input:" : r.meta;
        sensitive_ = r.status == 11;
        inputUrl_ = loadingUrl_;
        input_.set("");
        return;
    }
    if (kind == 3) {  // redirect
        if (++redirects_ > 5) {
            showError("Too many redirects", "The server redirected more than five times.");
            return;
        }
        gem::Url next = url.resolve(r.meta);
        if (next.scheme != "gemini") {
            showError("Redirect to another protocol", "The server redirected to " + next.str() + ".", false);
            return;
        }
        startFetch(next);
        return;
    }
    if (kind == 2) {
        Page p;
        p.url = loadingUrl_;
        p.status = statusText(r.status, lowerMime(r.meta.empty() ? "text/gemini" : r.meta));
        std::string mime = lowerMime(r.meta.empty() ? "text/gemini" : r.meta);
        if (mime == "text/gemini") p.lines = gem::parseGemtext(r.body);
        else if (mime.rfind("text/", 0) == 0) p.lines = gem::parsePlain(r.body);
        else
            p.lines = gem::parseGemtext("# Can't display this file\n\nIt is " + mime + " (" +
                                        std::to_string(r.body.size()) + " bytes); only text pages are shown.\n");
        show(p);
        return;
    }
    if (kind == 6) {
        showError("Client certificate required", "This page needs a client certificate, which padshell doesn't support yet.",
                  false);
        return;
    }
    // 4x temporary / 5x permanent failure.
    showError(kind == 4 ? "Temporary failure" : "Page not available", statusText(r.status, r.meta));
}

void BrowserApp::showError(const std::string& title, const std::string& detail, bool retry) {
    Page p;
    p.url = loadingUrl_;
    p.status = "error";
    std::string gmi = "# " + title + "\n\n" + detail + "\n";
    if (retry) gmi += "\n=> " + loadingUrl_ + " Try again\n";
    p.lines = gem::parseGemtext(gmi);
    show(p);
}

void BrowserApp::show(Page p) {
    if (pushOnLoad_ && !page_.url.empty()) {
        back_.push_back(std::move(page_));
        if (back_.size() > kMaxHistory) back_.erase(back_.begin());
        forward_.clear();
    }
    pushOnLoad_ = true;
    page_ = std::move(p);
    page_.scroll = 0;
    page_.selected = linkCount() > 0 ? 0 : -1;
    layout_ = BrowserLayout{};
    mode_ = Mode::Page;
}

void BrowserApp::back() {
    if (back_.empty()) return;
    forward_.push_back(std::move(page_));
    page_ = std::move(back_.back());
    back_.pop_back();
    layout_ = BrowserLayout{};
}

void BrowserApp::forward() {
    if (forward_.empty()) return;
    back_.push_back(std::move(page_));
    page_ = std::move(forward_.back());
    forward_.pop_back();
    layout_ = BrowserLayout{};
}

int BrowserApp::linkCount() const {
    return (int)std::count_if(page_.lines.begin(), page_.lines.end(), [](auto& l) { return l.kind == gem::Line::Link; });
}

const gem::Line* BrowserApp::selectedLink() const {
    int k = 0;
    for (const gem::Line& l : page_.lines)
        if (l.kind == gem::Line::Link && k++ == page_.selected) return &l;
    return nullptr;
}

void BrowserApp::openSelected() {
    if (const gem::Line* l = selectedLink()) navigate(l->url);
}

// --- scrolling and link selection --------------------------------------------------

void BrowserApp::scrollBy(float rows) {
    const float maxScroll = std::max(0, layout_.rows - layout_.visible);
    page_.scroll = std::clamp(page_.scroll + rows, 0.0f, maxScroll);
}

void BrowserApp::reveal() {
    if (page_.selected < 0 || page_.selected >= (int)layout_.links.size()) return;
    auto [first, last] = layout_.links[page_.selected];
    if (first < 0) return;
    if (first < page_.scroll) page_.scroll = (float)first;
    else if (last >= page_.scroll + layout_.visible) page_.scroll = (float)std::min(first, last - layout_.visible + 1);
    scrollBy(0);
}

// Up/down: the next link if it is on screen or close below/above; otherwise
// scroll half a screen, so long text between links can still be read.
void BrowserApp::moveSelection(int dir) {
    const auto& links = layout_.links;
    const float top = page_.scroll, bottom = page_.scroll + layout_.visible;
    const float half = layout_.visible / 2.0f;
    if (links.empty()) {
        scrollBy(dir * half);
        return;
    }
    auto onScreen = [&](int i) { return links[i].second >= top && links[i].first < bottom; };
    int cur = page_.selected;
    if (cur < 0 || cur >= (int)links.size() || !onScreen(cur)) {
        // The selection scrolled away: pick the first (or last) link on screen.
        int pick = -1;
        for (int i = 0; i < (int)links.size(); ++i)
            if (onScreen(i) && (pick < 0 || dir < 0)) pick = i;
        if (pick >= 0) {
            page_.selected = pick;
            revealPending_ = true;
        } else {
            scrollBy(dir * half);
        }
        return;
    }
    int next = cur + dir;
    if (next < 0 || next >= (int)links.size()) {
        scrollBy(dir * half);
        return;
    }
    bool near = dir > 0 ? links[next].first < bottom + half : links[next].second >= top - half;
    if (near) {
        page_.selected = next;
        revealPending_ = true;
    } else {
        scrollBy(dir * half);
    }
}

// --- menus --------------------------------------------------------------------------

std::vector<std::string> BrowserApp::menuItems() const {
    if (menu_ == Menu::Input) return {"send", "cancel"};
    std::vector<std::string> items;
    if (page_.url == "about:home") {
        if (const gem::Line* l = selectedLink(); l && bookmarked(l->url)) items.push_back("remove this bookmark");
    } else if (page_.url.rfind("gemini://", 0) == 0) {
        items.push_back(bookmarked(page_.url) ? "remove bookmark" : "bookmark this page");
    }
    items.push_back("bookmarks");
    items.push_back("go to address");
    if (page_.url.rfind("gemini://", 0) == 0) items.push_back("reload");
    if (!certHostPort_.empty()) items.push_back("trust new certificate");
    return items;
}

void BrowserApp::openMenu(Menu m) {
    menu_ = m;
    menuSel_ = 0;
}

void BrowserApp::updateMenu(const Gamepad& pad, Shell& shell) {
    const auto items = menuItems();
    const int n = (int)items.size();
    if (pad.hit(Plus) || pad.hit(kBack)) {
        menu_ = Menu::None;
        return;
    }
    if (pad.rep(DUp)) menuSel_ = (menuSel_ + n - 1) % n;
    if (pad.rep(DDown)) menuSel_ = (menuSel_ + 1) % n;
    if (!pad.hit(kConfirm)) return;
    const std::string choice = items[menuSel_];
    const Menu which = menu_;
    menu_ = Menu::None;
    if (which == Menu::Input) {
        if (choice == "cancel") {
            mode_ = Mode::Page;
            return;
        }
        // "send": the same as ZR + newline.
        input_.flush();
        if (mode_ == Mode::Address) {
            std::string text = input_.text();
            if (text.find("://") == std::string::npos && text.rfind("about:", 0) != 0) text = "gemini://" + text;
            mode_ = Mode::Page;
            navigate(text);
        } else {
            gem::Url u = gem::Url::parse(inputUrl_);
            u.hasQuery = true;
            u.query = gem::percentEncode(input_.text());
            mode_ = Mode::Page;
            startFetch(u);
        }
        return;
    }
    if (choice == "bookmark this page" || choice == "remove bookmark" || choice == "remove this bookmark")
        toggleBookmark(shell);
    else if (choice == "bookmarks") navigate("about:home");
    else if (choice == "go to address") {
        mode_ = Mode::Address;
        prompt_ = "Go to address:";
        sensitive_ = false;
        input_.set(page_.url.rfind("gemini://", 0) == 0 ? page_.url : "gemini://");
    } else if (choice == "reload") {
        pushOnLoad_ = false;
        redirects_ = 0;
        startFetch(gem::Url::parse(page_.url));
    } else if (choice == "trust new certificate") {
        gem::trustCertificate(knownHosts_, certHostPort_, certFingerprint_);
        shell.toast("certificate trusted");
        std::string url = certUrl_;
        certHostPort_.clear();
        pushOnLoad_ = false;
        startFetch(gem::Url::parse(url));
    }
}

// --- frame --------------------------------------------------------------------------

void BrowserApp::update(const Gamepad& pad, Shell& shell) {
    held_ = pad.held;
    const float dt = lastMs_ ? std::min(0.1f, (pad.ms - lastMs_) / 1000.0f) : 0.0f;
    lastMs_ = pad.ms;

    if (auto r = fetcher_.poll()) onResponse(std::move(*r));
    if (revealPending_ && !layout_.links.empty()) {
        reveal();
        revealPending_ = false;
    }

    if (menu_ != Menu::None) {
        updateMenu(pad, shell);
        return;
    }

    if (mode_ != Mode::Page) {
        if (pad.hit(Plus)) {
            openMenu(Menu::Input);
            return;
        }
        if (input_.update(pad)) {
            // Submitted with ZR + newline: reuse the menu's "send" path.
            menu_ = Menu::Input;
            menuSel_ = 0;
            Gamepad confirm;
            confirm.pressed = confirm.held = kConfirm;
            updateMenu(confirm, shell);
        }
        return;
    }

    if (pad.hit(Plus)) {
        openMenu(Menu::Page);
        return;
    }
    if (pad.hit(kBack)) {
        if (fetcher_.busy()) fetcher_.cancel();
        else back();
    }
    if (pad.hit(L)) back();
    if (pad.hit(R)) forward();
    if (pad.hit(kConfirm)) openSelected();
    if (pad.hit(West)) {
        mode_ = Mode::Address;
        prompt_ = "Go to address:";
        sensitive_ = false;
        input_.set(page_.url.rfind("gemini://", 0) == 0 ? page_.url : "gemini://");
    }
    if (pad.hit(North)) navigate("about:home");
    if (pad.rep(DDown)) moveSelection(1);
    if (pad.rep(DUp)) moveSelection(-1);
    if (pad.rep(DRight)) scrollBy((float)std::max(1, layout_.visible - 1));
    if (pad.rep(DLeft)) scrollBy((float)-std::max(1, layout_.visible - 1));

    // Analog scrolling: speed grows with how far the stick is pushed.
    const float stick = std::fabs(pad.ly) > std::fabs(pad.ry) ? pad.ly : pad.ry;
    const float dead = 0.2f;
    if (std::fabs(stick) > dead) {
        const float k = (std::fabs(stick) - dead) / (1 - dead);
        scrollBy((stick > 0 ? 1 : -1) * k * k * 40.0f * dt);
    }
}

void BrowserApp::render(Gfx& g, Rect area) {
    BrowserView v;
    v.url = mode_ == Mode::Input ? inputUrl_ : (fetcher_.busy() ? loadingUrl_ : page_.url);
    v.loading = fetcher_.busy();
    if (v.loading) {
        int secs = (int)((SDL_GetTicks() - loadStart_) / 1000);
        v.status = "loading" + std::string(secs > 0 ? " " + std::to_string(secs) + "s" : "") + "\xE2\x80\xA6";
    } else if (mode_ == Mode::Input) {
        v.status = sensitive_ ? "11 sensitive input" : "10 input";
    } else if (mode_ == Mode::Address) {
        v.status = "address";
    } else {
        v.status = page_.status;
    }
    v.lines = &page_.lines;
    v.selected = page_.selected;
    v.scroll = page_.scroll;
    v.held = held_;
    v.input = mode_ != Mode::Page;
    v.prompt = prompt_;
    v.sensitive = sensitive_;
    if (v.input) v.field = input_.view();
    v.layout = &layout_;
    Skin& skin = skins::current();
    skin.browser(g, area, v);
    scrollBy(0);  // keep the scroll in range once the layout is known
    if (menu_ != Menu::None) skin.menu(g, "gemini", menuItems(), menuSel_);
}
