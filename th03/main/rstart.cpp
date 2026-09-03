#pragma option -zCPLAYFLD_TEXT -zPmain_01

#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "libs/sprite16/sprite16.h"
#include "platform.h"
#include "th02/hardware/pages.hpp"
#include "th02/snd/snd.h"
#include "th01/rank.h"
#include "th03/main/demo.h"
#include "th03/main/difficul.hpp"
#include "th03/main/enemy/enemy.hpp"
#include "th03/main/hud/static.hpp"
#include "th03/main/player/cpu.hpp"
#include "th03/main/player/gba.hpp"
#include "th03/main/player/stuff.hpp"
#include "th03/main/replay.hpp"
#include "th03/main/round.hpp"
#include "th03/main/score.hpp"
#include "th03/practice.hpp"
#include "th03/resident.hpp"
#include "th03/snd/options.hpp"

extern unsigned char score[];

extern nearfunc_t_near fp_1E6EA;
extern nearfunc_t_near fp_1FBC0;

extern uint16_t word_1E6E8;
extern uint8_t byte_23AFA;

extern "C" const char aLose_bf2[];
extern "C" const char aRound_bf2[];
extern "C" const char aZikicw_bf2[];

extern "C" void pascal near sub_9EBF(void);
extern "C" void pascal near sub_B4A3(void);
extern "C" void pascal far sub_D5A2(void);
extern "C" void hflip_lut_generate(void);
extern "C" void near sub_E266(void);
extern "C" void pascal far SUB_A38E(void);

extern "C" void pascal near set_callbacks_reimu(int pid);
extern "C" void pascal near set_callbacks_mima(int pid);
extern "C" void pascal near set_callbacks_marisa(int pid);
extern "C" void pascal near set_callbacks_ellen(int pid);
extern "C" void pascal near set_callbacks_kotohime(int pid);
extern "C" void pascal near set_callbacks_kana(int pid);
extern "C" void pascal near set_callbacks_rikako(int pid);
extern "C" void pascal near set_callbacks_chiyuri(int pid);
extern "C" void pascal near set_callbacks_yumemi(int pid);

extern "C" void near demo_round_update(void);
extern "C" void near round_mode_update_none(void);

extern "C" void pascal far score_continues_used_digit_update(
	uint8_t credits_left
);

#define nopcall_noarg(func) { \
	_asm { nop; push cs; call near ptr func; } \
}

#pragma option -a2

extern "C" void pascal near round_startup(void)
{
	struct {
		uint8_t near *speed;
		long cpu_safety_frames;
		int playchar_id;
	} frame;
	register int i;
	register player_stuff_t near *p;

	replay_preroll_startup_mask();
	random_seed = resident->rand;
	text_fillca(' ', (TX_BLACK | TX_REVERSE));
	graph_copy_page(0);
	enemy_formations_load_deterministic();
	// The patch-owned wrapper's near call is one byte shorter than the stock call.
	_asm { nop; }
	round_id = 0;
	if(resident->game_mode == GM_VS_1P_CPU) {
		round_id = practice_initial_round();
	}
	replay_round_reset_seed_capture();
	sub_9EBF();
	hflip_lut_generate();
	nopcall_noarg(sub_D5A2);
	byte_23AFA = 0;

	if(resident->story_stage < STAGE_YUMEMI) {
		word_1E6E8 = 320;
	} else {
		word_1E6E8 = 460;
	}

	if(resident->game_mode == GM_STORY) {
		for(i = 0; i < SCORE_DIGITS; i++) {
			score[i] = resident->score_last[0].digits[i];
		}

		switch(resident->story_stage) {
		case 0:
		case 1:
			cpu_hit_damage_additional = 3;
			break;
		case 2:
		case 3:
			cpu_hit_damage_additional = 2;
			break;
		case 4:
		case 5:
			cpu_hit_damage_additional = 1;
			break;
		default:
			cpu_hit_damage_additional = 0;
			break;
		}

		gba_gauge_level[0] = (resident->story_stage + 1);
		gba_gauge_level[1] = (resident->story_stage + 1);
		extends_gained = score[6];
		if((extends_gained >= EXTENDS_MAX) || (score[7] != 0)) {
			extends_gained = EXTENDS_DISABLE;
		}
		_AL = resident->rem_credits;
		_asm { push ax; }
	} else {
		if(resident->is_cpu[1] && resident->is_cpu[0]) {
			frame.cpu_safety_frames = 0xFFFF;
		} else {
			switch(resident->rank) {
			case RANK_EASY:
				frame.cpu_safety_frames = 1000;
				break;
			case RANK_NORMAL:
				frame.cpu_safety_frames = 1900;
				break;
			case RANK_HARD:
				frame.cpu_safety_frames = 4000;
				break;
			case RANK_LUNATIC:
				frame.cpu_safety_frames = 0xFFFF;
				break;
			}
		}
		players[0].cpu_safety_frames = frame.cpu_safety_frames;
		players[1].cpu_safety_frames = frame.cpu_safety_frames;
		gba_gauge_level[0] = GBA_GAUGE_LEVEL_MIN;
		gba_gauge_level[1] = GBA_GAUGE_LEVEL_MIN;
		extends_gained = EXTENDS_DISABLE;
		cpu_hit_damage_additional = 0;
		_asm { push 3; }
	}
	_asm { nop; push cs; call near ptr score_continues_used_digit_update; }

	for(p = players, i = 0; i < PLAYER_COUNT; i++, p++) {
		p->rounds_won = 0;
		p->gauge_avail = TO_SP(64);
		p->combo_hits_max = 0;
		p->combo_bonus_max = 0;
	}
	if(resident->game_mode == GM_VS_1P_CPU) {
		practice_initial_apply();
	}

	if(resident->demo_num == 0) {
		fp_1E6EA = reinterpret_cast<nearfunc_t_near>(round_mode_update_none);
	} else {
		demo_frame = 0;
		fp_1E6EA = reinterpret_cast<nearfunc_t_near>(demo_round_update);
		round_speed = to_sp8(4.0f);
		bullet_base_speed.v = 0;
		gba_boss_level = 8;
		gba_gauge_level[0] = 9;
		gba_gauge_level[1] = 9;
	}

	fp_1FBC0 = sub_B4A3;
	if(resident->demo_num != 0) {
		input_mode = input_mode_attract;
	} else if(resident->is_cpu[1] && resident->is_cpu[0]) {
		input_mode = input_mode_cpu_vs_cpu;
	} else if(resident->is_cpu[1]) {
		input_mode = input_mode_1p_vs_cpu;
	} else if(resident->is_cpu[0]) {
		input_mode = input_mode_cpu_vs_1p;
	} else if(resident->key_mode == KM_KEY_KEY) {
		input_mode = input_mode_key_vs_key;
	} else if(resident->key_mode == KM_JOY_KEY) {
		input_mode = input_mode_joy_vs_key;
	} else {
		input_mode = input_mode_key_vs_joy;
	}

	for(i = 0; i < PLAYER_COUNT; i++) {
		frame.playchar_id = (
			static_cast<int>(players[i].playchar_paletted.v) - 1
		) / 2;
		frame.speed = reinterpret_cast<uint8_t near *>(
			&PLAYCHAR_SPEEDS[frame.playchar_id]
		);
		p = &players[i];
		p->speed_base.aligned.x.v = *frame.speed++;
		p->speed_base.aligned.y.v = *frame.speed++;
		p->speed_base.diagonal.x.v = *frame.speed++;
		p->speed_base.diagonal.y.v = *frame.speed++;
		p->gauge_charge_speed = *frame.speed;

		switch(frame.playchar_id) {
		case 0:
			set_callbacks_reimu(i);
			break;
		case 1:
			set_callbacks_mima(i);
			break;
		case 2:
			set_callbacks_marisa(i);
			break;
		case 3:
			set_callbacks_ellen(i);
			break;
		case 4:
			set_callbacks_kotohime(i);
			break;
		case 5:
			set_callbacks_kana(i);
			break;
		case 6:
			set_callbacks_rikako(i);
			break;
		case 7:
			set_callbacks_chiyuri(i);
			break;
		case 8:
			set_callbacks_yumemi(i);
			break;
		}
	}

	super_entry_bfnt(aLose_bf2);
	super_entry_bfnt(aRound_bf2);
	super_entry_bfnt(aZikicw_bf2);
	sub_E266();
	nopcall_noarg(SUB_A38E);
	graph_200line(0);
	sprite16_sprites_commit();
	page_back = 0;
	page_front = 1;
	graph_accesspage(0);
	graph_showpage(1);
	snd_se_reset();
	nopcall_noarg(hud_wipe);
	nopcall_noarg(hud_static_put);
	grc_setclip(0, 0, (RES_X - 1), (SPRITE16_RES_Y - 1));
	// Keep all process-local sound routing out of MAIN's stack-sensitive entry
	// window. MAINL has already probed both drivers and loaded this stage's
	// song; adopt that result and start playback only after round setup has
	// finished, at the original sound-helper call site.
	th03_snd_process_init_and_play();
}

// Keep all following PLAYFLD_TEXT code at its accepted offsets.
#pragma codestring "\x90\x90\x90\x90\x90"

#undef nopcall_noarg
