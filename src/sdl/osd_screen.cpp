/*
	SDL OSD - screen
*/

#include "osd.h"
#include "../fileio.h"

void OSD::initialize_screen()
{
	host_window_width = WINDOW_WIDTH;
	host_window_height = WINDOW_HEIGHT;
	host_window_mode = true;
	vm_screen_width = SCREEN_WIDTH;
	vm_screen_height = SCREEN_HEIGHT;
	vm_window_width = SCREEN_WIDTH;
	vm_window_height = SCREEN_HEIGHT;
	vm_window_width_aspect = WINDOW_WIDTH_ASPECT;
	vm_window_height_aspect = WINDOW_HEIGHT_ASPECT;
	first_draw_screen = false;
	screen_size_changed = true;
	now_record_video = false;
#ifdef USE_SCREEN_FILTER
	screen_skip_line = false;
#endif

	memset(&vm_screen_buffer, 0, sizeof(vm_screen_buffer));
	initialize_screen_buffer(&vm_screen_buffer, vm_screen_width, vm_screen_height, 0);

	window = NULL;
	renderer = NULL;
	texture = NULL;
	texture_width = texture_height = 0;

	// create the host window at the aspect-corrected size
	int win_w = vm_window_width_aspect;
	int win_h = vm_window_height_aspect;
	window = SDL_CreateWindow(_T(DEVICE_NAME),
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		win_w, win_h, SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
	if(window == NULL) {
		fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
		return;
	}
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if(renderer == NULL) {
		renderer = SDL_CreateRenderer(window, -1, 0);
	}
	if(renderer != NULL) {
		// NOTE: do NOT call SDL_RenderSetLogicalSize here. It installs a hidden
		// scale+letterbox transform on the renderer that (a) double-scales the
		// emulator rect against the manual hx/hy HiDPI math in render_frame, and
		// (b) offsets ImGui rendering vs. SDL-translated mouse input, so the menu
		// bar's hitbox ends up shifted down by the letterbox height. render_frame
		// composites in output pixels directly, so we keep the renderer 1:1.
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	}
}

void OSD::release_screen()
{
	release_screen_buffer(&vm_screen_buffer);
	if(texture != NULL) {
		SDL_DestroyTexture(texture);
		texture = NULL;
	}
	if(renderer != NULL) {
		SDL_DestroyRenderer(renderer);
		renderer = NULL;
	}
	if(window != NULL) {
		SDL_DestroyWindow(window);
		window = NULL;
	}
}

void OSD::initialize_screen_buffer(bitmap_t *buffer, int width, int height, int mode)
{
	release_screen_buffer(buffer);
	buffer->width = width;
	buffer->height = height;
	buffer->lpBmp = (scrntype_t*)calloc((size_t)width * height, sizeof(scrntype_t));
}

void OSD::release_screen_buffer(bitmap_t *buffer)
{
	if(buffer->lpBmp != NULL) {
		free(buffer->lpBmp);
		buffer->lpBmp = NULL;
	}
	buffer->width = buffer->height = 0;
}

double OSD::get_window_mode_power(int mode)
{
	if(mode + WINDOW_MODE_BASE == 2) {
		return 1.5;
	} else if(mode + WINDOW_MODE_BASE > 2) {
		return mode + WINDOW_MODE_BASE - 1;
	}
	return mode + WINDOW_MODE_BASE;
}

int OSD::get_window_mode_width(int mode)
{
	return (int)(vm_window_width * get_window_mode_power(mode) + 0.5);
}

int OSD::get_window_mode_height(int mode)
{
	return (int)(vm_window_height * get_window_mode_power(mode) + 0.5);
}

void OSD::set_host_window_size(int window_width, int window_height, bool window_mode)
{
	if(window_width != -1) {
		host_window_width = window_width;
	}
	if(window_height != -1) {
		host_window_height = window_height;
	}
	host_window_mode = window_mode;
	first_draw_screen = false;
}

void OSD::set_vm_screen_size(int screen_width, int screen_height, int window_width, int window_height, int window_width_aspect, int window_height_aspect)
{
	if(vm_screen_width != screen_width || vm_screen_height != screen_height) {
		if(window_width == -1) {
			window_width = screen_width;
		}
		if(window_height == -1) {
			window_height = screen_height;
		}
		if(window_width_aspect == -1) {
			window_width_aspect = window_width;
		}
		if(window_height_aspect == -1) {
			window_height_aspect = window_height;
		}
		vm_screen_width = screen_width;
		vm_screen_height = screen_height;
		vm_window_width = window_width;
		vm_window_height = window_height;
		vm_window_width_aspect = window_width_aspect;
		vm_window_height_aspect = window_height_aspect;
		screen_size_changed = true;
	}
	if(vm_screen_buffer.width != vm_screen_width || vm_screen_buffer.height != vm_screen_height) {
		initialize_screen_buffer(&vm_screen_buffer, vm_screen_width, vm_screen_height, 0);
	}
}

void OSD::set_vm_screen_lines(int lines)
{
}

scrntype_t* OSD::get_vm_screen_buffer(int y)
{
	return vm_screen_buffer.get_buffer(y);
}

void OSD::update_screen_texture()
{
	if(renderer == NULL) {
		return;
	}
	if(vm_screen_buffer.width != vm_screen_width || vm_screen_buffer.height != vm_screen_height) {
		initialize_screen_buffer(&vm_screen_buffer, vm_screen_width, vm_screen_height, 0);
	}
#ifdef USE_SCREEN_FILTER
	screen_skip_line = false;
#endif
	// let the virtual machine render into vm_screen_buffer
	// (may also change vm_screen_width/height via set_vm_screen_size)
	vm->draw_screen();
	if(vm_screen_buffer.width != vm_screen_width || vm_screen_buffer.height != vm_screen_height) {
		return;
	}
	if(texture == NULL || texture_width != vm_screen_width || texture_height != vm_screen_height) {
		if(texture != NULL) {
			SDL_DestroyTexture(texture);
		}
		texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888,
			SDL_TEXTUREACCESS_STREAMING, vm_screen_width, vm_screen_height);
		texture_width = vm_screen_width;
		texture_height = vm_screen_height;
	}
	if(texture != NULL && vm_screen_buffer.lpBmp != NULL) {
		SDL_UpdateTexture(texture, NULL, vm_screen_buffer.lpBmp,
			vm_screen_width * (int)sizeof(scrntype_t));
	}
	first_draw_screen = true;
	screen_size_changed = false;
}

void OSD::render_screen_to(int x, int y, int w, int h)
{
	if(renderer == NULL || texture == NULL) {
		return;
	}
	SDL_Rect src = { 0, 0, vm_window_width, vm_window_height };
	SDL_Rect dst = { x, y, w, h };
	SDL_RenderCopy(renderer, texture, &src, &dst);
}

int OSD::draw_screen()
{
	// standalone path (no GUI overlay): clear + aspect-fit + present
	if(renderer == NULL) {
		return 1;
	}
	update_screen_texture();
	int ow = 0, oh = 0;
	SDL_GetRendererOutputSize(renderer, &ow, &oh);
	SDL_RenderClear(renderer);
	render_screen_to(0, 0, ow, oh);
	SDL_RenderPresent(renderer);
	return 1;
}

void OSD::save_window_bmp(const char* path)
{
	// dump the actual VM framebuffer (reliable; renderer readback after
	// present is undefined on double-buffered backends)
	if(vm_screen_buffer.lpBmp == NULL) {
		return;
	}
	long nonzero = 0;
	int n = vm_screen_buffer.width * vm_screen_buffer.height;
	for(int i = 0; i < n; i++) {
		if((vm_screen_buffer.lpBmp[i] & 0xffffff) != 0) nonzero++;
	}
	fprintf(stderr, "[save_window_bmp] %dx%d, %ld/%d non-black pixels\n",
		vm_screen_buffer.width, vm_screen_buffer.height, nonzero, n);
	write_bitmap_to_file(&vm_screen_buffer, path);
}

void OSD::capture_screen()
{
	if(vm_screen_buffer.lpBmp == NULL) {
		return;
	}
	_TCHAR file_path[_MAX_PATH];
	create_date_file_path(file_path, _MAX_PATH, _T("bmp"));
	write_bitmap_to_file(&vm_screen_buffer, file_path);
}

bool OSD::start_record_video(int fps)
{
	// not yet implemented for SDL
	return false;
}

void OSD::stop_record_video()
{
	now_record_video = false;
}

void OSD::restart_record_video()
{
	bool tmp = now_record_video;
	stop_record_video();
	if(tmp) {
		start_record_video(rec_video_fps);
	}
}

void OSD::add_extra_frames(int extra_frames)
{
}

void OSD::write_bitmap_to_file(bitmap_t *bitmap, const _TCHAR *file_path)
{
	if(bitmap == NULL || bitmap->lpBmp == NULL) {
		return;
	}
	SDL_Surface* surf = SDL_CreateRGBSurfaceWithFormatFrom(
		bitmap->lpBmp, bitmap->width, bitmap->height,
		32, bitmap->width * (int)sizeof(scrntype_t), SDL_PIXELFORMAT_RGB888);
	if(surf != NULL) {
		SDL_SaveBMP(surf, file_path);
		SDL_FreeSurface(surf);
	}
}
