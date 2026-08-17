/// Full HUD
/// --------
/// One name, two bodies — and only TH04's is here. The row *set* is almost
/// the same, but the two games agree about nothing else: TH04 re-reads the
/// player character out of the resident structure and branches on it three
/// separate times, in between the rows that don't depend on it, while TH05
/// switches on [playchar] exactly once and draws all three
/// character-dependent labels from the same arm. TH05 additionally labels the
/// cumulative point item row, and moves the point item, graze and dream rows
/// around.
///
/// **TH05's hud_put() is deliberately still ASM** (`th05_main.asm`). Its
/// `switch(playchar)` compiles to a jump table that the original places one
/// byte later than Turbo C++ puts it in any translation unit that starts with
/// hud_put(), because the compiler aligns that table to an even offset
/// *within the object's code segment* and ZUN's object began 0x35B bytes
/// earlier than ours can. See state/notes/hud_put.md and kb/codegen/0093.
///
/// Every call from here into one of the per-row renderers has to be spelled
/// out in inline ASM: they all sit in other segments of the same main_01
/// group, and the original reaches them through TASM's 4-byte `push cs` +
/// near call, which no C++ far call reproduces (kb/codegen/0083).

#pragma option -zPmain_01

#include "libs/master.lib/pc98_gfx.hpp"
#include "th01/rank.h"
#include "th04/gaiji/gaiji.h"
#include "th04/main/hud/hud.hpp"
#include "th04/main/rank.hpp"
#include "th04/playchar.h"
#include "th04/resident.hpp"

// Gaiji strings
// -------------
// ZUN bloat: Same needlessly consistent length as TH02's, which again rules
// out any compile-time length calculation.

extern const gaiji_th04_t gRANKS[RANK_COUNT][8];

extern const gaiji_th04_t gsSCORE[5];
extern const gaiji_th04_t gsHISCORE[5];
extern const gaiji_th04_t gsREIGEKI[5];
extern const gaiji_th04_t gsREIMU[5];
extern const gaiji_th04_t gsREIRYOKU[5];
extern const gaiji_th04_t gsBOMB[5];
extern const gaiji_th04_t gsPLAYER[5];
extern const gaiji_th04_t gsPOWER[5];

static const tram_cell_amount_t gsSCORE_W = (3 * GAIJI_TRAM_W);
static const tram_cell_amount_t gsHISCORE_W = (4 * GAIJI_TRAM_W);
// -------------

// Coordinates
// -----------
// TH02's own rows live in th02/main/hud/hud.hpp; only the columns and the two
// score rows are shared with this game.

static const tram_x_t HUD_CENTER_X = (HUD_LEFT + (HUD_TRAM_W / 2));
static const tram_x_t HUD_LABEL_LEFT = (HUD_LEFT + HUD_LABEL_PADDING);

// The single-gaiji icon in front of a counter or bar row.
static const tram_x_t HUD_ICON_LEFT = (HUD_LEFT + 2);

// [HUD_BOMBS_Y] (11) and [HUD_LIVES_Y] (13) moved to th04/main/hud/hud.hpp
// when their renderers were lifted into their own translation unit, joining
// the three rows in between that were already shared for the same reason:
// [HUD_POINT_ITEMS_STAGE_Y] (15), [HUD_DREAM_Y] (17) and [HUD_GRAZE_Y] (19).
static const tram_y_t HUD_POWER_LABEL_Y = 21;
static const tram_y_t HUD_RANK_Y = 23;
// -----------

inline void hud_label_put(utram_x_t left, utram_y_t y, const gaiji_th04_t* s) {
	gaiji_putsa(left, y, reinterpret_cast<const char *>(s), TX_YELLOW);
}

inline void hud_icon_put(utram_x_t left, utram_y_t y, int gaiji) {
	gaiji_putca(left, y, gaiji, TX_YELLOW);
}

// ZUN bloat: [playchar] holds the same information, without the far
// dereference that this repeats at all three character-dependent labels.
#define playchar_is_reimu() \
	(resident->playchar_ascii == ('0' + PLAYCHAR_REIMU))

extern "C" void pascal hud_put(void)
{
	hud_label_put(
		(HUD_CENTER_X - (gsHISCORE_W / 2)), (HUD_HISCORE_Y - 1), gsHISCORE
	);
	hud_label_put(
		(HUD_CENTER_X - (gsSCORE_W / 2)), (HUD_SCORE_Y - 1), gsSCORE
	);
	hud_score_put();

	if(playchar_is_reimu()) {
		hud_label_put(HUD_LABEL_LEFT, HUD_BOMBS_Y, gsREIGEKI);
	} else {
		hud_label_put(HUD_LABEL_LEFT, HUD_BOMBS_Y, gsBOMB);
	}
	_asm { push cs; call near ptr hud_bombs_put; }
	_asm { push cs; call near ptr hud_lives_put; }

	if(playchar_is_reimu()) {
		hud_label_put(HUD_LABEL_LEFT, HUD_LIVES_Y, gsREIMU);
	} else {
		hud_label_put(HUD_LABEL_LEFT, HUD_LIVES_Y, gsPLAYER);
	}

	hud_icon_put(HUD_ICON_LEFT, HUD_POINT_ITEMS_STAGE_Y, gs_TEN);
	_asm { push cs; call near ptr hud_point_items_put; }

	hud_icon_put(HUD_ICON_LEFT, HUD_DREAM_Y, gs_YUME);
	_asm { push cs; call near ptr hud_dream_put; }

	hud_icon_put(HUD_ICON_LEFT, HUD_GRAZE_Y, gs_TAMA);
	_asm { push cs; call near ptr hud_graze_put; }

	if(playchar_is_reimu()) {
		hud_label_put(HUD_LABELED_LEFT, HUD_POWER_LABEL_Y, gsREIRYOKU);
	} else {
		hud_label_put(HUD_LABELED_LEFT, HUD_POWER_LABEL_Y, gsPOWER);
	}
	_asm { push cs; call near ptr hud_power_put; }

	gaiji_putsa(
		HUD_LABEL_LEFT,
		HUD_RANK_Y,
		reinterpret_cast<const char near *>(gRANKS[rank]),
		(
			(rank ==    RANK_EASY) ? TX_GREEN :
			(rank ==  RANK_NORMAL) ? TX_CYAN :
			(rank ==    RANK_HARD) ? TX_MAGENTA :
			/*       RANK_LUNATIC */ TX_RED
		)
	);

	// `push 0`, then the same hand-spelled call as above. __emit__() rather
	// than `_asm { push 0 }`, because the inline assembler is free to pick
	// the 3-byte `68 imm16` form. (kb/codegen/0083)
	__emit__(0x6A, 0);
	_asm { push cs; call near ptr hud_hp_put; }
}
