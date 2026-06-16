#include "th03/main/player/exatt.hpp"
#include "codegen.hpp"
#include "libs/master.lib/master.hpp"
#include "th02/snd/snd.h"
#include "th03/main/bullet/bullet.hpp"
#include "th03/main/hitbox.hpp"
#include "th03/main/player/cur.hpp"
#include "th03/main/player/stuff.hpp"
#include "th03/main/sprite16.hpp"
#include "th03/math/polar.hpp"
#include "th03/math/randring.hpp"

extern "C" uint8_t exatt_buffers[];
extern "C" uint8_t byte_202B8[];
extern "C" uint8_t byte_202B9[];
extern "C" uint8_t byte_202BA[];
extern "C" uint8_t ellen_exatt_refs[];
extern "C" uint8_t pid_PID_so_attack;
extern "C" uint16_t word_1FB3A;
extern "C" uint16_t word_2028A;
extern "C" uint16_t far randring_far_next16_raw(void);
extern "C" void pascal far SUB_CDBD(void);
extern "C" void pascal far SUB_CE0C(subpixel_t x, subpixel_t y, uint16_t pid);
extern "C" void near chiyuri_1905A(void);
extern "C" void near ellen_19510(void);
extern "C" void near kana_198DD(void);
extern "C" void near marisa_19B4F(void);
extern "C" uint8_t near sub_1A1A7(void);
extern "C" void pascal vector2(
	int &ret_x,
	int &ret_y,
	int length,
	int angle
);
extern "C" void pascal near sub_1A1ED(
	subpixel_t x,
	subpixel_t y1,
	subpixel_t target_x,
	subpixel_t target_y,
	pid_t pid,
	int velocity_base
);
extern "C" void pascal near sub_1A32A(screen_x_t left, screen_y_t top, uint8_t frame);
extern "C" void pascal near sub_1A377(screen_x_t left, screen_y_t top, uint8_t frame);

void far exatt_update_chiyuri(void)
{
	int pair_done;
	uint8_t pid_other;
	register uint8_t near *slot;
	register int i;

	_AL = pid_current;
	_AH = 0;
	_AX <<= 9;
	_AX += reinterpret_cast<uint16_t>(exatt_buffers);
	word_2028A = _AX;

	_AL = 1;
	_AL -= pid_current;
	pid_other = _AL;
	collmap_stripe_tile_w.v = (4 / COLLMAP_TILE_W);
	collmap_pid = _AL;

	i = 0;
	goto loop_test;
loop:
	if(*reinterpret_cast<uint8_t near *>(word_2028A) != 0) {
		slot = reinterpret_cast<uint8_t near *>(word_2028A);
		if(slot[0] != 1) {
			goto not_state_1;
		}
		slot[1]++;
		if(slot[1] == 4) {
			SUB_CE0C(
				*reinterpret_cast<subpixel_t near *>(slot + 2),
				0,
				slot[0x10]
			);
			slot += 0x20;
			SUB_CE0C(
				*reinterpret_cast<subpixel_t near *>(slot + 2),
				(368 << 4),
				slot[0x10]
			);
			goto pair_advance;
		} else if(slot[1] == 0x10) {
			snd_se_play(20);
			goto pair_advance;
		} else if((slot[1] > 0x28) && (slot[1] < 0x70)) {
			collmap_topleft.x.v = (
				*reinterpret_cast<subpixel_t near *>(slot + 2)
			);
			slot += 0x20;
			collmap_bottomright.x.v = (
				*reinterpret_cast<subpixel_t near *>(slot + 2)
			);
			collmap_set_slope_striped();
			goto pair_advance;
		} else if(slot[1] > 0x88) {
			slot[0] = 0;
			slot += 0x20;
			slot[0] = 0;
		}
	}
pair_advance:
	word_2028A += 0x20;
	goto next;

not_state_1:
	pair_done = 0;
	if(slot[0] == 2) {
		if(sub_1A1A7() != 0) {
			*reinterpret_cast<subpixel_t near *>(slot + 4) = 0;
		}
	} else if(slot[0] <= 0x1C) {
		slot[0]++;
	} else {
		slot[1] = 0;
		pair_done = 1;
	}
	slot[1]++;

	slot += 0x20;
	word_2028A += 0x20;
	if(slot[0] == 2) {
		if(sub_1A1A7() != 0) {
			*reinterpret_cast<subpixel_t near *>(slot + 4) = (368 << 4);
		}
	} else if(slot[0] <= 0x1C) {
		slot[0]++;
	} else {
		slot[1] = 0;
		if(pair_done != 0) {
			slot -= 0x20;
			slot[0] = 1;
		}
	}
	slot[1]++;
next:
	i += 2;
	word_2028A += 0x20;
loop_test:
	if(i < 0x10) {
		goto loop;
	}
}

void far exatt_render_chiyuri(void)
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
		chiyuri_1905A();
	}
	i += 2;
	word_2028A += 0x40;
loop_test:
	if(i < 0x10) {
		goto loop;
	}
}

extern "C" void far sub_193BC(void)
{
	register uint8_t near *refs_p1;
	register uint8_t near *refs_p2;

	refs_p1 = ellen_exatt_refs;
	refs_p2 = (ellen_exatt_refs + (12 * 30));
	_DX = 0;
	goto loop_test;
loop:
	_AX = _DX;
	_AX <<= 5;
	_AX += reinterpret_cast<uint16_t>(exatt_buffers);
	*reinterpret_cast<uint16_t near *>(refs_p1) = _AX;

	_AX = _DX;
	_AX <<= 5;
	_AX += reinterpret_cast<uint16_t>(exatt_buffers + 512);
	*reinterpret_cast<uint16_t near *>(refs_p2) = _AX;

	_DX++;
	refs_p1 += 30;
	refs_p2 += 30;
loop_test:
	if(static_cast<int>(_DX) < 12) {
		goto loop;
	}
}

extern "C" void pascal far exatt_add_ellen(
	subpixel_t center_x, subpixel_t center_y, pid_t pid
)
{
	subpixel_t target_x;
	subpixel_t target_y;
	uint8_t spawned;
	int8_t delta;
	uint8_t angle;
	register int i;

	spawned = 0;
	target_x = randring_far_next16_mod(200 << 4) + (44 << 4);
	target_y = randring_far_next16_mod(200 << 4) + (80 << 4);
	if(randring_far_next16_and(1) == 0) {
		_AL = -1;
	} else {
		_AL = 1;
	}
	delta = _AL;
	angle = randring_far_next16_raw();

	_AL = pid;
	_AH = 0;
	_asm { imul ax, ax, (12 * 30); }
	_AX += reinterpret_cast<uint16_t>(ellen_exatt_refs);
	word_1FB3A = _AX;

	i = 0;
	goto loop_test;
loop:
	if(*reinterpret_cast<uint8_t near *>(
		*reinterpret_cast<uint16_t near *>(word_1FB3A)
	) == 0) {
		word_2028A = *reinterpret_cast<uint16_t near *>(word_1FB3A);
		sub_1A1ED(
			center_x,
			center_y,
			target_x,
			target_y,
			pid,
			0x46
		);
		*reinterpret_cast<uint16_t near *>(word_2028A + 0x14) = 0;
		reinterpret_cast<uint8_t near *>(word_2028A)[0x11] = delta;
		if(spawned == 0) {
			reinterpret_cast<uint8_t near *>(word_2028A)[0x12] = angle;
			spawned++;
			goto next;
		}
		_AL = angle;
		_AL += 0x80;
		reinterpret_cast<uint8_t near *>(word_2028A)[0x12] = _AL;
		return;
	}
next:
	i++;
	word_1FB3A += 30;
loop_test:
	if(i < 12) {
		goto loop;
	}
}

extern "C" void pascal far ellen_194A9(
	subpixel_t x, subpixel_t y, uint8_t angle, uint8_t delta
)
{
	register uint8_t near *slot;

	_AL = pid_current;
	_AH = 0;
	_asm { imul ax, ax, (12 * 30); }
	_AX += reinterpret_cast<uint16_t>(ellen_exatt_refs);
	word_1FB3A = _AX;

	_DX = 0;
	goto loop_test;
loop:
	if(*reinterpret_cast<uint8_t near *>(
		*reinterpret_cast<uint16_t near *>(word_1FB3A)
	) == 0) {
		slot = reinterpret_cast<uint8_t near *>(
			*reinterpret_cast<uint16_t near *>(word_1FB3A)
		);
		slot[0] = 1;
		slot[1] = 0;
		*reinterpret_cast<subpixel_t near *>(slot + 2) = x;
		*reinterpret_cast<subpixel_t near *>(slot + 4) = y;
		*reinterpret_cast<uint16_t near *>(slot + 0x14) = 0;
		slot[0x11] = delta;
		slot[0x12] = angle;
		_AL = 1;
		_AL -= pid_current;
		slot[0x10] = _AL;
		return;
	}
	_DX++;
	word_1FB3A += 30;
loop_test:
	if(static_cast<int>(_DX) < 12) {
		goto loop;
	}
}

extern "C" void near ellen_19510(void)
{
	screen_x_t x;
	screen_y_t top;
	screen_y_t trail_top;
	screen_x_t trail_left;
	uint8_t frame;
	register int i;
	register sprite16_offset_t so;

	x = playfield_fg_x_to_screen(
		*reinterpret_cast<subpixel_t near *>(
			reinterpret_cast<uint8_t near *>(
				*reinterpret_cast<uint16_t near *>(word_1FB3A)
			) + 2
		),
		reinterpret_cast<uint8_t near *>(
			*reinterpret_cast<uint16_t near *>(word_1FB3A)
		)[0x10]
	);
	top = (
		(*reinterpret_cast<subpixel_t near *>(
			reinterpret_cast<uint8_t near *>(
				*reinterpret_cast<uint16_t near *>(word_1FB3A)
			) + 4
		) >> 4) + 16
	);
	if(reinterpret_cast<uint8_t near *>(
		*reinterpret_cast<uint16_t near *>(word_1FB3A)
	)[0x10] == 0) {
		sprite16_clip.left = PLAYFIELD1_CLIP_LEFT;
		sprite16_clip.right = PLAYFIELD1_CLIP_RIGHT;
	} else {
		sprite16_clip.left = PLAYFIELD2_CLIP_LEFT;
		sprite16_clip.right = PLAYFIELD2_CLIP_RIGHT;
	}

	frame = reinterpret_cast<uint8_t near *>(
		*reinterpret_cast<uint16_t near *>(word_1FB3A)
	)[1];
	if(*reinterpret_cast<uint8_t near *>(
		*reinterpret_cast<uint16_t near *>(word_1FB3A)
	) == 1) {
		sprite16_put_size.w.v = (32 / 16);
		sprite16_put_size.h = 16;
		_AL = pid_PID_so_attack;
		_AH = 0;
		_AX += ((8 * ROW_SIZE) + (96 / BYTE_DOTS));
		so = _AX;
		i = 6;
		goto trail_test;
trail:
		trail_left = playfield_fg_x_to_screen(
			reinterpret_cast<subpixel_t near *>(
				reinterpret_cast<uint8_t near *>(word_1FB3A) + 2
			)[i],
			reinterpret_cast<uint8_t near *>(
				*reinterpret_cast<uint16_t near *>(word_1FB3A)
			)[0x10]
		);
		trail_top = (
			(
				reinterpret_cast<subpixel_t near *>(
					reinterpret_cast<uint8_t near *>(word_1FB3A) + 0x10
				)[i] >> 4
			) + 16
		);
		sprite16_put((trail_left - 16), (trail_top - 16), so);
		so -= 4;
		i -= 2;
trail_test:
		__emit__(0x0B, 0xF6);
		asm { jge trail; }
		goto ret;
	} else if(*reinterpret_cast<uint8_t near *>(
		*reinterpret_cast<uint16_t near *>(word_1FB3A)
	) == 2) {
		sub_1A32A(x, top, frame);
	} else {
		sub_1A377(
			x,
			top,
			*reinterpret_cast<uint16_t near *>(
				*reinterpret_cast<uint16_t near *>(word_1FB3A)
			)
		);
	}
ret:
}

void far exatt_update_ellen(void)
{
	int i;
	int j;
	uint8_t pid_other;
	register uint8_t near *ref;

	_AL = pid_current;
	_AH = 0;
	_asm { imul ax, ax, (12 * 30); }
	_AX += reinterpret_cast<uint16_t>(ellen_exatt_refs);
	ref = reinterpret_cast<uint8_t near *>(_AX);
	_AL = 1;
	_AL -= pid_current;
	pid_other = _AL;
	playfield_clip_negative_radius.x.v = TO_SP(-24);
	playfield_clip_negative_radius.y.v = TO_SP(-24);
	collmap_stripe_tile_w.v = (12 / COLLMAP_TILE_W);
	collmap_tile_h.v = (12 / COLLMAP_TILE_H);
	collmap_pid = _AL;

	i = 0;
	goto loop_test;
loop:
	if(*reinterpret_cast<uint8_t near *>(
		*reinterpret_cast<uint16_t near *>(ref)
	) != 0) {
		_BX = *reinterpret_cast<uint16_t near *>(ref);
		if(*reinterpret_cast<uint8_t near *>(_BX) == 1) {
			j = 6;
			goto shift_test;
shift:
			reinterpret_cast<subpixel_t near *>(ref + 2)[j] = (
				reinterpret_cast<subpixel_t near *>(ref + 2)[j - 1]
			);
			reinterpret_cast<subpixel_t near *>(ref + 0x10)[j] = (
				reinterpret_cast<subpixel_t near *>(ref + 0x10)[j - 1]
			);
			j--;
shift_test:
			if(j > 0) {
				goto shift;
			}

			if(
				*reinterpret_cast<subpixel_t near *>(
					reinterpret_cast<uint8_t near *>(
						*reinterpret_cast<uint16_t near *>(ref)
					) + 0x14
				) <= 0x0D00
			) {
				_AL = reinterpret_cast<uint8_t near *>(
					*reinterpret_cast<uint16_t near *>(ref)
				)[0x12];
				_AH = 0;
				_AX += _AX;
				_BX = _AX;
				*reinterpret_cast<subpixel_t near *>(ref + 2) = polar(
					*reinterpret_cast<subpixel_t near *>(
						reinterpret_cast<uint8_t near *>(
							*reinterpret_cast<uint16_t near *>(ref)
						) + 2
					),
					*reinterpret_cast<subpixel_t near *>(
						reinterpret_cast<uint8_t near *>(
							*reinterpret_cast<uint16_t near *>(ref)
						) + 0x14
					),
					*reinterpret_cast<const short near *>(
						reinterpret_cast<const uint8_t near *>(CosTable8) + _BX
					)
				);
				_AL = reinterpret_cast<uint8_t near *>(
					*reinterpret_cast<uint16_t near *>(ref)
				)[0x12];
				_AH = 0;
				_AX += _AX;
				_BX = _AX;
				*reinterpret_cast<subpixel_t near *>(ref + 0x10) = polar(
					*reinterpret_cast<subpixel_t near *>(
						reinterpret_cast<uint8_t near *>(
							*reinterpret_cast<uint16_t near *>(ref)
						) + 4
					),
					*reinterpret_cast<subpixel_t near *>(
						reinterpret_cast<uint8_t near *>(
							*reinterpret_cast<uint16_t near *>(ref)
						) + 0x14
					),
					*reinterpret_cast<const short near *>(
						reinterpret_cast<const uint8_t near *>(SinTable8) + _BX
					)
				);
				*reinterpret_cast<subpixel_t near *>(
					reinterpret_cast<uint8_t near *>(
						*reinterpret_cast<uint16_t near *>(ref)
					) + 6
				) = (
					*reinterpret_cast<subpixel_t near *>(ref + 2) -
					*reinterpret_cast<subpixel_t near *>(ref + 4)
				);
				*reinterpret_cast<subpixel_t near *>(
					reinterpret_cast<uint8_t near *>(
						*reinterpret_cast<uint16_t near *>(ref)
					) + 8
				) = (
					*reinterpret_cast<subpixel_t near *>(ref + 0x10) -
					*reinterpret_cast<subpixel_t near *>(ref + 0x12)
				);
				*reinterpret_cast<subpixel_t near *>(
					reinterpret_cast<uint8_t near *>(
						*reinterpret_cast<uint16_t near *>(ref)
					) + 0x14
				) += 0x18;
				reinterpret_cast<uint8_t near *>(
					*reinterpret_cast<uint16_t near *>(ref)
				)[0x12] += reinterpret_cast<uint8_t near *>(
					*reinterpret_cast<uint16_t near *>(ref)
				)[0x11];
				goto hittest;
			}
			if(
				playfield_clip(
					reinterpret_cast<PlayfieldSubpixel near *>(ref + 2)[0],
					reinterpret_cast<PlayfieldSubpixel near *>(ref + 0x10)[0]
				) &&
				playfield_clip(
					reinterpret_cast<PlayfieldSubpixel near *>(ref + 0x0E)[0],
					reinterpret_cast<PlayfieldSubpixel near *>(ref + 0x1C)[0]
				)
			) {
				reinterpret_cast<uint8_t near *>(
					*reinterpret_cast<uint16_t near *>(ref)
				)[0] = 0;
				goto next;
			}
			*reinterpret_cast<subpixel_t near *>(ref + 2) += (
				*reinterpret_cast<subpixel_t near *>(
					reinterpret_cast<uint8_t near *>(
						*reinterpret_cast<uint16_t near *>(ref)
					) + 6
				)
			);
			*reinterpret_cast<subpixel_t near *>(ref + 0x10) += (
				*reinterpret_cast<subpixel_t near *>(
					reinterpret_cast<uint8_t near *>(
						*reinterpret_cast<uint16_t near *>(ref)
					) + 8
				)
			);
hittest:
			hitbox_hittest_skip_explosions = true;
			hitbox.radius.x.v = (16 << 4);
			hitbox.radius.y.v = (16 << 4);
			hitbox.pid = pid_other;
			hitbox.origin.center.x.v = (
				*reinterpret_cast<subpixel_t near *>(ref + 2)
			);
			hitbox.origin.center.y.v = (
				*reinterpret_cast<subpixel_t near *>(ref + 0x10)
			);
			hitbox_hittest();
			hitbox_hittest_skip_explosions = false;
			collmap_center.x.v = (
				*reinterpret_cast<subpixel_t near *>(ref + 2)
			);
			collmap_center.y.v = (
				*reinterpret_cast<subpixel_t near *>(ref + 0x10)
			);
			collmap_set_rect_striped();
		} else if(*reinterpret_cast<uint8_t near *>(
			*reinterpret_cast<uint16_t near *>(ref)
		) == 2) {
			word_2028A = *reinterpret_cast<uint16_t near *>(ref);
			if(sub_1A1A7() != 0) {
				j = 0;
				goto copy_test;
copy:
				reinterpret_cast<subpixel_t near *>(ref + 2)[j] = (
					*reinterpret_cast<subpixel_t near *>(
						reinterpret_cast<uint8_t near *>(word_2028A) + 2
					)
				);
				reinterpret_cast<subpixel_t near *>(ref + 0x10)[j] = (
					*reinterpret_cast<subpixel_t near *>(
						reinterpret_cast<uint8_t near *>(word_2028A) + 4
					)
				);
				j++;
copy_test:
				_asm { cmp word ptr [bp-4], 7; }
				asm { jl copy; }
				goto state_done;
			}
		} else if(*reinterpret_cast<uint8_t near *>(
			*reinterpret_cast<uint16_t near *>(ref)
		) <= 0x1C) {
			reinterpret_cast<uint8_t near *>(
				*reinterpret_cast<uint16_t near *>(ref)
			)[0]++;
		} else {
			reinterpret_cast<uint8_t near *>(
				*reinterpret_cast<uint16_t near *>(ref)
			)[1] = 0;
			reinterpret_cast<uint8_t near *>(
				*reinterpret_cast<uint16_t near *>(ref)
			)[0] = 1;
			snd_se_play(10);
		}
state_done:
		reinterpret_cast<uint8_t near *>(
			*reinterpret_cast<uint16_t near *>(ref)
		)[1]++;
	}
next:
	i++;
	ref += 30;
loop_test:
	if(i < 12) {
		goto loop;
	}
}

void far exatt_render_ellen(void)
{
	register uint8_t near *ref;
	register int i;

	_AL = pid_current;
	_AH = 0;
	_asm { imul ax, ax, (12 * 30); }
	_AX += reinterpret_cast<uint16_t>(ellen_exatt_refs);
	ref = reinterpret_cast<uint8_t near *>(_AX);

	i = 0;
	goto loop_test;
loop:
	if(*reinterpret_cast<uint8_t near *>(
		*reinterpret_cast<uint16_t near *>(ref)
	) != 0) {
		word_1FB3A = reinterpret_cast<uint16_t>(ref);
		ellen_19510();
	}
	i++;
	ref += 30;
loop_test:
	if(i < 12) {
		goto loop;
	}
}

extern "C" void pascal far exatt_add_kana(
	subpixel_t center_x, subpixel_t center_y, pid_t pid
)
{
	subpixel_t target_x;
	register uint8_t near *slot;
	register int i;

	target_x = randring_far_next16_mod(PLAYFIELD_W << 4);
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
			target_x,
			randring_far_next16_mod(128 << 4),
			pid,
			0x5A
		);
		slot[0x12] = (randring_far_next16_and(0x1F) - 0x0F);
		if(target_x >= (144 << 4)) {
			_AL = slot[0x12];
			_AL += 0x80;
			slot[0x12] = _AL;
		}
		return;
	}
	i++;
	slot += 0x20;
loop_test:
	if(i < 8) {
		goto loop;
	}
}

extern "C" void pascal far KANA_19896(subpixel_t x, subpixel_t y, uint8_t angle)
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
		*reinterpret_cast<subpixel_t near *>(slot + 2) = x;
		*reinterpret_cast<subpixel_t near *>(slot + 4) = y;
		slot[0x12] = angle;
		slot[0x13] = 8;
		slot[1] = 0;
		return;
	}
	_DX++;
	slot += 0x20;
loop_test:
	if(static_cast<int>(_DX) < 8) {
		goto loop;
	}
}

extern "C" void near kana_198DD(void)
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
	top = ((*reinterpret_cast<subpixel_t near *>(slot + 4) >> 4) + 0x10);
	_AL = pid_current;
	_AH = 0;
	_DX = 1;
	_DX -= _AX;
	_asm { jnz clip_p2; }
	sprite16_clip.left = PLAYFIELD1_CLIP_LEFT;
	sprite16_clip.right = PLAYFIELD1_CLIP_RIGHT;
	goto clip_done;
clip_p2:
	sprite16_clip.left = PLAYFIELD2_CLIP_LEFT;
	sprite16_clip.right = PLAYFIELD2_CLIP_RIGHT;
clip_done:

	frame = slot[1];
	if(slot[0] == 1) {
		sprite16_put_size.w.v = (32 / 16);
		sprite16_put_size.h = 16;
		_AL = pid_PID_so_attack;
		_AH = 0;
		_AX += ((8 * ROW_SIZE) + (32 / BYTE_DOTS));
		so = _AX;
		_AL = frame;
		_AH = 0;
		_BX = 4;
		_asm { cwd; }
		_asm { idiv bx; }
		_asm { cwd; }
		_asm { idiv bx; }
		_DX <<= 5;
		imul_reg_to_reg(_DX, _DX, 0x28);
		so += _DX;
		sprite16_put((left - 16), (top - 16), so);
	} else if(slot[0] == 2) {
		sub_1A32A(left, top, frame);
	} else {
		sub_1A377(left, top, frame);
	}
}

void far exatt_update_kana(void)
{
	int vector_x;
	int vector_y;
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
	playfield_clip_negative_radius.x.v = TO_SP(-24);
	playfield_clip_negative_radius.y.v = TO_SP(-24);

	i = 0;
	goto loop_test;
loop:
	if(slot[0] != 0) {
		word_2028A = reinterpret_cast<uint16_t>(slot);
		if(slot[0] == 1) {
			vector2(
				vector_x,
				vector_y,
				*reinterpret_cast<int near *>(slot + 0x12),
				(_AL = slot[0x13], _AH = 0, _AX)
			);
			slot[0x13]++;
			*reinterpret_cast<subpixel_t near *>(slot + 2) += vector_x;
			*reinterpret_cast<subpixel_t near *>(slot + 4) += vector_y;
			_AL = slot[1];
			_AH = 0;
			_BX = 0x10;
			_asm { cwd; }
			_asm { idiv bx; }
			if(_DX == 0) {
				bullet_template.type = BT_BULLET16_CUSTOM_WITH_ACCEL;
				bullet_template.angle = 0x40;
				bullet_template.group = BG_1;
				bullet_template.accel_type = BAT_Y;
				_AL = pid_PID_so_attack;
				_AH = 0;
				_AX += (72 * ROW_SIZE);
				bullet_template.sprite_offset = _AX;
				bullet_template.speed.v = (1 << 4);
				bullet_template.center.x.v = *reinterpret_cast<subpixel_t near *>(slot + 2);
				bullet_template.center.y.v = *reinterpret_cast<subpixel_t near *>(slot + 4);
				bullet_template.pid = pid_other;
				bullets_add();
			}
			if(playfield_clip(
				reinterpret_cast<PlayfieldSubpixel near *>(slot + 2)[0],
				reinterpret_cast<PlayfieldSubpixel near *>(slot + 4)[0]
			)) {
				slot[0] = 0;
				goto next;
			}
		} else if(slot[0] == 2) {
			sub_1A1A7();
		} else {
			if(slot[0] <= 0x1C) {
				slot[0]++;
			} else {
				slot[1] = 0;
				slot[0] = 1;
				slot[0x13] = 8;
			}
		}
		slot[1]++;
	}
next:
	i++;
	slot += 0x20;
loop_test:
	if(i < 8) {
		goto loop;
	}
}

void far exatt_render_kana(void)
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
		kana_198DD();
	}
	i++;
	slot += 0x20;
loop_test:
	if(i < 8) {
		goto loop;
	}
}

extern "C" void pascal far exatt_add_marisa(
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
			(368 << 4),
			pid,
			0x6E
		);
		return;
	}
	i++;
	slot += 0x20;
loop_test:
	if(i < 0x0E) {
		goto loop;
	}
}

extern "C" void pascal far marisa_19B06(
	subpixel_t x, subpixel_t y, pid_t pid
)
{
	register uint8_t near *slot;

	_AL = pid;
	_AH = 0;
	_DX = 1;
	_DX -= _AX;
	_DX <<= 9;
	_DX += reinterpret_cast<uint16_t>(exatt_buffers);
	slot = reinterpret_cast<uint8_t near *>(_DX);

	_CX = 0;
	goto loop_test;
loop:
	if(slot[0] == 0) {
		slot[0] = 3;
		slot[1] = 0;
		*reinterpret_cast<subpixel_t near *>(slot + 2) = x;
		*reinterpret_cast<subpixel_t near *>(slot + 4) = y;
		slot[0x10] = pid;
		return;
	}
	_CX++;
	slot += 0x20;
loop_test:
	_asm { cmp cx, 0x0E; }
	_asm { jl loop; }
}

extern "C" void near marisa_19B4F(void)
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
	if(pid_current == 1) {
		sprite16_clip.left = PLAYFIELD1_CLIP_LEFT;
		sprite16_clip.right = PLAYFIELD1_CLIP_RIGHT;
	} else {
		sprite16_clip.left = PLAYFIELD2_CLIP_LEFT;
		sprite16_clip.right = PLAYFIELD2_CLIP_RIGHT;
	}

	if(slot[0] == 1) {
		sprite16_put_size.w.v = (16 / 16);
		sprite16_put_size.h = 8;
		so = (pid_PID_so_attack + 0x1A);
		left -= 8;
		top -= 8;
		frame = slot[1];
		if(frame < 0x30) {
			sprite16_put(left, top, so);
			so += 2;
		} else if(frame < 0x60) {
			so += 2;
		} else if(frame < 0x66) {
			so += 4;
		} else if(frame < 0x6C) {
			so += 6;
		} else if(frame < 0x72) {
			so += 8;
		} else {
			so += 0x0A;
		}
		sprite16_putx(left, (top + 16), so, SPF_DOWNWARDS_COLUMN);
	} else if(slot[0] == 2) {
		sub_1A32A(
			left, top, *reinterpret_cast<uint16_t near *>(slot + 1)
		);
	} else {
		sub_1A377(left, top, *reinterpret_cast<uint16_t near *>(slot));
	}
}

void far exatt_update_marisa(void)
{
	int i;
	subpixel_t collmap_h;
	subpixel_t slot_x;
	register uint8_t near *slot;
	register subpixel_t top;

	collmap_stripe_tile_w.v = (4 / COLLMAP_TILE_W);
	_AL = pid_current;
	_AH = 0;
	_AX <<= 9;
	_AX += reinterpret_cast<uint16_t>(exatt_buffers);
	slot = reinterpret_cast<uint8_t near *>(_AX);

	i = 0;
	goto loop_test;
loop:
	if(slot[0] != 0) {
		if(slot[0] == 1) {
			if(slot[1] < 0x30) {
				*reinterpret_cast<subpixel_t near *>(slot + 4) -= (16 << 4);
				if(*reinterpret_cast<subpixel_t near *>(slot + 4) < TO_SP(-16)) {
					slot[1] = 0x30;
					*reinterpret_cast<subpixel_t near *>(slot + 4) = TO_SP(-16);
				}
			} else if(slot[1] >= 0x78) {
				slot[0] = 0;
				goto next;
			}

			slot_x = *reinterpret_cast<subpixel_t near *>(slot + 2);
			top = *reinterpret_cast<subpixel_t near *>(slot + 4);
			if(top < 0) {
				top = 0;
			}
			collmap_h = ((368 << 4) - top);
			if(slot[1] < 0x64) {
				collmap_center.x.v = slot_x;
				collmap_center.y.v = ((collmap_h / 2) + top);
				collmap_tile_h.v = (collmap_h / (64 / COLLMAP_TILE_H));
				_AL = 1;
				_AL -= pid_current;
				collmap_pid = _AL;
				collmap_set_rect_striped();
			}
		} else if(slot[0] == 2) {
			word_2028A = reinterpret_cast<uint16_t>(slot);
			if(sub_1A1A7() != 0) {
				*reinterpret_cast<subpixel_t near *>(slot + 4) = (368 << 4);
				goto next;
			}
		} else if(slot[0] <= 0x1C) {
			slot[0]++;
			if(slot[0] == 0x0C) {
				SUB_CE0C(
					*reinterpret_cast<subpixel_t near *>(slot + 2),
					(386 << 4),
					slot[0x10]
				);
			}
		} else {
			slot[1] = 0;
			slot[0] = 1;
			snd_se_play(20);
		}
		slot[1]++;
	}
next:
	i++;
	slot += 0x20;
loop_test:
	if(i < 0x0E) {
		goto loop;
	}
}

void far exatt_render_marisa(void)
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
		marisa_19B4F();
	}
	i++;
	slot += 0x20;
loop_test:
	if(i < 0x0E) {
		goto loop;
	}
}

extern "C" void pascal far exatt_add_kotohime(
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
			randring_far_next16_mod(64 << 4),
			pid,
			0x5A
		);
		*reinterpret_cast<subpixel_t near *>(slot + 0x0E) = (
			randring_far_next16_and(4095) + (96 << 4)
		);
		*reinterpret_cast<subpixel_t near *>(slot + 0x14) = (
			randring_far_next16_and(0x1F) + 0x10
		);
		slot[0x12] = 0;
		slot[0x11] = 0;
		return;
	}
	i++;
	slot += 0x20;
loop_test:
	if(i < 8) {
		goto loop;
	}
}

extern "C" void pascal far kotohime_19DD3(
	subpixel_t target_x, subpixel_t target_y
)
{
	register uint8_t near *slot;
	register player_stuff_t near *player;

	_AL = pid_current;
	_AH = 0;
	_AX <<= 7;
	_AX += reinterpret_cast<uint16_t>(players);
	player = reinterpret_cast<player_stuff_t near *>(_AX);
	_AL = pid_current;
	_AH = 0;
	_AX <<= 9;
	_asm { add ax, (offset exatt_buffers + 256); }
	slot = reinterpret_cast<uint8_t near *>(_AX);
	if(slot[0] != 0) {
		slot += 0x20;
	}
	word_2028A = reinterpret_cast<uint16_t>(slot);
	sub_1A1ED(
		player->center.x.v,
		player->center.y.v,
		target_x,
		target_y,
		pid_current,
		0x78
	);
	*reinterpret_cast<subpixel_t near *>(slot + 0x0E) = 0x0C00;
	*reinterpret_cast<subpixel_t near *>(slot + 0x14) = 0x20;
	slot[0x12] = 0;
	slot[0x11] = 1;
}

extern "C" void near kotohime_19E2A(void)
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
	top = ((*reinterpret_cast<subpixel_t near *>(slot + 4) >> 4) + 0x10);
	sprite16_clip.left = PLAYFIELD1_CLIP_LEFT;
	sprite16_clip.right = PLAYFIELD2_CLIP_RIGHT;
	frame = slot[1];
	if(slot[0] == 1) {
		so = (pid_PID_so_attack + (8 * ROW_SIZE));
		if(
			(
				*reinterpret_cast<subpixel_t near *>(slot + 0x0E) -
				*reinterpret_cast<subpixel_t near *>(slot + 4)
			) <= (96 << 4)
		) {
			if((static_cast<int>(frame) % 4) < 2) {
				so += (32 * ROW_SIZE);
			}
		}
		if(slot[0x11] == 0) {
			so += ((8 * ROW_SIZE) + (16 / BYTE_DOTS));
			sprite16_put_size.w.v = (32 / 16);
			sprite16_put_size.h = 16;
			sprite16_put((left - 16), (top - 16), so);
		} else {
			sprite16_put_size.w.v = (64 / 16);
			sprite16_put_size.h = 32;
			sprite16_put((left - 32), (top - 32), so);
		}
	} else if(slot[0] == 2) {
		sub_1A32A(left, top, frame);
	} else {
		sub_1A377(left, top, frame);
	}
}

extern "C" void near kotohime_19EF9(void)
{
	snd_se_play(3);
	_BX = word_2028A;
	_asm {
		push word ptr [bx+2]
		push word ptr [bx+4]
	}
	_AL = pid_current;
	_AH = 0;
	_DX = 1;
	_DX -= _AX;
	_asm {
		push dx
		call far ptr SUB_CDBD
	}
}

extern "C" void near kotohime_19F1F(void)
{
	uint8_t angle_delta;
	register int i;

	bullet_template.type = static_cast<bullet_type_t>(
		byte_202BA[pid_current * 4]
	);
	bullet_template.count = byte_202B8[pid_current * 4];
	if(byte_202B9[pid_current * 4] != 0) {
		angle_delta = -12;
	} else {
		angle_delta = 12;
	}
	bullet_template.speed.v = (1 << 4);

	for(i = 0; i < 4; i++) {
		bullets_add();
		bullet_template.speed.v += 2;
		bullet_template.angle += angle_delta;
	}
}

extern "C" void near kotohime_19F87(void)
{
	uint8_t angle_delta;
	register int i;

	bullet_template.type = static_cast<bullet_type_t>(
		byte_202BA[pid_current * 4]
	);
	bullet_template.speed.v = (1 << 4);
	bullet_template.count = byte_202B8[pid_current * 4];
	if(byte_202B9[pid_current * 4] != 0) {
		angle_delta = 7;
	} else {
		angle_delta = -7;
	}

	for(i = 0; i < 10; i++) {
		bullets_add();
		bullet_template.speed.v += 2;
		bullet_template.angle += angle_delta;
	}
}

void far exatt_update_kotohime(void)
{
	uint8_t pid_other;
	register uint8_t near *slot;
	register int i;

	_AL = pid_current;
	_AH = 0;
	_AX <<= 9;
	_AX += reinterpret_cast<uint16_t>(exatt_buffers);
	word_2028A = _AX;

	_AL = 1;
	_AL -= pid_current;
	pid_other = _AL;
	collmap_stripe_tile_w.v = (12 / COLLMAP_TILE_W);
	collmap_tile_h.v = (12 / COLLMAP_TILE_H);
	collmap_pid = _AL;

	i = 0;
	goto loop_test;
loop:
	if(*reinterpret_cast<uint8_t near *>(word_2028A) != 0) {
		slot = reinterpret_cast<uint8_t near *>(word_2028A);
		if(slot[0] == 1) {
			if(
				*reinterpret_cast<subpixel_t near *>(slot + 4) >=
				*reinterpret_cast<subpixel_t near *>(slot + 0x0E)
			) {
				if(slot[0x12] == 4) {
					kotohime_19EF9();
					if(slot[0x11] != 0) {
						bullet_template.speed.v = ((1 << 4) + 12);
						bullet_template.angle = randring_far_next16_raw();
						bullet_template.group = BG_RING;
						bullet_template.pid = pid_other;
						bullet_template.center.x.v = (
							*reinterpret_cast<subpixel_t near *>(slot + 2)
						);
						bullet_template.center.y.v = (
							*reinterpret_cast<subpixel_t near *>(slot + 4)
						);
						kotohime_19F1F();
					}
				} else if((slot[0x12] == 8) || (slot[0x12] == 0x0C)) {
					kotohime_19EF9();
				} else if(slot[0x12] == 0x10) {
					kotohime_19EF9();
					bullet_template.speed.v = ((1 << 4) + 8);
					bullet_template.angle = randring_far_next16_raw();
					bullet_template.group = BG_RING;
					bullet_template.pid = pid_other;
					bullet_template.center.x.v = (
						*reinterpret_cast<subpixel_t near *>(slot + 2)
					);
					bullet_template.center.y.v = (
						*reinterpret_cast<subpixel_t near *>(slot + 4)
					);
					if(slot[0x11] == 0) {
						bullet_template.type = BT_BULLET16_DEFAULT;
						bullet_template.count = 10;
						bullets_add();
					} else {
						kotohime_19F87();
					}
					slot[0] = 0;
				}
				slot[0x12]++;
			} else {
				*reinterpret_cast<subpixel_t near *>(slot + 4) += (
					*reinterpret_cast<subpixel_t near *>(slot + 0x14)
				);
				hitbox_hittest_skip_explosions = true;
				hitbox.radius.x.v = (16 << 4);
				hitbox.radius.y.v = (16 << 4);
				hitbox.pid = pid_other;
				hitbox.origin.center.x.v = (
					*reinterpret_cast<subpixel_t near *>(slot + 2)
				);
				hitbox.origin.center.y.v = (
					*reinterpret_cast<subpixel_t near *>(slot + 4)
				);
				hitbox_hittest();
				hitbox_hittest_skip_explosions = false;
				collmap_center.x.v = (
					*reinterpret_cast<subpixel_t near *>(slot + 2)
				);
				collmap_center.y.v = (
					*reinterpret_cast<subpixel_t near *>(slot + 4)
				);
				collmap_set_rect_striped();
			}
		} else if(slot[0] == 2) {
			sub_1A1A7();
		} else if(slot[0] <= 0x1C) {
			slot[0]++;
		} else {
			slot[1] = 0;
			slot[0] = 1;
		}
		slot[1]++;
	}
	i++;
	word_2028A += 0x20;
loop_test:
	if(i < 10) {
		goto loop;
	}
}

void far exatt_render_kotohime(void)
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
		kotohime_19E2A();
	}
	i++;
	word_2028A += 0x20;
loop_test:
	if(i < 10) {
		goto loop;
	}
}

void pascal exatt_add(Subpixel center_x_, Subpixel center_y_, pid_t pid)
{
	// ZUN bloat: Bloat within bloat!
	subpixel_t center_x = center_x_;
	subpixel_t center_y = center_y_;

	static_assert(PLAYER_COUNT == 2);
	if(pid == 0) {
		exatt_funcs[0].add(center_x, center_y, 0);
	} else {
		exatt_funcs[1].add(center_x, center_y, 1);
	}
}
