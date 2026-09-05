/// Cheeto bullets: the renderer
/// ----------------------------
/// Blits every live cheeto bullet: the trail first, from its oldest node
/// towards the head and only every second one, then the head sprite itself.
/// The state half of the same loop is th05/main/bullet/cheeto_u.cpp, whose
/// two functions this one mirrors down to the loop shape and the local names.
///
/// TH05 only — TH04 has no cheeto bullets at all.
///
/// (#included from th04/main/playfld.cpp under `#if (GAME == 5)`, AHEAD of
/// th04/main/item/splashes_render.cpp, which is kb/codegen/0129's host-source
/// form. The hand-written module this replaces was the whole of
/// th05_main.asm's PLAYFLD_TEXT root contribution and th05/playfld.cpp
/// already owned everything after it, so this #include is the original
/// address order and costs no carve, no new segment, no group-list edit and
/// no Tupfile.lua line (kb/codegen 0114 + 0129). That module is gone from the
/// tree — no other dump included it — and the root contribution is now zero
/// bytes; PLAYFLD_TEXT is `byte`-aligned, so an empty root block pads
/// nothing.)
///
/// ZUN's clipping here is bespoke rather than one of th02/main/playfld.hpp's
/// macros, and mixes their two variants: `_small` on the bottom and right
/// bounds, `_large` on the left, and neither on the top, which uses `<` at
/// the `_small` bound. Written out below for that reason.

// The original's prolog is `push bp` / `mov bp, sp` / `sub sp, 6`, not
// `ENTER 6, 0` (kb/codegen/0011). Bracketed and restored at the bottom,
// because on this side the file shares a translation unit with everything
// th04/main/playfld.cpp compiles after it, and those are already matched
// under the command line's own `-G-` (kb/codegen/0112 trap 1). Same bracket,
// same reason, as th04/main/bullet/render.cpp two #includes further down.
#pragma option -G

#include "th04/hardware/grcg.hpp"
#include "th04/main/custom.hpp"
#include "th05/main/bullet/cheeto.hpp"

// `extern "C"` + `pascal`, because the module published the undecorated
// upper-case `CHEETOS_RENDER`; th05/main/bullet/cheeto.hpp already declares it
// that way and says why. th05_main.asm both calls it and takes its address,
// so that dump keeps a declaration where the `include` used to be.
extern "C" void pascal near cheetos_render(void)
{
	#define left	_AX
	#define top 	_DX

	// The clip both halves of the body run, and the reason the original
	// spells it as a TASM macro: it is expanded twice, with only the label it
	// skips to differing. [top] is a raw pixel coordinate here — ZUN never
	// calls scroll_subpixel_y_to_vram_seg1() for cheetos, and instead wraps
	// it by hand below, which cheeto.hpp's declaration already warns about.
	#define clipped() ( \
		(static_cast<pixel_t>(top) < (PLAYFIELD_TOP - CHEETO_H)) || \
		(static_cast<pixel_t>(top) >= PLAYFIELD_BOTTOM) || \
		(static_cast<pixel_t>(left) < 0) || \
		(static_cast<pixel_t>(left) >= PLAYFIELD_RIGHT) \
	)

	// Y wrapping is necessary for these sprites: they are taller than
	// PLAYFIELD_TOP and do leave the playfield through it, so their top
	// coordinate lands between -1 and -16 before the clip above rejects it at
	// -16. cheeto_put() wraps the remaining rows itself.
	#define wrap_top() \
		if(static_cast<pixel_t>(top) < 0) { \
			top += RES_Y; \
		}

	int node_i;
	cheeto_trail_t near *trail_p;
	int b_i;
	int trail_sprite;
	cheeto_head_t near *head_p;

	head_p = cheeto_heads;
	trail_p = cheeto_trails;
	_ES = SEG_PLANE_B;

	for(b_i = 1; (b_i < (1 + CHEETO_COUNT)); (b_i++, head_p++, trail_p++)) {
		if(trail_p->flag == CF_FREE) {
			continue;
		}
		grcg_setcolor_direct(trail_p->col);

		// Yes, we only render every second node! You only start to notice
		// jagged edges and gaps between the nodes once their speed exceeds
		// roughly 11 pixels per second, which never happens during regular
		// gameplay. https://rec98.nmlgc.net/blog/2020-02-29 has a demo video
		// of how this optimization would look at higher speeds.
		for(
			node_i = (CHEETO_TRAIL_NODE_COUNT - 1);
			(node_i > 0);
			(node_i -= 2)
		) {
			trail_sprite = trail_p->node_sprite[node_i];
			top = trail_p->node_pos[node_i].y.to_pixel();
			left = trail_p->node_pos[node_i].to_screen_left(CHEETO_W);
			if(clipped()) {
				continue;
			}
			wrap_top();
			cheeto_put(left, top, trail_sprite);
		}

		top = head_p->pos.cur.y.to_pixel();
		left = head_p->pos.cur.to_screen_left(CHEETO_W);
		if(clipped()) {
			continue;
		}
		wrap_top();
		cheeto_put(left, top, head_p->sprite);
	}

	#undef wrap_top
	#undef clipped
	#undef top
	#undef left
}

// Restores what th04/main/playfld.cpp had before this file: the command line
// passes no -G, and everything this object compiles after this point needs
// `ENTER`.
#pragma option -G-
