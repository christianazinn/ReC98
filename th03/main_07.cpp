#pragma option -zCmain_07_TEXT -zPmain_07

#include "platform.h"
#include "th01/math/subpixel.hpp"
#include "th03/main/player/cur.hpp"
#include "th03/main/player/shot.hpp"

extern "C" uint8_t chiyuri_chargeshot_nodes[];

struct player_stuff_t {
	uint8_t unused_0[0x22];
	uint8_t shot_active;
	uint8_t unused_1[0x5D];
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
