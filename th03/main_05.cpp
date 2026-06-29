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
#include "th03/main/v_colors.hpp"
#include "th03/math/randring.hpp"
#include "x86real.h"

extern "C" uint8_t byte_20E92[];
extern "C" subpixel_t word_220EC;

extern "C" void far sub_B39E(void);
extern "C" void far SUB_A3D2(void);
extern "C" void pascal far SUB_CDBD(subpixel_t x, subpixel_t y, uint16_t pid);

extern "C" void pascal far rikako_bomb(void)
{
	uint8_t frame;
	uint8_t col;
	register screen_x_t left;

	if(bomb_flag[pid_current] == BF_INACTIVE) {
		return;
	}
	frame = bomb_frame[pid_current];
	egc_off();
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
		col = _AL;
		_AH = 0;
		_asm {
			push	ax
			push	word ptr pid_current
			call	far ptr SUB_A3D2
		}
		word_220EC = 0;
	} else if(frame < 128) {
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
		grcg_setcolor(GC_RMW, V_WHITE);

		_AX = TO_SP(144);
		_AX -= word_220EC;
		left = playfield_fg_x_to_screen(_AX, pid_current);
		grcg_vline(_AX, 8, 192);

		_AX = word_220EC;
		_AX += TO_SP(144);
		left = playfield_fg_x_to_screen(_AX, pid_current);
		grcg_vline(_AX, 8, 192);

		_AX = word_220EC;
		_AX += _AX;
		_DX = TO_SP(144);
		_DX -= _AX;
		left = playfield_fg_x_to_screen(_DX, pid_current);
		grcg_vline(_AX, 8, 192);

		_AX = word_220EC;
		_AX += _AX;
		_AX += TO_SP(144);
		left = playfield_fg_x_to_screen(_AX, pid_current);
		grcg_vline(_AX, 8, 192);

		word_220EC += 0x41;
		if(word_220EC >= TO_SP(72)) {
			word_220EC = 0;
		}
		grcg_off();

		if((frame % 8) == 0) {
			left = randring_far_next16_and(1023);
			while(left <= TO_SP(PLAYFIELD_W)) {
				SUB_CDBD(left, TO_SP(368), pid_current);
				left += TO_SP(96);
			}
		}
		if((frame % 4) == 0) {
			snd_se_play(5);
		}
	} else {
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
