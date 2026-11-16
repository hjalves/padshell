# padshell

A Nintendo Switch homebrew "shell": a set of apps in one process, driven
entirely by the gamepad. The first app is a text editor you type into with
phone-style multi-tap on the face buttons and D-pad, with no on-screen keyboard.

![padshell editor](docs/screenshot.png)

The same code builds for desktop Linux (SDL2), so you can iterate without
copying to the console.

## Screen

After the [ameno](https://github.com/hjalves/ameno-textpad) TV web app: a slim
top bar (brand, counts, clock), a large document with characters coloured by
kind, and a status line of flat button badges with their groups. While a
character is being selected it appears at the cursor in its button's colour,
the status line shows its cycle, and a bar counts down to the commit. Tags on
the right show caps and the current mode.

## Themes and fonts

The colour theme and the font are chosen independently in the − app menu:
highlight *theme* or *font* and press A, or D-pad left/right, to cycle while
the menu stays open as a preview. Both are saved in `settings.ini` next to
the notes (`theme=sunset`, `font=space-mono`).

![Themes](docs/styles.png)

| Theme | Look |
|---|---|
| **sunset** (default) | Pure black, hot orange accent with magenta and lime |
| **dusk** | Violet-black, violet accent |
| **lava** | Warm black, red-orange accent; edit mode is crimson-pink to stay distinct |
| **gold** | Black, amber accent; the caps tag is cyan to stand apart |
| **tropic** | Teal-black, orange accent |
| **rose** | Pink-black, rose accent |
| **ember** | Warm dark brown, coral and amber |
| **midnight** | Violet-black, pink accent (the original ameno colours) |
| **moon** | Deep blue-black with a white accent, pure white text and saturated cool colours |
| **grayscale** | Monochrome: near-black with white and grey accents; buttons, modes and character kinds differ by shade |
| **ocean** | Deep navy, cyan accent |
| **carbon** | Neutral black, IBM blue |
| **neon** | Pure black, electric magenta, lime and cyan |
| **cyber** | Pure black, electric cyan accent with yellow and magenta |
| **vapor** | Violet-black, hot pink accent, softer neons |
| **ice** | Blue-black, electric periwinkle accent with cyan and mint |
| **day** | Light paper, darkened ink colours for text |

Fonts: **Space Mono** (default), JetBrains Mono, IBM Plex Mono, Fira Mono,
DejaVu Sans Mono and Courier Prime.

Conventions shared by every theme (`src/skins/family.h`):

* **Contrast for TVs.** Dark themes use near-black backgrounds, saturated
  accents (at least 4.9:1 against the background, and for dark text on
  them) and greys at 6.8:1 or better, so they hold up on cheap panels.
* **Button colours.** Each button position has its own colour wherever it
  appears: bottom green, right red, left blue, top yellow (Xbox A, B, X, Y;
  on a Switch controller B, A, Y, X), D-pad down orange, left violet, up
  teal, right magenta. Exact shades vary by theme.
* **Character colours.** In the document, capitals, digits, brackets,
  quotes, operators and other punctuation each have their own colour.
* **Mode colours.** Cursor and mode tag use the theme's accent for letters,
  green while ZL is held and red while ZR is held. The "ABC" tag lights up
  for caps.

The layout is `src/skins/ameno.cpp`; a theme is a `Palette` in
`themes::` (`family.h`) listed in `kThemes` in `ameno.cpp`, and a font is a
Regular/Bold pair listed in `kFonts` there. Skins receive a read-only view of the shell
and editor state (`src/skin.h`), so a new layout needs no changes to the
editor or input code.

## Controls

Buttons are named as printed on a **Switch** controller. (The design came from
an Xbox layout: Xbox A/B/X/Y sit where the Switch has B/A/Y/X, and
LT/RT/LB are ZL/ZR/L.)

| Button | Use |
|---|---|
| B / A / Y / X, D-pad | character groups |
| ZL (hold) | numbers and symbols (green cursor) |
| ZR (hold) | newline, space, backspace, delete line; D-pad moves the cursor (red cursor) |
| L | toggle uppercase |
| + | editor menu (save, quit). In menus: A select, B cancel, D-pad up/down move |
| − | app menu |

### Entering text

Press a character button to preview the first character of its group, and
press it again within a second to cycle. The character is committed after one
second without input, or when a character from another group is selected.
B B, wait, Y, wait types `bd`. For two characters from the same group, wait for
the first to commit. Whether a press cycles depends on whether the *current*
character is in the pressed group, so symbol groups that share `"` `'` `.`
continue each other's cycle.

| Button | Letters | ZL held |
|---|---|---|
| B | `a b c` | `1 2 3` |
| Y | `d e f` | `4 5 6` |
| X | `g h i` | `7 8 9` |
| A | `j k l` | `0 .` |
| D-pad down | `m n o` | `. , ? ! _ : ; \|` |
| D-pad left | `p q r s` | `( [ { < " '` |
| D-pad up | `t u v` | `- = + * / ^ ~ # % @` |
| D-pad right | `w x y z` | `) ] } > " '` |

Releasing ZL commits the pending character.

**Uppercase:** L toggles it, including for the character being previewed, and
restarts that character's one-second timer. It turns off after the next
committed character. If L is held while characters are committed, it stays on
while held, and after release it stays on until the next commit (or another
press of L).

**Commands (hold ZR):** B newline, Y space, A backspace, X delete the
current line, and the D-pad moves the cursor (it auto-repeats while held).
Deleting on the last line is the spec's "delete the last segment"
(`first\nsecond` → `first`, `first\n` → `first`, a single line is
cleared); on another line it removes that line. Pressing ZR, or any
command, commits the pending character first. ZR takes precedence when both
triggers are held.

Text goes in at the cursor, which starts at the end of the document when it
is opened; the view scrolls to keep it visible and the top bar shows its
line and column. The editor works on
`sdmc:/switch/padshell/notes.txt` on the Switch, or
`~/.local/share/padshell/notes.txt` on desktop (override with
`PADSHELL_DATA`). It saves automatically when you quit.

### Desktop testing

On desktop the keyboard acts as a gamepad, for testing without a controller:
`I/L/K/J` top/right/bottom/left face button, arrows D-pad, `Q/E` L/R, `Z/C`
ZL/ZR, `Return` +, `Tab` −, `Esc` quit. `tools/xdo_type.py "text"` types a
string through it with xdotool. On-screen labels follow the platform (Xbox
labels on desktop).

## Building

### Desktop (Linux)

Needs CMake, a C++20 compiler, SDL2 and SDL2_ttf
(Arch: `pacman -S cmake sdl2-compat sdl2_ttf`).

```sh
cmake -S . -B build && cmake --build build -j
ctest --test-dir build          # unit tests
./build/padshell
```

### Switch (.nro)

With devkitPro installed natively (`dkp-pacman -S switch-dev switch-sdl2 switch-sdl2_ttf`):

```sh
/opt/devkitpro/portlibs/switch/bin/aarch64-none-elf-cmake -S . -B build-switch
cmake --build build-switch -j
```

Or use the official container, with no local install:

```sh
./tools/build-switch.sh         # -> build-switch/padshell.nro
```

### Running on the console

* **Copy:** put `padshell.nro` in `sdmc:/switch/` and start it from the
  Homebrew Menu.
* **Over Wi-Fi:** in the Homebrew Menu press Y (netloader), then run
  `nxlink -s build-switch/padshell.nro`. `-s` shows the app's log output in
  your terminal.
* Start the Homebrew Menu by holding R while launching a game (title
  override), not from the Album. Album/applet mode gives apps much less memory.

## Layout

```
src/
  main.cpp          SDL setup and main loop
  platform.*        Switch and desktop differences: paths, init, controller mapping
  pad.h             logical gamepad state + edge/repeat tracking
  multitap.*        multi-tap text entry (pure logic, unit tested)
  textbuffer.*      editable UTF-8 text
  gfx.*             drawing helpers, cached monospace glyphs
  shell.*           app launcher and app switching (drawing is done by skins)
  skin.*            skin interface, registry and shared helpers
  skins/            the ameno layout (ameno.cpp) and its themes (family.h)
  app.h             interface every app implements
  apps/editor.*     the text editor
tests/              host-only unit tests
romfs/              bundled fonts: DejaVu Sans Mono, plus JetBrains Mono,
                    IBM Plex Mono, Fira Mono, Space Mono and Courier Prime
                    under the SIL OFL (licences alongside)
tools/              build script, xdotool typing helper for desktop testing
```

### Adding an app

Subclass `App` (`title`, `update`, `render`, and optionally `onEnter`/`onLeave`
and `help`), then register it in `main.cpp` with `shell.add(...)`. Apps that
need text input own a `MultiTap` and apply the `TextEvent`s it emits.
