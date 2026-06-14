#pragma option -zCMAIN_010_TEXT -zPmain_01 -G

#include "th03/main/player/bomb.hpp"
#include "codegen.hpp"
#include "th03/main/player/cur.hpp"
#include "th03/main/player/stuff.hpp"
#include "th03/main/hud/static.hpp"
#include "th03/main/round.hpp"
#include "th03/math/randring.hpp"
#include "th03/math/vector.hpp"
#include "th02/snd/snd.h"
#include "libs/master.lib/pc98_gfx.hpp"
#include "x86real.h"

void near story_skill_decrement(void);
extern "C" void pascal far sub_CDBD(void);
extern "C" unsigned char pid_PID_current;
extern SPPoint8 player_velocity;

void pascal near player_pos_update_and_clamp(PlayfieldPoint near& center);

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
