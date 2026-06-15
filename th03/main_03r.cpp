#pragma codeseg main_03_TEXT

#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "libs/sprite16/sprite16.h"
#include "platform.h"
#include "th01/math/subpixel.hpp"
#include "th03/main/player/cur.hpp"
#include "th03/main/playfld.hpp"
#include "th03/main/sprite16.hpp"

extern "C" subpixel_t word_1F33E;
extern "C" subpixel_t word_1F340;
extern "C" subpixel_t word_20E50;
extern "C" subpixel_t word_20E52;
extern PlayfieldPoint point_1F342;
extern "C" uint16_t word_1F356;
extern "C" sprite16_offset_t sprite_1F34C;
extern "C" uint8_t byte_1F34E;
extern "C" uint8_t byte_1F353;
extern "C" uint8_t byte_1F354;

extern "C" void pascal near yumemi_108CA(void)
{
	sprite16_offset_t so;
	screen_x_t target_left;
	screen_y_t target_top;
	pid_t pid_other = (1 - pid_current);
	register screen_x_t left;
	register screen_y_t top;

	sprite16_put_size.w.v = (112 / 16);
	sprite16_put_size.h = 56;
	left = (playfield_fg_x_to_screen(word_1F33E, pid_other) - 56);
	top = ((word_1F340 >> 4) - 40);
	sprite16_put(left, top, sprite_1F34C);

	if(byte_1F353 != 0) {
		sprite16_put_size.w.v = (64 / 16);
		sprite16_put_size.h = 32;
		so = (sprite_1F34C + 0x0E);
		if(byte_1F353 == 2) {
			so += (32 * ROW_SIZE);
		}
		sprite16_put((left + 32), top, so);
	} else {
		if(byte_1F34E != 0) {
			sprite16_put_size.w.v = (80 / 16);
			sprite16_put_size.h = 48;
			left += 16;
			sprite16_put(left, top, (sprite_1F34C + 0x16));
		}
	}

	if((word_1F356 != 0) || (byte_1F354 != 0)) {
		if(pid_current != 0) {
			grc_setclip(16, 8, 303, 191);
		} else {
			grc_setclip(336, 8, 623, 191);
		}
		egc_off();
		grcg_setcolor(GC_RMW, 10);
		left = playfield_fg_x_to_screen(word_20E50, pid_other);
		top = ((word_20E52 >> 5) + 8);
		if(byte_1F354 == 0) {
			grcg_circle(left, top, word_1F356);
		} else {
			target_left = playfield_fg_x_to_screen(point_1F342.x.v, pid_other);
			target_top = ((point_1F342.y.v >> 5) + 8);
			grcg_line(left, top, target_left, target_top);
		}
		grcg_off();
		egc_on();
		grc_setclip(0, 0, (RES_X - 1), (SPRITE16_RES_Y - 1));
	}
}
