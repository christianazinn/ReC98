/// Boss background particles, and the dot square they are blitted as
/// -----------------------------------------------------------------

#ifndef TH02_MAIN_BG_PARTICLE_HPP
#define TH02_MAIN_BG_PARTICLE_HPP

#include "planar.h"
#include "th01/math/subpixel.hpp"
#include "th02/main/entity.hpp"

/// Dot square
/// ----------
/// The primitive that TH02's boss background effects blit their individual
/// points with: a filled square of [edge]×[edge] dots, at the single position
/// in [dot_square_left] and [dot_square_top]. Four subsystems set that
/// position and immediately call grcg_dot_square_put(); only the particles
/// below also unblit their squares again.

// Largest [edge] that DOT_SQUARE_ROWS is defined for.
static const int DOT_SQUARE_EDGE_MAX = 5;

// One row of a filled square of dots, indexed by that square's edge length.
extern const dots8_t DOT_SQUARE_ROWS[DOT_SQUARE_EDGE_MAX + 1];

// Top-left corner of the square blitted by the two functions below. Set by
// the caller immediately before each call.
extern screen_x_t dot_square_left;
extern vram_y_t dot_square_top;

// Blits a filled square of [edge]×[edge] dots. Assumptions:
// • The GRCG is active, and set to the intended color
// • [edge] ≤ DOT_SQUARE_EDGE_MAX
void pascal far grcg_dot_square_put(int edge);

// Clears the 16×[edge] rectangle that encloses such a square, i.e. rounds its
// width up to the two VRAM bytes it can span. Assumptions:
// • The GRCG is active, and set to the color to clear with
// • ES is set to SEG_PLANE_B
// • [edge] ≤ DOT_SQUARE_EDGE_MAX
void pascal near grcg_dot_square_unput(int edge);

/// Rings of dot squares
/// --------------------
/// Both walk a full 256-step circle around ([center_x], [center_y]) in
/// [angle_step] steps, and derive the edge length of each square from the
/// radius, as ((radius / 64) + 1). Their clipping box is one pixel wider on
/// the left and 8 pixels shorter at the top than the playfield.

// Blits one such ring. Assumes the GRCG to be active and set to the intended
// color, and converts each point to VRAM space before blitting it.
void pascal far dot_square_ring_put(
	screen_x_t center_x, screen_y_t center_y, int radius, int angle_step
);

// Marks the tiles covered by one such ring for redrawing. Note that this does
// *not* mirror dot_square_ring_put()'s conversion to VRAM space, because
// tiles_invalidate_rect() expects unscrolled screen coordinates.
void pascal far dot_square_ring_invalidate(
	screen_x_t center_x, screen_y_t center_y, int radius, int angle_step
);
/// --------------------
/// ----------

struct bg_particle_t {
	entity_flag_t flag;
	unsigned char angle;

	// Stores the current and previous position, indexed with the currently
	// rendered VRAM page.
	SPPoint screen_topleft[PAGE_COUNT];

	// Edge length of this particle's dot square, in pixels. Recalculated
	// from [speed] on every frame.
	// The tree has no attested word for a square's side length — every other
	// `edge` in it means "boundary of a region" — so this one is a coinage,
	// kept for consistency with DOT_SQUARE_EDGE_MAX and
	// [bg_particle_edge_step] above rather than because it has precedent.
	uint8_t edge;

	SubpixelLength8 speed;
};

static const int BG_PARTICLE_COUNT = 30;

// Size of the box a particle is clipped against. Larger than the largest
// square that can actually be blitted for one — exactly as SPARK_W / SPARK_H
// (th02/main/sparks.inc) are 8×8 for a spark that may be rendered as a single
// dot, which is why these keep the plain `_W` / `_H` spelling anyway.
static const pixel_t BG_PARTICLE_W = 8;
static const pixel_t BG_PARTICLE_H = 8;

extern bg_particle_t bg_particles[BG_PARTICLE_COUNT];

/// Defaults, initialized in bg_particles_reset() and overridden per boss
/// ----------------------------------------------------------------------

// [speed] each particle starts out with. Only the low byte is used.
extern int bg_particle_speed_initial;

// Added to each particle's [speed] and [angle] on every frame. Only the low
// bytes are used.
extern int bg_particle_speed_delta;
extern int bg_particle_angle_delta;

// Distance between two adjacent [edge] steps, in subpixels of [speed]. A
// particle's square grows to DOT_SQUARE_EDGE_MAX at (this × 5).
extern int bg_particle_edge_step;

extern vc_t bg_particle_col;
extern vc_t bg_particle_unput_col;
/// ----------------------------------------------------------------------

// Frees all particles and restores the defaults above.
void far bg_particles_reset(void);

// Spawns a single particle at the given screen position, moving at [angle].
// Plural like sparks_add() and items_add(), which — like this function — walk
// their array for a free slot themselves. That is what the plural marks here.
//
// It is NOT a tree-wide rule, and the earlier version of this comment claimed
// one: "every *_add() in the tree is named after the array it writes to, never
// after the one entity it adds" is false as measured. Singular-stem `*_add`
// names are common (shot_add, shot_option_add, thicklaser_add, chargeshot_add,
// exatt_add, combo_add, gauge_avail_add, and the item_add / mima_star_add
// macros), and this very binary carries the counter-pair with its contract
// written down: th02/main/player/shot.cpp's shottype spawners scan [shots] and
// then call shot_add(), "which is why neither of those takes the slot".
// th02/main/enemy/update.cpp's enemies_spawn() / enemy_add() is the second
// instance of that split. Cite the slot-ownership contract, not a stem census.
void pascal far bg_particles_add(
	screen_x_t left, screen_y_t top, unsigned char angle
);

void far bg_particles_invalidate(void);
void far bg_particles_update_and_render(void);

#endif /* TH02_MAIN_BG_PARTICLE_HPP */
