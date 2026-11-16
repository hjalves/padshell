#!/usr/bin/env python3
"""Type a string into a running desktop padshell through its keyboard-as-gamepad
mapping, using xdotool and multi-tap. For testing only.

Keys: I/L/K/J = North/East/South/West, arrows = D-pad, Z = ZL, C = ZR, Q = L."""
import subprocess, sys, time

FACE = {"S": "k", "W": "j", "N": "i", "E": "l", "Dd": "Down", "Dl": "Left", "Du": "Up", "Dr": "Right"}
LETTERS = [("S", "abc"), ("W", "def"), ("N", "ghi"), ("E", "jkl"),
           ("Dd", "mno"), ("Dl", "pqrs"), ("Du", "tuv"), ("Dr", "wxyz")]
SYMBOLS = [("S", "123"), ("W", "456"), ("N", "789"), ("E", "0."),
           ("Dl", "([{<\"'"), ("Dd", ".,?!_:;|"), ("Dr", ")]}>\"'"), ("Du", "-=+*/^~#%@")]
COMMIT_WAIT = 1.15

def xdo(*args):
    subprocess.run(["xdotool", *args], check=True)

def tap(key, hold=0.05, gap=0.05):
    # Held for a few frames: the app polls state once per frame.
    xdo("keydown", key); time.sleep(hold); xdo("keyup", key); time.sleep(gap)

def find(ch):
    for table, symbols in ((LETTERS, False), (SYMBOLS, True)):
        for btn, chars in table:
            if ch.lower() in chars and (symbols or ch.isalpha()):
                return btn, chars.index(ch.lower()) + 1, symbols
    raise SystemExit(f"cannot type {ch!r}")

prev_group = None
for ch in sys.argv[1]:
    if ch in " \n":
        xdo("keydown", "c"); time.sleep(0.05)
        tap("j" if ch == " " else "k")
        xdo("keyup", "c"); time.sleep(0.05)
        prev_group = None
        continue
    btn, presses, symbols = find(ch)
    group = (btn, symbols)
    if group == prev_group:
        time.sleep(COMMIT_WAIT)  # same group: wait for the commit first
    if symbols: xdo("keydown", "z"); time.sleep(0.05)
    if ch.isupper(): tap("q")
    for _ in range(presses):
        tap(FACE[btn])
    if symbols:
        time.sleep(COMMIT_WAIT)  # let it commit by timeout (releasing ZL would too)
        xdo("keyup", "z"); time.sleep(0.05)
        group = None
    prev_group = group
time.sleep(COMMIT_WAIT)
