---
title: "Threads that outlive your app"
date: 2026-11-16T21:00:00Z
summary: "Why padshell crashed on relaunch: on the Switch, hbloader reuses one process, so a thread left running corrupts the next program."
---
After the Gemini browser landed, padshell started crashing, but never in the
same place. The Atmosphere crash reports pointed at `malloc`, often right at
launch, and most reliably after launch, exit, relaunch.

## Lesson one: detach does nothing

The first version started a new thread for every page request and set the
"detached" flag in the pthread attributes so the threads would clean up
after themselves. On the Switch, libsysbase ignores that flag, and
`pthread_detach` is a no-op. Every page load leaked a joinable thread and its
512 KB stack.

The fix was one long-lived worker thread that handles requests one at a
time, with a generation counter to drop results nobody is waiting for any
more.

## Lesson two: the process doesn't end

That helped, but the relaunch crash remained. The reason is how homebrew
runs: apps launched from the Homebrew Menu run inside **hbloader**, which
keeps the *same process* and loads the next `.nro` into reused memory.

So when padshell exited, its worker thread did not die with it. It kept
waiting on a condition variable, in memory that the next program (the
Homebrew Menu, then padshell again) was now using. When that memory changed,
the thread woke up on garbage and corrupted the heap.

The rule is simple: **join every thread before you exit.** The fetcher's
destructor now cancels the request in flight, tells the worker to quit and
waits for it.

## Bonus: reading crash reports without the SD card

When padshell is started with `nxlink -s`, it prints the newest Atmosphere
crash report over the network, so a crash can be diagnosed from the computer
that sent the app. That is how an earlier exit crash was tracked down:
sockets were being torn down while stdout was still redirected to them.
