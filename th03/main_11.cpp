#pragma option -zCmain_11_TEXT -zPmain_11

#include "libs/master.lib/master.hpp"
#include "platform.h"
#include "th01/math/subpixel.hpp"
#include "th03/main/bullet/bullet.hpp"
#include "th03/main/hitbox.hpp"
#include "th03/main/hitcirc.hpp"
#include "th03/main/player/cur.hpp"
#include "th03/main/player/gba.hpp"
#include "th03/main/playfld.hpp"
#include "th03/main/sprite16.hpp"
#include "th03/math/polar.hpp"

extern "C" uint8_t byte_202B8[];
extern "C" uint8_t byte_202B9[];
extern "C" uint8_t pid_PID_current;
extern "C" uint8_t pid_PID_so_attack;
extern "C" uint8_t rikako_chargeshot_frames[];
extern "C" uint8_t rikako_chargeshot_nodes[];
extern "C" Subpixel rikako_chargeshot_origin_x[];
extern "C" Subpixel rikako_chargeshot_origin_y[];
extern "C" uint16_t rikako_chargeshot_radius[];
extern "C" uint8_t rikako_chargeshot_state[];
extern "C" uint8_t rikako_gauge_pattern_frames[];
extern "C" uint16_t word_20E86;

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

uint8_t far chargeshot_hittest_rikako(void)
{
	int hits;
	register uint8_t near *node;
	register int i;

	if(rikako_chargeshot_state[hitbox.pid] == 0) {
		return 0;
	}
	hits = 0;
	node = (rikako_chargeshot_nodes + (hitbox.pid * (4 * 6)));
	i = 0;
	goto node_loop_test;

node_loop:
	if(
		(
			*reinterpret_cast<subpixel_t near *>(node) -
			hitbox.right.v
		) > TO_SP(12)
	) {
		goto next;
	}
	if(
		(
			hitbox.origin.topleft.x.v -
			*reinterpret_cast<subpixel_t near *>(node)
		) > TO_SP(12)
	) {
		goto next;
	}
	if(
		(
			*reinterpret_cast<subpixel_t near *>(node + 2) -
			hitbox.bottom.v
		) > TO_SP(12)
	) {
		goto next;
	}
	if(
		(
			hitbox.origin.topleft.y.v -
			*reinterpret_cast<subpixel_t near *>(node + 2)
		) > TO_SP(12)
	) {
		goto next;
	}
	hitcircles_enemy_add(
		*reinterpret_cast<subpixel_t near *>(node),
		*reinterpret_cast<subpixel_t near *>(node + 2),
		hitbox.pid
	);
	hits++;

next:
	i++;
	node += 6;

node_loop_test:
	if(i < 4) {
		goto node_loop;
	}
	return hits;
}

extern "C" void pascal far chargeshot_render_rikako(void)
{
	register int i;

	if(rikako_chargeshot_state[pid_current] == 0) {
		goto ret;
	}
	word_20E86 = reinterpret_cast<uint16_t>(
		rikako_chargeshot_nodes + (pid_current * (4 * 6))
	);
	sprite16_put_size.w.v = (32 / 16);
	sprite16_put_size.h = 16;
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
	rikako_chargeshot_1C62A();
	i++;
	word_20E86 += 6;

node_loop_test:
	if(i < 4) {
		goto node_loop;
	}

ret:
}

void pascal near gauge_pattern_rikako(uint8_t type)
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
		rikako_gauge_pattern_frames[pid_current] = 0;
		gba_flag_active[pid_current]++;
		byte_202B8[pid_current * 4] = (
			0x10 - (static_cast<int>(gba_gauge_level[pid_current]) / 2)
		);
		byte_202B9[pid_current * 4] = (gba_gauge_level[pid_current] + 0x30);
		return;
	}

	if(gba_flag_active[pid_current] != (flag_expected + 1)) {
		return;
	}

	if((rikako_gauge_pattern_frames[pid_current] % byte_202B8[pid_current * 4]) != 0) {
		goto clear_test;
	}

	pid_other = (1 - pid_current);
	bullet_template.type = static_cast<bullet_type_t>(type);
	bullet_template.pid = pid_other;
	bullet_template.center.y.v = 0;
	bullet_template.group = BG_1_AIMED;
	bullet_template.angle = (
		0x20 - (rikako_gauge_pattern_frames[pid_current] / 4)
	);
	bullet_template.speed.v = byte_202B9[pid_current * 4];
	bullet_template.center.x.v = 0;
	bullets_add();
	bullet_template.center.x.v = (PLAYFIELD_W << 4);
	bullets_add();
	bullet_template.angle = -bullet_template.angle;
	bullets_add();
	bullet_template.center.x.v = 0;
	bullets_add();
	SUB_CE0C(0, 0, static_cast<uint16_t>(pid_other));
	SUB_CE0C((PLAYFIELD_W << 4), 0, static_cast<uint16_t>(pid_other));

clear_test:
	if(rikako_gauge_pattern_frames[pid_current] >= 0x80) {
		gba_flag_active[pid_current] = GBAF_NONE;
		sub_A3A8(1 - pid_current);
	}
	rikako_gauge_pattern_frames[pid_current]++;
}

extern "C" void pascal far gba_gauge_pattern_pellet_rikako(void)
{
	if(gba_flag_active[pid_current] != GBAF_NONE) {
		gauge_pattern_rikako(BT_PELLET);
	}
}

extern "C" void pascal far gba_gauge_pattern_bullet_rikako(void)
{
	if(gba_flag_active[pid_current] != GBAF_NONE) {
		gauge_pattern_rikako(BT_BULLET16_DEFAULT);
	}
}
