/// Stage 2 midboss - update
/// ------------------------
/// The renderer is th05/main/midboss/m2.cpp, in a different object; only the
/// update half lives in B4_UPDATE_TEXT. Structurally this is
/// th05/main/midboss/m3_updt.cpp's twin -- same phase dispatch, same
/// hittest/defeat blocks, same tail -- with three phases instead of two and an
/// orbit in place of the flap.
///
/// (#included from th05/midboss2.cpp, which is its OWN object rather than the
/// front of th05/boss_4.cpp -- see that file for the -a2 parity measurement
/// that settles it. Because it is a separate translation unit, it names every
/// header it needs and shares none of them with boss_4.cpp's four files.)
///
/// **With this file, th05_main.asm contributes nothing to B4_UPDATE_TEXT at
/// all.** The dump keeps the `segment`/`ends` pair and the `main_03` group
/// entry, the way every other emptied segment in these dumps does; the segment
/// is now two C++ objects and no assembly.

#include "libs/master.lib/master.hpp"
// Guarded, and the route the rest of this object uses to reach
// th04/math/randring.hpp, th03/math/randring.hpp and th05/sprites/main_pat.h,
// none of which has an include guard.
#include "th04/main/player/shot.hpp"
#include "th04/snd/snd.h"
#include "th04/main/phase.hpp"
#include "th04/main/bullet/bullet.hpp"
#include "th04/main/item/item.hpp"
#include "th04/main/boss/boss.hpp"
#include "th04/math/vector.hpp"
#include "th04/main/frames.h"
#include "th04/main/homing.hpp"
#include "th04/main/hud/hud.hpp"
#include "th04/main/bullet/clearzap.hpp"
#include "th04/main/midboss/midboss.hpp"
#include "th04/main/spark.hpp"

// The point the midboss orbits during phase 1, drifting sideways across the
// playfield while the orbit radius breathes in and out. This parcel renamed it
// in th05_main.asm's _BSS block, out of the address-suffixed placeholder
// state/re/NAMING_REVIEW_VERDICTS_19.md section 10 deferred to whichever parcel
// lifted its two readers; state/notes/midboss2_update.md records the old
// spelling.
extern "C" SPPoint midboss2_center;

// Constants
// ---------

static const pixel_t MIDBOSS2_W = 64;

// Same hitbox on both axes, and the same for the invincible test in phase 0 as
// for the damaging ones in phases 1 and 2.
static const subpixel_t MIDBOSS2_HITBOX_RADIUS = (
	(TO_SP(MIDBOSS2_W) / 2) - (TO_SP(MIDBOSS2_W) / 8)
);

static const int MIDBOSS2_HP_TOTAL = 1400;

// st02.bmt, the same two cels th05/main/midboss/m2.cpp animates.
static const int PAT_MIDBOSS2_OTHER = 206;

// The bounds the orbit centre turns around at, and the X it starts from.
static const pixel_t CENTER_LEFT = 128;
static const pixel_t CENTER_RIGHT = 256;
static const pixel_t CENTER_X_START = 192;

// The orbit radius grows to this and then shrinks back to the second.
static const pixel_t RADIUS_MAX = 64;
static const pixel_t RADIUS_MIN = 1;
// ---------

// State
// -----

// Signed: the dump reads this slot with `CBW`, not `MOV AH, 0`.
#define center_velocity_x	boss_statebyte[14]

// 0 while the orbit radius is growing, 1 while it shrinks back.
#define radius_shrinking 	boss_statebyte[15]

// MODDERS: The orbit radius, kept in a field that is never used as a velocity
// during this fight.
#define orbit_radius     	midboss.pos.velocity.x
// -----

// Walks the midboss one step around [midboss2_center], drifts that centre
// sideways, and breathes the radius in and out.
static void near midboss2_orbit(void)
{
	midboss.pos.prev = midboss.pos.cur;
	vector2_at(
		midboss.pos.cur,
		midboss2_center.x.v,
		midboss2_center.y.v,
		orbit_radius.v,
		midboss.angle
	);

	midboss2_center.x.v += static_cast<char>(center_velocity_x);
	if(midboss2_center.x.v < TO_SP(CENTER_LEFT)) {
		center_velocity_x += 2;
	} else if(midboss2_center.x.v > TO_SP(CENTER_RIGHT)) {
		center_velocity_x -= 2;
	}

	if(radius_shrinking == 0) {
		orbit_radius.v += TO_SP(1);
		if(orbit_radius.v > TO_SP(RADIUS_MAX)) {
			radius_shrinking = 1;
		}
	} else {
		orbit_radius.v -= TO_SP(1);
		if(orbit_radius.v <= TO_SP(RADIUS_MIN)) {
			radius_shrinking = 0;
		}
	}

	midboss.angle -= 2;
}

// Phase 1: a red stack at a random angle, plus a random spread that widens
// twice as the midboss loses HP.
static void near pattern_random_stacks_and_balls(void)
{
	if(stage_frame_mod16 != 0) {
		return;
	}
	bullet_template.spawn_type = BST_NO_DECELERATE;
	bullet_template.patnum = PAT_BULLET16_N_BALL_RED;
	bullet_template.group = BG_STACK;
	bullet_template.set_stack(3, 0.5f);
	bullet_template.speed.set(1.5f);
	bullet_template.angle = randring2_next16();
	bullet_template_tune();
	bullets_add_regular();

	bullet_template.spawn_type = (BST_CLOUD_FORWARDS | BST_NO_DECELERATE);
	bullet_template.patnum = 0;
	bullet_template.group = BG_RANDOM_ANGLE_AND_SPEED;
	bullet_template.spread = 3;
	if(midboss.hp <= 600) {
		bullet_template.spread = 6;
	} else if(midboss.hp <= 800) {
		bullet_template.spread = 4;
	}
	bullet_template.speed.set(1.5f);
	bullet_template.angle = randring2_next16();
	bullet_template_tune();
	bullets_add_regular();
}

// Phase 2: a taller blue stack at a random angle, four times as often.
static void near pattern_blue_stacks(void)
{
	if(stage_frame_mod4 != 0) {
		return;
	}
	bullet_template.spawn_type = BST_NO_DECELERATE;
	bullet_template.patnum = PAT_BULLET16_N_BALL_BLUE;
	bullet_template.group = BG_STACK;
	bullet_template.set_stack(3, 0.9375f);
	bullet_template.speed.set(1.5f);
	bullet_template.angle = randring2_next16();
	bullet_template_tune();
	bullets_add_regular();
}

void pascal far midboss2_update(void)
{
	bullet_template.origin = midboss.pos.cur;

	midboss.phase_frame++;

	switch(midboss.phase) {
	case 0:
		midboss.pos.update_seg3();
		midboss_hittest_shots_invincible(
			MIDBOSS2_HITBOX_RADIUS, MIDBOSS2_HITBOX_RADIUS
		);
		// Timeout condition
		if(midboss.phase_frame >= 256) {
			midboss.phase++;
			midboss.phase_frame = 0;
			midboss.angle = 0x00;
			radius_shrinking = 0;
			center_velocity_x = 0x20;
			orbit_radius.set(0.0f);
			midboss2_center.x.v = TO_SP(CENTER_X_START);
			midboss2_center.y = midboss.pos.cur.y;
		}
		break;

	case 1:
		pattern_random_stacks_and_balls();
		midboss2_orbit();
		midboss_hittest_shots(
			MIDBOSS2_HITBOX_RADIUS, MIDBOSS2_HITBOX_RADIUS
		);
		// Timeout condition
		if(midboss.phase_frame < 1000) {
			if(midboss.hp > 400) {
				break;
			}
			midboss_score_bonus(5);
			bullets_clear();
		}
		midboss.phase++;
		midboss.sprite = PAT_MIDBOSS2_OTHER;
		midboss.phase_frame = 0;
		sparks_add_circle(
			midboss.pos.cur.x, midboss.pos.cur.y, TO_SP(4), 32
		);
		snd_se_play(15);
		// Aim the midboss back at the top centre of the playfield for the
		// last phase, which moves it with the ordinary update_seg3() step.
		vector2_between_plus(
			midboss.pos.cur.x,
			midboss.pos.cur.y,
			TO_SP(192),
			TO_SP(64),
			0,
			midboss.pos.velocity.x.v,
			midboss.pos.velocity.y.v,
			1
		);
		break;

	case 2:
		pattern_blue_stacks();
		midboss.pos.update_seg3();
		midboss_hittest_shots(
			MIDBOSS2_HITBOX_RADIUS, MIDBOSS2_HITBOX_RADIUS
		);
		// Timeout condition
		if(midboss.phase_frame < 800) {
			if(midboss.hp > 0) {
				break;
			}
			bullet_zap.active = true;
			midboss_score_bonus(15);
			items_add(midboss.pos.cur.x, midboss.pos.cur.y, IT_BOMB);
		}
		midboss.phase = PHASE_EXPLODE_BIG;
		midboss.sprite = PAT_ENEMY_KILL;
		midboss.phase_frame = 0;
		sparks_add_circle(
			midboss.pos.cur.x, midboss.pos.cur.y, TO_SP(8), 48
		);
		snd_se_play(12);
		break;

	default:
		midboss_defeat_update();
		hud_hp_update_and_render(midboss.hp, MIDBOSS2_HP_TOTAL);
		return;
	}

	hud_hp_update_and_render(midboss.hp, MIDBOSS2_HP_TOTAL);
	homing_target.x = midboss.pos.cur.x;
	homing_target.y = midboss.pos.cur.y;
}

// Unprefixed tokens in a file that is #included into th05/boss_4.cpp's
// translation unit: without these they leak into b2.cpp, m3_updt.cpp, b3.cpp
// and b4_both.cpp below.
#undef center_velocity_x
#undef radius_shrinking
#undef orbit_radius
