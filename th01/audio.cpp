#pragma option -zCT1AUDIO_TEXT -zPT1AUDIO_TEXT -G-

#include "th01/snd/mdrv2.h"
#include "th01/snd/audio.hpp"
#include "th01/language.hpp"
#include "th01/hardware/frmdelay.h"
#include "th02/snd/mmd_load.hpp"

void mdrv2_bgm_load_raw(const char *fn);
void mdrv2_bgm_play_raw(void);
void mdrv2_bgm_stop_raw(void);
void mdrv2_bgm_fade_out_nonblock_raw(void);
void mdrv2_bgm_fade_out_block_raw(void);
void mdrv2_bgm_fade_in_raw(void);
void mdrv2_se_play_raw(int se);

static bool bgm_disabled;
static bool midi_mode;
static bool midi_song_loaded;
static bool midi_paused;
static uint16_t midi_paused_seg;

static void midi_func(uint8_t function, uint8_t parameter)
{
	if(mmd_pause_driver()) {
		_AX = ((function << 8) | parameter);
		geninterrupt(MMD);
	}
}

void far t1_audio_configure(int mode)
{
	bgm_disabled = (mode == 0);
	// Mode 2 is reserved for driver preparation; the Option count stays 2.
	midi_mode = (mode == 2);
	midi_paused = false;
	midi_song_loaded = false;
	if(bgm_disabled || midi_mode) {
		mdrv2_bgm_stop_raw();
	}
	if(!midi_mode) {
		midi_func(KAJA_SONG_STOP, 0);
	} else if(!mmd_pause_driver()) {
		bgm_disabled = true;
	}
}

bool far t1_audio_bgm_enabled(void)
{
	return !bgm_disabled;
}

bool far t1_audio_bgm_is_midi(void)
{
	return midi_mode;
}

bool16 far t1_midi_music_pause(void)
{
	if(midi_paused) {
		return true;
	}
	if(!midi_mode || bgm_disabled || !midi_song_loaded) {
		return false;
	}
	midi_paused = mmd_transport_pause(midi_paused_seg);
	return midi_paused;
}

void far t1_midi_music_resume(void)
{
	if(midi_paused) {
		midi_paused = false;
		if(midi_mode && !bgm_disabled && midi_song_loaded) {
			mmd_transport_resume(midi_paused_seg);
		}
	}
}

void mdrv2_bgm_load(const char *fn)
{
	if(midi_mode) {
		char midi_fn[13];
		midi_paused = false;
		midi_song_loaded = false;
		midi_func(KAJA_SONG_STOP, 0);
		if(!bgm_disabled && mmd_song_filename(fn, midi_fn, false)) {
			midi_song_loaded = mmd_song_load(midi_fn);
		}
	} else if(!bgm_disabled) {
		mdrv2_bgm_load_raw(fn);
	}
}

void mdrv2_bgm_play(void)
{
	if(midi_mode) {
		if(!bgm_disabled && midi_song_loaded) {
			midi_paused = false;
			midi_func(KAJA_SONG_PLAY, 0);
		}
	} else if(!bgm_disabled) {
		mdrv2_bgm_play_raw();
	}
}

void mdrv2_bgm_stop(void)
{
	if(midi_mode) {
		midi_paused = false;
		midi_func(KAJA_SONG_STOP, 0);
	} else {
		mdrv2_bgm_stop_raw();
	}
}

void mdrv2_bgm_fade_out_nonblock(void)
{
	if(midi_mode) {
		if(!bgm_disabled && midi_song_loaded) {
			midi_func(KAJA_SONG_FADE, 16);
		}
	} else if(!bgm_disabled) {
		mdrv2_bgm_fade_out_nonblock_raw();
	}
}

void mdrv2_bgm_fade_out_block(void)
{
	if(midi_mode) {
		if(!bgm_disabled && midi_song_loaded) {
			midi_func(KAJA_SONG_FADE, 16);
			for(unsigned frames = 0; frames < 256; frames++) {
				midi_func(KAJA_GET_VOLUME, 0);
				if(_AL == 255) {
					break;
				}
				frame_delay(1);
			}
			mdrv2_bgm_stop();
		}
	} else if(!bgm_disabled) {
		mdrv2_bgm_fade_out_block_raw();
	}
}

void mdrv2_bgm_fade_in(void)
{
	if(midi_mode) {
		// MMD has no fade-in service. No TH01 scene calls this legacy API.
		mdrv2_bgm_play();
	} else if(!bgm_disabled) {
		mdrv2_bgm_fade_in_raw();
	}
}

void mdrv2_se_play(int se)
{
	if(t1_sfx_enabled()) {
		mdrv2_se_play_raw(se);
	}
}
