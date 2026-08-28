/*
	SDL OSD - console (debugger CLI over the host terminal)
*/

#include "osd.h"
#include <unistd.h>
#include <fcntl.h>

void OSD::initialize_console()
{
	console_count = 0;
	console_closed = false;
	console_code_page = 65001; // UTF-8
	console_input_buffer[0] = _T('\0');
}

void OSD::release_console()
{
	close_console();
}

void OSD::open_console(int width, int height, const _TCHAR* title)
{
	if(console_count++ == 0) {
		console_closed = false;
		// make stdin non-blocking so read_console_input can poll
		int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
		fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
		if(title != NULL) {
			printf("\n=== %s ===\n", title);
			fflush(stdout);
		}
	}
}

void OSD::close_console()
{
	if(console_count > 0 && --console_count == 0) {
		set_console_text_attribute(OSD_CONSOLE_RED | OSD_CONSOLE_GREEN | OSD_CONSOLE_BLUE);
	}
}

unsigned int OSD::get_console_code_page()
{
	return console_code_page;
}

void OSD::set_console_code_page(unsigned int cp)
{
	console_code_page = cp;
}

void OSD::get_console_cursor_position(int *x, int *y)
{
	if(x != NULL) *x = 0;
	if(y != NULL) *y = 0;
}

void OSD::set_console_cursor_position(int x, int y)
{
}

void OSD::set_console_text_attribute(unsigned short attr)
{
	// map CSP console color bits to ANSI SGR
	int fg = 30;
	if(attr & OSD_CONSOLE_RED)   fg += 1;
	if(attr & OSD_CONSOLE_GREEN) fg += 2;
	if(attr & OSD_CONSOLE_BLUE)  fg += 4;
	bool bold = (attr & OSD_CONSOLE_INTENSITY) != 0;
	printf("\033[%d;%dm", bold ? 1 : 0, fg);
	fflush(stdout);
}

void OSD::write_console(const _TCHAR* buffer, unsigned int length)
{
	if(buffer == NULL) {
		return;
	}
	fwrite(buffer, sizeof(_TCHAR), length, stdout);
	fflush(stdout);
}

void OSD::write_console_char(const char* buffer, unsigned int length)
{
	if(buffer == NULL) {
		return;
	}
	fwrite(buffer, 1, length, stdout);
	fflush(stdout);
}

void OSD::write_console_wchar(const wchar_t* buffer, unsigned int length)
{
	if(buffer == NULL) {
		return;
	}
	for(unsigned int i = 0; i < length; i++) {
		putchar((int)buffer[i]);
	}
	fflush(stdout);
}

int OSD::read_console_input(_TCHAR* buffer, unsigned int length)
{
	if(buffer == NULL || length == 0) {
		return 0;
	}
	ssize_t n = read(STDIN_FILENO, buffer, length);
	if(n <= 0) {
		return 0;
	}
	return (int)n;
}

bool OSD::is_console_key_pressed(int vk)
{
	// host-terminal cannot poll virtual keys; report not pressed
	return false;
}

bool OSD::is_console_closed()
{
	return console_closed;
}

void OSD::close_debugger_console()
{
	console_closed = true;
	close_console();
}
