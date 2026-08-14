/// Point item HUD row(s)
/// ---------------------
/// One name, two bodies. TH04 renders a single white 5-digit counter for the
/// current stage, using a renderer that hardcodes the text attribute; TH05
/// renders two rows through a different, 4-argument renderer that takes the
/// attribute as a parameter, and its second row shows a counter that TH04
/// doesn't have. Nothing is shared past the prologue.

#pragma option -zPmain_01

#include "libs/master.lib/pc98_gfx.hpp"
#include "th04/main/hud/hud.hpp"
#include "th04/main/item/item.hpp"

#if (GAME == 5)
// Below the gs_TEN label that hud_put() draws at (58, 16).
static const utram_y_t HUD_POINT_ITEMS_STAGE_Y = 16;

// Below the gsRUIKEI ("cumulative") label that hud_put() draws at (57, 15).
static const utram_y_t HUD_POINT_ITEMS_EXTEND_Y = 15;
#else
// Below the gs_TEN label that hud_put() draws at (58, 15).
static const utram_y_t HUD_POINT_ITEMS_STAGE_Y = 15;
#endif

extern "C" void pascal hud_point_items_put(void)
{
#if (GAME == 5)
	hud_5_digit_put(
		HUD_LABELED_LEFT,
		HUD_POINT_ITEMS_STAGE_Y,
		stage_point_items_collected,
		TX_WHITE
	);
	hud_5_digit_put(
		HUD_LABELED_LEFT,
		HUD_POINT_ITEMS_EXTEND_Y,
		extend_point_items_collected,
		TX_CYAN
	);
#else
	// [stage_point_items_collected] is a byte in TH04, and the original
	// zero-extends it through AX right before the push. No `(_AX = ...)` is
	// needed to get that: kb/codegen/0034's wrapper is load-bearing only
	// against a byte-sized *formal*, and hud_5_digit_put()'s [val] is a
	// 16-bit `uint16_t`, so the ordinary widening conversion emits the same
	// `mov al` / `mov ah, 0` / `push ax` unaided. Measured byte-identical
	// both ways, 2026-08-14; the discriminator is the callee's formal width,
	// not whether the source byte is a global. (kb/codegen/0091)
	hud_5_digit_put(
		HUD_LABELED_LEFT,
		HUD_POINT_ITEMS_STAGE_Y,
		stage_point_items_collected
	);
#endif
}
