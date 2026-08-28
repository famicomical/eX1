# eX1 (Sharp X1turbo) — macOS / SDL2 port

Native macOS build of the Common Source Code Project X1turbo emulator,
using a new SDL2 OSD layer (`src/sdl/`) in place of the win32/DirectX layer.

## Build

Requires Homebrew SDL2 and Xcode command-line clang.

```sh
brew install sdl2
cd source
make            # produces build/ex1turbo
```

`make clean` removes `build/`.

## ROMs (you must supply these)

The emulator loads Sharp X1turbo ROM images from the **current working
directory** at launch. They are copyrighted by Sharp and not included.
Expected filenames (same as the Windows build):

- `IPL.ROM`      — boot ROM (required)
- `FNT0808.X1`, `FNT0816.X1`, `FNT1616.X1` — fonts
- `ANK8.ROM`, `ANK16.ROM` — ANK fonts
- `KANJI.ROM`   — kanji ROM
- `CZ8RB.ROM`   — (optional) extension
- `EMM.ROM`     — (optional) EMM

Put them next to where you run the binary.

## Run

```sh
cd /folder/with/roms
/path/to/source/build/ex1turbo            # boot to IPL
/path/to/source/build/ex1turbo disk.d88   # boot with a floppy in drive 0
```

Drag-and-drop a `.d88`/`.cmt`/`.rom` onto the window to mount it.
Config is saved to `x1turbo.ini` in the working directory on exit.

## Status / notes

- Implemented: video (SDL renderer/texture), audio (SDL queued audio,
  audio-driven VM timing), keyboard (SDL scancode → CSP virtual-key map),
  joystick, mouse, debugger console over the host terminal, basic printer
  bitmap rendering, screenshot (`capture_screen` → BMP), WAV sound record.
- Stubbed: MIDI (discarded), video recording, movie player / video capture
  (those features are off for X1turbo anyway).
- Key mapping is US-layout oriented; JIS-specific keys are approximate.

## Port internals

- `src/sdl/` — the SDL OSD (`osd*.cpp`, `main.cpp`, `osd.h`).
- `src/sdl/compat/` — `io.h`, `malloc.h`, `windefs.h` shims that satisfy
  win32-isms (`<io.h>`, `<malloc.h>`, `VK_*`, `LONG_PTR`, console attrs).
- Small portability fixes in the shared core: `common.h` (`_ftprintf`→
  `fprintf`, `size_t` min/max overloads), `common.cpp` (non-Qt POSIX
  `get_application_path`, `vswprintf` arity, includes), `emu.h`
  (`pthread_t` debugger thread id for the SDL OSD).
