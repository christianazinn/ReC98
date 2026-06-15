#pragma codeseg main_03_TEXT

#include "codegen.hpp"
#include "libs/sprite16/sprite16.h"
#include "platform.h"
#include "th01/math/subpixel.hpp"
#include "th03/main/player/cur.hpp"
#include "th03/main/player/gba.hpp"
#include "th03/main/playfld.hpp"
#include "th03/main/round.hpp"
#include "th03/main/sprite16.hpp"
#include "th03/sprites/main_s16.hpp"

extern "C" subpixel_t word_1F33E;
extern "C" subpixel_t word_1F340;
extern "C" uint16_t word_1F3B0;
extern "C" sprite16_offset_t sprite_1F34C;
extern "C" uint8_t byte_1F34E;
extern "C" uint8_t byte_1F34F;
extern "C" uint8_t byte_1F353;

extern "C" void pascal near sub_F58C(void);

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

extern "C" void pascal near marisa_FA71(screen_x_t distance)
{
	screen_y_t top;
	register screen_x_t distance_reg = distance;
	pid_t pid_other = (1 - pid_current);
	register screen_x_t left;

	sprite16_put_size.w.v = (176 / 16);
	sprite16_put_size.h = 48;
	left = (playfield_fg_x_to_screen(word_1F33E, pid_other) - 88);
	top = ((word_1F340 >> 4) - 32);

	_asm {
		mov ah, SPRITE16_SET_MASK
		mov dx, 0AAAAh
		int SPRITE16
	}
	sprite16_put((left - distance_reg), top, sprite_1F34C);

	_asm {
		mov ah, SPRITE16_SET_MASK
		mov dx, 05555h
		int SPRITE16
	}
	sprite16_put((left + distance_reg), top, sprite_1F34C);

	_asm {
		mov ah, SPRITE16_SET_MASK
		mov dx, 0FFFFh
		int SPRITE16
	}
}

extern "C" void pascal far gba_boss_render_marisa(void)
{
	pid_t pid_other;

	if(pid_current != gba_boss_launched_by) {
		return;
	}

	pid_other = (1 - pid_current);
	if(pid_other == 0) {
		sprite16_clip.left = PLAYFIELD1_CLIP_LEFT;
		sprite16_clip.right = PLAYFIELD1_CLIP_RIGHT;
	} else {
		sprite16_clip.left = PLAYFIELD2_CLIP_LEFT;
		sprite16_clip.right = PLAYFIELD2_CLIP_RIGHT;
	}

	if(byte_1F34F == 0) {
		marisa_FA71(200 - (word_1F3B0 * 2));
		return;
	}
	if(byte_1F34F != 0xFF) {
		marisa_F9A6();
		return;
	}
	sub_F58C();
}
