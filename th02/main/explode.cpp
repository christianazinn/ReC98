/* ReC98
 * -----
 * TH02's boss defeat explosion. ZUN compiled it into its own object, ahead of
 * the bullets in the same BULLET_TEXT segment, so it stays a separate object
 * here too — the segment and group are named by this file's object wrapper,
 * th02/explode.cpp. (kb/codegen/0112)
 */

// The original's prolog for the first function is `push bp; mov bp, sp`, which
// is -G; the second one's is `ENTER`, which is not. The bullet code that
// follows in this *segment* was built with -G again, and th02/main/bullet/
// bullet.cpp turns it back on at its own top — a separate object, so this
// file's -G- below cannot reach it either way. (kb/codegen/0011)
#pragma option -G

#include "platform.h"
#include "pc98.h"
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th01/math/polar.hpp"
#include "th02/hardware/egc.hpp"
#include "th02/hardware/pages.hpp"
#include "th02/snd/snd.h"
#include "th02/main/bg_particle.hpp"
#include "th02/main/explode.hpp"
#include "th02/main/playfld.hpp"
#include "th02/main/scroll.hpp"
#include "th02/main/slowdown.hpp"
#include "th02/main/tile/tile.hpp"

// Not declared in libs/master.lib/pc98_gfx.hpp, which only has the
// graph_scrollup() convenience wrapper around it.
extern "C" {
	void MASTER_RET graph_scroll(unsigned line1, unsigned adr1, unsigned adr2);
}

// The two 16-pixel-wide columns cleared by boss_explode_margins_clear(), in
// VRAM bytes. Also the exact columns that the clipping conditions further down
// allow a 16×16 sprite to be blitted into. Both are two byte columns wide and
// sit immediately outside the playfield, so all four derive from it — the right
// pair is literally PLAYFIELD_VRAM_RIGHT, which these used to spell out as a
// bare 416 / 8.
static const vram_x_t MARGIN_LEFT_LEFT = (PLAYFIELD_VRAM_LEFT - 2);
static const vram_x_t MARGIN_LEFT_RIGHT = (PLAYFIELD_VRAM_LEFT - 1);
static const vram_x_t MARGIN_RIGHT_LEFT = PLAYFIELD_VRAM_RIGHT;
static const vram_x_t MARGIN_RIGHT_RIGHT = (PLAYFIELD_VRAM_RIGHT + 1);

void near boss_explode_margins_clear(void)
{
	grcg_setcolor(GC_RMW, 0);
	graph_accesspage(page_front);
	grcg_byteboxfill_x(MARGIN_LEFT_LEFT, 0, MARGIN_LEFT_RIGHT, (RES_Y - 1));
	grcg_byteboxfill_x(MARGIN_RIGHT_LEFT, 0, MARGIN_RIGHT_RIGHT, (RES_Y - 1));
	graph_accesspage(page_back);
	grcg_byteboxfill_x(MARGIN_LEFT_LEFT, 0, MARGIN_LEFT_RIGHT, (RES_Y - 1));
	grcg_byteboxfill_x(MARGIN_RIGHT_LEFT, 0, MARGIN_RIGHT_RIGHT, (RES_Y - 1));
	grcg_off();
}

#pragma option -G-

void pascal near boss_explode_render(
	screen_x_t center_x, screen_y_t center_y, int frame
)
{
	int ring_y;
	int patnum;
	vram_y_t vram_top;
	int i;
	unsigned char angle;
	vc_t col;
	register int radius;
	register screen_x_t ring_x;

	if(frame < 0) {
		return;
	}
	if(frame == 0) {
		boss_explode_margins_clear();
	}
	if(frame < EXPLODE_PHASE_2_START) {
		if(frame == 1) {
			snd_se_play(18);
		}

		// Shifts the entire phase 1 ring 7 pixels to the right of the point the
		// caller asked for, and of the phase 2 ring.
		//
		// NOT `ZUN landmine`, which is what this said before: a landmine
		// requires "fix would be observable: no", and a 7-pixel displacement of
		// the whole ring is observable in ZUN's own build, under no special
		// assumption. [center_x] feeds only dot_square_ring_invalidate() and
		// dot_square_ring_put() below, both rendering-only, so a fix cannot
		// desync a replay either. That leaves `ZUN bug` or `ZUN quirk`, and the
		// phase 2 ring using the unshifted center is surrounding-code evidence
		// for `bug` — but the call is the taxonomy lane's, and an unclear
		// classification is a stop condition (kb/conventions/rec98-taxonomy.md).
		// See state/port/FIX_LAYER_CANDIDATES.md J4.
		center_x += 7;

		if(frame >= 2) {
			egc_start_copy_noframe();
			frame -= 2;
			radius = (240 - (frame * 8));
			dot_square_ring_invalidate(center_x, center_y, radius, 2);
			frame += 2;
			egc_off();
		}
		if(frame < EXPLODE_PHASE_1_FRAMES) {
			col = (((frame & 1) * 11) + 4);
			grcg_setcolor(GC_RMW, col);
			radius = (240 - (frame * 8));
			dot_square_ring_put(center_x, center_y, radius, 2);
			grcg_off();
		}
		return;
	}

	frame -= EXPLODE_PHASE_2_START;
	if(frame >= EXPLODE_PHASE_2_FRAMES) {
		return;
	}
	if(frame == 0) {
		snd_se_play(14);
	}
	if(frame >= 2) {
		frame -= 2;
		radius = (frame * 16);
		egc_start_copy_noframe();
		for(i = 0; i < 256; i += 4) {
			angle = (i + frame);

			// The two helpers are not interchangeable here, and the asymmetry
			// is ZUN's: X indexes CosTable8[] raw, which is polar_x_fast()'s
			// body character for character, while Y goes through Sin8()'s
			// `& 0xff` mask, which is the polar_y() *template*. polar_y_fast()
			// would turn that mask into an int -> unsigned char narrowing
			// instead — same value, different construct.
			ring_x = polar_x_fast(center_x, radius, angle);
			ring_y = polar_y(
				center_y, radius, (angle + boss_explode_angle_offset)
			);
			// The right and bottom bounds are the playfield's own edges. The
			// left and top ones deliberately are NOT: they are 16 and 0, not
			// PLAYFIELD_LEFT (32) and PLAYFIELD_TOP (16), because the ring is
			// allowed into the cleared margin columns above. Do not "fix" them
			// into the macros.
			if(
				(ring_x >= 16) && (ring_x < PLAYFIELD_RIGHT) &&
				(ring_y > 0) && (ring_y < PLAYFIELD_BOTTOM)
			) {
				tiles_invalidate_rect(ring_x, ring_y, 16, 16);
			}
		}
		frame += 2;
		tiles_egc_render();
		egc_off();
	}

	slowdown_factor = 1;
	if(frame >= 24) {
		return;
	}
	patnum = ((frame / 6) + 88);
	radius = (frame * 16);
	for(i = 0; i < 256; i += 4) {
		angle = (i + frame);
		ring_x = polar_x_fast(center_x, radius, angle);
		ring_y = polar_y(center_y, radius, (angle + boss_explode_angle_offset));
		vram_top = scroll_screen_y_to_vram(vram_top, ring_y);

		// One pixel narrower on the left than the invalidation loop above
		// (`> 16` here against `>= 16` there), which is the safe direction to
		// be inconsistent in: a square that is invalidated but not drawn leaves
		// no artifact, whereas the reverse would.
		//
		// NOT `ZUN bloat`, which is what this said before: bloat requires that
		// the code "could be significantly simplified without making observable
		// changes", and harmonising the two bounds *is* observable — a sprite
		// whose [ring_x] lands on exactly 16 is skipped now and would then be
		// drawn. Same open bug-vs-quirk question as the phase 1 shift above;
		// see state/port/FIX_LAYER_CANDIDATES.md J4.
		if(
			(ring_x > 16) && (ring_x < PLAYFIELD_RIGHT) &&
			(ring_y > 0) && (ring_y < PLAYFIELD_BOTTOM)
		) {
			super_roll_put_tiny(ring_x, vram_top, patnum);
		}
	}

	slowdown_factor = 2;
	if(frame <= 8) {
		palette_settone((((frame & 1) * 70) + 100));
	}
	if(frame <= 20) {
		i = (frame & 1);
		graph_scroll(
			(RES_Y - scroll_line), ((scroll_line * 40) + i), i
		);
		slowdown_factor = 3;
	}
}
