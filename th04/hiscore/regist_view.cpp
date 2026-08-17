/// The high score table, as drawn by MAINE.EXE's name registration screen
/// ----------------------------------------------------------------------
/// regist_menu() redraws the same table that th04/hiscore/view.cpp draws in
/// OP.EXE, but with a *separate* pair of functions rather than that file's.
/// The two differ in exactly the ways the registration screen needs them to:
///
/// • OP.EXE's take the pixel coordinates their caller already computed; these
///   take [place] and the player character and compute their own, which is
///   why they can't share a body with the OP.EXE pair no matter how the
///   coordinates are factored.
/// • These highlight the row that the run just entered — the score digits
///   with a second sprite set, the stage gaiji with a different color.
///
/// ONE body for both games. TH05's copies sat at `th05_maine.asm`'s
/// SCORE_TEXT root-block head (score_put at `0A54:1359`, stage_put right
/// after it) and TH04's at `th04_maine.asm`'s (`0A05:24B6`); both blocks
/// followed hi_end.cpp's own contribution to the same segment, so this is a
/// kb/codegen/0098 head lift into that object in both games — no carve, no
/// new segment, no Tupfile.lua line, exactly as regist_enter.cpp before it.

#include "libs/master.lib/pc98_gfx.hpp"

/// Coordinates
/// -----------
/// [measured] The registration screen's table uses the *same* layout as
/// OP.EXE's viewer, in both games: every coordinate literal in the two dumps
/// (TH04 172 / 480 / 292 / 600 / 96 / 112, TH05 174 / 494 / 294 / 614 / 88 /
/// 96 / 224 / 232) falls out of the formulas below, which are character-for-
/// character th04/hiscore/view.cpp's.
///
/// Deliberately duplicated rather than shared through a header: that file is
/// an OP.EXE translation unit and defines several more of these constants
/// which MAINE.EXE never uses. Handing a matched translation unit a set of
/// unused `static const` definitions risks emitting storage for them, which
/// is a codegen hazard bought for no gain. Fold the two sets together only
/// behind an oracle run that grades OP.EXE as well.

static const pixel_t DIGIT_W = 16;
static const pixel_t NAME_W = (SCOREDAT_NAME_LEN * GAIJI_W);

// The optional 9th score digit is written into the name padding.
static const pixel_t NAME_PADDED_W = (NAME_W + ((GAME == 5) ? 6 : 4) + DIGIT_W);

static const pixel_t SCORE_PADDED_W = ((DIGIT_W * SCORE_DIGITS) + 8);
static const pixel_t STAGE_PADDED_W = (GAIJI_W + 8);
static const pixel_t COLUMN_W = (
	NAME_PADDED_W + SCORE_PADDED_W + STAGE_PADDED_W + ((GAME == 5) ? 10 : 0)
);

// These obviously only apply to the leftmost column.
static const screen_x_t NAME_LEFT = 8;
static const screen_x_t SCORE_LEFT = (NAME_LEFT + NAME_PADDED_W);
static const screen_x_t STAGE_LEFT = (SCORE_LEFT + SCORE_PADDED_W);

#if (GAME == 5)
static const screen_y_t TABLE_1_TOP = 88;
static const screen_y_t TABLE_2_TOP = 224;
static const pixel_t PLACE_1_PADDING_BOTTOM = 8;
#else
static const screen_y_t TABLE_TOP = 96;
static const pixel_t PLACE_1_PADDING_BOTTOM = GLYPH_H;
#endif

#define top_for_place(table_top, place) ( \
	((place) == 0) \
		? (table_top) \
		: ((table_top) + PLACE_1_PADDING_BOTTOM + ((place) * GLYPH_H)) \
)
/// -----------

/// Pattern numbers for the super_*() functions
/// -------------------------------------------
/// [measured] MAINE.EXE loads a second copy of the scnum.bft digit sprites 10
/// patnums after the first one, and draws the row that the current run just
/// entered with it. In TH05 this is the same set that th04/hiscore/view.cpp
/// declares as PAT_SCNUM_UNUSED and that OP.EXE indeed never draws; TH04's
/// OP.EXE doesn't even declare it, yet TH04's MAINE.EXE offsets by the same
/// 10. So "unused" is an OP.EXE-only property in both games.

/// [measured] `int`, not an `enum`: the parameter these are assigned to is a
/// byte in TH04 and an `int` in TH05, and the original computes the ternary
/// below in each game's destination width (`mov al, 0Ah` vs `mov ax, 000Ah`).
/// A byte-sized enum constant forces the narrow form in both.
// scnum.bft
// ---------
static const int PAT_SCNUM = 0;
static const int PAT_SCNUM_ENTERED = (PAT_SCNUM + 10);
// ---------
/// -------------------------------------------

typedef enum {
	COL_STAGE = ((GAME == 5) ? 2 : 12),
	COL_STAGE_ENTERED = 7,
	COL_SHADOW = 14,
} regist_table_colors_t;

/// [measured] score_put()'s player character parameter is NOT [playchar2]. It
/// is a full `int` in TH05, as [playchar2] would be — but TH04's original
/// zero-extends it (`mov dl, [bp+4]` / `mov dh, 0`) every time it reads it
/// back, and th04/playchar.h's `playchar_t` has no forced-unsigned member, so
/// Turbo C++ types that enum as `signed char` and would sign-extend with
/// `cbw`. Widening th04/playchar.h to fix that would re-lower every TH04
/// translation unit in the campaign for one function, so the unsigned byte
/// lives here instead. stage_put() is unaffected: both games pass it an `int`.
#if (GAME == 5)
typedef int playchar_and_patnum_t;
#else
typedef unsigned char playchar_and_patnum_t;
#endif

// ZUN bloat: Could have been calculated arithmetically, and without spelling
// out the player characters — the same bloat th04/hiscore/view.cpp's
// place_put() carries, and for the same table.

// Renders [place]'s score digits into [pc]'s column, highlighted if it is the
// row that regist_score_enter_from_resident() just created.
void pascal near score_put(int place, playchar_and_patnum_t pc)
{
	int digit;
	screen_x_t left;
	screen_y_t top;

#if (GAME == 5)
	switch(pc) {
	case PLAYCHAR_REIMU:
		left = (SCORE_LEFT + DIGIT_W + (0 * COLUMN_W));
		top = top_for_place(TABLE_1_TOP, place);
		break;
	case PLAYCHAR_MARISA:
		left = (SCORE_LEFT + DIGIT_W + (1 * COLUMN_W));
		top = top_for_place(TABLE_1_TOP, place);
		break;
	case PLAYCHAR_MIMA:
		left = (SCORE_LEFT + DIGIT_W + (0 * COLUMN_W));
		top = top_for_place(TABLE_2_TOP, place);
		break;
	case PLAYCHAR_YUUKA:
		left = (SCORE_LEFT + DIGIT_W + (1 * COLUMN_W));
		top = top_for_place(TABLE_2_TOP, place);
		break;
	}
#else
	top = top_for_place(TABLE_TOP, place);
	left = ((pc == PLAYCHAR_REIMU)
		? (SCORE_LEFT + DIGIT_W + (PLAYCHAR_REIMU * COLUMN_W))
		: (SCORE_LEFT + DIGIT_W + (PLAYCHAR_MARISA * COLUMN_W))
	);
#endif

	// ZUN bloat: Reusing the player character parameter as the patnum base,
	// rather than declaring a fourth local.
	pc = static_cast<playchar_and_patnum_t>(
		(((entered_place == place)
			&& (pc == static_cast<playchar_and_patnum_t>(playchar)))
			? PAT_SCNUM_ENTERED
			: PAT_SCNUM)
	);

	// The 9th digit, and the same [gb_0] sign-promotion trap that
	// th04/hiscore/view.cpp documents at length for OP.EXE's copy of this
	// code. It is inherited verbatim here: the subtraction promotes to `int`,
	// so a digit ≥96 that overflowed to 0 divides to a negative patnum and
	// super_put() corrupts VRAM with it.
	static_assert(SCORE_DIGITS == 8);
	if((hi.score.g_score[place].digits[SCORE_DIGITS - 1] - gb_0) >= 10) {
		super_put((left - (2 * DIGIT_W)), top, (
			((hi.score.g_score[place].digits[SCORE_DIGITS - 1] - gb_0) / 10) + pc
		));
	}
	super_put((left - DIGIT_W), top, (
		((hi.score.g_score[place].digits[SCORE_DIGITS - 1] - gb_0) % 10) + pc
	));

	digit = (SCORE_DIGITS - 2);
	while(digit >= 0) {
		super_put(
			left, top, (hi.score.g_score[place].digits[digit] + pc - gb_0)
		);
		digit--;
		left += DIGIT_W;
	}
}

/// kb/codegen/0096, and the one place this pair needs the object boundary to
/// be different from ZUN's. Turbo C++ pads a generated `jmp cs:` table to an
/// even offset **within the compiling object** only under `-a2`. In this
/// object both tables land on an odd natural offset (score_put's at 0x429,
/// stage_put's at 0x4EB, counted from th0N/hi_end.cpp's SCORE_TEXT
/// contribution) — and the original pads stage_put's and not score_put's. So
/// the pragma covers exactly one function. File-wide it would pad both.
#pragma option -a2

// Renders [place]'s stage gaiji into [pc]'s column, with the same drop shadow
// that OP.EXE's stage_put() uses, but in the name colors rather than a
// dedicated one.
void pascal near stage_put(
	int place, int pc, int gaiji // ACTUAL TYPE: gaiji_th04_t
)
{
	screen_x_t left;
	screen_y_t top;
	uint8_t col;

	// The cast is th04/playchar.h's signed `playchar_t` again; see
	// [playchar_and_patnum_t]. Without it TH04 sign-extends with `cbw`
	// where the original zero-extends. A no-op in TH05.
	col = (((entered_place == place)
		&& (pc == static_cast<unsigned char>(playchar)))
		? COL_STAGE_ENTERED
		: COL_STAGE
	);

#if (GAME == 5)
	switch(pc) {
	case PLAYCHAR_REIMU:
		left = (STAGE_LEFT + (0 * COLUMN_W));
		top = top_for_place(TABLE_1_TOP, place);
		break;
	case PLAYCHAR_MARISA:
		left = (STAGE_LEFT + (1 * COLUMN_W));
		top = top_for_place(TABLE_1_TOP, place);
		break;
	case PLAYCHAR_MIMA:
		left = (STAGE_LEFT + (0 * COLUMN_W));
		top = top_for_place(TABLE_2_TOP, place);
		break;
	case PLAYCHAR_YUUKA:
		left = (STAGE_LEFT + (1 * COLUMN_W));
		top = top_for_place(TABLE_2_TOP, place);
		break;
	}
#else
	top = top_for_place(TABLE_TOP, place);
	left = ((pc == PLAYCHAR_REIMU)
		? (STAGE_LEFT + (PLAYCHAR_REIMU * COLUMN_W))
		: (STAGE_LEFT + (PLAYCHAR_MARISA * COLUMN_W))
	);
#endif

	// Unlike OP.EXE's stage_put(), this one has no g_HISCORE_STAGE_EMPTY
	// branch: MAINE.EXE only ever reaches it with a real stage gaiji.
	graph_gaiji_putc((left + 2), (top + 2), gaiji, COL_SHADOW);
	graph_gaiji_putc(left, top, gaiji, col);
}

#pragma option -a1
