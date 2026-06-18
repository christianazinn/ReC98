#pragma option -zCmain_05_TEXT

#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "platform.h"
#include "th02/snd/snd.h"
#include "th03/formats/mrs.hpp"
#include "th03/hardware/palette.hpp"
#include "th03/main/player/bomb.hpp"
#include "th03/main/player/cur.hpp"
#include "th03/main/playfld.hpp"
#include "th03/math/vector.hpp"
#include "x86real.h"

extern "C" uint8_t byte_20E92[];

struct ellen_bomb_vector_t {
	subpixel_t x;
	subpixel_t y;
	subpixel_t velocity_x;
	subpixel_t velocity_y;
};

static const int ELLEN_BOMB_VECTOR_COUNT = 8;
static const subpixel_t ELLEN_BOMB_VECTOR_FREE = 9999;
static const subpixel_t ELLEN_BOMB_VECTOR_INACTIVE = 19999;

extern "C" ellen_bomb_vector_t ellen_bomb_vectors[
	PLAYER_COUNT
][ELLEN_BOMB_VECTOR_COUNT];
extern "C" ellen_bomb_vector_t near *word_1FBBE;

extern "C" void far sub_B39E(void);
extern "C" void far SUB_A3D2(void);
extern "C" void pascal far sub_A3A8(uint8_t pid);
extern "C" uint16_t far randring_far_next16_raw(void);
extern "C" void pascal far SUB_CE0C(
	subpixel_t x, subpixel_t y, uint16_t pid
);
extern "C" void pascal far SUB_CE5B(
	subpixel_t x, subpixel_t y, uint16_t pid
);

extern "C" void pascal far chiyuri_bomb(void)
{
	uint8_t frame;
	uint8_t col;
	register screen_x_t left;

	if(bomb_flag[pid_current] == BF_INACTIVE) {
		return;
	}
	egc_off();
	frame = bomb_frame[pid_current];
	if(frame < 64) {
		grcg_setcolor(GC_RMW, pid_current);
		_BX = FP_OFF(byte_20E92);
		if(pid_current != 0) {
			_BX += 0x28;
		}
		sub_B39E();
		grcg_off();
		_AL = frame;
		_AL <<= 2;
		_DL = 255;
		_DL -= _AL;
		col = _DL;
		_AL = col;
		_AH = 0;
		_asm {
			push	ax
			push	word ptr pid_current
			call	far ptr SUB_A3D2
		}
		if((frame % 8) == 0) {
			SUB_CE0C(TO_SP(144), TO_SP(184), pid_current);
		}
	} else if(frame < 144) {
		palette_changed = true;
		if((frame & 3) < 2) {
			PaletteTone = 60;
			palette_changed = true;
			playfield_fg_shift_x[pid_current] = 4;
		} else {
			playfield_fg_shift_x[pid_current] = -4;
			PaletteTone = 120;
			palette_changed = true;
		}

		if((frame % 16) == 0) {
			snd_se_play(10);
			_AL = frame;
			_AH = 0;
			_AX += -64;
			_AX += _AX;
			_AX <<= 4;
			left = _AX;
			SUB_CE5B((TO_SP(144) - left), TO_SP(184), pid_current);
			SUB_CE5B((left + TO_SP(144)), TO_SP(184), pid_current);
			SUB_CE5B(TO_SP(144), (TO_SP(184) - left), pid_current);
			SUB_CE5B(TO_SP(144), (left + TO_SP(184)), pid_current);
		}

		left = PLAYFIELD_LEFT;
		if(pid_current != 0) {
			left += PLAYFIELD_W_BORDERED;
		}
		mrs_put_noalpha_8(
			left, PLAYFIELD_TOP, (pid_current + 2), (_AX = pid_current)
		);
	} else {
		PaletteTone = 100;
		palette_changed = true;
		playfield_fg_shift_x[pid_current] = 0;
		_AL = frame;
		_AL <<= 3;
		_DL = 255;
		_DL -= _AL;
		frame = _DL;
		_AL = frame;
		_AL += _AL;
		col = _AL;
		Palettes[pid_current].c.r = col;
		Palettes[pid_current].c.g = _DL;
		Palettes[pid_current].c.b = frame;
		palette_changed = true;
	}

	egc_on();
}

extern "C" void pascal far ellen_185AB(void)
{
	register int i;

	if(bomb_flag[pid_current] == BF_INACTIVE) {
		return;
	}
	if(bomb_flag[pid_current] == BF_PREPARING) {
		bomb_flag[pid_current] = BF_ACTIVE;
		bomb_frame[pid_current] = 0;
		i = 0;
		goto init_loop_test;

init_loop:
		ellen_bomb_vectors[pid_current][i].x = ELLEN_BOMB_VECTOR_INACTIVE;
		i++;

init_loop_test:
		if(i < ELLEN_BOMB_VECTOR_COUNT) {
			goto init_loop;
		}
		snd_se_play(17);
	}

	bomb_frame[pid_current]++;
	playfield_clip_negative_radius.x.v = TO_SP(-32);
	playfield_clip_negative_radius.y.v = TO_SP(-32);
	word_1FBBE = &ellen_bomb_vectors[pid_current][0];

	i = 0;
	goto update_loop_test;

update_loop:
	if(word_1FBBE->x == ELLEN_BOMB_VECTOR_FREE) {
		goto respawn;
	}
	if(word_1FBBE->x == ELLEN_BOMB_VECTOR_INACTIVE) {
		goto next;
	}
	word_1FBBE->x += word_1FBBE->velocity_x;
	word_1FBBE->y += word_1FBBE->velocity_y;
	if(playfield_clip(
		reinterpret_cast<PlayfieldSubpixel near *>(&word_1FBBE->x)[0],
		reinterpret_cast<PlayfieldSubpixel near *>(&word_1FBBE->y)[0]
	) == false) {
		goto next;
	}

respawn:
	word_1FBBE->x = TO_SP(144);
	word_1FBBE->y = TO_SP(184);
	vector2(
		word_1FBBE->velocity_x,
		word_1FBBE->velocity_y,
		randring_far_next16_raw(),
		224
	);

next:
	i++;
	word_1FBBE++;

update_loop_test:
	if(i < ELLEN_BOMB_VECTOR_COUNT) {
		goto update_loop;
	}

	if(bomb_frame[pid_current] >= BOMB_FRAMES) {
		bomb_flag[pid_current] = BF_INACTIVE;
		sub_A3A8(pid_current);
	}
}
