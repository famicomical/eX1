# eX1 — Sharp X1 / X1turbo / X1turboZ emulator for macOS

A native macOS/SDL2 port of the Sharp X1-family emulators from Toshiya
Takeda's [Common Source Code Project](http://takeda-toshiya.my.coocan.jp/)
(CSCP). The Windows-only OSD layer (Win32/DirectX/DirectSound) is replaced
with a new SDL2 backend and a Dear ImGui menu/status bar that clones the
original Windows UI.

Builds three variants from the same tree:

| binary | app | machine |
|---|---|---|
| `ex1` | eX1.app | SHARP X1 |
| `ex1turbo` | eX1turbo.app | SHARP X1turbo |
| `ex1turboz` | eX1turboZ.app | SHARP X1turboZ |

## Building

This repo tracks only the port; the machine cores come from upstream CSCP.

```sh
# prerequisites: Xcode command-line tools, SDL2, a 7z extractor
brew install sdl2 sevenzip          # (MacPorts works too: port install libsdl2 7zip)

git clone <this repo> && cd <repo>
./fetch_upstream.sh                 # downloads + unpacks upstream CSCP sources
make                                # -> build/ex1, build/ex1turbo, build/ex1turboz
```

Optional `.app` bundles (also needs ImageMagick for icon conversion —
`brew install imagemagick`):

```sh
make app                            # -> build/eX1.app, eX1turbo.app, eX1turboZ.app
```

The bundles are self-contained: the SDL2 dylib is copied into
`Contents/Frameworks/` and the binary re-pointed at it, so they run on
machines without Homebrew/MacPorts. They are ad-hoc codesigned; prebuilt
apps are not distributed (Gatekeeper, and the ROMs can't be shipped anyway) —
build them yourself with the two commands above.

## ROMs (you must supply your own)

The ROM images are copyrighted by Sharp and **not included**. If they are
missing, the emulator starts to a "ROM files required" screen that shows the
folder to fill, which files are still missing, and a Restart button.

Where the emulator looks:

- **Terminal launch**: the current working directory.
- **.app launch** (Finder/Dock): the shared `~/Library/Application Support/eX1/`
  — one ROM folder serves all three variants.

Expected files (same names as the Windows build):

| file | purpose |
|---|---|
| `IPL.ROM` (32KB) | X1turbo/turboZ boot ROM — or `IPLROM.X1T` |
| `IPLROM.X1` (4KB) | plain-X1 boot ROM (the 32KB turbo IPL will **not** boot the plain X1) |
| `FNT0808.X1` / `ANK8.ROM` | 8×8 font |
| `FNT0816.X1` / `ANK16.ROM` | 8×16 font (text display) |
| `KANJI.ROM` / `FNT1616.X1` | kanji font (optional) |
| `SUBROM.X1T`, `KBDROM.X1T` | sub-CPU/keyboard ROMs (optional; a pseudo sub-CPU is emulated without them) |
| `CZ8RB.ROM`, `EMM.ROM` | expansion ROMs (optional) |

## Running

```sh
cd build && ./ex1turbo              # boots the IPL ("looking for a program from FD0")
./ex1turbo game.d88                 # boot with a floppy in drive 0
```

- Drag-and-drop a disk/tape image onto the window to mount it (a multi-bank
  `.d88` auto-mounts bank 1 into FD1).
- The menu bar mirrors the Windows build: Control (reset, CPU speed,
  save/load state), FD0–FD3, CMT, HD, Device, Sound, Screen.
- File dialogs are native macOS panels.
- Config is saved on exit to `<variant>.ini` next to the ROMs.

## What the port changes

Upstream CSCP is untouched except for three files; everything else the port
adds is new code in its own directory.

- **`src/sdl/`** (new) — the whole macOS OSD layer: SDL2 video/audio/input
  (`osd_*.cpp`), the main loop (`main.cpp`), and a Dear ImGui menu + status
  bar (`gui.cpp`) cloning the win32 menus. Dear ImGui 1.92.9 is vendored in
  `src/sdl/imgui/`, win32-isms are shimmed in `src/sdl/compat/`.
- **`src/common.cpp`, `src/common.h`, `src/emu.h`** (patched) — small POSIX
  portability edits plus macOS app-bundle path handling. The exact diff
  against upstream is committed as `core-vs-upstream-20251109.patch`.
- **`Makefile`**, **`mkapp.sh`**, **`fetch_upstream.sh`** (new) — clang/SDL2
  build for the three variants, .app bundling (icons converted from the
  upstream `.ico` resources), upstream source fetcher.

More technical detail: [README_SDL_MAC.md](README_SDL_MAC.md) (OSD
implementation notes) and [HANDOFF.md](HANDOFF.md) (development log).

## Status

Working: video (incl. Retina/HiDPI), audio, keyboard/joystick/mouse, floppy /
tape / hard disk mounting with drag-and-drop, save states, screenshots, WAV
recording, the debugger console (over the host terminal), printer output.
Stubbed: MIDI, video recording. Keyboard mapping is US-layout oriented.

## License

GPLv2-or-later, same as upstream CSCP (see `COPYING.txt`; upstream's own
license notes arrive with `fetch_upstream.sh` in `readme.txt` and `license/`).
The emulator cores and the machines they emulate are the work of Toshiya
Takeda and the contributors credited in the upstream readme.
