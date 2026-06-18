#pragma codeseg main_03_TEXT

#include "codegen.hpp"
#include "libs/master.lib/master.hpp"
#include "platform.h"
#include "th01/math/subpixel.hpp"
#include "th03/main/bullet/bullet.hpp"
#include "th03/main/collmap.hpp"
#include "th03/main/hitbox.hpp"
#include "th03/main/player/cur.hpp"
#include "th03/main/player/gba.hpp"
#include "th03/math/polar.hpp"
#include "th03/math/randring.hpp"
#include "th02/snd/snd.h"

extern "C" uint8_t byte_1F324;
extern "C" uint8_t byte_1F34F;
extern "C" uint8_t byte_1F351;
extern "C" uint8_t byte_1F352;
extern "C" uint8_t byte_1F353;
extern "C" uint8_t byte_1F35E[];
extern "C" uint8_t byte_1F39F;
extern "C" uint8_t byte_1F3A0;
extern "C" uint8_t byte_1F3A1;
extern "C" uint8_t byte_1F3A2;
extern "C" uint8_t byte_1F3A3;
extern "C" uint8_t byte_1F3A4;
extern "C" uint8_t byte_1FE50;
extern "C" uint16_t word_1F34A;
extern "C" uint16_t word_1F3B0;
extern "C" uint16_t word_1F32A[];
extern "C" subpixel_t word_1F326;
extern "C" subpixel_t word_1F328;
extern "C" subpixel_t word_1F33E;
extern "C" subpixel_t word_1F340;
extern "C" subpixel_t word_1F346;
extern "C" subpixel_t word_1F348;
extern "C" uint8_t angle_1F350;
extern "C" uint8_t byte_1F34E;
extern uint16_t combo_points_for_boss_attack;

extern "C" void pascal far sub_A3A8(uint8_t pid);
extern "C" void pascal near sub_F1FA(uint16_t length, subpixel_t y, subpixel_t x);
extern "C" void pascal far marisa_19B06(pid_t pid, subpixel_t x, subpixel_t y);
extern "C" uint16_t far randring_far_next16_raw(void);

extern "C" void near sub_F356(void)
{
	word_1F33E += word_1F346;
	word_1F340 += word_1F348;
	angle_1F350++;
	word_1F348 = polar(0, 16, SinTable8[angle_1F350]);
	if(word_1F33E <= TO_SP(48)) {
		word_1F346 = TO_SP(2);
		return;
	}
	if(word_1F33E >= TO_SP(240)) {
		word_1F346 = TO_SP(-2);
	}
}

extern "C" void near sub_F3A9(void)
{
	word_1F33E += word_1F346;
	word_1F340 += TO_SP(2);
	if(word_1F33E <= TO_SP(48)) {
		word_1F346 = TO_SP(2);
	} else if(word_1F33E >= TO_SP(240)) {
		word_1F346 = TO_SP(-2);
	}
	word_1F32A[1 - pid_current] = 0;
	if(word_1F340 >= TO_SP(416)) {
		byte_1F34F = 0;
		gba_boss_launched_by = PID_NONE;
		combo_points_for_boss_attack = 5120;
	}
}

extern "C" uint8_t near sub_F402(void)
{
	if(gba_flag_active[pid_current] != GBAF_BOSS) {
		goto ret_false;
	}
	if(gba_boss_launched_by != PID_NONE) {
		goto already_launched;
	}

	_asm {
		mov	si, offset byte_1F35E
		cmp	byte ptr pid_current, 1
		jnz	short copy_params
		add	si, 20h
copy_params:
		mov	di, offset word_1F33E
		mov	ax, ds
		mov	es, ax
		mov	cx, 10h
		rep	movsw
	}
	byte_1F352 = (randring_far_next16_and(7) + 1);
	word_1F3B0 = 0;
	gba_boss_launched_by = pid_current;
	gba_flag_active[pid_current] = GBAF_NONE;
	snd_se_play(18);
	sub_A3A8(1 - pid_current);
	word_1F32A[1 - pid_current] = 1;
	return 1;

already_launched:
	if(byte_1F34F != 0xFF) {
		byte_1F34F = 0xFF;
		word_1F3B0 = 0;
		sub_A3A8(1 - pid_current);
		word_1F32A[pid_current] = 0;
	}

ret_false:
	return 0;
}

extern "C" void far sub_F4B4(void)
{
	uint8_t pid_other = (1 - pid_current);

	if(static_cast<int16_t>(word_1F34A) > 0) {
		return;
	}
	if(byte_1F34F == 0xFF) {
		return;
	}
	byte_1F34F = 0xFF;
	word_1F3B0 = 0;
	sub_A3A8(pid_other);
	if(gba_flag_active[pid_other] != GBAF_BOSS) {
		combo_points_for_boss_attack = 5120;
	}
	word_1F32A[pid_other] = 0;
	if(gba_boss_level < GBA_BOSS_LEVEL_MAX) {
		gba_boss_level++;
	}
}

extern "C" void pascal near sub_F512(void)
{
	if(byte_1F34F != 0x80) {
		word_1F326 = word_1F33E;
		word_1F328 = (word_1F340 + TO_SP(128));
	}
}

extern "C" void pascal near sub_F52D(void)
{
	uint8_t next;

	sub_F356();
	if(word_1F3B0 >= 0x50) {
		while(1) {
			next = randring_far_next16_and(0x0F);
			if((byte_1F324 + 2) < next) {
				break;
			}
			if((byte_1F324 - 2) <= next) {
				continue;
			}
			break;
		}
		byte_1F324 = next;
		word_1F3B0 = 0;
		byte_1F34F = (next + 2);
		byte_1F351++;
		if(byte_1F351 > byte_1F352) {
			byte_1F34F = 0x80;
		}
	}
}

#pragma option -G
extern "C" void pascal near sub_F58C(void)
{
	_asm {
		push	word ptr word_1F33E
		push	word ptr word_1F340
		push	word ptr word_1F3B0
		call	near ptr sub_F1FA
		mov	byte ptr byte_1F34F, al
	}
	if(byte_1F34F == 0) {
		gba_boss_launched_by = PID_NONE;
	}
}

extern "C" void pascal far marisa_F5AF(uint16_t slot)
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
	*reinterpret_cast<uint16_t near *>(record + 0x0E) = 0x0280;
	if(slot != 0) {
		*reinterpret_cast<uint16_t near *>(record + 0x0E) += 0x28;
	}
	record[0x15] = 0;
}

extern "C" void pascal near marisa_F5FE(void)
{
	byte_1F353 = 1;
	if(word_1F3B0 < 0x20) {
		return;
	}
	if(word_1F3B0 == 0x20) {
		byte_1FE50 = 0;
	}
	if(word_1F3B0 < 0x50) {
		if((word_1F3B0 & 0x03) != 0) {
			return;
		}
		bullet_template.angle = byte_1FE50;
		bullet_template.group = BG_2_SPREAD_HORIZONTALLY_SYMMETRIC;
		bullet_template.center.x.v = word_1F33E;
		bullet_template.center.y.v = word_1F340;
		bullet_template.speed.v = byte_1F39F;
		bullet_template.type = BT_BULLET16_DEFAULT;
		bullet_template.pid = (1 - pid_current);
		bullets_add();

		bullet_template.speed.v = (static_cast<int16_t>(byte_1F39F) / 2);
		bullets_add();
		byte_1FE50 += 7;
		return;
	}
	byte_1F353 = 0;
	byte_1F34F = 1;
	word_1F3B0 = 0;
}

#pragma warn -aus
#pragma option -G-
extern "C" void pascal near marisa_F685(void)
{
	pid_t pid_other = (1 - pid_current);

	byte_1F353 = 1;
	if(word_1F3B0 < 0x38) {
		return;
	}
	if(word_1F3B0 != 0x40) {
		goto check_50;
	}
	__emit__(0x66, 0x68, 0x00, 0x17, 0x80, 0x00);
	_asm {
		push	word ptr [bp-1]
		call	far ptr marisa_19B06
		push	1180h
	}
	goto spawn_second;

check_50:
	if(word_1F3B0 != 0x50) {
		goto check_60;
	}
	__emit__(0x66, 0x68, 0x00, 0x17, 0x80, 0x03);
	_asm {
		push	word ptr [bp-1]
		call	far ptr marisa_19B06
		push	0E80h
	}
	goto spawn_second;

check_60:
	if(word_1F3B0 != 0x60) {
		goto check_70;
	}
	__emit__(0x66, 0x68, 0x00, 0x17, 0x80, 0x06);
	_asm {
		push	word ptr [bp-1]
		call	far ptr marisa_19B06
		push	0B80h
	}
	goto spawn_second;

check_70:
	if(word_1F3B0 != 0x70) {
		goto check_done;
	}
	__emit__(0x66, 0x68, 0x00, 0x17, 0x80, 0x09);
	_asm {
		push	word ptr [bp-1]
		call	far ptr marisa_19B06
		push	0880h
	}

spawn_second:
	_asm {
		push	1700h
		push	word ptr [bp-1]
		call	far ptr marisa_19B06
	}
	return;

check_done:
	if(word_1F3B0 == 0x84) {
		byte_1F353 = 0;
		byte_1F34F = 1;
		word_1F3B0 = 0;
	}
}
#pragma option -G
#pragma warn .aus

extern "C" void pascal near marisa_F72D(void)
{
	sub_F356();
	if((word_1F3B0 & 0x1F) == 0) {
		bullet_template.angle = randring_far_next16_raw();
		bullet_template.group = BG_RING;
		bullet_template.center.x.v = word_1F33E;
		bullet_template.center.y.v = word_1F340;
		bullet_template.pid = (1 - pid_current);
		if(gba_boss_level < 8) {
			_AL = byte_1F3A0;
		} else {
			bullet_template.speed.v = byte_1F3A2;
			bullet_template.count = byte_1F3A3;
			bullet_template.type = BT_BULLET16_DEFAULT;
			bullets_add();
			bullet_template.angle += (
				256 / static_cast<int16_t>(byte_1F3A3)
			);
			_AL = byte_1F3A4;
		}
		bullet_template.count = _AL;
		bullet_template.speed.v = byte_1F3A1;
		bullet_template.type = BT_PELLET_CLOUD;
		bullets_add();
	}
	if(word_1F3B0 >= 0x82) {
		byte_1F34F = 1;
		word_1F3B0 = 0;
	}
}

#pragma warn -aus
#pragma option -G-
extern "C" void pascal near marisa_F7BD(void)
{
	pid_t pid_other = (1 - pid_current);

	byte_1F353 = 1;
	if(word_1F3B0 < 0x38) {
		return;
	}
	if(word_1F3B0 != 0x40) {
		goto check_50;
	}
	__emit__(0x66, 0x68, 0x00, 0x17, 0x00, 0x02);
	_asm {
		push	word ptr [bp-1]
		call	far ptr marisa_19B06
		push	1000h
	}
	goto spawn_second;

check_50:
	if(word_1F3B0 != 0x50) {
		goto check_60;
	}
	__emit__(0x66, 0x68, 0x00, 0x17, 0x00, 0x05);
	_asm {
		push	word ptr [bp-1]
		call	far ptr marisa_19B06
		push	0D00h
	}
	goto spawn_second;

check_60:
	if(word_1F3B0 != 0x60) {
		goto check_done;
	}
	__emit__(0x66, 0x68, 0x00, 0x17, 0x00, 0x08);
	_asm {
		push	word ptr [bp-1]
		call	far ptr marisa_19B06
		push	0A00h
	}

spawn_second:
	_asm {
		push	1700h
		push	word ptr [bp-1]
		call	far ptr marisa_19B06
	}
	return;

check_done:
	if(word_1F3B0 == 0x70) {
		byte_1F353 = 0;
		byte_1F34F = 1;
		word_1F3B0 = 0;
	}
}
#pragma option -G
#pragma warn .aus

#pragma warn -aus
#pragma option -G-
extern "C" void pascal far gba_boss_update_marisa(void)
{
	pid_t pid_other;
	uint16_t state;

	if(sub_F402()) {
		byte_1F39F = ((gba_boss_level * 2) + 0x32);
		byte_1F3A0 = ((gba_boss_level * 2) + 0x18);
		byte_1F3A1 = (gba_boss_level + 0x20);
		byte_1F3A2 = (gba_boss_level + 0x0A);
		byte_1F3A3 = (gba_boss_level + 0x16);
		byte_1F3A4 = (gba_boss_level + 0x16);
	}

	if(pid_current != gba_boss_launched_by) {
		return;
	}

	pid_other = (1 - pid_current);
	sub_F512();
	word_1F3B0++;
	state = byte_1F34F;

	// TCC places the generated switch table before any post-function
	// codestring, so keep this dispatch table as a raw byte island.
	__emit__(0xB9, 0x14, 0x00, 0xBB, 0x66, 0x07);
	__emit__(0x2E, 0x8B, 0x07, 0x3B, 0x46, 0xFC, 0x74, 0x07);
	__emit__(0x83, 0xC3, 0x02, 0xE2, 0xF3, 0xEB, 0x36);
	__emit__(0x2E, 0xFF, 0x67, 0x28);
	__emit__(0x83, 0x3E, 0x50, 0x1E, 0x64, 0x0F, 0x85, 0x87, 0x00);
	__emit__(0xC7, 0x06, 0x50, 0x1E, 0x00, 0x00);
	__emit__(0xC6, 0x06, 0xEF, 0x1D, 0x01, 0xEB, 0x1C);
	__emit__(0xE8, 0x51, 0xFC, 0xEB, 0x17);
	__emit__(0xE8, 0x1D, 0xFD, 0xEB, 0x12);
	__emit__(0xE8, 0x9F, 0xFD, 0xEB, 0x0D);
	__emit__(0xE8, 0x42, 0xFE, 0xEB, 0x08);
	__emit__(0xE8, 0xCD, 0xFE, 0xEB, 0x03);
	__emit__(0xE8, 0xB4, 0xFA);

	collmap_center.x.v = word_1F33E;
	collmap_center.y.v = word_1F340;
	collmap_stripe_tile_w.v = (56 / COLLMAP_TILE_W);
	collmap_tile_h.v = (56 / COLLMAP_TILE_H);
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

gba_boss_update_marisa_ret:
	}
}
#pragma codestring "\x00\x00\x00\x01\x00\x02\x00\x03\x00\x04\x00\x05\x00\x06\x00\x07\x00\x08\x00\x09\x00\x0A\x00\x0B\x00\x0C\x00\x0D\x00\x0E\x00\x0F\x00\x10\x00\x11\x00\x80\x00\xFF\x00\xD3\x06\xE9\x06\xEE\x06\xEE\x06\xEE\x06\xEE\x06\xFD\x06\xFD\x06\xF3\x06\xF3\x06\xF3\x06\xF8\x06\xF8\x06\xF8\x06\xF8\x06\xF8\x06\xF8\x06\xFD\x06\x02\x07\x63\x07"
#pragma option -G
#pragma warn .aus

#pragma codeseg
