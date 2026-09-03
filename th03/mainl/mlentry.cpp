#pragma codeseg CUTSCENE_TEXT group_01

#include <process.h>
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/v_colors.hpp"
#include "th03/core/initexit.h"
#define cdg_free_all cdg_free_all_default_distance
#include "th03/formats/cdg.h"
#undef cdg_free_all
#include "th03/formats/cfg.hpp"
#include "th03/formats/pi.hpp"
#include "th03/hiscore/regist.hpp"
#include "th03/cutscene/cutscene.hpp"
#include "th03/mainl/replay.hpp"
#include "th03/replay_handoff.hpp"
#include "th03/resident.hpp"
#include "th03/snd/snd.h"
#include "th03/snd/midi_diag.hpp"
#include "th03/snd/options.hpp"

#pragma warn -aus

extern const char mainl_pf_fn[];
extern const char mainl_gaiji_fn[];
extern const char stage_splash_yume_efc_fn[];
extern const char mainl_win_bgm_fn[];
extern char mainl_binary_op_fn[];
extern char mainl_binary_main_fn[];
extern char far *win_cutscene_script_fn;
extern PlaycharPaletted playchar[PLAYER_COUNT];

extern "C" void far hflip_lut_generate(void);
extern "C" void near pascal cdg_free_all(void);
extern "C" void far execl_raw(void);

void near win_load(void);
void near win_animate_and_wait(void);
int near sub_9887(void);
void near stage_splash_load(void);
void near stage_splash_show_and_wait(void);
int near continue_menu(void);
void near gameover_bgm_play_and_fade(void);
void near ending_staff_and_regist(void);
void near ending_after_regist(void);

inline void mainl_exit_to_op(void)
{
	mainl_replay_finish(RUER_GAME_OVER, T3_REPLAY_RES_MODE_SAVE_PROMPT);
	text_clear();
	gaiji_restore();
	game_exit();
	execl(mainl_binary_op_fn, mainl_binary_op_fn, nullptr);
}

inline void mainl_exit_to_main(void)
{
	mainl_replay_exit_to_main();
	execl(mainl_binary_main_fn, mainl_binary_main_fn, nullptr);
}

extern "C" void far mainl_entry(int argc, const char **argv, const char **envp)
{
	unsigned char result;
	unsigned char script_id;

	if(cfg_load_resident_ptr()) {
		#if defined(TH03_MIDI_DIAGNOSTICS)
		th03_midi_diag_log(T3MD_MAINL_ENTER, 0, 0);
		#endif
		game_init_main(mainl_pf_fn);
		respal_exist();
		th03_snd_process_init();
		gaiji_backup();
		gaiji_entry_bfnt(mainl_gaiji_fn);
		snd_load(stage_splash_yume_efc_fn, SND_LOAD_SE);
		snd_se_reset();
		hflip_lut_generate();
		result = mainl_replay_resume_take();
		mainl_replay_session_start();
		if(result == T3R_RES_MODE_USER_GAME_OVER) {
			regist_game_over_replay_playback_finish();
			goto exit_to_op;
		}

		if(resident->show_score_menu) {
			regist_menu();
			mainl_exit_to_op();
		}

		playchar[0].v = (resident->playchar_paletted[0].v - 1);
		playchar[1].v = (resident->playchar_paletted[1].v - 1);
		if(result == T3_REPLAY_RES_MODE_RESUME_GAME_OVER) {
			goto continue_menu_resume;
		}
		if(result == T3_REPLAY_RES_MODE_RESUME_CLEAR) {
			ending_after_regist();
			return;
		}
		if(!mainl_replay_stage_transition_needed()) {
			goto stage_splash_load_and_show;
		}
		if(resident->game_mode == GM_STORY) {
			result = sub_9887();
			if(result == 4) {
				goto ending;
			}
			if(result == 5) {
				ending_staff_and_regist();
			}
		}

		snd_load(mainl_win_bgm_fn, SND_LOAD_SONG);
		win_load();
		win_animate_and_wait();
		snd_kaja_func(KAJA_SONG_STOP, 0);

		if(resident->game_mode != GM_STORY) {
			goto gameover;
		}

		result = sub_9887();
		if(result == 0) {
stage_splash_show:
			stage_splash_show_and_wait();
			goto exit_to_main;
		}
		if(static_cast<uint8_t>(result - 3) < 2) {
			goto ending;
		}
		goto continue_menu_or_gameover;

ending:
		cdg_free_all();
		pi_free(0);
		script_id = (playchar[0].v / 2);
		if(script_id >= 10) {
			win_cutscene_script_fn[1] += (script_id / 10);
			script_id %= 10;
		}
		asm {
			les	bx, win_cutscene_script_fn
			mov	al, [bp - 2]
			add	es:[bx + 2], al
			cmp	byte ptr [bp - 1], 4
			jnz	mainl_entry_not_extra_ending
			inc	byte ptr es:[bx + 5]
		mainl_entry_not_extra_ending:
		}
		graph_accesspage(0);
		graph_showpage(0);
		graph_clear();
		graph_show();
		cutscene_script_load(win_cutscene_script_fn);
		cutscene_animate();
		cutscene_script_free();
		stage_splash_load();
		stage_splash_show_and_wait();
		gaiji_restore();

exit_to_main:
		#if defined(TH03_MIDI_DIAGNOSTICS)
		th03_midi_diag_log(T3MD_MAINL_TO_MAIN, 0, 0);
		#endif
		mainl_replay_exit_to_main();
		#if defined(TH03_MIDI_DIAGNOSTICS)
		th03_midi_diag_log(T3MD_MAINL_EXIT_DONE, 0, 0);
		#endif
		asm {
			db  	66h, 6Ah, 0
			push	ds
			push	offset mainl_binary_main_fn
			push	ds
			push	offset mainl_binary_main_fn
		}

execl_and_return:
		execl_raw();
		asm { add sp, 0Ch; }
		return;

continue_menu_or_gameover:
		cdg_free_all();
		pi_free(0);
		regist_menu();
		if(mainl_replay_finish(
			RUER_GAME_OVER, T3_REPLAY_RES_MODE_SAVE_PROMPT_GAME_OVER
		)) {
			goto exit_to_op;
		}
		goto continue_menu_ready;

continue_menu_resume:
		regist_next_screen_resume();

continue_menu_ready:
		if(continue_menu() != 0) {
			goto stage_splash_load_and_show;
		}
		gameover_bgm_play_and_fade();
		goto exit_to_op;

gameover:
		cdg_free_all();
		pi_free(0);

exit_to_op:
		mainl_replay_finish(RUER_GAME_OVER, T3_REPLAY_RES_MODE_SAVE_PROMPT);
		text_clear();
		gaiji_restore();
		game_exit();
		asm {
			db  	66h, 6Ah, 0
			push	ds
			push	offset mainl_binary_op_fn
			push	ds
			push	offset mainl_binary_op_fn
		}
		goto execl_and_return;

stage_splash_load_and_show:
		stage_splash_load();
		goto stage_splash_show;
	}
}

// Keeps every later original CUTSCENE_TEXT contribution at its accepted offset.
#pragma codestring "\x90\x90\x90\x90\x90"
#pragma codeseg
