/// Vertical boss lasers
/// --------------------
/// A laser is anchored at a point inside the playfield, plays a 32×32 charge
/// animation there, then extends a 16-pixel-wide beam straight down to the
/// bottom of the playfield. It only damages the player during a single phase
/// of that animation. Five different bosses and midbosses spawn them; the two
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
	// from [laser_wait_frames].
	int wait_frames;

	// Frames to spend in LASER_PHASE_ACTIVE.
	int active_frames;

	// Cel of the 32×32 charge animation, 0 to 5. Advanced by
	// lasers_invalidate(), not by the update function. Once it reaches 4, the
	// beam proper takes over.
	uint8_t charge_cel;

	// Base pattern number of the 16×16 beam strip. The rendered pattern is
	// this plus [phase]. ACTUAL TYPE: main_patnum_t
	uint8_t patnum_base;
};

extern laser_t lasers[LASER_COUNT];

// The [wait_frames] every newly spawned laser starts with. A single-field
// spawn-time template, in the family of TH04's [thicklaser_template] and TH05's
// [laser_template]: each of the five bosses that spawn lasers writes it
// immediately before its burst of lasers_add() calls, and lasers_reset()
// restores the default.
extern uint8_t laser_wait_frames;

// Frees every slot and restores [laser_wait_frames]. Called once per stage from
// stage_init(), regardless of whether that stage has any lasers at all.
void far lasers_reset(void);

// Installs the two per-frame functions below into their stage callback slots.
// stage_init() calls this for Stage 4 and Extra, and two of the bosses call it
// again from their own init function.
void far lasers_callbacks_set(void);

// Spawns a laser at ([left], [top]) in the first free slot, with the sound
// effect that every spawn plays. Does nothing if [left] is outside the
// playfield, or if all LASER_COUNT slots are taken.
void pascal near lasers_add(
	screen_x_t left,
	screen_y_t top,
	int active_frames,
	uint8_t patnum_base
);

void far lasers_invalidate(void);
void far lasers_update_and_render(void);

#endif /* TH02_MAIN_LASER_HPP */
