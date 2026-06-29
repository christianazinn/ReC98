#pragma option -zCPLAYER_M_TEXT -zPmain_01 -G

#include "pc98.h"
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th03/main/hud/start.hpp"
#include "th03/main/round.hpp"
#include "th03/main/v_colors.hpp"
#include "th03/math/randring.hpp"
#include "th03/resident.hpp"
#include "x86real.h"

struct hud_start_anim_t {
	int x;
	int y;
	int v;
	unsigned char angle;
	unsigned char active;
	signed char angle_delta_first;
	signed char angle_delta_second;
};

extern "C" hud_start_anim_t byte_207E4[];
extern "C" int word_2087A;
extern "C" hud_start_anim_t near *word_20CE4;
extern "C" unsigned char byte_20CE6;
extern "C" int word_20CE8;
extern "C" int word_20CEA;
extern "C" int x_20CEC;

extern "C" void pascal near sub_BDC2(int x, int y, int patnum, int slot);
extern "C" void pascal near sub_BE2A(int x, int y);

extern "C" void near sub_BE5D(void)
{
	register int i;

	if(hud_start_flag == HSF_DONE) {
		return;
	}
	if(byte_20CE6 == 0) {
		grcg_setcolor(GC_RMW, V_WHITE);
		word_20CE4 = byte_207E4;
		i = 0;
		goto banner_test;

banner_loop:
		sub_BDC2(word_20CE4->x, word_20CE4->y, word_20CEA, i);
		sub_BDC2((word_20CE4->x + 32), word_20CE4->y, (word_20CEA + 1), i);
		sub_BDC2((word_20CE4->x + 64), word_20CE4->y, (word_20CEA + 2), i);
		sub_BDC2((word_20CE4->x + 96), word_20CE4->y, (word_20CEA + 3), i);
		sub_BDC2((544 - word_20CE4->x), word_20CE4->y, (word_20CEA + 3), i);
		sub_BDC2((512 - word_20CE4->x), word_20CE4->y, (word_20CEA + 2), i);
		sub_BDC2((480 - word_20CE4->x), word_20CE4->y, (word_20CEA + 1), i);
		sub_BDC2((448 - word_20CE4->x), word_20CE4->y, word_20CEA, i);
		i++;
		word_20CE4++;

banner_test:
		if(i < 16) {
			goto banner_loop;
		}
		x_20CEC = word_2087A;
		goto grcg_done;
	}

	if(byte_20CE6 == 1) {
		if(resident->game_mode != GM_STORY) {
			if((word_20CE8 >= 0x18) && (word_20CE8 < 0x40)) {
				super_put(208, 84, (round_id + 34));
				super_put(544, 84, (round_id + 34));
			} else if(word_20CE8 == 0x40) {
				goto banner_settle;
			}
		} else {
			if((word_20CE8 >= 0x18) && (word_20CE8 < 0x38)) {
				super_put(208, 84, (resident->story_stage + 34));
				super_put(544, 84, (resident->story_stage + 34));
			} else if(word_20CE8 == 0x38) {
banner_settle:
				x_20CEC = 96;
				word_20CEA = 30;
			}
		}

		super_put(x_20CEC, 84, word_20CEA);
		super_put((x_20CEC + 32), 84, (word_20CEA + 1));
		super_put((x_20CEC + 64), 84, (word_20CEA + 2));
		super_put((x_20CEC + 96), 84, (word_20CEA + 3));
		super_put((x_20CEC + 320), 84, word_20CEA);
		super_put((x_20CEC + 352), 84, (word_20CEA + 1));
		super_put((x_20CEC + 384), 84, (word_20CEA + 2));
		super_put((x_20CEC + 416), 84, (word_20CEA + 3));
		return;
	}

	if(byte_20CE6 == 3) {
		grcg_setcolor(GC_RMW, V_WHITE);
		_AX = 0xA800;
		_ES = _AX;
		word_20CE4 = byte_207E4;
		i = 0;
		goto particle_test;

particle_loop:
		if(word_20CE4->active != 0) {
			asm {
				mov 	bx, word_20CE4;
				push	word ptr [bx];
				push	word ptr [bx+2];
				call	near ptr sub_BE2A;
			}
		}
		i++;
		word_20CE4++;

particle_test:
		if(i < 0x80) {
			goto particle_loop;
		}

grcg_done:
		grcg_off();
	}
}
