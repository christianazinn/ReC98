/// Stage 1 midboss - rendering
/// ---------------------------
/// The odd one out among TH05's five midboss renderers: it blits **twice**,
/// with a different sprite extent each time, and the second blit only happens
/// in two of the fight's phases.
///
/// The first blit is unconditional and animates on every frame. The second is
/// selected by [midboss.phase]: 2 and 3 pick different cel bases, and only
/// phase 3 honours [midboss.damage_this_frame] -- so the white flash cannot
/// appear during phase 2 at all.
///
/// Its own update half lives in th05/main/midboss/m1.cpp, which is a separate
/// object in main_03. That file's MIDBOSS1_W / MIDBOSS1_H are **subpixels**
/// (`TO_SP(32)`); the extents a renderer needs are pixels, so the two below
/// are declared here rather than shared.
///
/// (#included from th05/b34fg.cpp, at the front of that object. This function
/// was the last `proc` of th05_main.asm's MIDBOSSX_TEXT block once
/// sara_fg_render() was lifted out from under it. kb/codegen 0112 + 0114.)

#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/v_colors.hpp"
#include "th04/main/frames.h"
#include "th04/main/scroll.hpp"
#include "th04/main/phase.hpp"
#include "th04/main/midboss/midboss.hpp"

// Pixel extents, as opposed to th05/main/midboss/m1.cpp's subpixel ones. The
// two blits genuinely disagree: the first is placed as a 32x32 sprite and the
// second as a 64-tall one.
static const pixel_t MIDBOSS1_BODY_W = 32;
static const pixel_t MIDBOSS1_BODY_H = 32;
static const pixel_t MIDBOSS1_SECOND_H = 64;

static const int MIDBOSS1_BODY_FRAMES_PER_CEL = 4;
static const int MIDBOSS1_SECOND_FRAMES_PER_CEL = 2;

// [inferred] The two cel bases the second blit picks between, one per phase.
// th05/sprites/main_pat.h names [PAT_MIDBOSS1] as 204 and ends its block
// before either of these, so neither number has a name there and what they
// depict has not been decided -- only which phase reaches which.
static const int PAT_MIDBOSS1_PHASE2 = 208;
static const int PAT_MIDBOSS1_PHASE3 = 212;

void pascal near midboss1_render(void)
{
	screen_x_t left;
	vram_y_t top;
	int patnum;

	if(midboss.phase < PHASE_EXPLODE_BIG) {
		// Unlike every other midboss renderer in this segment, there is no
		// bottom-edge test here at all.
		left = midboss.pos.cur.to_screen_left(MIDBOSS1_BODY_W);
		top = midboss.pos.cur.to_vram_top_scrolled_seg1(MIDBOSS1_BODY_H);
		patnum = (
			midboss.sprite +
			(stage_frame_mod16 / MIDBOSS1_BODY_FRAMES_PER_CEL)
		);
		super_roll_put(left, top, patnum);

		left = midboss.pos.cur.x.to_pixel();
		top = midboss.pos.cur.to_vram_top_scrolled_seg1(MIDBOSS1_SECOND_H);
		if(midboss.phase == 2) {
			patnum = (
				PAT_MIDBOSS1_PHASE2 +
				(stage_frame_mod8 / MIDBOSS1_SECOND_FRAMES_PER_CEL)
			);
			super_roll_put(left, top, patnum);
		} else if(midboss.phase == 3) {
			patnum = (
				PAT_MIDBOSS1_PHASE3 +
				(stage_frame_mod8 / MIDBOSS1_SECOND_FRAMES_PER_CEL)
			);
			if(midboss.damage_this_frame == 0) {
				super_roll_put(left, top, patnum);
			} else {
				super_roll_put_1plane(
					left, top, patnum, 0, super_plane(V_WHITE)
				);
				midboss.damage_this_frame = 0;
			}
		}
	} else if(midboss.phase == PHASE_EXPLODE_BIG) {
		midboss_defeat_render();
	}
}
