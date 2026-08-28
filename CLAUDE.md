# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

macOS/SDL2 port of the Sharp X1turbo emulator from the Common Source Code Project (CSCP), a multi-machine retro PC emulator codebase. The upstream tree contains sources and Visual Studio projects for dozens of machines (`vc++2008/`, `vc++2017/`, `src/vm/<machine>/`), but this repo's active target is only **eX1turbo** built via the root `Makefile` against a new SDL OSD layer in `src/sdl/`. `HANDOFF.md` documents the port's current state and recent fixes — read it before touching `src/sdl/`.

## Build & run

```sh
brew install sdl2          # one-time prerequisite
make                       # all three variants -> build/ex1, build/ex1turbo, build/ex1turboz
make x1turbo               # single variant (x1 | x1turbo | x1turboz)
make clean                 # remove objects + binaries only; ROMs in build/ survive
cd build && ./ex1turbo     # ROMs must be in the CWD (build/ already has them)
./ex1turbo disk.d88        # boot with floppy in drive 0; drag-drop also mounts files
```

- No tests or lint. Verification is running the emulator: it should boot the IPL and print "looking for a program from FD0".
- Headless verification: `EX1_AUTOSHOT=/tmp/x.bmp ./ex1turbo` captures the framebuffer after 300 frames. Note `SDL_RenderReadPixels` does NOT capture the ImGui overlay — a black shot doesn't mean the UI is broken.
- ROMs (IPL.ROM, FNT*.X1, KANJI.ROM, etc.) are copyrighted, loaded from the working directory at launch. Config saves to `x1turbo.ini` in the CWD on exit.

## Architecture

Three layers, joined by `emu.h`/`emu.cpp`:

1. **Shared core** (`src/common.*`, `config.*`, `emu.*`, `fileio.*`, `fifo.*`, `debugger.cpp`) — machine-independent glue. Contains small portability edits for POSIX (documented in `README_SDL_MAC.md`).
2. **VM layer** (`src/vm/`) — device emulation. Generic chips at the top level (`z80.cpp`, `mb8877.cpp` FDC, `hd46505.cpp` CRTC, `ay_3_891x.cpp` PSG, `ym2151.cpp`, `fmgen/` FM synth...), machine-specific wiring in `src/vm/x1/` (`x1.cpp` is the machine definition connecting devices via the event/IO bus).
3. **OSD layer** — platform backend behind the `OSD` class in `osd.h`. The original win32/DirectX one lives in `src/win32/` (reference only, not built); the mac port is `src/sdl/` (`osd_screen/sound/input/console/printer/midi.cpp`, `main.cpp` event loop, `gui.cpp` Dear ImGui menu/status bar). Dear ImGui 1.92.9 is vendored in `src/sdl/imgui/`.

Machine and backend selection is purely by preprocessor defines: `-D_USE_SDL -D_RGB888` plus one machine macro per variant (`-D_X1`, `-D_X1TURBO`, or `-D_X1TURBOZ`; see `src/vm/x1/x1.h` for what each enables — `_X1TURBO_FEATURE` gates the turbo hardware). The Makefile builds each variant into its own object dir (`build/<variant>/`), sharing only the ImGui objects; `CORE_SRCS` is transcribed from `vc++2017/x1turbo.vcxproj` minus the win32 OSD (plain x1 drops `z80dma.cpp`, matching `x1.vcxproj`) — adding a core file means adding it there. Machine-conditional code in `src/sdl/` must use the feature macros from `x1.h` (`USE_HARD_DISK`, `USE_DRIVE_TYPE`, `USE_KEYBOARD_TYPE`, ...), not the machine macros. Plain X1 needs its own `IPLROM.X1` (4KB) — the turbo IPL in `build/` won't boot it; the UI still comes up.

### SDL port specifics

- `src/sdl/compat/` shims win32-isms (`io.h`, `malloc.h`, `windefs.h` for `VK_*` etc.) so the shared core compiles unmodified where possible.
- `gui.cpp` clones the Windows menus; the source of truth for every menu ID's behavior is `src/win32/winmain.cpp` (WM_COMMAND handling ~lines 674–1115, menu-update functions ~1555+, status bar ~2594). File dialogs use native macOS panels via `osascript`. The "Host" menu is intentionally omitted.
- Rendering: everything lives in one coordinate space (window points; SDL scales to pixels). `render_frame()` in `main.cpp` calls `SDL_RenderSetScale(r, out/win, ...)` each frame and composites emulator + ImGui in points. Do NOT reintroduce `SDL_RenderSetLogicalSize` in `osd_screen.cpp` — it caused double-scaling and shifted menu hitboxes (see HANDOFF.md "RESOLVED").
- Keyboard input to the VM is gated by `gui_want_keyboard()` so ImGui gets keys when a menu is active.

## Gotchas

- Core source files are ISO-8859 encoded with CRLF line endings. Use `grep -a` and `LC_ALL=C` to avoid multibyte errors; preserve the existing encoding/line endings when editing them. New `src/sdl/` files are ordinary UTF-8/LF.
- Git tracks only the port files (Makefile, docs, `src/sdl/`, edited core files); the upstream CSCP tree is intentionally untracked and `build/` is ignored.
- `vc++2008/`, `vc++2017/`, `src/win32/`, `src/vm/<other machines>/`, and `tool/` are upstream baggage kept for reference — don't build or modify them for the mac port.
