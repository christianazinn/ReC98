#pragma option -zCmain_11_TEXT -zPmain_11

#include "libs/master.lib/master.hpp"
#include "platform.h"
#include "th01/math/subpixel.hpp"
#include "th03/main/player/cur.hpp"
#include "th03/main/playfld.hpp"
#include "th03/main/sprite16.hpp"
#include "th03/math/polar.hpp"

extern "C" uint8_t pid_PID_current;
extern "C" uint8_t pid_PID_so_attack;
extern "C" uint8_t rikako_chargeshot_frames[];
extern "C" uint8_t rikako_chargeshot_nodes[];
extern "C" Subpixel rikako_chargeshot_origin_x[];
extern "C" Subpixel rikako_chargeshot_origin_y[];
extern "C" uint16_t rikako_chargeshot_radius[];
extern "C" uint8_t rikako_chargeshot_state[];
extern "C" uint16_t word_20E86;

struct player_stuff_t {
	Subpixel center_x;
	Subpixel center_y;
	uint8_t unused_0[0x14];
	uint16_t gauge_charged;
	uint8_t unused_1[0x66];
};

extern player_stuff_t players[PLAYER_COUNT];

extern "C" void far sub_1C40A(void)
{
	rikako_chargeshot_state[0] = 0;
	rikako_chargeshot_state[1] = 0;
}

extern "C" void pascal far chargeshot_add_rikako(
	Subpixel center_x, Subpixel center_y
)
{
	_DI = center_x.v;
	rikako_chargeshot_state[pid_PID_current] = 1;
	rikako_chargeshot_frames[pid_PID_current] = 0;
	rikako_chargeshot_radius[pid_PID_current] = 0x80;
	_SI = reinterpret_cast<uint16_t>(
		rikako_chargeshot_nodes + (pid_PID_current * (4 * 6))
	);
	_CX = 0;
	goto node_loop_check;

node_loop:
	*reinterpret_cast<subpixel_t near *>(_SI) = _DI;
	*reinterpret_cast<Subpixel near *>(_SI + 2) = center_y;
	_AL = _CL;
	_AL <<= 6;
	_AL += 0x20;
	reinterpret_cast<uint8_t near *>(_SI)[4] = _AL;
	_CX++;
	_SI += 6;

node_loop_check:
	asm { cmp cx, 4; }
	asm { jl node_loop; }

	rikako_chargeshot_origin_x[pid_PID_current].v = _DI;
	rikako_chargeshot_origin_y[pid_PID_current] = center_y;
}

extern "C" void pascal far rikako_1C497(
	Subpixel center_x, Subpixel center_y
)
{
	chargeshot_add_rikako(center_x, center_y);
	rikako_chargeshot_state[pid_PID_current] = 2;
}

extern "C" void pascal far rikako_hyper_1C4B4(void)
{
	rikako_chargeshot_state[pid_PID_current] = 0;
}

extern "C" void pascal far chargeshot_update_rikako(void)
{
	int i;
	player_stuff_t near *player;
	uint8_t frame;
	register uint8_t near *node;
	register uint16_t radius;

	if(rikako_chargeshot_state[pid_current] == 0) {
		goto ret;
	}
	player = &players[pid_current];
	player->gauge_charged = 0;
	frame = rikako_chargeshot_frames[pid_current];
	radius = rikako_chargeshot_radius[pid_current];
	node = (rikako_chargeshot_nodes + (pid_current * (4 * 6)));
	if(rikako_chargeshot_state[pid_current] == 1) {
		rikako_chargeshot_origin_y[pid_current].v -= 0x20;
		goto node_loop_setup;
	}
	rikako_chargeshot_origin_x[pid_current] = player->center_x;
	rikako_chargeshot_origin_y[pid_current] = player->center_y;

node_loop_setup:
	i = 0;
	goto node_loop_test;

node_loop:
	*reinterpret_cast<subpixel_t near *>(node) = polar(
		rikako_chargeshot_origin_x[pid_current].v,
		radius,
		CosTable8[node[4]]
	);
	*reinterpret_cast<subpixel_t near *>(node + 2) = polar(
		rikako_chargeshot_origin_y[pid_current].v,
		radius,
		SinTable8[node[4]]
	);
	if((i & 1) != 0) {
		_AL = 8;
	} else {
		_AL = -8;
	}
	_AL += node[4];
	node[4] = _AL;
	i++;
	node += 6;

node_loop_test:
	if(i < 4) {
		goto node_loop;
	}
	if(frame < 0x20) {
		radius += 0x20;
		goto store;
	}
	if(rikako_chargeshot_state[pid_current] == 1) {
		if(frame <= 0x80) {
			goto store;
		}
		radius += 0x60;
		if(frame <= 144) {
			goto store;
		}
		rikako_chargeshot_state[pid_current] = 0;
		goto store;
	}
	rikako_chargeshot_frames[pid_current] = 0x20;

store:
	rikako_chargeshot_radius[pid_current] = radius;
	rikako_chargeshot_frames[pid_current]++;

ret:
}

extern "C" void near rikako_chargeshot_1C62A(void)
{
	screen_x_t left;
	screen_y_t top;
	sprite16_offset_t sprite_offset;

	sprite_offset = (pid_PID_so_attack + (8 * ROW_SIZE));
	left = (playfield_fg_x_to_screen(
		*reinterpret_cast<subpixel_t near *>(word_20E86),
		pid_current
	) - 16);
	_BX = word_20E86;
	_AX = *reinterpret_cast<subpixel_t near *>(_BX + 2);
	asm { sar ax, 4; }
	top = _AX;
	sprite16_put(left, _AX, sprite_offset);
}
