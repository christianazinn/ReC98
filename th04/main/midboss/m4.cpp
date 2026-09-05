/// Stage 4 midboss
/// ---------------

#pragma option -zCM4_RENDER_TEXT -zPmain_01

#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/v_colors.hpp"
#include "th04/main/scroll.hpp"
#include "th04/main/phase.hpp"
#include "th04/main/midboss/midboss.hpp"
#include "th04/main/hud/hud.hpp"
#include "th04/sprites/main_pat.h"

// Constants
// ---------

static const pixel_t MIDBOSS4_W = 64;
static const pixel_t MIDBOSS4_H = 64;
// ---------

void pascal near midboss4_render(void)
{
	// ZUN bug: Should use the large variant instead. MIDBOSS4_H exceeds the
	// 32-pixel rolling playfield margin, and th02/main/playfld.hpp's own text
	// says such sprites take < and > rather than ≤ and ≥.
	//
	// [verified-by-emulator] Reached, and exactly once per encounter: the two
	// variants differ only in strictness, so they disagree only on a
	// coordinate that lands exactly on a bound — and this one does. The
	// midboss enters at pos.cur.y = TO_SP(-32) with velocity.y = TO_SP(2), so
	// y is exactly TO_SP(0) on the 16th frame after activation, which the
	// small variant rejects and the large variant draws. Measured over the
	// 707-frame encounter of the stock Stage 4 demo: 16 rejected frames
	// against the large variant's 15, one divergent frame, and none of the
	// other three bounds ever reached exactly. So the midboss's first drawn
	// frame is one frame later than intended, costing 64 × 32 pixels of sprite
	// below PLAYFIELD_TOP on that frame.
	//
	// Note for whoever fixes it: the early `return` below also skips the
	// midboss_put_generic() call that is the only place clearing
	// midboss.damage_this_frame, so this and the carryover bug move together.
	// "bug" is deliberate, against the `ZUN quirk` that
	// th04/main/midboss/midboss.hpp:64 still carries: that label is upstream's
	// own and was refuted on measured evidence in
	// state/port/FIX_LAYER_CANDIDATES.md §B. Fix the carryover first; the
	// ordering constraint is recorded there too.
	if(playfield_clip_point_yx_small_roll(
		midboss.pos.cur, MIDBOSS4_W, MIDBOSS4_H
	)) {
		return;
	}
	screen_x_t left = midboss.pos.cur.x.to_pixel();
	vram_y_t top = midboss.pos.cur.to_vram_top_scrolled_seg1(MIDBOSS4_H);
	if(midboss.phase <= 2) {
		int patnum = (PAT_MIDBOSS4_STILL_LEFT + midboss.sprite);
		if(midboss.pos.cur.x.v >= to_sp(PLAYFIELD_W / 2)) {
			patnum += M4C_CELS;
		}
		midboss_put_generic(left, top, patnum);
	} else if(midboss.phase == PHASE_EXPLODE_BIG) {
		midboss_defeat_render();
	}
}
