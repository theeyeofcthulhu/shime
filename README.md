# shime (**sh**ell t**ime**)

## Dependencies

### Runtime

- [ncurses](https://invisible-island.net/ncurses/ncurses.html) - for drawing to the terminal
- (Optional) [sdl2](https://www.libsdl.org/download-2.0.php) - for playing a short sound when a timer finishes
- (Optional) [libnotify](https://gitlab.gnome.org/GNOME/libnotify) - for giving notifications when a timer finishes

### Build

- CMake
- Make
- (Optional, for finding libnotify and dependencies) Pkg-Config
- C compiler

## Quick Description

Small terminal clock using ncurses.

## Quick Start

```console
$ ./build.sh
$ ./shime
```

## Usage

Move the clock around with vim (h, j, k, l) or arrow keys.
Quit with q, escape or Ctrl-c.

The default format is German (Day.Month.Year Hour:Minute:Second),
but can be switched to US (Month/Day/Year Hour:Minute:Second)
by passing '-f us' to the program.

The program can be run in timer mode:
```console
$ shime -t 00:30
```
The time to count down is specified like: HOURS:MINUTES:SECONDS.
The remaining time will be displayed on the screen.
At the end, a short 'ding' sound will play and the program will exit.

For other options see
```console
$ shime -h
```

## External Files

"Bell, Counter, A.wav" by InspectorJ ([www.jshaw.co.uk](https://www.jshaw.co.uk)) of [Freesound.org](https://freesound.org)
