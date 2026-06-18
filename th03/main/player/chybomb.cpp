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
#include "x86real.h"

extern "C" uint8_t byte_20E92[];

extern "C" void far sub_B39E(void);
extern "C" void far SUB_A3D2(void);
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
