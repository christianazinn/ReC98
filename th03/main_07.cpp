#pragma option -zCmain_07_TEXT -zPmain_07

#include "platform.h"
#include "th02/snd/snd.h"
#include "th01/math/subpixel.hpp"
#include "th03/main/player/cur.hpp"
#include "th03/main/player/shot.hpp"

extern "C" uint8_t chiyuri_chargeshot_nodes[];

struct player_stuff_t {
	Subpixel center_x;
	Subpixel center_y;
	uint8_t unused_0[0x14];
	uint16_t gauge_charged;
	uint8_t unused_1[8];
	uint8_t shot_active;
	uint8_t unused_2[0x5D];
};

extern player_stuff_t players[PLAYER_COUNT];

extern "C" void far sub_1B260(void)
{
	register uint8_t near *node;

	node = chiyuri_chargeshot_nodes;
	_AX = 0;
	goto node_loop_test;

node_loop:
	node[0] = 0;
	_AX++;

node_loop_test:
	if(static_cast<int>(_AX) < 16) {
		goto node_loop;
	}
}

extern "C" void pascal far chargeshot_add_chiyuri(
	Subpixel center_x, Subpixel center_y
)
{
	register uint8_t near *node;

	node = (chiyuri_chargeshot_nodes + (pid.current * 0x30));
	node[1] = 0;
	*reinterpret_cast<Subpixel near *>(node + 2) = center_x;
	*reinterpret_cast<Subpixel near *>(node + 4) = center_y;
	players[pid.current].shot_active = SA_DISABLED;
	_DX = 0;
	goto node_loop_test;

node_loop:
	_AL = _DL;
	_AL <<= 2;
	_AL += 2;
	node[0] = _AL;
	_DX++;
	node += 6;

node_loop_test:
	if(static_cast<int>(_DX) < 8) {
		goto node_loop;
	}
}

extern "C" void pascal far chargeshot_update_chiyuri(void)
{
	register uint8_t near *node;
	register int i;

	node = (chiyuri_chargeshot_nodes + (pid_current * 0x30) + (7 * 6));
	if(node[0] == 0) {
		goto ret;
	}
	players[pid_current].gauge_charged = 0;
	i = 0;
	goto node_loop_test;

node_loop:
	if(node[0] == 0) {
		goto ret;
	}
	if(node[0] == 1) {
		goto active;
	}
	node[0]--;
	if(node[0] != 1) {
		goto state_done;
	}
	*reinterpret_cast<Subpixel near *>(node + 2) = (
		players[pid_current].center_x
	);
	*reinterpret_cast<Subpixel near *>(node + 4) = (
		players[pid_current].center_y
	);
	snd_se_play(15);
	goto state_done;

active:
	reinterpret_cast<Subpixel near *>(node + 4)[0].v -= (14 << 4);
	if(reinterpret_cast<Subpixel near *>(node + 4)[0].v >= -0x300) {
		goto state_done;
	}
	if(i != 0) {
		goto clear;
	}
	players[pid_current].shot_active = SA_ENABLED;

clear:
	node[0] = 0;
	goto next;

state_done:
	node[1]++;

next:
	i++;
	node -= 6;

node_loop_test:
	if(i < 8) {
		goto node_loop;
	}

ret:
}
