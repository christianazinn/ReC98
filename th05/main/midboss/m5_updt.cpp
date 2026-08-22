/// Stage 5 midboss - update
/// ------------------------
/// The renderer is th05/main/midboss/m5.cpp, in a different object; only the
/// update half lives in main_036_TEXT. midboss5_update() was the last emitting
/// proc of th05_main.asm's contribution to that segment, and the five helpers
/// above it were the five before that, so this file is the tail of the middle
/// block that the kb/codegen/0080 three-way carve of main_036_TEXT left with no
/// C++ successor. It grows backwards into the hole from here (kb/codegen 0099 +
/// 0112 + 0114 + 0129); exalice_update() and its three jump tables are what
/// still stands ahead of it.
///
/// (#included from th05/main_036.cpp, which is its OWN object -- the segment had
/// no C++ contribution at all before this parcel -- so this file names every
/// header it needs and shares none of them.)
///
/// Structurally this is th05/main/midboss/m3_updt.cpp's cousin: the same phase
/// dispatch, the same hittest/defeat blocks and the same tail, over the same
/// three [boss_statebyte] slots. The difference is where the pattern selection
/// lives. midboss3_update() dispatches its three patterns from an inner
/// `switch`; this fight keeps the currently selected one in a function pointer
/// and picks the next out of a table, the way th04/main/midboss/mx_update.cpp
/// and th05/main/boss/b1.cpp do.

#include "th04/snd/snd.h"
// Guarded, and the route the rest of this file uses to reach
// th04/math/randring.hpp, th03/math/randring.hpp, th02/math/randring.hpp and
// th05/sprites/main_pat.h.
#include "th04/main/player/shot.hpp"
// Unguarded, and named here for player_angle_from().
#include "th05/main/player/player.hpp"
#include "th04/main/phase.hpp"
#include "th04/main/bullet/bullet.hpp"
#include "th04/main/bullet/clearzap.hpp"
// Unguarded, one each.
#include "th04/main/pattern.hpp"
#include "th04/main/gather.hpp"
#include "th04/main/homing.hpp"
#include "th04/main/spark.hpp"
#include "th04/main/midboss/midboss.hpp"
#include "th04/main/item/item.hpp"
#include "th04/main/boss/boss.hpp"
#include "th04/main/hud/hud.hpp"

// Constants
// ---------

static const pixel_t MIDBOSS5_W = 64;

// The hitbox is the same on both axes, and the same for the invincible test in
// phase 0 as for the damaging one in phase 1.
static const subpixel_t MIDBOSS5_HITBOX_RADIUS = (
	(TO_SP(MIDBOSS5_W) / 2) - (TO_SP(MIDBOSS5_W) / 8)
);

static const int MIDBOSS5_HP_TOTAL = 1550;

// 212 is the cel every cycle restarts from; the fight steps upwards from there
// and clamps at 219, which is therefore the last cel of the run.
// `[inferred]` from which cel each half of this file writes -- what they depict
// is not recoverable from the dump.
static const int PAT_MIDBOSS5_first = 212;
static const int PAT_MIDBOSS5_last = 219;

// The gather circle colors, spelled under the two names every TH05 boss and
// midboss charges with; th05/main/midboss/m3_updt.cpp and th05/main/boss/b1.cpp
// carry the same pair at their own values.
static const vc2 COL_GATHER_1 = 3;
static const vc2 COL_GATHER_2 = 2;

// The fight ends once this many patterns have been completed. Tested *after*
// the increment, so it is the bound the code compares against rather than a
// count of the patterns that fire.
static const int PATTERNS_DONE_MAX = 0x10;

static const int PATTERN_COUNT = 3;

// The three gather circles land on this frame of the charge-up and the two
// after it, which is what gather_add_only_3stack() offsets its own frame
// counter against.
static const int GATHER_FIRST_CIRCLE_FRAME = 16;
// ---------

// State
// -----

// Same slot, in the same role, as th05/main/midboss/m3_updt.cpp's and TH05's
// own @midboss4_update$qv's.
#define patterns_done	boss_statebyte[13]

// 0 while midboss5_flystep_sideways() is carrying the midboss to its next
// position, 1 while midboss5_gather_and_pattern() is charging and firing.
#define pattern      	boss_statebyte[14]

// Which of the four steps of the sideways flight cycle comes next. Steps 0 and
// 3 go left, 1 and 2 go right, so the midboss reverses every second step.
#define flystep_cycle	boss_statebyte[15]
// -----

// The pattern currently selected out of [MIDBOSS5_PATTERNS_PHASE_1], called by
// midboss5_gather_and_pattern() once its charge-up is over. Seeded with
// pattern_blue_spreads() when phase 1 begins. Mirrors its table exactly as
// midbossx_phase_1_pattern and sara_phase_2_3_pattern mirror theirs.
extern pattern_oneshot_func_t midboss5_phase_1_pattern;

// The three patterns phase 1 cycles through, indexed by [patterns_done] modulo
// PATTERN_COUNT -- spelled `%` rather than `&`, because [boss_statebyte]
// promotes to a *signed* int and only the remainder emits the original's
// `cwd`/`idiv` pair. Named by state/re/NAMING_REVIEW_VERDICTS_19.md section
// 10.2's formula for the pattern tables one boss over: SARA_PATTERNS_PHASE_2_3,
// SHINKI_PATTERNS_PHASE_2_3, MIDBOSSX_PATTERNS_PHASE_1.
extern "C" const pattern_oneshot_func_t MIDBOSS5_PATTERNS_PHASE_1[PATTERN_COUNT];
// -----

// Carries the midboss sideways at a constant 2 pixels per frame for 32 frames,
// and reports that flight as done at the end of them. The direction is decided
// on the first frame from [flystep_cycle]; the vertical velocity is zeroed when
// phase 1 begins and never written again, so this is the whole of the fight's
// movement.
//
// `flystep` is what this tree calls the movement half of a routine that steps a
// boss's or midboss's flight and reports whether that flight is done; the
// family's four unqualified members are boss_flystep_random,
// boss_flystep_towards, marisa_flystep_pointreflected and
// mai_yuki_flystep_random. `sideways` is the tree's own token for
// horizontal-only motion. `[inferred]` name, from the two facts above.
static bool near midboss5_flystep_sideways(void)
{
	if(midboss.phase_frame == 1) {
		if((flystep_cycle == 0) || (flystep_cycle == 3)) {
			midboss.pos.velocity.x.set(-2.0f);
		} else {
			midboss.pos.velocity.x.set(2.0f);
		}
		flystep_cycle++;
		flystep_cycle &= 3;
	}
	midboss.pos.update_seg3();
	if(midboss.phase_frame >= 32) {
		return true;
	}
	return false;
}

// The other half of a phase-1 cycle: the charge-up for the first GATHER_FRAMES,
// then the currently selected pattern for as long as that pattern wants. Ends
// the cycle by handing control back to the flight half once the pattern reports
// done.
//
// The aiming happens during the charge-up and not once the volleys start, so
// every pattern below fires along the angle the player was at on the last frame
// of the gather.
static void near midboss5_gather_and_pattern(void)
{
	if(midboss.phase_frame < GATHER_FRAMES) {
		gather_add_only_3stack(
			(midboss.phase_frame - GATHER_FIRST_CIRCLE_FRAME),
			COL_GATHER_1,
			COL_GATHER_2
		);
		if(midboss.phase_frame == GATHER_FIRST_CIRCLE_FRAME) {
			snd_se_play(8);
		}
		if(
			(midboss.phase_frame == 20) ||
			(midboss.phase_frame == 24) ||
			(midboss.phase_frame == 28)
		) {
			midboss.sprite++;
		}
		bullet_template.angle = player_angle_from(
			midboss.pos.cur.x, midboss.pos.cur.y, 0
		);
		return;
	}
	if(midboss.sprite < PAT_MIDBOSS5_last) {
		if((midboss.phase_frame % 4) == 0) {
			midboss.sprite++;
		}
	}
	if(midboss5_phase_1_pattern()) {
		midboss.phase_frame = 0;
		pattern = 0;
		midboss.sprite = PAT_MIDBOSS5_first;
	}
}

// MIDBOSS5_PATTERNS_PHASE_1[0], and the pattern phase 1 is seeded with: a
// 13-way spread of blue V bullets every 4th frame, at 5.5 pixels per frame,
// until frame 96. `[inferred]` name, in the form
// th05/main/midboss/m3_updt.cpp's pattern_wide_spreads() and
// th05/main/boss/b1.cpp's pattern_red_stacks() already use -- the group and the
// bullet color, both read straight off the template below.
//
// Its address is taken by the dump's own [MIDBOSS5_PATTERNS_PHASE_1] and by
// midboss5_update()'s seed assignment, so this may not be `static`.
bool near pattern_blue_spreads(void)
{
	if((midboss.phase_frame % 4) == 0) {
		bullet_template.spawn_type = (BST_CLOUD_FORWARDS | BST_NO_DECELERATE);
		bullet_template.patnum = PAT_BULLET16_V_BLUE;
		bullet_template.speed.set(5.5f);
		bullet_template.group = BG_SPREAD;
		bullet_template.set_spread(13, 0x7);
		bullets_add_regular();
		snd_se_play(3);
	}
	return (midboss.phase_frame >= 96);
}

// MIDBOSS5_PATTERNS_PHASE_1[1]: a 16-bullet ring of green D bullets every 4th
// frame, at a random angle and from a point randomly displaced by up to 32
// pixels on each axis from the midboss, until frame 96. `random` is the token
// th05/main/boss/b1.cpp's pattern_random_red_rings() -- the same construct with
// the same group, a random angle and no displacement -- already spells for
// this, so the name is that one's, one color over.
bool near pattern_random_green_rings(void)
{
	if((midboss.phase_frame % 4) == 0) {
		bullet_template.spawn_type = BST_NO_DECELERATE;
		bullet_template.patnum = PAT_BULLET16_D_GREEN;
		bullet_template.speed.set(2.0f);
		bullet_template.group = BG_RING;
		bullet_template.spread = 16;
		bullet_template.origin.x.v += (
			randring2_next16_mod(TO_SP(64)) - TO_SP(32)
		);
		bullet_template.origin.y.v += (
			randring2_next16_mod(TO_SP(64)) - TO_SP(32)
		);
		bullet_template.angle = randring2_next16();
		bullets_add_regular();
		snd_se_play(3);
	}
	return (midboss.phase_frame >= 96);
}

// MIDBOSS5_PATTERNS_PHASE_1[2]: one aimed spread-stack of pellets and one aimed
// ring-stack of them, both on frame 32 and nothing after, until the cycle ends
// at 64. The `aimed` is BG_*_STACK_AIMED's own token; `pellet` is what
// [patnum] 0 means in TH05 (th04/main/bullet/bullet.hpp), and `stacks` is what
// both volleys share. Same construct, one boss over, as
// th05/main/boss/b3.cpp's alice_puppet_pattern_19928().
bool near pattern_aimed_pellet_stacks(void)
{
	if(midboss.phase_frame == 32) {
		bullet_template.spawn_type = (BST_CLOUD_FORWARDS | BST_NO_DECELERATE);
		bullet_template.patnum = 0; // pellet
		bullet_template.group = BG_SPREAD_STACK_AIMED;
		bullet_template.angle = 0x00;
		bullet_template.set_spread_stack(5, 0xA, 5, 0.375f);
		bullet_template.speed.set(2.0f);
		bullets_add_regular();

		bullet_template.group = BG_RING_STACK_AIMED;
		bullet_template.stack_speed_delta.set(0.25f);
		bullet_template.spread = 32;
		bullets_add_regular();
		snd_se_play(15);
	}
	return (midboss.phase_frame >= 64);
}

void pascal far midboss5_update(void)
{
	bullet_template.origin = midboss.pos.cur;
	gather_template.center = midboss.pos.cur;

	midboss.phase_frame++;

	switch(midboss.phase) {
	case 0:
		midboss.pos.update_seg3();
		midboss_hittest_shots_invincible(
			MIDBOSS5_HITBOX_RADIUS, MIDBOSS5_HITBOX_RADIUS
		);
		// Timeout condition
		if(midboss.phase_frame >= 192) {
			midboss.phase++;
			midboss.phase_frame = 0;
			midboss.angle = 0x00;
			flystep_cycle = 0;
			pattern = 1;
			patterns_done = 0;
			midboss.pos.velocity.y.set(0.0f);
			midboss5_phase_1_pattern = pattern_blue_spreads;
		}
		break;

	case 1:
		midboss.pos.prev = midboss.pos.cur;

		switch(pattern) {
		case 0:
			if(midboss5_flystep_sideways()) {
				midboss.phase_frame = 0;
				patterns_done++;
				midboss5_phase_1_pattern = MIDBOSS5_PATTERNS_PHASE_1[
					patterns_done % PATTERN_COUNT
				];
				pattern++;
				if(patterns_done >= PATTERNS_DONE_MAX) {
					goto defeated;
				}
			}
			break;
		case 1:
			midboss5_gather_and_pattern();
			break;
		}

		midboss_hittest_shots(
			MIDBOSS5_HITBOX_RADIUS, MIDBOSS5_HITBOX_RADIUS
		);
		if(midboss.hp > 0) {
			break;
		}
		bullet_zap.active = true;
		midboss_score_bonus(30);
		items_add(midboss.pos.cur.x, midboss.pos.cur.y, IT_1UP);
defeated:
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
		hud_hp_update_and_render(midboss.hp, MIDBOSS5_HP_TOTAL);
		return;
	}

	hud_hp_update_and_render(midboss.hp, MIDBOSS5_HP_TOTAL);
	homing_target.x = midboss.pos.cur.x;
	homing_target.y = midboss.pos.cur.y;
}

#undef patterns_done
#undef pattern
#undef flystep_cycle
