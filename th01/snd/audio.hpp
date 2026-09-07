#ifndef TH01_SND_AUDIO_HPP
#define TH01_SND_AUDIO_HPP
#include "platform.h"

void far t1_audio_configure(int mode);
bool far t1_audio_bgm_enabled(void);
bool far t1_audio_bgm_is_midi(void);
bool16 far t1_midi_music_pause(void);
void far t1_midi_music_resume(void);

#endif
