#pragma codeseg main_03_TEXT

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
#include "th03/math/randring.hpp"

extern "C" subpixel_t word_1F33E;
extern "C" subpixel_t word_1F340;
extern "C" uint16_t word_1F356;
extern "C" uint16_t word_1F3B0;
extern "C" sprite16_offset_t sprite_1F34C;
extern "C" uint8_t pid_PID_so_attack;
extern "C" uint8_t byte_1F34E;
extern "C" uint8_t byte_1F34F;
extern "C" uint8_t byte_1F354;
extern "C" uint8_t byte_1F355;
extern "C" uint8_t byte_1F35E[];
extern "C" uint8_t byte_1F3A0;
extern "C" int16_t word_1DE12[];
extern "C" int16_t word_1DE24[];

extern "C" uint16_t far randring_far_next16_raw(void);
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
