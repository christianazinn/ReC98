#pragma option -zCmain_08_TEXT -zPmain_08

#include "libs/master.lib/master.hpp"
#include "platform.h"
#include "th02/snd/snd.h"
#include "th01/math/subpixel.hpp"
#include "th03/main/bullet/bullet.hpp"
#include "th03/main/hitbox.hpp"
#include "th03/main/hitcirc.hpp"
#include "th03/main/player/cur.hpp"
#include "th03/main/player/gba.hpp"
#include "th03/main/playfld.hpp"
#include "th03/main/sprite16.hpp"
#include "th03/math/randring.hpp"
#include "th03/math/vector.hpp"

extern "C" uint8_t byte_202B8[];
extern "C" uint8_t byte_202B9[];
extern "C" uint8_t ellen_chargeshot_nodes[];
extern "C" subpixel_t ellen_gauge_pattern_x[];
extern "C" subpixel_t ellen_gauge_pattern_y[];
extern "C" uint8_t ellen_gauge_pattern_frames[];
extern "C" uint8_t pid_PID_so_attack;
extern "C" uint16_t word_1F868;
extern "C" subpixel_t word_2142E;
extern "C" subpixel_t word_21430;

extern "C" uint16_t far randring_far_next16_raw(void);
extern "C" void pascal far sub_16983(uint8_t pid);
extern "C" void pascal far sub_A3A8(uint8_t pid);
extern "C" void pascal far SUB_CE0C(subpixel_t x, subpixel_t y, uint16_t pid);

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

extern "C" void pascal near ellen_chargeshot_1B8A6(Subpixel x, Subpixel y)
{
	sprite16_offset_t sprite_offset;
	uint8_t phase;
	register subpixel_t x_reg;
	register subpixel_t y_reg;

	x_reg = x.v;
	y_reg = y.v;
	sprite_offset = (pid_PID_so_attack + 0x10);
	_BX = word_1F868;
	_AL = reinterpret_cast<uint8_t near *>(_BX)[2];
	_AL += 0x10;
	phase = _AL;
	phase >>= 5;
	phase <<= 1;
	sprite_offset += phase;
	x_reg = (playfield_fg_x_to_screen(x_reg, pid_current) - 8);
	_AX = y_reg;
	asm { sar ax, 4; }
	_AX += 8;
	y_reg = _AX;
	sprite16_put(x_reg, _AX, sprite_offset);
}

uint8_t far chargeshot_hittest_ellen(void)
{
	uint8_t ret;
	register int i;

	ret = 0;
	word_1F868 = reinterpret_cast<uint16_t>(
		ellen_chargeshot_nodes + (hitbox.pid * (32 * 12))
	);
	i = 0;
	goto node_loop_test;

node_loop:
	_BX = word_1F868;
	if(reinterpret_cast<uint8_t near *>(_BX)[0] != 1) {
		goto next;
	}
	_BX = word_1F868;
	_AX = reinterpret_cast<Subpixel near *>(_BX + 4)[0].v;
	if(static_cast<int>(_AX) < hitbox.origin.topleft.x.v) {
		goto next;
	}
	if(static_cast<int>(_AX) > hitbox.right.v) {
		goto next;
	}
	_AX = reinterpret_cast<Subpixel near *>(_BX + 6)[0].v;
	if(static_cast<int>(_AX) < hitbox.origin.topleft.y.v) {
		goto next;
	}
	if(static_cast<int>(_AX) > hitbox.bottom.v) {
		goto next;
	}
	hitcircles_enemy_add(
		reinterpret_cast<Subpixel near *>(_BX + 4)[0].v,
		_AX,
		hitbox.pid
	);
	_BX = word_1F868;
	reinterpret_cast<uint8_t near *>(_BX)[0] = 0;
	_AL = ret;
	_AL += 2;
	ret = _AL;

next:
	i++;
	word_1F868 += 0x0C;

node_loop_test:
	if(i < 0x20) {
		goto node_loop;
	}
	return ret;
}

extern "C" void pascal far chargeshot_render_ellen(void)
{
	register int i;

	word_1F868 = reinterpret_cast<uint16_t>(
		ellen_chargeshot_nodes + (pid_current * (32 * 12))
	);
	sprite16_put_size.w.v = (16 / 16);
	sprite16_put_size.h = 8;
	if(pid_current == 0) {
		sprite16_clip.left = PLAYFIELD1_CLIP_LEFT;
		sprite16_clip.right = PLAYFIELD1_CLIP_RIGHT;
	} else {
		sprite16_clip.left = PLAYFIELD2_CLIP_LEFT;
		sprite16_clip.right = PLAYFIELD2_CLIP_RIGHT;
	}

	i = 0;
	goto node_loop_test;

node_loop:
	_BX = word_1F868;
	if(reinterpret_cast<uint8_t near *>(_BX)[0] != 1) {
		goto next;
	}
	_BX = word_1F868;
	ellen_chargeshot_1B8A6(
		reinterpret_cast<Subpixel near *>(_BX + 4)[0],
		reinterpret_cast<Subpixel near *>(_BX + 6)[0]
	);

next:
	i++;
	word_1F868 += 0x0C;

node_loop_test:
	if(i < 0x20) {
		goto node_loop;
	}
}

void pascal near gauge_pattern_ellen(uint8_t type)
{
	uint8_t pid_other;
	uint8_t flag_expected;

	flag_expected = GBAF_GAUGE_PELLET_INIT;
	if(type == BT_BULLET16_DEFAULT) {
		_AL = flag_expected;
		_AL += GBAF_PELLET_TO_BULLET;
		flag_expected = _AL;
	}

	if(gba_flag_active[pid_current] == flag_expected) {
		ellen_gauge_pattern_frames[pid_current] = 0;
		gba_flag_active[pid_current]++;
		ellen_gauge_pattern_x[pid_current] = TO_SP(144);
		ellen_gauge_pattern_y[pid_current] = 0;
		byte_202B8[pid_current * 4] = (gba_gauge_level[pid_current] + 0x0E);
		byte_202B9[pid_current * 4] = (gba_gauge_level[pid_current] + 0x1C);
		return;
	}

	if(gba_flag_active[pid_current] != (flag_expected + 1)) {
		return;
	}

	if((ellen_gauge_pattern_frames[pid_current] % 8) != 0) {
		goto frame_next;
	}

	bullet_template.type = static_cast<bullet_type_t>(type);
	bullet_template.center.y.v = TO_SP(8);
	pid_other = (1 - pid_current);
	bullet_template.pid = pid_other;

	if(ellen_gauge_pattern_x[pid_current] > 0) {
		bullet_template.angle = 0;
		bullet_template.speed.v = ((3 << 4) + 8);
		bullet_template.group = BG_RING_AIMED;
		bullet_template.count = byte_202B8[pid_current * 4];
		bullet_template.center.y.v = 0;
		bullet_template.center.x.v = ellen_gauge_pattern_x[pid_current];
		SUB_CE0C(
			bullet_template.center.x.v,
			0,
			static_cast<uint16_t>(pid_other)
		);
		bullets_add();
		bullet_template.center.x.v = (
			TO_SP(PLAYFIELD_W) - ellen_gauge_pattern_x[pid_current]
		);
		SUB_CE0C(
			bullet_template.center.x.v,
			0,
			static_cast<uint16_t>(pid_other)
		);
		bullets_add();
		ellen_gauge_pattern_x[pid_current] -= TO_SP(24);
		goto frame_next;
	}

	if(ellen_gauge_pattern_y[pid_current] < TO_SP(144)) {
		bullet_template.angle = randring_far_next16_raw();
		bullet_template.speed.v = byte_202B9[pid_current * 4];
		bullet_template.group = BG_16_RING;
		bullet_template.center.y.v = ellen_gauge_pattern_y[pid_current];
		bullet_template.center.x.v = 0;
		SUB_CE0C(
			0,
			ellen_gauge_pattern_y[pid_current],
			static_cast<uint16_t>(pid_other)
		);
		bullets_add();
		bullet_template.center.x.v = TO_SP(PLAYFIELD_W);
		SUB_CE0C(
			TO_SP(PLAYFIELD_W),
			ellen_gauge_pattern_y[pid_current],
			static_cast<uint16_t>(pid_other)
		);
		bullets_add();
		bullet_template.angle = randring_far_next16_raw();
		ellen_gauge_pattern_y[pid_current] += TO_SP(24);
		goto frame_next;
	}

	gba_flag_active[pid_current] = GBAF_NONE;
	sub_A3A8(pid_other);

frame_next:
	ellen_gauge_pattern_frames[pid_current]++;
}
