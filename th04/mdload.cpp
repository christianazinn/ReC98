#pragma option -zCMDLOAD_TEXT -zPMDLOAD_TEXT -G-

#include "th04/snd/snd.h"
#include "th02/snd/mmd_load.hpp"

extern "C" void pascal snd_load_raw(const char fn[PF_FN_LEN], snd_load_func_t func);

void pascal snd_load(const char fn[PF_FN_LEN], snd_load_func_t func)
{
	if((func == SND_LOAD_SONG) && (snd_bgm_mode == SND_BGM_MIDI)) {
		char midi_fn[13];
		if(!mmd_song_filename(fn, midi_fn, (GAME == 5)) ||
			!mmd_song_load(midi_fn)) {
			// Suppress PLAY after a missing, oversized, or partial song load.
			// The saved option is untouched; effects retain their own mode.
			snd_bgm_mode = SND_BGM_OFF;
		}
		return;
	}
	snd_load_raw(fn, func);
}
