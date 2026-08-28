/*
	SDL OSD - MIDI (stub; no host MIDI backend yet)
*/

#include "osd.h"

#ifdef USE_MIDI
void OSD::initialize_midi()
{
}

void OSD::release_midi()
{
}

void OSD::send_to_midi(uint8_t data)
{
	// discard
}

bool OSD::recv_from_midi(uint8_t *data)
{
	return false;
}
#endif
