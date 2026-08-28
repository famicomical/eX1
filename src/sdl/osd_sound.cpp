/*
	SDL OSD - sound (queued audio, audio-driven vm timing)
*/

#include "osd.h"
#include "../fileio.h"

void OSD::initialize_sound(int rate, int samples)
{
	sound_rate = rate;
	sound_samples = samples;
	sound_available = sound_started = sound_muted = now_record_sound = false;
	rec_sound_buffer_ptr = 0;
	audio_dev = 0;

	SDL_AudioSpec want, have;
	SDL_zero(want);
	want.freq = sound_rate;
	want.format = AUDIO_S16SYS;
	want.channels = 2;
	want.samples = 2048;
	want.callback = NULL;	// use SDL_QueueAudio

	audio_dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
	if(audio_dev == 0) {
		fprintf(stderr, "SDL_OpenAudioDevice failed: %s\n", SDL_GetError());
		return;
	}
	sound_available = true;
}

void OSD::release_sound()
{
	if(audio_dev != 0) {
		SDL_CloseAudioDevice(audio_dev);
		audio_dev = 0;
	}
	stop_record_sound();
}

void OSD::update_sound(int* extra_frames)
{
	*extra_frames = 0;
	sound_muted = false;

	if(!sound_available) {
		return;
	}
	// start playback
	if(!sound_started) {
		SDL_PauseAudioDevice(audio_dev, 0);
		sound_started = true;
	}

	// keep roughly two buffers queued; refill (and drive the vm) when low
	const Uint32 bytes_per_buffer = (Uint32)sound_samples * 2 * sizeof(int16_t);
	Uint32 queued = SDL_GetQueuedAudioSize(audio_dev);
	if(queued >= bytes_per_buffer * 2) {
		return;
	}

	// sound buffer must be updated -> drives the virtual machine
	uint16_t* sound_buffer = vm->create_sound(extra_frames);

	if(now_record_sound) {
		if(sound_samples > rec_sound_buffer_ptr) {
			int samples = sound_samples - rec_sound_buffer_ptr;
			int length = samples * (int)sizeof(uint16_t) * 2; // stereo
			rec_sound_fio->Fwrite(sound_buffer + rec_sound_buffer_ptr * 2, length, 1);
			rec_sound_bytes += length;
		}
		rec_sound_buffer_ptr = 0;
	}

	if(sound_buffer != NULL) {
		SDL_QueueAudio(audio_dev, sound_buffer, bytes_per_buffer);
	}
}

void OSD::mute_sound()
{
	if(sound_available && !sound_muted) {
		SDL_ClearQueuedAudio(audio_dev);
	}
	sound_muted = true;
}

void OSD::stop_sound()
{
	if(sound_available && sound_started) {
		SDL_PauseAudioDevice(audio_dev, 1);
		SDL_ClearQueuedAudio(audio_dev);
		sound_started = false;
	}
}

void OSD::start_record_sound()
{
	if(!now_record_sound) {
		create_date_file_path(sound_file_path, _MAX_PATH, _T("wav"));
		rec_sound_fio = new FILEIO();
		if(rec_sound_fio->Fopen(sound_file_path, FILEIO_WRITE_BINARY)) {
			wav_header_t wav_header;
			wav_chunk_t wav_chunk;
			memset(&wav_header, 0, sizeof(wav_header));
			memset(&wav_chunk, 0, sizeof(wav_chunk));
			rec_sound_fio->Fwrite(&wav_header, sizeof(wav_header), 1);
			rec_sound_fio->Fwrite(&wav_chunk, sizeof(wav_chunk), 1);

			rec_sound_bytes = 0;
			rec_sound_buffer_ptr = vm->get_sound_buffer_ptr();
			now_record_sound = true;
		} else {
			delete rec_sound_fio;
		}
	}
}

void OSD::stop_record_sound()
{
	if(now_record_sound) {
		if(rec_sound_bytes == 0) {
			rec_sound_fio->Fclose();
			FILEIO::RemoveFile(sound_file_path);
		} else {
			wav_header_t wav_header;
			wav_chunk_t wav_chunk;

			memcpy(wav_header.riff_chunk.id, "RIFF", 4);
			wav_header.riff_chunk.size = rec_sound_bytes + sizeof(wav_header) + sizeof(wav_chunk) - 8;
			memcpy(wav_header.wave, "WAVE", 4);
			memcpy(wav_header.fmt_chunk.id, "fmt ", 4);
			wav_header.fmt_chunk.size = 16;
			wav_header.format_id = 1;
			wav_header.channels = 2;
			wav_header.sample_bits = 16;
			wav_header.sample_rate = sound_rate;
			wav_header.block_size = wav_header.channels * wav_header.sample_bits / 8;
			wav_header.data_speed = wav_header.sample_rate * wav_header.block_size;

			memcpy(wav_chunk.id, "data", 4);
			wav_chunk.size = rec_sound_bytes;

			rec_sound_fio->Fseek(0, FILEIO_SEEK_SET);
			rec_sound_fio->Fwrite(&wav_header, sizeof(wav_header), 1);
			rec_sound_fio->Fwrite(&wav_chunk, sizeof(wav_chunk), 1);
			rec_sound_fio->Fclose();
		}
		delete rec_sound_fio;
		now_record_sound = false;
	}
}

void OSD::restart_record_sound()
{
	bool tmp = now_record_sound;
	stop_record_sound();
	if(tmp) {
		start_record_sound();
	}
}
