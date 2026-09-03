#pragma option -zCMIDI_TEXT -zPMIDI_TEXT

#include "game/pf.h"
#include "th02/snd/snd.h"
#include "th03/snd/midi_diag.hpp"
#include "x86real.h"

extern char snd_load_fn[PF_FN_LEN];
extern "C" void snd_load_raw(
	const char fn[PF_FN_LEN], snd_load_func_t func
);

static void th03_snd_midi_load(const char fn[PF_FN_LEN])
{
	int i;
	int handle;
	#if defined(TH03_MIDI_DIAGNOSTICS)
	uint16_t read_result;
	bool16 read_failed;
	#endif

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
	#if defined(TH03_MIDI_DIAGNOSTICS)
	th03_midi_diag_log(T3MD_LOAD_ENTER, 0, 0);
	#endif

	// Replay Patch: MMD's timer interrupt reads from this same resident buffer.
	// Stop it before DOS overwrites that buffer, particularly across executable
	// transitions where a skipped screen can otherwise expose a partial event.
	_AX = (KAJA_SONG_STOP << 8);
	geninterrupt(MMD);

	(char near *)(_DX) = snd_load_fn;
	_AX = 0x3D00;
	geninterrupt(0x21);
	if(_FLAGS & 1) {
		#if defined(TH03_MIDI_DIAGNOSTICS)
		int open_error = _AX;
		#endif
		_asm { pop ds; }
		#if defined(TH03_MIDI_DIAGNOSTICS)
		th03_midi_diag_log(T3MD_LOAD_OPEN_FAILED, open_error, 0);
		#endif
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
	#if defined(TH03_MIDI_DIAGNOSTICS)
	read_result = _AX;
	read_failed = ((_FLAGS & 1) != 0);
	#endif
	_asm { pop ds; }
	_BX = handle;
	_AH = 0x3E;
	geninterrupt(0x21);
	#if defined(TH03_MIDI_DIAGNOSTICS)
	th03_midi_diag_log(
		(read_failed ? T3MD_LOAD_READ_FAILED : T3MD_LOAD_READ_DONE),
		read_result,
		handle
	);
	th03_midi_diag_log(T3MD_LOAD_DONE, 0, 0);
	#endif
}

void snd_load(const char fn[PF_FN_LEN], snd_load_func_t func)
{
	if(func == SND_LOAD_SONG) {
		if(!snd_active) {
			#if defined(TH03_MIDI_DIAGNOSTICS)
			th03_midi_diag_log(T3MD_LOAD_SKIPPED, func, 0);
			#endif
			return;
		}
		if(snd_midi_active) {
			th03_snd_midi_load(fn);
			return;
		}
	}
	#if defined(TH03_MIDI_DIAGNOSTICS)
	th03_midi_diag_log(T3MD_LOAD_PMD, func, 0);
	#endif
	snd_load_raw(fn, func);
	#if defined(TH03_MIDI_DIAGNOSTICS)
	th03_midi_diag_log(T3MD_LOAD_PMD_DONE, func, 0);
	#endif
}
