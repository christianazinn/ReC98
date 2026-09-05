/// Thick laser rendering
/// ---------------------
/// (#included from th04/laser_r.cpp, which owns the segment and the `-G` this
/// function's hand-built frame needs.)
///
/// Every non-TF_FREE thick laser, drawn as a circle at the top of a beam that
/// runs down to the bottom of the playfield, in up to three concentric layers:
/// [col_outline], then [col_outline] + 1, then white. Writes no thicklaser_t
/// field, which is why the name is `_render` and not `_update_and_render`.
///
/// TH04-only: TH05 has no thick lasers, and neither of its dumps mentions
/// thicklaser_t.

#include "platform.h"
#include "pc98.h"
#include "libs/master.lib/pc98_gfx.hpp"
// AFTER pc98_gfx.hpp on purpose: it #undefs master.lib's grcg_off() and
// redefines it as the inline `outportb(0x7C, 0)` the original ends on --
// `mov dx, 7Ch` / `mov al, 0` / `out dx, al`, which is GRCG_OFF_CLOBBERING.
// master.lib's own is a far call. Same order as th04/main/boss/b5r.cpp.
#include "th01/hardware/grcg.hpp"
#include "th02/v_colors.hpp"
#include "th02/main/playfld.hpp"
#include "th04/hardware/grcg.hpp"
#include "th04/main/bullet/laser_t.hpp"

// The width of each of the two colored layers, and the amount by which each
// one insets the white core. Capped so that a laser wide enough never grows
// more than a 16-pixel border, which is also why a laser thinner than 8 pixels
// gets no outline at all and one thinner than 4 gets nothing but the core.
static const pixel_t THICKLASER_LAYER_W_MAX = 16;

void near thicklasers_render(void)
{
	int i;

	// The circle's center, and the left and right edges of the beam below it.
	// [circle_center_y] doubles as the top edge of every box drawn here: the
	// beam starts at the circle's center and runs to the bottom of the
	// playfield.
	screen_x_t center_x;
	vram_y_t circle_center_y;
	screen_x_t left;
	screen_x_t right;

	// The two register variables. The laser pointer earns SI by how often it
	// is dereferenced as a base (kb/codegen/0117); [layer_w] takes DI, and
	// [i] is left in memory because it is only touched twice.
	thicklaser_t near *tl = thicklasers;
	pixel_t layer_w;

	for(i = 0; i < THICKLASER_COUNT; i++, tl++) {
		if(tl->flag == TF_FREE) {
			continue;
		}

		// A TF_LINE laser is the 1-pixel telegraph line that precedes the
		// beam, and has no radius to offset its origin by.
		if(tl->flag == TF_LINE) {
			center_x = (tl->origin.x.to_pixel() + PLAYFIELD_LEFT);
			circle_center_y = (tl->origin.y.to_pixel() + PLAYFIELD_TOP);
			grcg_setcolor(GC_RMW, V_WHITE);
			grcg_vline(center_x, circle_center_y, (PLAYFIELD_BOTTOM - 1));
			continue;
		}

		center_x = (tl->origin.x.to_pixel() + PLAYFIELD_LEFT);
		circle_center_y = (
			tl->origin.y.to_pixel() + tl->radius_cur + PLAYFIELD_TOP
		);
		left = (center_x - tl->radius_cur);
		right = (tl->radius_cur + center_x);

		layer_w = (tl->radius_cur / 4);
		if(layer_w > THICKLASER_LAYER_W_MAX) {
			layer_w = THICKLASER_LAYER_W_MAX;
		}

		// Outermost layer: the full circle, and the two outer slivers of the
		// beam on either side of everything drawn after it.
		if((layer_w / 2) != 0) {
			grcg_setcolor(GC_RMW, tl->col_outline);
			grcg_circlefill(center_x, circle_center_y, tl->radius_cur);
			grcg_boxfill(
				left,
				circle_center_y,
				(left + (layer_w / 2)),
				(PLAYFIELD_BOTTOM - 1)
			);
			grcg_boxfill(
				(right - (layer_w / 2)),
				circle_center_y,
				right,
				(PLAYFIELD_BOTTOM - 1)
			);
		}

		// Second layer, inset by half a layer on each side.
		if(layer_w != 0) {
			grcg_setcolor(GC_RMW, (tl->col_outline + 1));
			grcg_circlefill(
				center_x, circle_center_y, (tl->radius_cur - (layer_w / 2))
			);
			grcg_boxfill(
				(left + (layer_w / 2)),
				circle_center_y,
				(left + layer_w),
				(PLAYFIELD_BOTTOM - 1)
			);
			grcg_boxfill(
				(right - layer_w),
				circle_center_y,
				(right - (layer_w / 2)),
				(PLAYFIELD_BOTTOM - 1)
			);
		}

		// White core, filling whatever is left between the two layers.
		left += layer_w;
		right -= layer_w;
		grcg_setcolor(GC_RMW, V_WHITE);
		grcg_circlefill(center_x, circle_center_y, (tl->radius_cur - layer_w));
		grcg_boxfill(left, circle_center_y, right, (PLAYFIELD_BOTTOM - 1));
	}
	grcg_off();
}
