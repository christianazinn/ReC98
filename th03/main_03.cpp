#pragma codeseg main_03_TEXT

#include "platform.h"
#include "th01/math/subpixel.hpp"
#include "th03/main/bullet/bullet.hpp"
#include "th03/main/player/cur.hpp"
#include "th03/main/player/gba.hpp"
#include "th03/math/randring.hpp"

extern "C" uint8_t byte_1F324;
extern "C" uint8_t byte_1F34F;
extern "C" uint8_t byte_1F351;
extern "C" uint8_t byte_1F352;
extern "C" uint8_t byte_1F353;
extern "C" uint8_t byte_1F35E[];
extern "C" uint8_t byte_1F39F;
extern "C" uint8_t byte_1FE50;
extern "C" uint16_t word_1F3B0;
extern "C" subpixel_t word_1F326;
extern "C" subpixel_t word_1F328;
extern "C" subpixel_t word_1F33E;
extern "C" subpixel_t word_1F340;

extern "C" void pascal near sub_F1FA(uint16_t length, subpixel_t y, subpixel_t x);
extern "C" void near sub_F356(void);

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

#pragma codeseg
