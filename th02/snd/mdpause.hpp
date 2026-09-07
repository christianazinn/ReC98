#ifndef TH02_SND_MDPAUSE_HPP
#define TH02_SND_MDPAUSE_HPP

#include "platform.h"

bool16 far snd_bgm_pause(void);
void far snd_bgm_resume(void);

#ifdef MIDI_PAUSE_IMPLEMENTATION

#include "th02/snd/mmd_pause.hpp"

static uint16_t midi_pause_seg;
static bool midi_pause_active;
static bool fm_pause_active;

bool16 far snd_bgm_pause(void)
{
	if(midi_pause_active || fm_pause_active) {
		return true;
	}
	if(!snd_bgm_active()) {
		return false;
	}
	if(snd_bgm_is_fm()) {
		snd_kaja_func(PMD_SONG_PAUSE, 0);
		fm_pause_active = true;
		return true;
	}
	midi_pause_active = mmd_transport_pause(midi_pause_seg);
	return midi_pause_active;
}

void far snd_bgm_resume(void)
{
	if(fm_pause_active) {
		fm_pause_active = false;
		if(snd_bgm_active() && snd_bgm_is_fm()) {
			snd_kaja_func(PMD_SONG_RESUME, 0);
		}
	}
	if(!midi_pause_active) {
		return;
	}
	midi_pause_active = false;
	if(snd_bgm_active() && !snd_bgm_is_fm()) {
		mmd_transport_resume(midi_pause_seg);
	}
}

#endif
#endif
