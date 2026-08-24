#ifndef TH04_MAIN_BOSS_B4R_HPP
#define TH04_MAIN_BOSS_B4R_HPP

#include "th04/main/custom.hpp"

/// Stage 4 Boss - Reimu
/// --------------------
/// Her orbs, which are the Stage 4 boss portion's only use of the
/// [custom_entities] block. Spelled here the way th04_main.asm spells them,
/// because two translation units need them now: th04/main/boss/b4r.cpp, which
/// owns her update code and abbreviates them back to `orb_*`, and
/// th04/main/boss/fg.cpp, which renders them.

// Constants
// ---------

static const pixel_t REIMU_ORB_W = 32;
static const pixel_t REIMU_ORB_H = 32;
// ---------

// Structures
// ----------

#define REIMU_ORB_COUNT CUSTOM_COUNT

enum reimu_orb_flag_t {
	OF_FREE = 0,

	// Moves out the orb from [origin] with a fixed speed (ignoring [velocity])
	// to a fixed distance away from Reimu, then transitions to OF_MOVE once
	// [spin_time] hits 0.
	OF_MOVEOUT_SPIN = 1,

	// Adds [velocity] to [center].
	OF_MOVE = 2,
};

struct reimu_orb_t {
	reimu_orb_flag_t flag;
	unsigned char angle;
	PlayfieldPoint center;
	PlayfieldPoint origin;
	PlayfieldPoint velocity;
	unsigned int spin_time;
	Subpixel distance;
	/* ------------------------- */ int16_t unknown;
	/* ------------------------- */ int8_t unused[4];
	SubpixelLength8 move_speed;
	char angle_speed;	// ACTUAL TYPE: unsigned char
};

#define reimu_orbs (reinterpret_cast<reimu_orb_t *>(custom_entities))
// ----------

// State
// -----

extern uint8_t orb_patnum_base; // ACTUAL TYPE: main_patnum_t

// Defined by th04/main/boss/b4r.cpp and read by th04/main/boss/b4r_upd.cpp,
// which is the other half of the same fight in a different segment.
extern unsigned char reimu_pattern8_angle;
extern int8_t reimu_bg_pulse_direction;
extern reimu_orb_t orb_template;
// -----

#endif /* TH04_MAIN_BOSS_B4R_HPP */
