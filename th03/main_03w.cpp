#pragma codeseg main_03_TEXT

#include "libs/master.lib/master.hpp"
#include "libs/sprite16/sprite16.h"
#include "platform.h"
#include "th01/math/subpixel.hpp"
#include "th03/main/player/cur.hpp"
#include "th03/main/playfld.hpp"
#include "th03/main/round.hpp"
#include "th03/main/sprite16.hpp"
#include "th03/math/polar.hpp"

extern "C" subpixel_t word_1F33E;
extern "C" subpixel_t word_1F340;
extern "C" uint16_t word_1F3B0;
extern "C" sprite16_offset_t sprite_1F34C;
extern "C" uint8_t pid_PID_so_attack;
extern "C" uint8_t byte_1F34E;
extern "C" uint8_t byte_1F353;
extern "C" uint8_t byte_1F354;
extern "C" uint8_t byte_1F355;

extern "C" void pascal near kana_13174(void)
{
	screen_y_t top;
	register sprite16_offset_t sprite_offset;
	register screen_x_t left;

	sprite16_put_size.w.v = (128 / 16);
	sprite16_put_size.h = 40;
	left = (playfield_fg_x_to_screen(word_1F33E, (1 - pid_current)) - 64);
	top = ((word_1F340 >> 4) - 24);
	sprite_offset = sprite_1F34C;
	if(byte_1F34E != 0) {
		sprite_offset += 0x10;
	}
	sprite16_put(left, top, sprite_offset);

	sprite16_put_size.w.v = (64 / 16);
	sprite16_put_size.h = 32;
	left += 32;
	if(byte_1F353 == 1) {
		sprite16_put(left, top, (sprite_1F34C + (40 * ROW_SIZE)));
		return;
	}
	if(byte_1F353 == 2) {
		sprite_offset = (sprite_1F34C + (40 * ROW_SIZE));
		if(byte_1F354 >= 8) {
			if(byte_1F354 < 0x10) {
				sprite_offset += 8;
			} else if(byte_1F354 < 0x18) {
				sprite_offset += 0x10;
			} else {
				sprite_offset += 0x18;
			}
		}
		sprite16_put(left, top, sprite_offset);
	}
}

extern "C" void pascal near kana_13223(void)
{
	screen_y_t top;
	sprite16_offset_t sprite_offset;
	subpixel_t radius;
	uint8_t angle;
	register int i;
	register screen_x_t left;

	radius = ((200 << SUBPIXEL_BITS) - ((word_1F3B0 << 1) << 4));
	sprite16_put_size.w.v = (32 / 16);
	sprite16_put_size.h = 16;
	if((word_1F3B0 < 0x20) && (round_frame_mod2 != 0)) {
		return;
	}

	sprite_offset = (
		pid_PID_so_attack + ((8 * ROW_SIZE) + (32 / BYTE_DOTS))
	);
	sprite_offset = (sprite_offset + ((((word_1F3B0 >> 2) & 3) << 5) * 40));
	angle = byte_1F355;
	angle = (0 - angle);
	i = 0;
	while(i < 0x10) {
		left = polar(word_1F33E, radius, CosTable8[angle]);
		top = polar(word_1F340, radius, SinTable8[angle]);
		left = (playfield_fg_x_to_screen(left, (1 - pid_current)) - 16);
		top = (top >> 4);
		sprite16_put(left, top, sprite_offset);
		i++;
		angle += 0x10;
	}
}
