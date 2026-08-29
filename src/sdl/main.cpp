/*
	SDL OSD - application entry point and main loop
	(replaces win32/winmain.cpp)
*/

#include <SDL.h>
#include "../emu.h"
#include "../config.h"
#include "../fileio.h"
#include "osd.h"
#include "gui.h"
#include <unistd.h>
#include <cerrno>
#include <cstring>
#if defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

int osd_sdl_scancode_to_vk(int sc);

static EMU* emu = NULL;
static const char* g_capture_path = NULL;	// set to grab one composited frame to BMP

// compose one frame: emulator texture (aspect-fit) + ImGui menu/status overlay
static void render_frame(EMU* emu, OSD* osd)
{
	osd->update_screen_texture();
	gui_viewport_t v = gui_build();		// in ImGui points (= window points)
	SDL_Renderer* r = osd->get_renderer();
	if(r == NULL) {
		return;
	}
	// HiDPI: render EVERYTHING in window points. The ImGui SDLRenderer2 backend
	// emits vertices in points and relies on SDL_RenderSetScale to map them to
	// output pixels; if we don't set that scale, ImGui draws into the top-left
	// 1/dpi of the framebuffer (tiny menu, status bar stuck mid-window). So set
	// the DPI scale once and composite the emulator in points too (no manual
	// point->pixel math). Re-read each frame in case the window changes display.
	int win_w = 0, win_h = 0, out_w = 0, out_h = 0;
	SDL_GetWindowSize(osd->get_window(), &win_w, &win_h);
	SDL_GetRendererOutputSize(r, &out_w, &out_h);
	float dpix = (win_w > 0) ? (float)out_w / win_w : 1.0f;
	float dpiy = (win_h > 0) ? (float)out_h / win_h : 1.0f;
	SDL_RenderSetScale(r, dpix, dpiy);

	SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
	SDL_RenderClear(r);
	int aw = osd->get_vm_window_width_aspect();
	int ah = osd->get_vm_window_height_aspect();
	if(aw <= 0) aw = 4;
	if(ah <= 0) ah = 3;
	float sx = (float)v.w / aw, sy = (float)v.h / ah;
	float s = (sx < sy) ? sx : sy;
	int dw = (int)(aw * s), dh = (int)(ah * s);
	int dx = v.x + (v.w - dw) / 2, dy = v.y + (v.h - dh) / 2;
	osd->render_screen_to(dx, dy, dw, dh);
	gui_render();
	if(g_capture_path != NULL) {
		int ow = 0, oh = 0;
		SDL_GetRendererOutputSize(r, &ow, &oh);
		SDL_Surface* surf = SDL_CreateRGBSurfaceWithFormat(0, ow, oh, 24, SDL_PIXELFORMAT_RGB24);
		if(surf != NULL) {
			if(SDL_RenderReadPixels(r, NULL, SDL_PIXELFORMAT_RGB24, surf->pixels, surf->pitch) == 0) {
				SDL_SaveBMP(surf, g_capture_path);
			}
			SDL_FreeSurface(surf);
		}
		g_capture_path = NULL;
	}
	SDL_RenderPresent(r);
}

// push a path to the front of a recent-file history list (mirrors the win32
// UPDATE_HISTORY macro in winmain.cpp)
static void update_history(_TCHAR recent[][_MAX_PATH], const _TCHAR* path)
{
	int index = MAX_HISTORY - 1;
	for(int i = 0; i < MAX_HISTORY; i++) {
		if(_tcsicmp(recent[i], path) == 0) {
			index = i;
			break;
		}
	}
	for(int i = index; i > 0; i--) {
		my_tcscpy_s(recent[i], _MAX_PATH, recent[i - 1]);
	}
	my_tcscpy_s(recent[0], _MAX_PATH, path);
}

// route a path to the right device by extension. Mirrors win32 open_any_file
// (winmain.cpp ~3193) for the subsystems the X1 build enables: cart, floppy,
// hard disk, tape. Always loads drive 0 / unit 0, updates the recent list and
// the remembered directory, and auto-mounts d88 bank 1 into FD1 when present.
static void open_any_file(const _TCHAR* path)
{
	if(path == NULL || path[0] == _T('\0') || emu == NULL) {
		return;
	}
#if defined(USE_CART)
	if(check_file_extension(path, _T(".rom")) ||
	   check_file_extension(path, _T(".bin")) ||
	   check_file_extension(path, _T(".hex"))) {
		update_history(config.recent_cart_path[0], path);
		my_tcscpy_s(config.initial_cart_dir, _MAX_PATH, get_parent_dir(path));
		emu->open_cart(0, path);
		return;
	}
#endif
#if defined(USE_FLOPPY_DISK)
	if(check_file_extension(path, _T(".d88")) ||
	   check_file_extension(path, _T(".d8e")) ||
	   check_file_extension(path, _T(".d77")) ||
	   check_file_extension(path, _T(".1dd")) ||
	   check_file_extension(path, _T(".td0")) ||
	   check_file_extension(path, _T(".imd")) ||
	   check_file_extension(path, _T(".dsk")) ||
	   check_file_extension(path, _T(".nfd")) ||
	   check_file_extension(path, _T(".fdi")) ||
	   check_file_extension(path, _T(".hdm")) ||
	   check_file_extension(path, _T(".hd5")) ||
	   check_file_extension(path, _T(".hd4")) ||
	   check_file_extension(path, _T(".hdb")) ||
	   check_file_extension(path, _T(".dd9")) ||
	   check_file_extension(path, _T(".dd6")) ||
	   check_file_extension(path, _T(".tfd")) ||
	   check_file_extension(path, _T(".xdf")) ||
	   check_file_extension(path, _T(".2d" )) ||
	   check_file_extension(path, _T(".sf7")) ||
	   check_file_extension(path, _T(".img")) ||
	   check_file_extension(path, _T(".ima")) ||
	   check_file_extension(path, _T(".vfd"))) {
		update_history(config.recent_floppy_disk_path[0], path);
		my_tcscpy_s(config.initial_floppy_disk_dir, _MAX_PATH, get_parent_dir(path));
		emu->open_floppy_disk(0, path, 0);
#if USE_FLOPPY_DISK >= 2
		if(emu->d88_file[0].bank_num > 1) {
			emu->open_floppy_disk(1, path, 1);
		}
#endif
		return;
	}
#endif
#if defined(USE_HARD_DISK)
	if(check_file_extension(path, _T(".thd")) ||
	   check_file_extension(path, _T(".nhd")) ||
	   check_file_extension(path, _T(".hdi")) ||
	   check_file_extension(path, _T(".hdd"))) {
		update_history(config.recent_hard_disk_path[0], path);
		my_tcscpy_s(config.initial_hard_disk_dir, _MAX_PATH, get_parent_dir(path));
		emu->open_hard_disk(0, path);
		return;
	}
#endif
#if defined(USE_TAPE)
	if(check_file_extension(path, _T(".wav")) ||
	   check_file_extension(path, _T(".cas")) ||
	   check_file_extension(path, _T(".cmt")) ||
	   check_file_extension(path, _T(".t88")) ||
	   check_file_extension(path, _T(".tap")) ||
	   check_file_extension(path, _T(".t77"))) {
		update_history(config.recent_tape_path[0], path);
		my_tcscpy_s(config.initial_tape_dir, _MAX_PATH, get_parent_dir(path));
		emu->play_tape(0, path);
		return;
	}
#endif
}

int main(int argc, char* argv[])
{
	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK | SDL_INIT_EVENTS) != 0) {
		fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
		return 1;
	}

	// load config
	load_config(create_local_path(_T("%s.ini"), _T(CONFIG_NAME)));

	// create emulation core (creates the OSD, window, audio)
	emu = new EMU();
	emu->set_host_window_size(WINDOW_WIDTH, WINDOW_HEIGHT, true);

	OSD* osd = emu->get_osd();

	// initialize the ImGui front-end and make room for menu + status bars
	gui_init(osd->get_window(), osd->get_renderer(), emu);
	gui_rom_check();	// arm the missing-ROM overlay if no IPL image exists
	if(osd->get_window() != NULL) {
		SDL_SetWindowSize(osd->get_window(), WINDOW_WIDTH_ASPECT, WINDOW_HEIGHT_ASPECT + 24 + 26);
	}

	// command-line media
	if(argc > 1) {
		open_any_file(argv[1]);
	}

	// optional auto-screenshot for headless verification
	const char* autoshot = getenv("EX1_AUTOSHOT");
	int frame_count = 0;

	// main loop (timing modeled on win32/winmain.cpp)
	bool running = true;
	Uint32 next_time = 0;
	bool prev_skip = false;
	int skip_frames = 0;
	int accum = 0;

	while(running) {
		SDL_Event ev;
		while(SDL_PollEvent(&ev)) {
			gui_process_event(&ev);
			bool gui_kbd = gui_want_keyboard();
			switch(ev.type) {
			case SDL_QUIT:
				running = false;
				break;
			case SDL_KEYDOWN:
				if(!gui_kbd) osd->on_sdl_key(ev.key.keysym.scancode, true);
				break;
			case SDL_KEYUP:
				if(!gui_kbd) osd->on_sdl_key(ev.key.keysym.scancode, false);
				break;
			case SDL_WINDOWEVENT:
				if(ev.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
					emu->key_lost_focus();
				}
				break;
			case SDL_DROPFILE:
				open_any_file(ev.drop.file);
				SDL_free(ev.drop.file);
				break;
			}
		}
		if(osd->request_terminate) {
			running = false;
		}
		if(gui_restart_requested()) {
			running = false;
		}
		if(!running) {
			break;
		}

		// drive the machine
		emu->run();
		if(autoshot != NULL && ++frame_count == 300) {
			g_capture_path = autoshot;
			render_frame(emu, osd);
			running = false;
			break;
		}

		// timing control
		bool now_skip = (config.full_speed || emu->is_frame_skippable()) &&
		                !emu->is_video_recording() && !emu->is_sound_recording();
		if((prev_skip && !now_skip) || next_time == 0) {
			next_time = SDL_GetTicks();
		}
		if(!now_skip) {
			accum += emu->get_frame_interval();
			int interval = accum >> 10;
			accum -= interval << 10;
			next_time += interval;
		}
		prev_skip = now_skip;

		if(next_time > SDL_GetTicks()) {
			render_frame(emu, osd);
			skip_frames = 0;
			Uint32 current_time = SDL_GetTicks();
			if((int)(next_time - current_time) >= 10) {
				SDL_Delay(next_time - current_time);
			}
		} else if(++skip_frames > (int)emu->get_frame_rate()) {
			render_frame(emu, osd);
			skip_frames = 0;
			next_time = SDL_GetTicks();
		}
	}

	// save config and shut down
	save_config(create_local_path(_T("%s.ini"), _T(CONFIG_NAME)));
	bool restart = gui_restart_requested();
	gui_shutdown();
	delete emu;
	emu = NULL;
	SDL_Quit();

	// missing-ROM screen requested a restart: re-exec ourselves so the VM
	// reloads ROMs from scratch (they are only read at VM construction)
	if(restart) {
#if defined(__APPLE__)
		char exe_path[_MAX_PATH];
		uint32_t exe_size = _MAX_PATH;
		if(_NSGetExecutablePath(exe_path, &exe_size) == 0) {
			execv(exe_path, argv);
		}
#endif
		execv(argv[0], argv);
		fprintf(stderr, "restart failed: %s\n", strerror(errno));
		return 1;
	}
	return 0;
}
