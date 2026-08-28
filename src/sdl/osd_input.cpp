/*
	SDL OSD - input (keyboard / joystick / mouse)
*/

#include "osd.h"

// ----------------------------------------------------------------------------
// SDL scancode -> Windows virtual-key code (CSP internal key space)
// ----------------------------------------------------------------------------
int osd_sdl_scancode_to_vk(int sc)
{
	switch(sc) {
	case SDL_SCANCODE_A: return 'A';
	case SDL_SCANCODE_B: return 'B';
	case SDL_SCANCODE_C: return 'C';
	case SDL_SCANCODE_D: return 'D';
	case SDL_SCANCODE_E: return 'E';
	case SDL_SCANCODE_F: return 'F';
	case SDL_SCANCODE_G: return 'G';
	case SDL_SCANCODE_H: return 'H';
	case SDL_SCANCODE_I: return 'I';
	case SDL_SCANCODE_J: return 'J';
	case SDL_SCANCODE_K: return 'K';
	case SDL_SCANCODE_L: return 'L';
	case SDL_SCANCODE_M: return 'M';
	case SDL_SCANCODE_N: return 'N';
	case SDL_SCANCODE_O: return 'O';
	case SDL_SCANCODE_P: return 'P';
	case SDL_SCANCODE_Q: return 'Q';
	case SDL_SCANCODE_R: return 'R';
	case SDL_SCANCODE_S: return 'S';
	case SDL_SCANCODE_T: return 'T';
	case SDL_SCANCODE_U: return 'U';
	case SDL_SCANCODE_V: return 'V';
	case SDL_SCANCODE_W: return 'W';
	case SDL_SCANCODE_X: return 'X';
	case SDL_SCANCODE_Y: return 'Y';
	case SDL_SCANCODE_Z: return 'Z';
	case SDL_SCANCODE_1: return '1';
	case SDL_SCANCODE_2: return '2';
	case SDL_SCANCODE_3: return '3';
	case SDL_SCANCODE_4: return '4';
	case SDL_SCANCODE_5: return '5';
	case SDL_SCANCODE_6: return '6';
	case SDL_SCANCODE_7: return '7';
	case SDL_SCANCODE_8: return '8';
	case SDL_SCANCODE_9: return '9';
	case SDL_SCANCODE_0: return '0';
	case SDL_SCANCODE_RETURN: return VK_RETURN;
	case SDL_SCANCODE_ESCAPE: return VK_ESCAPE;
	case SDL_SCANCODE_BACKSPACE: return VK_BACK;
	case SDL_SCANCODE_TAB: return VK_TAB;
	case SDL_SCANCODE_SPACE: return VK_SPACE;
	case SDL_SCANCODE_MINUS: return VK_OEM_MINUS;
	case SDL_SCANCODE_EQUALS: return VK_OEM_PLUS;
	case SDL_SCANCODE_LEFTBRACKET: return VK_OEM_4;
	case SDL_SCANCODE_RIGHTBRACKET: return VK_OEM_6;
	case SDL_SCANCODE_BACKSLASH: return VK_OEM_5;
	case SDL_SCANCODE_SEMICOLON: return VK_OEM_1;
	case SDL_SCANCODE_APOSTROPHE: return VK_OEM_7;
	case SDL_SCANCODE_GRAVE: return VK_OEM_3;
	case SDL_SCANCODE_COMMA: return VK_OEM_COMMA;
	case SDL_SCANCODE_PERIOD: return VK_OEM_PERIOD;
	case SDL_SCANCODE_SLASH: return VK_OEM_2;
	case SDL_SCANCODE_CAPSLOCK: return VK_CAPITAL;
	case SDL_SCANCODE_F1: return VK_F1;
	case SDL_SCANCODE_F2: return VK_F2;
	case SDL_SCANCODE_F3: return VK_F3;
	case SDL_SCANCODE_F4: return VK_F4;
	case SDL_SCANCODE_F5: return VK_F5;
	case SDL_SCANCODE_F6: return VK_F6;
	case SDL_SCANCODE_F7: return VK_F7;
	case SDL_SCANCODE_F8: return VK_F8;
	case SDL_SCANCODE_F9: return VK_F9;
	case SDL_SCANCODE_F10: return VK_F10;
	case SDL_SCANCODE_F11: return VK_F11;
	case SDL_SCANCODE_F12: return VK_F12;
	case SDL_SCANCODE_PRINTSCREEN: return VK_SNAPSHOT;
	case SDL_SCANCODE_SCROLLLOCK: return VK_SCROLL;
	case SDL_SCANCODE_PAUSE: return VK_PAUSE;
	case SDL_SCANCODE_INSERT: return VK_INSERT;
	case SDL_SCANCODE_HOME: return VK_HOME;
	case SDL_SCANCODE_PAGEUP: return VK_PRIOR;
	case SDL_SCANCODE_DELETE: return VK_DELETE;
	case SDL_SCANCODE_END: return VK_END;
	case SDL_SCANCODE_PAGEDOWN: return VK_NEXT;
	case SDL_SCANCODE_RIGHT: return VK_RIGHT;
	case SDL_SCANCODE_LEFT: return VK_LEFT;
	case SDL_SCANCODE_DOWN: return VK_DOWN;
	case SDL_SCANCODE_UP: return VK_UP;
	case SDL_SCANCODE_NUMLOCKCLEAR: return VK_NUMLOCK;
	case SDL_SCANCODE_KP_DIVIDE: return VK_DIVIDE;
	case SDL_SCANCODE_KP_MULTIPLY: return VK_MULTIPLY;
	case SDL_SCANCODE_KP_MINUS: return VK_SUBTRACT;
	case SDL_SCANCODE_KP_PLUS: return VK_ADD;
	case SDL_SCANCODE_KP_ENTER: return VK_RETURN;
	case SDL_SCANCODE_KP_1: return VK_NUMPAD1;
	case SDL_SCANCODE_KP_2: return VK_NUMPAD2;
	case SDL_SCANCODE_KP_3: return VK_NUMPAD3;
	case SDL_SCANCODE_KP_4: return VK_NUMPAD4;
	case SDL_SCANCODE_KP_5: return VK_NUMPAD5;
	case SDL_SCANCODE_KP_6: return VK_NUMPAD6;
	case SDL_SCANCODE_KP_7: return VK_NUMPAD7;
	case SDL_SCANCODE_KP_8: return VK_NUMPAD8;
	case SDL_SCANCODE_KP_9: return VK_NUMPAD9;
	case SDL_SCANCODE_KP_0: return VK_NUMPAD0;
	case SDL_SCANCODE_KP_PERIOD: return VK_DECIMAL;
	case SDL_SCANCODE_LCTRL: return VK_LCONTROL;
	case SDL_SCANCODE_LSHIFT: return VK_LSHIFT;
	case SDL_SCANCODE_LALT: return VK_LMENU;
	case SDL_SCANCODE_RCTRL: return VK_RCONTROL;
	case SDL_SCANCODE_RSHIFT: return VK_RSHIFT;
	case SDL_SCANCODE_RALT: return VK_RMENU;
	// JIS-ish extra keys
	case SDL_SCANCODE_INTERNATIONAL1: return VK_OEM_102; // ろ / _
	case SDL_SCANCODE_INTERNATIONAL3: return VK_OEM_5;   // ¥
	case SDL_SCANCODE_LANG1: return VK_KANA;             // かな / 英数
	case SDL_SCANCODE_LANG2: return VK_KANJI;            // 漢字
	default: return 0;
	}
}

void OSD::initialize_input()
{
	memset(key_status, 0, sizeof(key_status));
	for(int i = 0; i < 256; i++) {
		keycode_conv[i] = i;
	}
	key_shift_pressed = key_shift_released = false;
	key_caps_locked = false;
	lost_focus = false;

#ifdef USE_JOYSTICK
	memset(joy_status, 0, sizeof(joy_status));
	for(int i = 0; i < 4; i++) {
		joy_handle[i] = NULL;
	}
	SDL_InitSubSystem(SDL_INIT_JOYSTICK);
	joy_num = SDL_NumJoysticks();
	if(joy_num > 4) {
		joy_num = 4;
	}
	for(int i = 0; i < joy_num; i++) {
		joy_handle[i] = SDL_JoystickOpen(i);
	}
#endif
#ifdef USE_MOUSE
	memset(mouse_status, 0, sizeof(mouse_status));
	mouse_enabled = false;
#endif
}

void OSD::release_input()
{
#ifdef USE_JOYSTICK
	for(int i = 0; i < 4; i++) {
		if(joy_handle[i] != NULL) {
			SDL_JoystickClose(joy_handle[i]);
			joy_handle[i] = NULL;
		}
	}
#endif
}

void OSD::key_down(int code, bool extended, bool repeat)
{
#ifdef USE_AUTO_KEY
	if(!now_auto_key && !config.romaji_to_kana) {
#endif
		key_down_native(code, repeat);
#ifdef USE_AUTO_KEY
	}
#endif
}

void OSD::key_up(int code, bool extended)
{
#ifdef USE_AUTO_KEY
	if(!now_auto_key && !config.romaji_to_kana) {
#endif
		key_up_native(code);
#ifdef USE_AUTO_KEY
	}
#endif
}

void OSD::key_down_native(int code, bool repeat)
{
	bool keep_frames = false;

	if(!(code == VK_LSHIFT || code == VK_RSHIFT || code == VK_LCONTROL || code == VK_RCONTROL || code == VK_LMENU || code == VK_RMENU)) {
		code = keycode_conv[code];
	}
	if(code == 0) {
		return;
	}
	if(key_status[code] == 0 || keep_frames) {
		repeat = false;
	}
	key_status[code] = keep_frames ? KEY_KEEP_FRAMES : 0x80;

	uint8_t prev_shift = key_status[VK_SHIFT];
	uint8_t prev_control = key_status[VK_CONTROL];
	uint8_t prev_menu = key_status[VK_MENU];

	key_status[VK_SHIFT] = key_status[VK_LSHIFT] | key_status[VK_RSHIFT];
	key_status[VK_CONTROL] = key_status[VK_LCONTROL] | key_status[VK_RCONTROL];
	key_status[VK_MENU] = key_status[VK_LMENU] | key_status[VK_RMENU];

	if(code == VK_LSHIFT || code == VK_RSHIFT) {
		if(prev_shift == 0 && key_status[VK_SHIFT] != 0) {
			vm->key_down(VK_SHIFT, repeat);
		}
	} else if(code == VK_LCONTROL || code == VK_RCONTROL) {
		if(prev_control == 0 && key_status[VK_CONTROL] != 0) {
			vm->key_down(VK_CONTROL, repeat);
		}
	} else if(code == VK_LMENU || code == VK_RMENU) {
		if(prev_menu == 0 && key_status[VK_MENU] != 0) {
			vm->key_down(VK_MENU, repeat);
		}
	}
	vm->key_down(code, repeat);
}

void OSD::key_up_native(int code)
{
	if(!(code == VK_LSHIFT || code == VK_RSHIFT || code == VK_LCONTROL || code == VK_RCONTROL || code == VK_LMENU || code == VK_RMENU)) {
		code = keycode_conv[code];
	}
	if(code == 0 || key_status[code] == 0) {
		return;
	}
	if((key_status[code] &= 0x7f) != 0) {
		return;
	}
	vm->key_up(code);

	uint8_t prev_shift = key_status[VK_SHIFT];
	uint8_t prev_control = key_status[VK_CONTROL];
	uint8_t prev_menu = key_status[VK_MENU];

	key_status[VK_SHIFT] = key_status[VK_LSHIFT] | key_status[VK_RSHIFT];
	key_status[VK_CONTROL] = key_status[VK_LCONTROL] | key_status[VK_RCONTROL];
	key_status[VK_MENU] = key_status[VK_LMENU] | key_status[VK_RMENU];

	if(code == VK_LSHIFT || code == VK_RSHIFT) {
		if(prev_shift != 0 && key_status[VK_SHIFT] == 0) {
			vm->key_up(VK_SHIFT);
		}
	} else if(code == VK_LCONTROL || code == VK_RCONTROL) {
		if(prev_control != 0 && key_status[VK_CONTROL] == 0) {
			vm->key_up(VK_CONTROL);
		}
	} else if(code == VK_LMENU || code == VK_RMENU) {
		if(prev_menu != 0 && key_status[VK_MENU] == 0) {
			vm->key_up(VK_MENU);
		}
	}
}

void OSD::on_sdl_key(int sdl_scancode, bool down)
{
	int vk = osd_sdl_scancode_to_vk(sdl_scancode);
	if(vk == 0) {
		return;
	}
	if(down) {
		key_down(vk, false, false);
	} else {
		key_up(vk, false);
	}
}

void OSD::on_sdl_text(const char* utf8)
{
	// text input is currently driven through the key path; nothing extra
}

void OSD::update_input()
{
	// release / age keys
	if(lost_focus) {
		for(int i = 0; i < 256; i++) {
			if(key_status[i] & 0x80) {
				key_status[i] &= 0x7f;
				if(!key_status[i]) {
					vm->key_up(i);
				}
			}
		}
	} else {
		for(int i = 0; i < 256; i++) {
			if(key_status[i] & 0x7f) {
				key_status[i] = (key_status[i] & 0x80) | ((key_status[i] & 0x7f) - 1);
				if(!key_status[i]) {
					vm->key_up(i);
				}
			}
		}
	}
	lost_focus = false;
	key_status[0] = 0;

#ifdef USE_JOYSTICK
	memset(joy_status, 0, sizeof(joy_status));
	SDL_JoystickUpdate();
	for(int i = 0; i < joy_num && i < 4; i++) {
		SDL_Joystick* j = joy_handle[i];
		if(j == NULL) {
			continue;
		}
		int naxes = SDL_JoystickNumAxes(j);
		if(naxes >= 2) {
			int x = SDL_JoystickGetAxis(j, 0);
			int y = SDL_JoystickGetAxis(j, 1);
			if(y < -8000) joy_status[i] |= 0x01; // up
			if(y >  8000) joy_status[i] |= 0x02; // down
			if(x < -8000) joy_status[i] |= 0x04; // left
			if(x >  8000) joy_status[i] |= 0x08; // right
		}
		int nbtn = SDL_JoystickNumButtons(j);
		for(int b = 0; b < nbtn && b < 16; b++) {
			if(SDL_JoystickGetButton(j, b)) {
				joy_status[i] |= (1 << (b + 4));
			}
		}
		// hat as d-pad
		if(SDL_JoystickNumHats(j) > 0) {
			Uint8 hat = SDL_JoystickGetHat(j, 0);
			if(hat & SDL_HAT_UP)    joy_status[i] |= 0x01;
			if(hat & SDL_HAT_DOWN)  joy_status[i] |= 0x02;
			if(hat & SDL_HAT_LEFT)  joy_status[i] |= 0x04;
			if(hat & SDL_HAT_RIGHT) joy_status[i] |= 0x08;
		}
	}
#endif

#ifdef USE_MOUSE
	memset(mouse_status, 0, sizeof(mouse_status));
	if(mouse_enabled) {
		int dx = 0, dy = 0;
		Uint32 buttons = SDL_GetRelativeMouseState(&dx, &dy);
		mouse_status[0] = dx;
		mouse_status[1] = dy;
		mouse_status[2]  = (buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) ? 1 : 0;
		mouse_status[2] |= (buttons & SDL_BUTTON(SDL_BUTTON_RIGHT)) ? 2 : 0;
		mouse_status[2] |= (buttons & SDL_BUTTON(SDL_BUTTON_MIDDLE)) ? 4 : 0;
	}
#endif
}

#ifdef USE_MOUSE
void OSD::enable_mouse()
{
	if(!mouse_enabled) {
		SDL_SetRelativeMouseMode(SDL_TRUE);
	}
	mouse_enabled = true;
}

void OSD::disable_mouse()
{
	if(mouse_enabled) {
		SDL_SetRelativeMouseMode(SDL_FALSE);
	}
	mouse_enabled = false;
}

void OSD::toggle_mouse()
{
	if(mouse_enabled) {
		disable_mouse();
	} else {
		enable_mouse();
	}
}
#endif
