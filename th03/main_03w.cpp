#pragma codeseg main_03_TEXT

#include "codegen.hpp"
#include "libs/master.lib/master.hpp"
#include "libs/sprite16/sprite16.h"
#include "platform.h"
#include "th01/math/subpixel.hpp"
#include "th02/snd/snd.h"
#include "th03/main/bullet/bullet.hpp"
#include "th03/main/player/gba.hpp"
#include "th03/main/player/cur.hpp"
#include "th03/main/playfld.hpp"
#include "th03/main/round.hpp"
#include "th03/main/sprite16.hpp"
#include "th03/math/polar.hpp"
#include "th03/math/randring.hpp"

extern "C" subpixel_t word_1F33E;
extern "C" subpixel_t word_1F340;
extern "C" uint16_t word_1F3B0;
extern "C" sprite16_offset_t sprite_1F34C;
extern "C" uint8_t pid_PID_so_attack;
extern "C" uint8_t byte_1F34F;
extern "C" uint8_t byte_1F35E[];
extern "C" uint8_t byte_1F34E;
extern "C" uint8_t byte_1F353;
extern "C" uint8_t byte_1F354;
extern "C" uint8_t byte_1F355;
extern "C" uint8_t byte_1F358;
extern "C" uint8_t byte_1F39F;
extern "C" uint8_t byte_1F3A0;
extern "C" uint8_t byte_1F3A1;
extern "C" uint8_t byte_1F3A2;
extern "C" uint8_t byte_1F3A3;
extern "C" uint8_t byte_23DE8;
extern "C" uint8_t bullet_type_23DE9;

extern "C" void pascal far SUB_CDBD(subpixel_t x, subpixel_t y, uint16_t pid);
extern "C" void pascal far SUB_CE0C(subpixel_t x, subpixel_t y, uint16_t pid);
extern "C" void pascal far SUB_CE5B(subpixel_t x, subpixel_t y, uint16_t pid);
extern "C" void pascal far RIKAKO_1B006(
	subpixel_t x, subpixel_t y, uint8_t angle
);
extern "C" uint16_t far randring_far_next16_raw(void);
extern "C" void near sub_F356(void);
extern "C" void pascal near sub_F58C(void);

extern "C" void pascal near kana_13174(void)
{
	screen_y_t top;
	register sprite16_offset_t sprite_offset;
	register screen_x_t left;

	sprite16_put_size.w.v = (128 / 16);
	sprite16_put_size.h = 40;
	left = (playfield_fg_x_to_screen(word_1F33E, (1 - pid_current)) - 64);
	top = ((word_1F340 >> 4) - 24);
	sprite_offset = sprite_1F34C;
	if(byte_1F34E != 0) {
		sprite_offset += 0x10;
	}
	sprite16_put(left, top, sprite_offset);

	sprite16_put_size.w.v = (64 / 16);
	sprite16_put_size.h = 32;
	left += 32;
	if(byte_1F353 == 1) {
		sprite16_put(left, top, (sprite_1F34C + (40 * ROW_SIZE)));
		return;
	}
	if(byte_1F353 == 2) {
		sprite_offset = (sprite_1F34C + (40 * ROW_SIZE));
		if(byte_1F354 >= 8) {
			if(byte_1F354 < 0x10) {
				sprite_offset += 8;
			} else if(byte_1F354 < 0x18) {
				sprite_offset += 0x10;
			} else {
				sprite_offset += 0x18;
			}
		}
		sprite16_put(left, top, sprite_offset);
	}
}

extern "C" void pascal near kana_13223(void)
{
	screen_y_t top;
	sprite16_offset_t sprite_offset;
	subpixel_t radius;
	uint8_t angle;
	register int i;
	register screen_x_t left;

	radius = ((200 << SUBPIXEL_BITS) - ((word_1F3B0 << 1) << 4));
	sprite16_put_size.w.v = (32 / 16);
	sprite16_put_size.h = 16;
	if((word_1F3B0 < 0x20) && (round_frame_mod2 != 0)) {
		return;
	}

	sprite_offset = (
		pid_PID_so_attack + ((8 * ROW_SIZE) + (32 / BYTE_DOTS))
	);
	sprite_offset = (sprite_offset + ((((word_1F3B0 >> 2) & 3) << 5) * 40));
	angle = byte_1F355;
	angle = (0 - angle);
	i = 0;
	while(i < 0x10) {
		left = polar(word_1F33E, radius, CosTable8[angle]);
		top = polar(word_1F340, radius, SinTable8[angle]);
		left = (playfield_fg_x_to_screen(left, (1 - pid_current)) - 16);
		top = (top >> 4);
		sprite16_put(left, top, sprite_offset);
		i++;
		angle += 0x10;
	}
}

extern "C" void pascal far gba_boss_render_kana(void)
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
		kana_13223();
		return;
	}
	if(byte_1F34F != 0xFF) {
		kana_13174();
		return;
	}
	sub_F58C();
}

extern "C" void pascal far rikako_1334D(uint16_t slot)
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
	record[0x1A] = 4;
	*reinterpret_cast<uint16_t near *>(record + 0x18) = 0;
	*reinterpret_cast<uint16_t near *>(record + 0x0E) = 0x0286;
	if(slot != 0) {
		*reinterpret_cast<uint16_t near *>(record + 0x0E) += 0x28;
	}
	record[0x15] = 0;
}

extern "C" void pascal near rikako_133A5(void)
{
	uint8_t angle;
	uint8_t angle_delta;
	register int i;

	_asm {
		cmp	byte ptr byte_1F358, 8
		jg	short rikako_133A5_ramp_done
		test	byte ptr word_1F3B0, 7
		jnz	short rikako_133A5_ramp_done
		inc	byte ptr byte_1F358
rikako_133A5_ramp_done:
	}

	if(word_1F3B0 == 0x10) {
		SUB_CE0C(word_1F33E, word_1F340, (1 - pid_current));
		return;
	}

	if(word_1F3B0 == 0x28) {
		SUB_CDBD(word_1F33E, word_1F340, (1 - pid_current));
		bullet_template.type = BT_BULLET16_DEFAULT;
		bullet_template.group = BG_5_SPREAD_WIDE;
		bullet_template.speed.v = ((1 << 4) + 12);
		if(randring_far_next16_and(1) != 0) {
			_AL = 0x30;
		} else {
			_AL = -0x30;
		}
		angle_delta = _AL;

		i = 0;
		while(static_cast<int>(byte_1F39F) > i) {
			angle = ((i << 8) / static_cast<int>(byte_1F39F));
			bullet_template.center.x.v = polar(
				word_1F33E, TO_SP(48), CosTable8[angle]
			);
			bullet_template.center.y.v = polar(
				word_1F340, TO_SP(48), SinTable8[angle]
			);
			bullet_template.angle = (angle + angle_delta);
			bullets_add();
			i++;
		}
		snd_se_play(5);
		return;
	}

	if(word_1F3B0 > 0x50) {
		word_1F3B0 = 0;
		byte_1F34F = 1;
	}
}

extern "C" void pascal near rikako_134AA(void)
{
	uint8_t angle;
	register int i;

	_asm {
		cmp	byte ptr byte_1F358, -8
		jl	short rikako_134AA_ramp_done
		test	byte ptr word_1F3B0, 7
		jnz	short rikako_134AA_ramp_done
		dec	byte ptr byte_1F358
rikako_134AA_ramp_done:
	}

	if(word_1F3B0 == 0x10) {
		SUB_CE5B(word_1F33E, word_1F340, (1 - pid_current));
		return;
	}

	if(word_1F3B0 == 0x28) {
		SUB_CDBD(word_1F33E, word_1F340, (1 - pid_current));
		angle = randring_far_next16_raw();

		i = 0;
		while(static_cast<int>(byte_1F3A0) > i) {
			angle = ((i << 8) / static_cast<int>(byte_1F3A0));
			RIKAKO_1B006(word_1F33E, word_1F340, angle);
			i++;
		}
		snd_se_play(5);
		return;
	}

	if(word_1F3B0 == 0x40) {
		bullet_template.type = BT_PELLET;
		bullet_template.group = BG_RING;
		bullet_template.count = byte_1F3A1;
		bullet_template.center.x.v = word_1F33E;
		bullet_template.center.y.v = word_1F340;
		bullet_template.speed.v = (2 << 4);

		i = 0;
		while(i < 4) {
			bullet_template.angle = randring_far_next16_raw();
			bullet_template.speed.v += 8;
			bullets_add();
			i++;
		}
		snd_se_play(10);
		return;
	}

	if(word_1F3B0 > 0x50) {
		word_1F3B0 = 0;
		byte_1F34F = 1;
	}
}

extern "C" void pascal near rikako_135A4(void)
{
	uint8_t phase_mod;

	sub_F356();
	_asm {
		cmp	byte ptr byte_1F358, 2
		jz	short rikako_135A4_ramp_done
		test	byte ptr word_1F3B0, 7
		jnz	short rikako_135A4_ramp_done
		cmp	byte ptr byte_1F358, 2
		jle	short rikako_135A4_ramp_inc
		mov	al, -1
		jmp	short rikako_135A4_ramp_apply
rikako_135A4_ramp_inc:
		mov	al, 1
rikako_135A4_ramp_apply:
		add	al, byte ptr byte_1F358
		mov	byte ptr byte_1F358, al
rikako_135A4_ramp_done:
	}

	phase_mod = (word_1F3B0 % static_cast<uint16_t>(byte_1F3A2));
	if(phase_mod == 1) {
		byte_23DE8 = randring_far_next16_raw();
		randring_far_next16_and(1);
		_AL++;
		bullet_type_23DE9 = _AL;
	}

	bullet_template.type = static_cast<bullet_type_t>(bullet_type_23DE9);
	bullet_template.group = BG_4_SPREAD_NARROW;
	bullet_template.center.x.v = word_1F33E;
	bullet_template.center.y.v = word_1F340;
	bullet_template.angle = byte_23DE8;
	bullet_template.speed.v = byte_1F3A3;

	if(
		(phase_mod == 1) || (phase_mod == 5) ||
		(phase_mod == 9) || (phase_mod == 0x0D)
	) {
		snd_se_play(10);
		bullets_add();
		bullet_template.angle = (byte_23DE8 + 0x80);
		bullets_add();
	}

	if(word_1F3B0 >= 0x80) {
		byte_1F34F = 1;
		word_1F3B0 = 0;
	}
}
