# padshell

A Nintendo Switch homebrew "shell": a set of apps in one process, driven
entirely by the gamepad. The first app is a text editor that you type into with
a daisywheel-style input method instead of an on-screen keyboard.

The same code builds for desktop Linux (SDL2), so you can iterate without
copying to the console.

## Controls

| Input | Action |
|---|---|
| Left stick toward a petal + face button | type the character in that position of the petal |
| Face buttons, stick centred | South: space, East: backspace, North: newline, West: delete |
| L / R | previous / next layer (abc, ABC, 123) |
| ZR (hold) | uppercase |
| D-pad | move cursor |
| ZL + D-pad | Home / End / Page up / Page down |
| + | save |
| − | app menu (switch app, quit) |

Face buttons are by **position**. On a Switch, South is B, East is A, North is
X and West is Y. In menus, confirm and back follow each platform's convention
(A/B on the Switch).

The editor works on `sdmc:/switch/padshell/notes.txt` on the Switch, or
`~/.local/share/padshell/notes.txt` on desktop (override with `PADSHELL_DATA`).
It autosaves when you quit.

### Desktop keyboard as a fake gamepad

`WASD` left stick · `I/L/K/J` North/East/South/West · arrows D-pad ·
`Q/E` L/R · `Z/C` ZL/ZR · `Return` + · `Tab` − · `Esc` quit

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
  ime.*             input methods (logic); ime_view.cpp draws them
  textbuffer.*      editable UTF-8 text
  gfx.*             drawing helpers, cached monospace glyphs
  shell.*           top bar, launcher, app switching
  app.h             interface every app implements
  apps/editor.*     the text editor
tests/              host-only unit tests
romfs/              bundled assets (DejaVu Sans Mono, see its licence)
tools/              build script, xdotool typing helper for desktop testing
```

### Adding an app

Subclass `App` (`title`, `update`, `render`, plus `onEnter`/`onLeave` if needed),
then register it in `main.cpp` with `shell.add(...)`. Apps that need text input
own an `InputMethod` (e.g. `DaisyWheel`) and apply the `TextEvent`s it emits.
