#pragma codeseg main_03_TEXT

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

extern "C" void pascal near mima_10053(void)
{
	sprite16_offset_t so;
	pid_t pid_other = (1 - pid_current);
	register screen_x_t left;
	register screen_y_t top;

	sprite16_put_size.w.v = (144 / 16);
	sprite16_put_size.h = 56;
	left = (playfield_fg_x_to_screen(word_1F33E, pid_other) - 72);
	top = ((word_1F340 >> 4) - 40);
	sprite16_put(left, top, sprite_1F34C);

	if(byte_1F34E != 0) {
		sprite16_put_size.w.v = (64 / 16);
		sprite16_put_size.h = 40;
		left += 32;
		top += 16;
		sprite16_put(left, top, (sprite_1F34C + 0x12));
		return;
	}

	if(byte_1F353 == 2) {
state_2:
		sprite16_put_size.w.v = (16 / 16);
		sprite16_put_size.h = 8;
		so = (sprite_1F34C + 0x1A);
		sprite16_put((left + 48), (top + 32), so);

		sprite16_put_size.w.v = (32 / 16);
		sprite16_put_size.h = 16;
		left += 80;
		so = (sprite_1F34C + ((47 * ROW_SIZE) + (544 / BYTE_DOTS)));
		if(byte_1F354 >= 0x10) {
			so += 4;
			if(byte_1F354 >= 0x20) {
				byte_1F353 = 3;
			}
		}
		sprite16_put(left, top, so);
		byte_1F354++;
		return;
	}

	if(byte_1F353 == 3) {
		sprite16_put_size.w.v = (80 / 16);
		sprite16_put_size.h = 24;
		sprite16_put(
			(left + 32),
			(top + 16),
			(sprite_1F34C + ((40 * ROW_SIZE) + (144 / BYTE_DOTS)))
		);
		return;
	}

	if(byte_1F353 == 4) {
		sprite16_put_size.w.v = (16 / 16);
		sprite16_put_size.h = 8;
		so = (sprite_1F34C + 0x1A);
		sprite16_put((left + 48), (top + 32), so);
		return;
	}

	if(byte_1F353 == 5) {
		byte_1F354 = 0;
		goto state_2;
	}
}
