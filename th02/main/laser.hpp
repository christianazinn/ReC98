/// Vertical boss lasers
/// --------------------
/// A laser is anchored at a point inside the playfield, plays a 32×32 charge
/// animation there, then extends a 16-pixel-wide beam straight down to the
/// bottom of the playfield. It only damages the player during a single phase
/// of that animation. Five different bosses and midbosses spawn them; the two
/// per-frame functions are installed as stage callbacks by lasers_enable().

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

	// Frames to spend in LASER_PHASE_WAIT before growing. Seeded from
	// [laser_wait_frames] at spawn time.
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

// Origin of the laser currently being rendered. Scratch, only used to pass the
// position from lasers_update_and_render() to laser_render().
// ZUN bloat: laser_render() already receives the laser itself.
extern screen_point_t laser_origin;

#endif /* TH02_MAIN_LASER_HPP */
