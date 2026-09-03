#pragma option -zCPLAYFLD_TEXT -zPmain_01

#include "libs/master.lib/pc98_gfx.hpp"
#include "platform.h"
#include "th02/snd/snd.h"
#include "th03/core/initexit.h"
#include "th03/formats/cfg.hpp"
#include "th03/main/enemy/enemy.hpp"
#include "th03/main/replay.hpp"
#include "th03/main/round.hpp"
#include "th03/resident.hpp"
#include "th03/snd/options.hpp"

extern "C" const unsigned char aCOul[];
extern "C" const char aGameft_bft[];
extern "C" const char aOp[];
extern "C" const char arg0[];

extern farfunc_t_near farfp_20F20;

extern "C" uint8_t pascal near sub_9778(void);
extern "C" void pascal near round_startup(void);
extern "C" void pascal near sub_A21F(void);

int pascal GameExecl(const char *binary_fn);

extern "C" void far main_entry(void)
{
	register int route;

	game_init_main(aCOul);
	if(!cfg_load_resident_ptr()) {
		return;
	}

	// Keep the original MAIN startup's single snd_determine_mode() call depth.
	// MAINL already performed the real driver probe immediately before this
	// executable handoff; repeating it here perturbs stack bytes later read by
	// original gameplay code.
	snd_midi_possible = static_cast<bool>(
		resident->unused_3[T3_SND_MMD_HANDOFF_RES_INDEX]
	);
	snd_midi_active = (
		(resident->bgm_mode == SND_BGM_MIDI) && snd_midi_possible
	);
	snd_interrupt_if_midi = (snd_midi_active ? MMD : PMD);
	if(
		(resident->bgm_mode != SND_BGM_OFF) ||
		th03_snd_se_enabled()
	) {
		snd_determine_mode();
	}
	th03_snd_process_apply();
	if(
		(resident->bgm_mode == SND_BGM_OFF) ||
		(
			(resident->bgm_mode == SND_BGM_MIDI) &&
			!snd_midi_possible
		)
	) {
		snd_active = false;
	}

	gaiji_backup();
	gaiji_entry_bfnt(aGameft_bft);
	round_startup();
	farfp_20F20();
	replay_session_start();
	replay_round_start();

round_loop:
	PaletteTone = 100;
	palette_show();
	route = sub_9778();
	replay_route(route);
	resident->rand = round_frame;

	if(route == 1) {
		replay_round_reset_seed_capture();
		sub_A21F();
		replay_round_start();
		goto round_loop;
	}

	replay_finish(route);
	enemy_formations_free();
	snd_kaja_func(KAJA_SONG_STOP, 0);

	if(route != 0) {
		goto exit_to_mainl;
	}
	_asm {
		push	ds;
		push	offset aOp;
		jmp 	short game_execl;
	}

exit_to_mainl:
	resident->story_stage++;
	_asm {
		push	ds;
		push	offset arg0;
	}
game_execl:
	_asm { nop; push cs; call near ptr GameExecl; }
}

// Keeps every later original MAIN_01 contribution at its accepted offset.
#pragma codestring \
	"\x90\x90\x90\x90\x90\x90\x90\x90" \
	"\x90\x90\x90\x90\x90\x90\x90\x90"
