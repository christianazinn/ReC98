/// Graze counter HUD row
/// ---------------------
/// One name, one body, in contrast to this HUD row's neighbors: the two games
/// differ only in the row, and in the attribute parameter that TH05's
/// 4-argument renderer takes while TH04's 3-argument one hardcodes TX_WHITE.

#pragma option -zPmain_01

#include "libs/master.lib/pc98_gfx.hpp"
#include "th04/main/hud/hud.hpp"
#include "th04/main/score.hpp"

// [HUD_GRAZE_Y] is in th04/main/hud/hud.hpp; hud_put() needs the same row.

void hud_graze_put(void)
{
	hud_5_digit_put(
		HUD_LABELED_LEFT, HUD_GRAZE_Y, stage_graze
#if (GAME == 5)
		, TX_WHITE
#endif
	);
}
