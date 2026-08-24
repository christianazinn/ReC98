/// Run-wide gameplay initialization
/// --------------------------------
/// Called once before the first stage_init() of a run. Resets run statistics,
/// installs the selected character's shot and bomb callbacks, and chooses the
/// bullet tuning callbacks for the run's effective rank.

#include "th04/end/end.h"
#include "th04/formats/bb.h"

#include "th04/main/hiscore.hpp"
#include "th04/main/player/bomb.hpp"
#include "th04/main/player/player.hpp"
#include "th04/playchar.h"
#include "th04/sprites/main_pat.h"

void near score_reset(void);

// These callbacks execute in main_03. bullet.hpp first declared their
// lower-case C++ identifiers while stage_loop.cpp was in main_01, and a later
// declaration cannot change that binding. These case-distinct source aliases
// are fresh identifiers, but `extern "C"` + `pascal` exports them under the
// same all-uppercase public symbols as the existing definitions.
#pragma codeseg BULLET_A_TEXT main_03
extern "C" {
	void pascal near BULLET_TEMPLATE_TUNE_EASY(void);
	void pascal near BULLET_TEMPLATE_TUNE_NORMAL(void);
	void pascal near BULLET_TEMPLATE_TUNE_HARD(void);
	void pascal near BULLET_TEMPLATE_TUNE_LUNATIC(void);
	void pascal near BULLETS_ADD_REGULAR_EASY(void);
	void pascal near BULLETS_ADD_REGULAR_NORMAL(void);
	void pascal near BULLETS_ADD_REGULAR_HARD_LUNATIC(void);
	void pascal near BULLETS_ADD_SPECIAL_EASY(void);
	void pascal near BULLETS_ADD_SPECIAL_NORMAL(void);
	void pascal near BULLETS_ADD_SPECIAL_HARD_LUNATIC(void);
}
#pragma codeseg

extern nearfunc_t_near bullet_template_tune;
extern nearfunc_t_near bullets_add_regular;
extern nearfunc_t_near bullets_add_special;

// The four tables remain in th04_main.asm's _DATA contribution. This near
// pointer selects one of them for the shot-level installer.
extern nearfunc_t_near shot_funcs_reimu_a[];
extern nearfunc_t_near shot_funcs_reimu_b[];
extern nearfunc_t_near shot_funcs_marisa_a[];
extern nearfunc_t_near shot_funcs_marisa_b[];
extern nearfunc_t_near near *playchar_shot_funcs;

#pragma option -a2

void near gameplay_init(void)
{
	register int i;

	resident->graze = 0;
	resident->miss_count = 0;
	resident->bombs_used = 0;
	resident->end_sequence = ES_INGAME;
	// Keep the conditional result 16-bit before storing its low byte.
	_AX = (
		(resident->playchar_ascii == ('0' + PLAYCHAR_MARISA))
		? 1
		: 0
	);
	playchar = static_cast<playchar_t>(_AL);
	for(i = 0; i < SCORE_DIGITS; i++) {
		score.digits[i] = 0;
	}
	power = POWER_MIN;
	resident->rem_bombs = resident->credit_bombs;
	resident->rem_lives = resident->credit_lives;
	bb_txt_load();

	if(playchar == PLAYCHAR_REIMU) {
		player_option_patnum = PAT_OPTION_REIMU;
		if(resident->shottype == SHOTTYPE_A) {
			playchar_shot_funcs = reinterpret_cast<nearfunc_t_near near *>(
				FP_OFF(shot_funcs_reimu_a)
			);
		} else {
			playchar_shot_funcs = reinterpret_cast<nearfunc_t_near near *>(
				FP_OFF(shot_funcs_reimu_b)
			);
		}
		player_bomb_func = player_bomb;
		playchar_bomb_func = bomb_reimu;
	} else {
		player_option_patnum = PAT_OPTION_MARISA;
		if(resident->shottype == SHOTTYPE_A) {
			playchar_shot_funcs = reinterpret_cast<nearfunc_t_near near *>(
				FP_OFF(shot_funcs_marisa_a)
			);
		} else {
			playchar_shot_funcs = reinterpret_cast<nearfunc_t_near near *>(
				FP_OFF(shot_funcs_marisa_b)
			);
		}
		player_bomb_func = player_bomb;
		playchar_bomb_func = bomb_marisa;
	}

	if(resident->demo_num != 0) {
		playperf = 28;
		rank = RANK_HARD;
		turbo_mode = true;
	} else {
		playperf = 16;
		if(stage_id == 6) {
			rank = RANK_EXTRA;
			turbo_mode = true;
		} else {
			rank = resident->rank;
			turbo_mode = resident->turbo_mode;
		}
	}

	score_reset();
	hiscore_load();
	switch(rank) {
	case RANK_EASY:
		graze_score = 100;
		playperf_min = 4;
		playperf_max = 16;
		bullets_add_regular = BULLETS_ADD_REGULAR_EASY;
		bullets_add_special = BULLETS_ADD_SPECIAL_EASY;
		bullet_template_tune = BULLET_TEMPLATE_TUNE_EASY;
		break;

	case RANK_NORMAL:
		graze_score = 250;
		playperf_min = 11;
		playperf_max = 24;
		goto tune_normal;

	case RANK_HARD:
		playperf = 20;
		graze_score = 400;
		playperf_min = 20;
		playperf_max = 32;
		bullets_add_regular = BULLETS_ADD_REGULAR_HARD_LUNATIC;
		bullets_add_special = BULLETS_ADD_SPECIAL_HARD_LUNATIC;
		bullet_template_tune = BULLET_TEMPLATE_TUNE_HARD;
		break;

	case RANK_LUNATIC:
		graze_score = 500;
		playperf = 22;
		playperf_min = 22;
		playperf_max = 34;
		bullets_add_regular = BULLETS_ADD_REGULAR_HARD_LUNATIC;
		bullets_add_special = BULLETS_ADD_SPECIAL_HARD_LUNATIC;
		bullet_template_tune = BULLET_TEMPLATE_TUNE_LUNATIC;
		break;

	case RANK_EXTRA:
		graze_score = 2560;
		playperf_min = 16;
		playperf_max = 20;

	tune_normal:
		bullets_add_regular = BULLETS_ADD_REGULAR_NORMAL;
		bullets_add_special = BULLETS_ADD_SPECIAL_NORMAL;
		bullet_template_tune = BULLET_TEMPLATE_TUNE_NORMAL;
		break;
	}
}

#pragma option -a1
