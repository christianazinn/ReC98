#pragma codeseg main_03_TEXT

#include "codegen.hpp"
#include "libs/master.lib/master.hpp"
#include "libs/sprite16/sprite16.h"
#include "platform.h"
#include "th01/math/subpixel.hpp"
#include "th02/snd/snd.h"
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
extern "C" sprite16_offset_t sprite_1F34C;
extern "C" uint8_t pid_PID_so_attack;
extern "C" uint8_t byte_1F34E;
extern "C" uint8_t byte_1F34F;
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
extern "C" int16_t word_1DE12[];
extern "C" int16_t word_1DE24[];

extern "C" uint16_t far randring_far_next16_raw(void);
extern "C" void near sub_F356(void);
extern "C" void near sub_F3A9(void);
extern "C" uint8_t near sub_F402(void);
extern "C" void far sub_F4B4(void);
extern "C" void pascal near sub_F512(void);
extern "C" void pascal near sub_F52D(void);
extern "C" void pascal near sub_F58C(void);

extern "C" void pascal near ellen_11814(int frame)
{
	screen_x_t left;
	screen_y_t top;
	sprite16_offset_t so;
	register int i;

	i = frame;
	so = (sprite_1F34C + (word_1DE12[i] * 0x28) + 0xFB10);
	sprite16_put_size.w.v = (96 / 16);
	sprite16_put_size.h = (word_1DE24[i] / 2);
	left = (
		playfield_fg_x_to_screen(word_1F33E, (1 - pid_current)) - 48
	);
	top = ((word_1F340 >> 4) + word_1DE12[i] - 40);
	sprite16_put(left, top, so);
}

extern "C" void pascal near ellen_11885(void)
{
	screen_x_t left;
	screen_y_t top;
	uint8_t pid_other;
	register int i;
	register sprite16_offset_t so;

	pid_other = (1 - pid_current);
	sprite16_put_size.w.v = (64 / 16);
	sprite16_put_size.h = 48;
	left = (playfield_fg_x_to_screen(word_1F33E, pid_other) - 32);
	top = ((word_1F340 >> 4) - 32);
	so = sprite_1F34C;
	if(byte_1F34E != 0) {
		so += 8;
	}
	sprite16_put(left, top, so);

	i = ((byte_1F354 / 4) % 9);
	ellen_11814(i);
	i--;
	if(i < 0) {
		i = 8;
	}
	ellen_11814(i);
	i--;
	if(i < 0) {
		i = 8;
	}
	ellen_11814(i);
}

extern "C" void pascal near ellen_1190A(
	subpixel_t radius, uint8_t frame, uint8_t angle
)
{
	screen_y_t top;
	int pid_other;
	sprite16_offset_t so;
	int j;
	register int i;
	register screen_x_t left;

	pid_other = (1 - pid_current);
	i = (frame % 9);
	ellen_11814(i);
	i--;
	if(i < 0) {
		i = 8;
	}
	ellen_11814(i);
	i--;
	if(i < 0) {
		i = 8;
	}
	ellen_11814(i);
	if((round_frame_mod2 != 0) && (word_1F3B0 < 0x40)) {
		return;
	}

	sprite16_put_size.w.v = (32 / 16);
	sprite16_put_size.h = 16;
	so = (pid_PID_so_attack + ((8 * ROW_SIZE) + (96 / BYTE_DOTS)));
	for(i = 0; i < 4; i++) {
		for(j = 0; j < 8; j++) {
			left = polar(word_1F33E, radius, CosTable8[angle]);
			top = polar(word_1F340, radius, SinTable8[angle]);
			left = (playfield_fg_x_to_screen(left, pid_other) - 16);
			top = (top >> 4);
			sprite16_put(left, top, so);
			angle += 0x20;
		}
		angle += 3;
		so -= 4;
	}
}

extern "C" void pascal far gba_boss_render_ellen(void)
{
	uint8_t pid_other;

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
		ellen_1190A(
			((200 - (word_1F3B0 * 2)) << 4),
			word_1F3B0,
			(word_1F3B0 * 3)
		);
		return;
	}
	if(byte_1F34F != 0xFF) {
		ellen_11885();
		return;
	}
	sub_F58C();
}

extern "C" void pascal far kotohime_11A6D(uint16_t slot)
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
	*reinterpret_cast<uint16_t near *>(record + 0x18) = 0;
	*reinterpret_cast<uint16_t near *>(record + 0x0E) = 0x0288;
	if(slot != 0) {
		*reinterpret_cast<uint16_t near *>(record + 0x0E) += 0x28;
	}
	record[0x15] = 0;
}

extern "C" void pascal near kotohime_11AC1(int randomize_angle)
{
	subpixel_t center_x;
	subpixel_t center_y;
	uint8_t angle;
	register int i;

	bullet_template.type = BT_BULLET16_DEFAULT;
	for(i = 0; i < static_cast<int>(byte_1F3A0); i++) {
		if(randomize_angle != 0) {
			bullet_template.angle = randring_far_next16_raw();
		}
		angle = (((i << 8) / static_cast<int>(byte_1F3A0)) + byte_1F355);
		center_x = polar(word_1F33E, word_1F356, CosTable8[angle]);
		center_y = polar(word_1F340, word_1F356, SinTable8[angle]);
		bullet_template.center.x.v = center_x;
		bullet_template.center.y.v = center_y;
		bullets_add();
	}
}

extern "C" void pascal near kotohime_11B51(void)
{
	if(word_1F3B0 == 1) {
		byte_1F353 = 1;
		word_1F356 = 0;
		byte_1F354 = 0;
	}

	byte_1F355 += 4;
	if(word_1F356 > TO_SP(64)) {
		byte_1F353 = 2;
		_AL = byte_1F355;
		_AL += 2;
		byte_1F355 = _AL;
		byte_1F354++;
		if(byte_1F354 > 0x40) {
			bullet_template.group = BG_16_RING;
			bullet_template.speed.v = byte_1F39F;
			kotohime_11AC1(1);
			snd_se_play(3);
			byte_1F353 = 0;
			byte_1F34F = 1;
			word_1F3B0 = 0;
			return;
		}
		return;
	}

	sub_F356();
	word_1F356 += 0x20;
}

extern "C" void pascal near kotohime_11BC6(void)
{
	if(word_1F3B0 == 1) {
		byte_1F353 = 1;
		word_1F356 = 0;
		byte_1F354 = 0;
		byte_1F355 = 0;
	}

	_AL = byte_1F355;
	_AL += -2;
	byte_1F355 = _AL;
	if(word_1F356 > TO_SP(64)) {
		byte_1F353 = 2;
		word_1F356 += 8;
		_AL = byte_1F355;
		_AL += 2;
		byte_1F355 = _AL;
		byte_1F354++;
		if(byte_1F354 > 0x40) {
			bullet_template.group = BG_2_SPREAD_NARROW_AIMED;
			bullet_template.angle = 0;
			bullet_template.speed.v = byte_1F3A2;
			kotohime_11AC1(0);
			bullet_template.group = BG_5_SPREAD_MEDIUM_AIMED;
			bullet_template.angle = 0;
			bullet_template.speed.v = byte_1F3A1;
			kotohime_11AC1(0);
			snd_se_play(3);
			byte_1F353 = 0;
			byte_1F34F = 1;
			word_1F3B0 = 0;
			return;
		}
		return;
	}

	sub_F356();
	word_1F356 += 0x20;
}

extern "C" void pascal near kotohime_11C5F(void)
{
	sub_F356();
	if((word_1F3B0 % static_cast<uint16_t>(byte_1F3A4)) == 1) {
		snd_se_play(3);
		bullet_template.type = BT_PELLET;
		bullet_template.angle = static_cast<uint8_t>(word_1F3B0);
		bullet_template.speed.v = byte_1F3A3;
		bullet_template.group = BG_RING;
		bullet_template.count = 4;

		bullet_template.center.x.v = (word_1F33E + TO_SP(-48));
		bullet_template.center.y.v = word_1F340;
		bullets_add();

		bullet_template.center.x.v = (word_1F33E + TO_SP(-24));
		bullet_template.center.y.v = (word_1F340 + TO_SP(12));
		bullets_add();

		bullet_template.center.x.v = word_1F33E;
		bullet_template.center.y.v = (word_1F340 + TO_SP(24));
		bullets_add();

		bullet_template.center.x.v = (word_1F33E + TO_SP(24));
		bullet_template.center.y.v = (word_1F340 + TO_SP(12));
		bullets_add();

		bullet_template.center.x.v = (word_1F33E + TO_SP(48));
		bullet_template.center.y.v = word_1F340;
		bullets_add();
	}

	if(word_1F3B0 >= 0x60) {
		byte_1F34F = 1;
		word_1F3B0 = 0;
	}
}

extern "C" void pascal near kotohime_11D1A(void)
{
	subpixel_t center_x;
	subpixel_t center_y;
	uint8_t angle;
	register int inner;
	register int outer;

	if(word_1F3B0 == 1) {
		byte_1F353 = 1;
		word_1F356 = 0;
		byte_1F354 = 0;
	}

	_AL = byte_1F355;
	_AL += -4;
	byte_1F355 = _AL;
	if(word_1F356 > TO_SP(64)) {
		byte_1F353 = 2;
		word_1F356 += 8;
		_AL = byte_1F355;
		_AL += -2;
		byte_1F355 = _AL;
		byte_1F354++;
		if(byte_1F354 > 0x40) {
			bullet_template.group = BG_1;
			bullet_template.type = BT_BULLET16_DEFAULT;
			for(outer = 0; outer < static_cast<int>(byte_1F3A0); outer++) {
				angle = (
					((outer << 8) / static_cast<int>(byte_1F3A0)) + byte_1F355
				);
				center_x = polar(word_1F33E, word_1F356, CosTable8[angle]);
				center_y = polar(word_1F340, word_1F356, SinTable8[angle]);
				bullet_template.center.x.v = center_x;
				bullet_template.center.y.v = center_y;
				for(inner = 0; inner < static_cast<int>(byte_1F3A5); inner++) {
					bullet_template.angle = (
						((inner << 7) / static_cast<int>(byte_1F3A5)) +
						angle +
						0x40
					);
					bullet_template.speed.v = (
						((inner * 0x30) / static_cast<int>(byte_1F3A5)) +
						0x10
					);
					bullets_add();
				}
			}
			snd_se_play(3);
			byte_1F353 = 0;
			byte_1F34F = 1;
			word_1F3B0 = 0;
		}
		return;
	}

	sub_F356();
	word_1F356 += 0x20;
}

#pragma warn -aus
#pragma option -G-
extern "C" void pascal far gba_boss_update_kotohime(void)
{
	pid_t pid_other;

	if(sub_F402()) {
		byte_1F39F = ((gba_boss_level / 2) + 0x10);
		byte_1F3A0 = ((gba_boss_level / 3) + 6);
		byte_1F3A1 = ((gba_boss_level / 2) + 0x10);
		byte_1F3A2 = ((gba_boss_level / 2) + 0x20);
		byte_1F3A3 = ((gba_boss_level / 2) + 0x18);
		byte_1F3A4 = (0x20 - (gba_boss_level / 2));
		byte_1F3A5 = ((gba_boss_level / 2) + 8);
	}

	if(pid_current != gba_boss_launched_by) {
		return;
	}

	pid_other = (1 - pid_current);
	sub_F512();
	bullet_template.pid = pid_other;
	word_1F3B0++;

	switch(byte_1F34F) {
	case 0:
		if(word_1F3B0 != 0x64) {
			return;
		}
		word_1F3B0 = 0;
		byte_1F34F = 1;
		break;
	case 1:
		sub_F52D();
		break;
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
		kotohime_11B51();
		break;
	case 7:
	case 8:
	case 9:
		kotohime_11BC6();
		break;
	case 12:
	case 13:
	case 14:
		kotohime_11C5F();
		break;
	case 10:
	case 11:
	case 15:
	case 16:
	case 17:
		kotohime_11D1A();
		break;
	case 0x80:
		sub_F3A9();
		break;
	case 0xFF:
		return;
	default:
		break;
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
#pragma option -G
#pragma warn .aus
