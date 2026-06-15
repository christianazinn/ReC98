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
#include "th03/main/v_colors.hpp"
#include "th03/math/polar.hpp"

extern "C" subpixel_t word_1F33E;
extern "C" subpixel_t word_1F340;
extern "C" uint16_t word_1F3B0;
extern "C" uint16_t word_1F356;
extern "C" sprite16_offset_t sprite_1F34C;
extern "C" uint8_t byte_1F34E;
extern "C" uint8_t byte_1F34F;
extern "C" uint8_t byte_1F353;
extern "C" uint8_t byte_1F354;
extern "C" uint8_t byte_1F355;
extern "C" uint8_t byte_1F35E[];

extern "C" void pascal near chiyuri_12B38(int col);
extern "C" void pascal near sub_F58C(void);
extern "C" void pascal far sub_A3A8(uint8_t pid);

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

extern "C" void pascal near chiyuri_12A10(void)
{
	screen_y_t top;
	register sprite16_offset_t sprite_offset;
	register screen_x_t left;

	if(byte_1F353 == 0) {
		sprite16_put_size.w.v = (128 / 16);
		sprite16_put_size.h = 64;
		left = (playfield_fg_x_to_screen(word_1F33E, (1 - pid_current)) - 64);
		top = ((word_1F340 >> 4) - 48);
		sprite_offset = sprite_1F34C;
		sprite16_put(left, top, sprite_offset);
		left += 48;
		top += 32;
		if(byte_1F34E != 0) {
			sprite16_put_size.w.v = (48 / 16);
			sprite16_put_size.h = 40;
			sprite_offset += ((23 * ROW_SIZE) + (576 / BYTE_DOTS));
		} else {
			if(byte_1F354 == 0) {
				return;
			}
			sprite16_put_size.w.v = (32 / 16);
			sprite16_put_size.h = 24;
			sprite_offset += ((static_cast<uint16_t>(byte_1F354) << 2) + 0x0C);
		}
		sprite16_put(left, top, sprite_offset);
		return;
	}

	if(byte_1F353 >= 0x18) {
		chiyuri_12B38(6);
		return;
	}
	if(byte_1F353 >= 0x14) {
		chiyuri_12B38(5);
		return;
	}
	if(byte_1F353 >= 0x0C) {
		chiyuri_12B38(2);
		return;
	}
	if(byte_1F353 >= 8) {
		chiyuri_12B38(5);
		return;
	}
	if(byte_1F353 >= 4) {
		chiyuri_12B38(6);
		return;
	}
	chiyuri_12B38(V_WHITE);
}

extern "C" void pascal near chiyuri_12ADD(void)
{
	register int frame;

	if((round_frame_mod2 != 0) && (word_1F3B0 < 0x40)) {
		return;
	}

	frame = word_1F3B0;
	if(frame < 0x20) {
		word_1F356 = (256 - (frame << 3));
		byte_1F355 += 4;
	} else if(frame < 0x40) {
		word_1F356 = (512 - (frame << 3));
		byte_1F355 -= 4;
	} else {
		word_1F356 = (768 - (frame << 3));
		byte_1F355 += 4;
	}
	chiyuri_1295E();
}

extern "C" void pascal near chiyuri_12B38(int col)
{
	screen_x_t left;
	screen_y_t top;
	sprite16_offset_t sprite_offset;

	sprite16_put_size.w.v = (128 / 16);
	sprite16_put_size.h = 64;
	left = (playfield_fg_x_to_screen(word_1F33E, (1 - pid_current)) - 64);
	top = ((word_1F340 >> 4) - 48);
	sprite_offset = sprite_1F34C;
	sprite16_mono(true);
	sprite16_mono_color(col);
	sprite16_put(left, top, sprite_offset);
	_AH = SPRITE16_SET_MONO;
	__emit__(0x31, 0xD2); // XOR DX, DX
	geninterrupt(SPRITE16);
}

extern "C" void pascal far gba_boss_render_chiyuri(void)
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
		chiyuri_12ADD();
		return;
	}
	if(byte_1F34F != 0xFF) {
		chiyuri_12A10();
		return;
	}
	sub_F58C();
	if(gba_boss_launched_by == PID_NONE) {
		sub_A3A8(1 - pid_current);
	}
}

extern "C" void pascal far kana_12BFB(uint16_t slot)
{
	register uint8_t near *record = &byte_1F35E[slot * 32];

	*reinterpret_cast<subpixel_t near *>(record + 0x00) = TO_SP(144);
	*reinterpret_cast<subpixel_t near *>(record + 0x02) = TO_SP(80);
	*reinterpret_cast<uint16_t near *>(record + 0x08) = 0xFFE0;
	*reinterpret_cast<uint16_t near *>(record + 0x0A) = 0;
	record[0x12] = 0;
	record[0x10] = 0;
	record[0x11] = 0;
	*reinterpret_cast<uint16_t near *>(record + 0x0C) = 0x0064;
	record[0x13] = 0;
	*reinterpret_cast<uint16_t near *>(record + 0x18) = 0;
	*reinterpret_cast<uint16_t near *>(record + 0x0E) = 0x0288;
	if(slot != 0) {
		*reinterpret_cast<uint16_t near *>(record + 0x0E) += 0x28;
	}
	record[0x15] = 0;
}
