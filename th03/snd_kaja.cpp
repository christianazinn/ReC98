#if defined(TH03_MIDI_DIAGNOSTICS)

#pragma option -zCSHARED

#include "th02/snd/snd.h"
#include "th02/snd/impl.hpp"
#include "th03/snd/midi_diag.hpp"

int16_t DEFCONV snd_kaja_interrupt(int16_t ax)
{
	int16_t ret;
	if(!snd_bgm_active()) {
		th03_midi_diag_log(T3MD_KAJA_SKIPPED, ax, 0);
		return _AX;
	}

	th03_midi_diag_log(T3MD_KAJA_ENTER, ax, 0);
	_AX = ax;
	if(snd_bgm_is_fm()) {
		geninterrupt(PMD);
	} else {
		geninterrupt(MMD);
	}
	ret = _AX;
	th03_midi_diag_log(T3MD_KAJA_DONE, ax, ret);
	return ret;
}

#else
#include "th02/snd/kajaint.cpp"
#endif
