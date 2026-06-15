#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "libs/sprite16/sprite16.h"
#include "platform.h"
#include "th02/snd/snd.h"
#include "th03/formats/mrs.hpp"
#include "th03/hardware/palette.hpp"
#include "th03/main/player/bomb.hpp"
#include "th03/main/player/cur.hpp"
#include "th03/main/playfld.hpp"
#include "th03/math/polar.hpp"
#include "th03/math/vector.hpp"
#include "x86real.h"

extern "C" uint8_t angles_1DCEC[];

extern "C" void pascal far SUB_CE5B(subpixel_t x, subpixel_t y, uint16_t pid);

extern "C" void pascal far yumemi_bomb(void)
{
	screen_x_t tri_x[4];
	screen_y_t tri_y[3];
	uint8_t fade_frame[2];
	register int i;
	register screen_x_t radius;

#define fade  fade_frame[0]
#define frame fade_frame[1]

	if(bomb_flag[pid_current] == BF_INACTIVE) {
		return;
	}
	egc_off();
	frame = bomb_frame[pid_current];
	if(frame <= 64) {
		if(frame < 32) {
			radius = (224 - (frame * 3));
			grcg_setcolor(GC_RMW, 8);
		} else {
			radius = 0x80;
			grcg_setcolor(GC_RMW, 7);
		}
		if(pid_current == 0) {
			grc_setclip(16, 8, 303, 191);
			tri_x[3] = 0x10;
		} else {
			grc_setclip(336, 8, 623, 191);
			tri_x[3] = (PLAYFIELD_LEFT + PLAYFIELD_W_BORDERED);
		}

		i = 0;
		while(i < 3) {
			tri_x[i] = (
				polar(144, radius, CosTable8[angles_1DCEC[i]]) +
				tri_x[3]
			);
			tri_y[i] = (
				polar(200, radius, SinTable8[angles_1DCEC[i]]) / 2
			);
			i++;
		}
		grcg_triangle(
			tri_x[0], tri_y[0], tri_x[1], tri_y[1], tri_x[2], tri_y[2]
		);
		if(frame > 32) {
			radius = (320 - (frame * 3));
			grcg_setcolor(GC_RMW, 8);
			i = 3;
			while(i < 6) {
				tri_x[i - 3] = (
					polar(144, radius, CosTable8[angles_1DCEC[i]]) +
					tri_x[3]
				);
				tri_y[i - 3] = (
					polar(200, radius, SinTable8[angles_1DCEC[i]]) / 2
				);
				i++;
			}
			grcg_triangle(
				tri_x[0], tri_y[0], tri_x[1], tri_y[1], tri_x[2], tri_y[2]
			);
		}
		grc_setclip(0, 0, (RES_X - 1), (SPRITE16_RES_Y - 1));
		grcg_off();
		fade = (frame << 2);
		goto palette_fade;
	}

	if(frame < 144) {
		if((frame & 7) == 0) {
			snd_se_play(5);
			PaletteTone = 140;
			palette_show();
			SUB_CE5B(TO_SP(144), TO_SP(184), pid_current);
		} else {
			PaletteTone = 60;
			palette_show();
		}

		fade = (frame % 8);
		if((fade & 3) < 2) {
			playfield_fg_shift_x[pid_current] = 4;
		} else {
			playfield_fg_shift_x[pid_current] = -4;
		}
		mrs_put_noalpha_8(
			((pid_current * PLAYFIELD_W_BORDERED) + PLAYFIELD_LEFT),
			PLAYFIELD_TOP,
			(pid_current + 2),
			(_AX = pid_current)
		);
	} else {
		PaletteTone = 100;
		palette_show();
		playfield_fg_shift_x[pid_current] = 0;
		_AL = frame;
		_AL <<= 3;
		_DL = 255;
		_DL -= _AL;
		fade = _DL;

palette_fade:
		Palettes[pid_current].c.r = fade;
		Palettes[pid_current].c.g = _DL;
		Palettes[pid_current].c.b = _DL;
		palette_changed = true;
	}

	egc_on();

#undef frame
#undef fade
}
