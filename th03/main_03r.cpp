#pragma codeseg main_03_TEXT

#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "libs/sprite16/sprite16.h"
#include "platform.h"
#include "th01/math/subpixel.hpp"
#include "th03/main/player/cur.hpp"
#include "th03/main/player/gba.hpp"
#include "th03/main/playfld.hpp"
#include "th03/main/sprite16.hpp"

extern "C" subpixel_t word_1F33E;
extern "C" subpixel_t word_1F340;
extern "C" subpixel_t word_20E50;
extern "C" subpixel_t word_20E52;
extern PlayfieldPoint point_1F342;
extern "C" uint16_t word_1F356;
extern "C" uint16_t word_1F3B0;
extern "C" uint8_t byte_1F35E[];
extern "C" sprite16_offset_t sprite_1F34C;
extern "C" uint8_t byte_1F34E;
extern "C" uint8_t byte_1F34F;
extern "C" uint8_t byte_1F353;
extern "C" uint8_t byte_1F354;

extern "C" void pascal near sub_F58C(void);

extern "C" void pascal near yumemi_108CA(void)
{
	sprite16_offset_t so;
	screen_x_t target_left;
	screen_y_t target_top;
	pid_t pid_other = (1 - pid_current);
	register screen_x_t left;
	register screen_y_t top;

	sprite16_put_size.w.v = (112 / 16);
	sprite16_put_size.h = 56;
	left = (playfield_fg_x_to_screen(word_1F33E, pid_other) - 56);
	top = ((word_1F340 >> 4) - 40);
	sprite16_put(left, top, sprite_1F34C);

	if(byte_1F353 != 0) {
		sprite16_put_size.w.v = (64 / 16);
		sprite16_put_size.h = 32;
		so = (sprite_1F34C + 0x0E);
		if(byte_1F353 == 2) {
			so += (32 * ROW_SIZE);
		}
		sprite16_put((left + 32), top, so);
	} else {
		if(byte_1F34E != 0) {
			sprite16_put_size.w.v = (80 / 16);
			sprite16_put_size.h = 48;
			left += 16;
			sprite16_put(left, top, (sprite_1F34C + 0x16));
		}
	}

	if((word_1F356 != 0) || (byte_1F354 != 0)) {
		if(pid_current != 0) {
			grc_setclip(16, 8, 303, 191);
		} else {
			grc_setclip(336, 8, 623, 191);
		}
		egc_off();
		grcg_setcolor(GC_RMW, 10);
		left = playfield_fg_x_to_screen(word_20E50, pid_other);
		top = ((word_20E52 >> 5) + 8);
		if(byte_1F354 == 0) {
			grcg_circle(left, top, word_1F356);
		} else {
			target_left = playfield_fg_x_to_screen(point_1F342.x.v, pid_other);
			target_top = ((point_1F342.y.v >> 5) + 8);
			grcg_line(left, top, target_left, target_top);
		}
		grcg_off();
		egc_on();
		grc_setclip(0, 0, (RES_X - 1), (SPRITE16_RES_Y - 1));
	}
}

extern "C" void pascal near yumemi_10A17(void)
{
	screen_y_t top;
	pid_t pid_other;
	uint8_t wipe_frame;
	register screen_x_t left;
	register screen_x_t edge;

	pid_other = (1 - pid_current);
	_asm {
		db 31h, 0D2h
		mov ah, SPRITE16_SET_OVERLAP
		int SPRITE16
	}
	sprite16_put_size.w.v = (112 / 16);
	sprite16_put_size.h = 56;
	top = ((word_1F340 >> 4) - 40);

	if(word_1F3B0 < 0x18) {
		left = playfield_fg_x_to_screen(TO_SP(-104), pid_other);
		edge = playfield_fg_x_to_screen(
			(((word_1F3B0 << 3) << SUBPIXEL_BITS) - TO_SP(104)),
			pid_other
		);
		while(left <= edge) {
			sprite16_put(left, top, sprite_1F34C);
			left += 8;
		}
	} else if(word_1F3B0 < 0x30) {
		left = playfield_fg_x_to_screen(
			((static_cast<uint16_t>(
				wipe_frame = (static_cast<uint8_t>(word_1F3B0) - 0x18u)
			) << 3) << SUBPIXEL_BITS) + TO_SP(-104),
			pid_other
		);
		edge = playfield_fg_x_to_screen(TO_SP(88), pid_other);
		while(left <= edge) {
			sprite16_put(left, top, sprite_1F34C);
			left += 8;
		}
	} else if(word_1F3B0 < 0x48) {
		wipe_frame = (static_cast<uint8_t>(word_1F3B0) - 0x30u);
		left = playfield_fg_x_to_screen(TO_SP(272), pid_other);
		edge = playfield_fg_x_to_screen(
			(TO_SP(272) - ((static_cast<uint16_t>(wipe_frame) << 3) << SUBPIXEL_BITS)),
			pid_other
		);
		while(left >= edge) {
			sprite16_put(left, top, sprite_1F34C);
			left -= 8;
		}
		left = playfield_fg_x_to_screen(TO_SP(88), pid_other);
		sprite16_put(left, top, sprite_1F34C);
	} else if(word_1F3B0 < 0x60) {
		left = playfield_fg_x_to_screen(
			(TO_SP(272) - ((static_cast<uint16_t>(
				wipe_frame = (static_cast<uint8_t>(word_1F3B0) - 0x48u)
			) << 3) << SUBPIXEL_BITS)),
			pid_other
		);
		edge = playfield_fg_x_to_screen(TO_SP(88), pid_other);
		while(left >= edge) {
			sprite16_put(left, top, sprite_1F34C);
			left -= 8;
		}
	}

	_asm {
		mov dx, OVERLAP_CLEAR
		mov ah, SPRITE16_SET_OVERLAP
		int SPRITE16
	}
}

extern "C" void pascal far gba_boss_render_yumemi(void)
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
		yumemi_10A17();
		return;
	}
	if(byte_1F34F != 0xFF) {
		yumemi_108CA();
		return;
	}
	sub_F58C();
}

extern "C" void pascal far reimu_10BFE(uint16_t slot)
{
	register uint8_t near *record = &byte_1F35E[slot * 32];

	*reinterpret_cast<subpixel_t near *>(record + 0x00) = TO_SP(144);
	*reinterpret_cast<subpixel_t near *>(record + 0x02) = TO_SP(80);
	*reinterpret_cast<uint16_t near *>(record + 0x08) = 0xFFE0;
	*reinterpret_cast<uint16_t near *>(record + 0x0A) = 0;
	record[0x12] = 0;
	record[0x10] = 0;
	record[0x11] = 0;
	*reinterpret_cast<uint16_t near *>(record + 0x0C) = 0x006E;
	record[0x13] = 0;
	record[0x15] = 0;
	*reinterpret_cast<uint16_t near *>(record + 0x0E) = 0x028C;
	if(slot != 0) {
		*reinterpret_cast<uint16_t near *>(record + 0x0E) += 0x28;
	}
}
