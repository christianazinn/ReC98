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
/// followed hi_end.cpp's own contribution to the same segment in the classic
/// build. The merged product includes both lifts from th0N/regist.cpp.

#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/hiscore/regist.h"
#if (GAME == 5)
#include "th04/hardware/bgimage.hpp"
#else
/// [measured] TH04's MAINE.EXE links neither th04/hardware/bgimage.cpp's
/// unblitter nor th04/egcrect.cpp — Tupfile.lua gives the latter to TH04's
/// OP.EXE only. It keeps this screen's background on VRAM page 1 instead, and
/// unblits through its own `near` page-1-to-page-0 EGC rectangle copy, which
/// takes the same four parameters as bgimage_put_rect_16() and still sits in
/// ASM at the very END of this same SCORE_TEXT contribution — after
/// regist_menu(), so a head lift cannot reach it.
///
/// `[measured]` Its contract is exactly th04/hardware/egcrect.cpp's
/// egc_copy_rect_1_to_0_16(): it starts the EGC itself, copies the rectangle
/// 16 pixels at a time through plane B alone, and turns the EGC back off. Its
/// *body* is not — it is a plain-C double loop with a stack-allocated VRAM
/// offset, closer to th02/hardware/grp_rect.cpp's
/// graph_copy_rect_1_to_0_16() (which in turn is not this contract: it copies
/// all four planes explicitly and never touches the EGC). So TH04 has two
/// different bodies for one operation, and the `_near` suffix is what keeps
/// them apart — this is MAINE.EXE's own `near` one. Rename it freely once it
/// is decompiled; it has exactly one caller, below.
extern "C" void pascal near egc_copy_rect_1_to_0_16_near(
	screen_x_t left, vram_y_t top, pixel_t w, pixel_t h
);
#endif

// master.lib's GRCG_OFF_CLOBBERING macro, which spills the port number to DX
// instead of using the immediate-port form that _outportb_() would emit.
// Same spelling as th04/main/stage/loop.cpp's.
#define grcg_off_clobbering_dx() outportb(0x7C, GC_OFF)

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

// The name's drop shadow, in both games.
static const pixel_t SHADOW_OFFSET = 2;

#if (GAME == 5)
// name_put() unblits the name *and* its drop shadow before redrawing both.
static const pixel_t NAME_UNBLIT_W = (NAME_W + SHADOW_OFFSET);
static const pixel_t NAME_UNBLIT_H = (GLYPH_H + SHADOW_OFFSET);

/// [measured] The frame TH05 draws around the row being registered, relative
/// to that column's [NAME_LEFT] and that row's top: 2 pixels of padding to the
/// left of the name, 1 above it, GLYPH_H below it, and a right edge 4 pixels
/// past the right edge of the stage gaiji.
static const pixel_t FRAME_PADDING_LEFT = 2;
static const pixel_t FRAME_PADDING_TOP = 1;
static const pixel_t FRAME_RIGHT_REL = (
	(STAGE_LEFT - NAME_LEFT) + GAIJI_W + 4
);
#else
/// [measured] TH04's name column has its own horizontal geometry, and it does
/// not line up with the score and stage columns above: the leftmost name
/// starts at pixel 16 rather than at NAME_LEFT (8), and the two columns are
/// 304 pixels apart rather than COLUMN_W (308). Nor does it line up with
/// th04/hiscore/view.cpp's own `COLUMN_W + 4` ZUN bug, which happens to put
/// the *second* column at the same pixel 320 and the first one 8 pixels
/// further left. Vertically it is exactly top_for_place().
static const screen_x_t NAME_ROW_LEFT = 16;
static const pixel_t NAME_ROW_COLUMN_W = 304;

/// [measured] …and both of its functions need it in TRAM cells as well,
/// because TH04 draws the name row's *foreground* into text RAM rather than
/// onto the graphics plane. The two disagree about which unit is primary:
/// name_put() holds cells and multiplies up for the drop shadow, place_put()
/// holds pixels and divides back down. Both conversions are in the original.
static const tram_x_t NAME_ROW_TRAM_LEFT = (NAME_ROW_LEFT / GLYPH_HALF_W);
static const tram_cell_amount_t NAME_ROW_TRAM_COLUMN_W = (
	NAME_ROW_COLUMN_W / GLYPH_HALF_W
);
static const tram_y_t TABLE_TRAM_TOP = (TABLE_TOP / GLYPH_H);
static const tram_cell_amount_t PLACE_1_TRAM_PADDING_BOTTOM = (
	PLACE_1_PADDING_BOTTOM / GLYPH_H
);

#define tram_top_for_place(place) ( \
	((place) == 0) \
		? TABLE_TRAM_TOP \
		: (TABLE_TRAM_TOP + PLACE_1_TRAM_PADDING_BOTTOM + (place)) \
)
#endif

/// [measured] The name entry alphabet, in TRAM cells. Same table
/// (th04/hiscore/alphabet[data].asm) and the same 3×17 shape as TH02's, which
/// is why both dumps `include th02/hiscore/regist.inc` for the dimensions;
/// only the row differs between the games.
static const tram_x_t ALPHABET_LEFT = 23;
static const tram_y_t ALPHABET_TOP = ((GAME == 5) ? 21 : 18);

extern const unsigned char gALPHABET[ALPHABET_ROWS][ALPHABET_COLS];
/// -----------

#define scoredat_name(place) \
	reinterpret_cast<const char far *>(hi.score.g_name[place])

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

/// [measured] The row being registered gets its own name treatment in both
/// games, but only TH05 keeps it on the graphics plane and therefore has a
/// *color* for it — TH04 moves it to text RAM, and its TRAM attributes are
/// spelled out at the call sites, exactly as th02/hiscore/regist.cpp's
/// scoredat_name_puts() spells out its own TX_GREEN pair.
typedef enum {
	COL_NAME = ((GAME == 5) ? 2 : 12),
	COL_NAME_ENTERED = 6, // GAME == 5 only
	COL_NAME_CURSOR = 7, // GAME == 5 only
	COL_STAGE = ((GAME == 5) ? 2 : 12),
	COL_STAGE_ENTERED = 7,
	COL_SHADOW = 14,
} regist_table_colors_t;

/// [measured] score_put()'s player character parameter is NOT [playchar2]. It
/// is a full `int` in TH05, as [playchar2] would be — but TH04's original
/// zero-extends the low argument byte every time it reads it back, and
/// th04/playchar.h's `playchar_t` has no forced-unsigned member, so Turbo C++
/// types that enum as `signed char` and would sign-extend it. Widening
/// th04/playchar.h to fix that would re-lower every TH04
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

/// The direct descendant of TH02's scoredat_name_puts()
/// (th02/hiscore/regist.cpp): the same "redraw one name row, then redraw the
/// one glyph the cursor sits on in a highlighted attribute" shape, called from
/// regist_name_enter_menu() after every cursor move and every glyph commit.
/// Unlike TH02's, it also has to unblit the previous name first, because the
/// glyphs it overwrites are not blanks.
///
/// This is where the two games stop sharing anything but the shadow:
///
/// • TH04 puts the name's foreground into TEXT RAM (gaiji_putsa() /
///   gaiji_putca(), TRAM cell coordinates, TRAM attributes), and only its drop
///   shadow onto the graphics plane. So it only ever unblits the shadow.
/// • TH05 puts everything onto the graphics plane (graph_gaiji_puts() /
///   graph_gaiji_putc(), pixel coordinates, palette colors), unblits name and
///   shadow together, and additionally frames the entire row — name, score and
///   stage — with four GRCG lines.
///
/// The score and stage columns of the same row are drawn by score_put() and
/// stage_put() above, in both games.
void pascal near name_put(int place, playchar_t pc, unsigned char cursor)
{
#if (GAME == 5)
	screen_x_t left;
	screen_y_t top;

	switch(pc) {
	case PLAYCHAR_REIMU:
		left = (NAME_LEFT + (0 * COLUMN_W));
		top = top_for_place(TABLE_1_TOP, place);
		break;
	case PLAYCHAR_MARISA:
		left = (NAME_LEFT + (1 * COLUMN_W));
		top = top_for_place(TABLE_1_TOP, place);
		break;
	case PLAYCHAR_MIMA:
		left = (NAME_LEFT + (0 * COLUMN_W));
		top = top_for_place(TABLE_2_TOP, place);
		break;
	case PLAYCHAR_YUUKA:
		left = (NAME_LEFT + (1 * COLUMN_W));
		top = top_for_place(TABLE_2_TOP, place);
		break;
	}

	bgimage_put_rect_16(left, top, NAME_UNBLIT_W, NAME_UNBLIT_H);
	graph_gaiji_puts(
		(left + SHADOW_OFFSET),
		(top + SHADOW_OFFSET),
		GAIJI_W,
		scoredat_name(place),
		COL_SHADOW
	);
	graph_gaiji_puts(
		left, top, GAIJI_W, scoredat_name(place), COL_NAME_ENTERED
	);
	graph_gaiji_putc(
		(left + (cursor * GAIJI_W)),
		top,
		hi.score.g_name[place][cursor],
		COL_NAME_CURSOR
	);

	// The cursor's underline, and then the frame around the whole row.
	grcg_setcolor(GC_RMW, COL_NAME_CURSOR);
	grcg_hline(
		(left + (cursor * GAIJI_W)),
		(left + (cursor * GAIJI_W) + GAIJI_W),
		(top + GLYPH_H - 1)
	);
	grcg_vline(
		(left - FRAME_PADDING_LEFT),
		(top - FRAME_PADDING_TOP),
		(top + GLYPH_H)
	);
	grcg_vline(
		(left + FRAME_RIGHT_REL),
		(top - FRAME_PADDING_TOP),
		(top + GLYPH_H)
	);
	grcg_hline(
		(left - FRAME_PADDING_LEFT),
		(left + FRAME_RIGHT_REL),
		(top - FRAME_PADDING_TOP)
	);
	grcg_hline(
		(left - FRAME_PADDING_LEFT),
		(left + FRAME_RIGHT_REL),
		(top + GLYPH_H)
	);
	grcg_off_clobbering_dx();
#else
	tram_x_t left;
	tram_y_t top;

	left = ((pc == PLAYCHAR_REIMU)
		? (NAME_ROW_TRAM_LEFT + (PLAYCHAR_REIMU * NAME_ROW_TRAM_COLUMN_W))
		: (NAME_ROW_TRAM_LEFT + (PLAYCHAR_MARISA * NAME_ROW_TRAM_COLUMN_W))
	);
	top = tram_top_for_place(place);

	egc_copy_rect_1_to_0_16_near(
		((left * GLYPH_HALF_W) + SHADOW_OFFSET),
		((top * GLYPH_H) + SHADOW_OFFSET),
		NAME_W,
		GLYPH_H
	);
	graph_gaiji_puts(
		((left * GLYPH_HALF_W) + SHADOW_OFFSET),
		((top * GLYPH_H) + SHADOW_OFFSET),
		GAIJI_W,
		scoredat_name(place),
		COL_SHADOW
	);
	gaiji_putsa(left, top, scoredat_name(place), TX_RED);
	gaiji_putca(
		(left + (cursor * GAIJI_TRAM_W)),
		top,
		hi.score.g_name[place][cursor],
		(TX_RED | TX_REVERSE)
	);
#endif
}

/// One whole row of the table: name, score digits, stage gaiji. MAINE.EXE's
/// counterpart to th04/hiscore/view.cpp's place_put(), and the same ZUN bloat
/// — TH05 spells out all four player characters in a `switch` where two
/// multiplications would do, TH04 spells out its two in a ternary.
///
/// The row that regist_score_enter_from_resident() just created is the one
/// interesting difference between the games, and it runs the other way round
/// from everything else in this file: **TH05 skips its name entirely** and
/// leaves it to name_put(), which is drawing it on the graphics plane a glyph
/// at a time; **TH04 draws it here**, but into text RAM, matching what its own
/// name_put() puts there. TH04 therefore always lays down the graphics-plane
/// drop shadow first and only then decides which plane the foreground goes on
/// — which is why its shadow is unconditional and TH05's is not.
///
/// The two also disagree about statement order: TH05 draws the score and the
/// stage before the name, TH04 after.
void pascal near place_put(int place, playchar_and_patnum_t pc)
{
	// Declared in this order because the original keeps [top] in DI and
	// spills [left]; Turbo C++ hands the one free register to whichever of
	// the two is declared first.
	screen_y_t top;
	screen_x_t left;

#if (GAME == 5)
	switch(pc) {
	case PLAYCHAR_REIMU:
		left = (NAME_LEFT + (0 * COLUMN_W));
		top = top_for_place(TABLE_1_TOP, place);
		break;
	case PLAYCHAR_MARISA:
		left = (NAME_LEFT + (1 * COLUMN_W));
		top = top_for_place(TABLE_1_TOP, place);
		break;
	case PLAYCHAR_MIMA:
		left = (NAME_LEFT + (0 * COLUMN_W));
		top = top_for_place(TABLE_2_TOP, place);
		break;
	case PLAYCHAR_YUUKA:
		left = (NAME_LEFT + (1 * COLUMN_W));
		top = top_for_place(TABLE_2_TOP, place);
		break;
	}

	score_put(place, pc);
	stage_put(place, pc, hi.score.g_stage[place]);

	if((playchar != pc) || (entered_place != place)) {
		graph_gaiji_puts(
			(left + SHADOW_OFFSET),
			(top + SHADOW_OFFSET),
			GAIJI_W,
			scoredat_name(place),
			COL_SHADOW
		);
		graph_gaiji_puts(left, top, GAIJI_W, scoredat_name(place), COL_NAME);
	}
#else
	left = ((pc == PLAYCHAR_REIMU)
		? (NAME_ROW_LEFT + (PLAYCHAR_REIMU * NAME_ROW_COLUMN_W))
		: (NAME_ROW_LEFT + (PLAYCHAR_MARISA * NAME_ROW_COLUMN_W))
	);
	top = top_for_place(TABLE_TOP, place);

	graph_gaiji_puts(
		(left + SHADOW_OFFSET),
		(top + SHADOW_OFFSET),
		GAIJI_W,
		scoredat_name(place),
		COL_SHADOW
	);

	// The cast is th04/playchar.h's signed `playchar_t` again; see
	// [playchar_and_patnum_t].
	if((entered_place != place) ||
		(pc != static_cast<playchar_and_patnum_t>(playchar))
	) {
		graph_gaiji_puts(left, top, GAIJI_W, scoredat_name(place), COL_NAME);
	} else {
		// ZUN bloat: name_put() already has both coordinates in cells.
		gaiji_putsa(
			(left / GLYPH_HALF_W),
			(top / GLYPH_H),
			scoredat_name(place),
			TX_RED
		);
	}

	score_put(place, pc);
	stage_put(place, pc, hi.score.g_stage[place]);
#endif
}

#pragma option -a1

// ZUN bloat: th04/hiscore/view.cpp's TH04 rank_render() moves the first and
// last iterations out of the equivalent loop. This one doesn't, in either game.
void pascal near places_put(playchar_and_patnum_t pc)
{
	int place;

	for(place = 0; place < SCOREDAT_PLACES; place++) {
		place_put(place, pc);
	}
}

// Character-for-character TH02's alphabet_putca()
// (th02/hiscore/regist.cpp), down to the parameter order and the same
// "ZUN bloat: Should use the function throughout" macro at its call sites.
void pascal near alphabet_putca(int col, int row, tram_atrb2 atrb)
{
	gaiji_putca(
		(ALPHABET_LEFT + (col * GAIJI_TRAM_W)),
		(ALPHABET_TOP + row),
		gALPHABET[row][col],
		atrb
	);
}
