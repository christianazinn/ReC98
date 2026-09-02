#pragma option -zCSHARED

#include "th02/snd/snd.h"
#include "th02/snd/impl.hpp"

extern char snd_load_fn[PF_FN_LEN];

void snd_load(const char fn[PF_FN_LEN], snd_load_func_t func)
{
	int i;

#if (GAME == 3)
	// Replay Patch: MIDI is an explicit user selection. If its driver is not
	// active, reject BGM loads instead of sending the original FM data to PMD.
	if((func == SND_LOAD_SONG) && !snd_active) {
		return;
	}
#endif

	_asm { push ds; }

	_CX = sizeof(snd_load_fn);
	i = 0;
	fn_copy: {
		snd_load_fn[i] = fn[i];
		i++;
		asm { loop	fn_copy; }
	}

	asm { mov	ax, func; }
	if((_AX == SND_LOAD_SONG) && snd_midi_active) {
		_BX = 0;
		do {
			_BX++;
		} while(snd_load_fn[_BX]);
		snd_load_fn[_BX+0] = 'm';
		snd_load_fn[_BX+1] = 'd';
		snd_load_fn[_BX+2] = 0;
	}

	// DOS file open
	(char near *)(_DX) = snd_load_fn;
	_AX = 0x3D00;
	geninterrupt(0x21);
	_BX = _AX;
	// ZUN landmine: No error handling

	asm { mov	ax, func; }
	if((_AX == SND_LOAD_SONG) && snd_midi_active) {
		geninterrupt(MMD);
	} else {
		geninterrupt(PMD);
	}

	// DOS file read; song data address is in DS:DX
	_AX = 0x3F00;
	#if (GAME == 3)
	// Replay Patch: MMD has no buffer-size query. GAME.BAT reserves 40 KiB,
	// and the largest TH03 MIDI song exceeds the original 20 KiB PMD read.
	// Keep the original bound for FM songs and sound effects.
	_CX = (
		((func == SND_LOAD_SONG) && snd_midi_active) ? 0xA000 : snd_load_size()
	);
	#else
	_CX = snd_load_size();
	#endif
	geninterrupt(0x21);

	_asm { pop ds; }

	// DOS file close
	_AH = 0x3E;
	geninterrupt(0x21);
}
