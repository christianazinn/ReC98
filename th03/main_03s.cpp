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
extern "C" uint16_t word_1F3B0;
extern "C" sprite16_offset_t sprite_1F34C;
extern "C" uint8_t byte_1F34E;
extern "C" uint8_t byte_1F353;
extern "C" uint8_t byte_1F3A2;
extern "C" subpixel_t word_23DCA[];
extern "C" subpixel_t word_23DD6[];

extern "C" void pascal near reimu_111A3(void)
{
	screen_x_t left;
	screen_y_t top;
	pid_t pid_other;
	uint8_t frame_quarter;
	register sprite16_offset_t so;
	register int i;

	pid_other = (1 - pid_current);
	sprite16_put_size.w.v = (96 / 16);
	sprite16_put_size.h = 48;
	if(pid_other == 0) {
		sprite16_clip.left = PLAYFIELD1_CLIP_LEFT;
		sprite16_clip.right = PLAYFIELD1_CLIP_RIGHT;
	} else {
		sprite16_clip.left = PLAYFIELD2_CLIP_LEFT;
		sprite16_clip.right = PLAYFIELD2_CLIP_RIGHT;
	}

	so = sprite_1F34C;
	if(byte_1F34E != 0) {
		so += 0x0C;
	}
	left = (playfield_fg_x_to_screen(word_1F33E, pid_other) - 48);
	top = ((word_1F340 >> 4) - 32);
	sprite16_put(left, top, so);

	if(byte_1F353 != 1) {
		return;
	}

	sprite16_put_size.w.v = (48 / 16);
	sprite16_put_size.h = 24;
	so = (sprite_1F34C + 0xFFF4);
	frame_quarter = (word_1F3B0 >> 2);
	if((frame_quarter & 3) == 1) {
		so += 6;
	} else if((frame_quarter & 3) != 0) {
		so += (((frame_quarter & 1) * 6) + (24 * ROW_SIZE));
	}

	for(i = 0; i < static_cast<int>(byte_1F3A2); i++) {
		left = (playfield_fg_x_to_screen(word_23DCA[i], pid_other) - 24);
		top = ((word_23DD6[i] >> 4) - 8);
		sprite16_put(left, top, so);
	}
}
