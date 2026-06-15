#pragma codeseg main_03_TEXT

#include "codegen.hpp"
#include "platform.h"
#include "th01/math/subpixel.hpp"
#include "th03/main/player/cur.hpp"
#include "th03/main/playfld.hpp"
#include "th03/main/round.hpp"
#include "th03/main/sprite16.hpp"
#include "th03/sprites/main_s16.hpp"

extern "C" subpixel_t word_1F33E;
extern "C" subpixel_t word_1F340;
extern "C" sprite16_offset_t sprite_1F34C;
extern "C" uint8_t byte_1F34E;
extern "C" uint8_t byte_1F353;

extern "C" void pascal near marisa_F9A6(void)
{
	sprite16_offset_t so;
	pid_t pid_other = (1 - pid_current);
	register screen_x_t left;
	register screen_y_t top;

	sprite16_put_size.w.v = (176 / 16);
	sprite16_put_size.h = 48;
	left = (playfield_fg_x_to_screen(word_1F33E, pid_other) - 88);
	top = ((word_1F340 >> 4) - 32);
	sprite16_put(left, top, sprite_1F34C);

	if(byte_1F34E != 0) {
		sprite16_put_size.w.v = (48 / 16);
		sprite16_put_size.h = 40;
		left += 64;
		top += 16;
		sprite16_put(left, top, (sprite_1F34C + 0x16));
		return;
	}

	if(byte_1F353 != 1) {
		return;
	}

	if((round_or_result_frame & 3) < 2) {
		sprite16_put_size.w.v = (48 / 16);
		sprite16_put_size.h = 40;
		left += 64;
		top += 16;
		sprite16_put(left, top, (sprite_1F34C + 0x1C));
		left -= 32;
		top += 32;
		so = (sprite_1F34C + (48 * ROW_SIZE));
	} else {
		left += 32;
		top += 48;
		so = (sprite_1F34C + (56 * ROW_SIZE));
	}

	sprite16_put_size.w.v = (112 / 16);
	sprite16_put_size.h = 8;
	sprite16_put(left, top, so);
}
