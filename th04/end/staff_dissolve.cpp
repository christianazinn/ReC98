/// The staff roll's CDG dissolve effect
/// ------------------------------------
/// Three interchangeable "put" functions, an unblit helper, and three drivers
/// that animate a CDG image in or out by calling one of the puts once per two
/// VSyncs while flipping pages. Which put is used is chosen through the
/// [dissolve_put_func] pointer, which staffroll_animate() rewrites between
/// phases.
///
/// The effect itself is entirely in the *data*: TH04's .CDG files hold four
/// pre-dissolved bitplanes, and these functions merely blit each plane to its
/// own offset, computed by pushing the image's centre out along one of four
/// directions by a per-frame distance. At distance 0 the four coincide and the
/// image is whole, which is why every one of them short-circuits to a plain
/// cdg_put_8() there. See th04/formats/cdg.h's note on cdg_put_plane().
///
/// `[measured]` These are the first seven procs of th04_maine.asm's
/// MAINE_01_TEXT contribution and they have no sibling anywhere in the tree:
/// TH05's staff roll is a different engine (804 of 826 common bytes differ,
/// first difference at byte 0), TH03's cdg_put_dissolve_e_8() is a row-mask
/// AND on the E plane, and cdg_put_plane() has no other C++ caller in the
/// project. Lifted together with staffroll_animate() as the last tail lift out
/// of that dump, which empties it.

#include "planar.h"
#include "th02/v_colors.hpp"
#include "th03/math/polar.hpp"
#include "th04/formats/cdg.h"
#include "th04/hardware/bgimage.hpp"
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"

/// master.lib's GRCG_OFF_CLOBBERING macro, which spills the port number to DX
/// rather than using the 8-bit immediate form. Spelled the same way
/// th04/hiscore/regist_view.cpp and th04/main/player/bombchar.cpp spell it.
#define grcg_off_clobbering_dx() outportb(0x7C, GC_OFF)

/// The distance at which the four planes have travelled far enough for the
/// image to have left the screen entirely.
#define DISSOLVE_DISTANCE_MAX 64

/// The angle step between two consecutive planes of the radial variant, and
/// the per-frame rotation the drivers apply on top of it.
#define DISSOLVE_PLANE_ANGLE 0x40
#define DISSOLVE_ANGLE_STEP 8

#define dissolve_plane_put(plane, x_center, y_center, r, ratio_x, ratio_y) { \
	x = polar(x_center, r, ratio_x); \
	y = polar(y_center, r, ratio_y); \
	cdg_put_plane(x, y, (cdg_slot + 1), plane); \
}

/// Blits the four planes of ([cdg_slot] + 1) to four positions [distance]/4
/// away from (base_left, base_top), each a quarter-turn further around than
/// the last, starting from the running [radial_angle].
void pascal near dissolve_put_radial(
	screen_x_t base_left, screen_y_t base_top, pixel_t distance
)
{
	// \[measured]\ A register copy of the parameter, declared BEFORE [x] so
	// that it, and not [x], gets SI: the original divides in SI and never
	// writes the result back to the stack slot.
	register int radius = distance;
	register int x;
	int y;

	if(radius == 0) {
		cdg_put_8(base_left, base_top, cdg_slot);
	} else {
		grcg_setcolor(GC_RMW, V_WHITE);
		radius /= 4;

		#define radial_plane_put(plane) { \
			dissolve_plane_put( \
				plane, base_left, base_top, radius, \
				CosTable8[radial_angle], SinTable8[radial_angle] \
			); \
			radial_angle += DISSOLVE_PLANE_ANGLE; \
		}

		radial_plane_put(0);
		radial_plane_put(1);
		radial_plane_put(2);
		radial_plane_put(3);

		#undef radial_plane_put

		grcg_off_clobbering_dx();
	}
}

/// Blits the four planes of ([cdg_slot] + 1) to four fixed directions away
/// from (base_left, base_top): two diagonals at half the distance, and
/// straight down and up at the full one.
void pascal near dissolve_put_diagonal(
	screen_x_t base_left, screen_y_t base_top, pixel_t distance
)
{
	// \[measured]\ A register copy of the parameter, declared BEFORE [x] so
	// that it, and not [x], gets SI: the original divides in SI and never
	// writes the result back to the stack slot.
	register int radius = distance;
	register int x;
	int y;

	if(radius == 0) {
		cdg_put_8(base_left, base_top, cdg_slot);
	} else {
		grcg_setcolor(GC_RMW, V_WHITE);
		radius /= 2;

		dissolve_plane_put(
			0, base_left, base_top, (radius / 2),
			CosTable8[0x60], CosTable8[0x20]
		);
		dissolve_plane_put(
			1, base_left, base_top, radius,
			CosTable8[0x40], CosTable8[0x00]
		);
		dissolve_plane_put(
			2, base_left, base_top, (radius / 2),
			CosTable8[0xE0], CosTable8[0xA0]
		);
		dissolve_plane_put(
			3, base_left, base_top, radius,
			CosTable8[0xC0], CosTable8[0x80]
		);

		grcg_off_clobbering_dx();
	}
}

/// Blits the four planes of ([cdg_slot] + 1) left and right of
/// (base_left, base_top), two of them at half the distance.
void pascal near dissolve_put_horizontal(
	screen_x_t base_left, screen_y_t base_top, pixel_t distance
)
{
	// \[measured]\ A register copy of the parameter, declared BEFORE [x] so
	// that it, and not [x], gets SI: the original divides in SI and never
	// writes the result back to the stack slot.
	register int radius = distance;
	register int x;
	int y;

	if(radius == 0) {
		cdg_put_8(base_left, base_top, cdg_slot);
	} else {
		grcg_setcolor(GC_RMW, V_WHITE);
		radius /= 2;

		dissolve_plane_put(
			0, base_left, base_top, (radius / 2),
			CosTable8[0x00], SinTable8[0x00]
		);
		dissolve_plane_put(
			1, base_left, base_top, radius,
			CosTable8[0x00], SinTable8[0x00]
		);
		dissolve_plane_put(
			2, base_left, base_top, (radius / 2),
			CosTable8[0x80], CosTable8[0x40]
		);
		dissolve_plane_put(
			3, base_left, base_top, radius,
			CosTable8[0x80], CosTable8[0x40]
		);

		grcg_off_clobbering_dx();
	}
}

/// Restores the background behind the CDG image in [cdg_slot], grown by
/// [distance]/2 in every direction so that it also covers the four planes
/// wherever the put functions above have pushed them.
void pascal near dissolve_unput(
	screen_x_t base_left, screen_y_t base_top, pixel_t w, pixel_t h,
	pixel_t distance
)
{
	distance /= 2;
	bgimage_put_rect_16(
		(base_left - distance), (base_top - distance),
		((distance * 2) + w), ((distance * 2) + h)
	);
}

/// One animation frame: wait out two VSyncs, flip the pages, and start drawing
/// into the one that is no longer shown.
#define dissolve_frame_flip() { \
	while(vsync_Count1 < 2) {} \
	vsync_Count1 = 0; \
	graph_showpage(page); \
	page = (1 - page); \
	graph_accesspage(page); \
}

/// Converges the four planes of ([cdg_slot] + 1) back into a whole image at
/// (base_left, base_top), then leaves that image on both pages.
void pascal near dissolve_in_animate(screen_x_t base_left, screen_y_t base_top)
{
	int page;
	register int distance;

	distance = (DISSOLVE_DISTANCE_MAX - 1);
	page = 0;
	graph_accesspage(0);
	// `[measured]` Cross-jumped with the graph_accesspage() at the end of the
	// loop body below, which is why the original spills both port numbers to
	// DX and shares one `out`.
	graph_showpage(1);
	while(distance > 0) {
		dissolve_unput(
			base_left, base_top,
			cdg_slots[cdg_slot].pixel_w, cdg_slots[cdg_slot].pixel_h,
			(distance + 1)
		);
		distance--;
		radial_angle += DISSOLVE_ANGLE_STEP;
		dissolve_put_func(base_left, base_top, distance);
		dissolve_frame_flip();
	}
	graph_copy_page(page);
}

/// Pushes the four planes of ([cdg_slot] + 1) apart until the image has left
/// the screen, then leaves the bare background on both pages.
void pascal near dissolve_out_animate(screen_x_t base_left, screen_y_t base_top)
{
	int page;
	register int distance;

	distance = 0;
	page = 0;
	graph_accesspage(0);
	graph_showpage(1);
	while(1) {
		dissolve_unput(
			base_left, base_top,
			cdg_slots[cdg_slot].pixel_w, cdg_slots[cdg_slot].pixel_h, distance
		);
		distance++;
		radial_angle += DISSOLVE_ANGLE_STEP;
		if(distance >= DISSOLVE_DISTANCE_MAX) {
			break;
		}
		dissolve_put_func(base_left, base_top, distance);
		dissolve_frame_flip();
	}
	graph_copy_page(1 - page);
}

/// dissolve_out_animate() for two images at once — slot 2 at
/// (base_left_1, base_top_1) and slot 0 at (base_left_2, base_top_2) — with
/// the per-frame rotation running backwards.
void pascal near dissolve_out_2_animate(
	screen_x_t base_left_1, screen_y_t base_top_1,
	screen_x_t base_left_2, screen_y_t base_top_2
)
{
	int page;
	register int distance;

	distance = 0;
	page = 0;
	graph_accesspage(0);
	graph_showpage(1);
	while(1) {
		dissolve_unput(
			base_left_1, base_top_1,
			cdg_slots[2].pixel_w, cdg_slots[2].pixel_h, distance
		);
		dissolve_unput(
			base_left_2, base_top_2,
			cdg_slots[0].pixel_w, cdg_slots[0].pixel_h, distance
		);
		distance++;
		radial_angle -= DISSOLVE_ANGLE_STEP;
		if(distance >= DISSOLVE_DISTANCE_MAX) {
			break;
		}
		cdg_slot = 2;
		dissolve_put_func(base_left_1, base_top_1, distance);
		cdg_slot = 0;
		dissolve_put_func(base_left_2, base_top_2, distance);
		dissolve_frame_flip();
	}
	graph_copy_page(1 - page);
}
