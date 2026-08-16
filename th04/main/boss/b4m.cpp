/// Stage 4 Boss - Marisa
/// ---------------------

#pragma option -zCB4M_UPDATE_TEXT -zPmain_03

#include "th04/sprites/main_pat.h"
#include "th04/main/custom.hpp"
#include "th04/main/boss/boss.hpp"
#include "th04/math/randring.hpp"

// Constants
// ---------

static const pixel_t BIT_W = 32;
static const pixel_t BIT_H = 32;
static const int BIT_KILL_FRAMES_PER_CEL = 4;
// ---------

// Structures
// ----------

#define BIT_COUNT 4

enum bit_flag_t {
	BF_FREE = 0,
	BF_MOVEOUT_SPIN = 1,
	BF_SPIN = 2,
	BF_KILL_ANIM = 0x80,
	BF_KILL_ANIM_last = (
		BF_KILL_ANIM + (ENEMY_KILL_CELS * BIT_KILL_FRAMES_PER_CEL) - 1
	),

	_bit_flag_t_FORCE_UINT8 = 0xFF
};

struct bit_t {
	bit_flag_t flag;
	unsigned char angle;	// input for [center]
	PlayfieldPoint center;
	main_patnum_t patnum;
	/* ------------------------- */ int8_t unused_1[8];
	Subpixel distance;	// input for [center]
	Subpixel moveout_speed;
	int hp;
	int damage_this_frame;
	/* ------------------------- */ int8_t unused_2;
	char angle_speed;	// ACTUAL TYPE: unsigned char
};

#define bits (reinterpret_cast<bit_t *>(custom_entities))
// ----------

// State
// -----

#define flystep_pointreflected_tick boss_statebyte[13]

extern uint8_t bits_alive;

extern void (near pascal *near bit_fire)(bit_t near& bit);
extern screen_x_t bit_center_x[BIT_COUNT];
extern screen_x_t bit_center_y[BIT_COUNT];
// -----

// Game logic
// ----------

// Marisa's other flight step: a bounded random wander. Every 32nd frame — on
// the frame *after* each multiple of 32, since the test is against 1 rather
// than 0 — this picks a new velocity for each axis independently. An axis
// whose coordinate has left its box is turned back towards the middle at a
// fixed speed; otherwise the new direction is random. Every call, including
// the 31 that pick nothing, then applies the current velocity to [boss].
//
// The two axes are not symmetric, and the asymmetry is ZUN's: X gets a
// straight coin flip between two speeds, while Y draws from four, adding the
// two 1.5-pixel variants that have no X counterpart.
void near marisa_flystep_random(void)
{
	enum {
		INTERVAL_MASK = 0x1F,

		LEFT   = TO_SP(112),
		RIGHT  = TO_SP(272),
		TOP    = TO_SP(80),
		BOTTOM = TO_SP(144),

		SPEED_TURN_X = TO_SP(2),
		SPEED_TURN_Y = TO_SP(1),
		SPEED_RANDOM = TO_SP(1),

		// 1.5 pixels. Not expressible as TO_SP() of an integer, which is why
		// the original stores it as the raw subpixel literal 24.
		SPEED_RANDOM_FAST = (TO_SP(3) / 2),
	};
	if((boss.phase_frame & INTERVAL_MASK) == 1) {
		if(boss.pos.cur.x <= LEFT) {
			boss.pos.velocity.x.v = SPEED_TURN_X;
		} else if(boss.pos.cur.x >= RIGHT) {
			boss.pos.velocity.x.v = -SPEED_TURN_X;
		} else {
			boss.pos.velocity.x.v = (
				randring2_next16_and(1) ? SPEED_RANDOM : -SPEED_RANDOM
			);
		}

		if(boss.pos.cur.y <= TOP) {
			boss.pos.velocity.y.v = SPEED_TURN_Y;
		} else if(boss.pos.cur.y >= BOTTOM) {
			boss.pos.velocity.y.v = -SPEED_TURN_Y;
		} else {
			unsigned char direction = randring2_next16_and(3);
			boss.pos.velocity.y.v = (
				(direction == 0) ?  SPEED_RANDOM :
				(direction == 1) ? -SPEED_RANDOM :
				(direction == 2) ?  SPEED_RANDOM_FAST :
				                   -SPEED_RANDOM_FAST
			);
		}
	}
	boss.pos.update_seg3();
}

// On [flystep_pointreflected_tick] 0, this function sets up [boss] movement
// towards the point reflection of Marisa's position across a fixed position
// near the top of the sealed moon in the background. The velocity is
// calculated to reach this exact point at [duration - 12], with Marisa braking
// on the last 12 frames by halving that velocity each frame. Every call to
// this function, including the one on frame 0, then applies this velocity to
// [boss].
// [duration] values <12 will move Marisa into the opposite direction instead.
// Returns `true` if the function was called for [duration] frames.
//
// ZUN bug: Not defined for [duration] values of 12 or 13, which will crash the
// game with a division by zero ("Divide Error"). The two patterns that pass a
// variable [duration] to this function also only happen to call this function
// every 4 frames rather than every frame, introducing additional jerkiness.
bool pascal near marisa_flystep_pointreflected(int duration)
{
	enum {
		POINT_X = TO_SP(PLAYFIELD_W / 2),
		POINT_Y = TO_SP((PLAYFIELD_H * 7) / 23),
		BRAKE_DURATION = 12,
	};
	if(flystep_pointreflected_tick == 0) {
		boss.pos.velocity.x.v = (
			(POINT_X - boss.pos.cur.x) / ((duration / 2) - (BRAKE_DURATION / 2))
		);
		boss.pos.velocity.y.v = (
			(POINT_Y - boss.pos.cur.y) / ((duration / 2) - (BRAKE_DURATION / 2))
		);
	}
	flystep_pointreflected_tick++;
	if(flystep_pointreflected_tick >= (duration - BRAKE_DURATION)) {
		boss.pos.velocity.x.v /= 2;
		boss.pos.velocity.y.v /= 2;
	}
	if(flystep_pointreflected_tick >= duration) {
		return true;
	}
	boss.pos.update_seg3();
	return false;
}
// ----------
