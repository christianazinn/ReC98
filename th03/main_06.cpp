#pragma option -zCmain_06_TEXT -zPmain_06

#include "codegen.hpp"
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
