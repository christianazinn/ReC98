/// Extra Stage Boss EX-Alice - update helpers
/// ------------------------------------------
/// The tail of BX_TEXT, the head segment the kb/codegen/0080 three-way carve of
/// main_036_TEXT created. That block had NO C++ contribution at all, and the
/// last thing it contributed was the `include th05/main/boss/bx.asm` this file
/// replaces -- so this object lands at exactly the seam a carve-free tail lift
/// needs (kb/codegen 0099 + 0112 + 0114), and the fifteen `sub_1E8DA`-family
/// procs still above it become ordinary prepends into the front of this file.
///
/// exalice_update() itself is NOT here and cannot be: it is in main_036_TEXT,
/// the middle block of the same carve, together with the three jump tables its
/// `switch` statements compile to.
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
#include "th04/main/custom.hpp"
#include "th04/main/gather.hpp"
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
// -----------------------------------------------------

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
