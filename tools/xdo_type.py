#!/usr/bin/env python3
"""Type a string into a running desktop padshell through the keyboard-as-gamepad
mapping (WASD = stick, IJKL = face buttons), using xdotool. For testing only."""
import subprocess, sys, time

LOWER = "abcdefghijklmnopqrstuvwxyz.,?!'-"
DIRS = [["w"], ["w", "d"], ["d"], ["s", "d"], ["s"], ["s", "a"], ["a"], ["w", "a"]]
FACE = ["i", "l", "k", "j"]  # North, East, South, West

def xdo(*args):
    subprocess.run(["xdotool", *args], check=True)
    time.sleep(0.04)

def tap(key):
    # Hold for a few frames: the app polls key state once per frame, so an
    # instantaneous press/release would be missed (a real thumb is slower).
    xdo("keydown", key)
    time.sleep(0.06)
    xdo("keyup", key)

def press(dirs, face, shift=False):
    for k in dirs + (["c"] if shift else []):
        xdo("keydown", k)
    time.sleep(0.06)
    tap(face)
    for k in dirs + (["c"] if shift else []):
        xdo("keyup", k)

for ch in sys.argv[1]:
    if ch == " ":
        tap("k")
    elif ch == "\n":
        tap("i")
    else:
        idx = LOWER.index(ch.lower())
        press(DIRS[idx // 4], FACE[idx % 4], shift=ch.isupper())
