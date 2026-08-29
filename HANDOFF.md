# eX1 (Sharp X1 family) macOS/SDL2 port — session handoff

Updated 2026-08-29 (publishing-prep session). Previous handoff (menu/UI HiDPI
bugs) is fully resolved; see git history and the notes at the bottom.

## Current state: WORKS

All three variants build, boot the IPL ("IPL is looking for a program from FD0"),
and run with the full ImGui menu/status bar. Build is GREEN, repo is git-tracked.

```sh
make                # all three -> build/ex1, build/ex1turbo, build/ex1turboz
make x1turbo        # one variant (x1 | x1turbo | x1turboz)
make app            # -> build/eX1.app, eX1turbo.app, eX1turboZ.app
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
   (`_NSGetExecutablePath` contains `.app/Contents/MacOS/`) and uses the
   shared `~/Library/Application Support/eX1/` for ROMs+config (Finder CWD
   is `/`); terminal launches keep CWD behavior. ROM filenames are distinct
   per machine (IPLROM.X1 4KB vs IPL.ROM 32KB) and .ini names differ, so one
   dir serves all variants.

## ROMs (copyrighted, never committed)

- `build/` has a working turbo set: IPL.ROM (32KB), FNT0808/0816/1616.X1,
  KANJI2.ROM + `KANJI.ROM` symlink, and `IPLROM.X1` (4KB plain-X1 IPL, copied
  from `~/Desktop/gamestuff/retropc/HXC Sharp X1/アプリ/２ｄからD88/mameroms/x1/ipl.x1`).
  Backup source set: `~/Desktop/gamestuff/Xmil106RSV0190/DumpListEditorVer082/ex1/`.
- Bundled apps read from the shared `~/Library/Application Support/eX1/`
  (flattened from per-binary subdirs after they proved confusing); it holds a
  full set including both IPLs, and all three .apps boot from it.
- Plain X1 needs the 4KB `IPLROM.X1` (loader tries it first, falls back to
  `IPL.ROM` but reads only `IPL_ROM_FILE_SIZE`, so the turbo 32KB image won't boot it).

## Publishing prep (2026-08-29 session)

- **SDL2 bundled into .apps**: mkapp.sh copies the linked libSDL2 into
  `Contents/Frameworks/`, re-points the binary (`install_name_tool`), signs
  the dylib. Verified: `otool -L` shows `@executable_path/...`, app runs.
- **fetch_upstream.sh**: downloads Takeda's source.7z, extracts (needs
  7zz/7z/7za), rsyncs into the repo excluding all git-tracked files (patched
  core stays). Pinned sha256 warns (doesn't fail) if upstream updates the
  archive in place. End-to-end verified: fresh clone -> fetch -> make -> run.
- **README.md** (front page: build/ROMs/usage/changes/license) +
  **COPYING.txt** (GPLv2 — note: root `LICENSE` file impossible, collides
  with upstream `license/` dir on case-insensitive APFS).
- **Missing-ROM screen** (gui.cpp `rom_screen()`): overlays when no usable
  IPL at startup (wrong-size IPL.ROM counts as unusable — catches 32KB turbo
  image on plain X1). Checklist of ROM files rescanned 1/sec, Open Folder
  button, Restart re-execs the process (ROMs only load at VM construction),
  Quit pushes SDL_QUIT. Armed via `gui_rom_check()` in main.cpp.
- core-vs-upstream patch regenerated with clean a/ b/ paths (old one leaked
  absolute scratchpad paths).

## Known rough edges / possible next steps

- GitHub repo not yet created/pushed (no remote). Branch is `master`; consider
  renaming to `main` before publishing.
- Live upstream source.7z (downloaded 2026-08-29) matches the pinned sha256;
  archive is dated newer than 11/9/2025 but core files are identical.
- .app niceties not done: Dock-drop file-open verification, version string
  hardcoded 2025.11.9 in mkapp.sh.
- Plain-X1 menu guards visually confirmed in the bundle build (no Boot
  Device/Keyboard, Display = Scanline only).
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
