#pragma codeseg main_03_TEXT

#include "codegen.hpp"
#include "libs/master.lib/master.hpp"
#include "libs/sprite16/sprite16.h"
#include "platform.h"
#include "th01/math/subpixel.hpp"
#include "th03/main/bullet/bullet.hpp"
#include "th03/main/player/cur.hpp"
#include "th03/main/player/gba.hpp"
#include "th03/main/playfld.hpp"
#include "th03/main/round.hpp"
#include "th03/main/sprite16.hpp"
#include "th03/math/polar.hpp"

extern "C" subpixel_t word_1F33E;
extern "C" subpixel_t word_1F340;
extern "C" subpixel_t word_1F356;
extern "C" uint16_t word_1F3B0;
extern "C" sprite16_offset_t sprite_1F34C;
extern "C" uint8_t pid_PID_so_attack;
extern "C" uint8_t byte_1F34E;
extern "C" uint8_t byte_1F34F;
extern "C" uint8_t byte_1F353;
extern "C" uint8_t byte_1F355;
extern "C" uint8_t byte_1F35E[];
extern "C" uint8_t byte_1F39F;
extern "C" uint8_t byte_1F3A0;
extern "C" uint8_t byte_23DE4;
extern "C" uint8_t byte_23DE5;

extern "C" void pascal far SUB_CE5B(subpixel_t x, subpixel_t y, uint16_t pid);
extern "C" void pascal near sub_F58C(void);

extern "C" void pascal near kotohime_11FE4(int count)
{
	screen_y_t top;
	sprite16_offset_t sprite_offset;
	subpixel_t radius;
	uint8_t angle;
	register int i;
	register screen_x_t left;

	radius = word_1F356;
	sprite_offset = (pid_PID_so_attack + ((16 * ROW_SIZE) + (16 / BYTE_DOTS)));
	if(byte_1F353 == 2) {
		if((word_1F3B0 & 3) < 2) {
			sprite_offset += (32 * ROW_SIZE);
		}
	}

	sprite16_put_size.w.v = (32 / 16);
	sprite16_put_size.h = 16;
	for(i = 0; i < count; i++) {
		angle = (((i << 8) / count) + byte_1F355);
		left = polar(word_1F33E, radius, CosTable8[angle]);
		top = polar(word_1F340, radius, SinTable8[angle]);
		left = (playfield_fg_x_to_screen(left, (1 - pid_current)) - 16);
		top = (top >> 4);
		sprite16_put(left, top, sprite_offset);
	}
}

extern "C" void pascal near kotohime_120A0(void)
{
	screen_x_t left;
	screen_y_t top;
	register sprite16_offset_t sprite_offset;

	sprite16_put_size.w.v = (128 / 16);
	sprite16_put_size.h = 48;
	left = (playfield_fg_x_to_screen(word_1F33E, (1 - pid_current)) - 64);
	top = ((word_1F340 >> 4) - 32);
	sprite_offset = sprite_1F34C;
	if(byte_1F34E != 0) {
		sprite_offset += 0x10;
	}
	sprite16_put(left, top, sprite_offset);
	if(byte_1F353 != 0) {
		kotohime_11FE4(byte_1F3A0);
	}
}

extern "C" void pascal near kotohime_12103(subpixel_t radius)
{
	if((round_frame_mod2 == 0) || (word_1F3B0 >= 0x40)) {
		sprite16_put_size.w.v = (32 / 16);
		sprite16_put_size.h = 16;
		byte_1F353 = 1;
		_AL = byte_1F355;
		_AL += 2;
		byte_1F355 = _AL;
		word_1F356 = radius;
		kotohime_11FE4(8);
		byte_1F353 = 0;
	}
}

extern "C" void pascal far gba_boss_render_kotohime(void)
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
		kotohime_12103((200 - (word_1F3B0 * 2)) << SUBPIXEL_BITS);
		return;
	}
	if(byte_1F34F != 0xFF) {
		kotohime_120A0();
		return;
	}
	sub_F58C();
}

extern "C" void pascal far chiyuri_1219D(uint16_t slot)
{
	register uint8_t near *record = &byte_1F35E[slot * 32];

	*reinterpret_cast<subpixel_t near *>(record + 0x00) = TO_SP(144);
	*reinterpret_cast<subpixel_t near *>(record + 0x02) = TO_SP(80);
	*reinterpret_cast<uint16_t near *>(record + 0x08) = 0xFFE0;
	*reinterpret_cast<uint16_t near *>(record + 0x0A) = 0;
	record[0x12] = 0;
	record[0x10] = 0;
	record[0x11] = 0;
	*reinterpret_cast<uint16_t near *>(record + 0x0C) = 0x008C;
	record[0x13] = 0;
	record[0x17] = 0;
	*reinterpret_cast<uint16_t near *>(record + 0x18) = 0;
	*reinterpret_cast<uint16_t near *>(record + 0x0E) = 0x0288;
	if(slot != 0) {
		*reinterpret_cast<uint16_t near *>(record + 0x0E) += 0x28;
	}
	record[0x15] = 0;
}

extern "C" void pascal near chiyuri_121F5(void)
{
	uint8_t pid_other;

	if((word_1F3B0 % static_cast<uint16_t>(byte_1F3A0)) != 1) {
		return;
	}
	if(word_1F3B0 == 1) {
		byte_23DE4 = 224;
		byte_23DE5 = 0;
		pid_other = (1 - pid_current);
		SUB_CE5B(
			(word_1F33E + TO_SP(-56)), word_1F340, static_cast<uint16_t>(pid_other)
		);
		SUB_CE5B(
			(word_1F33E + TO_SP(56)), word_1F340, static_cast<uint16_t>(pid_other)
		);
		SUB_CE5B(
			word_1F33E, (word_1F340 + TO_SP(-56)), static_cast<uint16_t>(pid_other)
		);
		SUB_CE5B(
			word_1F33E, (word_1F340 + TO_SP(56)), static_cast<uint16_t>(pid_other)
		);
	} else if(word_1F3B0 >= 0x80) {
		byte_1F34F = 1;
		word_1F3B0 = 0;
		byte_1F353 = 0x20;
	}

	bullet_template.type = BT_PELLET;
	bullet_template.group = BG_1;
	bullet_template.speed.v = byte_1F39F;
	if(byte_23DE5 == 0) {
		byte_23DE4 += 8;
		if(byte_23DE4 == 0x20) {
			byte_23DE5 = 1;
			byte_23DE4 = 0x24;
		}
	} else {
		byte_23DE4 -= 8;
		if(byte_23DE4 == 228) {
			byte_23DE5 = 0;
			byte_23DE4 = 224;
		}
	}

	bullet_template.center.x.v = (word_1F33E + TO_SP(56));
	bullet_template.center.y.v = word_1F340;
	bullet_template.angle = byte_23DE4;
	bullets_add();
	bullet_template.center.x.v = word_1F33E;
	bullet_template.center.y.v = (word_1F340 + TO_SP(56));
	bullet_template.angle += 0x40;
	bullets_add();
	bullet_template.center.x.v = (word_1F33E + TO_SP(-56));
	bullet_template.center.y.v = word_1F340;
	bullet_template.angle += 0x40;
	bullets_add();
	bullet_template.center.x.v = word_1F33E;
	bullet_template.center.y.v = (word_1F340 + TO_SP(-56));
	bullet_template.angle += 0x40;
	bullets_add();
}
