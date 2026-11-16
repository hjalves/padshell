# padshell

A Nintendo Switch homebrew "shell": a set of apps in one process, driven
entirely by the gamepad. The first app is a text editor you type into with
phone-style multi-tap on the face buttons and D-pad, with no on-screen keyboard.

![padshell editor](docs/screenshot.png)

The same code builds for desktop Linux (SDL2), so you can iterate without
copying to the console.

## Screen

The editor shows the document with a cursor at the end. While a character is
being selected it appears at the cursor and its group shows the cycle. Every
style also shows the eight button groups (or, while a trigger is held, the
symbol groups or edit commands), lights up the buttons being pressed, and
indicates the current mode.

## Styles

Eight looks ("skins"), picked in the − app menu: highlight *style* and press
A, or D-pad left/right, to cycle while the menu stays open as a preview. The
choice is saved in `settings.ini` next to the notes.

![All styles](docs/styles.png)

Flat, typographic (Metro family):

| Style | Look |
|---|---|
| **metro** (default) | 10-foot TV layout. Black canvas, big light heading, flat colour tiles in controller order, per-mode palettes, TV-safe margins. |
| **zune** | Zune HD / Windows Phone hub. A giant cropped heading bleeding off the edge, modes as pivot headers, the groups as pure type with no boxes. |
| **tiles** | Windows 8 Start screen. Document on the left, groups as square live tiles in two 2x2 blocks (each diamond turned 45°: up/X top-left, right/A top-right, left/Y bottom-left, down/B bottom-right), wide mode tiles, charms-style menu. |
| **swiss** | International Typographic Style. Light paper, heavy black sans, ruled grid cells, a single red accent, the mode set vertically in the margin. |

Pixel art (drawn at 320x180, scaled 4x with no smoothing):

| Style | Look |
|---|---|
| **arcade** | Press Start 2P and a PICO-8-style palette, bevelled colour keys. |
| **gameboy** | The original 4-shade green LCD, RPG window boxes, inverse video, a dot-matrix pixel grid; the cursor shape shows the mode. |
| **synthwave** | 80s neon sunset: gradient sky, striped sun, a scrolling perspective grid, neon-outlined keys. |

And:

| Style | Look |
|---|---|
| **xmb** | Inspired by the XrossMediaBar. A colour wash that changes with the month, waving translucent ribbons, the groups as a horizontal icon row that glides to keep the active one in place, its characters as a vertical list beneath (earlier ones scroll above the row), and a slide-in options panel. |

Each skin is one file in `src/skins/` (pixel skins share `pixel.h`). Skins
receive a read-only view of the shell and editor state (`src/skin.h`) and draw
everything themselves, so a new look needs no changes to the editor or input
code.

## Controls

Buttons are named as printed on a **Switch** controller. (The design came from
an Xbox layout: Xbox A/B/X/Y sit where the Switch has B/A/Y/X, and
LT/RT/LB are ZL/ZR/L.)

| Button | Use |
|---|---|
| B / A / Y / X, D-pad | character groups |
| ZL (hold) | numbers and symbols (green cursor) |
| ZR (hold) | newline, space, backspace, delete line (red cursor) |
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

**Commands (hold ZR):** B newline, Y space, A backspace, X delete the last
line (`first\nsecond` → `first`, `first\n` → `first`, a single line is
cleared). Pressing ZR, or any command, commits the pending character first.
ZR takes precedence when both triggers are held.

Text is always added at the end of the document. The editor works on
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
  skins/            one file per visual style
  app.h             interface every app implements
  apps/editor.*     the text editor
tests/              host-only unit tests
romfs/              bundled fonts (DejaVu; Press Start 2P under the SIL OFL,
                    licences alongside)
tools/              build script, xdotool typing helper for desktop testing
```

### Adding an app

Subclass `App` (`title`, `update`, `render`, and optionally `onEnter`/`onLeave`
and `help`), then register it in `main.cpp` with `shell.add(...)`. Apps that
need text input own a `MultiTap` and apply the `TextEvent`s it emits.
