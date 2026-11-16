// Host-only unit tests for the non-drawing logic: text buffer,
// pad edge/repeat tracking and the daisywheel input method.
#include <cstdio>
#include <cstdlib>
#include <string>

#include "../src/ime.h"
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

static std::string typeWith(DaisyWheel& w, PadTracker& t, uint32_t& clock, float x, float y,
                            uint32_t btn, uint32_t mods = 0) {
    std::vector<TextEvent> ev;
    w.update(t.update(mods, x, y, 0, 0, clock += 16), ev);
    w.update(t.update(mods | btn, x, y, 0, 0, clock += 16), ev);
    w.update(t.update(mods, x, y, 0, 0, clock += 16), ev);
    std::string out;
    for (auto& e : ev) {
        if (e.kind == TextEvent::Insert) out += e.text;
        else if (e.kind == TextEvent::Backspace) out += "<BS>";
        else if (e.kind == TextEvent::Newline) out += "<NL>";
    }
    return out;
}

static void testDaisyWheel() {
    CHECK(DaisyWheel::petalFor(0, 0) == -1);
    CHECK(DaisyWheel::petalFor(0.2f, 0.2f) == -1);  // inside deadzone
    CHECK(DaisyWheel::petalFor(0, -1) == 0);        // up
    CHECK(DaisyWheel::petalFor(0.7f, -0.7f) == 1);  // up-right
    CHECK(DaisyWheel::petalFor(1, 0) == 2);         // right
    CHECK(DaisyWheel::petalFor(0, 1) == 4);         // down
    CHECK(DaisyWheel::petalFor(-1, 0) == 6);        // left
    CHECK(DaisyWheel::petalFor(-0.7f, -0.7f) == 7); // up-left

    DaisyWheel w;
    PadTracker t;
    uint32_t clock = 0;
    CHECK(typeWith(w, t, clock, 0, -1, North) == "a");
    CHECK(typeWith(w, t, clock, 0, -1, West) == "d");
    CHECK(typeWith(w, t, clock, 1, 0, East) == "j");       // petal 2, slot 1
    CHECK(typeWith(w, t, clock, -0.7f, -0.7f, West) == "-");
    CHECK(typeWith(w, t, clock, 0, -1, North, ZR) == "A"); // shift
    CHECK(typeWith(w, t, clock, 0, 0, South) == " ");
    CHECK(typeWith(w, t, clock, 0, 0, East) == "<BS>");
    CHECK(typeWith(w, t, clock, 0, 0, North) == "<NL>");

    typeWith(w, t, clock, 0, 0, R);  // next layer: ABC
    CHECK(w.layer() == 1);
    CHECK(typeWith(w, t, clock, 0, -1, South) == "C");
    typeWith(w, t, clock, 0, 0, R);  // 123
    CHECK(typeWith(w, t, clock, 0, -1, North) == "1");
    typeWith(w, t, clock, 0, 0, R);  // wraps to abc
    CHECK(w.layer() == 0);
    typeWith(w, t, clock, 0, 0, L);  // backwards wraps to 123
    CHECK(w.layer() == 2);
}

int main() {
    testBuffer();
    testTracker();
    testDaisyWheel();
    if (g_failed) {
        std::printf("%d check(s) failed\n", g_failed);
        return 1;
    }
    std::printf("all tests passed\n");
    return 0;
}
