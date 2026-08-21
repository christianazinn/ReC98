/// Stage 2 Boss - Louise
/// ---------------------
/// louise_update() and the eight helpers it dispatches. It was the last
/// emitting proc of th05_main.asm's contribution to B4_UPDATE_TEXT once the
/// Stage 3 midboss moved out from under it -- the `dw offset loc_...` run
/// below its `endp` is the 8-entry table its `switch(boss.phase)` compiles to,
/// not data the dump owns -- so this file grows the C++ object that already
/// follows the root backwards into the hole (kb/codegen 0099 + 0112 + 0114 +
/// 0129, state/re/JUMP_TABLE_TAILS.md).
///
/// (#included from th05/boss_4.cpp, ahead of th05/main/midboss/m3_updt.cpp,
/// because Louise sits below the midboss in the original and the address order
/// is the include order. The `-zCB4_UPDATE_TEXT -zPmain_03` pragma stays in
/// th05/boss_4.cpp: only the first file compiled into an object may name its
/// segment, kb/codegen/0112 trap 0.)
///
/// This file is now the earliest in that object, so it owns every unguarded
/// header the object shares: th04/main/gather.hpp, th04/main/homing.hpp and
/// th04/main/hud/hud.hpp come off m3_updt.cpp, th04/main/bg.hpp and
/// th05/main/bullet/laser.hpp off b3.cpp, and th04/math/vector.hpp off
/// b4_both.cpp. Naming any of them again further down the object would be a
/// redefinition rather than a no-op.

#include "libs/master.lib/master.hpp"
// Guarded, and the route the rest of this object uses to reach
// th04/math/randring.hpp, th03/math/randring.hpp and th05/sprites/main_pat.h,
// none of which has an include guard -- so randring2_next16() and
// randring2_next16_and() come from here rather than from a direct include.
#include "th04/main/player/shot.hpp"
#include "th04/snd/snd.h"
#include "th04/main/phase.hpp"
#include "th04/main/bullet/bullet.hpp"
#include "th04/main/boss/boss.hpp"
// The unguarded set this file takes over, per the block comment above.
#include "th04/math/vector.hpp"
#include "th04/main/bg.hpp"
#include "th04/main/gather.hpp"
#include "th04/main/homing.hpp"
#include "th04/main/hud/hud.hpp"
#include "th05/main/bullet/laser.hpp"
#include "th05/main/boss/bosses.hpp"

// Constants
// ---------

static const int LOUISE_HP_TOTAL = 4400;
#define HP_TOTAL	LOUISE_HP_TOTAL

// st02.bmt. 180 is the idle cel Louise returns to after every pattern, 184 the
// one she charges on, and 188 the one she fires and dies on. [inferred] from
// which of the three each half of this file writes.
static const int PAT_LOUISE = 180;
static const int PAT_LOUISE_CHARGE = 184;
static const int PAT_LOUISE_FIRE = 188;

static const int LOUISE_COL_GATHER_1 = 9;
static const int LOUISE_COL_GATHER_2 = 8;
#define COL_GATHER_1	LOUISE_COL_GATHER_1
#define COL_GATHER_2	LOUISE_COL_GATHER_2

// The box louise_flystep_random() bounces inside, in pixels.
static const pixel_t FLY_LEFT = 48;
static const pixel_t FLY_RIGHT = 336;
static const pixel_t FLY_TOP = 48;
static const pixel_t FLY_BOTTOM = 96;

// Every pattern phase ends after this many fly steps.
static const int FLY_STEPS_MAX = 20;

// The charge animation runs from CHARGE_FRAME to FIRE_FRAME, which is also
// GATHER_FRAMES long.
static const int CHARGE_FRAME = 16;
static const int FIRE_FRAME = 32;
// ---------

// State
// -----

// Ramped up by the two patterns that read it, and re-seeded per phase.
#define bullet_speed	boss_statebyte[10]

// Phase 5 alternates its two patterns through this, rather than off
// [boss.phase_state] the way phase 2 does.
#define mode_next   	boss_statebyte[14]

// Mirrors whatever the running pattern last did, so that its next volley goes
// the other way.
#define angle_flip  	boss_statebyte[15]
// -----

/// The shared flight step
/// ----------------------

// Seeds a random angle on the first frame of the step, then walks [boss.pos]
// along it, reflecting off each edge of the fly box. Returns whether [frames]
// have passed since the phase last reset [boss.phase_frame].
// See boss_flystep_random(), which is a different function with a different
// interface -- Louise's takes both a speed and a duration.
static bool pascal near louise_flystep_random(subpixel_t speed, int frames)
{
	unsigned char angle;

	if(boss.phase_frame == 1) {
		angle = randring2_next16();
		vector2(
			boss.pos.velocity.x.v, boss.pos.velocity.y.v, angle, speed
		);
	}
	boss.pos.cur.x.v += boss.pos.velocity.x.v;
	boss.pos.cur.y.v += boss.pos.velocity.y.v;
	// kb/codegen/0053's shape, the other way round: the constant goes into AX
	// and the memory word is the multiplicand, which is the one-operand
	// `IMUL`. A plain `-1 * x` folds to the three-operand form instead, one
	// byte shorter. th04/main/boss/b1_updt.cpp reflects Yuuka's X the same way.
	if(
		(boss.pos.cur.x.v <= TO_SP(FLY_LEFT)) ||
		(boss.pos.cur.x.v >= TO_SP(FLY_RIGHT))
	) {
		_AX = -1;
		_asm imul word ptr [boss+8]
		boss.pos.velocity.x.v = _AX;
	}
	if(
		(boss.pos.cur.y.v <= TO_SP(FLY_TOP)) ||
		(boss.pos.cur.y.v >= TO_SP(FLY_BOTTOM))
	) {
		_AX = -1;
		_asm imul word ptr [boss+10]
		boss.pos.velocity.y.v = _AX;
	}
	if(boss.phase_frame >= frames) {
		return true;
	}
	return false;
}

/// The shared charge-and-fire cycle
/// --------------------------------

// Where in its charge/fire cycle a pattern is. Returned in AL, which
// `#pragma option -b-` gives an `enum` that fits in a byte.
enum louise_stage_t {
	STAGE_CHARGING = 0,
	STAGE_FIRE_FIRST = 1,
	STAGE_FIRING = 2,
	STAGE_DONE = 3,

	// Every call site promotes the result with `MOV AH, 0`, not `CBW`, so the
	// type is UNSIGNED char and not plain char. Same device as
	// explosion_type_t's _FORCE_INT16 in th04/main/boss/boss.hpp.
	_louise_stage_t_FORCE_UINT8 = 0xFF,
};

// Drives the gather animation for the first FIRE_FRAME frames of a pattern and
// then reports which part of the volley the caller should run, ending it after
// [frames].
static louise_stage_t pascal near pattern_stage(int frames)
{
	if(boss.phase_frame >= CHARGE_FRAME) {
		if(boss.phase_frame < FIRE_FRAME) {
			boss.sprite = PAT_LOUISE_CHARGE;
			gather_add_only_3stack(
				(boss.phase_frame - CHARGE_FRAME),
				COL_GATHER_1,
				COL_GATHER_2
			);
			if(boss.phase_frame == CHARGE_FRAME) {
				snd_se_play(8);
			}
		} else if(boss.phase_frame == FIRE_FRAME) {
			boss.sprite = PAT_LOUISE_FIRE;
			return STAGE_FIRE_FIRST;
		} else if(boss.phase_frame < frames) {
			return STAGE_FIRING;
		} else {
			return STAGE_DONE;
		}
	}
	return STAGE_CHARGING;
}

// What every pattern does once pattern_stage() reports STAGE_DONE.
#define louise_pattern_done() { \
	boss.mode = 0; \
	boss.phase_frame = 0; \
	boss.sprite = PAT_LOUISE; \
}

/// The patterns
/// ------------

// Two blue ball fans, half a turn apart, whose speed ramps up over the volley.
static void near pattern_blue_fans(void)
{
	switch(pattern_stage(0x40)) {
	case STAGE_FIRE_FIRST:
		bullet_template.spawn_type = BST_NORMAL;
		bullet_template.patnum = PAT_BULLET16_N_BALL_BLUE;
		bullet_template.group = BG_SPREAD;
		bullet_template.speed.v = bullet_speed;
		bullet_template.set_spread(5, 0x06);
		bullet_template.special_motion = BSM_EXACT_LINEAR;
		angle_flip = (1 - angle_flip);
		if(angle_flip != 0) {
			bullet_template.angle = 0x20;
		} else {
			bullet_template.angle = 0x60;
		}
		bullet_template_tune();
		snd_se_play(15);
		if(bullet_speed < 0x24) {
			bullet_speed += 4;
		}
		// Falls through into the first frame of its own volley, exactly as
		// the original does.
	case STAGE_FIRING:
		if((boss.phase_frame % 2) == 0) {
			bullets_add_regular();
			bullet_template.angle += 0x80;
			bullets_add_regular();
			if(angle_flip != 0) {
				bullet_template.angle += 0x87;
			} else {
				bullet_template.angle += 0x79;
			}
			bullet_template.speed.v += 4;
		}
		break;

	case STAGE_DONE:
		louise_pattern_done();
		break;
	}
}

// A rank-scaled stack of red vector bullets, drifting to one side.
static void near pattern_red_stacks(void)
{
	switch(pattern_stage(0x60)) {
	case STAGE_FIRE_FIRST:
		bullet_template.spawn_type = (BST_CLOUD_FORWARDS | BST_NO_DECELERATE);
		bullet_template.patnum = PAT_BULLET16_V_RED;
		bullet_template.group = BG_STACK;
		bullet_template.speed.set(2.0f);
		bullet_template.set_stack_for_rank(
			10, 0.3125f,
			12, 0.3125f,
			12, 0.375f,
			14, 0.375f
		);
		bullet_template.angle = -0x40;
		break;

	case STAGE_FIRING:
		if((boss.phase_frame % 2) == 0) {
			bullets_add_regular();
			if(angle_flip != 0) {
				bullet_template.angle -= 0x08;
			} else {
				bullet_template.angle += 0x08;
			}
			snd_se_play(15);
		}
		break;

	case STAGE_DONE:
		louise_pattern_done();
		break;
	}
}

// Eight four-way bursts, fired as one salvo every 32 frames and mirrored about
// the other diagonal on every second salvo. Unlike the rest, this one runs
// beside a flight step rather than through pattern_stage().
static void near pattern_symmetric_bursts(void)
{
	int i;
	unsigned char angle_first;

	if((boss.phase_frame % 32) != 0) {
		return;
	}
	bullet_template.spawn_type = BST_CLOUD_BACKWARDS;
	bullet_template.group = BG_SINGLE;
	bullet_template.patnum = 0;
	bullet_template_tune();
	bullet_template.speed.set(3.0f);
	if((boss.phase_frame % 64) == 0) {
		bullet_template.angle = 0x24;
		for(i = 0; i < 8; i++) {
			angle_first = bullet_template.angle;
			bullets_add_regular();
			bullet_template.angle += 0x80;
			bullets_add_regular();
			bullet_template.angle = (0x40 - bullet_template.angle);
			bullets_add_regular();
			bullet_template.angle += 0x80;
			bullets_add_regular();
			bullet_template.angle = angle_first;
			bullet_template.speed.v -= 5;
			bullet_template.angle += 0x08;
		}
	} else {
		bullet_template.angle = 0x64;
		for(i = 0; i < 8; i++) {
			angle_first = bullet_template.angle;
			bullets_add_regular();
			bullet_template.angle += 0x80;
			bullets_add_regular();
			bullet_template.angle = (-0x40 - bullet_template.angle);
			bullets_add_regular();
			bullet_template.angle += 0x80;
			bullets_add_regular();
			bullet_template.angle = angle_first;
			bullet_template.speed.v -= 5;
			bullet_template.angle += 0x08;
		}
	}
}

// A fan of shoot-out lasers, swept from one side and optionally mirrored.
static void near pattern_sweeping_lasers(void)
{
	switch(pattern_stage(0x80)) {
	case STAGE_FIRE_FIRST:
		laser_template.coords.origin = boss.pos.cur;
		laser_template.coords.width.nonshrink = 6;
		laser_template.col = 8;
		laser_template.coords.angle = -16;
		laser_template.active_at_age.moveout = 30;
		laser_template.shootout_speed.set(6.5f);
		angle_flip = randring2_next16_and(1);
		// Falls through, exactly as the original does.
	case STAGE_FIRING:
		// The sweep runs from -16 up through 0 to 128 and stops; the gap
		// between is where the fan has already passed the playfield.
		if(
			(laser_template.coords.angle >= static_cast<unsigned char>(-16)) ||
			(laser_template.coords.angle <= 128)
		) {
			if((boss.phase_frame % 4) == 0) {
				if(angle_flip != 0) {
					lasers_shootout_add();
				} else {
					laser_template.coords.angle = (
						128 - laser_template.coords.angle
					);
					lasers_shootout_add();
					laser_template.coords.angle = (
						128 - laser_template.coords.angle
					);
				}
				laser_template.coords.angle += 10;
			}
		}
		break;

	case STAGE_DONE:
		louise_pattern_done();
		break;
	}
}

// One aimed ring stack whose bullets decelerate and then turn, to one side or
// the other.
static void near pattern_turning_rings(void)
{
	switch(pattern_stage(0x40)) {
	case STAGE_FIRE_FIRST:
		bullet_template.spawn_type = (BST_CLOUD_FORWARDS | BST_NO_DECELERATE);
		bullet_template.patnum = PAT_BULLET16_V_RED;
		bullet_template.group = BG_RING_STACK_AIMED;
		bullet_template.speed.v = bullet_speed;
		if(bullet_speed < 0x20) {
			bullet_speed += 4;
		}
		bullet_template.set_spread_stack(24, 0x08, 5, 0.5625f);
		bullet_template.angle = 0x00;
		bullet_template.special_motion = BSM_DECELERATE_THEN_TURN;
		bullet_special.turns_max = 1;
		if(randring2_next16_and(1)) {
			bullet_template_special_angle.turn_by = 0x20;
		} else {
			bullet_template_special_angle.turn_by = -0x20;
		}
		bullet_template_tune();
		snd_se_play(15);
		bullets_add_special();
		break;

	case STAGE_DONE:
		louise_pattern_done();
		break;
	}
}

// A pair of aimed spreads every 32 frames, alternating which side they lead.
static void near pattern_aimed_pairs(void)
{
	if((boss.phase_frame % 32) != 0) {
		return;
	}
	bullet_template.spawn_type = (BST_CLOUD_FORWARDS | BST_NO_DECELERATE);
	bullet_template.group = BG_SPREAD_AIMED;
	bullet_template.patnum = 0;
	bullet_template.special_motion = BSM_EXACT_LINEAR;
	bullet_template.speed.set(2.0f);
	bullet_template.set_spread(32, 0x02);
	bullet_template_tune();
	if((boss.phase_frame % 64) == 0) {
		bullet_template.angle = 0x1F;
	} else {
		bullet_template.angle = -0x1F;
	}
	bullets_add_special();
	snd_se_play(3);
}

#pragma option -a2

void pascal far louise_update(void)
{
	homing_target.x = boss.pos.cur.x;
	homing_target.y = boss.pos.cur.y;

	boss.phase_frame++;

	bullet_template.spawn_type = BST_NORMAL;
	bullet_template.origin = boss.pos.cur;
	gather_template.center = boss.pos.cur;

	switch(boss.phase) {
	case 0:
		if(boss.phase_frame == 1) {
			boss.hp = HP_TOTAL;
			boss.phase_end_hp = 3000;
			gather_template.radius.set(64.0f);
			gather_template.angle_delta = 0x02;
			gather_template.ring_points = 8;
		}
		boss_hittest_shots_invincible();
		// Timeout condition
		if(boss.phase_frame >= 128) {
			boss.phase++;
			boss.phase_frame = 0;
			snd_se_play(13);
			bg_render_bombing_func = louise_bg_render;
		}
		break;

	case 1:
		boss_hittest_shots_invincible();
		// Timeout condition
		if(boss.phase_frame >= 64) {
			boss.phase++;
			boss.phase_frame = 0;
			boss.mode = 1;
			boss.phase_state.patterns_seen = 0;
			bullet_speed = 0x18;
			angle_flip = 0;
		}
		break;

	case 2:
		switch(boss.mode) {
		case 0:
			if(louise_flystep_random(TO_SP(1), 96)) {
				boss.phase_frame = 0;
				boss.phase_state.patterns_seen++;
				boss.mode = ((boss.phase_state.patterns_seen & 1) + 1);
				if(boss.phase_state.patterns_seen >= FLY_STEPS_MAX) {
					goto phase_2_next;
				}
			}
			break;
		case 1:
			pattern_blue_fans();
			break;
		case 2:
			pattern_red_stacks();
			break;
		}
		if(!boss_hittest_shots()) {
			break;
		}
		boss_score_bonus(10);
phase_2_next:
		boss_phase_next(ET_NW_SE, 1900);
		break;

	case 3:
		boss_hittest_shots();
		// Timeout condition
		if(boss.phase_frame >= 64) {
			goto phase_3_next;
		}
		break;

	case 4:
		pattern_symmetric_bursts();
		if(louise_flystep_random(14, 128)) {
			boss.phase_frame = 0;
			boss.phase_state.patterns_seen++;
			if(boss.phase_state.patterns_seen >= FLY_STEPS_MAX) {
				goto phase_4_next;
			}
		}
		if(!boss_hittest_shots()) {
			break;
		}
		boss_score_bonus(10);
phase_4_next:
		boss_phase_next(ET_SW_NE, 500);
		mode_next = 0;
		bullet_speed = 0x14;
		break;

	case 5:
		switch(boss.mode) {
		case 0:
			if(louise_flystep_random(TO_SP(3), 48)) {
				boss.mode = (mode_next + 1);
				mode_next = (1 - mode_next);
				boss.phase_frame = 0;
				boss.phase_state.patterns_seen++;
				if(boss.phase_state.patterns_seen >= FLY_STEPS_MAX) {
					goto phase_5_next;
				}
			}
			break;
		case 1:
			pattern_sweeping_lasers();
			break;
		case 2:
			pattern_turning_rings();
			break;
		}
		if(!boss_hittest_shots()) {
			break;
		}
		boss_score_bonus(10);
phase_5_next:
		boss_phase_next(ET_NW_SE, 0);
		break;

	case 6:
		boss_hittest_shots();
		// Timeout condition
		if(boss.phase_frame >= 64) {
phase_3_next:
			boss.phase_frame = 0;
			boss.phase++;
			boss.sprite = PAT_LOUISE_FIRE;
		}
		break;

	case 7:
		pattern_aimed_pairs();
		if(louise_flystep_random(8, 128)) {
			boss.phase_frame = 0;
			boss.phase_state.patterns_seen++;
			// Timeout condition: running out of fly steps denies the bonus,
			// being shot down grants it.
			if(boss.phase_state.patterns_seen >= 10) {
				boss.phase_state.defeat_bonus = false;
				goto phase_7_defeat;
			}
		}
		if(!boss_hittest_shots()) {
			break;
		}
		boss.phase_state.defeat_bonus = true;
phase_7_defeat:
		boss.phase_frame = 0;
		boss.phase = PHASE_BOSS_EXPLODE_SMALL;
		break;

	default:
		boss_defeat_update(10);
		break;
	}

	hud_hp_update_and_render(boss.hp, HP_TOTAL);
}

#pragma option -a1

// Unprefixed tokens in a file that is #included into th05/boss_4.cpp's
// translation unit: without these they leak into m3_updt.cpp, b3.cpp and
// b4_both.cpp below, all three of which have their own.
#undef HP_TOTAL
#undef COL_GATHER_1
#undef COL_GATHER_2
#undef bullet_speed
#undef mode_next
#undef angle_flip
#undef louise_pattern_done
