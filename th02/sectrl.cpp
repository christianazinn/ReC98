#pragma option -zCT2SE_TEXT -zPT2SE_TEXT -G-

#include "th02/snd/snd.h"
#include "th02/language.hpp"

extern "C" void DEFCONV snd_se_play_raw(int se);
extern "C" void snd_se_update_raw(void);

void DEFCONV snd_se_play(int se)
{
	if(t2_sfx_enabled()) {
		snd_se_play_raw(se);
	}
}

void snd_se_update(void)
{
	if(t2_sfx_enabled()) {
		snd_se_update_raw();
	} else {
		snd_se_reset();
	}
}
