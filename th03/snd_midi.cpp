#pragma option -zCMIDI_TEXT -zPMIDI_TEXT

#include "game/pf.h"
#include "th02/snd/snd.h"
#include "x86real.h"

extern char snd_load_fn[PF_FN_LEN];
extern "C" void snd_load_raw(
	const char fn[PF_FN_LEN], snd_load_func_t func
);

static void th03_snd_midi_load(const char fn[PF_FN_LEN])
{
	int i;
	int handle;

	_asm { push ds; }
	_CX = sizeof(snd_load_fn);
	i = 0;
	fn_copy: {
		snd_load_fn[i] = fn[i];
		i++;
		asm { loop fn_copy; }
	}
	_BX = 0;
	do {
		_BX++;
	} while(snd_load_fn[_BX]);
	snd_load_fn[_BX + 0] = 'm';
	snd_load_fn[_BX + 1] = 'd';
	snd_load_fn[_BX + 2] = 0;

	// Replay Patch: MMD's timer interrupt reads from this same resident buffer.
	// Stop it before DOS overwrites that buffer, particularly across executable
	// transitions where a skipped screen can otherwise expose a partial event.
	_AX = (KAJA_SONG_STOP << 8);
	geninterrupt(MMD);

	(char near *)(_DX) = snd_load_fn;
	_AX = 0x3D00;
	geninterrupt(0x21);
	if(_FLAGS & 1) {
		_asm { pop ds; }
		snd_active = false;
		return;
	}
	handle = _AX;
	_AX = SND_LOAD_SONG;
	geninterrupt(MMD);
	_BX = handle;
	_AX = 0x3F00;
	_CX = (17 * 1024);
	geninterrupt(0x21);
	_asm { pop ds; }
	_BX = handle;
	_AH = 0x3E;
	geninterrupt(0x21);
}

void snd_load(const char fn[PF_FN_LEN], snd_load_func_t func)
{
	if(func == SND_LOAD_SONG) {
		if(!snd_active) {
			return;
		}
		if(snd_midi_active) {
			th03_snd_midi_load(fn);
			return;
		}
	}
	snd_load_raw(fn, func);
}
