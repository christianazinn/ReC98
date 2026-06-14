#pragma option -zCMAIN_010_TEXT -zPmain_01 -G

#include "th03/main/player/bomb.hpp"
#include "codegen.hpp"
#include "decomp.hpp"
#include "th02/hardware/pages.hpp"
#include "th03/main/defeat.hpp"
#include "th03/main/hitcirc.hpp"
#include "th03/main/hud/warning.hpp"
#include "th03/main/player/combo.hpp"
#include "th03/main/player/cur.hpp"
#include "th03/main/player/gba.hpp"
#include "th03/main/player/stuff.hpp"
#include "th03/main/hud/static.hpp"
#include "th03/main/round.hpp"
#include "th03/main/sprite16.hpp"
#include "th03/math/randring.hpp"
#include "th03/math/vector.hpp"
#include "th02/snd/snd.h"
#include "libs/master.lib/pc98_gfx.hpp"
#include "x86real.h"

void near story_skill_decrement(void);
extern "C" void pascal near sub_C248(player_stuff_t near *player);
extern "C" void pascal near sub_C0D8(player_stuff_t near *player);
extern "C" void pascal near sub_C54A(player_stuff_t near *player);
extern "C" void pascal far sub_CDBD(void);
extern "C" void pascal far marisa_hyper_14340(void);
extern "C" void pascal far ellen_hyper_1B6CA(subpixel_t x, subpixel_t y);
extern "C" void pascal far rikako_1C497(subpixel_t x, subpixel_t y);
extern "C" void pascal far rikako_hyper_1C4B4(void);
extern "C" unsigned char byte_20E3D;
extern "C" signed char byte_20E48;
extern "C" unsigned char byte_220FC[PLAYER_COUNT];
extern "C" unsigned char pid_PID_current;
extern speed_t player_speed_base;
extern SPPoint8 player_velocity;

enum move_ret_t {
	MOVE_INVALID = 0,
	MOVE_VALID = 1,
	MOVE_NOINPUT = 2,
};

void pascal near player_pos_update_and_clamp(PlayfieldPoint near& center);
move_ret_t pascal near player_move(input_t input);

extern "C" void pascal near hyper_standby(void)
{
	player_cur->shot_mode = SM_1_PAIR;
	if(player_cur->hyper_active != 0) {
		player_cur->hyper = player_cur->hyper_func;
	}
}

extern "C" void pascal near hyper_reimu(void)
{
	if(player_cur->hyper_active == 0) {
		player_cur->hyper = hyper_standby;
		return;
	}
	player_cur->shot_mode = SM_REIMU_HYPER;
	player_speed_base.aligned.x.v += TO_SP(2);
	player_speed_base.aligned.y.v += TO_SP(2);
	player_speed_base.diagonal.x.v += (TO_SP(1) + 8);
	player_speed_base.diagonal.y.v += (TO_SP(1) + 8);
}

extern "C" void pascal near hyper_mima(void)
{
	if(player_cur->hyper_active == 0) {
		player_cur->hyper = hyper_standby;
		return;
	}
	player_cur->shot_mode = SM_2_PAIRS;
	player_cur->shot_active = SA_BLOCKED_FOR_THIS_FRAME;
	player_speed_base.aligned.x.v += TO_SP(2);
	player_speed_base.aligned.y.v += TO_SP(2);
	player_speed_base.diagonal.x.v += (TO_SP(1) + 8);
	player_speed_base.diagonal.y.v += (TO_SP(1) + 8);
}

extern "C" void pascal near hyper_marisa(void)
{
	if(player_cur->hyper_active == 0) {
		player_cur->hyper = hyper_standby;
		return;
	}
	player_cur->shot_mode = SM_NONE;
	marisa_hyper_14340();
	player_speed_base.aligned.x.v += TO_SP(-1);
	player_speed_base.aligned.y.v += TO_SP(-1);
	player_speed_base.diagonal.x.v += -0x0C;
	player_speed_base.diagonal.y.v += -0x0C;
}

extern "C" void pascal near hyper_ellen(void)
{
	if(player_cur->hyper_active == 0) {
		player_cur->hyper = hyper_standby;
		return;
	}
	player_cur->shot_mode = SM_1_PAIR;
	if((round_frame & 3) == 0) {
		ellen_hyper_1B6CA(player_cur->center.x, player_cur->center.y);
	}
}

extern "C" void pascal near hyper_kotohime(void)
{
	if(player_cur->hyper_active == 0) {
		player_cur->hyper = hyper_standby;
		return;
	}
	player_cur->shot_mode = SM_4_PAIRS;
	player_speed_base.aligned.x.v += TO_SP(2);
	player_speed_base.aligned.y.v += TO_SP(2);
	player_speed_base.diagonal.x.v += (TO_SP(1) + 8);
	player_speed_base.diagonal.y.v += (TO_SP(1) + 8);
}

extern "C" void pascal near hyper_chiyuri(void)
{
	if(player_cur->hyper_active == 0) {
		player_cur->hyper = hyper_standby;
		return;
	}
	player_cur->shot_mode = SM_4_PAIRS;
	player_speed_base.aligned.x.v += TO_SP(2);
	player_speed_base.aligned.y.v += TO_SP(2);
	player_speed_base.diagonal.x.v += (TO_SP(1) + 8);
	player_speed_base.diagonal.y.v += (TO_SP(1) + 8);
}

extern "C" void pascal near hyper_yumemi(void)
{
	if(player_cur->hyper_active == 0) {
		player_cur->hyper = hyper_standby;
		return;
	}
	player_cur->shot_mode = SM_4_PAIRS;
	player_speed_base.aligned.x.v += TO_SP(2);
	player_speed_base.aligned.y.v += TO_SP(2);
	player_speed_base.diagonal.x.v += (TO_SP(1) + 8);
	player_speed_base.diagonal.y.v += (TO_SP(1) + 8);
}

extern "C" void pascal near hyper_kana(void)
{
	if(player_cur->hyper_active == 0) {
		player_cur->hyper = hyper_standby;
		return;
	}
	player_cur->shot_mode = SM_4_PAIRS;
	player_speed_base.aligned.x.v += TO_SP(2);
	player_speed_base.aligned.y.v += TO_SP(2);
	player_speed_base.diagonal.x.v += (TO_SP(1) + 8);
	player_speed_base.diagonal.y.v += (TO_SP(1) + 8);
}

extern "C" void pascal near hyper_rikako(void)
{
	if(player_cur->hyper_active == 0) {
		player_cur->hyper = hyper_standby;
		rikako_hyper_1C4B4();
		return;
	}
	if(player_cur->gauge_avail > TO_SP(250)) {
		rikako_1C497(player_cur->center.x, player_cur->center.y);
	}
	player_speed_base.aligned.x.v += TO_SP(-1);
	player_speed_base.aligned.y.v += TO_SP(-1);
	player_speed_base.diagonal.x.v += -8;
	player_speed_base.diagonal.y.v += -8;
}

extern "C" void pascal near player_update(
	input_t input, player_stuff_t near *player
)
{
	int gauge_charged;
	input_t movement_input;
	unsigned char acted;
	unsigned char use_previous_input;
	register input_t live_input = input;
	register player_stuff_t near *p = player;

	player_cur = &players[pid.current];
	if(p->invincibility_time != 0) {
		p->invincibility_time--;
	}
	if(damage_all_on[pid.current] != 0) {
		damage_all_on[pid.current]--;
	}

	if(defeat_flag == DF_BANNER) {
		goto update_patnums;
	}
	if(p->lose_anim_time != 0) {
		goto update_lose_anim;
	}

	player_speed_base.aligned.x.v = p->speed_base.aligned.x.v;
	player_speed_base.aligned.y.v = p->speed_base.aligned.y.v;
	player_speed_base.diagonal.x.v = p->speed_base.diagonal.x.v;
	player_speed_base.diagonal.y.v = p->speed_base.diagonal.y.v;
	p->hyper();

	if(p->move_lock_time != 0) {
		goto move_locked;
	}

	if(!p->is_cpu) {
		movement_input = (live_input & INPUT_MOVEMENT);
		use_previous_input = true;
		do {
			acted = player_move(movement_input);
			if(acted == MOVE_INVALID) {
				if(use_previous_input != false) {
					if(p->human_movement_last != movement_input) {
						movement_input &= ~p->human_movement_last;
						use_previous_input = false;
						continue;
					}
					goto human_after_position_update;
				}
			}
			break;
		} while(1);
		if(acted == MOVE_VALID) {
			player_pos_update_and_clamp(p->center);
		}
human_after_position_update:
		if(use_previous_input != false) {
			p->human_movement_last = movement_input;
		}
		goto after_move;
	}

	p->cpu_frame++;
	sub_C54A(p);
	if(byte_20E48 == 0) {
		acted = 6;
	} else if(byte_20E48 == -1) {
		acted = 0x20;
	} else if(byte_20E48 == -2) {
		acted = 7;
	} else {
		acted = 10;
	}
	if((round_or_result_frame % acted) == 0) {
		byte_220FC[pid.current] = 4;
		live_input = INPUT_SHOT;
	}
	byte_20E48 = 0;

	if(p->is_hit) {
		goto cpu_hit;
	}
	if(p->bombs == 0) {
		if(static_cast<shalfhearts_t>(p->halfhearts) <= 3) {
			goto after_move;
		}
	}
	if(
		(p->cpu_charge_at_avail_ring[p->cpu_charge_at_avail_ring_p] <
			(p->gauge_avail >> 4)) ||
		(p->gauge_avail == GAUGE_MAX)
	) {
		byte_220FC[pid.current] = 0;
		live_input = INPUT_SHOT;
		if(p->gauge_charged >= p->gauge_avail) {
			byte_220FC[pid.current] = 4;
			live_input = INPUT_NONE;
			p->cpu_charge_at_avail_ring_p++;
			if(p->cpu_charge_at_avail_ring_p >= CHARGE_AT_AVAIL_RING_SIZE) {
				p->cpu_charge_at_avail_ring_p = 0;
			}
		}
	}
	goto after_move;

cpu_hit:
	p->is_hit = false;
	byte_220FC[pid.current] = 4;
	live_input = INPUT_NONE;
	goto after_move;

move_locked:
	p->move_lock_time--;

after_move:
	acted = false;
	if(p->knockback_active) {
		player_knockback_update(p);
		p->gauge_charged = 0;
		p->cpu_frame = 0;
		acted = true;
		goto update_shot_bomb_hyper;
	}

	if(byte_220FC[pid.current] <= 2) {
		if(p->hyper_active == 0) {
			if(p->gauge_charged < p->gauge_avail) {
				p->gauge_charged += p->gauge_charge_speed;
				if(p->gauge_charged > p->gauge_avail) {
					p->gauge_charged = p->gauge_avail;
					p->spell_ready_frames = 0;
				}
				if(p->gauge_charged == 0x180) {
					snd_se_play(9);
				}
				goto update_shot_bomb_hyper;
			}
			p->spell_ready_frames++;
			if(p->spell_ready_frames < SPELL_AUTOFIRE_FRAMES) {
				goto update_shot_bomb_hyper;
			}
			optimization_barrier();
			goto release_gauge;
		}
	}
	if(byte_220FC[pid.current] > 4) {
		goto update_shot_bomb_hyper;
	}

release_gauge:
	gauge_charged = p->gauge_charged;
	if(gauge_charged >= TO_SP(64)) {
		p->chargeshot_add(p->center.x, p->center.y);
		if(gauge_charged >= TO_SP(128)) {
			warning_flag[pid.current] = WF_PORTRAIT;
			if(gauge_charged >= GAUGE_MAX) {
				if(gba_boss_launched_by == pid.current) {
					goto gauge_attack_bullet;
				}
				gba_flag_next[pid.current] = GBAF_BOSS;
				p->gauge_avail = TO_SP(64);
				if(gba_boss_level < GBA_BOSS_LEVEL_MAX) {
					gba_boss_level++;
				}
				combo_points_for_boss_attack += 10240;
				if(gba_boss_launched_by == PID_NONE) {
					p->boss_attacks_fired++;
				} else {
					p->boss_attacks_reversed++;
				}
				goto gauge_released;
			}
			if(gauge_charged < TO_SP(192)) {
				goto gauge_attack_pellet;
			}

gauge_attack_bullet:
			p->gauge_attacks_fired++;
			gba_flag_next[pid.current] = GBAF_GAUGE_BULLET_INIT;
			p->gauge_avail -= TO_SP(128);
			if(gba_gauge_level[pid.current] < GBA_GAUGE_LEVEL_MAX) {
				goto gauge_level_inc;
			}
			goto gauge_released;

gauge_attack_pellet:
			p->gauge_attacks_fired++;
			gba_flag_next[pid.current] = GBAF_GAUGE_PELLET_INIT;
			p->gauge_avail -= TO_SP(64);
			if(gba_gauge_level[pid.current] >= GBA_GAUGE_LEVEL_MAX) {
				goto gauge_released;
			}

gauge_level_inc:
			gba_gauge_level[pid.current]++;
		}

gauge_released:
		p->invincibility_time += 6;
		acted = true;
	}
	p->gauge_charged = 0;

update_shot_bomb_hyper:
	if(p->shot_active == SA_BLOCKED_FOR_THIS_FRAME) {
		live_input = INPUT_NONE;
	}
	if((p->move_lock_time == 0) && (p->shot_active != SA_DISABLED)) {
		if((live_input & INPUT_SHOT) || (p->shot_active == SA_BLOCKED_FOR_THIS_FRAME)) {
			if(byte_220FC[pid.current] > 2) {
				shots_add();
				byte_220FC[pid.current] = 0;
				goto after_shot_gate;
			}
		}
		if((live_input & INPUT_SHOT) == 0) {
			if(byte_220FC[pid.current] < 8) {
				byte_220FC[pid.current]++;
			}
		}

after_shot_gate:
		if(live_input & INPUT_BOMB) {
			player_bomb(p);
		}
		if(p->shot_active == SA_BLOCKED_FOR_THIS_FRAME) {
			p->shot_active = SA_ENABLED;
		}
	}

	if(p->hyper_active != 0) {
		if(p->gauge_avail > TO_SP(64)) {
			p->gauge_avail -= 6;
		} else {
			p->hyper_active = 0;
			p->gauge_avail = TO_SP(64);
		}
		acted = true;
	}

	if(defeat_flag != DF_NONE) {
		goto update_patnums;
	}
	if(acted != false) {
		goto update_patnums;
	}
	player_hittest(8 / 2);
	if(!p->is_hit) {
		goto update_patnums;
	}
	if(p->halfhearts != 0) {
		__emit__(0xFF, 0x34); // push word ptr [si]
		__emit__(0xFF, 0x74, 0x02); // push word ptr [si+2]
		_AX = pid.current;
		_asm {
			push	ax
			nop
			push	cs
			call	near ptr hitcircles_player_add
		}
		_AL = players_hit_damage_update(*p);
		_DL = p->halfhearts;
		_DL -= _AL;
		p->halfhearts = _DL;
		if(static_cast<shalfhearts_t>(p->halfhearts) < 0) {
			p->halfhearts = 0;
		}
		if(p->halfhearts == 0) {
			p->lose_anim_time = 0x30;
			byte_20E3D = pid.current;
		} else {
			p->knockback_active = true;
			p->knockback_time = KNOCKBACK_FRAMES;
			p->invincibility_time = MISS_INVINCIBILITY_FRAMES;
			p->move_lock_time = KNOCKBACK_FRAMES;
		}
		_asm {
			push	word ptr pid_PID_current
			nop
			push	cs
			call	near ptr hud_static_halfhearts_put
		}
	}
	p->is_hit = false;
	goto update_patnums;

update_lose_anim:
	if(p->lose_anim_time != 0xFF) {
		sub_C0D8(p);
	}

update_patnums:
	if(player_velocity.x.v == 0) {
		p->patnum_movement = 0;
	} else if(player_velocity.x.v < 0) {
		p->patnum_movement = 1;
	} else {
		p->patnum_movement = 2;
	}
	p->patnum_glow = 0;
	if(page_back != 0) {
		if(p->hyper_active != 0) {
			p->patnum_glow = 2;
		} else if(p->invincibility_time != 0) {
			p->patnum_glow = 1;
		}
	}
}

extern "C" void pascal near player_render(player_stuff_t near *player)
{
	int left;
	int top;
	register player_stuff_t near *p = player;
	register sprite16_offset_t sprite_offset;

	if(p->lose_anim_time != 0) {
		if(p->lose_anim_time != 0xFF) {
			sub_C248(p);
		}
		return;
	}
	if(p->patnum_glow != 0) {
		return;
	}

	sprite16_clip.left = PLAYFIELD1_CLIP_LEFT;
	sprite16_clip.right = PLAYFIELD2_CLIP_RIGHT;
	sprite16_put_size.w.v = (32 / 16);
	sprite16_put_size.h = 32;
	left = (playfield_fg_x_to_screen(p->center.x, pid.current) - 16);
	top = ((p->center.y.v >> 4) - 16);

	sprite_offset = (
		(p->patnum_movement << 2) + ((128 * ROW_SIZE) + (544 / BYTE_DOTS))
	);
	if(pid.current != 0) {
		sprite_offset += (32 * ROW_SIZE);
	}
	sprite16_put(left, top, sprite_offset);
}

extern "C" void pascal near player_overlay_render(player_stuff_t near *player)
{
	int left;
	int top;
	register player_stuff_t near *p = player;
	register int patnum;

	if(p->lose_anim_time != 0) {
		return;
	}
	if(p->patnum_glow != 0) {
		left = (playfield_fg_x_to_screen(p->center.x, pid.current) - 16);
		top = ((p->center.y.v >> 5) - 8);

		patnum = p->patnum_glow;
		if(pid.current != 0) {
			patnum += 9;
		}
		patnum += (p->patnum_movement * 2);
		patnum += p->patnum_movement;
		super_put(left, top, patnum);
	}

	if(p->gauge_charged < 0x100) {
		return;
	}
	if(p->gauge_charged < TO_SP(64)) {
		patnum = 0x2C;
	} else if(p->gauge_charged < TO_SP(128)) {
		patnum = 0x30;
	} else if(p->gauge_charged < GAUGE_MAX) {
		patnum = 0x34;
	} else {
		patnum = 0x38;
	}

	left = (playfield_fg_x_to_screen(p->center.x, pid.current) - 16);
	top = ((p->center.y.v >> 5) - 15);
	patnum += ((round_or_result_frame >> 2) & 3);
	super_put(left, top, patnum);
}

extern "C" void pascal near player_knockback_update(
	player_stuff_t near *player
)
{
	int vector_x;
	int vector_y;
	register player_stuff_t near *p = player;

	if(p->knockback_time == KNOCKBACK_FRAMES) {
		snd_se_play(2);
		p->knockback_angle = randring1_next16_and(0x3F);
		if(p->center.x.v < TO_SP(144)) {
			if(p->center.y.v > TO_SP(184)) {
				p->knockback_angle = (p->knockback_angle + 192);
			}
		} else {
			if(p->center.y.v < TO_SP(184)) {
				p->knockback_angle = (p->knockback_angle + 0x40);
			} else {
				p->knockback_angle = (p->knockback_angle + 0x80);
			}
		}
		p->knockback_length = 0x40;
	}

	if(p->knockback_time > 0x20) {
		p->knockback_length -= 2;
		vector2(vector_x, vector_y, p->knockback_angle, p->knockback_length);
		player_velocity.x.v = vector_x;
		player_velocity.y.v = vector_y;
		player_pos_update_and_clamp(p->center);
	} else if(p->knockback_time == 0) {
		p->knockback_active = false;
	}
	p->knockback_time--;
}

extern "C" void pascal near player_bomb(player_stuff_t near *player)
{
	register player_stuff_t near *p = player;

	if(bomb_flag[pid.current] != BF_INACTIVE) {
		return;
	}
	if(p->hyper_active != 0) {
		return;
	}
	if(p->bombs != 0) {
		bomb_flag[pid.current] = BF_PREPARING;
		p->bombs--;
		p->invincibility_time = BOMB_FRAMES;
		_asm {
			push	word ptr pid_PID_current
			nop
			push	cs
			call	near ptr hud_static_bombs_put
		}
	} else {
		if(p->gauge_avail < GAUGE_MAX) {
			return;
		}
		damage_all_on[pid.current] = true;
		snd_se_play(7);
		p->hyper_active = p->playchar_paletted.v;

		__emit__(0xFF, 0x34); // push word ptr [si]
		__emit__(0xFF, 0x74, 0x02); // push word ptr [si+2]
		_AX = pid.current;
		_asm {
			push	ax
			nop
			push	cs
			call	near ptr sub_CDBD
		}
	}
	story_skill_decrement();
}
