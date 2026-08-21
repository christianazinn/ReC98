/// Vertical boss lasers
/// --------------------
/// A laser is anchored at a point inside the playfield, plays a 32×32 charge
/// animation there, then extends a 16-pixel-wide beam straight down to the
/// bottom of the playfield. It only damages the player during a single phase
/// of that animation. Four bosses and midbosses spawn them, over eight procs
/// and 27 call sites (the census is under [laser_wait_frames] below); the two
/// per-frame functions are installed as stage callbacks by
/// lasers_callbacks_set(), which stage_init() calls for Stage 4 and Extra.

#ifndef TH02_MAIN_LASER_HPP
#define TH02_MAIN_LASER_HPP

#include "pc98.h"
#include "th02/main/entity.hpp"

#define LASER_COUNT 12

// Growth phase, which doubles as a sprite cel index. Only the landmarks are
// named; the values in between are pure animation frames.
// 	1        wait out [wait_frames]
// 	2 …  3   grow, one step every 8 frames
// 	4        the only phase that hits the player, for [active_frames] frames
// 	5 …  8   shrink, one step every 4 frames
// 	9        done
#define LASER_PHASE_WAIT 1
#define LASER_PHASE_ACTIVE 4
#define LASER_PHASE_SHRINK 5
#define LASER_PHASE_DONE 9

struct laser_t {
	entity_flag_t flag;
	uint8_t phase;

	// Top of the beam, in unscrolled screen space. [scroll_line] is added and
	// wrapped before blitting.
	screen_point_t origin;

	// Frames to spend in LASER_PHASE_WAIT before growing. Seeded at spawn time
	// from [laser_wait_frames], which the spawning boss usually overwrites
	// first — see that variable below for the measured census, and for the
	// placeholder name its ASM writers still use.
	int wait_frames;

	// Frames to spend in LASER_PHASE_ACTIVE.
	int active_frames;

	// Cel of the 32×32 charge animation, advanced by lasers_invalidate() rather
	// than by the update function. Only 0 … (LASER_CHARGE_CELS - 1) are actual
	// cels — th02/sprites/main_pat.h:42 caps the sprite range there. The counter
	// runs past that on both sides of the handover: the update function draws
	// the beam once it merely *reaches* LASER_CHARGE_CELS, while
	// lasers_invalidate() keeps unblitting the charge box until it passes it.
	// See th02/main/laser.cpp for what that window costs.
	uint8_t charge_cel;

	// Base pattern number of the 16×16 beam strip. The rendered pattern is
	// this plus [phase]. ACTUAL TYPE: main_patnum_t
	uint8_t patnum_base;
};

extern laser_t lasers[LASER_COUNT];

// The [wait_frames] every newly spawned laser starts with. A single-field
// spawn-time template, in the family of TH04's [thicklaser_template] and TH05's
// [laser_template].
//
// Every write is still ASM, and still spells the variable with its IDA
// placeholder name: in th02_main.asm's `_BSS`, the three-line publish alias
// `public _laser_wait_frames` / `_laser_wait_frames label byte` sits directly
// above the `byte_23A70 db ?` that reserves the storage, and all twelve writes
// spell it `byte_23A70`. Census, re-measured at ReC98 a99dbfc2 — four
// laser-spawning entities, spread over eight procs that call lasers_add():
//
// 	stones     	`stones_11B5D` 24h, `stones_11C37` 1Eh, `stones_11D30` 10h,
// 	           	`stones_11E76` 1Eh twice, plus `stones_12778` 0Ch, which
// 	           	writes the value without spawning anything
// 	rika       	`rika_13C91` 40h
// 	sigma      	`sigma_1619C` 20h, 30h, 64h, 10h; `sigma_162D3` writes nothing
// 	           	at all and spawns four lasers on whatever was left behind
// 	midboss4   	`midboss4_1A17E` 10h
//
// So: eight distinct values across twelve writes, and only three of them are
// the 16 that lasers_reset() restores.
extern uint8_t laser_wait_frames;

// Frees every slot and restores [laser_wait_frames]. Called once per stage from
// stage_init(), regardless of whether that stage has any lasers at all.
void far lasers_reset(void);

// Installs the two per-frame functions below into their stage callback slots.
// stage_init() calls this for Stage 4 and Extra, and two of the bosses call it
// again from their own init function.
//
// C linkage, for the same reason bullets_clear() has it: both of those boss
// calls land in the caller's own physical segment, so the original reaches
// this function through the linker-relaxed `nop; push cs; call near ptr` form
// that no plain C++ far call reproduces (kb/codegen/0083) - and Turbo C++'s
// inline assembler resolves the identifier in `call near ptr` as a *C* symbol
// and rejects the `$` in a mangled one. stones_init() in
// th02/main/boss/b3.cpp is the lifted one; rika_init(), still in
// th02_main.asm, spells its `nopcall` the C way for the same reason.
extern "C" void far lasers_callbacks_set(void);

// Spawns a laser at ([left], [top]) in the first free slot, with the sound
// effect that every spawn plays. Does nothing if [left] is outside the
// playfield, or if all LASER_COUNT slots are taken.
//
// [patnum_base] is `int` even though the slot it is stored into is a
// `uint8_t`, and the evidence is the ARGUMENT PACKER rather than the body.
// `[measured]` With -3, Turbo C++ folds each adjacent pair of 16-bit `pascal`
// arguments into one `66 68 imm32` push - but only when BOTH formals of the
// pair are `int`. Five probe shapes (`int`/`unsigned char` literal, an
// `(uint8_t)` cast, a `char` literal, a 255, a 300) confirm that an
// `unsigned char` formal never packs with its neighbour. stones_11B5D() in
// th02/main/boss/b3.cpp calls this with four literals and the original packs
// BOTH pairs, which `uint8_t patnum_base` cannot produce. Widening it is
// codegen-neutral for the body below - its `tcc -S` listing is line-identical
// either way, because a `pascal` argument occupies a whole word on the stack
// and the store reads only AL.
void pascal near lasers_add(
	screen_x_t left,
	screen_y_t top,
	int active_frames,
	int patnum_base
);

void far lasers_invalidate(void);
void far lasers_update_and_render(void);

#endif /* TH02_MAIN_LASER_HPP */
