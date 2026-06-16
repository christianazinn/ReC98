#include "codegen.hpp"
#include "th03/main/player/cur.hpp"
#include "th03/main/hud/dynamic.hpp"
#include "th03/main/round.hpp"
#include "th03/main/score.hpp"
#include "th03/main/v_colors.hpp"
#include "th03/main/enemy/efe.hpp"
#include "th03/main/player/stuff.hpp"
#include "th03/resident.hpp"
#include "th03/gaiji/gaiji.h"
#include "th02/snd/snd.h"
#include "libs/master.lib/pc98_gfx.hpp"

extern "C" void pascal far sub_14B0A(subpixel_t left, subpixel_t top);
extern "C" void pascal far sub_D668(
	screen_x_t left, vram_y_t top, uint16_t points, vc_t col
);

extern "C" unsigned char byte_20E3D;
extern "C" uint16_t score_23DEA;
extern "C" uint16_t word_23DEC;
extern "C" uint16_t word_23DEE;
extern "C" uint32_t score_23DF0;
extern "C" uint8_t byte_23DF9;

extern "C" uint8_t defeat_combo_hits_max;
extern "C" uint8_t defeat_gauge_attacks_fired;
extern "C" uint8_t defeat_boss_attacks_fired;
extern "C" uint8_t defeat_boss_attacks_reversed;
extern "C" uint8_t defeat_boss_panics_fired;
extern "C" uint8_t byte_23AF9;
extern "C" uint8_t byte_23B00;

extern "C" const char near aMAX_COMBO[];
extern "C" const char near aGAUGE_ATTACK_TIMES[];
extern "C" const char near aBOSS_ATTACK_TIMES[];
extern "C" const char near aBOSS_REVERSAL_TIMES[];
extern "C" const char near aBOSS_PANIC_TIMES[];
extern "C" const char near aTOTAL[];
extern "C" const char near aWINNER_BONUS[];
extern "C" const char near aALL_CLEAR[];
extern "C" const char near aPLAYER_REM[];

#define push_imm16_160() \
	__emit__(0x68, 0xA0, 0x00)

#define push_imm32_bytes(b0, b1, b2, b3) \
	__emit__(0x66, 0x68, b0, b1, b2, b3)

#define h5_const_prepare(left_offset, b0, b1, b2, b3) { \
	_AX = (left + left_offset); \
	_asm { push ax } \
	push_imm32_bytes(b0, b1, b2, b3); \
}

#define h5_call_nop(col) { \
	__emit__(0x6A, col); \
	_asm { nop; push cs; call near ptr hud_dynamic_5_digit_points_put; } \
}

#define h5_const_nop(left_offset, b0, b1, b2, b3, col) { \
	h5_const_prepare(left_offset, b0, b1, b2, b3); \
	h5_call_nop(col); \
}

#define h5_word_prepare(left_offset, points) { \
	_AX = (left + left_offset); \
	_asm { push ax } \
	push_imm16_160(); \
	_asm { push word ptr points } \
}

#define sub_D668_nopcall(left_offset, points, col) { \
	_AX = (left + left_offset); \
	_asm { push ax } \
	push_imm16_160(); \
	_asm { push word ptr points } \
	__emit__(0x6A, col); \
	_asm { nop; push cs; call near ptr sub_D668; } \
}

#define score_add_nopcall(score) { \
	_asm { push word ptr score; push word ptr [bp - 2]; } \
	_asm { nop; push cs; call near ptr score_add; } \
}

extern "C" void near sub_E266(void)
{
	graph_accesspage(1);
	super_put(544, 328,  0);
	super_put(576, 328,  3);
	super_put(608, 328,  6);
	super_put(544, 360,  9);
	super_put(576, 360, 12);
	super_put(608, 360, 15);
	graph_accesspage(0);
	super_put(544, 328,  0);
	super_put(576, 328,  3);
	super_put(608, 328,  6);
	super_put(544, 360,  9);
	super_put(576, 360, 12);
	super_put(608, 360, 15);
}

extern "C" void near sub_E313(void)
{
	int i;
	register shotpair_t near *shotpair = shotpairs;

	for(i = 0; i < SHOTPAIR_COUNT; i++, shotpair++) {
		shotpair->alive = false;
	}
	register efe_t near *efe = efes;
	for(i = 0; i < EFE_COUNT; i++, efe++) {
		efe->flag = EFF_FREE;
	}
	round_frame = 0;
	round_or_result_frame = 0;
	byte_23AF9 = 1;
	byte_23B00 = 0;
}

extern "C" void pascal near sub_E35B(
	tram_x_t left, vram_y_t top, uint16_t value
)
{
	register int value_rem;
	register tram_x_t left_reg;

	left_reg = left;
	value_rem = value;
	if(value_rem >= 10) {
		if(value_rem >= 100) {
			gaiji_putca(
				(left_reg + 18),
				((top + 96) / GLYPH_H),
				(gb_0 + (value_rem / 100)),
				TX_WHITE
			);
			value_rem %= 100;
		}
		gaiji_putca(
			(left_reg + 20),
			((top + 96) / GLYPH_H),
			(gb_0 + (value_rem / 10)),
			TX_WHITE
		);
		value_rem %= 10;
	}
	gaiji_putca(
		(left_reg + 22), ((top + 96) / GLYPH_H), (gb_0 + value_rem), TX_WHITE
	);
}

extern "C" void pascal near sub_E3F2(void)
{
	int pid_winner;
	long bonus_tmp;
	register int left;
	register player_stuff_t near *player;

	pid_winner = (1 - byte_20E3D);
	left = 4;
	if(pid_winner != 0) {
		left += 40;
	}

	if((round_or_result_frame & 7) == 1) {
		text_putsa(left, 8, aMAX_COMBO, TX_WHITE);
		text_putsa(left, 10, aGAUGE_ATTACK_TIMES, TX_WHITE);
		text_putsa(left, 12, aBOSS_ATTACK_TIMES, TX_WHITE);
		text_putsa(left, 14, aBOSS_REVERSAL_TIMES, TX_WHITE);
		text_putsa(left, 16, aBOSS_PANIC_TIMES, TX_WHITE);
		text_putsa(left, 20, aTOTAL, TX_WHITE);
		if(resident->story_stage < STAGE_YUMEMI) {
			text_putsa(left, 6, aWINNER_BONUS, TX_WHITE);
		} else {
			text_putsa(left, 6, aALL_CLEAR, TX_WHITE);
			text_putsa(left, 18, aPLAYER_REM, TX_WHITE);
		}

		player = &players[pid_winner];
		if(round_or_result_frame == 1) {
			defeat_combo_hits_max = player->combo_hits_max;
			defeat_gauge_attacks_fired = player->gauge_attacks_fired;
			defeat_boss_attacks_fired = player->boss_attacks_fired;
			defeat_boss_attacks_reversed = player->boss_attacks_reversed;
			defeat_boss_panics_fired = player->boss_panics_fired;

			score_23DF0 = (static_cast<uint32_t>(defeat_combo_hits_max) * 1000);
			score_23DF0 += (
				static_cast<uint32_t>(defeat_gauge_attacks_fired) * 10000
			);
			score_23DF0 += (
				static_cast<uint32_t>(defeat_boss_attacks_fired) * 15000
			);
			score_23DF0 += (
				static_cast<uint32_t>(defeat_boss_attacks_reversed) * 20000
			);
			score_23DF0 += (
				static_cast<uint32_t>(defeat_boss_panics_fired) * 30000
			);
			if(resident->story_stage == STAGE_YUMEMI) {
				byte_23DF9 = resident->story_lives;
				score_23DF0 += (static_cast<uint32_t>(byte_23DF9) * 100000);
				extends_gained = EXTENDS_DISABLE;
			}

			bonus_tmp = (static_cast<long>(score_23DF0) / 192);
			if(static_cast<unsigned long>(bonus_tmp) > 0xFFFF) {
				score_23DEA = -1;
			} else {
				score_23DEA = static_cast<uint16_t>(bonus_tmp);
			}

			bonus_tmp = score_23DF0;
			word_23DEC = static_cast<uint16_t>(bonus_tmp % 10000);
			word_23DEE = static_cast<uint16_t>(bonus_tmp / 10000);
		}

		sub_E35B(left, 32, defeat_combo_hits_max);
		sub_E35B(left, 64, defeat_gauge_attacks_fired);
		sub_E35B(left, 96, defeat_boss_attacks_fired);
		sub_E35B(left, 128, defeat_boss_attacks_reversed);
		sub_E35B(left, 160, defeat_boss_panics_fired);
		if(resident->story_stage == STAGE_YUMEMI) {
			sub_E35B(left, 192, byte_23DF9);
		}
	}

	left *= 8;
	h5_const_nop(216, 0xE8, 0x03, 0x40, 0x00, 0x0F);
	h5_const_nop(216, 0x10, 0x27, 0x50, 0x00, 0x0F);
	h5_const_nop(216, 0x98, 0x3A, 0x60, 0x00, 0x0F);
	h5_const_nop(216, 0x20, 0x4E, 0x70, 0x00, 0x0F);
	h5_const_nop(216, 0x30, 0x75, 0x80, 0x00, 0x0F);
	if(resident->story_stage == STAGE_YUMEMI) {
		h5_const_nop(208, 0x10, 0x27, 0x90, 0x00, 0x0F);
		h5_const_nop(216, 0x00, 0x00, 0x90, 0x00, 0x0F);
	}

	if(word_23DEC != 0) {
		h5_word_prepare(216, word_23DEC);
		goto total_remainder_put;
	} else {
		h5_const_nop(216, 0x00, 0x00, 0xA0, 0x00, 0x0C);
		h5_const_nop(200, 0x00, 0x00, 0xA0, 0x00, 0x0C);
		h5_const_prepare(192, 0x00, 0x00, 0xA0, 0x00);
	}
total_remainder_put:
	h5_call_nop(0x0C);
	sub_D668_nopcall(184, word_23DEE, 0x0C);

	if(static_cast<long>(score_23DEA) < static_cast<long>(score_23DF0)) {
		score_add_nopcall(score_23DEA);
		score_23DF0 -= static_cast<uint32_t>(score_23DEA);
	} else if(static_cast<long>(score_23DF0) > 0) {
		score_add_nopcall(score_23DF0);
		score_23DF0 = 0;
	}
}

#undef score_add_nopcall
#undef sub_D668_nopcall
#undef h5_word_prepare
#undef h5_const_nop
#undef h5_call_nop
#undef h5_const_prepare
#undef push_imm32_bytes
#undef push_imm16_160

void near shots_add(void)
{
	int i;
	subpixel_t left;
	subpixel_t top;
	int pairs_fired;
	register player_stuff_t near *player = &players[pid.current];
	register shotpair_t near *shotpair;

	top = player->center.y.v;
	if(player->shot_mode == SM_NONE) {
		return;
	}
	if(player->shot_mode == SM_4_PAIRS) {
		left = (player->center.x.v + TO_SP(-64));
		pairs_fired = 0;
	} else if(player->shot_mode == SM_2_PAIRS) {
		left = (player->center.x.v + TO_SP(-32));
		pairs_fired = 2;
	} else if(player->shot_mode == SM_1_PAIR) {
		left = (player->center.x.v + TO_SP(-16));
		pairs_fired = 3;
	} else if(player->shot_mode == SM_REIMU_HYPER) {
		left = (player->center.x.v + TO_SP(-24));
		top -= TO_SP(1);
		sub_14B0A(left, top);
		left += TO_SP(48);
		sub_14B0A(left, top);
		left = (player->center.x.v + TO_SP(-16));
		pairs_fired = 3;
	}

	shotpair = shotpairs;
	snd_se_play(1);
	while(left <= TO_SP(-32)) {
		left += TO_SP(32);
		pairs_fired++;
	}

	for(i = 0; i < SHOTPAIR_COUNT; i++, shotpair++) {
		if(!shotpair->alive) {
			shotpair->alive = true;
			shotpair->unused_1 = 0;
			shotpair->topleft.x.v = left;
			shotpair->topleft.y.v = top;
			shotpair->velocity_y.v = to_sp(SHOT_VELOCITY);
			shotpair->so_pid = ((pid.current == 0) ? 0 : SHOT_SO_PID);
			shotpair->so_anim = 0;
			shotpair->pid = pid.current;

			pairs_fired++;
			if(pairs_fired >= 4) {
				return;
			}
			left += TO_SP(32);
			if(left >= TO_SP(PLAYFIELD_W)) {
				return;
			}
		}
	}
}

void near shots_update(void)
{
	shotpair_t near *shotpair = shotpairs;
	for(int i = 0; i < SHOTPAIR_COUNT; i++, shotpair++) {
		if(shotpair->alive) {
			shotpair->topleft.y.v += shotpair->velocity_y.v;
			if(shotpair->topleft.y.v <= to_sp(-1.0f)) {
				shotpair->alive = false;
			}
		}
	}
}

void near shots_render(void)
{
	shotpair_t near *shotpair = shotpairs;

	sprite16_put_size.set(SHOT_W, SHOT_H);
	sprite16_clip.reset();

	for(int i = 0; i < SHOTPAIR_COUNT; i++, shotpair++) {
		if(shotpair->alive) {
			sprite16_offset_t so = (shotpair->so_anim + shotpair->so_pid);
			screen_x_t left = playfield_fg_x_to_screen(
				shotpair->topleft.x, shotpair->pid
			);
			screen_y_t top = shotpair->topleft.y.to_pixel() + PLAYFIELD_TOP;

			sprite16_put(left + 0,                 top, so);
			sprite16_put(left + SHOTPAIR_DISTANCE, top, so);

			shotpair->so_anim += SHOT_VRAM_W;
			if(shotpair->so_anim >= (SHOT_VRAM_W * SHOT_SPRITE_COUNT)) {
				shotpair->so_anim = 0;
			}
		}
	}
}
