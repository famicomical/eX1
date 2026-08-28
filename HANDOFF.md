# eX1 (Sharp X1 family) macOS/SDL2 port — session handoff

Updated 2026-08-29. Previous handoff (menu/UI HiDPI bugs) is fully resolved;
see git history and the notes at the bottom.

## Current state: WORKS

All three variants build, boot the IPL ("IPL is looking for a program from FD0"),
and run with the full ImGui menu/status bar. Build is GREEN, repo is git-tracked.

```sh
make                # all three -> build/ex1, build/ex1turbo, build/ex1turboz
make x1turbo        # one variant (x1 | x1turbo | x1turboz)
make app            # -> build/eX1.app, eX1turbo.app, eX1turboZ.app (needs ImageMagick)
make clean          # objects/binaries/bundles only — ROMs in build/ survive
```

Headless verification: `EX1_AUTOSHOT=/tmp/x.bmp ./ex1turbo` (framebuffer after
300 frames; captures the ImGui overlay too).

## What was done in this session block (see `git log`)

1. **Git snapshot** (`0bd2a9b`…): repo initialized, tracking ONLY port work —
   `Makefile`, `mkapp.sh`, docs, `src/sdl/`, and the 3 edited core files.
   Upstream CSCP tree deliberately untracked; `build/` ignored.
2. **Verified against pristine upstream** (Takeda's source.7z, same 11/9/2025
   version): only `common.cpp`, `common.h`, `emu.h` differ from upstream —
   `core-vs-upstream-20251109.patch` (committed) is the exact diff. Everything
   else the port adds lives in `src/sdl/`.
3. **Multi-variant build** (`7346cbc`): Makefile builds x1 / x1turbo / x1turboz
   via per-variant object dirs (`build/<variant>/`) + machine macro
   (`-D_X1`/`-D_X1TURBO`/`-D_X1TURBOZ`); plain x1 drops `z80dma.cpp` per
   upstream x1.vcxproj; ImGui objects shared. gui.cpp menus needed
   `USE_DRIVE_TYPE`/`USE_KEYBOARD_TYPE` guards (turbo-only config fields).
4. **Plain-X1 menu fix** (`913ffb8`): High Resolution/Standard radios gated on
   `_X1TURBO_FEATURE` — `config.monitor_type` only feeds the turbo dipswitch
   (port 0x1ff0); upstream's `USE_MONITOR_TYPE` is defined unconditionally so
   it can't be the guard.
5. **.app bundling** (`c0854d6`): `mkapp.sh` + `make app`. Icons generated from
   `src/res/<variant>.ico` (32x32) → `.icns` via ImageMagick nearest-neighbor
   upscale. Info.plist sets `NSHighResolutionCapable` (required for Retina in a
   bundle) + `.d88`/`.cmt` document types. Ad-hoc codesigned.
   **Path fix in `common.cpp`**: `get_application_path()` detects a bundle
   (`_NSGetExecutablePath` contains `.app/Contents/MacOS/`) and uses
   `~/Library/Application Support/eX1/<binary name>/` for ROMs+config
   (Finder CWD is `/`); terminal launches keep CWD behavior.

## ROMs (copyrighted, never committed)

- `build/` has a working turbo set: IPL.ROM (32KB), FNT0808/0816/1616.X1,
  KANJI2.ROM + `KANJI.ROM` symlink, and `IPLROM.X1` (4KB plain-X1 IPL, copied
  from `~/Desktop/gamestuff/retropc/HXC Sharp X1/アプリ/２ｄからD88/mameroms/x1/ipl.x1`).
  Backup source set: `~/Desktop/gamestuff/Xmil106RSV0190/DumpListEditorVer082/ex1/`.
- Bundled apps read from `~/Library/Application Support/eX1/<binary name>/` —
  only `ex1turbo/` is populated so far; `ex1/` and `ex1turboz/` need ROMs
  copied in before their .apps show anything.
- Plain X1 needs the 4KB `IPLROM.X1` (loader tries it first, falls back to
  `IPL.ROM` but reads only `IPL_ROM_FILE_SIZE`, so the turbo 32KB image won't boot it).

## Known rough edges / possible next steps

- Binaries link MacPorts SDL2 (`/opt/local/lib/libSDL2-2.0.0.dylib`) — CLAUDE.md
  says brew but sdl2-config resolves to MacPorts on this machine. Fine locally;
  a distributable .app would need the dylib bundled+`install_name_tool`'d or
  SDL2 linked statically.
- Distribution plan (discussed, not built): publish repo + builder script
  (`make app`) rather than shipping .apps — avoids Gatekeeper/notarization,
  ROMs can't ship anyway, GPLv2+ (CSCP license, readme.txt §License) satisfied.
- .app niceties not done: no "drop ROMs here" first-run message (blank screen
  if ROM dir is empty), no Dock-drop file-open verification, version string
  hardcoded 2025.11.9 in mkapp.sh.
- IDE clangd flags diagnostics in `src/sdl/` (missing SDL.h etc.) — noise; it
  lacks the build's include paths/defines. The real build is clean.

## Layout / gotchas (also in CLAUDE.md)

- Machine-conditional code in `src/sdl/` must use feature macros from
  `src/vm/x1/x1.h` (`USE_HARD_DISK`, `USE_SCANLINE`, ...) or
  `_X1TURBO_FEATURE` — never bare machine macros where a feature macro exists.
- Core files ISO-8859 + CRLF: `grep -a`, `LC_ALL=C`; patch byte-exact (this
  session used a python script to keep CRLF intact in common.cpp).
- Menu behavior source of truth: `src/win32/winmain.cpp` (WM_COMMAND ~674–1115).
- Do NOT reintroduce `SDL_RenderSetLogicalSize` in `osd_screen.cpp`
  (historic double-scale/hitbox bug — see git history of this file).
