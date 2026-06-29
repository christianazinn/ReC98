#pragma option -zCmain_06_TEXT -zPmain_06

#include "codegen.hpp"
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "libs/sprite16/sprite16.h"
#include "th02/snd/snd.h"
#include "th01/math/subpixel.hpp"
#include "th03/main/collmap.hpp"
#include "th03/main/difficul.hpp"
#include "th03/main/hitbox.hpp"
#include "th03/main/player/cur.hpp"
#include "th03/main/playfld.hpp"
#include "th03/main/sprite16.hpp"
#include "th03/math/randring.hpp"
#include "th03/math/vector.hpp"

extern "C" uint8_t exatt_buffers[];
extern "C" uint8_t pid_PID_so_attack;
extern "C" uint16_t word_2028A;
extern "C" uint16_t far randring_far_next16_raw(void);

struct player_stuff_t {
	PlayfieldPoint center;
	uint8_t unused[0x7C];
};
extern player_stuff_t players[PLAYER_COUNT];

extern "C" uint8_t near sub_1A1A7(void)
{
	register uint8_t near *slot;

	slot = reinterpret_cast<uint8_t near *>(word_2028A);
	_DX = *reinterpret_cast<subpixel_t near *>(slot + 2);
	_DX += *reinterpret_cast<subpixel_t near *>(slot + 6);
	_BX = word_2028A;
	if(reinterpret_cast<uint8_t near *>(_BX)[0x10] != 0) {
		goto other_side;
	}
	if(
		*reinterpret_cast<subpixel_t near *>(slot + 0x0C) >
		static_cast<int>(_DX)
	) {
		goto move;
	}

done:
	*reinterpret_cast<subpixel_t near *>(slot + 2) = (
		*reinterpret_cast<subpixel_t near *>(slot + 0x0A)
	);
	slot[0] = 3;
	_AL = 1;
	_AL -= pid_current;
	slot[0x10] = _AL;
	_AL = 1;
	goto ret;

other_side:
	if(
		*reinterpret_cast<subpixel_t near *>(slot + 0x0C) >=
		static_cast<int>(_DX)
	) {
		goto done;
	}

move:
	*reinterpret_cast<subpixel_t near *>(slot + 2) = _DX;
	*reinterpret_cast<subpixel_t near *>(slot + 4) += (
		*reinterpret_cast<subpixel_t near *>(slot + 8)
	);
	_AL = 0;

ret:
	return _AL;
}

extern "C" void pascal near sub_1A1ED(
	subpixel_t x,
	subpixel_t y1,
	subpixel_t target_x,
	subpixel_t target_y,
	pid_t pid,
	int velocity_base
)
{
	register uint8_t near *slot;
	register screen_x_t target_x_screen;

	target_x_screen = target_x;
	slot = reinterpret_cast<uint8_t near *>(word_2028A);
	slot[0] = 2;
	slot[1] = 0;
	*reinterpret_cast<subpixel_t near *>(slot + 2) = x;
	*reinterpret_cast<subpixel_t near *>(slot + 4) = y1;
	slot[0x10] = pid;
	x = playfield_fg_x_to_screen(x, pid);
	*reinterpret_cast<subpixel_t near *>(slot + 0x0A) = target_x_screen;
	target_x_screen = playfield_fg_x_to_screen(target_x_screen, (1 - pid));
	vector2_between_plus(
		(x << 4),
		y1,
		(target_x_screen << 4),
		target_y,
		0,
		*reinterpret_cast<int near *>(slot + 6),
		*reinterpret_cast<int near *>(slot + 8),
		(velocity_base + (static_cast<int>(round_speed) / 4))
	);
	*reinterpret_cast<subpixel_t near *>(slot + 0x0C) = screen_x_to_playfield(
		target_x_screen, pid
	);
}

extern "C" void pascal far exatt_add_reimu(
	subpixel_t center_x, subpixel_t center_y, pid_t pid
)
{
	register uint8_t near *slot;
	register int i;

	_AL = pid;
	_AH = 0;
	_AX <<= 9;
	_AX += reinterpret_cast<uint16_t>(exatt_buffers);
	slot = reinterpret_cast<uint8_t near *>(_AX);

	i = 0;
	goto loop_test;
loop:
	if(slot[0] == 0) {
		word_2028A = reinterpret_cast<uint16_t>(slot);
		sub_1A1ED(
			center_x,
			center_y,
			randring_far_next16_mod(PLAYFIELD_W << 4),
			randring_far_next16_and(2047),
			pid,
			0x5A
		);
		return;
	}
	i++;
	slot += 0x20;
loop_test:
	if(i < 8) {
		goto loop;
	}
}

extern "C" void pascal far reimu_1A2CE(
	subpixel_t x, subpixel_t y, uint8_t angle
)
{
	register uint8_t near *slot;
	register int i;

	_AL = pid_current;
	_AH = 0;
	_AX <<= 9;
	_AX += reinterpret_cast<uint16_t>(exatt_buffers);
	slot = reinterpret_cast<uint8_t near *>(_AX);

	i = 0;
	goto loop_test;
loop:
	if(slot[0] == 0) {
		slot[0] = 1;
		slot[1] = 0;
		*reinterpret_cast<subpixel_t near *>(slot + 2) = x;
		*reinterpret_cast<subpixel_t near *>(slot + 4) = y;
		_AL = 1;
		_AL -= pid_current;
		slot[0x10] = _AL;
		vector2(
			*reinterpret_cast<int near *>(slot + 6),
			*reinterpret_cast<int near *>(slot + 8),
			angle,
			0x50
		);
		return;
	}
	i++;
	slot += 0x20;
loop_test:
	if(i < 8) {
		goto loop;
	}
}

extern "C" void pascal near sub_1A32A(
	screen_x_t left, screen_y_t top, uint8_t frame
)
{
	register sprite16_offset_t so;

	if(frame & 1) {
		return;
	}
	so = ((80 * ROW_SIZE) + (320 / BYTE_DOTS));
	_AL = frame;
	_AH = 0;
	_AX &= 7;
	if(static_cast<int>(_AX) <= 3) {
		so += 4;
	}
	sprite16_put_size.w.v = (32 / 16);
	sprite16_put_size.h = 16;
	sprite16_clip.left = PLAYFIELD1_CLIP_LEFT;
	sprite16_clip.right = PLAYFIELD2_CLIP_RIGHT;
	sprite16_put((left - 16), (top - 16), so);
}

extern "C" void pascal near sub_1A377(
	screen_x_t left, screen_y_t top, uint8_t frame
)
{
	register sprite16_offset_t so;

	sprite16_clip.left = PLAYFIELD1_CLIP_LEFT;
	sprite16_clip.right = PLAYFIELD2_CLIP_RIGHT;
	sprite16_put_size.w.v = (48 / 16);
	sprite16_put_size.h = 24;
	so = ((80 * ROW_SIZE) + (384 / BYTE_DOTS));
	_AL = frame;
	_AH = 0;
	_BX = 4;
	asm { cwd; idiv bx; }
	_BX = 3;
	asm { cwd; idiv bx; }
	imul_reg_to_reg(_DX, _DX, 6);
	so += _DX;
	sprite16_put((left - 24), (top - 24), so);
}

extern "C" void near reimu_1A3C4(void)
{
	screen_x_t left;
	screen_y_t top;
	uint8_t frame_div4;
	register uint8_t near *slot;
	register sprite16_offset_t so;

	slot = reinterpret_cast<uint8_t near *>(word_2028A);
	left = playfield_fg_x_to_screen(
		*reinterpret_cast<subpixel_t near *>(slot + 2),
		slot[0x10]
	);
	top = ((*reinterpret_cast<subpixel_t near *>(slot + 4) >> 4) + 16);
	sprite16_put_size.w.v = (48 / 16);
	sprite16_put_size.h = 24;
	if(pid_current == 1) {
		sprite16_clip.left = PLAYFIELD1_CLIP_LEFT;
		sprite16_clip.right = PLAYFIELD1_CLIP_RIGHT;
	} else {
		sprite16_clip.left = PLAYFIELD2_CLIP_LEFT;
		sprite16_clip.right = PLAYFIELD2_CLIP_RIGHT;
	}

	if(slot[0] != 1) {
		goto generic;
	}
	_AL = pid_PID_so_attack;
	_AH = 0;
	_AX += (8 * ROW_SIZE);
	so = _AX;
	_AL = slot[1];
	_AH = 0;
	_BX = 4;
	asm { cwd; idiv bx; }
	frame_div4 = _AL;
	_AH = 0;
	_AX &= 3;
	if(static_cast<int>(_AX) == 1) {
		so += 6;
		goto put;
	}
	if((frame_div4 & 3) != 0) {
		_AL = frame_div4;
		_AH = 0;
		_AX &= 1;
		imul_reg_to_reg(_AX, _AX, 6);
		_AX += (24 * ROW_SIZE);
		so += _AX;
	}

put:
	sprite16_put((left - 24), (top - 24), so);
	goto ret;

generic:
	if(slot[0] == 2) {
		sub_1A32A(left, top, *reinterpret_cast<uint16_t near *>(slot + 1));
		goto ret;
	}
	sub_1A377(left, top, *reinterpret_cast<uint16_t near *>(slot));

ret:
}

extern "C" void pascal near sub_1A491(
	subpixel_t center_x, subpixel_t center_y
)
{
	collmap_center.x.v = center_x;
	collmap_center.y.v = center_y;
	collmap_stripe_tile_w.v = (16 / COLLMAP_TILE_W);
	collmap_tile_h.v = (32 / COLLMAP_TILE_H);
	_AL = 1;
	_AL -= pid_current;
	collmap_pid = _AL;
	collmap_set_rect_striped();

	collmap_center.x.v -= TO_SP(12);
	collmap_stripe_tile_w.v = (8 / COLLMAP_TILE_W);
	collmap_tile_h.v = (16 / COLLMAP_TILE_H);
	collmap_set_rect_striped();

	collmap_center.x.v += TO_SP(24);
	collmap_set_rect_striped();
}

void far exatt_update_reimu(void)
{
	uint8_t angle;
	register uint8_t near *slot;
	register int i;

	hitbox_hittest_skip_explosions = true;
	hitbox.radius.x.v = TO_SP(16);
	hitbox.radius.y.v = TO_SP(16);
	_AL = 1;
	_AL -= pid_current;
	hitbox.pid = _AL;
	_AL = pid_current;
	_AH = 0;
	_AX <<= 9;
	_AX += reinterpret_cast<uint16_t>(exatt_buffers);
	slot = reinterpret_cast<uint8_t near *>(_AX);

	i = 0;
	goto loop_test;
loop:
	if(slot[0] == 0) {
		goto next;
	}
	if(slot[0] != 1) {
		goto not_state_1;
	}
	*reinterpret_cast<subpixel_t near *>(slot + 2) += (
		*reinterpret_cast<subpixel_t near *>(slot + 6)
	);
	if(*reinterpret_cast<subpixel_t near *>(slot + 2) <= 0) {
		goto bounce_x;
	}
	if(*reinterpret_cast<subpixel_t near *>(slot + 2) < TO_SP(PLAYFIELD_W)) {
		goto update_y;
	}

bounce_x:
	_DX = 0xFFFF;
	_AX = *reinterpret_cast<subpixel_t near *>(slot + 6);
	asm { imul dx; }
	*reinterpret_cast<subpixel_t near *>(slot + 6) = _AX;
	*reinterpret_cast<subpixel_t near *>(slot + 2) += (
		*reinterpret_cast<subpixel_t near *>(slot + 6)
	);

update_y:
	*reinterpret_cast<subpixel_t near *>(slot + 4) += (
		*reinterpret_cast<subpixel_t near *>(slot + 8)
	);
	if(*reinterpret_cast<subpixel_t near *>(slot + 4) >= TO_SP(-24)) {
		goto bottom_check;
	}
	_DX = 0xFFFF;
	_AX = *reinterpret_cast<subpixel_t near *>(slot + 8);
	asm { imul dx; }
	*reinterpret_cast<subpixel_t near *>(slot + 8) = _AX;
	*reinterpret_cast<subpixel_t near *>(slot + 4) = TO_SP(-24);

bottom_check:
	if(*reinterpret_cast<subpixel_t near *>(slot + 4) < TO_SP(368)) {
		goto active_state_1;
	}
	slot[0] = 0;
	goto next;

active_state_1:
	(*reinterpret_cast<subpixel_t near *>(slot + 8))++;
	hitbox.origin.center.x.v = *reinterpret_cast<subpixel_t near *>(slot + 2);
	hitbox.origin.center.y.v = *reinterpret_cast<subpixel_t near *>(slot + 4);
	hitbox_hittest();
	sub_1A491(
		*reinterpret_cast<subpixel_t near *>(slot + 2),
		*reinterpret_cast<subpixel_t near *>(slot + 4)
	);
	goto next;

not_state_1:
	if(slot[0] != 2) {
		goto not_state_2;
	}
	word_2028A = reinterpret_cast<uint16_t>(slot);
	if(sub_1A1A7() == 0) {
		goto next;
	}
	angle = randring_far_next16_and(0x1F);
	_AX = randring_far_next16_and(1);
	imul_reg_to_reg(_AX, _AX, 0x60);
	_AL += angle;
	_AL += 0x80;
	angle = _AL;
	vector2(
		*reinterpret_cast<int near *>(slot + 6),
		*reinterpret_cast<int near *>(slot + 8),
		angle,
		((static_cast<int>(round_speed) / 8) + 0x32)
	);
	goto next;

not_state_2:
	if(slot[0] > 0x14) {
		goto start_state_1;
	}
	slot[0]++;
	goto next;

start_state_1:
	slot[0] = 1;

next:
	i++;
	slot += 0x20;

loop_test:
	if(i < 8) {
		goto loop;
	}
	hitbox_hittest_skip_explosions = false;
}

void far exatt_render_reimu(void)
{
	register uint8_t near *slot;
	register int i;

	_AL = pid_current;
	_AH = 0;
	_AX <<= 9;
	_AX += reinterpret_cast<uint16_t>(exatt_buffers);
	slot = reinterpret_cast<uint8_t near *>(_AX);

	i = 0;
	goto loop_test;
loop:
	if(slot[0] != 0) {
		word_2028A = reinterpret_cast<uint16_t>(slot);
		reimu_1A3C4();
		slot[1]++;
	}
	i++;
	slot += 0x20;

loop_test:
	if(i < 8) {
		goto loop;
	}
}

extern "C" void pascal far exatt_add_mima(
	subpixel_t center_x, subpixel_t center_y, pid_t pid
)
{
	register uint8_t near *slot;
	register int i;

	_AL = pid;
	_AH = 0;
	_AX <<= 9;
	_AX += reinterpret_cast<uint16_t>(exatt_buffers);
	slot = reinterpret_cast<uint8_t near *>(_AX);

	i = 0;
	goto loop_test;
loop:
	if(slot[0] == 0) {
		word_2028A = reinterpret_cast<uint16_t>(slot);
		sub_1A1ED(
			center_x,
			center_y,
			randring_far_next16_mod(PLAYFIELD_W << 4),
			randring_far_next16_mod(PLAYFIELD_W << 4),
			pid,
			0x5A
		);
		*reinterpret_cast<uint16_t near *>(slot + 0x0E) = 0;
		slot[1] = 3;
		return;
	}
	i++;
	slot += 0x20;

loop_test:
	if(i < 6) {
		goto loop;
	}
}

extern "C" void near mima_1A684(void)
{
	screen_x_t left;
	screen_y_t top;
	uint8_t frame;
	register uint8_t near *slot;
	register sprite16_offset_t so;

	slot = reinterpret_cast<uint8_t near *>(word_2028A);
	left = playfield_fg_x_to_screen(
		*reinterpret_cast<subpixel_t near *>(slot + 2),
		slot[0x10]
	);
	top = ((*reinterpret_cast<subpixel_t near *>(slot + 4) >> 4) + 16);
	sprite16_put_size.w.v = (48 / 16);
	sprite16_put_size.h = 24;
	if(pid_current == 1) {
		sprite16_clip.left = PLAYFIELD1_CLIP_LEFT;
		sprite16_clip.right = PLAYFIELD1_CLIP_RIGHT;
	} else {
		sprite16_clip.left = PLAYFIELD2_CLIP_LEFT;
		sprite16_clip.right = PLAYFIELD2_CLIP_RIGHT;
	}

	frame = slot[0x0E];
	if(slot[0] != 1) {
		goto not_state_1;
	}
	_AL = pid_PID_so_attack;
	_AH = 0;
	_AX += (8 * ROW_SIZE);
	so = _AX;
	if(slot[1] != 0) {
		goto not_frame_0;
	}
	so += ((24 * ROW_SIZE) + (48 / BYTE_DOTS));
	goto put;

not_frame_0:
	if(slot[1] != 1) {
		goto not_frame_1;
	}
	so += (24 * ROW_SIZE);
	goto put;

not_frame_1:
	if(slot[1] == 2) {
		so += 6;
	}

put:
	sprite16_put((left - 24), (top - 24), so);
	goto ret;

not_state_1:
	if(slot[0] != 2) {
		goto generic;
	}
	sub_1A32A(left, top, frame);
	goto ret;

generic:
	sub_1A377(left, top, *reinterpret_cast<uint16_t near *>(slot));

ret:
}

void far exatt_update_mima(void)
{
	int i;
	subpixel_t center_y;
	int timer;
	uint8_t pid_other;
	register uint8_t near *slot;
	register subpixel_t center_x;

	_AL = pid_current;
	_AH = 0;
	_AX <<= 9;
	_AX += reinterpret_cast<uint16_t>(exatt_buffers);
	slot = reinterpret_cast<uint8_t near *>(_AX);
	_AL = 1;
	_AL -= pid_current;
	pid_other = _AL;
	collmap_stripe_tile_w.v = (16 / COLLMAP_TILE_W);
	collmap_tile_h.v = (16 / COLLMAP_TILE_H);
	collmap_pid = _AL;
	i = 0;
	goto loop_test;

loop:
	if(slot[0] == 0) {
		goto next;
	}
	if(slot[0] != 1) {
		goto not_state_1;
	}
	center_x = (
		*reinterpret_cast<subpixel_t near *>(slot + 2) +
		*reinterpret_cast<subpixel_t near *>(slot + 6)
	);
	center_y = (
		*reinterpret_cast<subpixel_t near *>(slot + 4) +
		*reinterpret_cast<subpixel_t near *>(slot + 8)
	);
	if(center_x <= 0) {
		goto bounce_x;
	}
	if(center_x < TO_SP(PLAYFIELD_W)) {
		goto check_y;
	}

bounce_x:
	if(slot[1] == 0) {
		goto deactivate;
	}
	slot[1]--;
	*reinterpret_cast<subpixel_t near *>(slot + 6) = (
		-*reinterpret_cast<subpixel_t near *>(slot + 6)
	);
	goto update_angle;

check_y:
	if(center_y <= 0) {
		goto bounce_y;
	}
	if(center_y < TO_SP(PLAYFIELD_H)) {
		goto in_bounds;
	}

bounce_y:
	if(slot[1] == 0) {
		goto deactivate;
	}
	slot[1]--;
	*reinterpret_cast<subpixel_t near *>(slot + 8) = (
		-*reinterpret_cast<subpixel_t near *>(slot + 8)
	);

update_angle:
	slot[0x12] = iatan2(
		*reinterpret_cast<int near *>(slot + 8),
		*reinterpret_cast<int near *>(slot + 6)
	);
	goto active_state_1;

deactivate:
	slot[0] = 0;
	goto next;

in_bounds:
	*reinterpret_cast<subpixel_t near *>(slot + 2) = center_x;
	*reinterpret_cast<subpixel_t near *>(slot + 4) = center_y;

active_state_1:
	timer = *reinterpret_cast<uint16_t near *>(slot + 0x0E);
	if(timer < 0x10) {
		goto speed_done;
	}
	if(timer >= 0x20) {
		goto speed_high;
	}
	_AL = slot[0x13];
	_DL = static_cast<uint8_t>(timer);
	_DL &= 1;
	_AL += _DL;
	slot[0x13] = _AL;
	goto speed_done;

speed_high:
	if(timer < 0x50) {
		slot[0x13]++;
	}

speed_done:
	if(timer >= 0x50) {
		goto hittest;
	}
	vector2(
		*reinterpret_cast<int near *>(slot + 6),
		*reinterpret_cast<int near *>(slot + 8),
		slot[0x12],
		slot[0x13]
	);

hittest:
	hitbox_hittest_skip_explosions = true;
	hitbox.radius.x.v = TO_SP(16);
	hitbox.radius.y.v = TO_SP(16);
	hitbox.pid = pid_other;
	hitbox.origin.center.x.v = center_x;
	hitbox.origin.center.y.v = center_y;
	if(hitbox_hittest() != 0) {
		*reinterpret_cast<subpixel_t near *>(slot + 8) -= 8;
	}
	hitbox_hittest_skip_explosions = false;
	collmap_center.x.v = center_x;
	collmap_center.y.v = center_y;
	collmap_set_rect_striped();
	goto inc_timer;

not_state_1:
	if(slot[0] != 2) {
		goto not_state_2;
	}
	word_2028A = reinterpret_cast<uint16_t>(slot);
	if(sub_1A1A7() == 0) {
		goto inc_timer;
	}
	slot[0x12] = randring_far_next16_raw();
	slot[0x13] = 0;
	*reinterpret_cast<subpixel_t near *>(slot + 6) = 0;
	*reinterpret_cast<subpixel_t near *>(slot + 8) = 0;
	goto next;

not_state_2:
	if(slot[0] > 0x1C) {
		goto start_state_1;
	}
	slot[0]++;
	goto inc_timer;

start_state_1:
	*reinterpret_cast<uint16_t near *>(slot + 0x0E) = 0;
	slot[0] = 1;
	snd_se_play(10);

inc_timer:
	(*reinterpret_cast<uint16_t near *>(slot + 0x0E))++;

next:
	i++;
	slot += 0x20;

loop_test:
	if(i < 6) {
		goto loop;
	}
}

void far exatt_render_mima(void)
{
	register uint8_t near *slot;
	register int i;

	_AL = pid_current;
	_AH = 0;
	_AX <<= 9;
	_AX += reinterpret_cast<uint16_t>(exatt_buffers);
	slot = reinterpret_cast<uint8_t near *>(_AX);

	i = 0;
	goto loop_test;
loop:
	if(slot[0] != 0) {
		word_2028A = reinterpret_cast<uint16_t>(slot);
		mima_1A684();
	}
	i++;
	slot += 0x20;

loop_test:
	if(i < 6) {
		goto loop;
	}
}

void pascal far exatt_add_yumemi(
	subpixel_t center_x, subpixel_t center_y, pid_t pid
)
{
	register uint8_t near *slot;
	register int i;

	_AL = pid;
	_AH = 0;
	_AX <<= 9;
	_AX += reinterpret_cast<uint16_t>(exatt_buffers);
	slot = reinterpret_cast<uint8_t near *>(_AX);

	i = 0;
	goto loop_test;
loop:
	if(slot[0] == 0) {
		word_2028A = reinterpret_cast<uint16_t>(slot);
		sub_1A1ED(
			center_x,
			center_y,
			randring_far_next16_mod(PLAYFIELD_W << 4),
			(randring_far_next16_mod(256 << 4) + (96 << 4)),
			pid,
			0x46
		);
		*reinterpret_cast<uint16_t near *>(slot + 0x14) = 0;
		*reinterpret_cast<uint16_t near *>(slot + 0x0E) = 0x60;
		return;
	}
	i++;
	slot += 0x20;

loop_test:
	if(i < 8) {
		goto loop;
	}
}

extern "C" void pascal far yumemi_1A95F(
	subpixel_t target_x, subpixel_t target_y
)
{
	register uint8_t near *slot;

	_AL = pid_current;
	_AH = 0;
	_AX <<= 9;
	_asm { add ax, (offset exatt_buffers + 256); }
	slot = reinterpret_cast<uint8_t near *>(_AX);

	_DX = 8;
	goto loop_test;
loop:
	if(slot[0] == 0) {
		slot[0] = 3;
		slot[1] = 0;
		*reinterpret_cast<subpixel_t near *>(slot + 2) = target_x;
		*reinterpret_cast<subpixel_t near *>(slot + 4) = target_y;
		_AL = 1;
		_AL -= pid_current;
		slot[0x10] = _AL;
		*reinterpret_cast<subpixel_t near *>(slot + 0x14) = 0;
		*reinterpret_cast<subpixel_t near *>(slot + 0x0E) = 0x20;
		return;
	}
	_DX++;
	slot += 0x20;

loop_test:
	_asm { cmp dx, 0x10; }
	_asm { jl loop; }
}

extern "C" void near yumemi_1A9B0(void)
{
	sprite16_offset_t so;
	screen_x_t strip_left;
	screen_y_t top;
	int8_t frame;
	int8_t half_frame;
	uint8_t near *slot;
	register int y;
	register screen_x_t left;

	slot = reinterpret_cast<uint8_t near *>(word_2028A);
	left = playfield_fg_x_to_screen(
		*reinterpret_cast<subpixel_t near *>(slot + 2),
		slot[0x10]
	);
	top = ((*reinterpret_cast<subpixel_t near *>(slot + 4) >> 4) + 16);
	if(pid_current == 1) {
		sprite16_clip.left = PLAYFIELD1_CLIP_LEFT;
		sprite16_clip.right = PLAYFIELD1_CLIP_RIGHT;
	} else {
		sprite16_clip.left = PLAYFIELD2_CLIP_LEFT;
		sprite16_clip.right = PLAYFIELD2_CLIP_RIGHT;
	}

	if(slot[0] != 1) {
		goto not_state_1;
	}
	if(pid_current == 1) {
		grc_setclip(16, 8, 303, 191);
	} else {
		grc_setclip(336, 8, 623, 191);
	}
	if(slot[1] > 0x10) {
		goto collapse;
	}

	egc_off();
	grcg_setcolor(GC_RMW, 6);
	frame = slot[1];
	y = (top / 2);
	half_frame = (frame / 2);
	grcg_hline(
		(left + frame - 32),
		((left + 32) - frame),
		(y + half_frame - 40)
	);
	grcg_hline(
		(left + frame - 32),
		((left + 32) - frame),
		((y + 52) - half_frame)
	);
	grcg_vline(
		(left + frame - 32),
		(y + half_frame - 40),
		((y + 52) - half_frame)
	);
	grcg_vline(
		((left + 32) - frame),
		(y + half_frame - 40),
		((y + 52) - half_frame)
	);
	grcg_hline(
		(left + frame - 80),
		((left + 80) - frame),
		(y + half_frame - 16)
	);
	grcg_hline(
		(left + frame - 80),
		((left + 80) - frame),
		((y + 16) - half_frame)
	);
	grcg_vline(
		(left + frame - 80),
		(y + half_frame - 16),
		((y + 16) - half_frame)
	);
	grcg_vline(
		((left + 80) - frame),
		(y + half_frame - 16),
		((y + 16) - half_frame)
	);
	goto grcg_done;

collapse:
	egc_off();
	frame = (slot[0x0E] - 8 - slot[1]);
	asm {
		cmp	byte ptr [bp - 7], 10h
		jle	short collapse_color_6
	}
	grcg_setcolor(GC_RMW, 5);
	goto collapse_color_done;
collapse_color_6:
	grcg_setcolor(GC_RMW, 6);
collapse_color_done:
	y = (top / 2);
	half_frame = (frame / 2);
	grcg_hline(
		(left + frame - 16),
		((left + 16) - frame),
		(y + half_frame - 32)
	);
	grcg_hline(
		(left + frame - 16),
		((left + 16) - frame),
		((y + 44) - half_frame)
	);
	grcg_vline(
		(left + frame - 16),
		(y + half_frame - 32),
		((y + 44) - half_frame)
	);
	grcg_vline(
		((left + 16) - frame),
		(y + half_frame - 32),
		((y + 44) - half_frame)
	);
	grcg_hline(
		(left + frame - 64),
		((left + 64) - frame),
		(y + half_frame - 8)
	);
	grcg_hline(
		(left + frame - 64),
		((left + 64) - frame),
		((y + 8) - half_frame)
	);
	grcg_vline(
		(left + frame - 64),
		(y + half_frame - 8),
		((y + 8) - half_frame)
	);
	grcg_vline(
		((left + 64) - frame),
		(y + half_frame - 8),
		((y + 8) - half_frame)
	);

grcg_done:
	grcg_off();
	egc_on();
	grc_setclip(0, 0, (RES_X - 1), (SPRITE16_RES_Y - 1));
	left -= 16;
	sprite16_put_size.w.v = (32 / 16);
	sprite16_put_size.h = 8;
	_AL = pid_PID_so_attack;
	_AH = 0;
	_AX += (8 * ROW_SIZE);
	so = _AX;
	y = (top - *reinterpret_cast<subpixel_t near *>(slot + 0x14));
	sprite16_put(left, y, so);
	y += 16;
	while((top - 16) > y) {
		sprite16_put(
			left, y, (so + ((16 * ROW_SIZE) + (16 / BYTE_DOTS)))
		);
		y += 16;
	}
	_AX = (*reinterpret_cast<subpixel_t near *>(slot + 0x14) / 2);
	_DX = *reinterpret_cast<subpixel_t near *>(slot + 0x14);
	_DX += top;
	_AX += _DX;
	_AX += 0xFFF0;
	y = _AX;
	sprite16_put(left, _AX, (so + (8 * ROW_SIZE)));
	y -= 16;
	while(y > top) {
		sprite16_put(
			left, y, (so + ((16 * ROW_SIZE) + (16 / BYTE_DOTS)))
		);
		y -= 16;
	}
	sprite16_put_size.w.v = (16 / 16);
	sprite16_put_size.h = 16;
	top -= 16;
	strip_left = (
		left - *reinterpret_cast<subpixel_t near *>(slot + 0x14) + 16
	);
	sprite16_put(strip_left, top, so);
	strip_left += 16;
	while(strip_left < left) {
		sprite16_put(strip_left, top, (so + (16 * ROW_SIZE)));
		strip_left += 16;
	}
	strip_left = (
		*reinterpret_cast<subpixel_t near *>(slot + 0x14) + left
	);
	sprite16_put(strip_left, top, (so + 2));
	strip_left -= 16;
	while((left + 16) < strip_left) {
		sprite16_put(strip_left, top, (so + (16 * ROW_SIZE)));
		strip_left -= 16;
	}
	if(*reinterpret_cast<subpixel_t near *>(slot + 0x14) > 0x18) {
		sprite16_put_size.w.v = (32 / 16);
		sprite16_put_size.h = 16;
		sprite16_put(left, top, (so + 4));
	}
	goto ret;

not_state_1:
	if(slot[0] == 2) {
		sub_1A32A(
			left, top, *reinterpret_cast<uint16_t near *>(slot + 1)
		);
		goto ret;
	}
	sub_1A377(left, top, *reinterpret_cast<uint16_t near *>(slot + 1));

ret:
}

void far exatt_update_yumemi(void)
{
	uint8_t pid_other;
	register uint8_t near *slot;
	register int i;

	_AL = pid_current;
	_AH = 0;
	_AX <<= 9;
	_AX += reinterpret_cast<uint16_t>(exatt_buffers);
	slot = reinterpret_cast<uint8_t near *>(_AX);
	_AL = 1;
	_AL -= pid_current;
	pid_other = _AL;
	collmap_pid = _AL;
	i = 0;
	goto loop_test;

loop:
	if(slot[0] == 0) {
		goto next;
	}
	if(slot[0] != 1) {
		goto not_state_1;
	}
	if(slot[1] > 8) {
		goto collapse_check;
	}
	*reinterpret_cast<uint16_t near *>(slot + 0x14) += 6;
	goto active_state_1;

collapse_check:
	_AX = slot[1];
	if(static_cast<int>(_AX) < *reinterpret_cast<int near *>(slot + 0x0E)) {
		goto deactivate_check;
	}
	_AX = slot[1];
	_DX = *reinterpret_cast<uint16_t near *>(slot + 0x0E);
	_DX += 8;
	if(static_cast<int>(_AX) >= static_cast<int>(_DX)) {
		goto deactivate_check;
	}
	*reinterpret_cast<uint16_t near *>(slot + 0x14) -= 6;
	goto active_state_1;

deactivate_check:
	_AX = slot[1];
	_DX = *reinterpret_cast<uint16_t near *>(slot + 0x0E);
	_DX += 8;
	if(static_cast<int>(_AX) < static_cast<int>(_DX)) {
		goto active_state_1;
	}
	slot[0] = 0;

active_state_1:
	hitbox_hittest_skip_explosions = true;
	hitbox.pid = pid_other;
	hitbox.radius.x.v = (*reinterpret_cast<uint16_t near *>(slot + 0x14) << 3);
	hitbox.radius.y.v = TO_SP(4);
	hitbox.origin.center.x.v = *reinterpret_cast<subpixel_t near *>(slot + 2);
	hitbox.origin.center.y.v = *reinterpret_cast<subpixel_t near *>(slot + 4);
	hitbox_hittest();

	hitbox.radius.x.v = TO_SP(4);
	hitbox.radius.y.v = (*reinterpret_cast<uint16_t near *>(slot + 0x14) << 3);
	hitbox.origin.center.x.v = *reinterpret_cast<subpixel_t near *>(slot + 2);
	hitbox.origin.center.y.v = *reinterpret_cast<subpixel_t near *>(slot + 4);
	hitbox_hittest();
	hitbox_hittest_skip_explosions = false;

	collmap_stripe_tile_w.v = *reinterpret_cast<uint16_t near *>(slot + 0x14);
	collmap_tile_h.v = (16 / COLLMAP_TILE_H);
	collmap_center.x.v = *reinterpret_cast<subpixel_t near *>(slot + 2);
	collmap_center.y.v = *reinterpret_cast<subpixel_t near *>(slot + 4);
	collmap_set_rect_striped();

	collmap_center.y.v = (
		(*reinterpret_cast<uint16_t near *>(slot + 0x14) << 2) +
		*reinterpret_cast<subpixel_t near *>(slot + 4)
	);
	collmap_stripe_tile_w.v = (16 / COLLMAP_TILE_W);
	_AX = *reinterpret_cast<uint16_t near *>(slot + 0x14);
	_BX = 4;
	asm { cwd; }
	asm { idiv bx; }
	_AX += *reinterpret_cast<uint16_t near *>(slot + 0x14);
	collmap_tile_h.v = _AX;
	collmap_set_rect_striped();
	goto inc_frame;

not_state_1:
	if(slot[0] != 2) {
		goto not_state_2;
	}
	word_2028A = reinterpret_cast<uint16_t>(slot);
	if(sub_1A1A7() != 0) {
		goto next;
	}
	goto inc_frame;

not_state_2:
	if(slot[0] > 0x28) {
		goto start_state_1;
	}
	slot[0]++;
	goto inc_frame;

start_state_1:
	*reinterpret_cast<uint16_t near *>(slot + 0x14) = 0x10;
	slot[1] = 0;
	slot[0] = 1;
	snd_se_play(7);

inc_frame:
	slot[1]++;

next:
	i++;
	slot += 0x20;

loop_test:
	if(i < 0x10) {
		goto loop;
	}
}

void far exatt_render_yumemi(void)
{
	register int i;

	_AL = pid_current;
	_AH = 0;
	_AX <<= 9;
	_AX += reinterpret_cast<uint16_t>(exatt_buffers);
	word_2028A = _AX;

	i = 0;
	goto loop_test;
loop:
	if(*reinterpret_cast<uint8_t near *>(word_2028A) != 0) {
		yumemi_1A9B0();
	}
	i++;
	word_2028A += 0x20;

loop_test:
	if(i < 0x10) {
		goto loop;
	}
}

extern "C" void pascal far exatt_add_rikako(
	subpixel_t center_x, subpixel_t center_y, pid_t pid
)
{
	register uint8_t near *slot;
	register int i;

	_AL = pid;
	_AH = 0;
	_AX <<= 9;
	_AX += reinterpret_cast<uint16_t>(exatt_buffers);
	slot = reinterpret_cast<uint8_t near *>(_AX);

	i = 0;
	goto loop_test;
loop:
	if(slot[0] == 0) {
		word_2028A = reinterpret_cast<uint16_t>(slot);
		sub_1A1ED(
			center_x,
			center_y,
			randring_far_next16_mod(PLAYFIELD_W << 4),
			(16 << 4),
			pid,
			0x64
		);
		slot[0x17] = randring_far_next16_and(1);
		slot[0x12] = 0x40;
		slot[0x13] = (randring_far_next16_and(0x0F) + 0x20);
		return;
	}
	i++;
	slot += 0x20;

loop_test:
	if(i < 8) {
		goto loop;
	}
}

extern "C" void pascal far RIKAKO_1B006(
	subpixel_t x, subpixel_t y, uint8_t angle
)
{
	register uint8_t near *slot;

	_AL = pid_current;
	_AH = 0;
	_AX <<= 9;
	_AX += reinterpret_cast<uint16_t>(exatt_buffers);
	slot = reinterpret_cast<uint8_t near *>(_AX);

	_DX = 0;
	goto loop_test;
loop:
	if(slot[0] == 0) {
		slot[0] = 1;
		slot[0x17] = 1;
		slot[1] = 0x40;
		*reinterpret_cast<subpixel_t near *>(slot + 2) = x;
		*reinterpret_cast<subpixel_t near *>(slot + 4) = y;
		slot[0x12] = angle;
		slot[0x13] = 0x20;
		_AL = 1;
		_AL -= pid_current;
		slot[0x10] = _AL;
		return;
	}
	_DX++;
	slot += 0x20;

loop_test:
	_asm { cmp dx, 0x0E; }
	_asm { jl loop; }
}

extern "C" void near rikako_1B05A(void)
{
	screen_x_t screen_x;
	screen_y_t top;
	uint8_t frame;
	register uint8_t near *slot;
	register sprite16_offset_t so;

	slot = reinterpret_cast<uint8_t near *>(word_2028A);
	sprite16_put_size.w.v = (48 / 16);
	sprite16_put_size.h = 24;
	screen_x = playfield_fg_x_to_screen(
		*reinterpret_cast<subpixel_t near *>(slot + 2),
		slot[0x10]
	);
	top = ((*reinterpret_cast<subpixel_t near *>(slot + 4) >> 4) + 16);
	if(pid_current != 0) {
		sprite16_clip.left = PLAYFIELD1_CLIP_LEFT;
		sprite16_clip.right = PLAYFIELD1_CLIP_RIGHT;
	} else {
		sprite16_clip.left = PLAYFIELD2_CLIP_LEFT;
		sprite16_clip.right = PLAYFIELD2_CLIP_RIGHT;
	}
	frame = slot[1];
	if(slot[0] != 1) {
		goto not_state_1;
	}
	_AL = pid_PID_so_attack;
	_AH = 0;
	_AX += (24 * ROW_SIZE);
	so = _AX;
	if((frame & 1) != 0) {
		so += (24 * ROW_SIZE);
	}
	sprite16_put((screen_x - 24), (top - 24), so);
	goto ret;

not_state_1:
	if(slot[0] != 2) {
		goto generic;
	}
	sub_1A32A(screen_x, top, frame);
	goto ret;

generic:
	sub_1A377(screen_x, top, frame);

ret:
}

void far exatt_update_rikako(void)
{
	int vector_x;
	int vector_y;
	uint8_t pid_other;
	uint8_t target_angle;
	int8_t angle_delta;
	register uint8_t near *slot;
	register int i;

	_AL = pid_current;
	_AH = 0;
	_AX <<= 9;
	_AX += reinterpret_cast<uint16_t>(exatt_buffers);
	slot = reinterpret_cast<uint8_t near *>(_AX);
	_AL = 1;
	_AL -= pid_current;
	pid_other = _AL;
	playfield_clip_negative_radius.x.v = TO_SP(-32);
	playfield_clip_negative_radius.y.v = TO_SP(-32);
	hitbox_hittest_skip_explosions = true;
	hitbox.radius.x.v = TO_SP(16);
	hitbox.radius.y.v = TO_SP(16);
	hitbox.pid = _AL;
	i = 0;
	goto loop_test;

loop:
	if(slot[0] == 0) {
		goto next;
	}
	if(slot[0] != 1) {
		goto not_state_1;
	}
	vector2(vector_x, vector_y, slot[0x12], slot[0x13]);
	*reinterpret_cast<subpixel_t near *>(slot + 2) += vector_x;
	*reinterpret_cast<subpixel_t near *>(slot + 4) += vector_y;
	if(slot[0x17] != 0) {
		goto clip;
	}
	if(slot[1] < 0x40) {
		goto clip;
	}
	if(slot[1] > 0x50) {
		goto clip;
	}
	target_angle = iatan2(
		(players[pid_other].center.y.v - *reinterpret_cast<subpixel_t near *>(slot + 4)),
		(players[pid_other].center.x.v - *reinterpret_cast<subpixel_t near *>(slot + 2))
	);
	slot[0x13]++;
	angle_delta = (target_angle - slot[0x12]);
	angle_delta = (static_cast<int>(angle_delta) / 8);
	slot[0x12] += angle_delta;

clip:
	if(playfield_clip(
		reinterpret_cast<PlayfieldSubpixel near *>(slot + 2)[0],
		reinterpret_cast<PlayfieldSubpixel near *>(slot + 4)[0]
	) != false) {
		slot[0] = 0;
		goto next;
	}
	hitbox.origin.center.x.v = *reinterpret_cast<subpixel_t near *>(slot + 2);
	hitbox.origin.center.y.v = *reinterpret_cast<subpixel_t near *>(slot + 4);
	hitbox_hittest();
	sub_1A491(
		*reinterpret_cast<subpixel_t near *>(slot + 2),
		*reinterpret_cast<subpixel_t near *>(slot + 4)
	);
	goto inc_frame;

not_state_1:
	if(slot[0] != 2) {
		goto not_state_2;
	}
	word_2028A = reinterpret_cast<uint16_t>(slot);
	sub_1A1A7();
	goto inc_frame;

not_state_2:
	if(slot[0] > 0x1C) {
		goto start_state_1;
	}
	slot[0]++;
	goto inc_frame;

start_state_1:
	slot[1] = 0;
	slot[0] = 1;

inc_frame:
	slot[1]++;

next:
	i++;
	slot += 0x20;

loop_test:
	if(i < 0x0E) {
		goto loop;
	}
	hitbox_hittest_skip_explosions = false;
}

void far exatt_render_rikako(void)
{
	register uint8_t near *slot;
	register int i;

	_AL = pid_current;
	_AH = 0;
	_AX <<= 9;
	_AX += reinterpret_cast<uint16_t>(exatt_buffers);
	slot = reinterpret_cast<uint8_t near *>(_AX);

	i = 0;
	goto loop_test;
loop:
	if(slot[0] != 0) {
		word_2028A = reinterpret_cast<uint16_t>(slot);
		rikako_1B05A();
	}
	i++;
	slot += 0x20;

loop_test:
	if(i < 0x0E) {
		goto loop;
	}
}
