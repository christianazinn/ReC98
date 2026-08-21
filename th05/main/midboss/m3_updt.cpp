/// Stage 3 midboss - update
/// ------------------------
/// The renderer is th05/main/midboss/m3.cpp, in a different object; only the
/// update half lives in B4_UPDATE_TEXT. midboss3_update() was the last
/// emitting proc of th05_main.asm's contribution to that segment -- the
/// `dw offset loc_...` run below it is the jump table its own `switch`
/// compiles to, not data the dump owns -- so this file grows the C++ object
/// that already follows the root backwards into the hole
/// (kb/codegen 0099 + 0112 + 0114 + 0129, state/re/JUMP_TABLE_TAILS.md).
///
/// (#included from th05/boss_4.cpp, *ahead* of th05/main/boss/b3.cpp, because
/// this code sits below Alice's in the original. The
/// `-zCB4_UPDATE_TEXT -zPmain_03` pragma stays in th05/boss_4.cpp: only the
/// first file compiled into an object may name its segment, kb/codegen/0112
/// trap 0.)
///
/// Being the *first* file in that object, this one now owns the unguarded
/// headers it shares with b3.cpp -- th04/main/gather.hpp,
/// th04/main/homing.hpp, th04/main/hud/hud.hpp and
/// th04/main/bullet/clearzap.hpp. b3.cpp reaches all four through this file
/// and no longer names them; naming either of them there would be a
/// redefinition rather than a no-op.

#include "libs/master.lib/master.hpp"
// Guarded, and the route b3.cpp already uses to reach th04/math/randring.hpp
// and th05/sprites/main_pat.h -- neither of which has an include guard, so
// naming either of them directly here would break b3.cpp below.
#include "th04/main/player/shot.hpp"
#include "th04/snd/snd.h"
#include "th04/main/phase.hpp"
#include "th04/main/player/player.hpp"
#include "th04/main/bullet/bullet.hpp"
#include "th04/main/item/item.hpp"
#include "th04/main/boss/boss.hpp"
// The four taken over from b3.cpp, per the block comment above.
#include "th04/main/gather.hpp"
#include "th04/main/homing.hpp"
#include "th04/main/hud/hud.hpp"
#include "th04/main/bullet/clearzap.hpp"
// Both unguarded and both new to this object: nothing else compiled into
// th05/boss_4.cpp reaches either.
#include "th04/main/midboss/midboss.hpp"
#include "th04/main/spark.hpp"

// Constants
// ---------

static const pixel_t MIDBOSS3_W = 64;

// The hitbox is the same on both axes, and the same for the invincible test
// in phase 0 as for the damaging one in phase 1.
static const subpixel_t MIDBOSS3_HITBOX_RADIUS = (
	(TO_SP(MIDBOSS3_W) / 2) - (TO_SP(MIDBOSS3_W) / 8)
);

// Prefixed, then aliased to the house spelling for the body below, because
// th05/main/boss/b3.cpp is compiled into this same object and declares its own
// HP_TOTAL, COL_GATHER_1 and COL_GATHER_2. Same device, and for the same
// reason, as th04/main/midboss/m3.cpp's `patterns_done`; all three are
// #undef'd at the bottom of this file.
static const int MIDBOSS3_HP_TOTAL = 1400;
#define HP_TOTAL	MIDBOSS3_HP_TOTAL

// st03.bmt. 208 is the still cel -- th05/main/midboss/m3.cpp calls the same
// one PAT_MIDBOSS3_ANIMATED, because the renderer is the half that animates
// it. 210 and 211 are the two charge-up cels the gather animation steps
// through, and 212…219 is the flap that carries the midboss to its next
// position. [inferred] from which cel each half of this file writes; what
// they depict is not recoverable from the dump.
static const int PAT_MIDBOSS3 = 208;
static const int PAT_MIDBOSS3_GATHER_1 = 210;
static const int PAT_MIDBOSS3_GATHER_2 = 211;
static const int PAT_MIDBOSS3_MOVE_first = 212;
static const int PAT_MIDBOSS3_MOVE_last = 219;

// The gather circle colors, spelled under the two names every TH05 boss and
// midboss charges with; b1.cpp and b3.cpp carry the same pair at their own
// values.
enum midboss3_colors_t {
	MIDBOSS3_COL_GATHER_1 = 11,
	MIDBOSS3_COL_GATHER_2 = 10,
};
#define COL_GATHER_1	MIDBOSS3_COL_GATHER_1
#define COL_GATHER_2	MIDBOSS3_COL_GATHER_2

// [patterns_done] starts at 1 the frame phase 1 begins and is incremented once
// per completed pattern, with the bound tested *after* the increment. So the
// midboss dies once the counter reaches 13, and the twelfth pattern is the
// last one that actually runs -- the selector computed in the same step as the
// thirteenth increment is never dispatched. Spelled as the bound the code
// tests, not as the number of patterns that fire.
static const int PATTERNS_DONE_MAX = 13;

static const int PATTERN_COUNT = 3;
// ---------

// State
// -----

// Same two slots, in the same two roles, as TH05's own @midboss4_update$qv.
#define patterns_done	boss_statebyte[13]

// 0 while the midboss is between patterns and midboss3_update() is picking the
// next one; 1…PATTERN_COUNT while that pattern is running.
#define pattern      	boss_statebyte[14]

// Which half of the flap midboss3_reposition() is playing.
#define moving_down  	boss_statebyte[15]
// -----

// Steps the flap that moves the midboss to its next position, every 4th frame.
// On the way out, the midboss drops by 12 pixels and re-centers itself on the
// player; on the way back in, it hands control to midboss3_update()'s selector.
static void near midboss3_reposition(void)
{
	if((midboss.phase_frame & 3) != 0) {
		return;
	}
	if(moving_down == 0) {
		midboss.sprite++;
		if(midboss.sprite < (PAT_MIDBOSS3_MOVE_last + 1)) {
			return;
		}
		midboss.sprite = PAT_MIDBOSS3_MOVE_last;
		midboss.pos.cur.y += 12.0f;
		midboss.pos.cur.x = player_pos.cur.x;
		// [inferred] The two bounds are one sprite width in from each edge of
		// the playfield; the dump only has the two folded subpixel constants.
		if(midboss.pos.cur.x < TO_SP(MIDBOSS3_W)) {
			midboss.pos.cur.x.v = TO_SP(MIDBOSS3_W);
		} else if(midboss.pos.cur.x > TO_SP(PLAYFIELD_W - MIDBOSS3_W)) {
			midboss.pos.cur.x.v = TO_SP(PLAYFIELD_W - MIDBOSS3_W);
		}
		moving_down++;
	} else {
		midboss.sprite--;
		if(midboss.sprite >= PAT_MIDBOSS3_MOVE_first) {
			return;
		}
		midboss.sprite = PAT_MIDBOSS3;
		moving_down = 0;
		midboss.phase_frame = 0;
		pattern = 0;
	}
}

// The charge-up every pattern plays before its first volley.
static void near midboss3_gather(void)
{
	gather_add_only_3stack(
		(midboss.phase_frame - 1), COL_GATHER_1, COL_GATHER_2
	);
	if(midboss.phase_frame == 1) {
		snd_se_play(8);
		midboss.sprite = PAT_MIDBOSS3_GATHER_1;
		return;
	}
	if(midboss.phase_frame == 16) {
		midboss.sprite = PAT_MIDBOSS3_GATHER_2;
	}
}

// All three patterns share a schedule: the gather animation runs for its
// GATHER_FRAMES, then one volley on each of frames 32, 48 and 64, and the
// midboss starts moving again at frame 96. Every volley recoils it 4 pixels
// upwards.
//
// The volley is spelled out at the end of every branch rather than once after
// the `if` chain, because that is what the original does: Turbo C++'s -O
// cross-jumps the identical copies onto one, so the frame-32 branch keeps the
// code and falls through into it while the other two jump back. Writing it
// once below the chain instead costs each of these functions the 2-byte `jmp`
// that the fall-through does not need. Same merge that th04/hud_put.cpp
// documents for its two hud_label_put() pairs.
#define midboss3_fire() { \
	bullets_add_regular(); \
	snd_se_play(15); \
	midboss.pos.prev.y = midboss.pos.cur.y; \
	midboss.pos.cur.y -= 4.0f; \
}

#define midboss3_fire_tuned() { \
	bullet_template_tune(); \
	midboss3_fire(); \
}

static void near pattern_spread_stacks(void)
{
	if(midboss.phase_frame < GATHER_FRAMES) {
		midboss3_gather();
		return;
	}
	if(midboss.phase_frame == 32) {
		bullet_template.spawn_type = BST_CLOUD_FORWARDS;
		bullet_template.patnum = PAT_BULLET16_N_BALL_BLUE;
		bullet_template.speed.set(1.5f);
		bullet_template.group = BG_SPREAD_STACK;
		bullet_template.set_spread_stack(3, 0x10, 4, 0.5f);
		bullet_template.angle = 0x40;
		midboss3_fire_tuned();
	} else if(midboss.phase_frame == 48) {
		bullet_template.patnum = PAT_BULLET16_N_BALL_RED;
		bullet_template.speed.set(3.0f);
		bullet_template.set_spread_stack(4, 0x05, 4, 0.5f);
		midboss3_fire_tuned();
	} else if(midboss.phase_frame == 64) {
		bullet_template.patnum = PAT_BULLET16_N_BALL_BLUE;
		bullet_template.speed.set(2.0f);
		bullet_template.set_spread_stack(7, 0x0A, 4, 0.5f);
		midboss3_fire_tuned();
	} else if(midboss.phase_frame >= 96) {
		midboss3_reposition();
	}
}

static void near pattern_aimed_stacks(void)
{
	if(midboss.phase_frame < GATHER_FRAMES) {
		midboss3_gather();
		return;
	}
	// The one pattern of the three that does not tune: its spreads are already
	// rank-scaled. The frame-48 and frame-64 branches share the longer suffix
	// -- select_for_rank() onwards -- so they merge with each other, and the
	// frame-32 branch cross-jumps forward into the tail end of that block
	// rather than keeping a copy.
	if(midboss.phase_frame == 32) {
		bullet_template.spawn_type = (BST_CLOUD_FORWARDS | BST_NO_DECELERATE);
		bullet_template.patnum = PAT_BULLET16_N_BALL_BLUE;
		bullet_template.speed.set(1.5f);
		bullet_template.group = BG_STACK_AIMED;
		bullet_template.set_stack(8, 0.5f);
		bullet_template.angle = 0x00;
		midboss3_fire();
	} else if(midboss.phase_frame == 48) {
		bullet_template.group = BG_SPREAD_STACK_AIMED;
		bullet_template.patnum = PAT_BULLET16_N_BALL_RED;
		bullet_template.set_spread_for_rank(
			1, 0x1,
			2, 0x4,
			3, 0x3,
			4, 0x4
		);
		bullet_template.set_stack(8, 0.5f);
		midboss3_fire();
	} else if(midboss.phase_frame == 64) {
		bullet_template.patnum = PAT_BULLET16_N_BALL_BLUE;
		bullet_template.set_spread_for_rank(
			1, 0x1,
			3, 0x5,
			4, 0x5,
			5, 0x4
		);
		bullet_template.set_stack(8, 0.5f);
		midboss3_fire();
	} else if(midboss.phase_frame >= 96) {
		midboss3_reposition();
	}
}

static void near pattern_wide_spreads(void)
{
	if(midboss.phase_frame < GATHER_FRAMES) {
		midboss3_gather();
		return;
	}
	if(midboss.phase_frame == 32) {
		bullet_template.spawn_type = (BST_CLOUD_FORWARDS | BST_NO_DECELERATE);
		bullet_template.patnum = PAT_BULLET16_D_BLUE;
		bullet_template.speed.set(3.0f);
		bullet_template.group = BG_SPREAD;
		// Assigned separately rather than through set_spread(): the dump has
		// two byte moves here, not the one word move that helper compiles to.
		bullet_template.spread = 16;
		bullet_template.spread_angle_delta = 0x08;
		bullet_template.angle = 0x40;
		midboss3_fire_tuned();
	} else if(midboss.phase_frame == 48) {
		bullet_template.speed.set(3.0f);
		bullet_template.spread = 21;
		bullet_template.spread_angle_delta = 0x04;
		midboss3_fire_tuned();
	} else if(midboss.phase_frame == 64) {
		bullet_template.spread = 16;
		bullet_template.spread_angle_delta = 0x08;
		midboss3_fire_tuned();
	} else if(midboss.phase_frame >= 96) {
		midboss3_reposition();
	}
}

#pragma option -a2

void pascal far midboss3_update(void)
{
	bullet_template.origin = midboss.pos.cur;
	gather_template.center = midboss.pos.cur;

	midboss.phase_frame++;

	switch(midboss.phase) {
	case 0:
		midboss.pos.update_seg3();
		midboss_hittest_shots_invincible(
			MIDBOSS3_HITBOX_RADIUS, MIDBOSS3_HITBOX_RADIUS
		);
		// Timeout condition
		if(midboss.phase_frame >= 192) {
			midboss.phase++;
			midboss.phase_frame = 0;
			midboss.angle = 0x00;
			moving_down = 0;
			pattern = 1;
			patterns_done = 1;
			midboss.pos.velocity.x.set(0.0f);
		}
		break;

	case 1:
		midboss.pos.prev = midboss.pos.cur;

		switch(pattern) {
		case 0:
			if(midboss.phase_frame >= 32) {
				pattern = ((patterns_done % PATTERN_COUNT) + 1);
				patterns_done++;
				midboss.phase_frame = 0;
				if(patterns_done >= PATTERNS_DONE_MAX) {
					goto defeated;
				}
			}
			break;
		case 1:
			pattern_spread_stacks();
			break;
		case 2:
			pattern_aimed_stacks();
			break;
		case 3:
			pattern_wide_spreads();
			break;
		}

		// Invincible for as long as the flap is carrying it somewhere else.
		if(midboss.sprite < PAT_MIDBOSS3_MOVE_first) {
			midboss_hittest_shots(
				MIDBOSS3_HITBOX_RADIUS, MIDBOSS3_HITBOX_RADIUS
			);
		}
		if(midboss.hp > 0) {
			break;
		}
		bullet_zap.active = true;
		midboss_score_bonus(15);
		items_add(midboss.pos.cur.x, midboss.pos.cur.y, IT_1UP);
defeated:
		midboss.phase = PHASE_EXPLODE_BIG;
		midboss.sprite = PAT_ENEMY_KILL;
		midboss.phase_frame = 0;
		sparks_add_circle(
			midboss.pos.cur.x, midboss.pos.cur.y, (TO_SP(MIDBOSS3_W) / 8), 48
		);
		snd_se_play(12);
		break;

	default:
		midboss_defeat_update();
		hud_hp_update_and_render(midboss.hp, HP_TOTAL);
		return;
	}

	hud_hp_update_and_render(midboss.hp, HP_TOTAL);
	homing_target.x = midboss.pos.cur.x;
	homing_target.y = midboss.pos.cur.y;
}

#pragma option -a1

// Unprefixed tokens in a file that is #included into th05/boss_4.cpp's
// translation unit: without these they leak into b3.cpp below, which has its
// own `pattern_*` functions and its own [boss_statebyte] aliases.
#undef patterns_done
#undef pattern
#undef moving_down
#undef HP_TOTAL
#undef COL_GATHER_1
#undef COL_GATHER_2
#undef midboss3_fire
#undef midboss3_fire_tuned
