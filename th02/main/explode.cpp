/* ReC98
 * -----
 * TH02's boss defeat explosion. ZUN's object for this code segment held both
 * this and the bullets, which is why they are compiled into the same
 * translation unit here — see th02/bullet.cpp.
 */

// The original's prolog for the first function is `push bp; mov bp, sp`, which
// is -G; the second one's is `ENTER`, which is not. The bullet code that
// follows in this object was built with -G again, and th02/main/bullet/
// bullet.cpp turns it back on at its top. (kb/codegen/0011)
#pragma option -G

#include "platform.h"
#include "pc98.h"
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/hardware/egc.hpp"
#include "th02/hardware/pages.hpp"
#include "th02/snd/snd.h"
#include "th02/main/bg_particle.hpp"
#include "th02/main/explode.hpp"
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
// allow a 16×16 sprite to be blitted into.
static const vram_x_t MARGIN_LEFT_LEFT = (16 / BYTE_DOTS);
static const vram_x_t MARGIN_LEFT_RIGHT = (24 / BYTE_DOTS);
static const vram_x_t MARGIN_RIGHT_LEFT = (416 / BYTE_DOTS);
static const vram_x_t MARGIN_RIGHT_RIGHT = (424 / BYTE_DOTS);

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
	if(frame < EXPLODE_STAGE_2_BASE) {
		if(frame == 1) {
			snd_se_play(18);
		}

		// ZUN landmine: Shifts the entire stage 1 ring 7 pixels to the right
		// of the point the caller asked for, and of the stage 2 ring.
		center_x += 7;

		if(frame >= 2) {
			egc_start_copy_noframe();
			frame -= 2;
			radius = (240 - (frame * 8));
			dot_square_ring_invalidate(center_x, center_y, radius, 2);
			frame += 2;
			egc_off();
		}
		if(frame < EXPLODE_STAGE_1_FRAMES) {
			col = (((frame & 1) * 11) + 4);
			grcg_setcolor(GC_RMW, col);
			radius = (240 - (frame * 8));
			dot_square_ring_put(center_x, center_y, radius, 2);
			grcg_off();
		}
		return;
	}

	frame -= EXPLODE_STAGE_2_BASE;
	if(frame >= EXPLODE_STAGE_2_FRAMES) {
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
			ring_x = (
				((static_cast<long>(radius) * CosTable8[angle]) >> 8) + center_x
			);
			ring_y = (
				((static_cast<long>(radius) * Sin8(
					angle + boss_explode_angle_offset
				)) >> 8) + center_y
			);
			if(
				(ring_x >= 16) && (ring_x < 416) &&
				(ring_y > 0) && (ring_y < 384)
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
		ring_x = (
			((static_cast<long>(radius) * CosTable8[angle]) >> 8) + center_x
		);
		ring_y = (
			((static_cast<long>(radius) * Sin8(
				angle + boss_explode_angle_offset
			)) >> 8) + center_y
		);
		vram_top = scroll_screen_y_to_vram(vram_top, ring_y);

		// ZUN bloat: One pixel narrower on the left than the invalidation
		// loop above, which is the safe direction to be inconsistent in.
		if(
			(ring_x > 16) && (ring_x < 416) &&
			(ring_y > 0) && (ring_y < 384)
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
