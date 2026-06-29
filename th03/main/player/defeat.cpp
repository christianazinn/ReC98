#pragma option -zCPLAYER_M_TEXT -zPmain_01 -G-

#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/snd/snd.h"
#include "th03/main/defeat.hpp"
#include "th03/main/hud/static.hpp"
#include "th03/main/playfld.hpp"
#include "th03/main/player/bomb.hpp"
#include "th03/main/player/stuff.hpp"
#include "th03/main/round.hpp"
#include "th03/main/sprite16.hpp"
#include "th03/math/polar.hpp"
#include "th03/resident.hpp"
#include "x86real.h"

extern "C" uint8_t byte_23AF9;
extern "C" uint8_t pid_PID_current;
extern "C" int word_20E3E;
extern "C" int word_20E40;
extern "C" int word_20E42;

static const sprite16_offset_t DEFEAT_SPRITE_OFFSET = (
	(168 * ROW_SIZE) + (416 / BYTE_DOTS)
);

#pragma option -G
extern "C" void pascal near sub_C0D8(player_stuff_t near *player)
{
	register player_stuff_t near *p = player;

	if(p->lose_anim_time == 0x30) {
		word_20E3E = (playfield_fg_x_to_screen(p->center.x, pid_PID_current) - 24);
		word_20E40 = ((p->center.y.v >> 4) + 0xFFF8);
		defeat_flag = DF_EXPLODE;
		snd_se_play(14);
		damage_all_on[1 - pid_PID_current] = true;
		word_20E42 = 0;
	} else {
		word_20E42 += 9;
	}

	if((p->lose_anim_time & 3) < 2) {
		playfield_fg_shift_x[pid_PID_current] = 6;
	} else {
		playfield_fg_shift_x[pid_PID_current] = -6;
	}
	byte_23AF9 = 2;
	p->lose_anim_time--;
	if(p->lose_anim_time == 0) {
		word_20E3E = 0;
		word_20E40 = 0;
		word_20E42 = 0;
		playfield_fg_shift_x[pid_PID_current] = 0;
		p->lose_anim_time = 0xFF;
		PaletteTone = 100;
		palette_show();
		defeat_flag = DF_BANNER;
		players[1 - pid_PID_current].rounds_won++;
		resident->pid_winner = (1 - pid_PID_current);
		round_or_result_frame = 0;
		hud_static_rounds_won_put(1 - pid_PID_current);
	} else {
		PaletteTone = (100 + ((round_or_result_frame & 1) * 30));
		palette_show();
	}
}
#pragma option -G-

extern "C" void pascal near sub_C1E6(
	uint8_t angle_for_cos, uint8_t angle_for_sin
)
{
	volatile int left;
	int top;

	left = (
		polar(word_20E3E, word_20E42, CosTable8[angle_for_cos]) +
		playfield_fg_shift_x[pid_PID_current]
	);
	top = polar(word_20E40, word_20E42, SinTable8[angle_for_sin]);
	sprite16_put(left, top, DEFEAT_SPRITE_OFFSET);
}

#define defeat_angle_from_radius() { \
	_AX = word_20E42; \
	_BX = 4; \
	asm { cwd; idiv bx; } \
}

extern "C" void pascal near sub_C248(player_stuff_t near *)
{
	uint8_t angle;

	// Force the original unused SI save/restore.
	_SI = _SI;
	if(pid_PID_current == 0) {
		sprite16_clip.left = PLAYFIELD1_CLIP_LEFT;
		sprite16_clip.right = PLAYFIELD1_CLIP_RIGHT;
	} else {
		sprite16_clip.left = PLAYFIELD2_CLIP_LEFT;
		sprite16_clip.right = PLAYFIELD2_CLIP_RIGHT;
	}
	sprite16_put_size.w.v = (48 / 16);
	sprite16_put_size.h = 24;

	_DI = 0;
	defeat_angle_from_radius();
	goto ring1_test;
ring1_loop:
	sub_C1E6(angle, (angle + 0x20));
	_DI++;
	_AL = angle;
	_AL += 0x10;
ring1_test:
	angle = _AL;
	if(static_cast<int16_t>(_DI) < 0x10) {
		goto ring1_loop;
	}

	_DI = 0;
	defeat_angle_from_radius();
	_DL = 0;
	_DL -= _AL;
	angle = _DL;
	goto ring2_test;
ring2_loop:
	sub_C1E6(angle, (angle + 224));
	_DI++;
	_AL = angle;
	_AL += 0x10;
	angle = _AL;
ring2_test:
	if(static_cast<int16_t>(_DI) < 0x10) {
		goto ring2_loop;
	}

	_DI = 0;
	defeat_angle_from_radius();
	goto ring3_test;
ring3_loop:
	sub_C1E6(angle, angle);
	_DI++;
	_AL = angle;
	_AL += 0x10;
ring3_test:
	angle = _AL;
	if(static_cast<int16_t>(_DI) < 0x10) {
		goto ring3_loop;
	}
}

#undef defeat_angle_from_radius
