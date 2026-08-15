/// Dream HUD row
/// -------------
/// One name, two bodies — the campaign's clearest case of a shared name
/// hiding two different functions (0x17 bytes in TH04, 0x71 in TH05).
/// TH04's "dream" is the current point value of a single point item, and this
/// function only prints it as a 5-digit number, pre-multiplied by 10. TH05's
/// is a 0–BAR_MAX meter that fills as point items are collected, rendered as
/// a bar; that version also owns the edge detection for the DREAMBONUS MAX
/// popup, which has no TH04 counterpart at all. Nothing is shared past the
/// prologue, so this file is split down the middle like
/// th04/main/hud/points.cpp.

#pragma option -zPmain_01 -G

#include "libs/master.lib/pc98_gfx.hpp"
#include "th04/main/hud/hud.hpp"
#include "th04/main/item/item.hpp"
#if (GAME == 5)
	#include "th04/main/bullet/clearzap.hpp"
	#include "th04/main/hud/overlay.hpp"
#endif

// [HUD_DREAM_Y] is in th04/main/hud/hud.hpp; hud_put() needs the same row.

#if (GAME == 5)
	// Same value as the ASM-side HUD_DREAM_COLOR_COUNT in
	// th05/main/hud/dream[data].asm.
	static const int HUD_DREAM_COLOR_COUNT = 9;
#endif

extern "C" void pascal hud_dream_put(void)
{
#if (GAME == 5)
	struct hack_colors {
		uint8_t x[HUD_DREAM_COLOR_COUNT]; // ACTUAL TYPE: tram_atrb2[]
	};
	extern struct hack_colors HUD_DREAM_COLORS;

	// The [dream] value of the previous call, purely so that the transition
	// to a full meter can be detected here rather than wherever [dream] is
	// actually raised.
	extern uint8_t dream_prev;

	// An odd number of bytes, so this comes out as individual MOVs rather
	// than the REP MOVSW that HUD_POWER_COLORS' even size gets.
	const struct hack_colors COLORS = HUD_DREAM_COLORS;

	// Note the asymmetric spelling of the two halves. Turbo C++ 4.0J takes
	// each relational operator literally and never rewrites `<` into
	// `<= (c - 1)`, so `dream_prev < BAR_MAX` would emit `cmp 80h` / `JNB`
	// where the original has `cmp 7Fh` / `JA`. (kb/codegen/0092)
	if((dream_prev <= (BAR_MAX - 1)) && (dream >= BAR_MAX)) {
		overlay_popup_show(POPUP_ID_DREAMBONUS_MAX);
		bullets_clear();
	}
	dream_prev = dream;

	hud_bar_put(
		HUD_DREAM_Y,
		dream,
		COLORS.x[dream / (BAR_MAX / (HUD_DREAM_COLOR_COUNT - 1))]
	);
#else
	hud_5_digit_put(HUD_LABELED_LEFT, HUD_DREAM_Y, (dream_score * 10));
#endif
}
