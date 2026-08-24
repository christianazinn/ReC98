/* ReC98
 * -----
 * TH05 Staff Roll
 */

#pragma option -zCSCORE_TEXT

#include <stddef.h>
#include "planar.h"
#include "libs/master.lib/pc98_gfx.hpp"

// The scene's entities, its constants and space_window_set(). They are in a
// header because th05/space.cpp needs all of them too: ZUN put the scene's
// animation code in MAINE_01__TEXT, which this object does not contribute to.
#include "th05/staff.hpp"

// The definitions behind th05/staff.hpp's declarations. Their order is what
// puts them at their original _BSS addresses; nothing may be added between
// them.
orb_particle_t particles[ORB_PARTICLE_COUNT + 1];
SPPoint orb_trails_center[ORB_TRAIL_COUNT];
SPPoint stars_center[STAR_COUNT];

dots8_t __seg *verdict_bitmap;
// -----

// The tail of th05_maine.asm's SCORE_TEXT block, lifted backwards into the
// head of this object's contribution to the same segment. These have to
// precede space_window_set(), and keep their original order relative to each
// other, to land at their original addresses; see the files themselves for
// why this object is the one that can take them.
//
// verdict_animate() is ONE body shared with TH04, which reaches it through
// th04/staff.cpp instead; only its tail differs, under `#if (GAME == 5)`.
#include "th05/end/verdict_comment.cpp"
#include "th04/end/verdict_animate.cpp"
#include "th05/end/verdict_scores.cpp"

void pascal near space_window_set(
	screen_x_t center_x, screen_y_t center_y, pixel_t w, pixel_t h
)
{
	space_window_center.x = center_x;
	space_window_center.y = center_y;
	space_window_w = w;
	space_window_h = h;
	grc_setclip(
		(space_window_center.x - (space_window_w / 2)) - 8,
		(space_window_center.y - (space_window_h / 2)) - 8,
		(space_window_center.x + (space_window_w / 2)) + 7,
		(space_window_center.y + (space_window_h / 2)) + 7
	);
}
