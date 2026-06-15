#pragma codeseg main_03_TEXT

#include "codegen.hpp"
#include "libs/sprite16/sprite16.h"
#include "platform.h"
#include "th01/math/subpixel.hpp"
#include "th02/snd/snd.h"
#include "th03/main/bullet/bullet.hpp"
#include "th03/main/player/cur.hpp"
#include "th03/main/player/gba.hpp"
#include "th03/main/playfld.hpp"
#include "th03/main/round.hpp"
#include "th03/main/sprite16.hpp"

extern "C" subpixel_t word_1F33E;
extern "C" subpixel_t word_1F340;
extern "C" uint16_t word_1F3B0;
extern "C" uint8_t byte_1F35E[];
extern "C" sprite16_offset_t sprite_1F34C;
extern "C" uint8_t byte_1F34E;
extern "C" uint8_t byte_1F34F;
extern "C" uint8_t byte_1F353;
extern "C" uint8_t byte_1F354;
extern "C" uint8_t byte_1F39F;
extern "C" uint8_t byte_1F3A0;
extern "C" uint8_t byte_1F3A1;
extern "C" uint8_t byte_1F3A2;
extern "C" uint8_t byte_1F3A3;
extern "C" uint8_t byte_1F3A4;
extern "C" uint8_t byte_20E28;
extern "C" uint8_t byte_20E29;

extern "C" uint16_t far randring_far_next16_raw(void);
extern "C" void pascal far SUB_CE0C(subpixel_t x, subpixel_t y, uint16_t pid);
extern "C" void near sub_F356(void);
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

extern "C" void pascal far mima_FB46(uint16_t slot)
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
	*reinterpret_cast<uint16_t near *>(record + 0x0E) = 0x028C;
	if(slot != 0) {
		*reinterpret_cast<uint16_t near *>(record + 0x0E) += 0x28;
	}
	record[0x15] = 0;
}

extern "C" void pascal near mima_FB95(void)
{
	if(word_1F3B0 < 2) {
		byte_1F353 = 2;
		byte_1F354 = 0;
		byte_20E28 = 0;
		byte_20E29 = 192;
		return;
	}

	if(word_1F3B0 >= 0x20) {
		if((word_1F3B0 & 1) == 0) {
			snd_se_play(3);
			bullet_template.angle = byte_20E28;
			bullet_template.group = BG_2_SPREAD_HORIZONTALLY_SYMMETRIC;
			bullet_template.center.x.v = word_1F33E;
			bullet_template.center.y.v = word_1F340;
			bullet_template.speed.v = byte_1F39F;
			bullet_template.type = BT_BULLET16_DEFAULT;
			bullet_template.is_animated = false;
			bullets_add();
			bullet_template.is_animated = true;
			byte_20E28 += 9;

			if(static_cast<uint16_t>(byte_1F3A4) > word_1F3B0) {
				if((word_1F3B0 % 6) == 0) {
					bullet_template.type = BT_PELLET;
					bullet_template.group = BG_4_SPREAD_MEDIUM;
					bullet_template.speed.v = byte_1F3A3;
					bullet_template.angle = byte_20E29;
					bullets_add();
					bullet_template.angle = (0x80 - byte_20E29);
					bullets_add();
				}
			}

			byte_20E29 += 3;
		}
		if(word_1F3B0 > 0x82) {
			byte_1F353 = 0;
			byte_1F34F = 1;
			word_1F3B0 = 0;
		}
	}
}

extern "C" void pascal near mima_FC6B(void)
{
	pid_t pid_other = (1 - pid_current);
	register subpixel_t x;
	register subpixel_t y;

	if(word_1F3B0 == 0x10) {
		byte_1F353 = 5;
	} else if(word_1F3B0 < 0x20) {
		return;
	}

	x = (word_1F33E + TO_SP(24));
	y = (word_1F340 + TO_SP(-32));

	if((word_1F3B0 == 0x20) || (word_1F3B0 == 0x24) || (word_1F3B0 == 0x28)) {
		SUB_CE0C(x, y, static_cast<uint16_t>(pid_other));
		return;
	}

	if(word_1F3B0 > 0x30) {
		if((word_1F3B0 & 3) == 0) {
			bullet_template.angle = 0;
			bullet_template.group = BG_RANDOM_ANGLE_AND_SPEED;
			bullet_template.center.x.v = x;
			bullet_template.center.y.v = y;
			bullet_template.count = byte_1F3A0;
			bullet_template.speed.v = byte_1F3A1;
			bullet_template.type = BT_BULLET16_DEFAULT;
			bullet_template.is_animated = false;
			bullets_add();
			bullet_template.type = BT_PELLET;
			bullets_add();
			bullet_template.is_animated = true;
		}

		if(word_1F3B0 == 0x40) {
			bullet_template.angle = randring_far_next16_raw();
			bullet_template.group = BG_32_RING;
			bullet_template.center.x.v = x;
			bullet_template.center.y.v = y;
			bullet_template.speed.v = (2 << 4);
			bullet_template.type = BT_BULLET16_DEFAULT;
			bullet_template.is_animated = false;
			bullets_add();
			bullet_template.is_animated = true;
		}

		if((word_1F3B0 & 1) == 0) {
			snd_se_play(3);
		}

		if(word_1F3B0 >= 0x80) {
			byte_1F353 = 0;
			byte_1F34F = 1;
			word_1F3B0 = 0;
		}
	}
}

extern "C" void pascal near mima_FD71(void)
{
	uint8_t phase_mod = (word_1F3B0 % static_cast<uint16_t>(byte_1F3A2));

	sub_F356();
	byte_1F353 = 5;
	if(
		(phase_mod == 1) ||
		(static_cast<uint16_t>(phase_mod) == (static_cast<int16_t>(byte_1F3A2) / 2))
	) {
		if(static_cast<uint16_t>(phase_mod) == (static_cast<int16_t>(byte_1F3A2) / 2)) {
			bullet_template.group = BG_4_SPREAD_MEDIUM_AIMED;
		} else {
			bullet_template.group = BG_5_SPREAD_MEDIUM_AIMED;
		}
		snd_se_play(10);
		bullet_template.angle = 0;
		bullet_template.center.x.v = (word_1F33E + TO_SP(24));
		bullet_template.center.y.v = (word_1F340 + TO_SP(-32));
		bullet_template.speed.v = ((5 << 4) + 8);
		bullet_template.type = BT_BULLET16_DEFAULT;
		bullet_template.is_animated = false;
		bullet_template.has_trail = true;
		bullets_add();
		bullet_template.has_trail = false;
		bullet_template.is_animated = true;
	}

	if(word_1F3B0 >= 150) {
		byte_1F353 = 0;
		byte_1F34F = 1;
		word_1F3B0 = 0;
	}
}
