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
extern "C" int16_t word_1DE12[];
extern "C" int16_t word_1DE24[];

extern "C" void pascal near ellen_11814(int frame)
{
	screen_x_t left;
	screen_y_t top;
	sprite16_offset_t so;
	register int i;

	i = frame;
	so = (sprite_1F34C + (word_1DE12[i] * 0x28) + 0xFB10);
	sprite16_put_size.w.v = (96 / 16);
	sprite16_put_size.h = (word_1DE24[i] / 2);
	left = (
		playfield_fg_x_to_screen(word_1F33E, (1 - pid_current)) - 48
	);
	top = ((word_1F340 >> 4) + word_1DE12[i] - 40);
	sprite16_put(left, top, so);
}
