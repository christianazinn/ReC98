#pragma codeseg main_03_TEXT

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

extern "C" subpixel_t word_1F33E;
extern "C" subpixel_t word_1F340;
extern "C" uint16_t word_1F34A;
extern "C" uint16_t word_1F3B0;
extern "C" sprite16_offset_t sprite_1F34C;
extern "C" uint8_t pid_PID_so_attack;
extern "C" uint8_t byte_1F34E;
extern "C" uint8_t byte_1F34F;
extern "C" uint8_t byte_1F353;
extern "C" uint8_t byte_1F35E[];
extern "C" uint8_t byte_1F39F;
extern "C" uint8_t byte_1F3A0;
extern "C" uint8_t byte_1F3A1;
extern "C" uint8_t byte_1F3A2;
extern "C" uint8_t byte_1F354;
extern "C" subpixel_t word_23DCA[];
extern "C" subpixel_t word_23DD6[];
extern "C" uint8_t byte_23DE2;
extern "C" uint8_t byte_23DE3;

extern "C" uint16_t far randring_far_next16_raw(void);
extern "C" void pascal far SUB_CE0C(subpixel_t x, subpixel_t y, uint16_t pid);
extern "C" void pascal far ellen_194A9(
	subpixel_t x, subpixel_t y, uint8_t angle, uint8_t delta
);
extern "C" void near sub_F356(void);
extern "C" void near sub_F3A9(void);
extern "C" uint8_t near sub_F402(void);
extern "C" void far sub_F4B4(void);
extern "C" void pascal near sub_F512(void);
extern "C" void pascal near sub_F52D(void);
extern "C" void pascal near sub_F58C(void);

extern "C" void pascal near reimu_111A3(void)
{
	screen_x_t left;
	screen_y_t top;
	pid_t pid_other;
	uint8_t frame_quarter;
	register sprite16_offset_t so;
	register int i;

	pid_other = (1 - pid_current);
	sprite16_put_size.w.v = (96 / 16);
	sprite16_put_size.h = 48;
	if(pid_other == 0) {
		sprite16_clip.left = PLAYFIELD1_CLIP_LEFT;
		sprite16_clip.right = PLAYFIELD1_CLIP_RIGHT;
	} else {
		sprite16_clip.left = PLAYFIELD2_CLIP_LEFT;
		sprite16_clip.right = PLAYFIELD2_CLIP_RIGHT;
	}

	so = sprite_1F34C;
	if(byte_1F34E != 0) {
		so += 0x0C;
	}
	left = (playfield_fg_x_to_screen(word_1F33E, pid_other) - 48);
	top = ((word_1F340 >> 4) - 32);
	sprite16_put(left, top, so);

	if(byte_1F353 != 1) {
		return;
	}

	sprite16_put_size.w.v = (48 / 16);
	sprite16_put_size.h = 24;
	so = (sprite_1F34C + 0xFFF4);
	frame_quarter = (word_1F3B0 >> 2);
	if((frame_quarter & 3) == 1) {
		so += 6;
	} else if((frame_quarter & 3) != 0) {
		so += (((frame_quarter & 1) * 6) + (24 * ROW_SIZE));
	}

	for(i = 0; i < static_cast<int>(byte_1F3A2); i++) {
		left = (playfield_fg_x_to_screen(word_23DCA[i], pid_other) - 24);
		top = ((word_23DD6[i] >> 4) - 8);
		sprite16_put(left, top, so);
	}
}

extern "C" void pascal near reimu_112A6(uint8_t angle, subpixel_t radius)
{
	screen_y_t top;
	int i;
	uint8_t frame_quarter;
	pid_t pid_other;
	register sprite16_offset_t so;
	register screen_x_t left;

	pid_other = (1 - pid_current);
	if((round_frame_mod2 != 0) && (word_1F3B0 < 0x40)) {
		return;
	}

	sprite16_put_size.w.v = (48 / 16);
	sprite16_put_size.h = 24;
	if(pid_current != 0) {
		sprite16_clip.left = PLAYFIELD1_CLIP_LEFT;
		sprite16_clip.right = PLAYFIELD1_CLIP_RIGHT;
	} else {
		sprite16_clip.left = PLAYFIELD2_CLIP_LEFT;
		sprite16_clip.right = PLAYFIELD2_CLIP_RIGHT;
	}

	so = (pid_PID_so_attack + (8 * ROW_SIZE));
	frame_quarter = (word_1F3B0 >> 2);
	if((frame_quarter & 3) == 1) {
		so += 6;
	} else if((frame_quarter & 3) != 0) {
		so += (((frame_quarter & 1) * 6) + (24 * ROW_SIZE));
	}

	i = 0;
	while(i < 8) {
		left = polar(word_1F33E, radius, CosTable8[angle]);
		top = polar(word_1F340, radius, SinTable8[angle]);
		left = (playfield_fg_x_to_screen(left, pid_other) - 24);
		top = ((top >> 4) - 8);
		sprite16_put(left, top, so);
		i++;
		angle += 0x20;
	}
}

extern "C" void pascal far gba_boss_render_reimu(void)
{
	if(pid_current != gba_boss_launched_by) {
		return;
	}

	if(byte_1F34F == 0) {
		reimu_112A6(word_1F3B0, (TO_SP(200) - (word_1F3B0 << 5)));
		return;
	}
	if(byte_1F34F != 0xFF) {
		reimu_111A3();
		return;
	}
	sub_F58C();
}

extern "C" void pascal far ellen_113E2(uint16_t slot)
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
	record[0x16] = 0;
	record[0x17] = 0;
	*reinterpret_cast<uint16_t near *>(record + 0x0E) = 0x0780;
	if(slot != 0) {
		*reinterpret_cast<uint16_t near *>(record + 0x0E) += 0x28;
	}
	record[0x15] = 0;
}

extern "C" void pascal near ellen_11439(void)
{
	pid_t pid_other;
	register int i;

	pid_other = (1 - pid_current);
	if((word_1F3B0 == 0x10) || (word_1F3B0 == 0x14)) {
		SUB_CE0C(word_1F33E, word_1F340, static_cast<uint16_t>(pid_other));
		return;
	}

	if(word_1F3B0 == 0x18) {
		SUB_CE0C(word_1F33E, word_1F340, static_cast<uint16_t>(pid_other));
		byte_23DE2 = 0x40;
		return;
	}

	if(word_1F3B0 < 0x1C) {
		return;
	}

	bullet_template.angle = byte_23DE2;
	bullet_template.group = BG_2_SPREAD_HORIZONTALLY_SYMMETRIC;
	bullet_template.center.x.v = word_1F33E;
	bullet_template.center.y.v = word_1F340;
	bullet_template.speed.v = byte_1F39F;
	bullet_template.type = BT_BULLET16_DEFAULT;
	bullet_template.pid = pid_other;
	bullet_template.sprite_offset = (64 / BYTE_DOTS);
	if(pid_current != 0) {
		bullet_template.sprite_offset += (8 * ROW_SIZE);
	}

	if(word_1F3B0 < 0x24) {
		_asm {
			test byte ptr word_1F3B0, 1
			jnz end_check
			jmp single_shot
		}
	}
	if(word_1F3B0 == 0x24) {
		snd_se_play(3);
		for(i = 0; i < 8; i++) {
			bullets_add();
			byte_23DE2 += 8;
			bullet_template.angle = byte_23DE2;
		}
		goto end_check;
	} else {
		if(word_1F3B0 > 0x2C) {
			goto end_check;
		}
		if((word_1F3B0 & 1) != 0) {
			goto end_check;
		}
	}

single_shot:
	snd_se_play(3);
	bullets_add();
	byte_23DE2 += 8;

end_check:
	if(word_1F3B0 >= 0x2C) {
		byte_1F353 = 0;
		byte_1F34F = 1;
		word_1F3B0 = 0;
	}
}

extern "C" void pascal near ellen_11547(void)
{
	pid_t pid_other;
	uint8_t angle;
	register int i;

	pid_other = (1 - pid_current);
	byte_1F354 += 3;
	if(word_1F3B0 < 0x20) {
		return;
	}

	if((word_1F3B0 == 0x20) || (word_1F3B0 == 0x24) || (word_1F3B0 == 0x28)) {
		SUB_CE0C(word_1F33E, word_1F340, static_cast<uint16_t>(pid_other));
		return;
	}

	if(word_1F3B0 == 0x30) {
		i = 0;
		angle = randring_far_next16_raw();
		while(static_cast<int>(byte_1F3A0) > i) {
			ellen_194A9(word_1F33E, word_1F340, angle, 2);
			i++;
			angle += (256 / static_cast<int>(byte_1F3A0));
		}
		return;
	}

	if(word_1F3B0 == 0x60) {
		i = 0;
		angle = randring_far_next16_raw();
		while(static_cast<int>(byte_1F3A0) > i) {
			ellen_194A9(word_1F33E, word_1F340, angle, 254);
			i++;
			angle += (256 / static_cast<int>(byte_1F3A0));
		}
		byte_1F34F = 1;
		word_1F3B0 = 0;
	}
}

extern "C" void pascal near ellen_11620(void)
{
	sub_F356();
	if((word_1F3B0 % static_cast<uint16_t>(byte_1F3A2)) == 1) {
		snd_se_play(10);
		bullet_template.group = BG_4_SPREAD_NARROW;
		bullet_template.center.x.v = word_1F33E;
		bullet_template.center.y.v = word_1F340;
		bullet_template.angle = byte_23DE3;
		bullet_template.speed.v = byte_1F3A1;
		bullet_template.type = BT_PELLET;
		bullet_template.pid = (1 - pid_current);
		bullets_add();

		bullet_template.angle = (byte_23DE3 + 0x80);
		bullets_add();

		bullet_template.speed.v = (static_cast<int16_t>(byte_1F3A1) / 2);
		bullets_add();

		bullet_template.angle = byte_23DE3;
		bullets_add();

		byte_23DE3 += 0x10;
	}

	if(word_1F3B0 >= 0x60) {
		byte_1F34F = 1;
		word_1F3B0 = 0;
	}
}

#pragma warn -aus
#pragma option -G-
extern "C" void pascal far gba_boss_update_ellen(void)
{
	pid_t pid_other;
	uint16_t state;

	if(sub_F402()) {
		byte_1F39F = ((gba_boss_level * 2) + 0x32);
		byte_1F3A0 = ((gba_boss_level / 10) + 4);
		byte_1F3A1 = (gba_boss_level + 0x30);
		byte_1F3A2 = (10 - (gba_boss_level / 3));
	}

	if(pid_current != gba_boss_launched_by) {
		return;
	}

	pid_other = (1 - pid_current);
	sub_F512();
	byte_1F354++;
	word_1F3B0++;
	state = byte_1F34F;

	// TCC places the generated switch table before any post-function
	// codestring, so keep this dispatch table as a raw byte island.
	__emit__(0xB9, 0x14, 0x00, 0xBB, 0xD4, 0x25);
	__emit__(0x2E, 0x8B, 0x07, 0x3B, 0x46, 0xFC, 0x74, 0x07);
	__emit__(0x83, 0xC3, 0x02, 0xE2, 0xF3, 0xEB, 0x31);
	__emit__(0x2E, 0xFF, 0x67, 0x28);
	__emit__(0x83, 0x3E, 0x50, 0x1E, 0x64, 0x0F, 0x85, 0x82, 0x00);
	__emit__(0xC7, 0x06, 0x50, 0x1E, 0x00, 0x00);
	__emit__(0xC6, 0x06, 0xEF, 0x1D, 0x01, 0xEB, 0x17);
	__emit__(0xE8, 0xDE, 0xDD, 0xEB, 0x12);
	__emit__(0xE8, 0xE5, 0xFC, 0xEB, 0x0D);
	__emit__(0xE8, 0xEE, 0xFD, 0xEB, 0x08);
	__emit__(0xE8, 0xC2, 0xFE, 0xEB, 0x03);
	__emit__(0xE8, 0x46, 0xDC);

	collmap_center.x.v = word_1F33E;
	collmap_center.y.v = word_1F340;
	collmap_stripe_tile_w.v = (40 / COLLMAP_TILE_W);
	collmap_tile_h.v = (48 / COLLMAP_TILE_H);
	collmap_pid = pid_other;
	collmap_set_rect_striped();

	hitbox_hittest_skip_explosions = true;
	hitbox.radius.x.v = TO_SP(24);
	hitbox.radius.y.v = TO_SP(24);
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

gba_boss_update_ellen_ret:
	}
}
#pragma codestring "\x00\x00\x00\x01\x00\x02\x00\x03\x00\x04\x00\x05\x00\x06\x00\x07\x00\x08\x00\x09\x00\x0A\x00\x0B\x00\x0C\x00\x0D\x00\x0E\x00\x0F\x00\x10\x00\x11\x00\x80\x00\xFF\x00\x46\x25\x5C\x25\x61\x25\x61\x25\x61\x25\x61\x25\x66\x25\x66\x25\x66\x25\x66\x25\x66\x25\x6B\x25\x6B\x25\x6B\x25\x6B\x25\x6B\x25\x6B\x25\x6B\x25\x70\x25\xD1\x25"
#pragma option -G
#pragma warn .aus
