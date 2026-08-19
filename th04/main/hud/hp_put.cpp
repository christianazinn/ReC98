/// HP bar HUD row
/// --------------
/// The "Enemy!!" caption and the bar underneath it, drawn by one function
/// because a [bar_value] of 0 blanks both. Its own file rather than a second
/// function in th04/main/hud/hp.cpp, which holds the high-level updater that
/// calls this one: the two are in different original segments.
///
/// (#included from th04/hud_bar.cpp, AHEAD of th04/main/hud/bar_put.cpp, so
/// that this lands at its original address at the front of HUD_PUT_TEXT. It
/// therefore owns the #includes for that whole translation unit: all three of
/// the headers below reach unguarded files, so bar_put.cpp can no longer
/// include them for itself. kb/codegen/0129.)
///
/// Because this file shares a translation unit with bar_put.cpp, its
/// file-scope names are NOT file-local.
///
/// TH04 only. TH05 keeps this function in assembly, in
/// th04/main/hud/element_put.asm, which its MIDBOSSX_TEXT includes at the
/// HEAD of the segment rather than the tail — so lifting it there needs a
/// carve (kb/codegen/0080) rather than this seam, and the module stays.

#include "libs/master.lib/pc98_gfx.hpp"
#include "th04/gaiji/gaiji.h"
#include "th04/main/hud/hud.hpp"

// The two rows this function owns. hud_put() draws nothing on either, so
// unlike its sibling HUD_*_Y constants these stay private here.
//
// Macros rather than the house-style `static const`, because Turbo C++ 4.0J's
// __emit__() accepts literal constants only and the bar row is one of the
// arguments hand-pushed below. (kb/codegen/0089)
#define HUD_HP_CAPTION_Y 8
#define HUD_HP_BAR_Y 9

// Gaiji in a full-width HUD row, not counting the terminator. The ASM side
// spells this HUD_KANJI_W, in th02/main/hud/hud.inc.
static const int HUD_HP_KANJI_W = (HUD_TRAM_W / GAIJI_TRAM_W);

// One color per fill level of the bar, and the reason the bar is quantized
// into (HUD_HP_COLOR_COUNT - 1) steps.
static const int HUD_HP_COLOR_COUNT = 5;

// The blank row and the fill-level colors, both in th04/main/hud/hp[data].asm.
// Read through `struct` wrappers because the original copies each table into a
// stack local before using it, which only a local aggregate initializer
// produces. (kb/codegen/0084)
struct hack_hp_blank {
	char x[HUD_HP_KANJI_W + 1]; // ACTUAL TYPE: gaiji_th04_t[]
};
struct hack_hp_colors {
	uint8_t x[HUD_HP_COLOR_COUNT]; // ACTUAL TYPE: tram_atrb2[]
};

extern "C" struct hack_hp_blank gHUD_HP_BLANK;
extern "C" struct hack_hp_colors HUD_HP_COLORS;

// The "Enemy!!" caption, in th04/gaiji/hud[data].asm.
extern "C" char gsENEMY[];

void pascal hud_hp_put(int bar_value)
{
	const struct hack_hp_blank BLANK = gHUD_HP_BLANK;
	const struct hack_hp_colors COLORS = HUD_HP_COLORS;

	if(bar_value) {
		gaiji_putsa((HUD_LEFT + 5), HUD_HP_CAPTION_Y, gsENEMY, TX_YELLOW);

		// The original reached TH04's same-segment far hud_bar_put() through
		// `nop; push cs; call near ptr`, and no plain C++ far call reproduces
		// that (kb/codegen 0014 + 0083), so the arguments are pushed by hand
		// in pascal order.
		//
		// __emit__() rather than `_asm { push … }` for all three, and here
		// that is load-bearing beyond 0083's "the inline assembler is free to
		// pick the 3-byte form" argument: naming SI to the inline assembler
		// takes [bar_value] OUT of it. `_asm { push si; }` in this position
		// homes the parameter in DI instead, pushes both registers in the
		// prolog and costs 2 bytes. __emit__ is opaque to the allocator, and
		// `_asm { nop; push cs; call … }` names no general register, so the
		// parameter keeps SI.
		__emit__(0x6A, HUD_HP_BAR_Y);
		__emit__(0x56); // push si -- [bar_value]
		_AX = COLORS.x[bar_value / (BAR_MAX / (HUD_HP_COLOR_COUNT - 1))];
		__emit__(0x50); // push ax
		_asm { nop; push cs; call near ptr hud_bar_put; }
	} else {
		// The caption is blanked with the tail of the same row: 3 gaiji plus
		// the terminator, which is exactly what [gsENEMY] occupies.
		gaiji_putsa(
			(HUD_LEFT + 5), HUD_HP_CAPTION_Y, (BLANK.x + 5), TX_WHITE
		);
		gaiji_putsa(HUD_LEFT, HUD_HP_BAR_Y, BLANK.x, TX_WHITE);
	}
}
