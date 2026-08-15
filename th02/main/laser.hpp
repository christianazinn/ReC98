/// Vertical boss lasers
/// --------------------
/// A laser is anchored at a point inside the playfield, plays a 32×32 charge
/// animation there, then extends a 16-pixel-wide beam straight down to the
/// bottom of the playfield. It only damages the player during a single phase
/// of that animation. Five different bosses and midbosses spawn them; the two
/// per-frame functions are installed as stage callbacks by the still-ASM
/// `sub_129FC` (th02_main.asm), which stage_init() calls for Stage 4 and Extra.

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
	// from the still-unnamed [byte_23A70], which `sub_129DD` defaults to 16 and
	// every boss that spawns lasers overwrites beforehand. Its only reader is
	// the still-ASM spawner `sub_12A19`, so naming it waits for that lift.
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

#endif /* TH02_MAIN_LASER_HPP */
