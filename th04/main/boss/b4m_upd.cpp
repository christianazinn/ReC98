/// Stage 4 Boss - Marisa: the fight's own update function
/// ------------------------------------------------------
/// (#included from th04/std_run.cpp, ahead of enemies_add() and std_run(),
/// which is this function's original address order at the head of
/// ENM_BTPL_TEXT's C++ object. That ZUN's object for this segment held
/// Marisa's fight, the enemy spawner and the stage script VM is
/// kb/codegen/0112.)
///
/// Marisa's patterns and her bits live in th04/main/boss/b4m.cpp, which is a
/// different segment (B4M_UPDATE_TEXT) and therefore a different object; only
/// the two functions that segment's own tail lift reached are C++ so far.
///
/// This file deliberately includes NO unguarded header that
/// th04/main/enemy/add.cpp or th04/formats/std_run.cpp includes after it --
/// th04/main/frames.h and th04/math/randring.hpp in particular
/// (kb/codegen/0129). The three declarations that would have come from those
/// are repeated below instead.

#include "platform.h"
#include "pc98.h"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/v_colors.hpp"
#include "th02/main/player/player.hpp"
#include "th03/hardware/palette.hpp"
#include "th04/snd/snd.h"
#include "th04/sprites/main_pat.h"
#include "th04/main/bg.hpp"
#include "th04/main/homing.hpp"
#include "th04/main/hud/hud.hpp"
#include "th04/main/tile/bb.hpp"
#include "th04/main/gather.hpp"
#include "th04/main/score.hpp"
#include "th04/main/spark.hpp"
#include "th04/math/vector.hpp"
#include "th04/main/bullet/bullet.hpp"
#include "th04/main/bullet/clearzap.hpp"
#include "th04/main/boss/boss.hpp"
#include "th04/main/boss/b4m.hpp"

/// From headers this object must not expand twice
/// ----------------------------------------------
// th04/main/frames.h, expanded by th04/formats/std_run.cpp below.
extern unsigned int stage_frame;
extern unsigned char stage_frame_mod4;

// th03/math/randring.hpp, reached by th04/main/enemy/add.cpp below.
uint16_t near randring2_next16(void);
uint16_t pascal near randring2_next16_and(uint16_t mask);
uint16_t pascal near randring2_next16_mod(uint16_t mask);

// th04/main/player/shot.hpp, which reaches th04/math/randring.hpp and would
// therefore expand the same inline bodies a second time in this object.
extern SPPoint shot_hitbox_center;
extern SPPoint shot_hitbox_radius;
int shots_hittest(void);

// th04/main/player/player.hpp, for the same reason.
extern PlayfieldMotion player_pos;

// libs/master.lib/master.hpp, which pulls in far too much for this object.
// `far pascal` is MASTER_RET under the large model (libs/master.lib/func.hpp);
// this line read `__cdecl` for exactly as long as nothing in the object called
// the function, and `__cdecl` costs an `ADD SP, 4` and reverses the pushes.
extern "C" int far pascal iatan2(int y, int x);
/// ----------------------------------------------

/// Already C++, in th04/main/boss/b4m.cpp
/// --------------------------------------
/// B4M_UPDATE_TEXT, so a different object -- but the same main_03 group, so
/// every one of these calls stays near. No TH04 header declares them.

// Marisa's random flight step, used by the interval between two patterns.
void near marisa_flystep_random(void);

// Runs the 64-frame charge-up that every one of Marisa's patterns opens with,
// and tells the pattern which of its three stages this frame is.
marisa_charge_t near marisa_charge_animate(void);

// Flies Marisa to the point reflection of her position across a fixed point
// near the top of the background's sealed moon, over [duration] frames.
// Returns `true` on the frame that completes the flight.
bool pascal near marisa_flystep_pointreflected(int duration);
/// --------------------------------------

/// Still in th04_main.asm, and staying there
/// -----------------------------------------
// The HP each of the four bits respawns with. ZUN's object emitted it into the
// middle of _DATA, where the dump still owns it and where a contribution from
// THIS object could never land -- so the table stays put and this is only the
// `extern` for it.
//
// Wrapped in a `struct` rather than declared as a plain `int[BIT_COUNT]`,
// because marisa_bits_respawn() copies the whole table into a stack local
// before indexing it and an array cannot be copy-initialized. Only a structure
// copy makes Turbo C++ emit the `push ss / push ax / push ds /
// push offset _MARISA_BIT_HP / mov cx, 8 / call SCOPY@` that opens the
// original function, against THIS symbol rather than against a second copy of
// the four words in this object's own data.
struct bit_hp_t {
	int v[BIT_COUNT];
};
extern "C" bit_hp_t MARISA_BIT_HP;
/// -----------------------------------------

// Marisa's fight state, all of it th04_main.asm `.data?` bytes with no
// `public` of ZUN's, and all of it reached from nowhere but this segment.
// Five of the six are written and read entirely inside marisa_update(), so
// their names are `[inferred]` from that function alone and a naming round is
// owed for all six.
//
// [marisa_25671] is the exception and keeps the dump's address-suffixed
// spelling. Both of its readers are C++ now -- marisa_bits_respawn() seeds
// every bit's [angle_speed] from it, and pattern 0 negates it after each
// respawn -- but that only says it is the direction and the rate the ring
// spins at, which is a name a naming round should settle rather than this
// parcel.
extern "C" {
	// The pattern [boss.mode] last ran, so that rolling a new one can reject
	// a repeat.
	extern unsigned char marisa_pattern_prev;

	// [bits_alive] at the moment the current pattern was chosen. Zero here
	// AND zero now is what makes an interval count as bit-less.
	extern unsigned char marisa_bits_at_pattern_start;

	// Direction of the background's blue pulse: `false` while color 0 climbs
	// to 192, `true` while it falls back to 38.
	extern bool marisa_pulse_dimming;

	extern unsigned char marisa_25671;

	// Bit-less intervals since the last time the bits were respawned. The
	// second one forces pattern 0, which is the one that spawns them.
	extern unsigned char marisa_patterns_without_bits;

	// How many of the three HP milestones (4500, 2500, 1000) have already
	// paid out. Doubles as the explosion type each one shows.
	extern unsigned char marisa_explode_milestone;
}

// b4m.cpp spells this one the same way, for the same slot.
#define flystep_pointreflected_tick boss_statebyte[13]

// The five other [boss_statebyte] slots this half of the fight uses, under the
// names th04_main.asm's own `boss_statebyte_t` overlay gives them. That overlay
// is a `union`, so every one of the first four is the SAME byte 15 -- one slot,
// reused by patterns that can never run at the same time, and the dump keeps
// all four spellings because the four meanings have nothing to do with each
// other.

// [boss.phase_frame] as of the last frame on which at least one bit was still
// alive. Patterns 1 and 2 subtract it from their own length to get the time
// Marisa has left to fly once she is on her own.
#define last_frame_with_bits_alive  boss_statebyte[15]

// Pattern 3's sweep counter, whose lowest bit also decides the sweep direction.
#define subpattern_num              boss_statebyte[15]

// Cleared on the frame a pattern fires, and set again by its first bit-less
// frame, which is the one that rolls the base angle.
#define bitless_pattern_started     boss_statebyte[15]

// +1 or -1, the amount pattern 11 turns its ring by on every volley.
#define delta_angle_between_rings   boss_statebyte[15]

// Coin flip that decides whether pattern 7 negates the angle its bits fire at,
// mirroring the whole sweep across the vertical axis.
#define angle_mirror_y              boss_statebyte[15]

// The speed pattern 7's blue-ball volley is fired at, growing by 2 subpixels on
// every one of them.
#define spread_speed                boss_statebyte[14]

// Declared FAR here, and only here. th04/main/boss/bosses.hpp declares it
// `near`, which is what it is -- but a near reference under this object's
// `-zPmain_03` frames its offset on main_03, and this function is in main_01.
// kb/codegen/0162.
void pascal far reimu_marisa_bg_render(void);
/// ---------

/// Constants
/// ---------
// Marisa's cels are absolute patnums, like Yuuka's: marisa_fg_render() blits
// [boss.sprite] as it stands, with no per-boss base added.
static const int PAT_MARISA_FIGHT = (PAT_STAGE + 1);

static const int MARISA_HP = 6000;

// The fight ends on its own after this many intervals, and that timeout is the
// one way to reach the defeat phase without the bonus.
static const int MARISA_INTERVALS_MAX = 52;

// Frames an interval lasts before the next pattern is rolled.
static const int MARISA_INTERVAL_FRAMES = 64;

// [boss.mode] values that are not patterns.
static const unsigned char MODE_PATTERN_0 = 0;
static const unsigned char MODE_INTERVAL = 255;

// The two patterns that run while no bit is alive, picked between at random.
static const unsigned char MODE_BITLESS_FIRST = 10;
/// ---------

/// Marisa's bits
/// -------------
/// The three helpers ZUN put at the head of ENM_BTPL_TEXT, ahead of all eleven
/// of her patterns. Every one of them walks all four [bits] the same way: a
/// near pointer in SI and a separate counter, never the pointer alone.
///
/// `static` is correct for all three: marisa_bits_update_and_hittest() is only
/// called by marisa_update(), marisa_bits_respawn() is only called by pattern
/// 0, and marisa_bits_fire() has seven callers among the patterns below. All of
/// those callers now live in this translation unit.

// Brings all four bits back at once, evenly spread around Marisa and moving
// outwards. Each one starts at zero distance with the same speed, and
// BF_MOVEOUT_SPIN is what makes marisa_bits_update_and_hittest() stop that
// motion again once the bit is far enough out. The first bit's angle is
// random and each further one is a quarter turn ahead of it, so the spread is
// fixed and only its rotation is not.
//
// Called from pattern 0 (marisa_16E9D() below) and from nowhere else,
// which is why marisa_update() forces that pattern after two bit-less
// intervals.
static void near marisa_bits_respawn(void)
{
	enum {
		MOVEOUT_SPEED = TO_SP(2),
	};

	// [bp-1] and [bp-0Ah] in declaration order, with [bp-2] left unused by the
	// array's even alignment. (kb/codegen/0010)
	unsigned char angle;
	bit_hp_t bit_hp = MARISA_BIT_HP;

	angle = randring2_next16();

	// The two register variables: the bit pointer earns SI by how often it is
	// dereferenced as a base (kb/codegen/0117), the counter takes DI.
	bit_t near *bit = bits;
	int i;

	for(i = 0; i < BIT_COUNT; i++, bit++) {
		bit->flag = BF_MOVEOUT_SPIN;
		bit->angle_speed = marisa_25671;
		bit->angle = angle;
		// A full turn divided evenly between the bits, and deliberately NOT
		// an `enum` constant beside MOVEOUT_SPEED: under `-b-` a 64-valued
		// enum is byte-sized, which folds this whole statement into one
		// `add byte ptr [bp-1], 64`. The original keeps AL live across the
		// store instead (kb/codegen/0032), which an `int` constant is what
		// produces. 1 byte. `[measured]`
		angle += (0x100 / BIT_COUNT);
		bit->patnum = static_cast<main_patnum_t>(i + PAT_MARISA_BIT);
		bit->distance.v = 0;
		bit->hp = bit_hp.v[i];
		bit->moveout_speed.v = MOVEOUT_SPEED;
	}
}

// Everything the bits do on every frame regardless of Marisa's pattern:
// advancing their spin and their outwards motion, deriving the new center
// position from the two, running both hittests, and rebuilding the two
// [bit_center_*] arrays that the renderer and the patterns read.
//
// The bit-less frames matter as much as the others: this function is also the
// only writer of [bits_alive], which it recounts from scratch every frame, and
// the [bit_center_*] arrays are indexed by that running count rather than by
// the bit's own slot -- so they are packed, and a dead bit leaves no hole.
static void near marisa_bits_update_and_hittest(void)
{
	enum {
		// Damage box against the player's shots.
		SHOT_HITBOX_RADIUS = TO_SP(12),

		// ... and against the player, who gets the same box expressed as a
		// top-left corner and an extent, because the original tests it with
		// two unsigned comparisons rather than four signed ones.
		PLAYER_HITBOX_EXTENT = TO_SP(24),

		// Distance at which BF_MOVEOUT_SPIN stops the outwards motion, and
		// the one at which the state after it advances again.
		MOVEOUT_DISTANCE = TO_SP(64),
		SPIN_DISTANCE = TO_SP(4),

		KILL_SCORE = 5120,
	};

	// [i] and [top] are [bp-2] and [bp-4] in declaration order
	// (kb/codegen/0010); [patnum] is the one that wins DI.
	//
	// ZUN reuses that single variable for two unrelated things — the kill
	// animation's patnum up here, and the player hitbox's left edge further
	// down — and it has to be ONE function-scope variable to match. Written
	// the natural way, as two variables in the two inner blocks that use
	// them, Turbo C++ ranks neither above [i], puts [i] in DI, homes both of
	// them on the stack and the function comes out 4 bytes short. `register`
	// on both does not fix it either: it moves them into CX rather than DI.
	// `[measured]`
	int i;
	int patnum;
	subpixel_t top;

	bits_alive = 0;

	bit_t near *bit = bits;

	for(i = 0; i < BIT_COUNT; i++, bit++) {
		if(bit->flag == BF_FREE) {
			continue;
		}

		// The kill animation runs entirely out of [flag], which doubles as its
		// frame counter from BF_KILL_ANIM upwards. A bit in this state is not
		// alive, so it is not counted and it has no hitbox.
		if(bit->flag >= BF_KILL_ANIM) {
			patnum = ++reinterpret_cast<unsigned char &>(bit->flag);
			patnum = (
				((patnum - BF_KILL_ANIM) / BIT_KILL_FRAMES_PER_CEL) +
				PAT_ENEMY_KILL
			);
			bit->patnum = static_cast<main_patnum_t>(patnum);
			if(patnum >= (PAT_ENEMY_KILL + ENEMY_KILL_CELS)) {
				bit->flag = BF_FREE;
			}
			continue;
		}

		bit->angle += bit->angle_speed;
		bit->distance.v += bit->moveout_speed.v;
		vector2_at(
			bit->center,
			boss.pos.cur.x.v,
			boss.pos.cur.y.v,
			bit->distance.v,
			bit->angle
		);

		// Both arms advance [flag] by one rather than assigning the successor
		// state, and BF_SPIN's threshold is met the moment the bit exists --
		// so BF_SPIN lasts exactly one frame, and the state the bits actually
		// spin in is the unnamed one after it. A pattern that pulls them back
		// in does so by setting [moveout_speed] negative, and never returns
		// them to either of these two states.
		if(bit->flag == BF_MOVEOUT_SPIN) {
			if(bit->distance.v >= MOVEOUT_DISTANCE) {
				reinterpret_cast<unsigned char &>(bit->flag)++;
				bit->moveout_speed.v = 0;
			}
		} else if(bit->flag == BF_SPIN) {
			if(bit->distance.v >= SPIN_DISTANCE) {
				reinterpret_cast<unsigned char &>(bit->flag)++;
			}
		}

		// th04/main/player/shot.hpp's inline shots_hittest() overload, spelled
		// out because this object deliberately does not include that header.
		shot_hitbox_radius.x.v = SHOT_HITBOX_RADIUS;
		shot_hitbox_radius.y.v = SHOT_HITBOX_RADIUS;
		shot_hitbox_center.x.v = bit->center.x.v;
		shot_hitbox_center.y.v = bit->center.y.v;
		bit->damage_this_frame = shots_hittest();

		bit->hp -= bit->damage_this_frame;
		if(bit->hp <= 0) {
			bit->patnum = PAT_ENEMY_KILL;
			bit->flag = BF_KILL_ANIM;
			snd_se_play(3);
			score_delta += KILL_SCORE;

			// kb/codegen/0083: the original reaches sparks_add_random()
			// through a nopcall -- 90 0E E8 plus a 16-bit displacement --
			// which a C++ far call
			// never becomes. Identical in length to the `9A` it would emit,
			// so only the bytes differ -- and the two SI-relative pushes have
			// to be `db`s as well, because naming SI to the inline assembler
			// would move [patnum] out of DI. Same island, same callee, as
			// th04/main/bullet/update.cpp's grazing branch.
			/* TODO: Replace with the decompiled call
			 * 	sparks_add_random(bit->center.x, bit->center.y, TO_SP(4), 8);
			 * once that function is part of the same segment */
			_asm {
				db  	0xFF, 0x74, 0x02;
				db  	0xFF, 0x74, 0x04;
				db  	0x66, 0x68, 8, 0x00, (4 * 16), 0x00;
				nop;
				push	cs;
				call	near ptr sparks_add_random;
			}
			continue;
		}

		// The player hittest, biased into the top-left corner of the bit's
		// box so that one unsigned comparison per axis covers both directions
		// -- `jnb`, not `jge`, is what says these are unsigned. [patnum]'s
		// second life as the left edge starts here.
		patnum = (bit->center.x.v + TO_SP(-12));
		top = (bit->center.y.v + TO_SP(-12));
		if(
			(static_cast<unsigned int>(player_pos.cur.x.v - patnum) <
				PLAYER_HITBOX_EXTENT) &&
			(static_cast<unsigned int>(player_pos.cur.y.v - top) <
				PLAYER_HITBOX_EXTENT)
		) {
			player_is_hit = true;
		}

		// Marisa's own [homing_target], set by marisa_update() just before it
		// calls this function, loses to any bit that is further up the
		// playfield.
		if(bit->center.y.v < homing_target.y.v) {
			homing_target.x.v = bit->center.x.v;
			homing_target.y.v = bit->center.y.v;
		}

		bit_center_x[bits_alive] = bit->center.to_screen_left();
		bit_center_y[bits_alive] = bit->center.to_screen_top();
		bits_alive++;
	}
}

// Fires every bit that is alive, through whichever [bit_fire] the current
// pattern installed. Not gated on anything else: a pattern that wants a subset
// has to say so inside its own [bit_fire].
static void near marisa_bits_fire(void)
{
	bit_t near *bit = bits;
	int i;

	for(i = 0; i < BIT_COUNT; i++, bit++) {
		if(bit->flag == BF_FREE) {
			continue;
		}
		if(bit->flag >= BF_KILL_ANIM) {
			continue;
		}
		bit_fire(*bit);
	}
}

/// Marisa's patterns
/// -----------------

// The one bit-less pattern that fires: a pair of clouds of 16x16 blue balls,
// spat out of two points 6 pixels to either side of Marisa on every second
// frame, each one a spread of 1 to 4 bullets at a random speed. The shared
// angle starts pointing straight down and sweeps backwards by 8 per volley,
// which is also the pattern's clock -- it ends on the frame that sweep wraps
// back around to 0, 32 frames after it started.
//
// [charge] is `unsigned char` and not `marisa_charge_t`: Turbo C++ 4.02 gives
// a byte-sized *enum* local a word-aligned slot ([bp-2]) and only a
// `char`-family local the odd one ([bp-1]) that the original uses. Both are
// one byte, and both compile the comparisons below to the same
// `CMP BYTE PTR` -- the enum spelling costs nothing but the frame offset.
static void near marisa_16DFF(void)
{
	unsigned char charge = marisa_charge_animate();

	if(charge == MC_FIRE) {
		bullet_template.spawn_type = BST_BULLET16_CLOUD_FORWARDS;
		bullet_template.patnum = PAT_BULLET16_N_BALL_BLUE;
		bullet_template.group = BG_SPREAD;
		bullet_template.angle = 0x80;
		bullet_template.delta.spread_angle = 8;
		bullet_template_tune();
	} else if(charge == MC_RUNNING) {
		if((boss.phase_frame % 2) == 0) {
			// kb/codegen/0032, twice per volley: the original increments and
			// adds to the returned byte in AL, where the same arithmetic
			// written on the 16-bit return value widens to `INC AX` and
			// `ADD AX`.
			_AL = randring2_next16_and(3);
			_AL++;
			bullet_template.count = _AL;
			_AL = randring2_next16_and(0x1F);
			_AL += TO_SP(2);
			bullet_template.speed.v = _AL;

			bullet_template.angle -= 8;
			bullet_template.origin.x.v -= TO_SP(6);
			bullets_add_regular();

			_AL = randring2_next16_and(3);
			_AL++;
			bullet_template.count = _AL;
			_AL = randring2_next16_and(0x1F);
			_AL += TO_SP(2);
			bullet_template.speed.v = _AL;

			// marisa_update() re-seeds the origin from [boss.pos] on every
			// frame, so this 12-pixel step never accumulates: it just moves
			// the second cloud to the mirrored point.
			bullet_template.origin.x.v += TO_SP(12);
			bullets_add_regular();
		}
		if(bullet_template.angle == 0) {
			boss.phase_frame = 0;
			boss.mode = MODE_INTERVAL;
			boss.sprite = PAT_MARISA_FIGHT;
		}
	}
}

// Pattern 0, and the only one that ever brings the bits back: it fires nothing
// itself, and just runs a gather animation for 64 frames before respawning all
// four of them and flipping the direction they will spin in. The charge
// function draws no ring for [boss.mode] 0, which is why this pattern has to
// stack its own three circles -- larger, and in different colors than the ones
// every other pattern gets.
//
// The three gather frames share a tail rather than repeating the call, which
// is why case 34 is written between 32 and 36 and still ends in the same
// gather_add_only(): Turbo C++ merges the identical epilogues, leaving case 34
// with a `JMP SHORT` into case 36's body and case 32 falling straight through
// into it.
static void near marisa_16E9D(void)
{
	marisa_charge_animate();
	switch(boss.phase_frame) {
	case 32:
		gather_template.center.x.v = boss.pos.cur.x.v;
		gather_template.center.y.v = boss.pos.cur.y.v;
		gather_template.ring_points = 32;
		gather_template.angle_delta = -2;
		gather_template.col = 9;
		gather_template.radius.v = TO_SP(256);
		gather_add_only();
		break;
	case 34:
		gather_template.col = 8;
		gather_add_only();
		break;
	case 36:
		gather_add_only();
		break;
	case 64:
		marisa_bits_respawn();

		// Negated rather than assigned, so that every respawn sends the ring
		// around the other way.
		marisa_25671 = -marisa_25671;
		break;
	case 96:
		boss.phase_frame = 0;
		boss.mode = MODE_INTERVAL;
		boss.sprite = PAT_MARISA_FIGHT;
		break;
	}
}

// Pattern 1's [bit_fire] callback: a spread fired sideways out of the bit,
// 90 degrees off the angle it currently sits at and on whichever side it is
// orbiting *away* from. Widened from the 3 bullets the pattern configures to
// 5 once no more than two bits are left, so that losing bits costs the player
// less than it looks like it should.
//
// The cast is load-bearing (kb/codegen/0142): only `signed char` narrows this
// test to `CMP BYTE PTR [SI+...], 0` with a signed branch.
// [bit_t::angle_speed] is declared plain `char`, which compiles the same test
// to `CBW` + `OR AX, AX` and costs 2 bytes.
static void pascal near marisa_bit_fire_16F24(bit_t near& bit)
{
	unsigned char angle;

	if(bits_alive <= 2) {
		bullet_template.count = 5;
	}
	angle = (
		(static_cast<signed char>(bit.angle_speed) >= 0) ? -0x40 : 0x40
	);
	bullet_template.angle = (bit.angle + angle);
	bullet_template.origin = bit.center;
	bullets_add_regular();
}

// Pattern 1: the bits do the shooting for as long as any of them are alive,
// each one spitting a sideways 3-way pellet spread every 4th frame. Once they
// are all gone, Marisa takes the pattern over herself -- flying to the point
// reflection of her position across the moon in exactly the frames the pattern
// has left, and firing a 4-way of 3-bullet blue spreads, 0x40 apart, on the
// same 4-frame clock. [last_frame_with_bits_alive] is what makes that
// hand-over seamless; it is also what turns the flight into a division by zero
// if the last bit happens to die on frame 146 or 147 (see
// marisa_flystep_pointreflected()'s own ZUN bug note).
//
// The pattern always runs its full 160 frames, and closes by reversing every
// bit's spin -- including the dead ones, whose [angle_speed] pattern 0 will
// overwrite anyway.
static void near marisa_16F61(void)
{
	unsigned char charge;
	bit_t near *bit;
	int i;

	charge = marisa_charge_animate();
	if(charge == MC_FIRE) {
		bullet_template.spawn_type = BST_PELLET;
		bullet_template.speed.v = (TO_SP(3) + 8);
		bullet_template.group = BG_SPREAD;
		bullet_template.count = 3;
		bullet_template.delta.spread_angle = 8;
		bullet_template_tune();
		bit_fire = marisa_bit_fire_16F24;
		last_frame_with_bits_alive = boss.phase_frame;
	} else if(charge == MC_RUNNING) {
		if((boss.phase_frame % 4) == 0) {
			if(bits_alive != 0) {
				marisa_bits_fire();
				last_frame_with_bits_alive = boss.phase_frame;
			} else {
				marisa_flystep_pointreflected(
					160 - last_frame_with_bits_alive
				);
				bullet_template.spawn_type = BST_BULLET16;
				bullet_template.patnum = PAT_BULLET16_D_BLUE;
				bullet_template.speed.v = (TO_SP(3) + 4);
				bullet_template.group = BG_SPREAD;
				bullet_template.count = 3;
				bullet_template.delta.spread_angle = 6;
				bullet_template.angle += 6;
				bullet_template_tune();
				bullets_add_regular();
				bullet_template.angle += 0x40;
				bullets_add_regular();
				bullet_template.angle += 0x40;
				bullets_add_regular();
				bullet_template.angle += 0x40;
				bullets_add_regular();
			}
			snd_se_play(9);
		}
		if(boss.phase_frame >= 160) {
			for((bit = bits, i = 0); i < BIT_COUNT; (i++, bit++)) {
				bit->angle_speed = -bit->angle_speed;
			}
			boss.phase_frame = 0;
			boss.mode = MODE_INTERVAL;
			boss.sprite = PAT_MARISA_FIGHT;
		}
	}
}

/// Constants
/// ---------
// 1.5 pixels, the rate pattern 3 spirals the bits out and back in at. Not
// expressible as TO_SP() of an integer, exactly like marisa_flystep_random()'s
// SPEED_RANDOM_FAST in th04/main/boss/b4m.cpp.
static const subpixel_t BIT_DISTANCE_SPEED = (TO_SP(3) / 2);
/// ---------

// Pattern 2's per-bit fire callback: one bullet from the bit's own center,
// with everything else left to the [bullet_template] the pattern staged on its
// fire frame. The whole PlayfieldPoint is copied in one 32-bit move.
static void pascal near marisa_bit_fire_17061(bit_t near& bit)
{
	bullet_template.origin = bit.center;
	bullets_add_regular();
}

// Pattern 2. The fire frame doubles every bit's angular speed and points
// [bit_fire] at the single-pellet callback above; from then on, every 4th
// frame aims the template at the player and fires one pellet from each bit
// that is still alive.
//
// Once no bit is left, Marisa takes the volley over herself: a 3-way aimed
// spread of star bullets, and a flight to the point reflection of her position
// whose duration is exactly the frames remaining until the pattern's own
// 160-frame end -- which is what [last_frame_with_bits_alive] is for. Either
// way the volley speeds up by 0.25 pixels each time. The pattern closes by
// halving the bits' angular speed again, undoing the fire frame's doubling.
//
// Candidate divide-by-zero [inferred from the arithmetic alone; reachability
// and observable failure unverified]: [last_frame_with_bits_alive] is always a
// multiple of 4, so a last bit dying between frames 148 and 151 makes the
// duration below exactly 12 -- the one value
// marisa_flystep_pointreflected() divides by zero on.
static void near marisa_17079(void)
{
	unsigned char charge;

	// The bit pointer earns SI, the counter DI (kb/codegen/0117), and
	// [charge] is the function's only frame byte.
	bit_t near *bit;
	int i;

	charge = marisa_charge_animate();
	if(charge == MC_FIRE) {
		bullet_template.spawn_type = BST_PELLET;
		bullet_template.speed.v = TO_SP(1);
		bullet_template.group = BG_SINGLE;
		bullet_template_tune();
		bit_fire = marisa_bit_fire_17061;
		bit = bits;
		for(i = 0; i < BIT_COUNT; i++, bit++) {
			bit->angle_speed += bit->angle_speed;
		}
		last_frame_with_bits_alive = boss.phase_frame;
	} else if(charge == MC_RUNNING) {
		if((boss.phase_frame % 4) == 0) {
			bullet_template.angle = iatan2(
				(player_pos.cur.y.v - boss.pos.cur.y.v),
				(player_pos.cur.x.v - boss.pos.cur.x.v)
			);
			if(bits_alive != 0) {
				marisa_bits_fire();
				last_frame_with_bits_alive = boss.phase_frame;
			} else {
				marisa_flystep_pointreflected(
					160 - last_frame_with_bits_alive
				);
				bullet_template.spawn_type = BST_BULLET16;
				bullet_template.patnum = PAT_BULLET16_N_STAR;
				bullet_template.group = BG_SPREAD_AIMED;
				bullet_template.count = 3;
				bullet_template.delta.spread_angle = 0x0C;
				bullet_template.angle = 0;
				bullet_template_tune();
				bullets_add_regular();
			}
			bullet_template.speed.v += 4;	// 0.25 pixels
			snd_se_play(9);
		}
		if(boss.phase_frame >= 160) {
			bit = bits;
			for(i = 0; i < BIT_COUNT; i++, bit++) {
				// `char`, and therefore a signed halving: CBW / CWD /
				// SUB AX, DX / SAR AX, 1.
				bit->angle_speed /= 2;
			}
			boss.phase_frame = 0;
			boss.mode = MODE_INTERVAL;
			boss.sprite = PAT_MARISA_FIGHT;
		}
	}
}

// Pattern 3, which is really two unrelated patterns picked by whether any bit
// is still alive on the frame in question -- not once, at the start, but every
// frame, so killing the last bit switches Marisa over mid-pattern.
//
// With bits: a four-stage timeline on [boss.phase_frame]. The bits spiral
// outwards until frame 192, then fire a 3-way spread each every 4th frame --
// with a 16-bullet aimed ring on top of it every 32nd -- until 256, then
// spiral back in until 384, and the pattern ends by reversing the spin of
// every OTHER bit. That last loop really does step by two; the odd-indexed
// bits keep their direction.
//
// Without bits: a 96-frame flight to the point reflection of her position,
// with two randomised clouds of blue balls leaving either side of her every
// other frame. Their shared angle walks 8 units per volley, and
// [subpattern_num] both counts the sweeps and — through its lowest bit —
// decides the direction, flipping each time the angle reaches 0 or wraps past
// 0x80. Four sweeps end the pattern.
static void near marisa_1717D(void)
{
	unsigned char charge;
	bit_t near *bit;
	int i;

	charge = marisa_charge_animate();
	if(charge == MC_FIRE) {
		bullet_template.spawn_type = BST_PELLET;
		bit_fire = marisa_bit_fire_16F24;
		subpattern_num = 0;
	} else if(charge == MC_RUNNING) {
		if(bits_alive != 0) {
			if(boss.phase_frame <= 192) {
				bit = bits;
				for(i = 0; i < BIT_COUNT; i++, bit++) {
					bit->distance.v += BIT_DISTANCE_SPEED;
				}
			} else if(boss.phase_frame <= 256) {
				if((boss.phase_frame % 4) == 0) {
					if((boss.phase_frame % 32) == 0) {
						bullet_template.speed.v = TO_SP(2);
						bullet_template.group = BG_RING_AIMED;
						bullet_template.count = 16;
						bullet_template_tune();
						bullets_add_regular();
					}
					bullet_template.group = BG_SPREAD;
					bullet_template.count = 3;
					bullet_template.delta.spread_angle = 6;
					bullet_template_tune();

					// After the tuning, so the spread leaves at this exact
					// speed regardless of [playperf].
					bullet_template.speed.v = TO_SP(4);
					snd_se_play(9);
					marisa_bits_fire();
				}
			} else if(boss.phase_frame <= 384) {
				bit = bits;
				for(i = 0; i < BIT_COUNT; i++, bit++) {
					bit->distance.v -= BIT_DISTANCE_SPEED;
				}
			} else {
				bit = bits;
				for(i = 0; i < BIT_COUNT; i += 2, bit += 2) {
					bit->angle_speed = -bit->angle_speed;
				}
				goto pattern_over;
			}
		} else {
			marisa_flystep_pointreflected(96);
			if(subpattern_num == 0) {
				subpattern_num = 1;
				bullet_template.spawn_type = BST_BULLET16_CLOUD_FORWARDS;
				bullet_template.patnum = PAT_BULLET16_N_BALL_BLUE;
				bullet_template.group = BG_SPREAD;
				bullet_template.angle = 0x80;
				bullet_template.delta.spread_angle = 8;
				snd_se_play(15);
			}
			if((boss.phase_frame % 2) == 0) {
				// kb/codegen/0032: the original increments and offsets the
				// returned byte in AL and only then stores it, where
				// `and(3) + 1` would widen to an INC AX.
				_AL = randring2_next16_and(3);
				_AL++;
				bullet_template.count = _AL;
				_AL = randring2_next16_and(0x1F);
				_AL += TO_SP(1);
				bullet_template.speed.v = _AL;

				// One conditional expression, not a compound assignment: the
				// original loads [angle] separately in each arm and stores it
				// once after the join.
				bullet_template.angle = ((subpattern_num & 1)
					? (bullet_template.angle - 8)
					: (bullet_template.angle + 8)
				);
				bullet_template.origin.x.v -= TO_SP(6);
				bullets_add_regular();
				_AL = randring2_next16_and(3);
				_AL++;
				bullet_template.count = _AL;
				_AL = randring2_next16_and(0x1F);
				_AL += TO_SP(1);
				bullet_template.speed.v = _AL;
				bullet_template.origin.x.v += TO_SP(12);
				bullets_add_regular();

				// Unsigned, both of them: the sweep is over when the angle
				// comes back to 0 or crosses 0x80.
				if(
					(bullet_template.angle == 0) ||
					(bullet_template.angle >= 0x80)
				) {
					subpattern_num++;
					if(subpattern_num >= 4) {
pattern_over:
						boss.phase_frame = 0;
						boss.mode = MODE_INTERVAL;
						boss.sprite = PAT_MARISA_FIGHT;
					} else {
						snd_se_play(15);
					}
				}
			}
		}
	}
}

// Pattern 4. While at least one bit is alive, every 32nd frame fires an aimed
// ring out of every live bit, and the ring gets SMALLER as the bits die: 24
// bullets minus 2 per surviving bit, or a flat 28 once the second HP milestone
// has paid out. From frame 160 on, the pattern reverses the spin of every
// second bit and ends.
//
// With no bits left it becomes a completely different attack: Marisa flies to
// the point reflection of her position over 64 frames, and every 16th frame
// sprays 32 aimed red balls out of random points in a 64x64-pixel box around
// herself. Each bullet also draws its own speed, and the slow half of that
// draw is the interesting part -- a bullet below 4 pixels/frame gets a random
// angle out of a window that widens the slower it is, so the spray fans out
// exactly as far as it is late.
static void near marisa_17335(void)
{
	// [speed] is the word and [charge] the byte behind it, in that declaration
	// order (kb/codegen/0010). Neither can take a register: [charge] is a
	// byte, and [speed] is READ as one by the [bullet_template.speed] store
	// below (kb/codegen/0131), which is what leaves SI and DI to the loops.
	int speed;
	unsigned char charge;
	int i;
	bit_t near *bit;

	charge = marisa_charge_animate();
	if(charge == MC_FIRE) {
		bullet_template.spawn_type = BST_BULLET16_CLOUD_BACKWARDS;
		bullet_template.patnum = PAT_BULLET16_N_BALL_BLUE;
		bullet_template.speed.v = (TO_SP(3) + 2);
		bullet_template.group = BG_RING_AIMED;
		bullet_template.angle = 0x00;
		bullet_template_tune();
		bit_fire = marisa_bit_fire_17061;
	} else if(charge == MC_RUNNING) {
		if(bits_alive != 0) {
			if((boss.phase_frame % 32) == 0) {
				bullet_template.count = (24 - (bits_alive * 2));
				if(marisa_explode_milestone == 2) {
					bullet_template.count = 28;
				}
				marisa_bits_fire();
				snd_se_play(9);
			}
			if(boss.phase_frame >= 160) {
				// Every second bit, not every bit.
				bit = bits;
				for(i = 0; i < BIT_COUNT; i += 2, bit += 2) {
					bit->angle_speed = -bit->angle_speed;
				}
				goto phase_over;
			}
		} else {
			if(marisa_flystep_pointreflected(64)) {
				goto phase_over;
			}
			if((boss.phase_frame % 16) == 0) {
				bullet_template.spawn_type = BST_BULLET16_CLOUD_FORWARDS;
				bullet_template.patnum = PAT_BULLET16_N_BALL_RED;
				bullet_template.group = BG_SINGLE_AIMED;
				bullet_template.special_motion = BSM_NONE;
				bullet_template_tune();
				for(i = 0; i < 32; i++) {
					bullet_template.origin.x.v = (
						randring2_next16_mod(TO_SP(64)) +
						(boss.pos.cur.x.v - TO_SP(52))
					);
					bullet_template.origin.y.v = (
						randring2_next16_mod(TO_SP(64)) +
						(boss.pos.cur.y.v - TO_SP(40))
					);
					speed = (randring2_next16_mod(TO_SP(6)) + TO_SP(1));
					bullet_template.speed.v = speed;
					if(speed >= TO_SP(4)) {
						bullet_template.angle = 0x00;
					} else {
						// ... and [speed] is then reused as the WIDTH of the
						// angle window, which is why the fan is widest for
						// the slowest bullets.
						speed = (TO_SP(4) - speed);
						bullet_template.angle = (
							randring2_next16_mod(speed) - (speed / 2)
						);
					}
					bullets_add_special();
				}
				snd_se_play(15);
			}
		}
	}
	return;

	// Emitted once, at the end, and jumped to from both of the arms that end
	// the pattern -- not duplicated into either of them.
phase_over:
	boss.phase_frame = 0;
	boss.mode = MODE_INTERVAL;
	boss.sprite = PAT_MARISA_FIGHT;
}

// Pattern 5, and the longest single body in Marisa's chain. While bits are
// alive, [boss.phase_frame] walks five 32-frame windows, each firing a 4-way
// of decelerating stars every 4th frame and differing only in the angle those
// stars turn towards on the way out: 0x40, then an interleaved 0x30/0x50 pair,
// then 0x70, 0x10 and 0x40 again. The second window is also the one that is
// not a loop -- its four bullets alternate two turn targets, so ZUN spelled
// all four out -- and the sound effect all five windows end on is emitted
// exactly ONCE, inside that window, with the other four jumping into it.
//
// With no bits left, Marisa instead flies to the point reflection of her
// position over 128 frames while a 16-bullet stack sweeps around her: the base
// angle is rolled once, out of the 32 values just below straight down, and
// then turns by -8 on every 8th frame. The pattern ends when that sweep has
// carried the angle a little past half a turn, or on frame 256 if it has not.
static void near marisa_17491(void)
{
	unsigned char charge;
	int i;

	charge = marisa_charge_animate();
	if(charge == MC_FIRE) {
		bullet_template.spawn_type = BST_BULLET16;
		bullet_template.patnum = PAT_BULLET16_N_STAR;
		bullet_template.speed.v = (TO_SP(5) + 12);
		bullet_template.group = BG_SINGLE;
		bullet_template.special_motion = BSM_DECELERATE_TO_ANGLE;
		bullet_template_tune();
		bit_fire = marisa_bit_fire_17061;
		bitless_pattern_started = 0;
	} else if(charge == MC_RUNNING) {
		if(bits_alive != 0) {
			if(boss.phase_frame <= 96) {
				if((boss.phase_frame % 4) == 0) {
					bullet_template_special_angle.target = 0x40;
					bullet_template.angle = 0x10;
					for(i = 0; i < 4; i++) {
						bullets_add_special_fixedspeed();
						bullet_template.angle += 0x20;
					}
					goto fired;
				}
			} else if(boss.phase_frame <= 128) {
				if((boss.phase_frame % 4) == 0) {
					// The same 4-way, rotated by half a turn, but with two
					// turn targets alternating across its four bullets.
					bullet_template_special_angle.target = 0x30;
					bullet_template.angle = 0x90;
					bullets_add_special_fixedspeed();
					bullet_template_special_angle.target = 0x50;
					bullet_template.angle = 0xB0;
					bullets_add_special_fixedspeed();
					bullet_template_special_angle.target = 0x30;
					bullet_template.angle = 0xD0;
					bullets_add_special_fixedspeed();
					bullet_template_special_angle.target = 0x50;
					bullet_template.angle = 0xF0;
					bullets_add_special_fixedspeed();

					// The one copy of this call, shared by all five windows.
fired:
					snd_se_play(3);
				}
			} else if(boss.phase_frame <= 160) {
				if((boss.phase_frame % 4) == 0) {
					bullet_template_special_angle.target = 0x70;
					bullet_template.angle = 0x10;
					for(i = 0; i < 4; i++) {
						bullets_add_special_fixedspeed();
						bullet_template.angle += 0x20;
					}
					goto fired;
				}
			} else if(boss.phase_frame <= 192) {
				if((boss.phase_frame % 4) == 0) {
					bullet_template_special_angle.target = 0x10;
					bullet_template.angle = 0x10;
					for(i = 0; i < 4; i++) {
						bullets_add_special_fixedspeed();
						bullet_template.angle += 0x20;
					}
					goto fired;
				}
			} else if(boss.phase_frame <= 224) {
				if((boss.phase_frame % 4) == 0) {
					bullet_template_special_angle.target = 0x40;
					bullet_template.angle = 0x10;
					for(i = 0; i < 4; i++) {
						bullets_add_special_fixedspeed();
						bullet_template.angle += 0x20;
					}
					goto fired;
				}
			}
		} else {
			marisa_flystep_pointreflected(128);
			if(bitless_pattern_started == 0) {
				bullet_template.angle = (0x80 - randring2_next16_and(0x1F));
				bitless_pattern_started = 1;
			}
			if((boss.phase_frame % 8) == 0) {
				bullet_template.spawn_type = BST_BULLET16_CLOUD_FORWARDS;
				bullet_template.patnum = PAT_BULLET16_N_BALL_BLUE;
				bullet_template.group = BG_STACK;
				bullet_template.count = 16;
				bullet_template.delta.stack_speed.v = 5;
				bullet_template.speed.v = TO_SP(1);
				bullet_template_tune();
				bullets_add_regular_fixedspeed();
				bullet_template.angle += -8;
				snd_se_play(15);
			}

			// The sweep only ever decreases, so this is the window it reaches
			// after wrapping past 0 -- roughly 16 to 20 volleys in.
			if(
				(bullet_template.angle > 0x80) &&
				(bullet_template.angle <= 0xE0)
			) {
				goto phase_over;
			}
		}

		// Reached by every one of the five windows above, and the only end
		// condition the bit-ful half has.
		if(boss.phase_frame >= 256) {
phase_over:
			boss.phase_frame = 0;
			boss.mode = MODE_INTERVAL;
			boss.sprite = PAT_MARISA_FIGHT;
		}
	}
}

// Pattern 6. The bullet the fire frame sets up is never spawned there -- it
// only stages [bullet_template] for the bits, which fire it through
// marisa_bit_fire_17061().
//
// With bits alive this is two attacks back to back: for the first 128 frames,
// every 4th frame fires one star out of every live bit; for the next 64, a
// curtain rains in from three sides of the playfield instead, every 4th frame
// spawning one bullet each from the left edge, the right edge and the top, at
// a random point along that edge, a random speed between 1.0 and 2.9 pixels
// and a random angle that always points inwards. The pattern then stands still
// until frame 256.
//
// With no bits left it becomes Marisa's usual bit-less shape: a 160-frame
// flight to the point reflection of her position, with a 16-ball stack fired
// every 8th frame from a base angle rolled once at the start and turned by 8
// on every volley. That half ends 64 frames earlier, at frame 192.
static void near marisa_1769E(void)
{
	// `unsigned char` rather than `marisa_charge_t`, and the difference is
	// load-bearing: [measured] a lone `marisa_charge_t` local is homed at
	// `[bp-2]` -- Turbo C++ still reserves a word for the enum under `-b-`,
	// even though every access to it is byte-wide -- while a lone
	// `unsigned char` gets the `[bp-1]` the original uses. Both variants retain
	// the same two-byte frame and total length, so only a disassembly catches it.
	unsigned char charge = marisa_charge_animate();

	if(charge == MC_FIRE) {
		bullet_template.spawn_type = BST_BULLET16;
		bullet_template.patnum = PAT_BULLET16_N_STAR;
		bullet_template.group = BG_SINGLE;
		bullet_template.special_motion = BSM_NONE;
		bullet_template.angle = -0x40;
		bullet_template.speed.v = TO_SP(6);
		bit_fire = marisa_bit_fire_17061;
		bitless_pattern_started = 0;
		return;
	}
	if(charge != MC_RUNNING) {
		return;
	}
	if(bits_alive != 0) {
		if(boss.phase_frame <= 128) {
			if(stage_frame_mod4 == 0) {
				marisa_bits_fire();
				snd_se_play(9);
			}
			return;
		}
		if(boss.phase_frame <= 192) {
			if(stage_frame_mod4 != 0) {
				return;
			}
			// Left edge, aimed between straight right and straight down.
			// `0x30u`, not `0x30`: the unsigned literal is what keeps this a
			// `SUB AL` rather than an `ADD AL, -0x30`. kb/codegen/0022.
			bullet_template.origin.x.v = 0;
			bullet_template.origin.y.v = randring2_next16_mod(TO_SP(192));
			bullet_template.speed.v = (randring2_next16_and(0x1F) + TO_SP(1));
			bullet_template.angle = (0x30u - randring2_next16_and(0x1F));
			bullets_add_special();

			// Right edge, aimed between straight down and straight left.
			bullet_template.origin.x.v = TO_SP(384);
			bullet_template.origin.y.v = randring2_next16_mod(TO_SP(192));
			bullet_template.speed.v = (randring2_next16_and(0x1F) + TO_SP(1));
			bullet_template.angle = (randring2_next16_and(0x1F) + 0x50);
			bullets_add_special();

			// Top edge, aimed downwards to either side.
			bullet_template.origin.x.v = randring2_next16_mod(TO_SP(384));
			bullet_template.origin.y.v = 0;
			bullet_template.speed.v = (randring2_next16_and(0x1F) + TO_SP(1));
			bullet_template.angle = (randring2_next16_and(0x1F) + 0x30);
			bullets_add_special();
			return;
		}

		// A jump to the shared epilogue and a separate early exit, rather
		// than one guarded early exit over the whole bit-less half below:
		// that form is semantically identical and exactly as long, but it lets
		// Turbo C++ fold the jump over the bit-less half into the compare and
		// drop the unconditional one. The original keeps both, with the
		// conditional going to the shared epilogue instead.
		if(boss.phase_frame >= 256) {
			goto pattern_over;
		}
		return;
	}

	marisa_flystep_pointreflected(160);
	if(bitless_pattern_started == 0) {
		bullet_template.angle = randring2_next16_and(0x1F);
		bitless_pattern_started = 1;
	}
	if((boss.phase_frame % 8) == 0) {
		bullet_template.spawn_type = BST_BULLET16_CLOUD_FORWARDS;
		bullet_template.patnum = PAT_BULLET16_N_BALL_BLUE;
		bullet_template.group = BG_STACK;
		bullet_template.count = 16;
		bullet_template.delta.stack_speed.v = 5;
		bullet_template.speed.v = TO_SP(1);
		bullet_template_tune();
		bullets_add_regular_fixedspeed();

		// Incremented by a plain `int` literal, which is the spelling that
		// emits the original's AL round trip rather than an `ADD mem, imm8`.
		// kb/codegen/0094.
		bullet_template.angle += 8;
		snd_se_play(15);
	}
	if(boss.phase_frame < 192) {
		return;
	}
pattern_over:
	boss.phase_frame = 0;
	boss.mode = MODE_INTERVAL;
	boss.sprite = PAT_MARISA_FIGHT;
}

// Pattern 11, and one of the two that only ever run while no bit is alive: a
// 32-bullet pellet ring every 8th frame for 128 frames, turned by exactly one
// angle unit per volley. The coin flip on the fire frame only picks which way.
// Marisa does not move at all during it.
static void near marisa_17813(void)
{
	unsigned char charge = marisa_charge_animate();

	if(charge == MC_FIRE) {
		bullet_template.spawn_type = BST_PELLET;
		bullet_template.count = 32;
		bullet_template.group = BG_RING;
		bullet_template.speed.v = (TO_SP(3) + 8);
		bullet_template_tune();

		// `== 0` first, not `(rand ? -1 : 1)`: kb/codegen/0120's byte
		// conditional either way, but the arm the compare falls THROUGH to has
		// to be the +1 one to match the original's branch order.
		delta_angle_between_rings = ((randring2_next16_and(1) == 0) ? 1 : -1);
		return;
	}
	if(charge != MC_RUNNING) {
		return;
	}
	if((boss.phase_frame % 8) == 0) {
		bullets_add_regular();
		bullet_template.angle += delta_angle_between_rings;
		snd_se_play(9);
	}
	if(boss.phase_frame >= 128) {
		boss.phase_frame = 0;
		boss.mode = MODE_INTERVAL;
		boss.sprite = PAT_MARISA_FIGHT;
	}
}

// Pattern 7, the only one of Marisa's patterns that the regular reroll cannot
// reach: marisa_update() rolls 1 through 7 and rejects a repeat, so this one
// only ever comes up through the bit-less branch.
//
// With bits alive, every 4th frame fires one pellet out of every live bit at an
// angle that sweeps a full turn every 32 frames, optionally mirrored across the
// vertical axis for the whole pattern. Every 8th frame adds a second volley of
// blue balls straight ahead, and THAT one speeds up by 2 subpixels every time,
// so the two streams drift apart over the pattern's 160 frames. It ends by
// reversing the spin of every bit.
//
// With no bits left, it is a 72-frame flight to the point reflection with a
// 16-ball aimed stack every 8th frame, and it ends when the flight does.
static void near marisa_1788E(void)
{
	// [bit] before [i], because the original keeps the pointer in SI and the
	// counter in DI, and Turbo C++ hands out SI to whichever register variable
	// is declared first. kb/codegen/0146.
	unsigned char charge;
	bit_t near *bit;
	int i;

	charge = marisa_charge_animate();
	if(charge == MC_FIRE) {
		spread_speed = TO_SP(2);
		angle_mirror_y = randring2_next16_and(1);
	}

	// ZUN bloat, and it is ZUN's rather than a codegen artifact: the charge is
	// animated a SECOND time on every frame of this pattern, and it is the
	// second call's return value that decides whether the pattern runs. Since
	// marisa_charge_animate() also spawns the gather ring, the shrinking
	// circle and both sound effects, every one of its landmark frames happens
	// twice for this pattern alone. The homed [charge] above is only ever used
	// for the fire frame.
	if(marisa_charge_animate() != MC_RUNNING) {
		return;
	}
	if(bits_alive != 0) {
		if((boss.phase_frame % 4) == 0) {
			bit_fire = marisa_bit_fire_17061;
			bullet_template.spawn_type = BST_PELLET;
			bullet_template.speed.v = TO_SP(2);
			bullet_template.group = BG_SINGLE;

			// Narrowed to a byte by the store target, which is what keeps the
			// shift in AL and lets the mirror below reuse it as a bare `NEG`
			// under `-Z`. kb/codegen/0032.
			bullet_template.angle = (stage_frame << 3);
			if(angle_mirror_y != 0) {
				bullet_template.angle = -bullet_template.angle;
			}
			bullet_template_tune();
			marisa_bits_fire();
			if((boss.phase_frame % 8) == 0) {
				bit_fire = marisa_bit_fire_16F24;
				bullet_template.spawn_type = BST_BULLET16_CLOUD_FORWARDS;
				bullet_template.patnum = PAT_BULLET16_N_BALL_BLUE;
				bullet_template.speed.v = spread_speed;
				spread_speed += 2;
				bullet_template.group = BG_SINGLE;
				bullet_template.angle = 0;
				bullet_template_tune();
				marisa_bits_fire();
			}
			snd_se_play(9);
		}
		if(boss.phase_frame < 160) {
			return;
		}
		for(bit = bits, i = 0; i < BIT_COUNT; i++, bit++) {
			bit->angle_speed = -bit->angle_speed;
		}
	} else {
		if(!marisa_flystep_pointreflected(72)) {
			if((boss.phase_frame % 8) == 0) {
				bullet_template.spawn_type = BST_BULLET16_CLOUD_FORWARDS;
				bullet_template.patnum = PAT_BULLET16_N_BALL_BLUE;
				bullet_template.group = BG_STACK_AIMED;
				bullet_template.count = 16;
				bullet_template.delta.stack_speed.v = 5;
				bullet_template.angle = 0;
				bullet_template.speed.v = TO_SP(1);
				bullet_template_tune();
				bullets_add_regular_fixedspeed();
				snd_se_play(15);
			}
			return;
		}
	}
	boss.phase_frame = 0;
	boss.mode = MODE_INTERVAL;
	boss.sprite = PAT_MARISA_FIGHT;
}

// Damages Marisa by this frame's shot hits, divided by the number of bits
// still alive plus one, and returns `true` once that took her below
// [boss.phase_end_hp].
//
// The divide is SIGNED, which the original's `CWD`/`IDIV` says outright:
// [boss.damage_this_frame] is an `unsigned char` and [bits_alive] a `uint8_t`,
// but both promote to `int` before `/` ever sees them. This is also the one
// place in the fight that owns [boss.phase_frame], which is why none of the
// patterns above advance it.
static bool near marisa_179BC(void)
{
	boss.phase_frame++;
	boss.damage_this_frame = boss_hittest_shots_damage(TO_SP(24), TO_SP(24), 4);
	boss.hp -= (boss.damage_this_frame / (bits_alive + 1));
	if(boss.hp <= boss.phase_end_hp) {
		return true;
	}
	return false;
}


void pascal far marisa_update(void)
{
	int pattern;

	bullet_template.origin.x.v = (boss.pos.cur.x.v - TO_SP(20));
	bullet_template.origin.y.v = (boss.pos.cur.y.v - TO_SP(8));

	switch(boss.phase) {
	case 0:
		if(boss.phase_frame == 0) {
			boss.hp = MARISA_HP;
			marisa_25671 = 2;
		}
		boss_hittest_shots_invincible();
		if(boss.phase_frame > 96) {
			boss.phase++;
			Palettes[0].c.r = 0;
			Palettes[0].c.g = 0;
			Palettes[0].c.b = 7;
			palette_changed = true;
			boss.phase_frame = 0;
			snd_se_play(13);
			_asm mov word ptr bg_render_bombing_func, offset reimu_marisa_bg_render
			tiles_bb_col = V_WHITE;
			marisa_pulse_dimming = false;
		}
		break;

	case 1:
		boss.phase_frame++;
		boss_hittest_shots_invincible();
		if(boss.phase_frame >= 128) {
			boss.phase++;
			boss.pos.velocity.x.v = 0;
			boss.phase_state.patterns_seen = 0;
			boss.mode = MODE_BITLESS_FIRST;
			boss.phase_frame = 0;
			boss.sprite = PAT_MARISA_FIGHT;
			marisa_patterns_without_bits = 1;
			marisa_explode_milestone = 0;
			marisa_pattern_prev = MODE_BITLESS_FIRST;
			marisa_bits_at_pattern_start = 0;
			flystep_pointreflected_tick = 0;
		}
		break;

	case 2:
		switch(boss.mode) {
		case 0:
			marisa_16E9D();
			break;
		case 1:
			marisa_16F61();
			break;
		case 2:
			marisa_17079();
			break;
		case 3:
			marisa_1717D();
			break;
		case 4:
			marisa_17335();
			break;
		case 5:
			marisa_17491();
			break;
		case 6:
			marisa_1769E();
			break;
		case 7:
			marisa_1788E();
			break;
		case 10:
			marisa_16DFF();
			break;
		case 11:
			marisa_17813();
			break;
		case MODE_INTERVAL:
			marisa_flystep_random();
			if(boss.phase_frame >= MARISA_INTERVAL_FRAMES) {
				boss.phase_state.patterns_seen++;
				flystep_pointreflected_tick = 0;
				if((marisa_bits_at_pattern_start == 0) && (bits_alive == 0)) {
					marisa_patterns_without_bits++;
					if(marisa_patterns_without_bits >= 2) {
						boss.mode = MODE_PATTERN_0;
						marisa_patterns_without_bits = 0;
					} else {
						boss.mode = (
							randring2_next16_and(1) + MODE_BITLESS_FIRST
						);
					}
				} else {
					// Rerolled until it differs, which is why the eighth
					// pattern is only ever reachable through the bit-less
					// branch above.
					do {
						pattern = (randring2_next16_mod(7) + 1);
					} while(marisa_pattern_prev == pattern);
					boss.mode = pattern;
					marisa_pattern_prev = pattern;
					marisa_bits_at_pattern_start = bits_alive;
				}
				boss.phase_frame = 0;
				if(boss.phase_state.patterns_seen >= MARISA_INTERVALS_MAX) {
					boss.phase_state.defeat_bonus = false;
					goto phase_over;
				}
			}
			break;
		}
		if(marisa_179BC()) {
			boss.phase_state.defeat_bonus = true;
phase_over:
			boss_explode_small(ET_HORIZONTAL);
			boss.phase++;
			boss.phase_frame = 0;
		}

		if(stage_frame_mod4 == 0) {
			// kb/codegen/0032, and NOT a compound assignment to the same
			// palette component: a compound
			// assignment through Palette::operator []() materialises the far
			// reference it returns into a `LES BX` pair, which costs this
			// function 8 bytes of frame and 0x24 bytes of code. The original
			// is a byte load, a byte add and a byte store.
			if(!marisa_pulse_dimming) {
				_AL = Palettes[0].c.b;
				_AL += 2;
				Palettes[0].c.b = _AL;
				if(Palettes[0].c.b >= 192) {
					marisa_pulse_dimming = true;
				}
			} else {
				_AL = Palettes[0].c.b;
				_AL += -2;
				Palettes[0].c.b = _AL;
				if(Palettes[0].c.b <= 38) {
					marisa_pulse_dimming = false;
				}
			}
			palette_changed = true;
		}

		if(
			((boss.hp <= 4500) && (marisa_explode_milestone == 0)) ||
			((boss.hp <= 2500) && (marisa_explode_milestone == 1)) ||
			((boss.hp <= 1000) && (marisa_explode_milestone == 2))
		) {
			boss_items_drop();
			bullets_clear();
			boss_score_bonus(10);
			boss_explode_small(
				static_cast<explosion_type_t>(marisa_explode_milestone)
			);
			marisa_explode_milestone++;
		}
		break;

	case 3:
		boss.phase_frame++;
		if(boss.phase_frame == 16) {
			boss_explode_small(ET_VERTICAL);
		}
		if(boss.phase_frame == 32) {
			boss_defeat_explode_big(ET_SW_NE, 40);
			snd_se_play(12);

			// Only two of the three components, unlike every other TH04 boss.
			Palettes[0].c.r = 0;
			Palettes[0].c.b = 0;
			palette_changed = true;
			player_invincibility_time = BOSS_DEFEAT_INVINCIBILITY_FRAMES;
		}
		break;

	default:
		boss_defeat_update();
		return;
	}

	homing_target.x.v = boss.pos.cur.x.v;
	homing_target.y.v = boss.pos.cur.y.v;
	marisa_bits_update_and_hittest();
	hud_hp_update_and_render(boss.hp, MARISA_HP);
}
/// ------------------------------------------------------
