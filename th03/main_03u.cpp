#pragma codeseg main_03_TEXT

#include "codegen.hpp"
#include "libs/master.lib/master.hpp"
#include "libs/sprite16/sprite16.h"
#include "platform.h"
#include "th01/math/subpixel.hpp"
#include "th03/main/bullet/bullet.hpp"
#include "th03/main/collmap.hpp"
#include "th03/main/hitbox.hpp"
#include "th03/main/player/cur.hpp"
#include "th03/main/player/gba.hpp"
#include "th03/main/playfld.hpp"
#include "th03/main/round.hpp"
#include "th03/main/sprite16.hpp"
#include "th03/math/polar.hpp"
#include "th03/math/randring.hpp"

extern "C" subpixel_t word_1F33E;
extern "C" subpixel_t word_1F340;
extern "C" uint16_t word_1F34A;
extern "C" subpixel_t word_1F356;
extern "C" uint16_t word_1F3B0;
extern "C" subpixel_t word_1DE36[];
extern "C" subpixel_t word_1DE38[];
extern "C" sprite16_offset_t sprite_1F34C;
extern "C" uint8_t pid_PID_so_attack;
extern "C" uint8_t byte_1F34E;
extern "C" uint8_t byte_1F34F;
extern "C" uint8_t byte_1F351;
extern "C" uint8_t byte_1F352;
extern "C" uint8_t byte_1F353;
extern "C" uint8_t byte_1F354;
extern "C" uint8_t byte_1F355;
extern "C" uint8_t byte_1F35E[];
extern "C" uint8_t byte_1F39F;
extern "C" uint8_t byte_1F3A0;
extern "C" uint8_t byte_1F3A1;
extern "C" uint8_t byte_1F3A2;
extern "C" uint8_t byte_1F3A3;
extern "C" uint8_t byte_1F3A4;
extern "C" uint8_t byte_1F3A5;
extern "C" uint8_t byte_23DE4;
extern "C" uint8_t byte_23DE5;

extern "C" uint16_t far randring_far_next16_raw(void);
extern "C" void pascal far sub_A3A8(uint8_t pid);
extern "C" void pascal far _sub_A3D2(uint16_t value, uint8_t pid);
extern "C" void pascal far SUB_CE0C(subpixel_t x, subpixel_t y, uint16_t pid);
extern "C" void pascal far SUB_CE5B(subpixel_t x, subpixel_t y, uint16_t pid);
extern "C" void near sub_F3A9(void);
extern "C" uint8_t near sub_F402(void);
extern "C" void far sub_F4B4(void);
extern "C" void pascal near sub_F512(void);
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

extern "C" void pascal near chiyuri_12355(void)
{
	if((word_1F3B0 % static_cast<uint16_t>(byte_1F3A2)) != 1) {
		return;
	}

	SUB_CE5B(word_1F33E, word_1F340, (1 - pid_current));
	if(word_1F3B0 >= 0x80) {
		byte_1F34F = 1;
		word_1F3B0 = 0;
		byte_1F353 = 0x20;
	}

	bullet_template.center.x.v = word_1F33E;
	bullet_template.center.y.v = word_1F340;
	bullet_template.angle = randring_far_next16_raw();
	bullet_template.type = BT_BULLET16_DEFAULT;
	bullet_template.group = BG_32_RING;
	bullet_template.speed.v = byte_1F3A1;
	bullets_add();
	if(gba_boss_level < 8) {
		return;
	}

	bullet_template.type = BT_PELLET;
	bullet_template.group = BG_8_RING;
	bullet_template.speed.v = (static_cast<int16_t>(byte_1F3A1) / 4);
	bullet_template.center.x.v = (word_1F33E + TO_SP(56));
	bullets_add();
	bullet_template.center.x.v = (word_1F33E + TO_SP(-56));
	bullets_add();
	bullet_template.center.x.v = word_1F33E;
	bullet_template.center.y.v = (word_1F340 + TO_SP(56));
	bullets_add();
	bullet_template.center.y.v = (word_1F340 + TO_SP(-56));
	bullets_add();
}

extern "C" void pascal near chiyuri_12425(void)
{
	if((word_1F3B0 & 7) == 0) {
		SUB_CE5B(word_1F33E, word_1F340, (1 - pid_current));
	}

	if(word_1F3B0 == 0x30) {
		bullet_template.center.x.v = word_1F33E;
		bullet_template.center.y.v = word_1F340;
		bullet_template.angle = randring_far_next16_raw();
		bullet_template.type = BT_BULLET16_DEFAULT;
		bullet_template.group = BG_RING;
		bullet_template.count = 48;
		bullet_template.speed.v = byte_1F3A3;
		bullet_template.has_trail = true;
		bullets_add();
		bullet_template.has_trail = false;
		byte_1F34F = 1;
		word_1F3B0 = 0;
		byte_1F353 = 0x20;
	}
}

extern "C" void pascal near chiyuri_12498(void)
{
	uint8_t pid_other;
	register int i;

	pid_other = (1 - pid_current);
	if(word_1F3B0 == 0x20) {
		SUB_CE5B(
			(word_1F33E + TO_SP(-56)), word_1F340, static_cast<uint16_t>(pid_other)
		);
		SUB_CE5B(
			(word_1F33E + TO_SP(56)), word_1F340, static_cast<uint16_t>(pid_other)
		);
		bullet_template.center.y.v = word_1F340;
		bullet_template.group = BG_1;
		bullet_template.type = BT_PELLET;
		bullet_template.speed.v = byte_1F3A4;

		i = 0;
		while(static_cast<int>(byte_1F3A5) > i) {
			bullet_template.center.x.v = (
				(((i * 0x70) / static_cast<int>(byte_1F3A5)) << SUBPIXEL_BITS) +
				word_1F33E +
				TO_SP(-56)
			);
			bullet_template.angle = (
				0x60 - ((i << 6) / static_cast<int>(byte_1F3A5))
			);
			bullets_add();
			bullet_template.angle = (0 - bullet_template.angle);
			bullets_add();
			bullet_template.speed.v += 2;
			i++;
		}

		i = 0;
		while(static_cast<int>(byte_1F3A5) > i) {
			bullet_template.center.x.v = (
				(((i * 0x70) / static_cast<int>(byte_1F3A5)) << SUBPIXEL_BITS) +
				word_1F33E +
				TO_SP(-56)
			);
			bullet_template.angle = (
				0x60 - ((i << 6) / static_cast<int>(byte_1F3A5))
			);
			bullets_add();
			bullet_template.angle = (0 - bullet_template.angle);
			bullets_add();
			bullet_template.speed.v -= 2;
			i++;
		}
		return;
	}

	if(word_1F3B0 == 0x40) {
		SUB_CE5B(
			word_1F33E, (word_1F340 + TO_SP(-56)), static_cast<uint16_t>(pid_other)
		);
		SUB_CE5B(
			word_1F33E, (word_1F340 + TO_SP(56)), static_cast<uint16_t>(pid_other)
		);
		bullet_template.center.x.v = word_1F33E;
		bullet_template.type = BT_PELLET;
		bullet_template.group = BG_2_SPREAD_HORIZONTALLY_SYMMETRIC;
		bullet_template.speed.v = byte_1F3A4;

		i = 0;
		while(static_cast<int>(byte_1F3A5) > i) {
			bullet_template.center.y.v = (
				(((i * 0x70) / static_cast<int>(byte_1F3A5)) << SUBPIXEL_BITS) +
				word_1F340 +
				TO_SP(-56)
			);
			bullet_template.angle = (
				((i << 6) / static_cast<int>(byte_1F3A5)) - 0x20
			);
			bullets_add();
			bullet_template.speed.v += 2;
			i++;
		}

		i = 0;
		while(static_cast<int>(byte_1F3A5) > i) {
			bullet_template.center.y.v = (
				(((i * 0x70) / static_cast<int>(byte_1F3A5)) << SUBPIXEL_BITS) +
				word_1F340 +
				TO_SP(-56)
			);
			bullet_template.angle = (
				((i << 6) / static_cast<int>(byte_1F3A5)) - 0x20
			);
			bullets_add();
			bullet_template.speed.v -= 2;
			i++;
		}

		byte_1F34F = 1;
		word_1F3B0 = 0;
		byte_1F353 = 0x20;
	}
}

#pragma warn -aus
#pragma option -G-
extern "C" void pascal far gba_boss_update_chiyuri(void)
{
	pid_t pid_other;
	uint8_t random_point;
	uint16_t state;

	if(sub_F402()) {
		byte_1F39F = (gba_boss_level + 0x30);
		byte_1F3A0 = (6 - (gba_boss_level / 4));
		byte_1F3A1 = (gba_boss_level + 0x34);
		byte_1F3A2 = (0x20 - gba_boss_level);
		byte_1F3A3 = ((gba_boss_level * 2) + 0x38);
		byte_1F3A4 = (gba_boss_level + 0x10);
		byte_1F3A5 = ((gba_boss_level / 2) + 0x0C);
	}

	if(pid_current != gba_boss_launched_by) {
		return;
	}

	pid_other = (1 - pid_current);
	sub_F512();
	bullet_template.is_animated = false;
	bullet_template.pid = pid_other;
	word_1F3B0++;
	state = byte_1F34F;

	// TCC places the generated switch table before any post-function
	// codestring, so keep this dispatch table as a raw byte island.
	__emit__(0xB9, 0x14, 0x00, 0xBB, 0x1E, 0x37, 0x2E, 0x8B, 0x07, 0x3B, 0x46, 0xFC);
	__emit__(0x74, 0x08, 0x83, 0xC3, 0x02, 0xE2, 0xF3, 0xE9, 0x1C, 0x01, 0x2E, 0xFF);
	__emit__(0x67, 0x28, 0x83, 0x3E, 0x50, 0x1E, 0x60, 0x75, 0x23, 0xC7, 0x06, 0x50);
	__emit__(0x1E, 0x00, 0x00, 0xC6, 0x06, 0xEF, 0x1D, 0x01, 0x68, 0xFF, 0x00, 0xFF);
	__emit__(0x76, 0xFF);
	_asm { call	far ptr _sub_A3D2 }
	__emit__(0xC6, 0x06, 0xF5, 0x1D, 0x10, 0xC6, 0x06, 0xF3, 0x1D, 0x10, 0xE9, 0xEE);
	__emit__(0x00, 0x83, 0x3E, 0x50, 0x1E, 0x50, 0x0F, 0x85, 0xDE, 0x00, 0xA1, 0xDE);
	__emit__(0x1D, 0x05, 0x80, 0xFC, 0x50, 0xFF, 0x36, 0xE0, 0x1D, 0x8A, 0x46, 0xFF);
	__emit__(0xB4, 0x00, 0x50);
	_asm { call	far ptr SUB_CE0C }
	__emit__(0xA1, 0xDE, 0x1D, 0x05, 0x80, 0x03, 0x50, 0xFF, 0x36, 0xE0, 0x1D, 0x8A);
	__emit__(0x46, 0xFF, 0xB4, 0x00, 0x50);
	_asm { call	far ptr SUB_CE0C }
	__emit__(0xFF, 0x36, 0xDE, 0x1D, 0xA1, 0xE0, 0x1D, 0x05, 0x80, 0xFC, 0x50, 0x8A);
	__emit__(0x46, 0xFF, 0xB4, 0x00, 0x50);
	_asm { call	far ptr SUB_CE0C }
	__emit__(0xFF, 0x36, 0xDE, 0x1D, 0xA1, 0xE0, 0x1D, 0x05, 0x80, 0x03, 0x50, 0x8A);
	__emit__(0x46, 0xFF, 0xB4, 0x00, 0x50);
	_asm { call	far ptr SUB_CE0C }
	__emit__(0xE9, 0x8A, 0x00, 0x80, 0x3E, 0xF3, 0x1D, 0x10, 0x75, 0x2E, 0x6A, 0x05);
	_asm { call	far ptr randring_far_next16_mod }
	__emit__(0x02, 0xC0, 0x88, 0x46, 0xFE, 0xB4, 0x00, 0x03, 0xC0, 0x8B, 0xD8, 0x8B);
	__emit__(0x87, 0xD6, 0x08, 0xA3, 0xDE, 0x1D, 0x8A, 0x46, 0xFE, 0xB4);
	__emit__(0x00, 0x03, 0xC0, 0x8B, 0xD8, 0x8B, 0x87, 0xD8, 0x08, 0xA3, 0xE0, 0x1D);
	__emit__(0xC6, 0x06, 0xF5, 0x1D, 0x10, 0x83, 0x3E, 0x50, 0x1E, 0x30, 0x72, 0x4E);
	__emit__(0xC7, 0x06, 0x50, 0x1E, 0x00, 0x00, 0x6A, 0x0F);
	_asm { call	far ptr randring_far_next16_and }
	__emit__(0x04, 0x02, 0xA2, 0xEF, 0x1D, 0xFE, 0x06, 0xF1, 0x1D, 0xA0, 0xF1, 0x1D);
	__emit__(0x3A, 0x06, 0xF2, 0x1D, 0x76, 0x2F, 0xC6, 0x06, 0xEF, 0x1D, 0x80, 0xEB);
	__emit__(0x28, 0xE8, 0xBA, 0xF9, 0xEB, 0x23, 0xE8, 0x15, 0xFB, 0xEB, 0x1E);
	__emit__(0xE8, 0xE0, 0xFB, 0xEB, 0x19, 0xE8, 0x4E, 0xFC, 0xEB, 0x14, 0xE8, 0x5A);
	__emit__(0xCB, 0xFF, 0x76, 0xFF);
	_asm { call	far ptr sub_A3A8 }
	__emit__(0xEB, 0x07, 0xC6, 0x06, 0xF0, 0x68, 0x01, 0xC9, 0xCB);

	bullet_template.is_animated = true;
	byte_1F354 = (((round_or_result_frame & 3) == 0) + byte_1F354);
	byte_1F354 &= 3;
	if(byte_1F355 != 0) {
		byte_1F355--;
		if(byte_1F355 != 0) {
			_sub_A3D2((static_cast<uint16_t>(byte_1F355) << 4), pid_other);
		}
	}
	if(byte_1F353 != 0) {
		byte_1F353--;
	}

	collmap_center.x.v = word_1F33E;
	collmap_center.y.v = word_1F340;
	collmap_stripe_tile_w.v = (64 / COLLMAP_TILE_W);
	collmap_tile_h.v = (48 / COLLMAP_TILE_H);
	collmap_pid = pid_other;
	collmap_set_rect_striped();

	hitbox_hittest_skip_explosions = true;
	hitbox.radius.x.v = TO_SP(32);
	hitbox.radius.y.v = TO_SP(32);
	hitbox.pid = pid_other;
	hitbox.origin.center.x.v = word_1F33E;
	hitbox.origin.center.y.v = word_1F340;
	_AL = hitbox_hittest();
	byte_1F34E = _AL;
	_AH = 0;
	word_1F34A -= _AX;
	hitbox_hittest_skip_explosions = false;

	_asm {
		nop
		push	cs
		call	near ptr sub_F4B4
	}
}
#pragma codestring "\x00\x00\x00\x01\x00\x02\x00\x03\x00\x04\x00\x05\x00\x06\x00\x07\x00\x08\x00\x09\x00\x0A\x00\x0B\x00\x0C\x00\x0D\x00\x0E\x00\x0F\x00\x10\x00\x11\x00\x80\x00\xFF\x00\x58\x35\xE6\x35\x48\x36\x48\x36\x48\x36\x48\x36\x4D\x36\x4D\x36\x4D\x36\x4D\x36\x52\x36\x52\x36\x52\x36\x52\x36\x57\x36\x57\x36\x57\x36\x57\x36\x5C\x36\x69\x36"
#pragma option -G
#pragma warn .aus
