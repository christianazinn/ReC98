#pragma option -zCmain_10_TEXT -zPmain_10

#include "libs/sprite16/sprite16.h"
#include "platform.h"
#include "th02/snd/snd.h"
#include "th01/math/subpixel.hpp"
#include "th03/main/bullet/bullet.hpp"
#include "th03/main/enemy/efe.hpp"
#include "th03/main/hitbox.hpp"
#include "th03/main/hitcirc.hpp"
#include "th03/main/playfld.hpp"
#include "th03/main/player/cur.hpp"
#include "th03/main/player/gba.hpp"
#include "th03/main/sprite16.hpp"
#include "th03/math/randring.hpp"

extern "C" uint8_t byte_202B8[];
extern "C" uint8_t byte_202B9[];
extern "C" uint8_t byte_202BA[];
extern "C" uint8_t kotohime_chargeshot[];
extern "C" uint8_t kotohime_gauge_pattern_frames[];
extern "C" uint8_t pid_PID_so_attack;
extern "C" uint16_t word_1FE6A;

extern "C" void pascal far kotohime_19DD3(
	subpixel_t target_x, subpixel_t target_y
);
extern "C" void pascal far sub_A3A8(uint8_t pid);

struct player_stuff_t {
	uint8_t unused_0[0x18];
	uint16_t gauge_charged;
	uint8_t unused_1[0x66];
};

extern player_stuff_t players[PLAYER_COUNT];

extern "C" void far sub_1C158(void)
{
	kotohime_chargeshot[0] = 0;
	kotohime_chargeshot[8] = 0;
}

extern "C" void pascal far chargeshot_add_kotohime(
	Subpixel center_x, Subpixel center_y
)
{
	word_1FE6A = reinterpret_cast<uint16_t>(
		kotohime_chargeshot + (pid.current * 8)
	);
	_BX = word_1FE6A;
	reinterpret_cast<uint8_t near *>(_BX)[0] = 1;
	reinterpret_cast<uint8_t near *>(_BX)[1] = 0;
	*reinterpret_cast<Subpixel near *>(_BX + 2) = center_x;
	*reinterpret_cast<Subpixel near *>(_BX + 4) = center_y;
	*reinterpret_cast<subpixel_t near *>(_BX + 6) = -0x10;
	snd_se_play(6);
}

extern "C" void pascal far chargeshot_update_kotohime(void)
{
	word_1FE6A = reinterpret_cast<uint16_t>(
		kotohime_chargeshot + (pid_current * 8)
	);
	_BX = word_1FE6A;
	if(reinterpret_cast<uint8_t near *>(_BX)[0] == 0) {
		goto ret;
	}
	players[pid_current].gauge_charged = 0;
	_BX = word_1FE6A;
	_AX = *reinterpret_cast<subpixel_t near *>(_BX + 6);
	asm { add [bx+4], ax; }
	if(*reinterpret_cast<subpixel_t near *>(_BX + 4) > -0x100) {
		goto accelerate;
	}
	reinterpret_cast<uint8_t near *>(_BX)[0] = 0;
	return;

accelerate:
	_BX = word_1FE6A;
	*reinterpret_cast<subpixel_t near *>(_BX + 6) -= 2;

ret:
}

#pragma warn -aus
extern "C" void near kotohime_chargeshot_1C1E9(void)
{
	screen_x_t left;
	screen_y_t top;
	sprite16_offset_t sprite_offset;

	sprite_offset = (
		pid_PID_so_attack + ((56 * ROW_SIZE) + (64 / BYTE_DOTS))
	);
	_BX = word_1FE6A;
	left = (playfield_fg_x_to_screen(
		*reinterpret_cast<subpixel_t near *>(_BX + 2),
		pid_current
	) - 48);
	_BX = word_1FE6A;
	_AX = *reinterpret_cast<subpixel_t near *>(_BX + 4);
	asm { sar ax, 4; }
	_AX += 8;
	top = _AX;
	sprite16_put(left, _AX, sprite_offset);
}

uint8_t far chargeshot_hittest_kotohime(void)
{
	word_1FE6A = reinterpret_cast<uint16_t>(
		kotohime_chargeshot + (hitbox.pid * 8)
	);
	_BX = word_1FE6A;
	if(reinterpret_cast<uint8_t near *>(_BX)[0] == 0) {
		goto not_hit;
	}
	_BX = word_1FE6A;
	_AX = *reinterpret_cast<subpixel_t near *>(_BX + 2);
	_AX -= hitbox.right.v;
	if(static_cast<int>(_AX) > TO_SP(40)) {
		goto not_hit;
	}
	_AX = hitbox.origin.topleft.x.v;
	_AX -= *reinterpret_cast<subpixel_t near *>(_BX + 2);
	if(static_cast<int>(_AX) > TO_SP(40)) {
		goto not_hit;
	}
	_AX = *reinterpret_cast<subpixel_t near *>(_BX + 4);
	if(static_cast<int>(_AX) < hitbox.origin.topleft.y.v) {
		goto not_hit;
	}
	if(static_cast<int>(_AX) > hitbox.bottom.v) {
		goto not_hit;
	}
	ef_onehit = true;
	_AX = hitbox.origin.topleft.x.v;
	_AX += hitbox.radius.x.v;
	hitcircles_enemy_add(
		_AX,
		*reinterpret_cast<subpixel_t near *>(_BX + 4),
		hitbox.pid
	);
	return 3;

not_hit:
	return 0;
}

extern "C" void pascal far chargeshot_render_kotohime(void)
{
	word_1FE6A = reinterpret_cast<uint16_t>(
		kotohime_chargeshot + (pid_current * 8)
	);
	_BX = word_1FE6A;
	if(reinterpret_cast<uint8_t near *>(_BX)[0] == 0) {
		goto ret;
	}
	sprite16_put_size.w.v = (96 / 16);
	sprite16_put_size.h = 16;
	if(pid_current == 0) {
		sprite16_clip.left = PLAYFIELD1_CLIP_LEFT;
		sprite16_clip.right = PLAYFIELD1_CLIP_RIGHT;
	} else {
		sprite16_clip.left = PLAYFIELD2_CLIP_LEFT;
		sprite16_clip.right = PLAYFIELD2_CLIP_RIGHT;
	}
	kotohime_chargeshot_1C1E9();

ret:
}

void pascal near gauge_pattern_kotohime(uint8_t type)
{
	uint8_t flag_expected;

	flag_expected = GBAF_GAUGE_PELLET_INIT;
	if(type == BT_BULLET16_DEFAULT) {
		_AL = flag_expected;
		_AL += GBAF_PELLET_TO_BULLET;
		flag_expected = _AL;
	}

	if(gba_flag_active[pid_current] == flag_expected) {
		kotohime_gauge_pattern_frames[pid_current] = 0;
		gba_flag_active[pid_current]++;
		byte_202B8[pid_current * 4] = (
			(static_cast<int>(gba_gauge_level[pid_current]) / 2) + 4
		);
		byte_202B9[pid_current * 4] = randring_far_next16_and(1);
		byte_202BA[pid_current * 4] = type;
		return;
	}

	if(gba_flag_active[pid_current] != (flag_expected + 1)) {
		return;
	}

	if(kotohime_gauge_pattern_frames[pid_current] == 0) {
		_AX = randring_far_next16_mod(160 << 4);
		_AX += (64 << 4);
		asm {
			push ax;
			push 0;
			call far ptr kotohime_19DD3;
		}
		goto frame_next;
	}

	if(kotohime_gauge_pattern_frames[pid_current] >= 0x80) {
		gba_flag_active[pid_current] = GBAF_NONE;
		sub_A3A8(1 - pid_current);
	}

frame_next:
	kotohime_gauge_pattern_frames[pid_current]++;
}

extern "C" void pascal far gba_gauge_pattern_pellet_kotohime(void)
{
	if(gba_flag_active[pid_current] != GBAF_NONE) {
		gauge_pattern_kotohime(BT_PELLET);
	}
}

extern "C" void pascal far gba_gauge_pattern_bullet_kotohime(void)
{
	if(gba_flag_active[pid_current] != GBAF_NONE) {
		gauge_pattern_kotohime(BT_BULLET16_DEFAULT);
	}
}
