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
extern "C" uint16_t word_1F356;
extern "C" sprite16_offset_t sprite_1F34C;
extern "C" uint8_t byte_1F354;
extern "C" uint8_t byte_1F355;

extern "C" void pascal near chiyuri_1295E(void)
{
	screen_y_t top;
	sprite16_offset_t sprite_offset;
	int pid_other;
	subpixel_t radius;
	uint8_t angle;
	register int i;
	register screen_x_t left;

	pid_other = (1 - pid_current);
	radius = (word_1F356 << SUBPIXEL_BITS);
	sprite16_put_size.w.v = (32 / 16);
	sprite16_put_size.h = 16;
	sprite_offset = (sprite_1F34C + ((24 * ROW_SIZE) + (128 / BYTE_DOTS)));
	sprite_offset += (static_cast<uint16_t>(byte_1F354) << 2);
	i = 0;
	angle = byte_1F355;
	while(i < 16) {
		left = polar(word_1F33E, radius, CosTable8[angle]);
		top = polar(word_1F340, radius, SinTable8[angle]);
		left = (playfield_fg_x_to_screen(left, pid_other) - 16);
		top = (top >> 4);
		sprite16_put(left, top, sprite_offset);
		i++;
		angle += 0x10;
	}
}
