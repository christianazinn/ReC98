#pragma option -zCmain_09_TEXT -zPmain_09

#include "platform.h"
#include "th01/math/subpixel.hpp"
#include "th03/main/player/cur.hpp"

extern "C" uint8_t kana_chargeshot_frames[];
extern "C" uint8_t kana_chargeshot_nodes[];
extern "C" uint8_t kana_chargeshot_state[];

extern "C" void far sub_1BC4D(void)
{
	kana_chargeshot_state[0] = 0;
	kana_chargeshot_state[1] = 0;
}

extern "C" void pascal far chargeshot_add_kana(
	Subpixel center_x, Subpixel center_y
)
{
	register uint8_t near *node;
	int group_i;
	int point_i;

	kana_chargeshot_state[pid.current] = 1;
	kana_chargeshot_frames[pid.current] = 0;
	node = (kana_chargeshot_nodes + (pid.current * (4 * 54)));
	group_i = 0;
	goto group_loop_test;

group_loop:
	point_i = 0;
	goto point_loop_test;

point_loop:
	reinterpret_cast<Subpixel near *>(node)[point_i] = center_x;
	reinterpret_cast<Subpixel near *>(node + 0x1A)[point_i] = center_y;
	point_i++;

point_loop_test:
	if(point_i <= 0x0C) {
		goto point_loop;
	}
	node[0x35] = 0x30;
	group_i++;
	node += 0x36;

group_loop_test:
	if(group_i < 4) {
		goto group_loop;
	}
	node -= 0x36;
	node[0x34] = 0xF0;
	node -= 0x36;
	node[0x34] = 0xC8;
	node -= 0x36;
	node[0x34] = 0xB8;
	node -= 0x36;
	node[0x34] = 0x90;
}
