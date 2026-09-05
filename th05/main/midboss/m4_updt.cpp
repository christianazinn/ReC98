/// Stage 4 midboss - update
/// ------------------------
/// The renderer is th05/main/midboss/m4.cpp, in a different object; only the
/// update half and its four helpers live in main_035_TEXT. midboss4_update()
/// was the last emitting proc of th05_main.asm's contribution to that segment
/// -- the four-entry `dw offset loc_...` run below it is the jump table its
/// own pattern dispatch compiles to, not data the dump owns -- so this file
/// grows the C++ object that already follows the root backwards into the hole
/// (kb/codegen 0099 + 0112 + 0114 + 0129, state/re/JUMP_TABLE_TAILS.md).
///
/// (#included from th05/main/boss/b4_mai.cpp, ahead of that file's include of
/// th05/main/bullet/b4balls_add.cpp, because this code sits below the ball
/// bullets' in the original. The `-zCmain_035_TEXT -zPmain_03` pragma stays in
/// th05/b4mai.cpp: only the first file compiled into an object may name its
/// segment, kb/codegen/0112 trap 0.)
///
/// Structurally the twin of th05/main/midboss/m3_updt.cpp -- the same three
/// [boss_statebyte] slots in the same three roles, the same `phase` switch,
/// the same pattern selector and the same defeat tail -- and that file's own
/// comment says so. What the Stage 4 midboss does differently is its
/// reposition: instead of a flap that carries it somewhere, it vibrates
/// horizontally, warps, and vibrates to a stop.
///
/// This file owns th04/main/midboss/midboss.hpp and th04/main/spark.hpp, both
/// unguarded and both new to this translation unit -- nothing else compiled
/// into th05/b4mai.cpp reaches either (kb/codegen/0129).

#include "th04/main/phase.hpp"
#include "th04/main/item/item.hpp"
#include "th04/main/bullet/clearzap.hpp"
#include "th04/main/midboss/midboss.hpp"
#include "th04/main/spark.hpp"

// Constants
// ---------

static const pixel_t MIDBOSS4_W = 64;

// The same box, and the same formula, as th05/main/midboss/m3_updt.cpp's --
// one quarter of the sprite in from each side.
static const subpixel_t MIDBOSS4_HITBOX_RADIUS = (
	(TO_SP(MIDBOSS4_W) / 2) - (TO_SP(MIDBOSS4_W) / 8)
);

static const int MIDBOSS4_HP_TOTAL = 1100;

// st04.bmt. 208 is the cel the midboss idles on and 212 the one it attacks
// from; th05/main/midboss/m4.cpp adds ([stage_frame_mod16] / 4) to whichever
// of the two this file wrote last, so each is the first cel of a four-cel
// animation. [inferred] from which half of the fight writes which number;
// what they depict is not recoverable from the dump.
static const int PAT_MIDBOSS4_IDLE = 208;
static const int PAT_MIDBOSS4_ATTACK = 212;

// All three patterns run to the same schedule: nothing for the first
// FRAME_SETUP frames, the volley parameters on that frame, then one volley per
// multiple of 4 or 8 from FRAME_VOLLEY_first to FRAME_VOLLEY_last, and back to
// the selector at FRAME_DONE. The warp has the same lead-in.
static const int FRAME_SETUP = 16;
static const int FRAME_VOLLEY_first = 48;
static const int FRAME_VOLLEY_last = 80;
static const int FRAME_DONE = 96;

// [patterns_done] is incremented once per completed warp and the bound is
// tested *after* the increment, so the 16th warp is the one that kills the
// midboss. Spelled as the bound the code tests, the way m3_updt.cpp spells its
// own.
static const int PATTERNS_DONE_MAX = 0x10;

static const int PATTERN_COUNT = 3;

// Vibration amplitude grows by one step per frame up to this value and at half
// that rate above it, so the shake accelerates and then eases off.
static const int WARP_AMPLITUDE_TAPER = 20;

// How long each of the warp's two vibrations lasts, and therefore when the
// warp itself happens.
static const int WARP_FRAMES_PER_VIBRATION = 32;
static const int WARP_FRAMES = (WARP_FRAMES_PER_VIBRATION * 2);
// ---------

// State
// -----

// Same two slots, in the same two roles, as th05/main/midboss/m3_updt.cpp's.
#define patterns_done	boss_statebyte[13]

// 0 while the midboss is warping to its next position and midboss4_update() is
// picking the pattern to run there; 1 to PATTERN_COUNT while that pattern
// runs.
#define pattern      	boss_statebyte[14]

// 1 while the midboss is standing still, 0 while it is warping.
// th05/main/midboss/m4.cpp reads the same slot and names it
// `midboss4_blit_normally` there, noting that whichever state sets it is
// "still in ZUN's assembly" -- this file is that assembly, and the answer is
// that the two roles are one state: while the midboss warps it is drawn as a
// white silhouette AND takes no damage, and while it stands still it is drawn
// normally and can be hit. [inferred] only in that the two are spelled here as
// one state rather than as a coincidence of two.
#define midboss4_solid	boss_statebyte[15]

// The x the vibration is centered on: the midboss's own x on the way out, and
// the warp destination on the way in.
extern subpixel_t midboss4_warp_x;
// -----

// Vibrates the midboss horizontally for WARP_FRAMES_PER_VIBRATION frames,
// warps it to a random position along the top of the playfield, then vibrates
// it to a stop over the same number of frames again. Returns true on the frame
// the second vibration reaches zero amplitude, which is the frame the caller
// picks the next pattern on.
static bool near midboss4_warp(void)
{
	int amplitude;

	if(midboss.phase_frame < FRAME_SETUP) {
		return false;
	}
	amplitude = (midboss.phase_frame - FRAME_SETUP);
	if(amplitude < WARP_FRAMES_PER_VIBRATION) {
		if(amplitude == 0) {
			midboss4_solid = 0;
			snd_se_play(13);
			midboss4_warp_x = midboss.pos.cur.x.v;
		}
		midboss.pos.cur.x.v = midboss4_warp_x;
		if(amplitude > WARP_AMPLITUDE_TAPER) {
			amplitude = (
				((amplitude - WARP_AMPLITUDE_TAPER) / 2) + WARP_AMPLITUDE_TAPER
			);
		}
		if(stage_frame_mod2 != 0) {
			midboss.pos.cur.x.v += (amplitude << 5);
		} else {
			midboss.pos.cur.x.v -= (amplitude << 5);
		}
		return false;
	}
	if(amplitude == WARP_FRAMES_PER_VIBRATION) {
		// [inferred] Both ranges start one sprite width in from the edge they
		// are measured from; the dump only has the folded subpixel constants.
		midboss4_warp_x = (
			randring2_next16_mod(TO_SP(PLAYFIELD_W - (MIDBOSS4_W * 2))) +
			TO_SP(MIDBOSS4_W)
		);
		midboss.pos.cur.y.v = (
			randring2_next16_mod(TO_SP(MIDBOSS4_W / 2)) + TO_SP(MIDBOSS4_W)
		);
		return false;
	}
	amplitude = (WARP_FRAMES - amplitude);
	if(amplitude > WARP_AMPLITUDE_TAPER) {
		amplitude = (
			((amplitude - WARP_AMPLITUDE_TAPER) / 2) + WARP_AMPLITUDE_TAPER
		);
	}
	midboss.pos.cur.x.v = midboss4_warp_x;
	if(stage_frame_mod2 != 0) {
		midboss.pos.cur.x.v += (amplitude << 5);
	} else {
		midboss.pos.cur.x.v -= (amplitude << 5);
	}
	// `[measured]` Spelled with the `return true` inside the branch, not as an
	// early `return false`: the original falls through into the true path and
	// jumps to the shared `return false` that every test above it also jumps
	// to, and the other spelling inverts this jump and swaps the two tails.
	if(amplitude == 0) {
		midboss4_solid = 1;
		return true;
	}
	return false;
}

// Four bullets per volley, each from a random point in a 48-pixel box around
// the midboss, each with a random angle and speed, and every other one a
// pellet rather than a 16x16 bullet.
static void near pattern_random_clouds(void)
{
	int i;

	if(midboss.phase_frame == FRAME_SETUP) {
		snd_se_play(8);
		bullet_template.spawn_type = BST_CLOUD_FORWARDS;
		bullet_template.group = BG_RANDOM_ANGLE_AND_SPEED;
		bullet_template.spread = 7;
		bullet_template_tune();
		midboss.sprite = PAT_MIDBOSS4_ATTACK;
	}
	if(
		(midboss.phase_frame >= FRAME_VOLLEY_first) &&
		(midboss.phase_frame <= FRAME_VOLLEY_last) &&
		((midboss.phase_frame % 8) == 0)
	) {
		bullet_template.speed.set(0.5f);
		for(i = 0; i < 4; i++) {
			bullet_template.patnum = (
				(i & 1) ? PAT_BULLET16_V_BLUE : 0 /* pellet */
			);
			bullet_template.origin.x.v = (
				randring2_next16_mod(TO_SP(48)) +
				midboss.pos.cur.x.v -
				TO_SP(24)
			);
			bullet_template.origin.y.v = (
				randring2_next16_mod(TO_SP(48)) +
				midboss.pos.cur.y.v -
				TO_SP(32)
			);
			bullets_add_regular_fixedspeed();
			bullet_template.speed.v += TO_SP(1);
		}
		snd_se_play(15);
	}
	if(midboss.phase_frame >= FRAME_DONE) {
		midboss.sprite = PAT_MIDBOSS4_IDLE;
		midboss.phase_frame = 0;
		pattern = 0;
	}
}

// A 16-way ring every 4th frame, rotated by 4 angle units each time, with
// every other one spawned as a cloud rather than directly.
static void near pattern_rotating_rings(void)
{
	if(midboss.phase_frame == FRAME_SETUP) {
		snd_se_play(8);
		bullet_template.group = BG_RING;
		bullet_template.spread = 16;
		bullet_template_tune();
		midboss.sprite = PAT_MIDBOSS4_ATTACK;
		bullet_template.patnum = PAT_BULLET16_V_RED;
	}
	if(
		(midboss.phase_frame >= FRAME_VOLLEY_first) &&
		(midboss.phase_frame <= FRAME_VOLLEY_last) &&
		((midboss.phase_frame % 4) == 0)
	) {
		if((midboss.phase_frame % 8) == 0) {
			bullet_template.speed.set(3.5f);
			bullet_template.spawn_type = BST_CLOUD_FORWARDS;
		} else {
			bullet_template.speed.set(2.5f);
			bullet_template.spawn_type = BST_NORMAL;
		}
		bullets_add_regular();
		bullet_template.angle += 4;
		snd_se_play(3);
	}
	if(midboss.phase_frame >= FRAME_DONE) {
		midboss.sprite = PAT_MIDBOSS4_IDLE;
		midboss.phase_frame = 0;
		pattern = 0;
	}
}

// Aimed spread-stacks every 4th frame, each one faster than the last and aimed
// 8 angle units further to one side, so the fan sweeps as it speeds up.
static void near pattern_sweeping_stacks(void)
{
	if(midboss.phase_frame == FRAME_SETUP) {
		snd_se_play(8);
		bullet_template.spawn_type = (BST_CLOUD_FORWARDS | BST_NO_DECELERATE);
		bullet_template.group = BG_SPREAD_STACK_AIMED;
		bullet_template.angle = 0x20;
		bullet_template.set_spread_stack(3, 0x08, 4, 0.25f);
		bullet_template.speed.set(1.0f);
		bullet_template.patnum = PAT_BULLET16_N_BALL_BLUE;
		bullet_template_tune();
		midboss.sprite = PAT_MIDBOSS4_ATTACK;
	}
	if(
		(midboss.phase_frame >= FRAME_VOLLEY_first) &&
		(midboss.phase_frame <= FRAME_VOLLEY_last) &&
		((midboss.phase_frame % 4) == 0)
	) {
		bullets_add_regular();
		bullet_template.speed.v += 12;
		bullet_template.angle -= 8;
		snd_se_play(15);
	}
	if(midboss.phase_frame >= FRAME_DONE) {
		midboss.sprite = PAT_MIDBOSS4_IDLE;
		midboss.phase_frame = 0;
		pattern = 0;
	}
}

#pragma option -a2

void pascal far midboss4_update(void)
{
	bullet_template.origin = midboss.pos.cur;
	gather_template.center = midboss.pos.cur;

	midboss.phase_frame++;

	switch(midboss.phase) {
	case 0:
		if(midboss.phase_frame == 1) {
			midboss4_solid = 1;
		}
		midboss.pos.update_seg3();
		midboss_hittest_shots_invincible(
			MIDBOSS4_HITBOX_RADIUS, MIDBOSS4_HITBOX_RADIUS
		);
		// Timeout condition
		if(midboss.phase_frame >= 192) {
			midboss.phase++;
			midboss.phase_frame = 0;
			midboss.angle = 0x00;
			pattern = 1;
			patterns_done = 0;
			midboss.pos.velocity.x.set(0.0f);
		}
		break;

	case 1:
		midboss.pos.prev = midboss.pos.cur;

		switch(pattern) {
		case 0:
			if(midboss4_warp()) {
				midboss.phase_frame = 0;
				patterns_done++;
				pattern = ((patterns_done % PATTERN_COUNT) + 1);
				if(patterns_done >= PATTERNS_DONE_MAX) {
					goto defeated;
				}
			}
			break;
		case 1:
			pattern_random_clouds();
			break;
		case 2:
			pattern_rotating_rings();
			break;
		case 3:
			pattern_sweeping_stacks();
			break;
		}

		// Invincible for as long as it is warping.
		if(midboss4_solid) {
			midboss_hittest_shots(
				MIDBOSS4_HITBOX_RADIUS, MIDBOSS4_HITBOX_RADIUS
			);
		}
		if(midboss.hp > 0) {
			break;
		}
		bullet_zap.active = true;
		midboss_score_bonus(15);
		items_add(midboss.pos.cur.x, midboss.pos.cur.y, IT_BOMB);
defeated:
		midboss.phase = PHASE_EXPLODE_BIG;
		midboss.sprite = PAT_ENEMY_KILL;
		midboss.phase_frame = 0;
		sparks_add_circle(
			midboss.pos.cur.x, midboss.pos.cur.y, (TO_SP(MIDBOSS4_W) / 8), 48
		);
		snd_se_play(12);
		break;

	default:
		midboss_defeat_update();
		hud_hp_update_and_render(midboss.hp, MIDBOSS4_HP_TOTAL);
		return;
	}

	hud_hp_update_and_render(midboss.hp, MIDBOSS4_HP_TOTAL);
	homing_target.x = midboss.pos.cur.x;
	homing_target.y = midboss.pos.cur.y;
}

#pragma option -a1

// Unprefixed tokens in a file that is #included into th05/b4mai.cpp's
// translation unit: without these they leak into the ball bullets and both
// halves of the Stage 4 boss fight below, all of which use [boss_statebyte]
// directly.
#undef patterns_done
#undef pattern
#undef midboss4_solid
