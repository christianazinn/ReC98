/// Item splash circles: the renderer
/// ---------------------------------
/// The expanding dotted circle shown when an item spawns. Each live circle is
/// walked as one full 256-unit turn of the angle in ITEM_SPLASH_DOTS steps,
/// and every dot is clipped against the playfield in *subpixels* before it is
/// blitted — the circle can and does grow past the playfield edge.
///
/// TH05 emits this body at the front of playfld.cpp's PLAYFLD_TEXT
/// contribution. TH04 emits it from circle.cpp after the 26h-byte
/// grcg_modecol.asm prefix in IT_SPL_R_TEXT. The following IT_SPL_D_TEXT starts
/// at spark_render(), preserving the original one-byte pad through `codestring`
/// (kb/codegen 0080 + 0114 + 0161).
///
/// The two games' bodies differ only in ITEM_SPLASH_DOTS (32 against 64),
/// which th04/main/item/splash.hpp already supplies per game.

#include "th02/v_colors.hpp"
#include "th04/hardware/grcg.hpp"
#include "th04/main/drawp.hpp"
#include "th04/main/item/splash.hpp"
// Wraps th02/math/vector.hpp in `extern "C"`, which is the linkage the dump's
// bare `call vector2_at` needs.
#include "th04/math/vector.hpp"

// `extern "C"` and `pascal`, because the original module's export was
// undecorated and upper-case; th04/main/item/splash.hpp already declares it
// that way and says why.
extern "C" void pascal near item_splashes_render(void)
{
	item_splash_t near *splash;
	int angle;
	int i;

	// Stored and then passed, rather than passed directly: the original does
	// both, and the store is what the [bp-4] slot is for.
	subpixel_t radius;

	grcg_setcolor_direct(V_WHITE);
	splash = item_splashes;
	for(i = 0; i < ITEM_SPLASH_COUNT; (i++, splash++)) {
		if(splash->flag != F_ALIVE) {
			continue;
		}
		for(angle = 0; angle < 256; angle += (256 / ITEM_SPLASH_DOTS)) {
			radius = splash->radius_cur;
			vector2_at(
				drawpoint, splash->center.x, splash->center.y, radius, angle
			);

			// Clipped in subpixels against the playfield, Y first and both
			// bounds before X, with `>= 0` rather than `> 0` — which is the
			// `_yx_lt_ge` half of th02/main/playfld.hpp's pair, at a sprite
			// extent of zero. The dot is one pixel, so there is nothing to
			// center and both `w` and `h` are 0.
			if(!playfield_encloses_yx_lt_ge(drawpoint.x, drawpoint.y, 0, 0)) {
				continue;
			}

			// Through the pseudo-registers, and in this order, exactly as
			// gather_render() (th04/main/gather.cpp:178-180) calls the same
			// shape of `__fastcall` blitter: written as one call expression,
			// the Y that is computed first gets spilled with `push ax`/`pop dx`
			// instead of the original's `mov dx, ax`.
			_DX = drawpoint.to_vram_top_scrolled_seg1(0);
			_AX = drawpoint.to_screen_left();
			item_splash_dot_render(_AX, _DX);
		}
	}
}
