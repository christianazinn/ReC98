#ifndef TH04_MAIN_PLAYER_SHOT_REIMU_HPP
#define TH04_MAIN_PLAYER_SHOT_REIMU_HPP

// Guarded: th04/player_b.cpp compiles all three shot_reimu*.cpp bodies into one
// object, and this file is the closure they share (kb/codegen/0129).
#include "libs/master.lib/master.hpp"
#include "th04/main/homing.hpp"
#include "th04/main/player/shot.hpp"
#include "th04/sprites/main_pat.h"

extern "C" {

// Counts the rounds of shots already fired within the current shot cycle: 0 on
// the round at [shot_time] == SHOT_CYCLE_FRAMES, which is where it is reset,
// then 1 and 2 on the two further rounds at the cycle's ⅓ and ⅔ points.
// Reimu's patterns divide it to fire their option shots less often than every
// round -- `% 3` for once per cycle, `% 2` for twice.
//
// [inferred] name, [measured] role and population: the sixteen
// shot_reimu_{a,b}_l{2..9} procs are the only code in either dump that touches
// this byte, Marisa's sixteen use lasers instead, and the spelling follows
// TH02's [shot_c_cycle] (th02/main/player/shot.cpp), the identical construct --
// a uint8_t bumped once per shot-control call and read `% N` to gate the option
// half. Evidence and the full population:
// state/notes/th04-shot-cycle-counter.md.
//
// ZUN quirk: shot_reimu_a_l8() increments this byte and neither resets nor
// reads it, so its rounds all fire the same shots and it only shifts the phase
// the next pattern sees.
extern uint8_t shot_reimu_cycle;

// Points [shot]'s velocity at [homing_target], [angle_offset] units off the
// exact aim, and gives it the usual 12-pixel length. Shottype A's option shots
// are the only callers, and they only call it while a target exists.
//
// ZUN quirk, and it is in the caller's favour: the vertical difference is
// measured from the PLAYER's position while the horizontal one is measured
// from the SHOT's, so the two option shots of a volley aim at slightly
// different angles even before [angle_offset] is added.
void pascal near shot_velocity_set_homing(
	Shot near *shot, unsigned char angle_offset
);

}

#endif
