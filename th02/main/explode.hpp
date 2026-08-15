/// Boss defeat explosion
/// ---------------------
/// The two-stage effect that every TH02 boss and midboss plays back on defeat.
/// Its two stages are keyed to a caller-supplied frame number rather than to
/// any state of its own, and each defeat animation calls the renderer up to
/// three times per frame with the same center point and three different frame
/// numbers, spaced 24 frames apart. That is also why the frame number is
/// signed: the later two calls pass negative numbers for the first 24 and 48
/// frames of the animation, and the renderer immediately returns for those.

#ifndef TH02_MAIN_EXPLODE_HPP
#define TH02_MAIN_EXPLODE_HPP

#include "pc98.h"

/// Frame numbers
/// -------------
/// Stage 1 is a ring of dot squares that shrinks towards the center; stage 2
/// is a ring of 16×16 sprites that expands away from it, together with a
/// palette flash, a horizontal screen shake, and a slowdown.

// Frame the stage 1 ring stops shrinking at.
static const int EXPLODE_STAGE_1_FRAMES = 30;

// Frame number that stage 2 starts at, and which is subtracted from the frame
// number for all of stage 2's own thresholds below.
static const int EXPLODE_STAGE_2_BASE = 0x20;

// Frame the stage 2 ring stops expanding at.
static const int EXPLODE_STAGE_2_FRAMES = 26;
/// -------------

// Offset added to the angle of every stage 2 point's Y coordinate, but not to
// the one of its X coordinate, turning the ring's circle into a slanted
// ellipse. Written by the defeat animations, which use it to spin each ring
// as it expands. (TH04 has the same field, as explosion_t::angle_offset.)
extern unsigned char boss_explode_angle_offset;

// Clears the two 16-pixel-wide columns immediately outside the playfield, on
// both VRAM pages. Both rings are clipped to a slightly wider box than the
// playfield, so their sprites and dot squares can be blitted into these two
// columns – where tiles_egc_render() would never unblit them again, since it
// only ever redraws tiles *inside* the playfield.
void near boss_explode_margins_clear(void);

// Renders the single frame [frame] of an explosion centered on the given
// point, and advances the effects that come with it. Does nothing for
// negative [frame] numbers, or once the stage the number falls into is over.
void pascal near boss_explode_render(
	screen_x_t center_x, screen_y_t center_y, int frame
);

#endif /* TH02_MAIN_EXPLODE_HPP */
