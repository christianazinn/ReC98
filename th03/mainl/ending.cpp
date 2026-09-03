#pragma codeseg STAFF_TEXT group_01

#include <process.h>
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/core/initexit.h"
#include "th02/hardware/frmdelay.h"
#include "th03/cutscene/cutscene.hpp"
#include "th03/formats/cdg.h"
#include "th03/formats/pi.hpp"
#include "th03/hiscore/regist.hpp"
#include "th03/mainl/replay.hpp"
#include "th03/pixel_capture.hpp"
#include "th03/replay_handoff.hpp"
#include "th03/resident.hpp"
#include "th03/scorefile.hpp"
#include "th03/snd/snd.h"

#pragma warn -aus

extern const char gameover_bgm_fn[];
extern char far *ending_script_fn;
extern const char extra_ending_script_fn[];
extern char binary_op_fn[];

void near staffroll_and_verdict_animate(void);

static void near ending_exit_to_op(void)
{
	scorestat_exit_checkpoint();
	text_clear();
	gaiji_restore();
	game_exit();
	execl(binary_op_fn, binary_op_fn, nullptr);
}

void near gameover_bgm_play_and_fade(void)
{
	snd_kaja_func(KAJA_SONG_STOP, 0);
	snd_load(gameover_bgm_fn, SND_LOAD_SONG);
	snd_kaja_func(KAJA_SONG_PLAY, 0);
	palette_black_in(1);
	// snd_get_song_measure() uses TH03's quarter-note MMD presentation clock.
	// OVER.M retains regular four-quarter-note measures.
	snd_delay_until_measure((snd_midi_active ? 12 : 3), 64);
	palette_black_out(1);
	snd_kaja_func(KAJA_SONG_STOP, 0);
}

void near ending_after_regist(void)
{
	if(
		(resident->rem_credits == 3) &&
		(resident->playchar_paletted[0].v < TO_OPTIONAL_PALETTED(PLAYCHAR_CHIYURI))
	) {
		graph_accesspage(1);
		graph_clear();
		graph_accesspage(0);
		graph_clear();
		graph_showpage(0);
		cutscene_script_load(extra_ending_script_fn);
		cutscene_animate();
		cutscene_script_free();
	}

	ending_exit_to_op();
}

void near ending_staff_and_regist(void)
{
	t3pix_scene_set(T3PIX_SCENE_ENDING);
	char playchar_id;

	if(mainl_replay_clear_playback_finish()) {
		ending_exit_to_op();
		return;
	}

	cdg_free(0);
	cdg_free(1);
	cdg_free(2);
	pi_free(0);

	playchar_id = resident->playchar_paletted[0].char_id_16();
	asm {
		cmp 	byte ptr [bp - 1], 10
		jl  	ending_script_ones_digit
		les 	bx, ending_script_fn
		mov 	al, es:[bx + 1]
		push	ax
		mov 	al, [bp - 1]
		cbw
		mov 	bx, 10
		cwd
		idiv	bx
		pop 	dx
		db  	02h, 0D0h
		mov 	bx, word ptr ending_script_fn
		mov 	es:[bx + 1], dl
		mov 	al, [bp - 1]
		cbw
		mov 	bx, 10
		cwd
		idiv	bx
		mov 	[bp - 1], dl

	ending_script_ones_digit:
		les 	bx, ending_script_fn
		mov 	al, [bp - 1]
		add 	es:[bx + 2], al
	}

	PaletteTone = 0;
	palette_show();
	frame_delay(96);
	graph_accesspage(0);
	graph_showpage(0);
	graph_clear();
	graph_show();
	cutscene_script_load(ending_script_fn);
	cutscene_animate();
	cutscene_script_free();
	staffroll_and_verdict_animate();
	resident->story_stage = STAGE_ALL;
	regist_menu();
	if(mainl_replay_finish(RUER_COMPLETE, T3_REPLAY_RES_MODE_SAVE_PROMPT_CLEAR)) {
		ending_exit_to_op();
		return;
	}
	ending_after_regist();
}

#pragma codeseg
