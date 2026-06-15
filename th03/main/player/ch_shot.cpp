#pragma codeseg HITBOX_TEXT

#include "libs/master.lib/pc98_gfx.hpp"
#include "libs/sprite16/sprite16.h"
#include "platform.h"
#include "th02/snd/snd.h"
#include "th03/formats/mrs.hpp"
#include "th03/hardware/palette.hpp"
#include "th03/main/bullet/bullet.hpp"
#include "th03/main/hitbox.hpp"
#include "th03/main/hitcirc.hpp"
#include "th03/main/player/bomb.hpp"
#include "th03/main/player/cur.hpp"
#include "th03/main/player/gba.hpp"
#include "th03/main/player/stuff.hpp"
#include "th03/main/playfld.hpp"
#include "th03/main/round.hpp"
#include "th03/main/sprite16.hpp"
#include "th03/main/v_colors.hpp"
#include "th03/math/vector.hpp"
#include "th03/math/randring.hpp"
#include "x86real.h"

extern "C" uint8_t byte_1FDEA;
extern "C" uint8_t byte_1FE1C;
extern "C" subpixel_t word_1FDE4[];
extern "C" uint8_t byte_1FDE8[];
extern "C" uint8_t near *word_1FE4E;
extern "C" PlayfieldPoint point_1FE52;
extern "C" uint8_t byte_202B8[];
extern "C" uint8_t byte_202B9[];
extern "C" uint8_t byte_202C8[];
extern "C" uint8_t byte_202CA[];
extern "C" uint8_t near *word_205CA;
extern "C" uint8_t byte_205CC;
extern "C" uint8_t byte_205E0[];
extern "C" uint8_t near *word_207E0;
extern "C" uint8_t byte_20E92[];
extern "C" uint8_t hitbox_pid;
extern "C" uint8_t pid_PID_current;
extern "C" uint8_t pid_PID_so_attack;

extern "C" void far sub_B39E(void);
extern "C" void pascal far sub_A3A8(uint8_t pid);
extern "C" void pascal far SUB_CDBD(
	subpixel_t x, subpixel_t y, uint16_t pid
);
extern "C" void pascal far SUB_CE0C(subpixel_t x, subpixel_t y, uint16_t pid);

extern "C" void far sub_142D0(void)
{
	byte_1FDEA = 0;
	byte_1FE1C = 0;
}

extern "C" void pascal far chargeshot_add_marisa(
	Subpixel center_x, Subpixel center_y
)
{
	register int i;

	word_1FE4E = (&byte_1FDEA + (pid.current * 0x32));
	word_1FE4E[0] = 1;
	word_1FE4E[1] = 0;
	players[pid.current].shot_active = SA_DISABLED;
	i = 0;
	while(i < 12) {
		reinterpret_cast<Subpixel near *>(word_1FE4E + 2)[i] = center_x;
		reinterpret_cast<Subpixel near *>(word_1FE4E + 0x1A)[i].v = (
			center_y.v - 0x100
		);
		i++;
	}
	snd_se_play(6);
}

extern "C" void pascal far marisa_hyper_14340(void)
{
	register int i;

	word_1FE4E = (&byte_1FDEA + (pid.current * 0x32));
	if(word_1FE4E[0] == 0) {
		word_1FE4E[0] = 1;
		i = 0;
		while(i < 12) {
			reinterpret_cast<Subpixel near *>(word_1FE4E + 2)[i] = (
				players[pid.current].center.x
			);
			reinterpret_cast<Subpixel near *>(word_1FE4E + 0x1A)[i].v = (
				players[pid.current].center.y.v - 0x100
			);
			i++;
		}
		snd_se_play(6);
	}
	word_1FE4E[1] = 0x10;
	players[pid.current].shot_active = SA_DISABLED;
}

extern "C" void pascal far chargeshot_update_marisa(void)
{
	register int i;

	word_1FE4E = (&byte_1FDEA + (pid_current * 0x32));
	if(word_1FE4E[0] != 0) {
		players[pid_current].gauge_charged = 0;
		word_1FE4E[1]++;

		i = 11;
		while(i > 0) {
			reinterpret_cast<Subpixel near *>(word_1FE4E + 2)[i] = (
				reinterpret_cast<Subpixel near *>(word_1FE4E + 2)[i - 1]
			);
			reinterpret_cast<Subpixel near *>(word_1FE4E + 0x1A)[i].v = (
				reinterpret_cast<Subpixel near *>(word_1FE4E + 0x1A)[i - 1].v + 0x18
			);
			i--;
		}

		if(word_1FE4E[1] < 0x18) {
			reinterpret_cast<Subpixel near *>(word_1FE4E + 2)[0] = (
				players[pid_current].center.x
			);
			reinterpret_cast<Subpixel near *>(word_1FE4E + 0x1A)[0].v = (
				players[pid_current].center.y.v - 0x100
			);
			return;
		}

		if(word_1FE4E[1] > 0x24) {
			players[pid_current].shot_active = SA_ENABLED;
			word_1FE4E[0] = 0;
		}
	}
}

uint8_t far chargeshot_hittest_marisa(void)
{
	uint8_t ret;
	register int i;

	word_1FE4E = (&byte_1FDEA + (hitbox.pid * 0x32));
	if(word_1FE4E[0] == 0) {
		return 0;
	}

	ret = 0;
	i = 0;
	while(i < 12) {
		if(
			(reinterpret_cast<Subpixel near *>(word_1FE4E + 2)[i].v >= hitbox.origin.topleft.x.v) &&
			(reinterpret_cast<Subpixel near *>(word_1FE4E + 2)[i].v <= hitbox.right.v) &&
			(reinterpret_cast<Subpixel near *>(word_1FE4E + 0x1A)[i].v >= hitbox.origin.topleft.y.v)
		) {
			hitcircles_enemy_add(
				reinterpret_cast<Subpixel near *>(word_1FE4E + 2)[i].v,
				(hitbox.origin.topleft.y.v + hitbox.radius.y.v),
				hitbox.pid
			);
			ret = 1;
			break;
		}
		i += 4;
	}
	return ret;
}

extern "C" void pascal far chargeshot_render_marisa(void)
{
	screen_y_t top;
	screen_x_t last_left;
	register int i;
	register screen_x_t left;

	word_1FE4E = (&byte_1FDEA + (pid_current * 0x32));
	if(word_1FE4E[0] == 0) {
		return;
	}

	egc_off();
	grcg_settile_1line(GC_RMW, 0xAA0055AAL);
	last_left = 0;
	i = 11;
	while(i > 6) {
		left = playfield_fg_x_to_screen(
			reinterpret_cast<Subpixel near *>(word_1FE4E + 2)[i].v,
			pid_current
		);
		top = (
			(reinterpret_cast<Subpixel near *>(word_1FE4E + 0x1A)[i].v >> 5) + 8
		);
		if(last_left != left) {
			grcg_boxfill((left - 2), 8, (left + 1), top);
		}
		last_left = left;
		i--;
	}

	grcg_settile_1line(GC_RMW, 0xFF00AA55L);
	last_left = 0;
	i = 6;
	while(i > 3) {
		left = playfield_fg_x_to_screen(
			reinterpret_cast<Subpixel near *>(word_1FE4E + 2)[i].v,
			pid_current
		);
		top = (
			(reinterpret_cast<Subpixel near *>(word_1FE4E + 0x1A)[i].v >> 5) + 8
		);
		if(last_left != left) {
			grcg_boxfill((left - 2), 8, (left + 1), top);
		}
		last_left = left;
		i--;
	}

	grcg_settile_1line(GC_RMW, 0xFF55FF55L);
	last_left = 0;
	i = 3;
	while(i > 0) {
		left = playfield_fg_x_to_screen(
			reinterpret_cast<Subpixel near *>(word_1FE4E + 2)[i].v,
			pid_current
		);
		top = (
			(reinterpret_cast<Subpixel near *>(word_1FE4E + 0x1A)[i].v >> 5) + 8
		);
		if(last_left != left) {
			grcg_boxfill((left - 2), 8, (left + 1), top);
		}
		last_left = left;
		i--;
	}

	grcg_setcolor(GC_RMW, V_WHITE);
	left = playfield_fg_x_to_screen(
		reinterpret_cast<Subpixel near *>(word_1FE4E + 2)[0].v,
		pid_current
	);
	top = (
		(reinterpret_cast<Subpixel near *>(word_1FE4E + 0x1A)[0].v >> 5) + 8
	);
	grcg_boxfill((left - 2), 8, (left + 1), top);
	grcg_off();
	egc_on();
}

#define bullets_add_nopcall() { \
	_asm { nop; push cs; call near ptr bullets_add; } \
}

void pascal near gauge_pattern_marisa(uint8_t type)
{
	uint8_t pid_other;
	uint8_t flag_expected;
	register int i;

	flag_expected = GBAF_GAUGE_PELLET_INIT;
	if(type == BT_BULLET16_DEFAULT) {
		_AL = flag_expected;
		_AL += GBAF_PELLET_TO_BULLET;
		flag_expected = _AL;
	}

	if(gba_flag_active[pid_current] == flag_expected) {
		word_1FDE4[pid_current] = (-randring2_next16_and(0x1F) << 4);
		byte_1FDE8[pid_current] = 0;
		gba_flag_active[pid_current]++;
		byte_202B8[pid_current * 4] = (0x30 - gba_gauge_level[pid_current]);
		return;
	}

	if(gba_flag_active[pid_current] != (flag_expected + 1)) {
		return;
	}

	bullet_template.type = static_cast<bullet_type_t>(type);
	bullet_template.center.y.v = word_1FDE4[pid_current];
	pid_other = (1 - pid_current);
	bullet_template.pid = pid_other;
	bullet_template.speed.v = ((3 << 4) + 8);
	bullet_template.group = BG_1;
	if((byte_1FDE8[pid_current] % 0x18) == 8) {
		SUB_CE0C(0, word_1FDE4[pid_current], static_cast<uint16_t>(pid_other));
	} else if((byte_1FDE8[pid_current] % 0x18) == 0x14) {
		SUB_CE0C(
			(PLAYFIELD_W << 4),
			word_1FDE4[pid_current],
			static_cast<uint16_t>(pid_other)
		);
	} else if((byte_1FDE8[pid_current] % 0x18) == 0) {
		bullet_template.angle = 0x20;
		bullet_template.center.x.v = 0;
		i = 0;
		while(i < 5) {
			bullets_add_nopcall();
			bullet_template.speed.v += -6;
			i++;
		}
		word_1FDE4[pid_current] += (byte_202B8[pid_current * 4] << 4);
	} else if((byte_1FDE8[pid_current] % 0x18) == 0x0C) {
		bullet_template.angle = 0x60;
		bullet_template.center.x.v = (PLAYFIELD_W << 4);
		i = 0;
		while(i < 5) {
			bullets_add_nopcall();
			bullet_template.speed.v += -6;
			i++;
		}
		word_1FDE4[pid_current] += (byte_202B8[pid_current * 4] << 4);
	}

	if(word_1FDE4[pid_current] >= (PLAYFIELD_H << 4)) {
		gba_flag_active[pid_current] = GBAF_NONE;
		sub_A3A8(pid_other);
	}
	byte_1FDE8[pid_current]++;
}

extern "C" void pascal far gba_gauge_pattern_pellet_marisa(void)
{
	if(gba_flag_active[pid_current] != GBAF_NONE) {
		gauge_pattern_marisa(BT_PELLET);
	}
}

extern "C" void pascal far gba_gauge_pattern_bullet_marisa(void)
{
	if(gba_flag_active[pid_current] != GBAF_NONE) {
		gauge_pattern_marisa(BT_BULLET16_DEFAULT);
	}
}

#undef bullets_add_nopcall

#pragma warn -aus
extern "C" void pascal far marisa_bomb(void)
{
	uint8_t frame;
	uint8_t col;
	register screen_x_t left;
	register sprite16_offset_t so;

	if(bomb_flag[pid_current] == BF_INACTIVE) {
		return;
	}
	egc_off();
	frame = bomb_frame[pid_current];
	if(frame < 64) {
		grcg_setcolor(GC_RMW, pid_current);
		_BX = FP_OFF(byte_20E92);
		if(pid_current != 0) {
			_BX += 0x28;
		}
		sub_B39E();
		grcg_off();
		_AL = frame;
		_AL <<= 2;
		_DL = 0;
		goto palette_fade;
	}

	if(frame < 128) {
		if((frame & 1) != 0) {
			snd_se_play(16);
		}
		if((frame & 3) < 2) {
			playfield_fg_shift_x[pid_current] = 4;
			left = 0x10;
		} else {
			playfield_fg_shift_x[pid_current] = -4;
			left = 8;
		}
		if(pid_current != 0) {
			left += PLAYFIELD_W_BORDERED;
		}
		mrs_put_noalpha_8(
			left, PLAYFIELD_TOP, (pid_current + 2), (_AX = pid_current)
		);
		if((frame % 8) == 0) {
			point_1FE52.x.v = randring2_next16_mod(PLAYFIELD_W << 4);
			point_1FE52.y.v = randring2_next16_mod(PLAYFIELD_H << 4);
			_asm {
				push	word ptr point_1FE52
				push	ax
				mov	al, pid_current
				mov	ah, 0
				push	ax
				call	far ptr SUB_CDBD
			}
			point_1FE52.x.v = playfield_fg_x_to_screen(
				point_1FE52.x.v, pid_current
			);
			point_1FE52.y.v = ((point_1FE52.y.v >> 4) + 16);
		}
	} else {
		playfield_fg_shift_x[pid_current] = 0;
		_AL = frame;
		_AL <<= 3;
		_DL = 255;

palette_fade:
		_DL -= _AL;
		col = _DL;
		Palettes[pid_current].c.r = _DL;
		Palettes[pid_current].c.g = _DL;
		Palettes[pid_current].c.b = _DL;
		palette_changed = true;
	}

	egc_on();
	if(frame < 64) {
		return;
	}
	if(frame >= 128) {
		return;
	}

	sprite16_put_size.w.v = (48 / 16);
	sprite16_put_size.h = 24;
	sprite16_clip.left = PLAYFIELD1_CLIP_LEFT;
	sprite16_clip.right = PLAYFIELD2_CLIP_RIGHT;
	so = (pid_PID_so_attack + ((48 * ROW_SIZE) + (176 / BYTE_DOTS)));
	if((frame & 3) < 2) {
		so += 6;
	}
	sprite16_put((point_1FE52.x.v - 24), (point_1FE52.y.v - 24), so);
}
#pragma warn .aus

extern "C" void far sub_14A76(void)
{
	word_205CA = byte_202CA;
	_AX = 0;
	goto clear_loop_check;

clear_loop:
	{
		word_205CA[0] = 0;
		_AX++;
		word_205CA += 0x20;
	}

clear_loop_check:
	asm { cmp ax, 18h; }
	asm { jl clear_loop; }
}

extern "C" void pascal far chargeshot_add_reimu(
	Subpixel center_x, Subpixel center_y
)
{
	uint8_t angle;

	word_205CA = (byte_202CA + (pid_PID_current * 384));
	angle = 0x90;
	_CX = 0;
	goto group_loop_check;

group_loop:
	{
		word_205CA[0] = 1;
		word_205CA[1] = 0;
		_DX = 0;

		goto node_loop_check;
	node_loop:
		{
			reinterpret_cast<Subpixel near *>(word_205CA + 2)[_DX] = center_x;
			reinterpret_cast<Subpixel near *>(word_205CA + 0x10)[_DX] = (
				center_y
			);
			_DX++;
		}
	node_loop_check:
		asm { cmp dx, 7; }
		asm { jl node_loop; }

		word_205CA[0x1E] = angle;
		angle += 0x20;
		word_205CA[0x1F] = 0x60;
		_CX++;
		word_205CA += 0x20;
	}

group_loop_check:
	asm { cmp cx, 4; }
	asm { jl group_loop; }

	byte_205CC = 1;
}

extern "C" void pascal far sub_14B0A(Subpixel center_x, Subpixel center_y)
{
	word_205CA = (byte_202CA + (pid_PID_current * 384));
	_CX = 0;
	goto group_loop_check;

group_loop:
	{
		if(word_205CA[0] == 0) {
			_asm {
				mov	bx, word_205CA
				mov	byte ptr [bx], 2
				mov	byte ptr [bx+1], 0
			}
			_DX = 0;

			goto node_loop_check;
		node_loop:
			{
				reinterpret_cast<Subpixel near *>(word_205CA + 2)[_DX] = (
					center_x
				);
				reinterpret_cast<Subpixel near *>(word_205CA + 0x10)[_DX] = (
					center_y
				);
				_DX++;
			}
		node_loop_check:
			asm { cmp dx, 7; }
			asm { jl node_loop; }

			word_205CA[0x1E] = 0xC0;
			word_205CA[0x1F] = 0xC0;
			goto done;
		}
		_CX++;
		word_205CA += 0x20;
	}

group_loop_check:
	asm { cmp cx, 0Ch; }
	asm { jl group_loop; }

done:
	byte_205CC = 0;
}

extern "C" void pascal far chargeshot_update_reimu(void)
{
	int vector_x;
	int vector_y;

	word_205CA = (byte_202CA + (pid_current * 384));
	playfield_clip_negative_radius.x.v = TO_SP(-24);
	playfield_clip_negative_radius.y.v = TO_SP(-24);
	_DI = 0;
	goto reimu_update_group_loop_check;

reimu_update_group_loop:
	{
		if(word_205CA[0] != 0) {
			players[pid_current].gauge_charged = 0;
			_SI = 6;

			goto reimu_update_shift_loop_check;
		reimu_update_shift_loop:
			{
				asm {
					lea	bx, [si-1]
					db	03h, 0DBh
					add	bx, word_205CA
					mov	ax, [bx+2]
					db	08Bh, 0DEh
					db	03h, 0DBh
					add	bx, word_205CA
					mov	[bx+2], ax
					lea	bx, [si-1]
					db	03h, 0DBh
					add	bx, word_205CA
					mov	ax, [bx+10h]
					db	08Bh, 0DEh
					db	03h, 0DBh
					add	bx, word_205CA
					mov	[bx+10h], ax
					dec	si
				}
			}
		reimu_update_shift_loop_check:
			__emit__(0x0B, 0xF6);
			asm { jg reimu_update_shift_loop; }

			vector2(vector_x, vector_y, word_205CA[0x1E], word_205CA[0x1F]);
			reinterpret_cast<Subpixel near *>(word_205CA + 2)[0].v += vector_x;
			reinterpret_cast<Subpixel near *>(word_205CA + 0x10)[0].v += (
				vector_y
			);

			if(word_205CA[0] == 1) {
				if(word_205CA[0x1F] > 0) {
					_AL = word_205CA[0x1F];
					_AL += -4;
					word_205CA[0x1F] = _AL;
					goto state_done;
				}
				if(word_205CA[1] >= 0x40) {
					word_205CA[0] = 2;
					asm { cmp di, 2; }
					asm { jge release_right; }
					_asm { mov byte ptr [bx+1Eh], 0B0h; }
					goto release_speed;
				release_right:
					word_205CA[0x1E] = 0xD0;
				release_speed:
					word_205CA[0x1F] = 0xE0;
				}
			}

		state_done:
			word_205CA[1]++;
			if(playfield_clip(
				reinterpret_cast<Subpixel near *>(word_205CA + 0x0E)[0],
				reinterpret_cast<Subpixel near *>(word_205CA + 0x1C)[0]
			)) {
				word_205CA[0] = 0;
			}
		}
		_DI++;
		word_205CA += 0x20;
	}

reimu_update_group_loop_check:
	if(static_cast<int>(_DI) < 0x0C) {
		goto reimu_update_group_loop;
	}
}

extern "C" void pascal near reimu_chargeshot_14C8C(
	Subpixel center_x, Subpixel top, int phase
)
{
	sprite16_offset_t sprite_offset;
	register int phase_reg;
	register subpixel_t left;

	left = center_x.v;
	phase_reg = phase;
	phase_reg += round_or_result_frame;
	phase_reg &= 3;
	sprite_offset = ((phase_reg * 6) + pid_PID_so_attack + (56 * ROW_SIZE));
	left = (playfield_fg_x_to_screen(left, pid_current) - 24);
	_AX = top.v;
	asm { sar ax, SUBPIXEL_BITS; }
	_AX += -8;
	top.v = _AX;
	sprite16_put(left, _AX, sprite_offset);
}

uint8_t far chargeshot_hittest_reimu(void)
{
	register int i;

	if(byte_205CC != 0) {
		if(round_or_result_frame & 1) {
			goto miss;
		}
	}
	word_205CA = (byte_202CA + (hitbox.pid * 384));
	i = 0;
	goto loop_check;

loop:
	if(word_205CA[0] == 0) {
		goto next;
	}
	if(
		(reinterpret_cast<Subpixel near *>(word_205CA + 2)[0].v <= TO_SP(-16)) ||
		(
			reinterpret_cast<Subpixel near *>(word_205CA + 2)[0].v >=
			TO_SP(PLAYFIELD_W)
		) ||
		(
			reinterpret_cast<Subpixel near *>(word_205CA + 0x10)[0].v <=
			TO_SP(-16)
		)
	) {
		goto next;
	}
	if(
		(
			(reinterpret_cast<Subpixel near *>(word_205CA + 2)[0].v -
				hitbox.right.v) > TO_SP(20)
		) ||
		(
			(hitbox.origin.topleft.x.v -
				reinterpret_cast<Subpixel near *>(word_205CA + 2)[0].v) >
			TO_SP(20)
		) ||
		(
			(reinterpret_cast<Subpixel near *>(word_205CA + 0x10)[0].v -
				hitbox.bottom.v) > TO_SP(20)
		) ||
		(
			(hitbox.origin.topleft.y.v -
				reinterpret_cast<Subpixel near *>(word_205CA + 0x10)[0].v) >
			TO_SP(20)
		)
	) {
		goto next;
	}
	_asm {
		push	word ptr [bx+2]
		push	word ptr [bx+10h]
		mov	al, hitbox_pid
		mov	ah, 0
		push	ax
		call	far ptr hitcircles_enemy_add
	}
	return 1;

next:
	i++;
	word_205CA += 0x20;

loop_check:
	if(i < 6) {
		goto loop;
	}

miss:
	return 0;
}

extern "C" void pascal far chargeshot_render_reimu(void)
{
	register int i;

	word_205CA = (byte_202CA + (pid_current * 384));
	if(pid_current == 0) {
		sprite16_clip.left = PLAYFIELD1_CLIP_LEFT;
		sprite16_clip.right = PLAYFIELD1_CLIP_RIGHT;
	} else {
		sprite16_clip.left = PLAYFIELD2_CLIP_LEFT;
		sprite16_clip.right = PLAYFIELD2_CLIP_RIGHT;
	}
	sprite16_put_size.w.v = (48 / 16);
	sprite16_put_size.h = 24;

	i = 0;
	while(i < 0x0C) {
		if(word_205CA[0] != 0) {
			sprite16_mono_(true);
			sprite16_mono_color_(2);
			_asm {
				mov	bx, word_205CA
				push	word ptr [bx+0Eh]
				push	word ptr [bx+1Ch]
				push	3
				call	near ptr reimu_chargeshot_14C8C
			}
			sprite16_mono_color_(5);
			reimu_chargeshot_14C8C(
				reinterpret_cast<Subpixel near *>(word_205CA + 0x02)[4],
				reinterpret_cast<Subpixel near *>(word_205CA + 0x10)[4],
				2
			);
			sprite16_mono_color_(6);
			reimu_chargeshot_14C8C(
				reinterpret_cast<Subpixel near *>(word_205CA + 0x02)[2],
				reinterpret_cast<Subpixel near *>(word_205CA + 0x10)[2],
				1
			);
			__emit__(0x31, 0xD2); // XOR DX, DX
			_AH = SPRITE16_SET_MONO;
			geninterrupt(SPRITE16);
			reimu_chargeshot_14C8C(
				reinterpret_cast<Subpixel near *>(word_205CA + 0x02)[0],
				reinterpret_cast<Subpixel near *>(word_205CA + 0x10)[0],
				0
			);
		}
		i++;
		word_205CA += 0x20;
	}
}

#define bullets_add_nopcall() { \
	_asm { nop; push cs; call near ptr bullets_add; } \
}

void pascal near gauge_pattern_reimu(uint8_t type)
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
		byte_202C8[pid_current] = 0;
		gba_flag_active[pid_current]++;
		byte_202B8[pid_current * 4] = (gba_gauge_level[pid_current] + 0x20);
		byte_202B9[pid_current * 4] = (0x20 - gba_gauge_level[pid_current]);
		return;
	}

	if(gba_flag_active[pid_current] != (flag_expected + 1)) {
		return;
	}

	bullet_template.type = static_cast<bullet_type_t>(type);
	bullet_template.center.y.v = (8 << 4);
	pid_other = (1 - pid_current);
	bullet_template.pid = pid_other;
	bullet_template.speed.v = (2 << 4);
	bullet_template.angle = 0;

	if((byte_202C8[pid_current] % byte_202B9[pid_current * 4]) == 0) {
		bullet_template.group = BG_5_SPREAD_MEDIUM_AIMED;
		bullet_template.center.x.v = (32 << 4);
		SUB_CE0C((32 << 4), (8 << 4), static_cast<uint16_t>(pid_other));
		bullets_add_nopcall();

		bullet_template.center.x.v = (256 << 4);
		SUB_CE0C((256 << 4), (8 << 4), static_cast<uint16_t>(pid_other));
		bullets_add_nopcall();
	}

	if((byte_202C8[pid_current] % 0x20) == 0x10) {
		bullet_template.group = BG_RING;
		bullet_template.count = byte_202B8[pid_current * 4];
		bullet_template.speed.v = ((3 << 4) + 2);
		bullet_template.center.x.v = (64 << 4);
		SUB_CE0C((64 << 4), (8 << 4), static_cast<uint16_t>(pid_other));
		bullets_add_nopcall();

		bullet_template.center.x.v = ((PLAYFIELD_W - 64) << 4);
		SUB_CE0C(
			((PLAYFIELD_W - 64) << 4),
			(8 << 4),
			static_cast<uint16_t>(pid_other)
		);
		bullets_add_nopcall();
	}

	if(byte_202C8[pid_current] > 0x40) {
		gba_flag_active[pid_current] = GBAF_NONE;
		sub_A3A8(pid_other);
	}
	byte_202C8[pid_current]++;
}

extern "C" void pascal far gba_gauge_pattern_pellet_reimu(void)
{
	if(gba_flag_active[pid_current] != GBAF_NONE) {
		gauge_pattern_reimu(BT_PELLET);
	}
}

extern "C" void pascal far gba_gauge_pattern_bullet_reimu(void)
{
	if(gba_flag_active[pid_current] != GBAF_NONE) {
		gauge_pattern_reimu(BT_BULLET16_DEFAULT);
	}
}

extern "C" void pascal far sub_1501E(void)
{
	if(bomb_flag[pid_current] == BF_INACTIVE) {
		return;
	}
	if(bomb_flag[pid_current] == BF_PREPARING) {
		bomb_flag[pid_current] = BF_ACTIVE;
		bomb_frame[pid_current] = 0;
		snd_se_play(18);
	}

	bomb_frame[pid_current]++;
	if(bomb_frame[pid_current] < BOMB_FRAMES) {
		return;
	}
	bomb_flag[pid_current] = BF_INACTIVE;
	sub_A3A8(pid_current);
}

extern "C" void pascal far reimu_1508C(void)
{
	register int i;

	if(bomb_flag[pid_current] == BF_INACTIVE) {
		return;
	}
	if(bomb_flag[pid_current] == BF_PREPARING) {
		bomb_flag[pid_current] = BF_ACTIVE;
		bomb_frame[pid_current] = 0;
		i = 0;
		while(i < 0x20) {
			byte_205E0[(pid_current << 8) + (i << 3)] = 0;
			i++;
		}
		snd_se_play(18);
	}

	bomb_frame[pid_current]++;
	word_207E0 = (byte_205E0 + (pid_current << 8));
	i = 0;
	while(i < 0x20) {
		if(word_207E0[0] != 0) {
			_asm {
				mov	bx, word_207E0
				mov	ax, [bx+6]
				add	[bx+4], ax
				cmp	word ptr [bx+4], -128
				jg	short record_update_done
				mov	word ptr [bx+4], (384 shl 4)
				add	word ptr [bx+6], 8
			}
record_update_done:
		}
		i++;
		word_207E0 += 8;
	}

	if(bomb_frame[pid_current] < BOMB_FRAMES) {
		return;
	}
	bomb_flag[pid_current] = BF_INACTIVE;
	sub_A3A8(pid_current);
}

extern "C" void pascal near reimu_bomb_1515D(void)
{
	register int i;
	screen_x_t left;
	screen_y_t top;
	sprite16_offset_t sprite_offset;

	word_207E0 = (byte_205E0 + (pid_current << 8));
	sprite16_put_size.w.v = (16 / 16);
	sprite16_put_size.h = 8;
	sprite16_clip.left = PLAYFIELD1_CLIP_LEFT;
	sprite16_clip.right = PLAYFIELD2_CLIP_RIGHT;
	sprite_offset = (pid_PID_so_attack + 0x18);

	i = 0;
	while(i < 0x20) {
		if(word_207E0[0] != 0) {
			_BX = reinterpret_cast<uint16_t>(word_207E0);
			left = (playfield_fg_x_to_screen(
				reinterpret_cast<Subpixel near *>(_BX + 2)[0].v,
				pid_current
			) - 8);

			_AX = reinterpret_cast<Subpixel near *>(word_207E0 + 4)[0].v;
			asm { sar ax, SUBPIXEL_BITS; }
			_AX += 8;
			top = _AX;
			sprite16_put(left, _AX, sprite_offset);
		}
		i++;
		word_207E0 += 8;
	}
}

extern "C" void pascal far reimu_bomb(void)
{
	uint8_t frame;
	uint8_t col;

	if(bomb_flag[pid_current] == BF_INACTIVE) {
		return;
	}
	egc_off();
	frame = bomb_frame[pid_current];
	if(frame < 64) {
		grcg_setcolor(GC_RMW, pid_current);
		_BX = FP_OFF(byte_20E92);
		if(pid_current != 0) {
			_BX += 0x28;
		}
		sub_B39E();
		grcg_off();
		if(frame < 32) {
			word_207E0 = (
				byte_205E0 + (pid_current << 8) + (frame << 3)
			);
			word_207E0[0] = 1;
			_AX = randring2_next16_and(0x7F);
			_asm {
				mov	dx, 0FFA0h
				db	02Bh, 0D0h
				mov	bx, word_207E0
				mov	[bx+6], dx
			}
			_AX = randring2_next16_mod(PLAYFIELD_W << 4);
			_asm {
				mov	bx, word_207E0
				mov	[bx+2], ax
				mov	word ptr [bx+4], (384 shl 4)
			}
		}
		Palettes[pid_current].c.r = (frame << 2);
		Palettes[pid_current].c.g = (frame * 3);
		Palettes[pid_current].c.b = (frame * 3);
		palette_changed = true;
	} else if(frame < 112) {
		if((frame & 1) != 0) {
			snd_se_play(16);
		}
		if(frame <= 96) {
			PaletteTone = (100 + ((frame & 1) * 80));
			palette_changed = true;
			mrs_put_noalpha_8(
				((pid_current * PLAYFIELD_W_BORDERED) + PLAYFIELD_LEFT),
				PLAYFIELD_TOP,
				(pid_current + 2),
				(_AX = pid_current)
			);
		} else {
			PaletteTone = 100;
			palette_changed = true;
		}
		col = (frame % 8);
		if((col & 3) < 2) {
			playfield_fg_shift_x[pid_current] = 4;
		} else {
			playfield_fg_shift_x[pid_current] = -4;
		}
	} else {
		playfield_fg_shift_x[pid_current] = 0;
		_AL = frame;
		_AL <<= 3;
		_DL = 255;
		_DL -= _AL;
		col = _DL;
		Palettes[pid_current].c.r = _DL;
		Palettes[pid_current].c.g = _DL;
		Palettes[pid_current].c.b = _DL;
		palette_changed = true;
	}

	egc_on();
	if(frame < 96) {
		reimu_bomb_1515D();
	}
}

#undef bullets_add_nopcall
