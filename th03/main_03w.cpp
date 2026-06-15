#pragma codeseg main_03_TEXT

#include "libs/master.lib/master.hpp"
#include "libs/sprite16/sprite16.h"
#include "platform.h"
#include "th01/math/subpixel.hpp"
#include "th03/main/player/cur.hpp"
#include "th03/main/playfld.hpp"
#include "th03/main/sprite16.hpp"

extern "C" subpixel_t word_1F33E;
extern "C" subpixel_t word_1F340;
extern "C" sprite16_offset_t sprite_1F34C;
extern "C" uint8_t byte_1F34E;
extern "C" uint8_t byte_1F353;
extern "C" uint8_t byte_1F354;

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
