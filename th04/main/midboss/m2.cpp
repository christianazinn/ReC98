/// Stage 2 midboss
/// ---------------
/// (#included from th04/mai.cpp. ZUN's object for this code segment held
/// the shared tile renderer and then this renderer - that an original object
/// held several unrelated sources is kb/codegen/0112. Both now compile from
/// C++ into the same replacement object.)

#include "platform.h"
#include "pc98.h"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/v_colors.hpp"
#include "th04/main/frames.h"
#include "th04/main/scroll.hpp"
#include "th04/main/midboss/midboss.hpp"
#include "th04/sprites/main_pat.h"

// Constants
// ---------

static const pixel_t MIDBOSS2_W = 64;
static const pixel_t MIDBOSS2_H = 64;

// [midboss.sprite] is not a patnum for this midboss, unlike every other one in
// TH04: it is a 0…2 animation selector, and each value picks one of these
// three consecutive st01.bmt cel ranges. What they depict is not recoverable
// from the binary, so they are numbered rather than named. [inferred] that
// they are consecutive at all — the original hardcodes 146, 150 and 152, and
// the cel counts below are what make those three numbers add up.
static const int PAT_MIDBOSS2_ANIM0 = (PAT_STAGE + 18);
static const int MIDBOSS2_ANIM0_CELS = 4;

static const int PAT_MIDBOSS2_ANIM1 = (PAT_MIDBOSS2_ANIM0 + MIDBOSS2_ANIM0_CELS);
static const int MIDBOSS2_ANIM1_CELS = 2;

static const int PAT_MIDBOSS2_ANIM2 = (PAT_MIDBOSS2_ANIM1 + MIDBOSS2_ANIM1_CELS);

// All three animations run at this speed; ANIM0 fills the 16-frame
// [stage_frame_mod16] cycle, the other two fill the 8-frame one.
static const int MIDBOSS2_FRAMES_PER_CEL = 4;
// ---------

// Rendering
// ---------

void pascal near midboss2_render(void)
{
	// [left] is declared before [top] to break their register-candidate tie
	// the way the original does: both are mentioned three times, [patnum]
	// five, so [patnum] takes SI outright and the earlier declaration takes
	// DI. (kb/codegen/0146) [top] is what lands in the `ENTER 2, 0` slot.
	screen_x_t left;
	vram_y_t top;
	int patnum;

	// The only clip, and it is on the *subpixel* coordinate rather than on
	// the VRAM row scroll_subpixel_y_to_vram_seg1() returns — so it keeps the
	// midboss from being blitted while it is still above the playfield, and
	// does nothing at the bottom edge.
	if(midboss.pos.cur.y > 0) {
		left = midboss.pos.cur.to_screen_left(MIDBOSS2_W);
		top = midboss.pos.cur.to_vram_top_scrolled_seg1(MIDBOSS2_H);

		// Not `!= PHASE_EXPLODE_BIG`: the comparison really is against 2, so
		// the midboss stops being drawn during any phase past its third —
		// including the defeat phases, which is why midboss_defeat_render()
		// is not called here the way midboss1_render() calls it.
		if(midboss.phase <= 2) {
			// Candidate `ZUN landmine`, deliberately reproduced and
			// deliberately NOT labelled by this parcel: there is no `else`,
			// so a [midboss.sprite] of 3 or more leaves [patnum] holding
			// whatever the caller had in SI. midboss_reset() does not clear
			// [midboss.sprite], and mx_update.cpp:141 leaves it at
			// PAT_ENEMY_KILL, so the value really can be out of range at
			// activation time. Whether the arm is ever *reached* depends on
			// whether midboss_update runs before midboss_render on the
			// activation frame, which this parcel did not measure. The same
			// deferral th04/main/midboss/m1.cpp makes for its own missing
			// bound: the taxonomy lane owns the call for the whole family.
			if(midboss.sprite == 0) {
				patnum = (
					(stage_frame_mod16 / MIDBOSS2_FRAMES_PER_CEL) +
					PAT_MIDBOSS2_ANIM0
				);
			} else if(midboss.sprite == 1) {
				patnum = (
					(stage_frame_mod8 / MIDBOSS2_FRAMES_PER_CEL) +
					PAT_MIDBOSS2_ANIM1
				);
			} else if(midboss.sprite == 2) {
				patnum = (
					(stage_frame_mod8 / MIDBOSS2_FRAMES_PER_CEL) +
					PAT_MIDBOSS2_ANIM2
				);
			}
			midboss_put_generic(left, top, patnum);
		}
	}
}
// ---------
