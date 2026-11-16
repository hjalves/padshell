// Host-only unit tests for the non-drawing logic: text buffer,
// pad edge/repeat tracking and the multi-tap input method.
#include <cstdio>
#include <cstdlib>
#include <string>

#include "../src/gemini/gemtext.h"
#include "../src/gemini/url.h"
#include "../src/multitap.h"
#include "../src/skin.h"
#include "../src/pad.h"
#include "../src/textbuffer.h"
#include "../src/utf8.h"

static int g_failed = 0;
#define CHECK(cond)                                                          \
    do {                                                                     \
        if (!(cond)) {                                                       \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);      \
            ++g_failed;                                                      \
        }                                                                    \
    } while (0)

static void testBuffer() {
    TextBuffer b;
    b.insert("hello");
    b.newline();
    b.insert("world");
    CHECK(b.text() == "hello\nworld");
    CHECK(b.line() == 1 && b.column() == 5);

    b.home();
    b.backspace();  // joins lines
    CHECK(b.text() == "helloworld");
    CHECK(b.line() == 0 && b.column() == 5);

    b.deleteForward();
    CHECK(b.text() == "helloorld");

    b.setText("ab\ncdef\ng");
    b.end();
    b.down();
    CHECK(b.line() == 1 && b.column() == 2);
    b.down();  // short line clamps...
    CHECK(b.line() == 2 && b.column() == 1);
    b.up();    // ...but remembers the goal column
    CHECK(b.line() == 1 && b.column() == 2);
    CHECK(!b.modified());

    // UTF-8: cursor steps over whole codepoints.
    b.setText("h\xC3\xA9llo");  // "héllo"
    b.right();
    b.right();
    CHECK(b.column() == 2 && b.col() == 3);
    b.backspace();
    CHECK(b.text() == "hllo");

    b.setText("x");
    b.insert("1\n2\n3");
    CHECK(b.text() == "1\n2\n3x");

    // Round-trip through disk.
    std::string path = "/tmp/padshell_test.txt";
    b.setText("line one\nline two");
    CHECK(b.save(path));
    TextBuffer c;
    CHECK(c.load(path));
    CHECK(c.text() == "line one\nline two");
    std::remove(path.c_str());
}

static void testTracker() {
    PadTracker t;
    Gamepad s = t.update(South, 0, 0, 0, 0, 0);
    CHECK(s.hit(South) && s.rep(South));
    s = t.update(South, 0, 0, 0, 0, 100);
    CHECK(!s.hit(South) && !s.rep(South));
    s = t.update(South, 0, 0, 0, 0, 400);  // past repeat delay
    CHECK(s.rep(South) && !s.hit(South));
    s = t.update(South, 0, 0, 0, 0, 410);  // within repeat rate
    CHECK(!s.rep(South));
    s = t.update(0, 0, 0, 0, 0, 500);
    CHECK(s.released & South);
}

// Drives a MultiTap with a simulated clock. Each call is one frame.
struct Sim {
    MultiTap tap;
    PadTracker tracker;
    uint32_t now = 0;
    uint32_t held = 0;
    std::string out;  // committed text, with \n, <BS> and <DL> markers

    void frame(uint32_t dt = 16) {
        now += dt;
        std::vector<TextEvent> ev;
        tap.update(tracker.update(held, 0, 0, 0, 0, now), ev);
        for (auto& e : ev) {
            switch (e.kind) {
                case TextEvent::Insert: out += e.text; break;
                case TextEvent::Newline: out += "\n"; break;
                case TextEvent::Backspace: out += "<BS>"; break;
                case TextEvent::DeleteLine: out += "<DL>"; break;
                case TextEvent::Left: out += "<L>"; break;
                case TextEvent::Right: out += "<R>"; break;
                case TextEvent::Up: out += "<U>"; break;
                case TextEvent::Down: out += "<D>"; break;
            }
        }
    }
    void press(uint32_t b, int times = 1) {
        for (int i = 0; i < times; ++i) {
            held |= b; frame();
            held &= ~b; frame();
        }
    }
    void hold(uint32_t b) { held |= b; frame(); }
    void release(uint32_t b) { held &= ~b; frame(); }
    void wait(uint32_t ms) { frame(ms); }
};

static void testMultiTap() {
    {   // Spec example: A twice, wait, X once, wait -> "bd" (A = South, X = West).
        Sim s;
        s.press(South, 2);
        CHECK(s.tap.pendingText() == "b" && s.out.empty());
        s.wait(1000);
        CHECK(s.out == "b" && !s.tap.hasPending());
        s.press(West);
        s.wait(1000);
        CHECK(s.out == "bd");
    }
    {   // A different group commits immediately; same group cycles and wraps.
        Sim s;
        s.press(South);
        s.press(DRight, 5);  // w x y z w
        CHECK(s.out == "a" && s.tap.pendingText() == "w");
        s.press(DDown);
        CHECK(s.out == "aw" && s.tap.pendingText() == "m");
    }
    {   // Not committed before the full second of inactivity.
        Sim s;
        s.press(South);
        s.wait(900);
        CHECK(s.out.empty());
        s.press(South);  // cycling restarts the timer
        s.wait(900);
        CHECK(s.out.empty() && s.tap.pendingText() == "b");
        s.wait(200);
        CHECK(s.out == "b");
    }
    {   // Symbols (hold left trigger); shared characters continue the cycle.
        Sim s;
        s.hold(ZL);
        CHECK(s.tap.mode() == MultiTap::Mode::Symbols);
        s.press(DLeft, 5);  // ( [ { < "
        CHECK(s.tap.pendingText() == "\"");
        s.press(DRight);    // " is in ) ] } > " '  -> continue to '
        CHECK(s.out.empty() && s.tap.pendingText() == "'");
        s.press(East, 1);   // ' not in "0." -> commit, start 0
        CHECK(s.out == "'" && s.tap.pendingText() == "0");
        s.press(East);      // .
        s.press(DDown);     // . is in . , ? ... -> ,
        CHECK(s.tap.pendingText() == ",");
        s.release(ZL);      // releasing the trigger commits
        CHECK(s.out == "',");
        s.press(South);     // back to letters
        CHECK(s.tap.pendingText() == "a");
    }
    {   // Uppercase is one-shot and applies to the character being previewed.
        Sim s;
        s.press(South);
        s.press(L);
        CHECK(s.tap.pendingText() == "A");
        s.wait(1000);
        s.press(South);
        s.wait(1000);
        CHECK(s.out == "Aa" && !s.tap.uppercase());
    }
    {   // Toggling uppercase restarts the pending character's timer.
        Sim s;
        s.press(South);
        s.wait(900);
        s.press(L);
        s.wait(900);
        CHECK(s.out.empty());
        s.wait(200);
        CHECK(s.out == "A");
    }
    {   // Holding the shoulder through commits keeps uppercase on; after release
        // it stays on until the next commit.
        Sim s;
        s.hold(L);                  // toggles on
        s.press(South); s.wait(1000);
        s.press(West); s.wait(1000);
        CHECK(s.out == "AD" && s.tap.uppercase());
        s.release(L);
        CHECK(s.tap.uppercase());
        s.press(North); s.wait(1000);
        CHECK(s.out == "ADG" && !s.tap.uppercase());
        s.press(North); s.wait(1000);
        CHECK(s.out == "ADGg");
    }
    {   // Right trigger: commands; pressing it commits the pending character.
        Sim s;
        s.press(South);
        s.hold(ZR);
        CHECK(s.out == "a" && s.tap.mode() == MultiTap::Mode::Commands);
        s.press(South);  // newline
        s.press(West);   // space
        s.press(East);   // backspace
        s.press(North);  // delete line
        s.press(DUp);    // D-pad moves the cursor while ZR is held
        CHECK(s.out == "a\n <BS><DL><U>");
        s.hold(ZL);      // both triggers: commands win
        s.press(South);
        CHECK(s.out == "a\n <BS><DL><U>\n");
    }
}

static void testDeleteLastLine() {
    TextBuffer b;
    b.setText("first\nsecond");
    b.deleteLastLine();
    CHECK(b.text() == "first");
    b.setText("first\n");
    b.deleteLastLine();
    CHECK(b.text() == "first");
    b.deleteLastLine();
    CHECK(b.text().empty());
    b.setText("a\n\n");
    b.deleteLastLine();
    CHECK(b.text() == "a\n");
    b.setText("");
    b.deleteLastLine();
    CHECK(b.text().empty() && !b.modified());
}

static void testCursorMoves() {
    {   // ZR + D-pad commits the pending character, then moves.
        Sim s;
        s.press(South);
        s.hold(ZR);
        s.press(DLeft);
        s.press(DRight);
        s.press(DDown);
        CHECK(s.out == "a<L><R><D>");
    }
    {   // Holding a direction auto-repeats.
        Sim s;
        s.hold(ZR);
        s.hold(DLeft);
        s.wait(500);  // past the repeat delay
        s.wait(50);
        s.release(DLeft);
        int moves = 0;
        for (size_t i = s.out.find("<L>"); i != std::string::npos; i = s.out.find("<L>", i + 1)) ++moves;
        CHECK(moves >= 2);
    }
    {   // Without ZR the D-pad still types.
        Sim s;
        s.press(DLeft);
        CHECK(s.tap.pendingText() == "p" && s.out.empty());
    }
}

static void testEditingAtCursor() {
    TextBuffer b;
    b.setText("hello world");
    b.end();               // cursor at the end of line 1
    for (int i = 0; i < 6; ++i) b.left();
    b.insert(",");
    CHECK(b.text() == "hello, world");
    b.backspace();
    CHECK(b.text() == "hello world");
    b.newline();
    CHECK(b.text() == "hello\n world" && b.line() == 1 && b.column() == 0);

    // Deleting a middle line keeps the others; on the last line it matches
    // the spec's delete-last-segment.
    b.setText("one\ntwo\nthree");
    b.down();              // line 2
    b.deleteCurrentLine();
    CHECK(b.text() == "one\nthree" && b.line() == 1 && b.column() == 0);
    b.deleteCurrentLine(); // last line
    CHECK(b.text() == "one" && b.line() == 0 && b.column() == 3);
    b.deleteCurrentLine(); // single line: cleared
    CHECK(b.text().empty());
}

static void testLayout() {
    TextBuffer b;
    b.setText("abcdef\ngh");
    MultiTap tap;
    EditorView v;
    v.lines = &b.lines();
    v.tap = &tap;

    v.cursorLine = 0; v.cursorCol = 2;
    auto L = v.layout(4);  // "abcd" "ef" "gh"
    CHECK(L.rows.size() == 3 && L.cursorRow == 0 && L.cursorX == 2);
    CHECK(L.rowText(L.rows[1]) == "ef");

    v.cursorCol = 4;       // wraps onto the second row
    L = v.layout(4);
    CHECK(L.cursorRow == 1 && L.cursorX == 0);

    b.setText("abcd");     // cursor at the end of a full row gets its own row
    v.cursorLine = 0; v.cursorCol = 4;
    L = v.layout(4);
    CHECK(L.rows.size() == 2 && L.cursorRow == 1 && L.cursorX == 0);
}

static void testUrl() {
    using gem::Url;
    // RFC 3986 section 5.4 reference resolution examples (adapted to gemini:).
    Url base = Url::parse("gemini://a/b/c/d;p?q");
    struct Case { const char* ref; const char* want; };
    const Case cases[] = {
        {"g", "gemini://a/b/c/g"},        {"./g", "gemini://a/b/c/g"},     {"g/", "gemini://a/b/c/g/"},
        {"/g", "gemini://a/g"},           {"//g", "gemini://g/"},          {"?y", "gemini://a/b/c/d;p?y"},
        {"g?y", "gemini://a/b/c/g?y"},    {"#s", "gemini://a/b/c/d;p?q"},  {"", "gemini://a/b/c/d;p?q"},
        {".", "gemini://a/b/c/"},         {"./", "gemini://a/b/c/"},       {"..", "gemini://a/b/"},
        {"../", "gemini://a/b/"},         {"../g", "gemini://a/b/g"},      {"../..", "gemini://a/"},
        {"../../g", "gemini://a/g"},      {"../../../g", "gemini://a/g"},  {"/./g", "gemini://a/g"},
        {"g.", "gemini://a/b/c/g."},      {"./../g", "gemini://a/b/g"},    {"g;x=1/../y", "gemini://a/b/c/y"},
        {"https://x.org/p", "https://x.org/p"},
    };
    for (const Case& c : cases) {
        std::string got = base.resolve(c.ref).str();
        if (got != c.want) std::printf("  resolve(\"%s\") = %s, want %s\n", c.ref, got.c_str(), c.want);
        CHECK(got == c.want);
    }
    Url u = Url::parse("GEMINI://Example.ORG:1965");
    CHECK(u.str() == "gemini://example.org/" && u.portNumber() == 1965 && u.hostPort() == "example.org:1965");
    CHECK(Url::parse("gemini://h:7000/x").portNumber() == 7000);
    CHECK(Url::parse("about:home").scheme == "about" && Url::parse("about:home").path == "home");
    CHECK(!Url::parse("no scheme here").valid());
    CHECK(gem::percentEncode("a b/ç?") == "a%20b%2F%C3%A7%3F");
}

static void testGemtext() {
    auto lines = gem::parseGemtext(
        "# Title\n## Sub\n### Small\ntext line\n=> gemini://x.org/ Label here\n=>/rel\n=>  \n* item\n> quote\n"
        "```alt\n# not a heading\n=> not a link\n```\nafter\r\n");
    using L = gem::Line;
    CHECK(lines.size() == 12);
    CHECK(lines[0].kind == L::H1 && lines[0].text == "Title");
    CHECK(lines[1].kind == L::H2 && lines[2].kind == L::H3);
    CHECK(lines[3].kind == L::Text && lines[3].text == "text line");
    CHECK(lines[4].kind == L::Link && lines[4].url == "gemini://x.org/" && lines[4].text == "Label here");
    CHECK(lines[5].kind == L::Link && lines[5].url == "/rel" && lines[5].text == "/rel");
    CHECK(lines[6].kind == L::Text);  // "=>" with no URL
    CHECK(lines[7].kind == L::Item && lines[7].text == "item");
    CHECK(lines[8].kind == L::Quote && lines[8].text == "quote");
    CHECK(lines[9].kind == L::Pre && lines[9].text == "# not a heading");
    CHECK(lines[10].kind == L::Pre && lines[10].text == "=> not a link");
    CHECK(lines[11].kind == L::Text && lines[11].text == "after");
}

int main() {
    testBuffer();
    testTracker();
    testMultiTap();
    testDeleteLastLine();
    testCursorMoves();
    testEditingAtCursor();
    testLayout();
    testUrl();
    testGemtext();
    if (g_failed) {
        std::printf("%d check(s) failed\n", g_failed);
        return 1;
    }
    std::printf("all tests passed\n");
    return 0;
}
