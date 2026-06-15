#pragma codeseg main_03_TEXT

#include "codegen.hpp"
#include "libs/master.lib/master.hpp"
#include "libs/sprite16/sprite16.h"
#include "platform.h"
#include "th01/math/subpixel.hpp"
#include "th02/snd/snd.h"
#include "th03/main/bullet/bullet.hpp"
#include "th03/main/player/cur.hpp"
#include "th03/main/player/gba.hpp"
#include "th03/main/playfld.hpp"
#include "th03/main/sprite16.hpp"
#include "th03/math/polar.hpp"
#include "th03/math/randring.hpp"

extern "C" subpixel_t word_1F33E;
extern "C" subpixel_t word_1F340;
extern "C" uint16_t word_1F3B0;
extern "C" uint16_t word_1F356;
extern "C" uint8_t byte_1F35E[];
extern "C" sprite16_offset_t sprite_1F34C;
extern "C" uint8_t byte_1F34E;
extern "C" uint8_t byte_1F34F;
extern "C" uint8_t byte_1F353;
extern "C" uint8_t byte_1F354;
extern "C" uint8_t byte_1F39F;
extern "C" uint8_t byte_1F3A0;
extern "C" uint8_t byte_20E4C;
extern "C" uint8_t byte_20E4D;
extern "C" uint8_t byte_20E4E;
extern "C" subpixel_t word_20E50;
extern "C" subpixel_t word_20E52;

extern "C" uint16_t far randring_far_next16_raw(void);
extern "C" void pascal far SUB_CDBD(subpixel_t x, subpixel_t y, uint16_t pid);
extern "C" void pascal far SUB_CE5B(subpixel_t x, subpixel_t y, uint16_t pid);
extern "C" void pascal near sub_F58C(void);

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

extern "C" void pascal near mima_10184(screen_x_t radius, uint8_t angle)
{
	register screen_x_t radius_reg = radius;
	screen_x_t center_left;
	screen_y_t center_top;
	screen_y_t top;
	pid_t pid_other = (1 - pid_current);
	register screen_x_t left;

	sprite16_put_size.w.v = (144 / 16);
	sprite16_put_size.h = 56;
	center_left = (playfield_fg_x_to_screen(word_1F33E, pid_other) - 72);
	center_top = ((word_1F340 >> 4) - 40);

	_asm {
		mov ah, SPRITE16_SET_MASK
		mov dx, 0AAAAh
		int SPRITE16
	}
	left = polar(center_left, radius_reg, CosTable8[angle]);
	top = polar(center_top, radius_reg, SinTable8[angle]);
	sprite16_put(left, top, sprite_1F34C);

	_asm {
		mov ah, SPRITE16_SET_MASK
		mov dx, 05555h
		int SPRITE16
	}
	angle += 0x80;
	left = polar(center_left, radius_reg, CosTable8[angle]);
	top = polar(center_top, radius_reg, SinTable8[angle]);
	sprite16_put(left, top, sprite_1F34C);

	_asm {
		mov ah, SPRITE16_SET_MASK
		mov dx, 0FFFFh
		int SPRITE16
	}
}

extern "C" void pascal far gba_boss_render_mima(void)
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
		mima_10184((200 - (word_1F3B0 * 2)), (word_1F3B0 * 3));
		return;
	}
	if(byte_1F34F != 0xFF) {
		mima_10053();
		return;
	}
	sub_F58C();
}

extern "C" void pascal far yumemi_102C8(uint16_t slot)
{
	register uint8_t near *record = &byte_1F35E[slot * 32];

	*reinterpret_cast<subpixel_t near *>(record + 0x00) = TO_SP(144);
	*reinterpret_cast<subpixel_t near *>(record + 0x02) = TO_SP(80);
	*reinterpret_cast<uint16_t near *>(record + 0x08) = 0xFFE0;
	*reinterpret_cast<uint16_t near *>(record + 0x0A) = 0;
	record[0x12] = 0;
	record[0x10] = 0;
	record[0x11] = 0;
	*reinterpret_cast<uint16_t near *>(record + 0x0C) = 0x0082;
	record[0x13] = 0;
	record[0x16] = 0;
	record[0x17] = 0;
	*reinterpret_cast<uint16_t near *>(record + 0x18) = 0;
	*reinterpret_cast<uint16_t near *>(record + 0x0E) = 0x0288;
	if(slot != 0) {
		*reinterpret_cast<uint16_t near *>(record + 0x0E) += 0x28;
	}
	record[0x15] = 0;
}

extern "C" void pascal near yumemi_10324(void)
{
	if(word_1F3B0 == 1) {
		byte_1F353 = 1;
		byte_20E4E = randring_far_next16_raw();
		byte_20E4C = 0;
		word_1F356 = 256;
		if(randring_far_next16_and(1) == 0) {
			_AL = 3;
		} else {
			_AL = -3;
		}
		byte_20E4D = _AL;
	}

	if(word_1F3B0 < 4) {
		SUB_CE5B(word_20E50, word_20E52, bullet_template.pid);
		return;
	}

	if(word_1F3B0 < 0x20) {
		return;
	}
	if(word_1F3B0 == 0x20) {
		SUB_CDBD(word_20E50, word_20E52, bullet_template.pid);
		byte_1F353 = 2;
		snd_se_play(5);
	}
	if((static_cast<uint8_t>(word_1F3B0) & 3) == 0) {
		bullet_template.type = BT_BULLET16_DEFAULT;
		bullet_template.angle = byte_20E4E;
		bullet_template.group = BG_RING;
		bullet_template.center.x.v = word_20E50;
		bullet_template.center.y.v = word_20E52;
		bullet_template.speed.v = (byte_1F39F + byte_20E4C);
		bullet_template.count = byte_1F3A0;
		bullets_add();
		byte_20E4E += byte_20E4D;
		byte_20E4C += 8;
	}
	if(word_1F3B0 > 0x3C) {
		byte_1F353 = 0;
		byte_1F34F = 1;
		word_1F3B0 = 0;
	}
}
