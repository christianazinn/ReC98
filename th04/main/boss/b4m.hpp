#ifndef TH04_MAIN_BOSS_B4M_HPP
#define TH04_MAIN_BOSS_B4M_HPP

#include "th04/main/custom.hpp"
#include "th04/sprites/main_pat.h"

/// Stage 4 Boss - Marisa
/// ---------------------
/// Her bits, which are the Stage 4 boss portion's second use of the
/// [custom_entities] block (Reimu's orbs are th04/main/boss/b4r.hpp). Two
/// translation units need them now, because ZUN's code for this fight is split
/// across two code segments and therefore two objects:
///
/// • th04/main/boss/b4m.cpp (B4M_UPDATE_TEXT) owns the per-bit helpers that
///   the fight shares with Yuuka's, and
/// • th04/main/boss/b4m_upd.cpp (ENM_BTPL_TEXT) owns marisa_update() and every
///   one of its patterns.
///
/// Neither segment's object may include an unguarded header the other files in
/// it expand as well (kb/codegen/0129), which is the second reason this is a
/// header rather than a repeated block: it is guarded, and everything it needs
/// is guarded too.

// Constants
// ---------

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

// What marisa_charge_animate() tells the pattern it opens. Deliberately a
// byte-sized enum: the original returns these in AL alone, and every caller
// homes the result in a byte local before testing it.
enum marisa_charge_t {
	// Still charging. The pattern must not fire yet.
	MC_CHARGING = 0,

	// The charge is over and the pattern has already fired; keep running it.
	MC_RUNNING = 1,

	// This one frame is the frame the pattern fires on.
	MC_FIRE = 2,

	_marisa_charge_t_FORCE_UINT8 = 0xFF
};
// ----------

// State
// -----

extern uint8_t bits_alive;

extern void (near pascal *near bit_fire)(bit_t near& bit);
void pascal near marisa_bit_fire_16F24(bit_t near& bit);
void pascal near marisa_bit_fire_17061(bit_t near& bit);
extern screen_x_t bit_center_x[BIT_COUNT];
extern screen_x_t bit_center_y[BIT_COUNT];
// -----

#endif /* TH04_MAIN_BOSS_B4M_HPP */
