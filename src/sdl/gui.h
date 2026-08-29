/*
	SDL OSD - Dear ImGui front-end (menu bar + status bar)
	Clones the win32 menu/status UI for the Sharp X1turbo.
*/
#ifndef _SDL_GUI_H_
#define _SDL_GUI_H_

#include <SDL.h>

class EMU;

struct gui_viewport_t {
	int x, y, w, h;	// emulator drawing rect (between menu bar and status bar)
};

bool gui_init(SDL_Window* window, SDL_Renderer* renderer, EMU* emu);
void gui_shutdown();
void gui_process_event(const SDL_Event* ev);
bool gui_want_keyboard();			// true while ImGui is capturing the keyboard
// begin a frame, build menu + status bars, return the emulator viewport
gui_viewport_t gui_build();
void gui_render();					// ImGui::Render + draw

// missing-ROM screen: call gui_rom_check() once after gui_init to arm the
// overlay if no usable IPL image exists; gui_restart_requested() goes true
// when the user clicks Restart there (caller re-execs the process)
void gui_rom_check();
bool gui_restart_requested();

#endif // _SDL_GUI_H_
