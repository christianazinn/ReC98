#pragma option -zCmain_08_TEXT -zPmain_08

#include "libs/master.lib/master.hpp"
#include "platform.h"
#include "th02/snd/snd.h"
#include "th01/math/subpixel.hpp"
#include "th03/main/player/cur.hpp"
#include "th03/main/playfld.hpp"
#include "th03/math/vector.hpp"

extern "C" uint8_t ellen_chargeshot_nodes[];
extern "C" uint16_t word_1F868;
extern "C" subpixel_t word_2142E;
extern "C" subpixel_t word_21430;

extern "C" uint16_t far randring_far_next16_raw(void);
extern "C" void pascal far sub_16983(uint8_t pid);

struct player_stuff_t {
	Subpixel center_x;
	Subpixel center_y;
	uint8_t unused_0[0x14];
	uint16_t gauge_charged;
	uint8_t unused_1[0x66];
};

extern player_stuff_t players[PLAYER_COUNT];

extern "C" void far sub_1B653(void)
{
	word_1F868 = reinterpret_cast<uint16_t>(ellen_chargeshot_nodes);
	_AX = 0;
	goto node_loop_test;

node_loop:
	_BX = word_1F868;
	reinterpret_cast<uint8_t near *>(_BX)[0] = 0;
	_AX++;
	word_1F868 += 0x0C;

node_loop_test:
	if(static_cast<int>(_AX) < 0x40) {
		goto node_loop;
	}
}

extern "C" void pascal far chargeshot_add_ellen(
	Subpixel center_x, Subpixel center_y
)
{
	int i;

	word_1F868 = reinterpret_cast<uint16_t>(
		ellen_chargeshot_nodes + (pid.current * (32 * 12))
	);
	i = 0;
	goto node_loop_test;

node_loop:
	_BX = word_1F868;
	_AL = reinterpret_cast<uint8_t near *>(&i)[0];
	reinterpret_cast<uint8_t near *>(_BX)[0] = _AL;
	reinterpret_cast<uint8_t near *>(_BX)[1] = 0;
	*reinterpret_cast<Subpixel near *>(_BX + 4) = center_x;
	*reinterpret_cast<Subpixel near *>(_BX + 6) = center_y;
	_AX = randring_far_next16_raw();
	_BX = word_1F868;
	reinterpret_cast<uint8_t near *>(_BX)[2] = _AL;
	reinterpret_cast<uint8_t near *>(_BX)[3] = 0x50;
	i += 2;
	word_1F868 += 0x0C;

node_loop_test:
	if(i < 0x40) {
		goto node_loop;
	}
}

extern "C" void pascal far ellen_hyper_1B6CA(
	Subpixel center_x, Subpixel center_y
)
{
	register int i;

	word_1F868 = reinterpret_cast<uint16_t>(
		ellen_chargeshot_nodes + (pid.current * (32 * 12))
	);
	i = 0;
	goto node_loop_test;

node_loop:
	_BX = word_1F868;
	if(reinterpret_cast<uint8_t near *>(_BX)[0] != 0) {
		goto next;
	}
	_BX = word_1F868;
	reinterpret_cast<uint8_t near *>(_BX)[0] = 1;
	reinterpret_cast<uint8_t near *>(_BX)[1] = 0;
	*reinterpret_cast<Subpixel near *>(_BX + 4) = center_x;
	*reinterpret_cast<Subpixel near *>(_BX + 6) = center_y;
	_AX = randring_far_next16_raw();
	_BX = word_1F868;
	reinterpret_cast<uint8_t near *>(_BX)[2] = _AL;
	reinterpret_cast<uint8_t near *>(_BX)[3] = 0x50;
	goto ret;

next:
	i++;
	word_1F868 += 0x0C;

node_loop_test:
	if(i < 0x20) {
		goto node_loop;
	}

ret:
}

extern "C" void pascal far chargeshot_update_ellen(void)
{
	subpixel_t target_x;
	subpixel_t target_y;
	subpixel_t player_x;
	subpixel_t player_y;
	uint8_t length;
	register int i;

	word_1F868 = reinterpret_cast<uint16_t>(
		ellen_chargeshot_nodes + (pid_current * (32 * 12))
	);
	i = 0;
	goto active_search_test;

active_search:
	_BX = word_1F868;
	if(reinterpret_cast<uint8_t near *>(_BX)[0] != 0) {
		goto active_found;
	}
	i++;
	word_1F868 += 0x0C;

active_search_test:
	__emit__(0x83, 0xFE, 0x20); // CMP SI, 20h
	asm { jl active_search; }
	goto ret;

active_found:
	word_1F868 = reinterpret_cast<uint16_t>(
		ellen_chargeshot_nodes + (pid_current * (32 * 12))
	);
	sub_16983(pid_current);
	target_x = word_2142E;
	target_y = (word_21430 + 0xF920);
	players[pid_current].gauge_charged = 0;
	player_x = players[pid_current].center_x;
	player_y = players[pid_current].center_y;
	playfield_clip_negative_radius.x.v = TO_SP(-8);
	playfield_clip_negative_radius.y.v = TO_SP(-8);
	i = 0;
	goto update_loop_test;

update_loop:
	_BX = word_1F868;
	if(reinterpret_cast<uint8_t near *>(_BX)[0] == 0) {
		goto next;
	}
	_BX = word_1F868;
	if(reinterpret_cast<uint8_t near *>(_BX)[0] != 1) {
		goto not_state_1;
	}
	reinterpret_cast<Subpixel near *>(_BX + 4)[0].v += (
		reinterpret_cast<Subpixel near *>(_BX + 8)[0].v
	);
	reinterpret_cast<Subpixel near *>(_BX + 6)[0].v += (
		reinterpret_cast<Subpixel near *>(_BX + 10)[0].v
	);
	if(reinterpret_cast<uint8_t near *>(_BX)[1] != 0) {
		goto clip;
	}
	_AL = reinterpret_cast<uint8_t near *>(_BX)[3];
	_AL += -2;
	length = _AL;
	vector2(
		reinterpret_cast<Subpixel near *>(word_1F868 + 8)[0].v,
		reinterpret_cast<Subpixel near *>(word_1F868 + 10)[0].v,
		reinterpret_cast<uint8_t near *>(_BX)[2],
		length
	);
	_BX = word_1F868;
	_AL = length;
	reinterpret_cast<uint8_t near *>(_BX)[3] = _AL;
	if(length != 0) {
		goto next;
	}
	reinterpret_cast<uint8_t near *>(_BX)[1] = 1;
	_AL = iatan2(
		(target_y - reinterpret_cast<Subpixel near *>(_BX + 6)[0].v),
		(target_x - reinterpret_cast<Subpixel near *>(_BX + 4)[0].v)
	);
	_BX = word_1F868;
	reinterpret_cast<uint8_t near *>(_BX)[2] = _AL;
	vector2(
		reinterpret_cast<Subpixel near *>(word_1F868 + 8)[0].v,
		reinterpret_cast<Subpixel near *>(word_1F868 + 10)[0].v,
		reinterpret_cast<uint8_t near *>(_BX)[2],
		160
	);
	goto next;

clip:
	_BX = word_1F868;
	if(playfield_clip(
		reinterpret_cast<Subpixel near *>(_BX + 4)[0],
		reinterpret_cast<Subpixel near *>(_BX + 6)[0]
	)) {
		_BX = word_1F868;
		reinterpret_cast<uint8_t near *>(_BX)[0] = 0;
	}
	goto next;

not_state_1:
	_BX = word_1F868;
	_AL = reinterpret_cast<uint8_t near *>(_BX)[0];
	_AL += -1;
	reinterpret_cast<uint8_t near *>(_BX)[0] = _AL;
	if(_AL == 1) {
		snd_se_play(1);
	}
	_BX = word_1F868;
	reinterpret_cast<Subpixel near *>(_BX + 4)[0].v = player_x;
	reinterpret_cast<Subpixel near *>(_BX + 6)[0].v = player_y;

next:
	i++;
	word_1F868 += 0x0C;

update_loop_test:
	if(i < 0x20) {
		goto update_loop;
	}

ret:
}
