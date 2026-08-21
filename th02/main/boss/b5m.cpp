/// Stage 5 boss - Mima
/// -------------------
/// Her per-frame update, and the pattern-advance and vertical-drift helper it
/// is the only caller of. Together they are the carve-free tail of
/// th02_main.asm's BOSS_5_TEXT contribution, so this file needs no new segment
/// - th02/boss_5.cpp includes it directly ahead of skill_calculate(), which is
/// the one body at the address after mima_update()'s generated jump table.
/// (kb/codegen/0099)
///
/// mima_193A4() is `static`: mima_update() is its only caller, and the dump no
/// longer holds one.

#include "platform.h"
#include "pc98.h"
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/math/randring.hpp"
#include "th02/main/frames.hpp"
#include "th02/main/bg_particle.hpp"
#include "th02/main/bullet/bullet.hpp"
#include "th02/main/boss/boss.hpp"
#include "th02/main/player/player.hpp"
#include "th02/main/stage/stage.hpp"
#include "th02/hardware/pages.hpp"
#include "th02/main/item/item.hpp"
#include "th02/math/vector.hpp"
#include "th01/rank.h"
#include "th02/sprites/main_pat.h"

// Declared the way th02/main/boss/b4.cpp already declares them, rather than by
// including th02/resident.hpp and th02/core/globals.hpp - th02/main/boss/b5_.cpp
// is included into this same translation unit below and owns those two
// unguarded headers.
extern "C" bool reduce_effects;
extern "C" int boss_pos_x;
extern "C" int boss_pos_y;
extern char rank;
extern "C" uint8_t boss_rank_param[5];
extern "C" void __cdecl snd_se_play(int new_se);

// The sprite the boss and midboss renderers blit. Declared exactly the way
// th02/main/boss/b4.cpp and th02/main/boss/b5.cpp already declare it.
extern "C" int patnum_2064E;

/// Mima's other procs
/// ------------------
/// Her patterns, her renderers, and the hit test. All near, all still ASM in
/// BOSS_5_TEXT except mima_19C8D(), which is th02/main/boss/b5.cpp in
/// main_03__TEXT; this object reaches it near through the MAIN_03 group.

extern "C" void near mima_17C92(void);
extern "C" void near mima_17D59(void);
extern "C" void near mima_17F27(void);
extern "C" void near mima_180EC(void);
extern "C" void near mima_181B3(void);
extern "C" void near mima_188AA(void);
extern "C" void near mima_18905(void);
extern "C" void near mima_18A1B(void);
extern "C" void near mima_18B4B(void);
extern "C" void near mima_18BA6(void);
extern "C" void near mima_18C4A(void);
extern "C" void near mima_18DE0(void);
extern "C" void near mima_18EB8(void);
extern "C" bool16 near mima_19C8D(void);

// th02/main/boss/b5.cpp, in main_03__TEXT. Reached with the same
// `nop; push cs; call near ptr` same-segment far call as everywhere else in
// this group. (kb/codegen/0014)
extern "C" void far mima_end(void);

// Turbo C++ compiled ZUN's far calls to same-code-group functions as
// `nop; push cs; call near ptr`; a plain C++ far call is `call far ptr`
// instead, so only inline ASM still emits those bytes. Spelled exactly the way
// th02/main/stage/init.cpp and th02/main/stage/loop.cpp already spell it, and
// naming no register, so [i] below stays in SI. (kb/codegen/0014,
// kb/codegen/0083)
#define nopcall_same_group(func) _asm { \
	nop; \
	push	cs; \
	call	near ptr func; \
}
/// ----------------------

/// Mima's state
/// ------------
/// The six left_*/x_*/top_*/y_* slots below are the muzzle and anchor points
/// her patterns fire from; they keep the hand names the dump already gave them.
/// [mima_muzzle_left] and [mima_muzzle_top] are the pair mima_180EC() and
/// mima_181B3() use, and are the only two of the eight this parcel renames.

extern "C" screen_x_t left_26C56;
extern "C" screen_x_t mima_muzzle_left;
extern "C" screen_x_t left_26C5A;
extern "C" screen_x_t x_26C5C;
extern "C" screen_y_t top_26C5E;
extern "C" screen_y_t mima_muzzle_top;
extern "C" screen_y_t top_26C62;
extern "C" screen_y_t y_26C64;

// 0 until [mima_patterns_until_vulnerable] patterns of the current step have
// run, 1 afterwards. mima_17C92() multiplies the frame's shot damage by it, so
// Mima cannot be damaged during the opening patterns of any step.
// [marisa_damage_multiplier]'s mechanism, one boss up.
extern "C" int mima_damage_multiplier;

// The eleven-step machine the whole fight runs on. Even steps are the
// charge-ups, odd ones run patterns, 8 is her first defeat and 10 the end.
extern "C" int mima_phase;

// Which of her patterns is running, and how many have run in this step.
extern "C" int mima_pattern;
extern "C" int mima_patterns_this_phase;

// Raised in step 7, and never lowered. Widens the pattern cycle from
// [mima_pattern_count] to all 8, and switches six of the pattern functions to
// their harder variant.
extern "C" bool mima_all_patterns;

// The ring and the filled circle mima_17A7F() draws behind her, and their
// colors. th02/main/boss/b5.cpp named the ring's two colors _head and _tail;
// only the lagging one is ever reassigned after mima_init().
extern "C" uint8_t mima_bg_ring_radius;
extern "C" uint8_t mima_bg_circle_radius;
extern "C" uint8_t mima_bg_ring_col_tail;
extern "C" uint8_t mima_bg_circle_col;

// The four parameters each charge-up step hands the odd step that follows it.
// The step ends on either [mima_phase_damage_max] damage or
// [mima_patterns_max] patterns; steps 7 and 9 set the latter to 200 and so end
// on damage alone.
extern "C" int mima_phase_damage_max;
extern "C" int mima_patterns_max;
extern "C" int mima_pattern_count;
extern "C" int mima_patterns_until_vulnerable;

// The two expanding dot-square rings mima_update() blits itself, at this
// radius and this radius + 128 around a fixed (224, 200).
extern "C" uint8_t mima_ring_radius;

// The circle's radius pulses down from this base on a 64-frame counter.
extern "C" uint8_t mima_bg_circle_radius_base;
extern "C" uint8_t mima_bg_circle_pulse_frame;

// Her vertical drift, re-rolled once per pattern by mima_193A4().
extern "C" int mima_velocity_y;

// mima_19173()'s running spread angle, advanced by boss_rank_param[4] per shot.
extern "C" unsigned char mima_spiral_angle;

// `[measured]` mima_191CC() aims this at the player once, on the pattern's
// frame 50, and NOTHING ever reads it - not this proc, not any other in the
// binary. A dead store, spelled the way the dump already spells
// [boss_pos_x_unused].
extern "C" unsigned char mima_aim_angle_unused;

// The radius of mima_191CC()'s charge-up rings. Starts at 30 and shrinks by 2
// every other frame; the rings are blitted at this radius, twice it and four
// times it.
extern "C" unsigned char mima_charge_ring_radius;
/// ------------

static const int MIMA_RING_CENTER_X = 224;
static const int MIMA_RING_CENTER_Y = 200;
static const int MIMA_RING_COUNT = 2;
static const int MIMA_RING_ANGLE_STEP = 16;
static const int MIMA_RING_DISTANCE = 128;

static const int MIMA_CIRCLE_PULSE_FRAMES = 64;

// How long each charge-up step's palette fade runs for.
static const int MIMA_CHARGE_FRAMES = 100;

// Her drift only re-aims while a pattern is this young, and only inside this
// vertical band.
static const int MIMA_DRIFT_FRAMES = 10;
static const int MIMA_TOP_MIN = 48;
static const int MIMA_TOP_MAX = 64;

// mima_19353()'s window and cadence.
static const int MIMA_SPREAD_FIRST_FRAME = 50;
static const int MIMA_SPREAD_LAST_FRAME = 190;
static const int MIMA_SPREAD_INTERVAL = 32;

// mima_19173()'s.
static const int MIMA_SPIRAL_FIRST_FRAME = 50;
static const int MIMA_SPIRAL_LAST_FRAME = 150;

// mima_191CC()'s two halves.
static const int MIMA_CHARGE_FIRST_FRAME = 50;
static const int MIMA_CHARGE_LAST_FRAME = 80;
static const int MIMA_CHARGE_RING_RADIUS_INITIAL = 30;
static const int MIMA_SPRAY_LAST_FRAME = 200;


/// Her four orbs, and the five patterns that were the tail of BOSS_5_TEXT
/// ---------------------------------------------------------------------

// The orbs' positions, per page. `[measured]` The same 80-byte run as
// [mima_orb_flag], which th02/main/boss/b5.cpp already published: five 8-entry
// word arrays 16 bytes apart, of which these two are the left/top pair. Only
// the lower four slots are ever written. (kb/codegen/0123)
extern "C" screen_x_t mima_orb_left_on_page[PAGE_COUNT][8];
extern "C" screen_y_t mima_orb_top_on_page[PAGE_COUNT][8];
extern "C" int16_t mima_orb_flag[8];

// Where the orbs orbit, and how far out they are. `[measured]` ZUN quirk: the
// seeding pass below divides the centre by 16 and the per-frame pass does not,
// so the four orbs spend their first frame at a sixteenth of the distance from
// the origin that every later frame puts them at.
extern "C" screen_x_t mima_orb_center_x;
extern "C" screen_y_t mima_orb_center_y;
extern "C" int16_t mima_orb_radius;

// The angle of the first orb; the other three are a quarter-turn apart. Walks
// back by 3 every frame, so the formation counter-rotates as it expands.
extern "C" unsigned char mima_orb_angle;

// Which of two patterns mima_183D0() - still ASM, and its only reader - is
// running. `[measured]` mima_188AA() clears it and mima_18B4B() sets it, and
// the flag picks between the two halves of that function's body: the cleared
// one detonates each orb into two 16-pellet rings once the formation has
// closed back in, the set one does not.
extern "C" bool mima_orb_variant;

// The worker both orb patterns hand their velocity vector to. Still ASM in
// this segment, and published for this object's sake.
extern "C" void pascal near mima_183D0(int *velocity_x, int *velocity_y);

// mima_18B4B()'s half of that vector. Kept under the dump's own hand name,
// because its twin [point_26CD6] belongs to mima_188AA(), which is still ASM:
// renaming one of a matched pair and not the other reads worse than renaming
// neither.
extern "C" screen_point_t point_26CDE;

// The aim angle of her bouncing-star pattern, and the direction it sweeps in.
extern "C" unsigned char mima_star_angle;
extern "C" signed char mima_star_direction;

// The running angle of her symmetric pellet fan.
extern "C" unsigned char mima_fan_angle;

static const int MIMA_ORB_COUNT = 4;

// A quarter turn, so the four orbs sit on the corners of a square.
static const unsigned char MIMA_ORB_ANGLE_STEP = 0x40;

// One mirrored pair of her pellet fan, and the 10 steps the angle then walks.
#define mima_fan_fire() { 	bullets_add_pellet( 		left_26C5A, top_26C62, (mima_fan_angle + 0x08), 		BG_3_SPREAD_NARROW, (5 << 4) 	); 	bullets_add_pellet( 		left_26C5A, top_26C62, (0x78 - mima_fan_angle), 		BG_3_SPREAD_NARROW, (5 << 4) 	); 	mima_fan_angle = (mima_fan_angle + 0x0A); }

// One star of the bouncing-star pattern. Every one of the six is fired from
// the same muzzle at the same speed, so only the angle differs.
#define mima_star_add(angle) bullets_add_16x16( \
	left_26C5A, \
	top_26C62, \
	(angle), \
	BSM_BOUNCE_TOP_BOTTOM, \
	PAT_BULLET16_STAR, \
	(5 << 4) \
)


// Her orb pattern's setup, and the one that leaves [mima_orb_variant] set: on
// phase frame 40, aim a 52-long vector from her middle at the player and hand
// it to mima_183D0(), which flies the four orbs along it from then on.
extern "C" void near mima_18B4B(void)
{
	int x1;
	int y1;

	if(boss_phase_frame < 40) {
		return;
	}
	if(boss_phase_frame == 40) {
		mima_orb_variant = true;
		x1 = (*boss_left_on_back_page + 80);
		y1 = (*boss_top_on_back_page + 64);
		vector2_between_plus(
			x1, y1, player_topleft.x, player_topleft.y, 0,
			point_26CDE.x, point_26CDE.y, 52
		);
	}
	mima_183D0(&point_26CDE.x, &point_26CDE.y);
}


// The three sound cues of her defeat, and the item it drops: a bomb if she was
// killed in step 6, an extra life in step 2, and a big power item otherwise.
extern "C" void near mima_18BA6(void)
{
	if(boss_phase_frame < 10) {
		return;
	}
	if(boss_phase_frame == 10) {
		snd_se_play(9);
		patnum_2064E = 131;
		return;
	}
	if(boss_phase_frame < 40) {
		return;
	}
	// items_add()'s parameters are (left, top, type), which is th02/item.cpp's
	// definition order rather than th02/main/item/item.hpp's declaration
	// order. th02/main/boss/b3.cpp carries the same note.
	if(boss_phase_frame == 40) {
		snd_se_play(10);
		patnum_2064E = 134;
		if(mima_phase == 6) {
			items_add(left_26C5A, top_26C62, IT_BOMB);
			return;
		}
	} else if(boss_phase_frame == 60) {
		snd_se_play(10);
	} else if(boss_phase_frame == 80) {
		snd_se_play(10);
		patnum_2064E = 128;
		if(mima_phase == 2) {
			items_add(left_26C5A, top_26C62, IT_1UP);
			return;
		}
	} else {
		return;
	}
	items_add(left_26C5A, top_26C62, IT_BIGPOWER);
}


// Her bouncing-star pattern: she closes in on the player for 60 frames, then
// fires four stars - two on Easy - every 8th frame from phase frame 100, on an
// aim angle that sweeps one step per odd frame and sweeps back again at 210.
extern "C" void near mima_18C4A(void)
{
	if(boss_phase_frame < 10) {
		return;
	}
	if(boss_phase_frame < 70) {
		patnum_2064E = 128;
		// A conditional expression rather than two assignments: the original
		// picks the step in AX and adds it through the pointer once, at the
		// join.
		*boss_left_on_back_page += (
			((*boss_left_on_back_page + 64) < player_topleft.x) ? 2 : -2
		);
		return;
	}
	if(boss_phase_frame == 70) {
		snd_se_play(9);
		patnum_2064E = 131;
		return;
	}
	if(boss_phase_frame < 100) {
		return;
	}
	if(boss_phase_frame == 100) {
		snd_se_play(10);
		patnum_2064E = 134;
		mima_star_angle = iatan2(
			(player_topleft.y - top_26C62), (player_topleft.x - left_26C5A)
		);
		// Spelled as the LEFT half's condition, which is what puts the +1 into
		// the branch and the -1 into the fall-through.
		mima_star_direction = (
			(player_topleft.x <= PLAYER_LEFT_START) ? -1 : 1
		);
		bullet_special.u3.turns_max = mima_all_patterns;
		return;
	}
	if(boss_phase_frame <= 300) {
		if((boss_phase_frame % 8) == 0) {
			snd_se_play(10);
			if(rank != RANK_EASY) {
				mima_star_add(mima_star_angle + 0x0A);
				mima_star_add(mima_star_angle - 0x0A);
				mima_star_add(mima_star_angle + 0x1E);
				mima_star_add(mima_star_angle - 0x1E);
			} else {
				mima_star_add(mima_star_angle + 0x0F);
				mima_star_add(mima_star_angle - 0x0F);
			}
		}
		if(boss_phase_frame < 150) {
			return;
		}
		if(!(boss_phase_frame & 1)) {
			return;
		}
		if(boss_phase_frame < 210) {
			mima_star_angle += mima_star_direction;
			return;
		}
		mima_star_angle -= mima_star_direction;
		return;
	}
	boss_phase_frame = 0;
	patnum_2064E = 128;
}


// Her symmetric pellet fan: the same 60-frame approach, then a mirrored pair
// of narrow 3-spreads every 10th frame from phase frame 120, with the angle
// walking 10 steps per pair.
extern "C" void near mima_18DE0(void)
{
	if(boss_phase_frame < 30) {
		return;
	}
	if(boss_phase_frame < 70) {
		patnum_2064E = 128;
		*boss_left_on_back_page += (
			((*boss_left_on_back_page + 64) < player_topleft.x) ? 2 : -2
		);
		return;
	}
	if(boss_phase_frame == 70) {
		snd_se_play(9);
		patnum_2064E = 131;
		return;
	}
	if(boss_phase_frame < 120) {
		return;
	}
	// Written out in both arms rather than after the `if`: the original falls
	// straight from the frame-120 arm into this block and jumps BACKWARD into
	// it from the other, which is Turbo C++ 4.02's -O cross-jump on two
	// identical tails. Hoisting it below the `if` puts it after the whole
	// chain and turns the first arm's fall-through into a forward jump.
	if(boss_phase_frame == 120) {
		snd_se_play(3);
		patnum_2064E = 134;
		mima_fan_angle = 0x04;
		mima_fan_fire();
		return;
	} else if(boss_phase_frame <= 160) {
		if((boss_phase_frame % 10) != 0) {
			return;
		}
		snd_se_play(3);
		mima_fan_fire();
		return;
	}
	boss_phase_frame = 0;
	patnum_2064E = 128;
}


// Her orb pattern proper: four orbs seeded on a square around her at phase
// frame 80, expanding by 2 a frame and counter-rotating, each firing a pellet
// outward every 4th frame once the formation is wide enough for the rank.
// Retires at radius 460.
extern "C" void near mima_18EB8(void)
{
	int i;
	unsigned char pellet_angle;
	unsigned char angle;

	if(boss_phase_frame < 40) {
		return;
	}
	if(boss_phase_frame == 40) {
		snd_se_play(9);
		patnum_2064E = 131;
		mima_orb_center_x = (*boss_left_on_back_page + 80);
		mima_orb_center_y = (*boss_top_on_back_page + 80);
		mima_orb_radius = 4;
		mima_orb_angle = randring2_next8();
	} else if(boss_phase_frame < 80) {
		return;
	} else if(boss_phase_frame == 80) {
		snd_se_play(10);
		patnum_2064E = 134;
		for(
			i = 0, angle = mima_orb_angle;
			i < MIMA_ORB_COUNT;
			i++, angle = (angle + MIMA_ORB_ANGLE_STEP)
		) {
			mima_orb_left_on_page[0][i] = mima_orb_left_on_page[1][i] = (
				(((long)(mima_orb_radius) * CosTable8[angle]) >> 8) +
				(mima_orb_center_x >> 4)
			);
			mima_orb_top_on_page[0][i] = mima_orb_top_on_page[1][i] = (
				(((long)(mima_orb_radius) * SinTable8[angle]) >> 8) +
				(mima_orb_center_y >> 4)
			);
			mima_orb_flag[i] = 1;
		}
	} else {
		mima_orb_radius += 2;
		for(
			i = 0, angle = mima_orb_angle;
			i < MIMA_ORB_COUNT;
			i++, angle = (angle + MIMA_ORB_ANGLE_STEP)
		) {
			mima_orb_left_on_page[page_back][i] = (
				(((long)(mima_orb_radius) * CosTable8[angle]) >> 8) +
				mima_orb_center_x
			);
			mima_orb_top_on_page[page_back][i] = (
				(((long)(mima_orb_radius) * SinTable8[angle]) >> 8) +
				mima_orb_center_y
			);
			// ZUN quirk: the sound effect plays before the four bounds tests,
			// so an orb that has left the screen is still audible.
			if(
				((180 - (rank << 4)) < mima_orb_radius) &&
				!(boss_phase_frame & 3)
			) {
				snd_se_play(3);
				if(
					(mima_orb_left_on_page[page_back][i] > 0) &&
					(mima_orb_left_on_page[page_back][i] < PLAYFIELD_RIGHT) &&
					(mima_orb_top_on_page[page_back][i] > 0) &&
					(mima_orb_top_on_page[page_back][i] < RES_Y)
				) {
					if(!mima_all_patterns) {
						pellet_angle = ((i << 6) + mima_orb_angle + 0x40);
					} else {
						pellet_angle = ((i << 6) + mima_orb_angle + 0x60);
					}
					bullets_add_pellet(
						mima_orb_left_on_page[page_back][i],
						mima_orb_top_on_page[page_back][i],
						pellet_angle,
						BG_1,
						(8 << 4)
					);
				}
			}
		}
		if(mima_orb_radius > 460) {
			for(i = 0; i < MIMA_ORB_COUNT; i++) {
				mima_orb_flag[i] = 0;
			}
			boss_phase_frame = 0;
			patnum_2064E = 128;
		}
	}
	mima_orb_angle = (mima_orb_angle - 3);
}


// The first of her three step-9 patterns: a symmetric spread every other frame
// between phase frames 50 and 150, with the angle walking by
// boss_rank_param[4] each time. The block-order shapes below are the same two
// mima_19353() needed - the restart FOLLOWS the window if-statement, and every
// arm ends in an explicit return statement.
static void near mima_19173(void)
{
	if(boss_phase_frame < MIMA_SPIRAL_FIRST_FRAME) {
		return;
	}
	if(boss_phase_frame == MIMA_SPIRAL_FIRST_FRAME) {
		mima_spiral_angle = 0;
		return;
	}
	if(boss_phase_frame < MIMA_SPIRAL_LAST_FRAME) {
		if((boss_phase_frame & 1) != 0) {
			snd_se_play(3);
			bullets_add_16x16(
				x_26C5C,
				y_26C64,
				mima_spiral_angle,
				BG_2_SPREAD_HORIZONTALLY_SYMMETRIC,
				PAT_BULLET16_BALL,
				((3 << 4) + 2)
			);
			mima_spiral_angle += boss_rank_param[4];
		}
		return;
	}
	boss_phase_frame = 0;
}


// The second: a charge-up that shrinks three concentric rings around her for 30
// frames, then sprays randomly aimed bullets and pellets for another 120.
static void near mima_191CC(void)
{
	if(boss_phase_frame < MIMA_CHARGE_FIRST_FRAME) {
		return;
	}
	if(boss_phase_frame == MIMA_CHARGE_FIRST_FRAME) {
		// 12 on both axes, which is neither the player's center
		// ((PLAYER_W / 2) is 16, (PLAYER_H / 2) is 24) nor any other named
		// offset in this game - so it stays a literal, the way
		// th02/main/player/shot.cpp keeps its own aiming offsets.
		mima_aim_angle_unused = iatan2(
			((player_topleft.y + 12) - y_26C64),
			((player_topleft.x + 12) - x_26C5C)
		);
		mima_charge_ring_radius = MIMA_CHARGE_RING_RADIUS_INITIAL;
		snd_se_play(9);
		return;
	}
	if(boss_phase_frame <= MIMA_CHARGE_LAST_FRAME) {
		if((boss_phase_frame & 1) == 0) {
			return;
		}
		// Unblit at the old radius, shrink, blit at the new one - except on
		// the last frame, which only unblits.
		grcg_setcolor(GC_RMW, 0);
		grcg_circle(x_26C5C, y_26C64, mima_charge_ring_radius);
		grcg_circle(x_26C5C, y_26C64, (mima_charge_ring_radius * 2));
		grcg_circle(x_26C5C, y_26C64, (mima_charge_ring_radius * 4));
		mima_charge_ring_radius -= 2;
		if(boss_phase_frame == MIMA_CHARGE_LAST_FRAME) {
			return;
		}
		grcg_setcolor(GC_RMW, 13);
		grcg_circle(x_26C5C, y_26C64, mima_charge_ring_radius);
		grcg_circle(x_26C5C, y_26C64, (mima_charge_ring_radius * 2));
		grcg_circle(x_26C5C, y_26C64, (mima_charge_ring_radius * 4));
		grcg_off();
		return;
	}
	if(boss_phase_frame < MIMA_SPRAY_LAST_FRAME) {
		if((boss_phase_frame & 7) == 0) {
			bullets_add_16x16(
				x_26C5C,
				y_26C64,
				2,
				BG_RANDOM_ANGLE_AND_SPEED,
				PAT_BULLET16_BALL,
				((3 << 4) + 12)
			);
			bullets_add_pellet(
				x_26C5C, y_26C64, 2, BG_RANDOM_ANGLE_AND_SPEED, ((3 << 4) + 12)
			);
		}
		// A second spray on the higher ranks, offset by half the interval.
		if((rank > 0) && ((boss_phase_frame & 7) == 4)) {
			bullets_add_16x16(
				x_26C5C,
				y_26C64,
				2,
				BG_RANDOM_ANGLE_AND_SPEED,
				PAT_BULLET16_BALL,
				((3 << 4) + 12)
			);
			bullets_add_pellet(
				x_26C5C, y_26C64, 2, BG_RANDOM_ANGLE_AND_SPEED, ((3 << 4) + 12)
			);
		}
		if((boss_phase_frame & 7) == 0) {
			snd_se_play(3);
		}
		return;
	}
	boss_phase_frame = 0;
}


// The third of her step-9 patterns: a single aimed spread every 16 frames, on
// a 32-frame cycle, alternating between the 5-bullet and the 4-bullet group.
// The pattern only fires between phase frames 50 and 190, and restarts the
// phase at 190.
static void near mima_19353(void)
{
	if(boss_phase_frame < MIMA_SPREAD_FIRST_FRAME) {
		return;
	}
	// `[measured]` The window is the OUTER if-statement, the restart is what
	// follows it, and each spread arm ends in an explicit return statement.
	// Both are load-bearing: written as an early return, the restart lands
	// ABOVE the spreads instead of between them and the epilogue; and without
	// the explicit return in the first arm, -O hosts the two calls' merged
	// tail at that arm and makes the second jump BACKWARD into it. Five other
	// spellings of the same control flow were screened; only this one gives
	// the original's block order. (kb/codegen/0152)
	if(boss_phase_frame < MIMA_SPREAD_LAST_FRAME) {
		if((boss_phase_frame & (MIMA_SPREAD_INTERVAL - 1)) == 0) {
			bullets_add_16x16(
				x_26C5C,
				y_26C64,
				0,
				BG_5_SPREAD_MEDIUM_AIMED,
				PAT_BULLET16_BALL,
				((5 << 4) + 10)
			);
			return;
		}
		if(
			(boss_phase_frame & (MIMA_SPREAD_INTERVAL - 1)) ==
			(MIMA_SPREAD_INTERVAL / 2)
		) {
			bullets_add_16x16(
				x_26C5C,
				y_26C64,
				0,
				BG_4_SPREAD_MEDIUM_AIMED,
				PAT_BULLET16_BALL,
				((5 << 4) + 10)
			);
		}
		return;
	}
	boss_phase_frame = 0;
}


// Ends the current pattern: advances the step if it is over, and otherwise
// picks the next pattern and re-rolls Mima's vertical drift.
static void near mima_193A4(void)
{
	register int pattern_next;

	if(boss_phase_frame == 0) {
		if(
			(boss_damage >= mima_phase_damage_max) ||
			(mima_patterns_this_phase > mima_patterns_max)
		) {
			mima_phase++;
			mima_damage_multiplier = 0;
			mima_patterns_this_phase = 0;
		} else {
			mima_patterns_this_phase++;
			if(mima_patterns_this_phase >= mima_patterns_until_vulnerable) {
				mima_damage_multiplier = 1;
			}
			// The `+ 1` is one expression in each arm, because the original
			// computes it in AX and moves it to SI - the opposite of
			// kb/codegen/0146's split-statement case.
			if(!mima_all_patterns) {
				pattern_next = (mima_pattern + 1);
				if(pattern_next >= mima_pattern_count) {
					pattern_next = 0;
				}
			} else {
				pattern_next = (mima_pattern + 1);
				if(pattern_next > 7) {
					pattern_next = 0;
				}
			}
			mima_pattern = pattern_next;
			if(*boss_top_on_back_page < MIMA_TOP_MIN) {
				mima_velocity_y = 1;
			} else if(*boss_top_on_back_page > MIMA_TOP_MAX) {
				mima_velocity_y = -1;
			} else {
				mima_velocity_y = (1 - (randring2_next8() % 3));
			}
		}
	}
	if(boss_phase_frame < MIMA_DRIFT_FRAMES) {
		*boss_top_on_back_page += mima_velocity_y;
	}
}


// Her per-frame update. Installed into [boss_update] by stage_init().
extern "C" int far mima_update(void)
{
	// [radius] first: Turbo C++ allocates stack locals in declaration order,
	// and it is the original's only one, at [bp-1]. (kb/codegen/0010)
	unsigned char radius;
	register int i;

	boss_phase_frame++;
	boss_pos_x = (*boss_left_on_back_page + 72);
	boss_pos_y = (*boss_top_on_back_page + 56);
	left_26C56 = (*boss_left_on_back_page + 32);
	mima_muzzle_left = (*boss_left_on_back_page + 40);
	left_26C5A = (*boss_left_on_back_page + 64);
	x_26C5C = (*boss_left_on_back_page + 64);
	top_26C5E = (*boss_top_on_back_page + 96);
	mima_muzzle_top = (*boss_top_on_back_page + 16);
	top_26C62 = (*boss_top_on_back_page + 114);
	y_26C64 = (*boss_top_on_back_page + 44);
	if((stage_frame & 1) == 0) {
		if(!reduce_effects) {
			mima_ring_radius += 8;
			grcg_setcolor(GC_RMW, 3);

			// [i] is initialized ahead of [radius], and the increment is a
			// statement of its own rather than a loop-header increment,
			// because that is what puts `inc si` ahead of the radius update
			// and lets -O merge both stores to [radius] into the shared test
			// block, as the original has them.
			i = 0;
			radius = mima_ring_radius;
			while(i < MIMA_RING_COUNT) {
				dot_square_ring_put(
					MIMA_RING_CENTER_X,
					MIMA_RING_CENTER_Y,
					radius,
					MIMA_RING_ANGLE_STEP
				);
				i++;
				radius += MIMA_RING_DISTANCE;
			}
			grcg_off();
		}
		if(mima_phase & 1) {
			mima_bg_circle_pulse_frame++;
			if(mima_bg_circle_pulse_frame >= MIMA_CIRCLE_PULSE_FRAMES) {
				mima_bg_circle_pulse_frame = 0;
			}
			mima_bg_circle_radius = (
				mima_bg_circle_radius_base - (mima_bg_circle_pulse_frame / 4)
			);
		}
	}

	if(mima_phase == 0) {
		mima_bg_ring_radius = (boss_phase_frame / 2);
		mima_bg_circle_radius = (boss_phase_frame / 3);
		Palettes[0].c.r = (boss_phase_frame >> 1);
		Palettes[0].c.g = 0;
		Palettes[0].c.b = (boss_phase_frame >> 1);
		palette_show();
		if(boss_phase_frame > MIMA_CHARGE_FRAMES) {
			mima_bg_circle_radius_base = mima_bg_circle_radius;
			mima_bg_circle_pulse_frame = 0;
			mima_phase = 1;
			boss_phase_frame = 0;
			mima_pattern = (randring2_next8() % 3);
			mima_patterns_this_phase = 0;
			mima_phase_damage_max = 600;
			mima_patterns_until_vulnerable = 2;
			mima_patterns_max = 10;
			mima_pattern_count = 3;
			boss_damage = 0;
		}
	} else if(mima_phase == 1) {
		switch(mima_pattern) {
		case 0:
			mima_180EC();
			break;
		case 1:
			mima_181B3();
			break;
		case 2:
			mima_188AA();
			break;
		}
		mima_193A4();
	} else if(mima_phase == 2) {
		mima_bg_ring_radius = ((boss_phase_frame / 4) + 50);
		mima_bg_circle_radius = ((boss_phase_frame / 5) + 30);
		mima_18BA6();
		Palettes[0].c.r = (51 - (boss_phase_frame >> 1));
		Palettes[0].c.g = (boss_phase_frame >> 1);
		Palettes[0].c.b = 50;
		palette_show();
		if(boss_phase_frame > MIMA_CHARGE_FRAMES) {
			mima_phase = 3;
			boss_phase_frame = 0;
			mima_pattern = (randring2_next8() % 3);
			mima_patterns_this_phase = 0;
			mima_bg_circle_radius_base = mima_bg_circle_radius;
			mima_phase_damage_max = 700;
			mima_patterns_until_vulnerable = 2;
			mima_patterns_max = 12;
			mima_pattern_count = 3;
			boss_damage = 0;
		}
	} else if(mima_phase == 3) {
		switch(mima_pattern) {
		case 0:
			mima_18905();
			break;
		case 1:
			mima_18A1B();
			break;
		case 2:
			mima_18B4B();
			break;
		}
		mima_193A4();
	} else if(mima_phase == 4) {
		mima_bg_ring_radius = ((boss_phase_frame / 4) + 75);
		mima_bg_circle_radius = ((boss_phase_frame / 5) + 50);
		mima_18BA6();
		Palettes[0].c.r = (boss_phase_frame >> 1);
		Palettes[0].c.g = (51 - (boss_phase_frame >> 1));
		Palettes[0].c.b = (51 - (boss_phase_frame >> 1));
		palette_show();
		if(boss_phase_frame > MIMA_CHARGE_FRAMES) {
			mima_phase = 5;
			boss_phase_frame = 0;
			mima_pattern = (randring2_next8() % 3);
			mima_bg_circle_radius_base = mima_bg_circle_radius;
			mima_patterns_this_phase = 0;
			mima_phase_damage_max = 800;
			mima_patterns_until_vulnerable = 2;
			mima_patterns_max = 12;
			mima_pattern_count = 3;
			boss_damage = 0;
		}
	} else if(mima_phase == 5) {
		switch(mima_pattern) {
		case 0:
			mima_18C4A();
			break;
		case 1:
			mima_18DE0();
			break;
		case 2:
			mima_18EB8();
			break;
		}
		mima_193A4();
	} else if(mima_phase == 6) {
		mima_bg_ring_radius = ((boss_phase_frame / 4) + 100);
		mima_bg_circle_radius = ((boss_phase_frame / 5) + 70);
		mima_18BA6();
		Palettes[0].c.r = (51 - (boss_phase_frame >> 1));
		Palettes[0].c.g = 0;
		Palettes[0].c.b = 0;
		palette_show();
		if(boss_phase_frame > MIMA_CHARGE_FRAMES) {
			mima_bg_circle_radius_base = mima_bg_circle_radius;
			mima_phase = 7;
			boss_phase_frame = 0;
			mima_pattern = randring2_next8_and(7);
			mima_patterns_this_phase = 0;
			mima_phase_damage_max = 1500;
			mima_patterns_until_vulnerable = 3;
			mima_patterns_max = 200;
			mima_pattern_count = 9;
			mima_all_patterns = 1;
			boss_damage = 0;
			mima_bg_ring_col_tail = 2;
		}
	} else if(mima_phase == 7) {
		switch(mima_pattern) {
		case 0:
			mima_18C4A();
			break;
		case 1:
			mima_188AA();
			break;
		case 2:
			mima_18905();
			break;
		case 3:
			mima_181B3();
			break;
		case 4:
			mima_18A1B();
			break;
		case 5:
			mima_18B4B();
			break;
		case 6:
			mima_180EC();
			break;
		case 7:
			mima_18DE0();
			break;
		case 8:
			mima_18EB8();
			break;
		}
		mima_193A4();
	} else if(mima_phase == 9) {
		switch(mima_pattern) {
		case 0:
			mima_19173();
			break;
		case 1:
			mima_191CC();
			break;
		case 2:
			mima_19353();
			break;
		}
		mima_193A4();
		if(boss_phase_frame == 1) {
			*boss_top_on_back_page = 64;
		}
		if(boss_phase_frame < 30) {
			*boss_left_on_back_page += (
				(x_26C5C < player_topleft.x) ? 1 : -1
			);
		}
	}

	mima_17C92();
	mima_17F27();
	mima_17D59();
	if(mima_phase == 8) {
		if(!mima_19C8D()) {
			mima_phase++;
			patnum_2064E = 128;
			mima_patterns_this_phase = 0;
			boss_phase_frame = 0;
			mima_pattern = (randring2_next8() % 3);
			mima_patterns_this_phase = 0;
			boss_damage = 0;
			mima_phase_damage_max = 1100;
			mima_patterns_until_vulnerable = 2;
			mima_patterns_max = 200;
			mima_pattern_count = 3;
			mima_bg_circle_col = 1;
			mima_bg_ring_col_tail = 2;
			mima_bg_ring_radius = 200;
			mima_bg_circle_radius = 150;
			mima_bg_circle_radius_base = mima_bg_circle_radius;
		}
	} else if(mima_phase == 10) {
		nopcall_same_group(mima_end);
	}
	// Through an `int` rather than [stage_progression_t]: -b- sizes a
	// three-value enum as a `char`, and the original returns in AX.
	// th02/main/boss/b4.cpp declares marisa_update() the same way.
	return SP_BOSS;
}
