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
#include "th03/math/polar.hpp"
#include "th03/main/sprite16.hpp"
#include "th03/math/vector.hpp"
#include "x86real.h"

extern "C" uint8_t byte_20E92[];
extern "C" uint8_t pid_PID_so_attack;
extern "C" uint8_t angle_1FBD4;
extern "C" subpixel_t word_1FE56;

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
extern "C" int word_1FB3C;
extern "C" ellen_bomb_vector_t near *word_1FBBE;

extern "C" void far sub_B39E(void);
extern "C" void far SUB_A3D2(void);
extern "C" void pascal far sub_A3A8(uint8_t pid);
extern "C" uint16_t far randring_far_next16_raw(void);
extern "C" void pascal far SUB_CDBD(
	subpixel_t x, subpixel_t y, uint16_t pid
);
extern "C" void pascal far SUB_CE0C(
	subpixel_t x, subpixel_t y, uint16_t pid
);
extern "C" void pascal far SUB_CE5B(
	subpixel_t x, subpixel_t y, uint16_t pid
);

extern "C" void pascal near ellen_bomb_186C3(void);

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

#pragma warn -aus
extern "C" void pascal near ellen_bomb_186C3(void)
{
	volatile screen_x_t left;
	volatile screen_y_t top;
	volatile sprite16_offset_t sprite_offset;
	register int i;

	word_1FBBE = &ellen_bomb_vectors[pid_current][0];
	sprite16_put_size.w.v = (64 / 16);
	sprite16_put_size.h = 32;
	if(pid_current == 0) {
		sprite16_clip.left = PLAYFIELD1_CLIP_LEFT;
		sprite16_clip.right = PLAYFIELD1_CLIP_RIGHT;
	} else {
		sprite16_clip.left = PLAYFIELD2_CLIP_LEFT;
		sprite16_clip.right = PLAYFIELD2_CLIP_RIGHT;
	}

	_AL = pid_PID_so_attack;
	_AH = 0;
	_AX += ((8 * ROW_SIZE) + (224 / BYTE_DOTS));
	sprite_offset = _AX;

	i = 0;
	goto loop_test;

loop:
	if(word_1FBBE->x == ELLEN_BOMB_VECTOR_INACTIVE) {
		goto next;
	}
	if(word_1FBBE->x == ELLEN_BOMB_VECTOR_FREE) {
		goto next;
	}
	left = (playfield_fg_x_to_screen(word_1FBBE->x, pid_current) + -32);
	_AX = word_1FBBE->y;
	asm { sar ax, 4; }
	_AX += -16;
	top = _AX;
	sprite16_put(left, _AX, sprite_offset);

next:
	i++;
	word_1FBBE++;

loop_test:
	if(i < ELLEN_BOMB_VECTOR_COUNT) {
		goto loop;
	}
}
#pragma warn .aus

#pragma warn -aus
extern "C" void pascal far ellen_bomb(void)
{
	volatile screen_x_t shift_x_backup;
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
		_AL += _AL;
		col = _AL;
		_asm {
			mov	al, pid_current
			mov	ah, 0
			imul	ax, ax, 3
			mov	dl, byte ptr [bp-4]
			db	02h, 0D2h
			db	08Bh, 0D8h
			mov	byte ptr Palettes[bx], dl
			mov	al, pid_current
			mov	ah, 0
			imul	ax, ax, 3
			mov	dl, byte ptr [bp-4]
			db	08Bh, 0D8h
			mov	byte ptr Palettes[bx+1], dl
			mov	al, pid_current
			mov	ah, 0
			imul	ax, ax, 3
			db	08Bh, 0D8h
			mov	byte ptr Palettes[bx+2], dl
		}
		palette_changed = true;
		word_1FB3C = 0;
		goto active_done;
	}

	if(frame < 128) {
		if((frame & 1) != 0) {
			snd_se_play(10);
			Palettes[pid_current].c.r = 255;
			Palettes[pid_current].c.g = 128;
			Palettes[pid_current].c.b = 128;
		} else {
			Palettes[pid_current].c.r = 0;
			Palettes[pid_current].c.g = 0;
			Palettes[pid_current].c.b = 32;
		}
		palette_changed = true;
		if((frame & 3) < 2) {
			playfield_fg_shift_x[pid_current] = 4;
		} else {
			playfield_fg_shift_x[pid_current] = -4;
		}
		left = PLAYFIELD_LEFT;
		if(pid_current != 0) {
			left += PLAYFIELD_W_BORDERED;
		}
		mrs_put_noalpha_8(
			left, PLAYFIELD_TOP, (pid_current + 2), (_AX = pid_current)
		);
		if((frame % 4) == 0) {
			SUB_CDBD(TO_SP(144), TO_SP(184), pid_current);
			if(word_1FB3C < ELLEN_BOMB_VECTOR_COUNT) {
				ellen_bomb_vectors[pid_current][word_1FB3C].x = (
					ELLEN_BOMB_VECTOR_FREE
				);
				word_1FB3C++;
			}
		}
		goto active_done;
	}

	playfield_fg_shift_x[pid_current] = 0;
	_AL = frame;
	_AL <<= 3;
	_DL = 255;
	_DL -= _AL;
	col = _DL;
	_asm {
		mov	al, pid_current
		mov	ah, 0
		imul	ax, ax, 3
		db	02h, 0D2h
		and	dl, 255
		db	08Bh, 0D8h
		mov	byte ptr Palettes[bx], dl
		mov	al, pid_current
		mov	ah, 0
		imul	ax, ax, 3
		mov	dl, byte ptr [bp-4]
		db	08Bh, 0D8h
		mov	byte ptr Palettes[bx+1], dl
		mov	al, pid_current
		mov	ah, 0
		imul	ax, ax, 3
		db	08Bh, 0D8h
		mov	byte ptr Palettes[bx+2], dl
	}
	palette_changed = true;

active_done:
	egc_on();
	if(frame < 64) {
		return;
	}
	if(frame >= 128) {
		return;
	}
	shift_x_backup = playfield_fg_shift_x[pid_current];
	playfield_fg_shift_x[pid_current] = 0;
	ellen_bomb_186C3();
	playfield_fg_shift_x[pid_current] = shift_x_backup;
}
#pragma warn .aus

extern "C" void pascal far kana_bomb(void)
{
	uint8_t frame;
	uint8_t col;
	register screen_x_t left;
	register screen_y_t top;

	if(bomb_flag[pid_current] == BF_INACTIVE) {
		return;
	}
	frame = bomb_frame[pid_current];
	if(frame < 64) {
		_AL = 64;
		_AL -= frame;
		col = _AL;
		_AH = 0;
		_asm {
			push	ax
			push	word ptr pid_current
			call	far ptr SUB_A3D2
		}
		if((frame % 8) == 0) {
			SUB_CE0C(TO_SP(144), TO_SP(184), pid_current);
		}
		angle_1FBD4 = 0;
		return;
	}

	if(frame < 128) {
		egc_off();
		if((frame & 3) < 2) {
			snd_se_play(10);
			_asm {
				push	160
				push	word ptr pid_current
				call	far ptr SUB_A3D2
			}
			playfield_fg_shift_x[pid_current] = 4;
		} else {
			playfield_fg_shift_x[pid_current] = -4;
			_asm {
				push	0
				push	word ptr pid_current
				call	far ptr SUB_A3D2
			}
		}

		if((frame % 4) == 0) {
			left = polar(144, 144, CosTable8[angle_1FBD4]);
			top = polar(184, 144, SinTable8[angle_1FBD4]);
			SUB_CDBD(TO_SP(left), TO_SP(top), pid_current);
			angle_1FBD4 = (0x80 - angle_1FBD4);
			left = polar(144, 144, CosTable8[angle_1FBD4]);
			top = polar(184, 144, SinTable8[angle_1FBD4]);
			SUB_CDBD(TO_SP(left), TO_SP(top), pid_current);
			angle_1FBD4 = (0x80 - angle_1FBD4);
			angle_1FBD4 += 0x10;
		}

		left = PLAYFIELD_LEFT;
		if(pid_current != 0) {
			left += PLAYFIELD_W_BORDERED;
		}
		mrs_put_noalpha_8(
			left, PLAYFIELD_TOP, (pid_current + 2), (_AX = pid_current)
		);
		egc_on();
		return;
	}

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

extern "C" void pascal far kotohime_bomb(void)
{
	uint8_t frame;
	uint8_t col;
	register subpixel_t x;
	register int i;

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
		_AH = 0;
		_asm { imul ax, ax, 3; }
		col = _AL;
		Palettes[pid_current].c.r = (col + 64);
		Palettes[pid_current].c.g = (col + 32);
		Palettes[pid_current].c.b = col;
		palette_changed = true;
		if((frame % 8) == 0) {
			_AL = frame;
			_AH = 0;
			_asm { imul ax, ax, 48h; }
			_DX = (TO_SP(PLAYFIELD_W) - 72);
			_DX -= _AX;
			x = _DX;
			_AL = frame;
			_AH = 0;
			_asm { imul ax, ax, 5Ch; }
			_AX += 0x5C;
			word_1FE56 = _AX;
			_asm {
				push	dx
				push	ax
				mov	al, pid_current
				mov	ah, 0
				push	ax
				call	far ptr SUB_CDBD
			}
		}
	} else if(frame < 128) {
		palette_changed = true;
		if((frame & 3) < 2) {
			snd_se_play(10);
			// Replay Patch: Reduce Kotohime's 35 Hz full-screen brightness
			// pulse without changing this position-dependent code segment's
			// size. The side-field animation and shake remain intact.
			PaletteTone = 100;
			palette_changed = true;
			playfield_fg_shift_x[pid_current] = 4;
		} else {
			playfield_fg_shift_x[pid_current] = -4;
			PaletteTone = 100;
			palette_changed = true;
		}

		if((frame % 8) == 0) {
			x = 0;
			i = ((frame % 0x10) / 2);
			while(x <= TO_SP(PLAYFIELD_W)) {
				SUB_CDBD(x, word_1FE56, pid_current);
				x += TO_SP(96);
				i++;
			}
			word_1FE56 -= TO_SP(46);
		}

		x = PLAYFIELD_LEFT;
		if(pid_current != 0) {
			x += PLAYFIELD_W_BORDERED;
		}
		mrs_put_noalpha_8(
			x, PLAYFIELD_TOP, (pid_current + 2), (_AX = pid_current)
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
		_AL = col;
		_AL += _AL;
		col = _AL;
		Palettes[pid_current].c.r = col;
		Palettes[pid_current].c.g = _DL;
		Palettes[pid_current].c.b = frame;
		palette_changed = true;
	}

	egc_on();
}
