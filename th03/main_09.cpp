#pragma option -zCmain_09_TEXT -zPmain_09

#include "platform.h"
#include "th01/math/subpixel.hpp"
#include "th03/main/player/cur.hpp"
#include "th03/math/vector.hpp"

extern "C" uint8_t kana_chargeshot_frames[];
extern "C" uint8_t kana_chargeshot_nodes[];
extern "C" uint8_t kana_chargeshot_state[];

struct player_stuff_t {
	uint8_t unused_0[0x18];
	uint16_t gauge_charged;
	uint8_t unused_1[0x66];
};

extern player_stuff_t players[PLAYER_COUNT];

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

extern "C" void pascal far chargeshot_update_kana(void)
{
	subpixel_t vector_x;
	subpixel_t vector_y;
	int point_i;
	uint8_t state;
	register uint8_t near *node;
	register int group_i;

	if(kana_chargeshot_state[pid_current] == 0) {
		goto ret;
	}
	players[pid_current].gauge_charged = 0;
	state = kana_chargeshot_state[pid_current];
	node = (kana_chargeshot_nodes + (pid_current * (4 * 54)));
	group_i = 0;
	goto group_loop_test;

group_loop:
	vector2(
		vector_x,
		vector_y,
		node[0x34],
		node[0x35]
	);
	point_i = 0x0C;
	goto shift_loop_test;

shift_loop:
	reinterpret_cast<subpixel_t near *>(node)[point_i] = (
		reinterpret_cast<subpixel_t near *>(node)[point_i - 1]
	);
	reinterpret_cast<subpixel_t near *>(node + 0x1A)[point_i] = (
		reinterpret_cast<subpixel_t near *>(node + 0x1A)[point_i - 1]
	);
	point_i--;

shift_loop_test:
	if(point_i > 0) {
		goto shift_loop;
	}
	reinterpret_cast<subpixel_t near *>(node)[0] += vector_x;
	reinterpret_cast<subpixel_t near *>(node + 0x1A)[0] += vector_y;
	if(state == 1) {
		node[0x35]--;
	}
	group_i++;
	node += 0x36;

group_loop_test:
	if(group_i < 4) {
		goto group_loop;
	}
	if(state != 1) {
		goto clear_test;
	}
	node -= 0x36;
	node[0x34] += -2;
	node -= (3 * 54);
	node[0x34] += 2;
	if(kana_chargeshot_frames[pid_current] <= 0x28) {
		goto clear_test;
	}
	kana_chargeshot_state[pid_current] = 2;
	group_i = 0;
	goto length_loop_test;

length_loop:
	node[0x35] = 0x80;
	group_i++;
	node += 0x36;

length_loop_test:
	if(group_i < 4) {
		goto length_loop;
	}

clear_test:
	node = (kana_chargeshot_nodes + (pid_current * (4 * 54)));
	if(*reinterpret_cast<int near *>(node + 0x32) <= -0x100) {
		kana_chargeshot_state[pid_current] = 0;
	}
	kana_chargeshot_frames[pid_current]++;

ret:
}
