#pragma codeseg main_03_TEXT

#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
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
#include "th03/math/randring.hpp"

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
extern "C" uint8_t byte_1F39F;
extern "C" uint8_t byte_1F3A1;
extern "C" uint8_t byte_1F3A2;
extern "C" uint8_t byte_1F3A3;
extern "C" uint8_t byte_1F3A4;
extern "C" uint8_t byte_23DC6;
extern "C" uint8_t byte_23DC7;
extern "C" uint8_t byte_23DC8;
extern "C" subpixel_t word_23DCA[];
extern "C" subpixel_t word_23DD6[];

extern "C" uint16_t far randring_far_next16_raw(void);
extern "C" void pascal far reimu_1A2CE(subpixel_t x, subpixel_t y, uint8_t angle);
extern "C" void near sub_F356(void);
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

extern "C" void pascal near reimu_10C4D(void)
{
	uint8_t angle;

	if(word_1F3B0 == 0) {
		point_1F342.x.v = word_1F33E;
		point_1F342.y.v = (word_1F340 + TO_SP(48));
		byte_23DC6 = randring_far_next16_and(1);
		byte_23DC7 = 0x10;
	}

	_AL = static_cast<uint8_t>(word_1F3B0);
	_AL += _AL;
	angle = _AL;
	if(byte_23DC6 != 0) {
		angle = (0 - angle);
	}

	if((word_1F3B0 % static_cast<uint16_t>(byte_1F39F)) == 0) {
		bullet_template.type = BT_PELLET_CLOUD;
		bullet_template.group = BG_1;
		bullet_template.pid = (1 - pid_current);
		_AL = angle;
		_AL += _AL;
		bullet_template.angle = _AL;
		bullet_template.speed.v = byte_23DC7;
		bullet_template.center.x.v = polar(word_1F33E, TO_SP(32), CosTable8[angle]);
		bullet_template.center.y.v = polar(word_1F340, TO_SP(32), SinTable8[angle]);
		bullets_add();

		_AL = angle;
		_AL += _AL;
		_AL += 0x80;
		bullet_template.angle = _AL;
		angle += 0x80;
		bullet_template.center.x.v = polar(word_1F33E, TO_SP(32), CosTable8[angle]);
		bullet_template.center.y.v = polar(word_1F340, TO_SP(32), SinTable8[angle]);
		bullets_add();
	}

	if((static_cast<uint8_t>(word_1F3B0) & 1) == 0) {
		byte_23DC7++;
	}
	_AL = static_cast<uint8_t>(word_1F3B0);
	_AL += _AL;
	angle = _AL;
	_AL += -0x40;
	angle = _AL;
	word_1F340 = polar(point_1F342.y.v, TO_SP(48), SinTable8[_AL]);

	if(word_1F3B0 >= 0x80) {
		byte_1F34F = 1;
		word_1F3B0 = 0;
	}
}

extern "C" void pascal near reimu_10DA0(void)
{
	sub_F356();
	bullet_template.type = BT_BULLET16_DEFAULT;
	bullet_template.speed.v = byte_1F3A1;
	bullet_template.pid = (1 - pid_current);
	bullet_template.center.x.v = word_1F33E;
	bullet_template.center.y.v = word_1F340;

	if(word_1F3B0 < 0x40) {
		if(round_frame_mod16 != 0) {
			return;
		}
		bullet_template.angle = 0x20;
		bullet_template.group = BG_3_SPREAD_NARROW;
		bullets_add();
		bullet_template.angle = 0x60;
		bullets_add();
		return;
	}

	if(word_1F3B0 < 0x64) {
		if(round_frame_mod16 != 0) {
			return;
		}
		bullet_template.group = BG_5_SPREAD_MEDIUM;
		bullet_template.angle = 0x40;
		bullets_add();
		return;
	}

	byte_1F34F = 1;
	word_1F3B0 = 0;
}

extern "C" void pascal near reimu_10E16(void)
{
	uint8_t angle;
	register int i;

	sub_F356();
	if(word_1F3B0 == 0) {
		byte_1F353 = 1;
		byte_23DC8 = randring_far_next16_raw();
	}

	if(word_1F3B0 < 0x18) {
		for(i = 0; i < static_cast<int>(byte_1F3A2); i++) {
			angle = (((i << 8) / static_cast<int>(byte_1F3A2)) + byte_23DC8);
			word_23DCA[i] = polar(
				word_1F33E,
				((word_1F3B0 << SUBPIXEL_BITS) << 1),
				CosTable8[angle]
			);
			word_23DD6[i] = polar(
				word_1F340,
				((word_1F3B0 << SUBPIXEL_BITS) << 1),
				SinTable8[angle]
			);
		}
		byte_23DC8 += 8;
		return;
	}

	if(word_1F3B0 < 0x50) {
		for(i = 0; i < static_cast<int>(byte_1F3A2); i++) {
			angle = (((i << 8) / static_cast<int>(byte_1F3A2)) + byte_23DC8);
			word_23DCA[i] = polar(word_1F33E, TO_SP(48), CosTable8[angle]);
			word_23DD6[i] = polar(word_1F340, TO_SP(48), SinTable8[angle]);
		}
		byte_23DC8 += 8;
		if(round_frame_mod16 != 0) {
			return;
		}
		bullet_template.type = BT_BULLET16_DEFAULT;
		bullet_template.speed.v = byte_1F3A3;
		bullet_template.pid = (1 - pid_current);
		bullet_template.center.x.v = word_1F33E;
		bullet_template.center.y.v = word_1F340;
		bullet_template.angle = randring_far_next16_raw();
		bullet_template.group = BG_RING;
		bullet_template.count = byte_1F3A4;
		bullets_add();
		return;
	}

	if(word_1F3B0 == 0x50) {
		for(i = 0; i < static_cast<int>(byte_1F3A2); i++) {
			angle = (((i << 8) / static_cast<int>(byte_1F3A2)) + byte_23DC8);
			reimu_1A2CE(word_23DCA[i], word_23DD6[i], angle);
		}
		byte_1F34F = 1;
		word_1F3B0 = 0;
		byte_1F353 = 0;
	}
}
