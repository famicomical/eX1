/*
	SDL OSD - lifecycle
*/

#include "osd.h"

void OSD::initialize(int rate, int samples)
{
	request_terminate = false;
	initialize_console();
	initialize_input();
	initialize_screen();
	initialize_sound(rate, samples);
#ifdef USE_MIDI
	initialize_midi();
#endif
}

void OSD::release()
{
	release_console();
	release_input();
	release_screen();
	release_sound();
#ifdef USE_MIDI
	release_midi();
#endif
}

void OSD::power_off()
{
	request_terminate = true;
}

void OSD::suspend()
{
	mute_sound();
}

void OSD::restore()
{
}

void OSD::lock_vm()
{
	lock_count++;
}

void OSD::unlock_vm()
{
	if(--lock_count <= 0) {
		force_unlock_vm();
	}
}

void OSD::force_unlock_vm()
{
	lock_count = 0;
}

void OSD::sleep(uint32_t ms)
{
	SDL_Delay(ms);
}

#ifdef USE_DEBUGGER
void OSD::start_waiting_in_debugger()
{
}

void OSD::finish_waiting_in_debugger()
{
}

void OSD::process_waiting_in_debugger()
{
	// keep the window alive while the debugger CLI blocks the main thread
	SDL_PumpEvents();
}
#endif
