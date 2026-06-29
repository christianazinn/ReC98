#pragma codeseg main_03_TEXT

#include "libs/master.lib/master.hpp"
#include "libs/sprite16/sprite16.h"
#include "platform.h"
#include "th01/math/subpixel.hpp"
#include "th03/main/player/cur.hpp"
#include "th03/main/player/gba.hpp"
#include "th03/main/playfld.hpp"
#include "th03/main/round.hpp"
#include "th03/main/sprite16.hpp"
#include "th03/math/polar.hpp"

extern "C" subpixel_t word_1F33E;
extern "C" subpixel_t word_1F340;
extern "C" uint16_t word_1F3B0;
extern "C" sprite16_offset_t sprite_1F34C;
extern "C" uint8_t byte_1F34F;
extern "C" uint8_t pid_PID_so_attack;
extern "C" uint8_t byte_1F34E;
extern "C" uint8_t byte_1F354;

extern "C" void pascal near sub_F58C(void);

extern "C" void pascal near rikako_137CF(void)
{
	screen_x_t left;
	screen_y_t top;
	screen_x_t orbit_left;
	screen_y_t orbit_top;
	uint8_t angle;
	register sprite16_offset_t sprite_offset;
	register int i;

	sprite16_put_size.w.v = (96 / 16);
	sprite16_put_size.h = 48;
	left = (playfield_fg_x_to_screen(word_1F33E, (1 - pid_current)) - 48);
	top = ((word_1F340 >> 4) - 32);
	sprite_offset = sprite_1F34C;
	if(byte_1F34E != 0) {
		sprite_offset += 0x0C;
	}
	sprite16_put(left, top, sprite_offset);

	sprite16_put_size.w.v = (16 / 16);
	sprite16_put_size.h = 8;
	angle = byte_1F354;
	left += 40;
	top += 40;
	i = 0;
	while(i < 8) {
		orbit_left = polar(left, 48, CosTable8[angle]);
		orbit_top = polar(top, 48, SinTable8[angle]);
		if((i & 3) != 0) {
			sprite_offset = (
				((i & 3) * 2) + ((72 * ROW_SIZE) - (16 / BYTE_DOTS))
			);
		} else {
			sprite_offset = ((8 * ROW_SIZE) + (32 / BYTE_DOTS));
		}
		sprite16_put(
			orbit_left, orbit_top, (pid_PID_so_attack + sprite_offset)
		);
		i++;
		angle += 0x20;
	}
}

extern "C" void pascal near rikako_138B3(void)
{
	screen_y_t top;
	sprite16_offset_t sprite_offset;
	subpixel_t radius;
	uint8_t angle;
	register int i;
	register screen_x_t left;

	radius = ((200 << SUBPIXEL_BITS) - ((word_1F3B0 << 1) << 4));
	sprite16_put_size.w.v = (48 / 16);
	sprite16_put_size.h = 24;
	if((word_1F3B0 < 0x40) && ((round_or_result_frame & 1) != 0)) {
		return;
	}

	sprite_offset = (pid_PID_so_attack + (24 * ROW_SIZE));
	if((word_1F3B0 & 1) != 0) {
		sprite_offset += (24 * ROW_SIZE);
	}
	_AL = word_1F3B0;
	_AL <<= 2;
	angle = _AL;
	angle = (0 - angle);

	i = 0;
	while(i < 0x10) {
		left = polar(word_1F33E, radius, CosTable8[angle]);
		top = polar(word_1F340, radius, SinTable8[angle]);
		left = (playfield_fg_x_to_screen(left, (1 - pid_current)) - 24);
		top = ((top >> 4) - 8);
		sprite16_put(left, top, sprite_offset);
		i++;
		angle += 0x10;
	}
}

extern "C" void pascal far gba_boss_render_rikako(void)
{
	if(pid_current != gba_boss_launched_by) {
		return;
	}

	if((1 - pid_current) == 0) {
		sprite16_clip.left = PLAYFIELD1_CLIP_LEFT;
		sprite16_clip.right = PLAYFIELD1_CLIP_RIGHT;
	} else {
		sprite16_clip.left = PLAYFIELD2_CLIP_LEFT;
		sprite16_clip.right = PLAYFIELD2_CLIP_RIGHT;
	}

	if(byte_1F34F == 0) {
		rikako_138B3();
		return;
	}
	if(byte_1F34F != 0xFF) {
		rikako_137CF();
		return;
	}
	sub_F58C();
}
