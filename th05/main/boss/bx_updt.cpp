/// Extra Stage Boss EX-Alice - update helpers
/// ------------------------------------------
/// ALL of BX_TEXT, the head segment the kb/codegen/0080 three-way carve of
/// main_036_TEXT created. That block had NO C++ contribution at all until this
/// object took over the ASM include at its tail (kb/codegen 0099 + 0112 + 0114),
/// which left a seam at exactly the address the procs above it needed to be
/// prepended into the front of this file one at a time. th05_main.asm's BX_TEXT
/// contribution is empty now, so everything in the segment is here.
///
/// exalice_update() itself is NOT here and cannot be: it is in main_036_TEXT,
/// the middle block of the same carve, together with the three jump tables its
/// `switch` statements compile to. It reaches every function below that it
/// calls, stores or tabulates through the `procdesc` list in th05_main.asm's
/// BX_TEXT block, which is all that block emits now.
///
/// (#included from th05/exalice.cpp, which is its OWN object, so this file
/// names every header it needs and shares none of them -- kb/codegen/0129 has
/// nothing to collide.)

#include "libs/master.lib/master.hpp"
#include "th02/main/player/bomb.hpp"
#include "th03/math/polar.hpp"
#include "th04/math/randring.hpp"
#include "th04/snd/snd.h"
#include "th04/main/boss/boss.hpp"
#include "th04/main/bullet/bullet.hpp"
#include "th04/main/bullet/clearzap.hpp"
#include "th04/main/bullet/pellet_r.hpp"
#include "th04/main/custom.hpp"
#include "th04/main/gather.hpp"
#include "th04/main/pattern.hpp"
#include "th04/main/player/player.hpp"
#include "th05/main/boss/boss.hpp"
#include "th05/main/bullet/cheeto.hpp"
#include "th05/main/bullet/laser.hpp"
#include "th05/main/player/player.hpp"
#include "th05/sprites/main_pat.h"

// Constants
// ---------

// EX-Alice's last phase starts here and ends at 0: exalice_update()'s
// ET_HORIZONTAL transition passes 3400 as the phase after it ends at, and the
// ET_VERTICAL one after that passes 0. Both of the fights's HP-scaled
// quantities below are therefore fractions of the damage done in this phase.
static const int EXALICE_PHASE_LAST_HP = 3400;
// ---------

// Still ZUN's assembly, named here for this file's reads
// -----------------------------------------------------

// Counts down the bomb-triggered invincibility window.
// th05/main/boss/bx_fg.cpp, the fight's other reader, carries the full note on
// why this name is a reading rather than a guess. A th05_main.asm `.data?`
// label with no `public` of ZUN's (kb/codegen/0123). [inferred] name.
extern "C" unsigned char exalice_invincibility_frames;

// The X coordinate pattern_mirrored_crosses() spawns both its gather circles
// and its bullets from. Initialised to the horizontal center of the playfield
// and re-aimed at the player on the frame that pattern ENDS, so each run of it
// starts where the player was standing when the previous one finished. That
// pattern is its only reader and its only writer -- and the reason it stays in
// th05_main.asm's `_DATA` rather than becoming a variable of this file is that
// it is initialised storage at a fixed address, which a C++ definition here
// would move. A kb/codegen/0123 zero-byte alias reaches it instead.
extern "C" subpixel_t exalice_pattern_origin_x;

// The pattern function exalice_gather_and_pattern() calls. Written only by
// exalice_update(): once per pattern out of [off_22874], the 4×2 table still in
// th05_main.asm's `_DATA`, and directly at the eight phase transitions that
// pick one by hand. This file is that pointer's only reader, so the table needs
// no C++ name and only the pointer gets a kb/codegen/0123 alias.
// `pattern_oneshot_func_t` rather than the loop variant because the call site
// tests the returned AL.
extern "C" pattern_oneshot_func_t exalice_pattern;

// The point pattern_spreads_and_firewaves() aims from and spawns its single
// bullets at, re-picked every 16 frames. Uninitialised storage at a fixed
// address in th05_main.asm's `.data?`, so it stays there; that pattern was its
// last ASM reader, so this parcel renamed the label outright rather than
// aliasing it. [inferred] name.
extern "C" PlayfieldPoint exalice_random_origin;

// The [lasers] slot pattern_lasers_and_green_rings() spawns its next fixed
// laser into, cycling 0…15. Initialised storage (`dw 0`) in th05_main.asm's
// `_DATA`, renamed there rather than defined here for the same reason
// [exalice_pattern_origin_x] above is. [inferred] name.
extern "C" int exalice_laser_slot;
// -----------------------------------------------------

// th05/main/boss/bx.cpp, which th05/boss_x.cpp compiles into BX_UPDATE_TEXT
// rather than this segment -- but the same main_03 group, so the call stays
// near. Declared here because that file has no header of its own.
void pascal near firewaves_add(pixel_t amp, bool is_right);

// Runs whichever pattern [exalice_pattern] currently points at, announced by a
// 3-stack gather animation that starts 48 frames into the phase and by
// EX-Alice's casting sprite on the frame it converges. Returns whether the
// pattern reported itself done, which is also where the phase frame is reset
// and [boss.mode] goes back to the movement half of the phase.
bool near exalice_gather_and_pattern(void)
{
	gather_add_only_3stack((boss.phase_frame - 48), 6, 7);
	if(boss.phase_frame == 48) {
		boss.sprite = 181;
		snd_se_play(8);
	}
	if(boss.phase_frame >= 64) {
		if(exalice_pattern()) {
			boss.phase_frame = 0;
			boss.mode = 0;
			return true;
		}
	}
	return false;
}

// A 5-way aimed spread of red arrowheads on every single frame, each at a
// random speed between 2.5 and 4.4 and turned by a random multiple of 8 units
// within a quarter turn to either side of the player -- so the spread's own 12
// units of separation are lost in the noise. Only every fourth of them plays
// the spawn sound; all of them fire. Runs for 128 frames.
int near pattern_random_red_spreads(void)
{
	bullet_template.spawn_type = BST_NO_DECELERATE;
	bullet_template.speed.v = (randring2_next16_and(0x1F) + to_sp8(2.5f));
	bullet_template.angle = ((randring2_next16_and(0x7F) & 0xF8) - 0x40);
	bullet_template.group = BG_SPREAD_AIMED;
	bullet_template.patnum = PAT_BULLET16_V_RED;
	bullet_template.set_spread(5, 12);
	bullets_add_regular();
	if((boss.phase_frame % 4) == 0) {
		snd_se_play(3);
	}
	return (boss.phase_frame == 128);
}

// Red rings every 4 frames, alternating a 12-way ring of balls with an 8-way
// ring of arrowheads that decelerates and then turns onto the player once. The
// base angle steps by 2 units per ring, in a direction that flips on frame 64.
// EX-Alice flies randomly throughout, with the step counted from 100 frames
// BEFORE the phase started -- this fight's own idiom, which
// pattern_speedup_spreads() below repeats. Runs for 128 frames.
int near pattern_rotating_red_rings(void)
{
	if(boss.phase_frame == 64) {
		boss_statebyte[15] = (1 - boss_statebyte[15]);
	}
	if((boss.phase_frame % 4) == 0) {
		bullet_template.spawn_type = (BST_CLOUD_BACKWARDS | BST_NO_DECELERATE);
		bullet_template.speed.set(4.0f);
		if(boss_statebyte[15] != 0) {
			bullet_template.angle += 2;
		} else {
			bullet_template.angle += -2;
		}
		bullet_template.group = BG_RING;
		bullet_template.special_motion = BSM_DECELERATE_THEN_TURN_AIMED;
		bullet_special.turns_max = 1;
		bullet_template.set_spread(12, 10);
		bullet_template.patnum = PAT_BULLET16_N_BALL_RED;
		if((boss.phase_frame % 8) == 0) {
			bullets_add_regular();
		} else {
			bullet_template.set_spread(8, 10);
			bullet_template.patnum = PAT_BULLET16_V_RED;
			bullets_add_special();
		}
		snd_se_play(3);
	}
	boss_flystep_random(boss.phase_frame - 100);
	return (boss.phase_frame == 128);
}

// The only pattern here with no end condition of its own, and the busiest:
// mirrored pairs of 5-way pellet spreads every 4 frames whose angle sweeps
// between 0x14 and 0x40 and back in steps of 3, a single aimed arrowhead every
// 2 frames from a point that jumps to somewhere within +/-48 by +/-32 pixels of
// EX-Alice every 16, a fire wave every 128 alternating between the two sides,
// and random flight from frame 256 onwards.
//
// The three [boss_statebyte] slots are this pattern's own: [14] is the sweep
// angle, [13] the flag that reverses the sweep, and [15] the angle the single
// bullets are fired at -- aimed once when the origin is picked rather than per
// bullet, so a whole 16-frame burst keeps pointing where the player was.
bool near pattern_spreads_and_firewaves(void)
{
	if(boss.phase_frame == 64) {
		boss_statebyte[14] = 0x40;
		boss_statebyte[13] = 0;
		pellet_bottom_col = 7;
	}
	if((boss.phase_frame % 2) == 0) {
		if((boss.phase_frame % 4) == 0) {
			bullet_template.spawn_type = BST_NO_DECELERATE;
			bullet_template.patnum = 0;
			bullet_template.group = BG_SPREAD_AIMED;
			bullet_template.set_spread(5, 6);
			bullet_template.speed.set(5.0f);
			bullet_template.angle = boss_statebyte[14];
			bullets_add_regular();
			bullet_template.angle = -bullet_template.angle;
			bullets_add_regular();
			if(boss_statebyte[13] == 0) {
				boss_statebyte[14] += -3;
				if(boss_statebyte[14] <= 0x14) {
					boss_statebyte[13]++;
				}
			} else {
				boss_statebyte[14] += 3;
				if(boss_statebyte[14] >= 0x40) {
					boss_statebyte[13]--;
				}
			}
			if((boss.phase_frame % 16) == 0) {
				exalice_random_origin.x.v = (
					randring2_next16_mod(to_sp(96.0f)) +
					bullet_template.origin.x.v - to_sp(48.0f)
				);
				exalice_random_origin.y.v = (
					randring2_next16_mod(to_sp(64.0f)) +
					bullet_template.origin.y.v - to_sp(32.0f)
				);
				boss_statebyte[15] = player_angle_from(exalice_random_origin, 0);
			}
		}
		bullet_template.origin.x.v = exalice_random_origin.x.v;
		bullet_template.origin.y.v = exalice_random_origin.y.v;
		bullet_template.spawn_type = (BST_CLOUD_FORWARDS | BST_NO_DECELERATE);
		bullet_template.angle = boss_statebyte[15];
		bullet_template.group = BG_SINGLE;
		bullet_template.speed.set(6.0f);
		bullet_template.patnum = PAT_BULLET16_V_RED;
		bullets_add_regular();
	}
	if((boss.phase_frame % 128) == 0) {
		firewaves_add(128, ((boss.phase_frame % 256) == 0));
		snd_se_play(13);
	}
	if(boss.phase_frame >= 256) {
		boss_flystep_random((boss.phase_frame & 0x7F) - 96);
	}
	return false;
}

// Slow, gravity-bound small blue balls in 3-way spreads at random downward
// angles every 2 frames, with a 4-pellet aimed stack thrown in from a random
// point near EX-Alice on every fourth frame. Runs for 128 frames.
int near pattern_gravity_balls_and_stacks(void)
{
	if((boss.phase_frame % 2) == 0) {
		bullet_template.spawn_type = BST_NO_DECELERATE;
		bullet_template.speed.v = (randring2_next16_and(0x1F) + to_sp8(0.75f));
		bullet_template.angle = (randring2_next16_and(0x7F) + 0x80);
		bullet_template.group = BG_SPREAD;
		bullet_template.special_motion = BSM_GRAVITY;
		bullet_special.speed_delta.set(0.0625f);
		bullet_template.patnum = PAT_BULLET16_N_SMALL_BALL_BLUE;
		bullet_template.set_spread(3, 12);
		bullets_add_special();
		if((boss.phase_frame % 4) == 0) {
			snd_se_play(3);
			bullet_template.spawn_type = (
				BST_CLOUD_FORWARDS | BST_NO_DECELERATE
			);
			bullet_template.origin.x.v = (
				randring2_next16_mod(to_sp(96.0f)) - to_sp(48.0f) +
				bullet_template.origin.x.v
			);
			bullet_template.origin.y.v = (
				randring2_next16_mod(to_sp(64.0f)) - to_sp(32.0f) +
				bullet_template.origin.y.v
			);
			bullet_template.patnum = 0;
			bullet_template.group = BG_STACK_AIMED;
			bullet_template.speed.set(3.0f);
			bullet_template.set_stack(4, 0.375f);
			bullet_template.angle = 0;
			bullets_add_regular();
		}
	}
	return (boss.phase_frame == 128);
}

// 22-way rings of blue arrowheads every 8 frames, the ring's base angle
// stepping by 3 units in a direction that flips on frame 64. Runs for 128
// frames.
int near pattern_rotating_blue_rings(void)
{
	if(boss.phase_frame == 64) {
		boss_statebyte[15] = (1 - boss_statebyte[15]);
	}
	if((boss.phase_frame % 8) == 0) {
		bullet_template.spawn_type = (BST_CLOUD_BACKWARDS | BST_NO_DECELERATE);
		bullet_template.speed.set(5.5f);
		if(boss_statebyte[15] != 0) {
			bullet_template.angle += 3;
		} else {
			bullet_template.angle += -3;
		}
		bullet_template.group = BG_RING;
		bullet_template.set_spread(22, 8);
		bullet_template.patnum = PAT_BULLET16_D_BLUE;
		bullets_add_regular();
		snd_se_play(3);
	}
	return (boss.phase_frame == 128);
}

// Rings of slow blue arrowheads that bounce off the left, right and top
// playfield edges once, fired every 16 frames and rotated by 2 units each time.
// The ring starts at 8 bullets and gains one every 128 frames up to 14, so the
// pattern keeps thickening for as long as exalice_update()'s own phase timeout
// lets it run -- it never ends by itself.
//
// [bullet_template.spread] is written directly rather than through
// set_spread(), because the bullet count is the only half of that word this
// pattern ever touches; the angle delta a ring ignores anyway.
bool near pattern_bouncing_blue_rings(void)
{
	if(boss.phase_frame == 64) {
		bullet_template.spread = 8;
	}
	if((boss.phase_frame % 16) == 0) {
		bullet_template.patnum = PAT_BULLET16_D_BLUE;
		bullet_template.spawn_type = (BST_CLOUD_BACKWARDS | BST_NO_DECELERATE);
		bullet_template.speed.set(2.0f);
		bullet_template.group = BG_RING;
		bullet_template.special_motion = BSM_BOUNCE_LEFT_RIGHT_TOP;
		if((boss.phase_frame % 128) == 0) {
			if(bullet_template.spread < 14) {
				bullet_template.spread++;
			}
		}
		bullet_special.turns_max = 1;
		bullet_template.angle += 2;
		bullets_add_special();
		snd_se_play(3);
	}
	return false;
}

// Fixed lasers aimed an eighth of a turn to alternating sides of the player,
// one every 8 frames until frame 144 and each into the next of the 16 slots
// this pattern cycles through, with a 32-way ring of green bullets at a random
// angle alongside every second one. Every fixed laser that is still waiting to
// grow then rotates by one unit per frame, in a direction taken from the parity
// of its slot -- so the fan winds itself up before any of it can kill. Runs for
// 208 frames, and hands the slot counter back at 0 for the next run.
bool near pattern_lasers_and_green_rings(void)
{
	int i;
	int cycle_frame = (boss.phase_frame % 16);

	laser_template.col = 3;
	laser_template.coords.width.nonshrink = 8;
	laser_template.active_at_age.grow = 47;
	laser_template.shrink_at_age = 80;
	if(boss.phase_frame < 144) {
		if(cycle_frame == 0) {
			laser_template.coords.angle = player_angle_from(
				laser_template.coords.origin, 0x30
			);
			laser_fixed_spawn(exalice_laser_slot++);
			bullet_template.spawn_type = BST_NO_DECELERATE;
			bullet_template.patnum = PAT_BULLET16_D_GREEN;
			bullet_template.group = BG_RING;
			bullet_template.angle = randring2_next16();
			bullet_template.speed.set(4.0f);
			bullet_template.set_spread(32, 10);
			bullets_add_regular();
		} else if(cycle_frame == 8) {
			laser_template.coords.angle = player_angle_from(
				laser_template.coords.origin, -0x30
			);
			laser_fixed_spawn(exalice_laser_slot++);
		}
	}
	exalice_laser_slot &= 0xF;
	for(i = 0; i < 16; i++) {
		if(lasers[i].flag == LF_FIXED_WAIT_TO_GROW) {
			if((i & 1) != 0) {
				lasers[i].coords.angle += 1;
			} else {
				lasers[i].coords.angle += -1;
			}
		}
	}
	if(boss.phase_frame == 208) {
		exalice_laser_slot = 0;
		return true;
	}
	return false;
}

// An aimed 7-way spread of accelerating green bullets every other frame for 128
// frames, each spread spawned from an X within +/-24 pixels of the previous
// origin, so the wall of bullets wanders. EX-Alice flies randomly throughout,
// with the step counted from 100 frames BEFORE the phase started -- the
// negative prefix is ZUN's, not a transcription slip.
int near pattern_speedup_spreads(void)
{
	if((boss.phase_frame % 2) == 0) {
		bullet_template.spawn_type = (BST_CLOUD_FORWARDS | BST_NO_DECELERATE);
		bullet_template.group = BG_SPREAD_AIMED;
		bullet_template.special_motion = BSM_SPEEDUP;
		bullet_special.speed_delta.set(0.125f);
		bullet_template.speed.set(4.0f);
		bullet_template.set_spread(7, 9);
		bullet_template.patnum = PAT_BULLET16_D_GREEN;
		bullet_template.origin.x.v += (
			randring2_next16_mod(to_sp(48.0f)) - to_sp(24.0f)
		);
		bullet_template.angle = 0;
		bullets_add_special();
		snd_se_play(3);
	}
	boss_flystep_random(boss.phase_frame - 100);
	return (boss.phase_frame == 128);
}

// Shoot-out lasers fired every 4 frames along an angle that walks from 0 up to
// 128 in steps of 8 and then back down again, indefinitely -- [boss_statebyte]
// [15] is which way the walk is currently going. EX-Alice starts flying
// randomly after frame 256, and from 512 onwards also drops an aimed 32-way
// ring of green bullets every 16 frames. It has no end condition of its own and
// always returns `false`: exalice_update()'s own phase timeout is what stops
// it.
bool near pattern_pingpong_lasers(void)
{
	if(boss.phase_frame == 64) {
		laser_template.col = 2;
		laser_template.coords.width.nonshrink = 6;
		laser_template.coords.angle = 0;
		laser_template.active_at_age.moveout = 30;
		laser_template.shootout_speed.set(5.5f);
		boss_statebyte[15] = 0;
	}
	if((boss.phase_frame % 4) == 0) {
		if(boss_statebyte[15] == 0) {
			laser_template.coords.angle += 8;
			if(laser_template.coords.angle >= 128) {
				boss_statebyte[15] = 1;
			}
			lasers_shootout_add();
		} else if(boss_statebyte[15] == 1) {
			laser_template.coords.angle += -8;
			if(laser_template.coords.angle == 0) {
				boss_statebyte[15] = 0;
			}
			lasers_shootout_add();
		}
	}
	if(boss.phase_frame >= 256) {
		boss_flystep_random((boss.phase_frame & 0x3F) - 32);
		if(boss.phase_frame >= 512) {
			if((boss.phase_frame % 16) == 0) {
				bullet_template.spawn_type = BST_NO_DECELERATE;
				bullet_template.group = BG_RING_AIMED;
				bullet_template.angle = 0;
				bullet_template.speed.set(4.0f);
				bullet_template.set_spread(32, 10);
				bullet_template.patnum = PAT_BULLET16_D_GREEN;
				bullets_add_regular();
			}
		}
	}
	return false;
}

// A 9-way aimed spread of cross bullets every 16 frames, with a curved laser
// thrown in on every fourth of them, aimed an eighth of a turn to alternating
// sides of the player. Runs for 256 frames. pattern_aimed_cheetos() below is
// [off_22874]'s partner entry for this one, and shares its cheeto half almost
// verbatim -- different angles, different interval, same shape.
int near pattern_aimed_crosses(void)
{
	if((boss.phase_frame % 16) == 0) {
		bullet_template.speed.set(3.5f);
		bullet_template.spawn_type = (BST_CLOUD_FORWARDS | BST_NO_DECELERATE);
		bullet_template.group = BG_SPREAD_AIMED;
		bullet_template.patnum = PAT_BULLET16_N_CROSS_BLUE;
		bullet_template.angle = 0;
		bullet_template.set_spread(9, 15);
		bullets_add_regular();
		if((boss.phase_frame % 64) == 0) {
			cheeto_template.col = 11;
			cheeto_template.speed.set(3.0f);
			if((boss.phase_frame % 128) == 0) {
				cheeto_template.angle = player_angle_from(
					cheeto_template.origin, 0x20
				);
			} else {
				cheeto_template.angle = player_angle_from(
					cheeto_template.origin, -0x20
				);
			}
			cheetos_add();
			snd_se_play(15);
		}
	}
	return (boss.phase_frame == 256);
}

// One curved laser every 8 frames, aimed a quarter turn to one side of the
// player and alternating which side every 16, while EX-Alice flies randomly
// within her clamp rectangle. Returns whether the run is over, which it is on
// the single frame 128 -- so unlike this file's other patterns it does not end
// on a state machine but on a fixed length.
int near pattern_aimed_cheetos(void)
{
	if((boss.phase_frame % 8) == 0) {
		cheeto_template.col = 11;
		cheeto_template.speed.set(4.0f);
		if((boss.phase_frame % 16) == 0) {
			cheeto_template.angle = player_angle_from(
				cheeto_template.origin, 0x40
			);
		} else {
			cheeto_template.angle = player_angle_from(
				cheeto_template.origin, -0x40
			);
		}
		cheetos_add();
		snd_se_play(15);
	}
	boss_flystep_random(boss.phase_frame % 32);

	// Returned as an expression rather than through an `if` with two `return`
	// statements, which is not a style choice: the `if` form gives each
	// constant its own `pop bp` / `retn`, and the original materializes the
	// comparison into AX and then falls into ONE epilogue through a `jmp
	// short`. Same length, two bytes apart (kb/codegen/0074's shape, in the
	// direction that shares the epilogue).
	return (boss.phase_frame == 128);
}

// Mirrored pairs of 3-way cross-bullet spreads, announced by a ring of gather
// circles that alternates its spin direction every 16 frames. Each shot fires
// two spreads reflected across the vertical axis -- `0x80 - angle`, applied
// twice, leaves the base angle where it was -- and then steps that base angle
// by [boss_statebyte][15], which ramps up to 0x10 and back down to 0 over the
// run. Reaching 0 again is what ends the pattern, and the frame it ends on is
// where [exalice_pattern_origin_x] is re-aimed at the player.
//
// The three [boss_statebyte] slots are this pattern's own: [15] is the angle
// step, [14] the ramp counter, and [13] the flag that flips the ramp from
// rising to falling.
bool near pattern_mirrored_crosses(void)
{
	bullet_template.origin.y += 104.0f;
	bullet_template.origin.x.v = exalice_pattern_origin_x;
	gather_template.center.x.v = exalice_pattern_origin_x;
	if(boss.phase_frame == 64) {
		cheeto_template.angle = -0x3C;
		bullet_template.angle = 0x20;
		cheeto_template.speed.set(5.0f);
		bullet_template.speed.set(3.0f);
		bullet_template.spawn_type = (BST_CLOUD_FORWARDS | BST_NO_DECELERATE);
		bullet_template.group = BG_SPREAD;
		bullet_template.set_spread(3, 8);
		bullet_template.patnum = PAT_BULLET16_N_CROSS_BLUE;
		boss_statebyte[15] = 0;
		boss_statebyte[14] = 0;
		boss_statebyte[13] = 0;
		snd_se_play(8);
	}
	if(boss.phase_frame < 128) {
		if((boss.phase_frame % 8) == 0) {
			gather_template.col = 14;
			gather_template.center.y.v = bullet_template.origin.y;
			gather_template.radius.set(256.0f);
			if((boss.phase_frame % 16) == 0) {
				gather_template.angle_delta = 2;
			} else {
				gather_template.angle_delta = -2;
			}
			gather_add_only();
		}
		return false;
	}
	if((boss.phase_frame % 2) != 0) {
		return false;
	}
	if((boss.phase_frame % 16) == 0) {
		gather_template.center.y.v = bullet_template.origin.y;
		gather_template.radius.set(128.0f);
		if((boss.phase_frame % 32) == 0) {
			gather_template.angle_delta = 2;
		} else {
			gather_template.angle_delta = -2;
		}
		gather_add_only();
	}
	bullet_template.angle = (0x80 - bullet_template.angle);
	bullets_add_regular_fixedspeed();
	bullet_template.angle = (0x80 - bullet_template.angle);
	bullets_add_regular_fixedspeed();
	snd_se_play(3);
	bullet_template.angle -= boss_statebyte[15];
	if((boss.phase_frame % 32) != 0) {
		return false;
	}
	boss_statebyte[15] += boss_statebyte[14];
	if(boss_statebyte[13] == 0) {
		boss_statebyte[14]++;
		if(boss_statebyte[14] >= 0x10) {
			boss_statebyte[13]++;
		}
	} else {
		boss_statebyte[14]--;
		if(boss_statebyte[14] == 0) {
			exalice_pattern_origin_x = player_pos.cur.x;
			return true;
		}
	}
	return false;
}

// The last phase's pattern and movement, fused into one per-frame call. A
// 3-stack gather animation announces it at [phase_frame] 64, which is also
// where the 5-way spread it fires is set up; from 128 onwards it fires two of
// those spreads on every other frame, half a turn apart, and rotates the base
// angle by 0x87 afterwards so the next pair lands somewhere else. EX-Alice
// herself oscillates around the horizontal center of the playfield for as long
// as it runs.
//
// Both of the HP-scaled quantities reuse [boss.pos.velocity.x] as scratch
// storage rather than as a velocity -- it is the oscillation's amplitude, and
// nothing steps the position by it. They are the same expression apart from
// polar_y()'s subpixel scale factor, and are written in the shape
// th05/main/midboss/m1.cpp uses for the same idiom: a 16-bit signed difference,
// widened by the `lu` literal into the unsigned 32-bit multiply and divide the
// original performs.
void near exalice_spreads_and_sweep(void)
{
	gather_add_only_3stack((boss.phase_frame - 64), 6, 7);
	if(boss.phase_frame == 64) {
		boss.sprite = 181;
		snd_se_play(8);
		bullet_template.patnum = PAT_BULLET16_V_BLUE;
		bullet_template.group = BG_SPREAD;
		bullet_template.angle = randring2_next16();
		bullet_template.set_spread(5, 21);
		boss_statebyte[15] = 0;
		boss.pos.velocity.x.v = 0;
	}
	if(boss.phase_frame >= 128) {
		if((boss.phase_frame % 2) == 0) {
			bullet_template.speed.v = (
				(((EXALICE_PHASE_LAST_HP - boss.hp) * 64lu) /
					EXALICE_PHASE_LAST_HP
				) + to_sp8(2.5f)
			);
			bullets_add_regular();
			bullet_template.angle += 0x80;
			bullets_add_regular();
			bullet_template.angle += 0x87;
			snd_se_play(9);
		}
		boss.pos.cur.x.v = polar_y(
			TO_SP(192), boss.pos.velocity.x, boss_statebyte[15]
		);
		boss_statebyte[15] += 2;
		boss.pos.velocity.x.v = (
			((EXALICE_PHASE_LAST_HP - boss.hp) * 64lu * SUBPIXEL_FACTOR) /
			EXALICE_PHASE_LAST_HP
		);
	}
}

// EX-Alice's own hittest, and the only thing a bomb does to her in this fight:
// every frame the bomb animation runs re-arms 39 frames of invincibility, so
// she takes no damage for as long as it lasts and for 39 frames afterwards.
// Returns whether the current phase's HP threshold was crossed, which is what
// every one of exalice_update()'s four call sites tests.
//
// `far`, and that is ZUN's declaration rather than a choice of ours: `-ml`
// makes every function far unless it says `near`, and this is the one body in
// this segment whose does not.
//
// The `int` return is not decoration either. boss_hittest_shots() returns
// `bool`, so a `bool` return here would forward AL untouched; the original
// zero-extends it with `mov ah, 0` and returns `xor ax, ax` on the invincible
// path, which is a 16-bit value. Measured against all three shapes from a
// `tcc -S` listing (kb/codegen 0152 + 0028).
int exalice_hittest(void)
{
	if(bombing) {
		exalice_invincibility_frames = 39;
	}
	if(exalice_invincibility_frames == 0) {
		return boss_hittest_shots();
	}
	return 0;
}

// Ends the current phase: plays the given explosion, drops this phase's items
// through a bullet zap if the phase was beaten rather than timed out, and
// resets every piece of per-phase state. The phase this moves on to will
// itself end at [next_end_hp].
//
// ET_NONE skips the whole explosion-and-items half, which is how the fight
// steps through its color-fade phases without giving anything away.
//
// `extern "C"`, because that is the only spelling that reproduces the plain,
// undecorated `public EXALICE_PHASE_NEXT` the dump used to export for this
// function, and which exalice_update() still calls through a `procdesc` in
// th05_main.asm. `pascal` alone does NOT suffice: C++ linkage upper-cases the
// name by the calling convention and then STILL appends the mangled argument
// suffix (kb/codegen/0027, and th04/main/boss/b6_next.cpp says the same thing
// about TH04's yuuka6_phase_next(), which is this function's counterpart).
//
// th05/main/boss/bx.cpp declares this function WITHOUT `extern "C"`, so that
// declaration has never resolved against anything; it is harmless only
// because nothing in th05/boss_x.cpp's object calls it.
extern "C" void pascal near exalice_phase_next(
	explosion_type_t explosion_type, int next_end_hp
)
{
	if(explosion_type != ET_NONE) {
		boss_explode_small(explosion_type);
		if(!boss_phase_timed_out) {
			bullet_zap_drop_point_items = true;
			bullet_zap.active = true;
			boss_items_drop();
		}
	}
	boss_phase_timed_out = true;
	boss.phase++;
	boss.phase_frame = 0;
	boss.mode = 0;
	boss.phase_state.patterns_seen = 0;
	boss.hp = boss.phase_end_hp;
	boss.phase_end_hp = next_end_hp;
}
