# eX1 (Sharp X1turbo) macOS/SDL2 port — session handoff

## How to build & run
```sh
cd /Users/rmb/Desktop/gamestuff/eX1/source
make                       # -> build/ex1turbo  (needs: brew install sdl2)
make clean                 # wipe build/
cd build && ./ex1turbo     # ROMs already in build/ (IPL.ROM, FNT*.X1, KANJI2.ROM->KANJI.ROM symlink)
./ex1turbo disk.d88        # boot with a floppy; or drag-drop onto window
```
Headless screenshot for verification: `EX1_AUTOSHOT=/tmp/x.bmp ./ex1turbo`
(captures the emulator framebuffer after 300 frames; note: SDL_RenderReadPixels does
NOT capture the ImGui overlay, so a black-looking shot doesn't mean the UI is broken —
the live window renders the menus fine.)

## Current state: WORKS, with 2 open UI bugs
The emulator boots and runs (IPL → "looking for a program from FD0"). The Dear ImGui
menu bar + status bar are fully implemented and render live. Build is GREEN.

### Implemented this session (the menu/UI clone)
- Vendored Dear ImGui 1.92.9 in `src/sdl/imgui/` (core + imgui_impl_sdl2 + imgui_impl_sdlrenderer2).
- `src/sdl/gui.cpp` / `gui.h`: full menu bar cloning the Windows menus —
  Control, FD0–3, CMT, HD0–3, Device (Boot/Keyboard/Sound/Display/Printer/Serial), Sound,
  Screen(Capture). "Host" menu intentionally omitted (per user). All handlers wired to
  `emu->`/`config` per the win32 WM_COMMAND spec, with checkmarks/radios/enable-state.
  File dialogs use native macOS panels via `osascript` (choose file / choose file name).
  Status bar: FD: + HD: per-drive activity LEDs (off/red/green from
  `emu->is_floppy_disk_accessed()` + `floppy_disk_indicator_color()` / `is_hard_disk_accessed()`)
  and `CMT: <emu->get_tape_message(0)>`.
- `src/sdl/main.cpp`: ImGui event pump, keyboard gated by `gui_want_keyboard()`,
  `render_frame()` composites emulator + GUI; window grown to fit menu+status bars.
- `src/sdl/osd_screen.cpp`: split `draw_screen()` into `update_screen_texture()` +
  `render_screen_to(x,y,w,h)` so the GUI can overlay. Added `get_renderer()`,
  `get_texture()`, `get_vm_screen_width/height()` to `osd.h`.

## RESOLVED (this session)
Both UI bugs below were one root cause: `SDL_RenderSetLogicalSize(renderer, ...)` in
`osd_screen.cpp` (was line ~49). It locks the renderer to 640x480 logical; the window grows
to 640x530 for the menu+status bars, so the renderer letterboxes+centers all output (push
down ~25pt ≈ menu bar height) AND double-scales against the manual hx/hy HiDPI math in
`render_frame`. That single hidden transform produced BOTH the 2x overflow ("scale wrong")
and the menu hitbox-shifted-down ("mouse lag") symptoms. FIX: deleted that one line.
`render_frame`'s existing manual compositing is correct once the transform is gone.
Verified via OS window screengrab: X1 fills width (no overflow), menu bar flush under title
bar, menu items respond at their drawn location. The handoff's planned ImGui::Image rewrite
was rejected: over-engineered and would not have fixed the hitbox (it never removed the
logical-size call). PRESENTVSYNC left on (lag was a symptom of the offset, not real latency).

### Follow-up: HiDPI menu/status sizing (also fixed)
After deleting the logical-size call, ImGui rendered tiny in the top-left quarter (menu
shrunk, status bar stuck mid-window). Cause: the SDLRenderer2 backend emits vertices in
POINTS and relies on SDL_RenderSetScale to reach the 2x framebuffer. FIX in
`main.cpp render_frame`: call `SDL_RenderSetScale(r, out/win, out/win)` each frame and
composite the emulator in POINTS (dropped the hx/hy point->pixel math). Everything now lives
in one coordinate space (window points; SDL scales to pixels). Verified against the Windows
reference: full-size menu bar, status bar at the bottom, X1 aspect-fit.

## OLD BUG NOTES (kept for context; both fixed above)
### 1. X1 display scale is wrong (HiDPI) — PRIORITY
Window uses `SDL_WINDOW_ALLOW_HIGHDPI` (needed so ImGui mouse hit-testing is accurate on
Retina). But `render_frame()` in `main.cpp` manually aspect-fits + scales the emulator rect
from ImGui points → renderer pixels (`hx/hy = out/win`). That math double-scales: the X1
is drawn ~2x and overflows. Debug confirmed: win=640x530, rout=1280x1060, fbscale=2,
emulator dest computed 1280x960 but renders too large on screen.

**Planned clean fix (was mid-implementation):** stop manual compositing. Render the X1
as an `ImGui::Image` inside a borderless full-area ImGui window between the menu and status
bars. ImGui then handles layout + HiDPI uniformly for emulator/menu/status — no manual
point↔pixel math. `OSD::get_texture()` is already added for this.
Steps:
  - In `gui.cpp`, add `gui_draw_emulator()` called inside `gui_build()` after the menu bar:
    create a no-decoration/no-pad window at (0, menu_h) sized to fill down to the status bar;
    aspect-fit using `vm_window_width_aspect:height_aspect` (640:480); draw
    `ImGui::Image((ImTextureID)(intptr_t)emu->get_osd()->get_texture(), size, uv0, uv1)`
    where uv1 = (vm_window_width/vm_screen_width, vm_window_height/vm_screen_height).
    (texture is vm_screen_width×height; visible region is vm_window_width×height.)
  - In `main.cpp render_frame()`: delete the hx/hy pixel math and the `osd->render_screen_to(...)`
    call. Just: `osd->update_screen_texture(); gui_build(); SDL_RenderClear; gui_render(); present;`
  - `get_texture()` returns the SDL_Texture*; sdlrenderer2 backend uses SDL_Texture* as ImTextureID.

### 2. Mouse is laggy
Renderer created with `SDL_RENDERER_PRESENTVSYNC` AND the loop also `SDL_Delay`s to hit the
frame interval → ~1–2 frames of stacked latency. Fix: drop `SDL_RENDERER_PRESENTVSYNC` in
`osd_screen.cpp` (renderer creation, ~line 44) and rely on the existing frame-timing
`SDL_Delay` in `main.cpp`. Re-test responsiveness.

## Notes / gotchas
- Source files are ISO-8859/CRLF: use `grep -a`, `LC_ALL=C` to avoid multibyte errors.
- Win-isms are shimmed in `src/sdl/compat/` (io.h, malloc.h, windefs.h). Core portability
  edits live in `common.h`, `common.cpp`, `emu.h` (see git-less notes in `README_SDL_MAC.md`).
- The full win32→action spec for every menu ID is encoded directly in `gui.cpp`; the
  source of truth is `src/win32/winmain.cpp` (WM_COMMAND ~674-1115, macros 1143-1389,
  update_control_menu 1555, update_tape_menu 1756, status bar 2594/2652).
- After the two fixes: remove leftover `EX1_AUTOSHOT` capture hook in `main.cpp` if desired
  (harmless), and consider nearest-neighbor texture scaling for crisp X1 pixels
  (`SDL_SetTextureScaleMode(texture, SDL_ScaleModeNearest)` in `update_screen_texture`).
```
