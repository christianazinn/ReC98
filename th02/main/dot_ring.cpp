/* ReC98
 * -----
 * The two rings of dot squares that TH02's boss defeat explosion draws.
 * Declared together with the dot-square primitive itself in
 * th02/main/bg_particle.hpp, but ZUN compiled them into their own object, in
 * their own code segment, between the two enemy objects — hence the separate
 * translation unit and its object wrapper, th02/main_04.cpp.
 */

// The original's prologs are `push bp; mov bp, sp; sub sp, 4`, which is -G.
// (kb/codegen/0011)
#pragma option -G

#include "platform.h"
#include "pc98.h"
#include "libs/master.lib/master.hpp"
#include "th01/math/polar.hpp"
#include "th02/main/bg_particle.hpp"
#include "th02/main/playfld.hpp"
#include "th02/main/scroll.hpp"
#include "th02/main/tile/tile.hpp"

// Clipping box shared by both functions below. Note that it is neither the
// playfield nor either of the two boxes in th02/main/explode.cpp: it starts 8
// pixels outside the playfield's left and top edges, but ends exactly at its
// right and bottom ones.
#define ring_clipped(left, top) ( \
	((left) >= PLAYFIELD_RIGHT) || \
	((left) <= (PLAYFIELD_LEFT - 8)) || \
	((top) <= (PLAYFIELD_TOP - 8)) || \
	((top) >= PLAYFIELD_BOTTOM) \
)

void pascal far dot_square_ring_put(
	screen_x_t center_x, screen_y_t center_y, int radius, int angle_step
)
{
	unsigned int angle_cur;
	unsigned char angle;
	uint8_t edge;

	edge = ((radius >> 6) + 1);
	for(angle_cur = 0; angle_cur < 256; angle_cur += angle_step) {
		angle = angle_cur;
		dot_square_left = polar_x_fast(center_x, radius, angle);
		dot_square_top = polar_y_fast(center_y, radius, angle);
		if(ring_clipped(dot_square_left, dot_square_top)) {
			continue;
		}

		// Same as scroll_screen_y_to_vram(), which can't be used here because
		// its `ret` is already the value to be converted.
		dot_square_top += scroll_line;
		if(dot_square_top >= RES_Y) {
			dot_square_top -= RES_Y;
		}

		grcg_dot_square_put(edge);
	}
}

void pascal far dot_square_ring_invalidate(
	screen_x_t center_x, screen_y_t center_y, int radius, int angle_step
)
{
	unsigned int angle_cur;
	unsigned char angle;
	uint8_t edge;
	register screen_x_t left;
	register vram_y_t top;

	edge = ((radius >> 6) + 1);
	for(angle_cur = 0; angle_cur < 256; angle_cur += angle_step) {
		angle = angle_cur;
		left = polar_x_fast(center_x, radius, angle);
		top = polar_y_fast(center_y, radius, angle);
		if(ring_clipped(left, top)) {
			continue;
		}
		// [top] is pushed out of AX, where the assignment above left it, rather
		// than out of DI where the variable actually lives — which is what the
		// clipping test just read it from. Naming the register is the only way
		// to reproduce that, the same way th02/main/bg_particle.cpp has to name
		// DX and AX to keep its own clipping test from reloading.
		tiles_invalidate_rect(left, _AX, edge, edge);
	}
}

#undef ring_clipped

#pragma option -G-
