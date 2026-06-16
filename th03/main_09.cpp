#pragma option -zCmain_09_TEXT -zPmain_09

#include "libs/sprite16/sprite16.h"
#include "platform.h"
#include "th01/math/subpixel.hpp"
#include "th03/main/bullet/bullet.hpp"
#include "th03/main/hitbox.hpp"
#include "th03/main/hitcirc.hpp"
#include "th03/main/playfld.hpp"
#include "th03/main/player/cur.hpp"
#include "th03/main/player/gba.hpp"
#include "th03/main/sprite16.hpp"
#include "th03/math/vector.hpp"

extern "C" uint8_t byte_202B8[];
extern "C" uint8_t byte_202B9[];
extern "C" uint8_t kana_chargeshot_frames[];
extern "C" uint8_t kana_chargeshot_nodes[];
extern "C" uint8_t kana_chargeshot_state[];
extern "C" uint8_t kana_gauge_pattern_frames[];
extern "C" subpixel_t kana_gauge_pattern_x[];
extern "C" uint8_t pid_PID_so_attack;
extern "C" uint16_t word_1FD8C;

extern "C" void pascal far sub_A3A8(uint8_t pid);
extern "C" void pascal far SUB_CE0C(subpixel_t x, subpixel_t y, uint16_t pid);

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

#pragma warn -aus
extern "C" void near kana_chargeshot_1BDF8(void)
{
	screen_x_t left;
	screen_y_t top;
	register int i;
	register sprite16_offset_t so;

	so = (pid_PID_so_attack + (56 * ROW_SIZE));
	i = 0x0C;
	goto point_loop_test;

point_loop:
	_BX = i;
	_BX += _BX;
	_BX += word_1FD8C;
	left = (playfield_fg_x_to_screen(
		*reinterpret_cast<subpixel_t near *>(_BX),
		pid_current
	) - 16);
	_BX = i;
	_BX += _BX;
	_BX += word_1FD8C;
	_AX = *reinterpret_cast<subpixel_t near *>(_BX + 0x1A);
	asm { sar ax, 4; }
	top = _AX;
	sprite16_put(left, _AX, so);
	i -= 4;
	so -= (16 * ROW_SIZE);

point_loop_test:
	if(i >= 0) {
		goto point_loop;
	}
}

uint8_t far chargeshot_hittest_kana(void)
{
	int group_i;
	register int point_i;
	register uint8_t near *node;

	if(kana_chargeshot_state[hitbox.pid] == 0) {
		goto not_hit;
	}
	node = (kana_chargeshot_nodes + (hitbox.pid * (4 * 54)));
	group_i = 0;
	goto group_loop_test;

group_loop:
	point_i = 0;
	goto point_loop_test;

point_loop:
	if(
		(
			reinterpret_cast<subpixel_t near *>(node)[point_i] -
			hitbox.right.v
		) > TO_SP(12)
	) {
		goto next;
	}
	if(
		(
			hitbox.origin.topleft.x.v -
			reinterpret_cast<subpixel_t near *>(node)[point_i]
		) > TO_SP(12)
	) {
		goto next;
	}
	if(
		(
			reinterpret_cast<subpixel_t near *>(node + 0x1A)[point_i] -
			hitbox.bottom.v
		) > TO_SP(12)
	) {
		goto next;
	}
	if(
		(
			hitbox.origin.topleft.y.v -
			reinterpret_cast<subpixel_t near *>(node + 0x1A)[point_i]
		) > TO_SP(12)
	) {
		goto next;
	}
	hitcircles_enemy_add(
		reinterpret_cast<subpixel_t near *>(node)[point_i],
		reinterpret_cast<subpixel_t near *>(node + 0x1A)[point_i],
		hitbox.pid
	);
	_AL = 1;
	goto ret;

next:
	point_i += 4;

point_loop_test:
	if(point_i <= 0x0C) {
		goto point_loop;
	}
	group_i++;
	node += 0x36;

group_loop_test:
	if(group_i < 4) {
		goto group_loop;
	}

not_hit:
	_AL = 0;

ret:
	return _AL;
}

extern "C" void pascal far chargeshot_render_kana(void)
{
	register int i;

	if(kana_chargeshot_state[pid_current] == 0) {
		goto ret;
	}
	word_1FD8C = reinterpret_cast<uint16_t>(
		kana_chargeshot_nodes + (pid_current * (4 * 54))
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
	__emit__(0x31, 0xD2); // XOR DX, DX
	_AH = SPRITE16_SET_OVERLAP;
	geninterrupt(SPRITE16);
	i = 0;
	goto group_loop_test;

group_loop:
	kana_chargeshot_1BDF8();
	i++;
	word_1FD8C += 0x36;

group_loop_test:
	if(i < 4) {
		goto group_loop;
	}
	_asm {
		mov	dx, OVERLAP_CLEAR
		mov	ah, SPRITE16_SET_OVERLAP
		int	SPRITE16
	}

ret:
}

void pascal near gauge_pattern_kana(uint8_t type)
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
		kana_gauge_pattern_frames[pid_current] = 0;
		gba_flag_active[pid_current]++;
		kana_gauge_pattern_x[pid_current] = 0;
		byte_202B8[pid_current * 4] = (
			8 - (static_cast<int>(gba_gauge_level[pid_current]) / 4)
		);
		byte_202B9[pid_current * 4] = (gba_gauge_level[pid_current] + 0x30);
		return;
	}

	if(gba_flag_active[pid_current] != (flag_expected + 1)) {
		return;
	}

	if((kana_gauge_pattern_frames[pid_current] % byte_202B8[pid_current * 4]) != 0) {
		goto motion;
	}

	pid_other = (1 - pid_current);
	bullet_template.type = static_cast<bullet_type_t>(type);
	bullet_template.pid = pid_other;
	bullet_template.center.y.v = 0;
	bullet_template.center.x.v = kana_gauge_pattern_x[pid_current];
	bullet_template.group = BG_1;
	bullet_template.angle = 0x40;
	bullet_template.speed.v = byte_202B9[pid_current * 4];
	bullets_add();
	SUB_CE0C(
		bullet_template.center.x.v,
		0,
		static_cast<uint16_t>(pid_other)
	);
	bullet_template.center.x.v = (
		(PLAYFIELD_W << 4) - kana_gauge_pattern_x[pid_current]
	);
	bullets_add();
	SUB_CE0C(
		bullet_template.center.x.v,
		0,
		static_cast<uint16_t>(pid_other)
	);

motion:
	if((kana_gauge_pattern_frames[pid_current] % 0x40) >= 0x20) {
		goto move_left;
	}
	kana_gauge_pattern_x[pid_current] += 0x80;
	goto clear_test;

move_left:
	kana_gauge_pattern_x[pid_current] -= 0x80;

clear_test:
	if(kana_gauge_pattern_frames[pid_current] >= 0x80) {
		gba_flag_active[pid_current] = GBAF_NONE;
		sub_A3A8(1 - pid_current);
	}
	kana_gauge_pattern_frames[pid_current]++;
}
