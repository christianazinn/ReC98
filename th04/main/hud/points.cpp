/// Point item HUD row(s)
/// ---------------------
/// One name, two bodies. TH04 renders a single white 5-digit counter for the
/// current stage, using a renderer that hardcodes the text attribute; TH05
/// renders two rows through a different, 4-argument renderer that takes the
/// attribute as a parameter, and its second row shows a counter that TH04
/// doesn't have. Nothing is shared past the prologue.

#pragma option -zPmain_01

#include "libs/master.lib/pc98_gfx.hpp"
#include "th04/gaiji/gaiji.h"
#include "th04/main/hud/hud.hpp"
#include "th04/main/item/item.hpp"
#if (GAME == 5)
	#include "th05/resident.hpp"
#else
	#include "th04/resident.hpp"
#endif

// [HUD_POINT_ITEMS_STAGE_Y] and [HUD_POINT_ITEMS_EXTEND_Y] are in
// th04/main/hud/hud.hpp; hud_put() needs the same rows.

// ZUN's object for this code segment also held the remaining-lives and
// remaining-bombs rows, immediately ahead of this one and in this order, in
// *both* games, and score_reset() ahead of those again (kb/codegen/0129). The
// `GAME != 5` guard that used to sit around the two HUD rows was a
// lifting-order artifact of TH04 landing first, and is gone now that TH05's
// copies have left th05_main.asm too.
#if (GAME == 5)
	// TH05's slot here holds score_highest_update_and_reset(), a related but
	// different function -- see that file's docblock.
	#include "th05/main/score_highest.cpp"
#else
	// Ahead of score_reset() again, and the front of this object: it was the
	// last thing th04_main.asm contributed to HUD_PNT_TEXT. TH05's proc in
	// the same slot is a different body and is still assembly.
	#include "th04/main/score_extend.cpp"

	#include "th04/main/score_reset.cpp"
#endif
#include "th04/main/hud/lives.cpp"
#include "th04/main/hud/bombs.cpp"

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
