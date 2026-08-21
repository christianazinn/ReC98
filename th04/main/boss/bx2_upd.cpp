/// Extra Boss 2 - Gengetsu: the fight's own update function
/// --------------------------------------------------------
/// (#included from th04/main_036.cpp. main_036_TEXT has no other C++
/// contribution, and TLINK lays a segment's contributions out in link order
/// with the root dump first, so this object lands at the segment's tail by
/// construction — which is where this function already was.
/// kb/codegen/0112 + 0114.)
///
/// gengetsu_fg_render() and the background both live elsewhere;
/// th04/main/boss/bx2.hpp holds what this fight shares with them.

#include "platform.h"
#include "pc98.h"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/v_colors.hpp"
#include "th02/main/player/bomb.hpp"
#include "th02/main/player/player.hpp"
#include "th03/hardware/palette.hpp"
#include "th04/snd/snd.h"
#include "th04/sprites/main_pat.h"
#include "th04/math/randring.hpp"
#include "th04/main/bg.hpp"
#include "th04/main/homing.hpp"
#include "th04/main/hud/hud.hpp"
#include "th04/main/player/player.hpp"
#include "th04/main/bullet/bullet.hpp"
#include "th04/main/bullet/clearzap.hpp"
#include "th04/main/bullet/laser_t.hpp"
#include "th04/main/tile/bb.hpp"
#include "th04/main/boss/boss.hpp"
#include "th04/main/boss/bx2.hpp"
#include "th04/main/circle.hpp"
#include "th04/main/gather.hpp"

// Unguarded, like the two above -- but th04/main_036.cpp #includes this file
// and nothing else, so nothing in this object can expand any of them a second
// time (kb/codegen/0129).
#include "th04/main/frames.h"

/// Still ASM
/// ---------
// libs/master.lib/master.hpp, which pulls in far too much for this object.
// `far pascal` is MASTER_RET under the large model (libs/master.lib/func.hpp).
extern "C" int far pascal iatan2(int y, int x);

extern "C" {
	// Copies [thicklaser_template] into the first TF_FREE slot and plays the
	// spawn sound effect. th04_main.asm's `sub_15DBD`, which is in
	// B4M_UPDATE_TEXT rather than this segment -- but the same main_03 group,
	// so the call stays near. It is private to ZUN's object, so this parcel
	// gave it the zero-byte `label` alias that makes it linkable at all
	// (kb/codegen/0123); the name is `[inferred]` from the body, which is the
	// sole caller of thicklaser_template_pull() and the counterpart of
	// thicklasers_update_and_hittest(). **A naming round is owed for it.**
	void near thicklaser_add(void);
}

// Shared between Mugetsu and Gengetsu, and th04_main.asm publishes neither it
// nor a name for it. `[inferred]`, and **a naming round is owed**: a bomb sets
// it to 32 and every frame decrements it towards 0, and the only thing that
// reads it is the two Extra bosses' hittest helpers, which take the
// invincibility path while it is nonzero.
extern "C" unsigned char extra_boss_bomb_immunity;

// Declared FAR here, and only here: th04/main/boss/bosses.hpp declares the
// same function `near`, which is what it is, and that header is deliberately
// not included. A near reference under this object's `-zPmain_03` frames its
// offset on main_03, and mugetsu_gengetsu_bg_render() lives in main_01.
// kb/codegen/0162.
void pascal far mugetsu_gengetsu_bg_render(void);
/// ---------

/// Constants
/// ---------
static const int GENGETSU_HP = 18700;

// th04/main/boss/bx2.cpp's, repeated because that file is a translation unit
// rather than a header.
static const pixel_t WAVE_TARGET_MARGIN = (PLAYFIELD_W / 12);

// Frames of bomb immunity. The value is a bomb's length, not a constant this
// fight owns.
static const int BOMB_IMMUNITY_FRAMES = 32;

// [boss.phase_state] counts teleports, not patterns: past this many, every
// remaining phase adds gengetsu_2023B() on top of whatever else it runs…
static const int TELEPORTS_UNTIL_FILLER = 18;

// …and past THIS many the phase can no longer be ended by damage at all.
static const int TELEPORTS_UNTIL_UNKILLABLE = 22;
/// ---------

/// State
/// -----
/// The four [boss_statebyte] slots Gengetsu's patterns use, under the names
/// th04_main.asm's own `boss_statebyte_t` overlay gives them. That overlay is
/// a `union`, so the first three are the SAME byte 15 -- one slot, reused by
/// patterns that can never run at the same time, and the dump keeps all three
/// spellings because the three meanings have nothing to do with each other.

// The spread angle the phase-4 pattern carries across frames.
#define gengetsu_spread_angle  boss_statebyte[15]

// The aim the spawn-column pattern's charge leaves behind, and the angle its
// pellet stacks then sweep out of.
#define pellet_stack_angle     boss_statebyte[15]

// Horizontal offset of a pattern's spawn point from Gengetsu's own position.
#define origin_offset_x        boss_statebyte[15]

// Base angle of the cluster the phase-5 pattern fires.
#define cluster_angle          boss_statebyte[14]
/// -----

// The one thing the middle four phases do not share: phase 2 advances
// [boss.phase_state] BEFORE deriving [boss.mode] from it, and the other three
// do it after. Both spellings are one `INC` and one `AND`, in the two possible
// orders, so this is ZUN's and not a compiler artifact.
#define GENGETSU_TELEPORT_FIRST { \
	boss.phase_state.patterns_seen++; \
	boss.mode = (boss.phase_state.patterns_seen & 1); \
}
#define GENGETSU_TELEPORT_LATER { \
	boss.mode = (boss.phase_state.patterns_seen & 1); \
	boss.phase_state.patterns_seen++; \
}

// Phases 2 through 5, which differ only in their two patterns, in that one
// ordering above, and in what the phase ends into.
#define gengetsu_pattern_phase( \
	pattern_0, pattern_1, on_teleport_done, explosion, next_end_hp \
) { \
	switch(boss.mode) { \
	case 0: \
		pattern_0(); \
		break; \
	case 1: \
		pattern_1(); \
		break; \
	case 255: \
		if(boss.phase_frame == 1) { \
			/* Every other teleport goes to the player, clamped away from */ \
			/* the playfield edges; the ones in between go anywhere in the */ \
			/* middle two thirds. */ \
			if(boss.phase_state.patterns_seen & 1) { \
				gengetsu_wave_target_x.v = player_pos.cur.x.v; \
				if(gengetsu_wave_target_x.v < TO_SP(WAVE_TARGET_MARGIN)) { \
					gengetsu_wave_target_x.v = TO_SP(WAVE_TARGET_MARGIN); \
				} else if(gengetsu_wave_target_x.v > TO_SP( \
					PLAYFIELD_W - WAVE_TARGET_MARGIN \
				)) { \
					gengetsu_wave_target_x.v = TO_SP( \
						PLAYFIELD_W - WAVE_TARGET_MARGIN \
					); \
				} \
			} else { \
				gengetsu_wave_target_x.v = (randring2_next16_mod(TO_SP( \
					PLAYFIELD_W - (WAVE_TARGET_MARGIN * 4) \
				)) + TO_SP(WAVE_TARGET_MARGIN * 2)); \
			} \
		} \
		if(gengetsu_1F97A()) { \
			on_teleport_done; \
			boss.phase_frame = 0; \
		} \
		break; \
	} \
	if( \
		(boss.phase_state.patterns_seen >= TELEPORTS_UNTIL_FILLER) && \
		(boss.mode != 255) \
	) { \
		gengetsu_2023B(); \
	} \
	if(boss.phase_state.patterns_seen < TELEPORTS_UNTIL_UNKILLABLE) { \
		if(!gengetsu_20202()) { \
			break; \
		} \
		boss_score_bonus(100); \
	} \
	boss_phase_next(explosion, next_end_hp); \
	boss.mode = 255; \
	gengetsu_wave_target_x.v = TO_SP(PLAYFIELD_W / 2); \
	break; \
}

// Adds the mirrored pair of gather circles that make up one step of
// gengetsu_1F903()'s stack: the template's own ring, and a second one
// counter-rotating against it. Everything else about them -- center, radius,
// point count, color -- is whatever the caller left in [gather_template].
static void near gengetsu_1F8EE(void)
{
	gather_template.angle_delta = -2;
	gather_add_only();
	gather_template.angle_delta = 2;
	gather_add_only();
}

// The charge animation in front of each of Gengetsu's late patterns, run
// every frame and doing nothing on all but four of them. It is
// gather_add_only_3stack() rewritten with the pair above in place of the
// single circle, on an absolute [boss.phase_frame] schedule rather than a
// relative one: three steps two frames apart, white for the first and color 9
// for the other two -- that function's exact color scheme -- and then a
// shrinking circle onto the same center 12 frames after the last of them.
//
// The center is her bullet origin, not [boss.pos]: the same
// (-13, -48) offset gengetsu_update() puts into [bullet_template].
//
// `-a2` is for the one pad byte between the epilogue and the generated
// switch table. It is parity-dependent on where this object's code
// contribution starts, so re-probe it if anything is ever added ahead of
// gengetsu_1F8EE() -- kb/codegen/0139 and 0154.
#pragma option -a2
static void near gengetsu_1F903(void)
{
	switch(boss.phase_frame) {
	case 48:
		gather_template.radius.v = TO_SP(320);
		gather_template.center.y.v = (boss.pos.cur.y.v - TO_SP(48));
		gather_template.center.x.v = (boss.pos.cur.x.v - TO_SP(13));
		gather_template.ring_points = 16;
		gather_template.col = V_WHITE;
		gengetsu_1F8EE();
		break;
	case 50:
		gather_template.col = 9;
		gengetsu_1F8EE();
		break;
	case 52:
		gengetsu_1F8EE();
		break;
	case 64:
		circles_add_shrinking(
			gather_template.center.x.v, gather_template.center.y.v
		);
		circles_color = V_WHITE;
		break;
	}
}
#pragma option -a1

// Runs the teleport, and returns `true` on the frame it lands.
// The horizontal velocity is set once, on the first frame, to cover the whole
// distance to [gengetsu_wave_target_x] in the 64 frames the animation lasts;
// the wave amplitude rises for the first half and falls for the second, and
// is zeroed rather than left at whatever the ±2 steps ended on.
static bool near gengetsu_1F97A(void)
{
	if(boss.phase_frame == 1) {
		boss.pos.velocity.x.v = (
			(gengetsu_wave_target_x.v - boss.pos.cur.x.v) / 64
		);
	}
	boss.pos.cur.x.v += boss.pos.velocity.x.v;
	if(boss.phase_frame <= 32) {
		gengetsu_wave_amp += 2;
	} else {
		gengetsu_wave_amp -= 2;
	}
	if(boss.phase_frame == 64) {
		gengetsu_wave_amp = 0;
		return true;
	}
	return false;
}

// Phase 6's one-shot, returning `true` once it is over.
// The same wave as the teleport above, over twice as many frames at half the
// rate, so it peaks at the same amplitude. The destination is hardcoded to
// the center of the playfield rather than aimed: a fixed ±TO_SP(2) per frame
// towards TO_SP(192) from whichever side she starts on, stopping as soon as
// she is past it. The two stop tests are not mirror images — leftward stops
// below TO_SP(193) and rightward above TO_SP(192) — so a right-hand approach
// parks a pixel short, and only the frame-128 snap puts her on TO_SP(192)
// exactly.
static bool near gengetsu_1F9C5(void)
{
	if(boss.phase_frame == 1) {
		boss.pos.velocity.x.v = (
			(boss.pos.cur.x.v < TO_SP(192)) ? TO_SP(2) : -TO_SP(2)
		);
	}
	if(
		((boss.pos.velocity.x.v < 0) && (boss.pos.cur.x.v >= TO_SP(193))) ||
		((boss.pos.velocity.x.v > 0) && (boss.pos.cur.x.v <= TO_SP(192)))
	) {
		boss.pos.cur.x.v += boss.pos.velocity.x.v;
	}
	if(boss.phase_frame <= 64) {
		gengetsu_wave_amp++;
	} else {
		gengetsu_wave_amp--;
	}
	if(boss.phase_frame == 128) {
		boss.pos.cur.x.v = TO_SP(192);
		gengetsu_wave_amp = 0;
		return true;
	}
	return false;
}

// The 144-frame schedule that all eight of the phase-2-to-5 patterns open
// with, and the only thing that animates Gengetsu during them. Runs the
// gather/circle animation every frame, then tells the pattern which stage of
// that schedule this frame is:
//
// • 0 — the pattern must not do anything yet. Either the first 8 frames, or
//   any later frame on which the sprite is still one of the sub-32 cels the
//   teleport and the damage animations own.
// • 1 — charging, frames 8 to 79, with a sound effect at frame 32 and a
//   two-cel flicker for as long as it lasts.
// • 2 — frame 80, the one frame the pattern fires on.
// • 3 — frames 81 to 143, the tail the every-other-frame patterns keep
//   spraying through.
// • 4 — frame 144 and up: the pattern is over, and every caller answers this
//   by resetting [boss.phase_frame] and handing back to the teleport.
//
// **A naming round is owed.** Its role is exactly that of
// marisa_charge_animate() (th04/main/boss/b4m_upd.cpp) — a per-pattern
// charge-up whose return value IS the pattern's own schedule — so the mirror
// rule on that family is where a name should come from. Kept address-suffixed
// here only because
// its five remaining callers are still ASM and would have to keep the dump's
// spelling anyway.
static unsigned char near gengetsu_1FA33(void)
{
	gengetsu_1F903();
	if(boss.phase_frame < 8) {
		return 0;
	}
	if(boss.phase_frame == 8) {
		boss.sprite = 130;
		return 0;
	}
	if(boss.sprite < 32) {
		return 0;
	}
	if(boss.phase_frame < 80) {
		if(boss.phase_frame == 32) {
			snd_se_play(8);
		}
		if(stage_frame_mod2 != 0) {
			boss.sprite = 134;
		} else {
			boss.sprite = 130;
		}
		return 1;
	}
	if(boss.phase_frame == 80) {
		boss.sprite = 132;
		return 2;
	}
	if(boss.phase_frame < 144) {
		return 3;
	}
	boss.sprite = 132;
	return 4;
}

// Phase 2, [boss.mode] 0: one 90-bullet ring of red heart balls, fired as
// backwards clouds from a random angle, and nothing else for the remaining 64
// frames.
static void near gengetsu_1FAAA(void)
{
	switch(gengetsu_1FA33()) {
	case 2:
		bullet_template.spawn_type = BST_BULLET16_CLOUD_BACKWARDS;
		bullet_template.patnum = PAT_BULLET16_N_HEART_BALL_RED;
		bullet_template.speed.v = (TO_SP(4) + 6);
		bullet_template.group = BG_RING;
		bullet_template.count = 90;
		bullet_template.angle = randring2_next16();
		bullets_add_regular();
		snd_se_play(9);
		break;
	case 4:
		boss.phase_frame = 0;
		boss.mode = 255;
		break;
	}
}

// Phase 2, [boss.mode] 1: a counter-rotating pair of pellet spreads. The fire
// frame only aims — it parks (angle to the player - 0x40) in the state byte —
// and then every other frame of the tail fires one 5-to-8-bullet spread at
// that angle and a second one at its mirror image around 0x40, each volley
// with a new random speed. Advancing the state byte by 7 rotates the first
// spread clockwise and the mirrored one counter-clockwise, so the pair opens
// away from each other.
static void near gengetsu_1FAF7(void)
{
	switch(gengetsu_1FA33()) {
	case 2:
		bullet_template.group = BG_SPREAD;
		bullet_template.delta.spread_angle = 8;
		_AL = iatan2(
			(player_pos.cur.y.v - bullet_template.origin.y.v),
			(player_pos.cur.x.v - bullet_template.origin.x.v)
		);
		_AL += -0x40;
		gengetsu_spread_angle = _AL;
		break;
	case 3:
		if(stage_frame_mod2 == 0) {
			_AL = randring2_next16_and(0x3F);
			_AL += 8;
			bullet_template.speed.v = _AL;
			_AL = randring2_next16_and(3);
			_AL += 5;
			bullet_template.count = _AL;
			bullet_template.angle = gengetsu_spread_angle;
			bullets_add_regular();
			bullet_template.angle = (0x80 - gengetsu_spread_angle);
			bullets_add_regular();
			gengetsu_spread_angle += 7;
			snd_se_play(9);
		}
		break;
	case 4:
		boss.phase_frame = 0;
		boss.mode = 255;
		break;
	}
}

// Phase 3, [boss.mode] 0: bouncing crosses. The fire frame only arms the
// template for up to 4 bounces off any playfield edge; every other frame of
// the tail then launches two yellow crosses diagonally upwards, at a random
// speed each and from a random point in the 64×32 pixel box directly above
// Gengetsu. Every 16th frame adds an aimed 32-bullet ring of blue directional
// bullets on top, which are regular bullets and therefore do not bounce.
static void near gengetsu_1FB86(void)
{
	switch(gengetsu_1FA33()) {
	case 2:
		bullet_template.special_motion = BSM_BOUNCE_LEFT_RIGHT_TOP_BOTTOM;
		bullet_special.turns_max = 4;
		break;
	case 3:
		if(stage_frame_mod2 != 0) {
			bullet_template.group = BG_SINGLE;
			bullet_template.spawn_type = BST_BULLET16;
			bullet_template.patnum = PAT_BULLET16_N_CROSS_YELLOW;
			_AL = randring2_next16_and(0x3F);
			_AL += (TO_SP(2) + 10);
			bullet_template.speed.v = _AL;
			bullet_template.origin.x.v = (randring2_next16_mod(TO_SP(64)) + (
				boss.pos.cur.x.v - TO_SP(32)
			));
			bullet_template.origin.y.v = (randring2_next16_mod(TO_SP(32)) + (
				boss.pos.cur.y.v - TO_SP(32)
			));
			bullet_template.angle = -0x20;
			bullets_add_special();
			_AL = randring2_next16_and(0x3F);
			_AL += (TO_SP(2) + 10);
			bullet_template.speed.v = _AL;
			bullet_template.angle = -0x60;
			bullets_add_special();
			snd_se_play(9);
			if(stage_frame_mod16 == 1) {
				bullet_template.patnum = PAT_BULLET16_D_BLUE;
				bullet_template.group = BG_RING_AIMED;
				bullet_template.count = 32;
				bullet_template.speed.v = TO_SP(5);
				bullets_add_regular();
			}
		}
		break;
	case 4:
		boss.phase_frame = 0;
		boss.mode = 255;
		break;
	}
}

/// Gengetsu's patterns
/// -------------------
/// Four of the eight that [boss.mode] 0 and 1 alternate between during phases
/// 2 to 5. Every one of them is nothing but a dispatch over gengetsu_1FA33()'s
/// stage, and every one of them ends the same way: stage 4 rewinds
/// [boss.phase_frame] and hands [boss.mode] back to gengetsu_update()'s
/// teleport arm.

// Phase 3, [boss.mode] 1. A stream of decelerating 16x16 balls that scatter
// out of a box around her and then each turn a quarter circle, half of them
// clockwise and half counter-clockwise, with a wide aimed ring dropping out of
// a gather circle somewhere across the playfield on every 4th frame.
static void near gengetsu_1FC46(void)
{
	switch(gengetsu_1FA33()) {
	case 2:
		// One direction change per bullet, and only one.
		bullet_special.turns_max = 1;
		break;

	case 3:
		if(stage_frame_mod2 != 0) {
			bullet_template.special_motion = BSM_DECELERATE_THEN_TURN;
			bullet_template.group = BG_SINGLE;
			bullet_template.spawn_type = BST_BULLET16;
			bullet_template.patnum = PAT_BULLET16_N_SMALL_BALL_YELLOW;

			// Rolled once here and then immediately rolled again below the
			// origin: the first of the two speeds is dead. Both are
			// kb/codegen/0032 AL arithmetic on the returned byte.
			bullet_template.speed.v = (randring2_next16_and(0x3F) + TO_SP(1));

			bullet_template.origin.x.v = (
				randring2_next16_mod(TO_SP(64)) +
				(boss.pos.cur.x.v - TO_SP(32))
			);
			bullet_template.origin.y.v = (
				randring2_next16_mod(TO_SP(32)) +
				(boss.pos.cur.y.v - TO_SP(26))
			);
			bullet_template.speed.v = (randring2_next16_and(0x3F) + TO_SP(1));

			bullet_template.angle = 0x80;
			bullet_template_special_angle.turn_by = -0x40;
			bullets_add_special();
			bullet_template.angle = 0x00;
			bullet_template_special_angle.turn_by = 0x40;
			bullets_add_special();
			snd_se_play(9);
		}
		if(stage_frame_mod4 == 0) {
			gather_template.ring_points = 8;
			gather_template.radius.v = TO_SP(64);
			gather_template.col = 14;
			bullet_template.spawn_type = BST_PELLET;

			// The Y of whichever bullet the block above placed last, and an
			// unrelated random X: the ring is not tied to her position at all.
			gather_template.center.y.v = bullet_template.origin.y.v;
			gather_template.center.x.v = (
				randring2_next16_mod(TO_SP(320)) + TO_SP(32)
			);

			bullet_template.group = BG_RING_AIMED;
			bullet_template.count = 16;
			bullet_template.speed.v = TO_SP(4);
			gather_add_bullets();
		}
		break;

	case 4:
		boss.phase_frame = 0;
		boss.mode = -1;
		break;
	}
}

// Phase 4, [boss.mode] 0. The wind-up is the pattern: every 16th frame fires
// four fixed-speed cloud bullets at 1.0, 2.25, 3.5 and 4.75, and each volley
// then widens the aimed spread by two bullets while narrowing its angle by 2.
// The fire frame itself only swaps the template over to the aimed stacks that
// the pattern's second half drops every 4th frame.
static void near gengetsu_1FD30(void)
{
	int i;

	switch(gengetsu_1FA33()) {
	case 0:
		bullet_template.patnum = PAT_BULLET16_N_HEART_BALL_RED;
		bullet_template.group = BG_SPREAD_AIMED;
		bullet_template.special_motion = BSM_NONE;
		bullet_template.delta.spread_angle = 0x10;
		bullet_template.angle = 0;
		bullet_template.count = 2;
		break;

	case 1:
		if((boss.phase_frame & 0x0F) != 8) {
			break;
		}
		bullet_template.spawn_type = BST_BULLET16_CLOUD_FORWARDS;

		// A bottom-tested loop rather than a counted one, because the index is
		// incremented AHEAD of the speed step: the original's `INC SI` sits
		// between the call and the `ADD AL`, where a counted loop's body would
		// put it after both.
		i = 0;
		bullet_template.speed.v = TO_SP(1);
		while(i < 4) {
			bullets_add_special_fixedspeed();
			i++;

			// kb/codegen/0094: the addend has to stay `int`-typed, or this
			// AL round trip folds into an `ADD byte ptr [mem], imm8`.
			bullet_template.speed.v += (TO_SP(1) + 4);
		}
		snd_se_play(15);
		bullet_template.count += 2;
		bullet_template.delta.spread_angle -= 2;
		break;

	case 2:
		bullet_template.group = BG_STACK_AIMED;
		bullet_template.count = 8;
		bullet_template.delta.stack_speed.v = 10;
		bullet_template.speed.v = TO_SP(2);
		bullet_template.patnum = PAT_BULLET16_N_BALL_BLUE;
		bullet_template.angle = 0;
		break;

	case 3:
		if((boss.phase_frame & 3) != 0) {
			break;
		}
		bullet_template.spawn_type = BST_BULLET16_CLOUD_FORWARDS;
		bullets_add_regular();
		snd_se_play(15);
		break;

	case 4:
		boss.phase_frame = 0;
		boss.mode = -1;
		break;
	}
}

// Phase 4, [boss.mode] 1. One aimed 5.0 bullet on the fire frame, then a
// spread around that same frozen angle that gains one more bullet on every 4th
// frame -- so the fan grows for as long as the pattern runs, and never re-aims.
static void near gengetsu_1FDFE(void)
{
	switch(gengetsu_1FA33()) {
	case 2:
		bullet_template.speed.v = TO_SP(5);
		bullet_template.group = BG_SPREAD;
		bullet_template.count = 1;
		bullet_template.delta.spread_angle = 6;

		// Aimed from the bullet ORIGIN, which gengetsu_update() parks 13
		// pixels left and 48 pixels above her -- not from [boss.pos].
		bullet_template.angle = iatan2(
			(player_pos.cur.y.v - bullet_template.origin.y.v),
			(player_pos.cur.x.v - bullet_template.origin.x.v)
		);
		break;

	case 3:
		if(stage_frame_mod4 == 0) {
			bullets_add_regular();

			// kb/codegen/0094: the increment operator, not an add of one. Those
			// are two different codegens for a byte global, and only this one is
			// `INC mem`.
			bullet_template.count++;
			snd_se_play(9);
		}
		break;

	case 4:
		boss.phase_frame = 0;
		boss.mode = -1;
		break;
	}
}

// Phase 5, [boss.mode] 0. Two opposed 5-way fans of directional blue bullets,
// fired on every other frame. Each pair mirrors the angle around 0x40 and then
// around 0x3C, which nets the whole thing 8 units backwards per frame pair.
static void near gengetsu_1FE6A(void)
{
	switch(gengetsu_1FA33()) {
	case 2:
		bullet_template.patnum = PAT_BULLET16_D_BLUE;
		bullet_template.group = BG_SPREAD;
		bullet_template.count = 5;
		bullet_template.delta.spread_angle = 0x0B;
		bullet_template.angle = 0;
		bullet_template.speed.v = (TO_SP(5) + 10);
		break;

	case 3:
		if(stage_frame_mod2 == 0) {
			bullet_template.spawn_type = BST_BULLET16;
			bullets_add_regular();
			bullet_template.angle = (0x80 - bullet_template.angle);
			bullets_add_regular();
			bullet_template.angle = (0x78 - bullet_template.angle);
			snd_se_play(3);
		}
		break;

	case 4:
		boss.phase_frame = 0;
		boss.mode = -1;
		break;
	}
}
/// -------------------

// Phase 5's second pattern, and the one the [custom_entities] block exists
// for: the columns that gengetsu_fg_render() telegraphs as vertical lines
// (its GENGETSU_PHASE_SPAWNCOLUMNS and GENGETSU_MODE_SPAWNCOLUMNS are this
// phase and this mode). All 16 are seeded on the pattern's first frame, with
// a single coin flip deciding for all of them at once whether they sit
// exactly on a 24-pixel grid or anywhere within 12 pixels of each grid cell.
//
// The switch below is then the four stages of gengetsu_1FA33()'s schedule:
// the charge fires a five-way spread of white 16x16 balls at a fixed 0xC0 on
// every frame while continuously re-aiming [pellet_stack_angle] at the
// player, the fire frame spawns a thick laser down her own column, and the 63
// frames after that rain the seeded columns down on every 4th frame, with a
// pellet stack sweeping out of the aim the charge left behind on every 8th.
static void near gengetsu_1FEDF(void)
{
	unsigned char columns_aligned;
	gengetsu_spawncolumn_t near *column;
	int i;

	if(boss.phase_frame == 1) {
		columns_aligned = randring2_next16_and(1);
		column = gengetsu_spawncolumns;
		for(i = 0; i < GENGETSU_SPAWNCOLUMN_COUNT; i++, column++) {
			if(columns_aligned == 0) {
				column->pos.x.v = (
					(i * TO_SP(PLAYFIELD_W / GENGETSU_SPAWNCOLUMN_COUNT)) +
					TO_SP(12)
				);
			} else {
				column->pos.x.v = (randring2_next16_mod(TO_SP(12)) + (
					(i * TO_SP(PLAYFIELD_W / GENGETSU_SPAWNCOLUMN_COUNT)) +
					TO_SP(6)
				));
			}
			column->pos.y.v = 0;
		}
	}
	switch(gengetsu_1FA33()) {
	case 1:
		bullet_template.spawn_type = BST_BULLET16;
		bullet_template.group = BG_SPREAD;
		bullet_template.count = 5;
		bullet_template.delta.spread_angle = 0x18;
		bullet_template.angle = 0xC0;
		bullet_template.patnum = PAT_BULLET16_N_OUTLINED_BALL_WHITE;
		bullet_template.speed.v = (TO_SP(7) + 15);
		bullet_template_tune();
		bullets_add_regular();
		snd_se_play(3);

		// Re-aimed on every frame of the charge, so the stack in `case 3`
		// starts from wherever the player was on its last one.
		pellet_stack_angle = (iatan2(
			(player_pos.cur.y.v - bullet_template.origin.y.v),
			(player_pos.cur.x.v - bullet_template.origin.x.v)
		) - 0x30);
		break;

	case 2:
		// Her own X, but not her own Y: the origin is gengetsu_update()'s, 13
		// pixels to the left of her center and 48 above it.
		thicklaser_template.origin.x.v = bullet_template.origin.x.v;
		thicklaser_template.origin.y.v = boss.pos.cur.y.v;
		thicklaser_template.radius_max = 64;
		thicklaser_template.radius_speed = 6;
		thicklaser_template.line_frames = 32;
		thicklaser_template.static_frames = 48;
		thicklaser_template.col_outline = 8;
		thicklaser_add();
		break;

	case 3:
		if(stage_frame_mod4 != 0) {
			break;
		}
		if(stage_frame_mod8 == 0) {
			bullet_template.spawn_type = BST_PELLET;
			bullet_template.angle = pellet_stack_angle;
			bullet_template.group = BG_STACK;
			bullet_template.count = 12;
			bullet_template.speed.v = TO_SP(2);
			bullet_template.delta.stack_speed.v = 8;
			pellet_stack_angle += 0x0C;
			bullets_add_regular();
		}

		// One bullet per column, straight down out of the top edge, at a
		// speed the difficulty is not allowed to touch.
		bullet_template.speed.v = TO_SP(8);
		bullet_template.angle = 0x40;
		bullet_template.patnum = PAT_BULLET16_N_OUTLINED_BALL_WHITE;
		bullet_template.spawn_type = BST_BULLET16;
		bullet_template.origin.y.v = 0;
		bullet_template.group = BG_SINGLE;
		column = gengetsu_spawncolumns;
		for(i = 0; i < GENGETSU_SPAWNCOLUMN_COUNT; i++, column++) {
			bullet_template.origin.x.v = column->pos.x.v;
			bullets_add_regular_fixedspeed();
		}
		snd_se_play(3);
		break;

	case 4:
		boss.phase_frame = 0;
		boss.mode = 255;
		break;
	}
}

// Phase 7's only pattern, and the simplest one she has: on every 8th frame, a
// 16-bullet ring of blue directional bullets, thrown from a random point in a
// 64x32-pixel box around her at a random angle and a random speed between 1.0
// and just under 5.0 pixels per frame. Nothing here reads [boss.phase_frame],
// so the phase runs until gengetsu_update()'s own 1500-frame timeout.
static void near gengetsu_20050(void)
{
	boss.sprite = 128;
	if(stage_frame_mod8 == 0) {
		bullet_template.angle = randring2_next16();
		bullet_template.origin.x.v = (
			randring2_next16_mod(TO_SP(64)) + (boss.pos.cur.x.v - TO_SP(32))
		);
		bullet_template.origin.y.v = (
			randring2_next16_mod(TO_SP(32)) + (boss.pos.cur.y.v - TO_SP(26))
		);
		bullet_template.group = BG_RING;
		bullet_template.count = 16;
		bullet_template.spawn_type = BST_BULLET16;
		bullet_template.patnum = PAT_BULLET16_D_BLUE;
		bullet_template.speed.v = (randring2_next16_and(0x3F) + TO_SP(1));
		bullets_add_regular();
		snd_se_play(3);
	}
}

// The first 3000 frames of phase 8, on every 4th frame: two eight-way spreads
// half a turn apart, whose shared angle is [stage_frame] doubled and then
// mirrored for every other 256 frames of a 512-frame cycle -- and, on top of
// those, a symmetric pair of three-bullet cloud clusters that walk outwards
// from her in 16-pixel steps until they pass 176 and snap back, turning by
// 0x0B every time. The other three frames out of every four only put her back
// into the idle sprite.
static void near gengetsu_200B6(void)
{
	if(stage_frame_mod4 == 0) {
		boss.sprite = 134;
		bullet_template.group = BG_SPREAD;
		bullet_template.delta.spread_angle = 9;
		bullet_template.count = 8;
		bullet_template.speed.v = (TO_SP(3) + 8);
		bullet_template.angle = (stage_frame * 2);
		if((stage_frame % 512) >= 256) {
			bullet_template.angle = -bullet_template.angle;
		}
		bullets_add_regular();
		bullet_template.angle += 0x80;
		bullets_add_regular();

		bullet_template.speed.v = TO_SP(2);
		bullet_template.spawn_type = BST_BULLET16_CLOUD_BACKWARDS;
		bullet_template.delta.spread_angle = 1;
		bullet_template.count = 3;
		bullet_template.patnum = PAT_BULLET16_N_BALL_BLUE;
		bullet_template.origin.y.v = (
			randring2_next16_mod(TO_SP(32)) + (boss.pos.cur.y.v - TO_SP(26))
		);
		bullet_template.origin.x.v = (
			boss.pos.cur.x.v + TO_SP(origin_offset_x)
		);
		bullet_template.angle = cluster_angle;
		bullets_add_regular();
		bullet_template.origin.x.v = (
			boss.pos.cur.x.v - TO_SP(origin_offset_x)
		);
		bullet_template.angle = -cluster_angle;
		bullets_add_regular();

		origin_offset_x += 16;
		if(origin_offset_x > 176) {
			origin_offset_x = 16;
		}
		cluster_angle += 0x0B;
		snd_se_play(3);
	} else {
		boss.sprite = 130;
	}
}

// Phase 8's second pattern, from frame 3001 onwards: one ring of 32 bullets
// every 4th frame, spawned at a random point in the 64×32 pixels around her
// upper half rather than at a fixed origin, and at a random angle. The spawn
// type is a coin flip between BST_GATHER_ONLY — which, outside a gather_t,
// spawns 16×16 bullets in whatever [patnum] the previous pattern left behind —
// and BST_PELLET. Unlike gengetsu_200B6(), this one flips her two cels on
// every frame, not only on the frames it fires.
static void near gengetsu_20195(void)
{
	if(stage_frame_mod2 != 0) {
		boss.sprite = 134;
	} else {
		boss.sprite = 130;
	}
	if(stage_frame_mod4 == 0) {
		bullet_template.spawn_type = randring2_next16_and(1);
		bullet_template.origin.x.v = (
			randring2_next16_mod(TO_SP(64)) + (boss.pos.cur.x.v - TO_SP(32))
		);
		bullet_template.origin.y.v = (
			randring2_next16_mod(TO_SP(32)) + (boss.pos.cur.y.v - TO_SP(26))
		);
		bullet_template.group = BG_RING;
		bullet_template.count = 32;
		bullet_template.angle = randring2_next16();
		bullet_template.speed.v = (TO_SP(6) + 4);
		bullets_add_regular();
		snd_se_play(3);
	}
}

// The hittest, and the owner of [boss.phase_frame] for the whole fight: every
// path through it advances that counter exactly once, either here or inside
// boss_hittest_shots(). Gengetsu can only be damaged while her sprite is
// visible and she is not mid-teleport; during a bomb, the hitbox still runs —
// on a fixed 48-pixel radius on both axes rather than on
// [boss_hitbox_radius], and with the
// invincibility sound effect — but the damage it returns is thrown away.
// Returns `true` only out of the one path that can end the phase.
static bool near gengetsu_20202(void)
{
	if((extra_boss_bomb_immunity != 0) && (gengetsu_wave_amp == 0)) {
		boss_hittest_shots_damage(TO_SP(48), TO_SP(48), 10);
	} else if((boss.sprite != 0) && (gengetsu_wave_amp == 0)) {
		return boss_hittest_shots();
	}
	boss.phase_frame++;
	return false;
}

// The between-patterns filler: one ring of 32 blue 16×16 bullets every 8th
// frame, at a random angle, out of the origin that gengetsu_update() re-sets
// on every frame. Plays no sound effect of its own: it is heard as part of
// whatever pattern it is layered on top of.
static void near gengetsu_2023B(void)
{
	if(stage_frame_mod8 == 0) {
		bullet_template.group = BG_RING;
		bullet_template.count = 32;
		bullet_template.patnum = PAT_BULLET16_D_BLUE;
		bullet_template.spawn_type = BST_BULLET16;
		bullet_template.angle = randring2_next16();
		bullet_template.speed.v = TO_SP(7);
		bullets_add_regular();
	}
}


void pascal far gengetsu_update(void)
{
	if(bombing) {
		extra_boss_bomb_immunity = BOMB_IMMUNITY_FRAMES;
	}
	if(extra_boss_bomb_immunity != 0) {
		extra_boss_bomb_immunity--;
	}

	bullet_template.origin.x.v = (boss.pos.cur.x.v - TO_SP(13));
	bullet_template.origin.y.v = (boss.pos.cur.y.v - TO_SP(48));
	bullet_template.spawn_type = BST_PELLET;

	switch(boss.phase) {
	case 0:
		gengetsu_20202();

		// Re-set on every frame of the entrance, not once.
		boss.hp = GENGETSU_HP;

		if(boss.phase_frame > 128) {
			boss.phase_end_hp = 14700;
			boss.phase++;
			boss.phase_frame = 0;
			snd_se_play(13);
			tiles_bb_col = V_WHITE;
			_asm mov word ptr bg_render_bombing_func, offset mugetsu_gengetsu_bg_render
		}
		break;

	case 1:
		gengetsu_20202();
		if(boss.phase_frame >= 64) {
			boss.phase++;
			boss.mode = 0;
			boss.phase_state.patterns_seen = 0;
			boss.phase_frame = 0;
			boss.pos.velocity.x.v = 0;
		}
		break;

	case 2:
		gengetsu_pattern_phase(
			gengetsu_1FAAA, gengetsu_1FAF7, GENGETSU_TELEPORT_FIRST,
			ET_CIRCLE, 12700
		);

	case 3:
		gengetsu_pattern_phase(
			gengetsu_1FB86, gengetsu_1FC46, GENGETSU_TELEPORT_LATER,
			ET_HORIZONTAL, 8900
		);

	case 4:
		gengetsu_pattern_phase(
			gengetsu_1FD30, gengetsu_1FDFE, GENGETSU_TELEPORT_LATER,
			ET_HORIZONTAL, 5500
		);

	case 5:
		gengetsu_pattern_phase(
			gengetsu_1FE6A, gengetsu_1FEDF, GENGETSU_TELEPORT_LATER,
			ET_HORIZONTAL, 3000
		);

	case 6:
		gengetsu_20202();
		if(gengetsu_1F9C5()) {
			boss.phase++;
			boss.phase_frame = 32;
		}
		break;

	case 7:
		gengetsu_20050();
		if(boss.phase_frame < 1500) {
			if(!gengetsu_20202()) {
				break;
			}
			boss_score_bonus(100);
		}
		boss_phase_next(ET_VERTICAL, 0);
		boss.mode = 255;
		gengetsu_wave_target_x.v = TO_SP(PLAYFIELD_W / 2);
		boss_statebyte[15] = 16;
		break;

	case 8:
		if(boss.phase_frame <= 3000) {
			gengetsu_200B6();
		} else {
			gengetsu_20195();
		}
		if(!gengetsu_20202() && (boss.phase_frame < 5000)) {
			break;
		}
		boss_explode_small(ET_NW_SE);
		boss.phase++;

		// The defeat bonus is the one thing that distinguishes killing
		// Gengetsu from surviving her: the timeout takes the same branch.
		if(boss.phase_frame < 5000) {
			boss.phase_state.defeat_bonus = true;
		} else {
			boss.phase_state.defeat_bonus = false;
		}
		boss.phase_frame = 0;
		boss.mode = 0;
		PaletteTone = 100;
		palette_changed = true;
		break;

	case 9:
		boss.phase_frame++;
		if(boss.phase_frame == 16) {
			boss_explode_small(ET_VERTICAL);
		}
		if(boss.phase_frame == 32) {
			boss_defeat_explode_big(ET_HORIZONTAL, 200);
			snd_se_play(12);

			// No Palettes write at all, unlike every other TH04 boss: the
			// Extra Stage's palette is whatever the fade left it as.
			palette_changed = true;
			player_invincibility_time = BOSS_DEFEAT_INVINCIBILITY_FRAMES;
		}
		break;

	default:
		boss_defeat_update();
		return;
	}

	// Homing bullets keep aiming at wherever she left from for the length of
	// a teleport, because the wave animation moves the sprite and not
	// [boss.pos].
	if(gengetsu_wave_amp == 0) {
		homing_target.x.v = boss.pos.cur.x.v;
		homing_target.y.v = boss.pos.cur.y.v;
	}
	thicklasers_update_and_hittest();
	hud_hp_update_and_render(boss.hp, GENGETSU_HP);
}
/// --------------------------------------------------------
