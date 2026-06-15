#pragma codeseg main_03_TEXT

#include "libs/master.lib/master.hpp"
#include "libs/sprite16/sprite16.h"
#include "platform.h"
#include "th01/math/subpixel.hpp"
#include "th03/main/player/cur.hpp"
#include "th03/main/player/gba.hpp"
#include "th03/main/playfld.hpp"
#include "th03/main/sprite16.hpp"
#include "th03/math/polar.hpp"

extern "C" subpixel_t word_1F33E;
extern "C" subpixel_t word_1F340;
extern "C" subpixel_t word_1F356;
extern "C" uint16_t word_1F3B0;
extern "C" uint8_t pid_PID_so_attack;
extern "C" uint8_t byte_1F353;
extern "C" uint8_t byte_1F355;

extern "C" void pascal near kotohime_11FE4(int count)
{
	screen_y_t top;
	sprite16_offset_t sprite_offset;
	subpixel_t radius;
	uint8_t angle;
	register int i;
	register screen_x_t left;

	radius = word_1F356;
	sprite_offset = (pid_PID_so_attack + ((16 * ROW_SIZE) + (16 / BYTE_DOTS)));
	if(byte_1F353 == 2) {
		if((word_1F3B0 & 3) < 2) {
			sprite_offset += (32 * ROW_SIZE);
		}
	}

	sprite16_put_size.w.v = (32 / 16);
	sprite16_put_size.h = 16;
	for(i = 0; i < count; i++) {
		angle = (((i << 8) / count) + byte_1F355);
		left = polar(word_1F33E, radius, CosTable8[angle]);
		top = polar(word_1F340, radius, SinTable8[angle]);
		left = (playfield_fg_x_to_screen(left, (1 - pid_current)) - 16);
		top = (top >> 4);
		sprite16_put(left, top, sprite_offset);
	}
}
