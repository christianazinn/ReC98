/// TH05's stage-clear and all-clear bonus tallies
/// ----------------------------------------------
/// Four functions, in the original's address order: the two multiplier helpers
/// that scale a finished tally, then the two tallies themselves.
///
/// The tallies share a skeleton with TH04's functions of the same name and
/// nothing else: the frames are different sizes, every term between the opening
/// and the closing differs, and the two games reach their tally printer through
/// entirely different mechanisms (TH05 nopcalls a 4-argument renderer that takes
/// the text attribute, TH04 plainly calls a 3-argument one that hardcodes it).
/// So these bodies are TH05-only and deliberately not shared.
///
/// Not compiled on its own: th05/gather.cpp #includes this file above
/// th04/main/gather.cpp. Within one object code is emitted in source order, so
/// that include position is what keeps this file's *first* function at the front
/// of that object's contribution to main_032_TEXT, i.e. at its original address
/// immediately after the root dump's shrunken contribution. (kb/codegen/0112)

// kb/codegen/0129: `th04/gaiji/gaiji.h` has no include guard, so it may appear
// exactly ONCE in this object's include set. It is NOT included directly here:
// `th05/main/boss/boss.hpp` -> `th04/main/boss/boss.hpp` ->
// `th04/main/hud/overlay.hpp:6` already pulls it in, and adding the direct line
// back produces ~200 `Multiple declaration` errors rather than a silent
// mismatch. This TU is the first in the tree to need both gaiji and `boss`.
#include "libs/master.lib/pc98_gfx.hpp"
#include "th04/main/hud/hud.hpp"
#include "th04/main/playperf.hpp"
#include "th04/main/rank.hpp"
#include "th04/main/score.hpp"
#include "th04/main/stage/bonus.hpp"
#include "th04/main/stage/stage.hpp"
#include "th04/main/item/item.hpp"
#include "th04/main/language.hpp"
#include "th05/main/boss/boss.hpp"
#include "th05/resident.hpp"

extern "C" {
	// The nine tally labels are `dw offset a…` near-pointer *variables* in the
	// root dump, not the strings themselves, so the call sites push the word's
	// contents rather than an immediate offset. Reached through kb/codegen/0123
	// zero-byte aliases; no Shift-JIS byte is touched.
	extern const char near *ALL_CLEAR;
	extern const char near *BONUS_STAGE;
	extern const char near *BONUS_DREAM;
	extern const char near *GRAZEX50;
	extern const char near *PLAYER_REM;
	extern const char near *POINT_ITEMS;
	extern const char near *BONUS_NOMISS;
	extern const char near *BONUS_NOBOMB;
	extern const char near *POINT_TOTAL;
	extern const char near *BONUS_TOTAL;

	// These two *are* the arrays, hence the `offset` at their call sites.
	extern const gaiji_th04_t gpCLEAR_BONUS[];
	extern const gaiji_th04_t gpCONGRATULATION[];

	// No header declares TH05's life counter; the root dump publishes `_lives`
	// directly, with no alias needed.
	extern uint8_t lives;

	// The previous stage's [miss_count] and [bombs_used], latched by both
	// tallies below to decide the no-miss and no-bomb bonuses.
	// [placeholder names] Searched th04/, th05/ and every *.hpp for an existing
	// name for either: none exists, and neither is referenced outside this file,
	// so there is no call site to take a name from. Re-run and still failing as
	// of this parcel. They are plain `db 0` storage in the root dump's data
	// segment; lifting that storage is a data parcel, not this one.
	extern uint8_t byte_22274;
	extern uint8_t byte_22275;

	// The label for each multiplier row, indexed by the `desc` parameter of
	// stage_clear_bonus_multiplier_apply_and_put() below. Same `dw offset a…`
	// shape as the tally labels above, and the name is upstream's own — it is
	// already `public _STAGE_CLEAR_BONUS_DESC` in the root dump, so it is
	// adopted verbatim rather than coined. Each label ends in the multiplier it
	// selects, printed to one decimal place, which is where the `_tenths`
	// spelling below comes from:
	// (dump spellings, as the `dw offset` table lists them)
	//	0        aBOSS_FINAL_TIMEOUT       × 0.0
	//	1  - 3   aPENALTY_6 / _5 / _4      × 0.3 / 0.5 / 0.7  (starting lives)
	//	4  - 6   aPENALTY_CONT_1 / _2 / _3 × 0.8 / 0.6 / 0.4  (continues used)
	//	7  - 10  aBONUS_EASY / aBONUS_NORMAL / aBONUS_HARD / aBONUS_LUNATIC
	//	                                   × 0.5 / 1.0 / 1.2 / 1.4
	extern const char near *STAGE_CLEAR_BONUS_DESC[];
}

/// *Multiplies* the running tally by one multiplier given in tenths — it does
/// not add to it — and renders the row that explains it. Below × 1.0 the row is
/// a penalty and prints red; at or above it, a bonus, and prints green.
///
/// The two statements really are separate in the original: the product is
/// stored back through the far pointer and reloaded before the divide, which is
/// why the divide reloads only BX and keeps ES (kb/codegen/0002).
void pascal near stage_clear_bonus_multiplier_apply_and_put(
	int y, int desc, int multiplier_tenths, unsigned long far *points
)
{
	int col;

	*points *= multiplier_tenths;
	*points /= 10;

	col = ((multiplier_tenths < 10) ? TX_RED : TX_GREEN);
	language_main_clear_bonus_putsa(6, y, STAGE_CLEAR_BONUS_DESC[desc], col);
}

/// Applies every setting-dependent multiplier to a finished stage tally, in
/// three rows: the starting-lives penalty, the continue penalty, and the
/// per-rank scaling — so `rank` is the difficulty one, and `credit_lives` is
/// not. Failing to defeat the final boss instead zeroes the tally with a × 0.0
/// row and skips all three.
///
/// Each dispatch is deliberately a `switch` and not an `if`/`else if` chain:
/// Borland cross-jumps the identical `points` push and call out of every arm, so
/// all four arms of the rank dispatch share one call site, and the timeout arm
/// shares it too.
///
/// The case bodies are in ZUN's source order, which is NOT the order of the
/// compare chains: Turbo C++ sorts a sparse `switch`'s comparisons by ascending
/// case value while emitting the bodies where they were written, so the
/// descending 6/5/4 below is what produces an ascending 4/5/6 chain.
void pascal near stage_clear_bonus_multipliers_apply(unsigned long far *points)
{
	if(boss.phase_state.defeat_bonus == false) {
		stage_clear_bonus_multiplier_apply_and_put(20, 0, 0, points);
		return;
	}

	switch(resident->credit_lives) {
	case 6:	stage_clear_bonus_multiplier_apply_and_put(18, 1, 3, points);	break;
	case 5:	stage_clear_bonus_multiplier_apply_and_put(18, 2, 5, points);	break;
	case 4:	stage_clear_bonus_multiplier_apply_and_put(18, 3, 7, points);	break;
	}

	switch(score.continues_used) {
	case 1:	stage_clear_bonus_multiplier_apply_and_put(19, 4, 8, points);	break;
	case 2:	stage_clear_bonus_multiplier_apply_and_put(19, 5, 6, points);	break;
	case 3:	stage_clear_bonus_multiplier_apply_and_put(19, 6, 4, points);	break;
	}

	switch(rank) {
	case RANK_EASY:   	stage_clear_bonus_multiplier_apply_and_put(20,  7,  5, points);	break;
	case RANK_NORMAL: 	stage_clear_bonus_multiplier_apply_and_put(20,  8, 10, points);	break;
	case RANK_HARD:   	stage_clear_bonus_multiplier_apply_and_put(20,  9, 12, points);	break;
	case RANK_LUNATIC:	stage_clear_bonus_multiplier_apply_and_put(20, 10, 14, points);	break;
	}
}

/// kb/codegen/0083: Turbo C++ 4.0J emits `9A seg:off` for every far call to a
/// symbol outside the translation unit, and TLINK does not relax it even when
/// caller and callee sit in the same group. The original reaches both HUD
/// renderers through the same-group `nop` / `push cs` / `call near ptr` island,
/// so the call and its arguments have to be spelled out by hand.
///
/// kb/codegen/0122: with -3, each adjacent pair of 16-bit `pascal` arguments is
/// folded into one 32-bit push, and the *first* argument of the pair lands in
/// the *high* half. So (left, y) is one `66 68` push with y low and left high.
///
/// The 32-bit pushes rely on the widening store that immediately precedes them
/// leaving the term in EAX, which is exactly what -Z does and what the
/// original's bytes show. They also have to be spelled as bytes: Turbo C++
/// 4.02's inline assembler has no 32-bit register names, and rejects `push eax`
/// with the diagnostic "Undefined symbol 'eax'". `66 50` is `push eax`; a bare
/// `66` operand-
/// size prefix in front of a 16-bit memory push widens it to `push dword ptr`,
/// which keeps BASM computing [points]'s own frame offset instead of us
/// hardcoding one.
#define HUD_POINTS_PUT_EAX(left, y) _asm { \
	db	0x66, 0x68, y, 0x00, left, 0x00; \
	db	0x66, 0x50; \
	nop; push cs; call near ptr hud_points_put; \
}

#define HUD_POINTS_PUT_MEM(left, y) _asm { \
	db	0x66, 0x68, y, 0x00, left, 0x00; \
	db	0x66; \
	push	word ptr points; \
	nop; push cs; call near ptr hud_points_put; \
}

/// [bonus] is this function's one register variable, and the original pushes it
/// directly. Spelling that as `push si` would name SI to the inline assembler,
/// which reserves it and pushes [bonus] into DI instead — a second register
/// variable, so a second `push`/`pop` pair and a body two bytes too long
/// (kb/codegen/0083's "do not mention SI/DI"). `56` is the same instruction
/// without the register reference.
#define HUD_5_DIGIT_PUT_SI(left, y, atrb) _asm { \
	db	0x66, 0x68, y, 0x00, left, 0x00; \
	db	0x56; \
	push	atrb; \
	nop; push cs; call near ptr hud_5_digit_put; \
}

/// Both bonuses open by latching the previous stage's miss and bomb counts, and
/// the no-bomb bonus is additionally gated on the no-miss one — so a stage you
/// bombed through cannot pay a no-bomb bonus.
#define BONUS_LATCH_MISS_AND_BOMB() \
	no_miss = (resident->miss_count == byte_22274); \
	byte_22274 = resident->miss_count; \
	no_bomb = ((resident->bombs_used == byte_22275) && no_miss); \
	byte_22275 = resident->bombs_used;

void near stage_clear_bonus(void)
{
	unsigned long points;
	bool no_miss;
	bool no_bomb;
	unsigned int bonus;

	BONUS_LATCH_MISS_AND_BOMB();

	PaletteTone = 60;
	palette_show();

	gaiji_putsa(
		20, 4, reinterpret_cast<const char near *>(gpCLEAR_BONUS), TX_WHITE
	);
	language_main_clear_bonus_putsa( 6,  8, BONUS_STAGE, TX_WHITE);
	language_main_clear_bonus_putsa( 6, 10, BONUS_DREAM, TX_WHITE);
	language_main_clear_bonus_putsa( 6, 12, GRAZEX50,    TX_WHITE);
	language_main_clear_bonus_putsa( 6, 14, POINT_ITEMS, TX_WHITE);
	if(no_miss) {
		language_main_clear_bonus_putsa(6, 16, BONUS_NOMISS, TX_CYAN);
	}
	if(no_bomb) {
		language_main_clear_bonus_putsa(6, 17, BONUS_NOBOMB, TX_CYAN);
	}
	language_main_clear_bonus_putsa( 6, 21, BONUS_TOTAL, TX_WHITE);

	bonus = ((stage_id * 100) + 100);
	points = bonus;
	HUD_POINTS_PUT_EAX(34, 8);

	bonus = (dream * 10);
	points += bonus;
	HUD_POINTS_PUT_EAX(34, 10);

	bonus = (stage_graze * 5);
	points += bonus;
	HUD_POINTS_PUT_EAX(34, 12);

	// ZUN bloat: as in stage_allclear_bonus(), the point item count
	// *multiplies* the running total rather than adding to it.
	bonus = stage_point_items_collected;
	points = (bonus * points);
	HUD_5_DIGIT_PUT_SI(40, 14, TX_WHITE);

	// Both bonuses are worth the same, and it is computed once for both.
	bonus = ((stage_id * 5000) + 10000);
	if(no_miss) {
		points += bonus;
		HUD_POINTS_PUT_EAX(34, 16);
	}
	if(no_bomb) {
		points += bonus;
		HUD_POINTS_PUT_EAX(34, 17);
	}

	// Note the gap: a total between 200,001 and 499,999 moves playperf nowhere.
	if(points >= 1200000) {
		playperf_raise(4);
	} else if(points >= 800000) {
		playperf_raise(2);
	} else if(points >= 500000) {
		playperf_raise(1);
	} else if(points <= 100000) {
		playperf_lower(2);
	} else if(points <= 200000) {
		playperf_lower(1);
	}

	stage_clear_bonus_multipliers_apply(&points);
	HUD_POINTS_PUT_MEM(34, 21);

	score_delta += points;
}

void near stage_allclear_bonus(void)
{
	unsigned long points;
	unsigned long bonus_l;
	bool no_miss;
	bool no_bomb;
	unsigned int bonus;

	BONUS_LATCH_MISS_AND_BOMB();

	PaletteTone = 60;
	palette_show();
	extends_gained = 10;

	gaiji_putsa(
		19, 4, reinterpret_cast<const char near *>(gpCONGRATULATION), TX_WHITE
	);
	language_main_clear_bonus_putsa( 6,  6, ALL_CLEAR,   TX_WHITE);
	language_main_clear_bonus_putsa( 6,  8, BONUS_DREAM, TX_WHITE);
	language_main_clear_bonus_putsa( 6, 10, GRAZEX50,    TX_WHITE);
	language_main_clear_bonus_putsa( 6, 12, PLAYER_REM,  TX_WHITE);
	language_main_clear_bonus_putsa( 6, 14, POINT_ITEMS, TX_WHITE);
	if(no_miss) {
		language_main_clear_bonus_putsa(6, 16, BONUS_NOMISS, TX_CYAN);
	}
	if(no_bomb) {
		language_main_clear_bonus_putsa(6, 17, BONUS_NOBOMB, TX_CYAN);
	}
	language_main_clear_bonus_putsa( 6, 18, POINT_TOTAL, TX_CYAN);
	language_main_clear_bonus_putsa( 6, 21, BONUS_TOTAL, TX_WHITE);

	bonus = 1000;
	points = bonus;
	HUD_POINTS_PUT_EAX(34, 6);

	bonus = (dream * 10);
	points += bonus;
	HUD_POINTS_PUT_EAX(34, 8);

	bonus = (stage_graze * 5);
	points += bonus;
	HUD_POINTS_PUT_EAX(34, 10);

	bonus = ((lives * 1000) - 1000);
	points += bonus;
	HUD_POINTS_PUT_EAX(34, 12);

	// ZUN bloat: the point item count *multiplies* the running total rather
	// than adding to it, which is why a no-item all-clear scores nothing.
	bonus = stage_point_items_collected;
	points = (bonus * points);
	HUD_5_DIGIT_PUT_SI(40, 14, TX_WHITE);

	bonus_l = 50000;
	if(no_miss) {
		points += bonus_l;
		HUD_POINTS_PUT_EAX(34, 16);
	}
	if(no_bomb) {
		points += bonus_l;
		HUD_POINTS_PUT_EAX(34, 17);
	}

	bonus_l = (extend_point_items_collected * 250L);
	points += bonus_l;
	HUD_POINTS_PUT_EAX(34, 18);

	stage_clear_bonus_multipliers_apply(&points);
	HUD_POINTS_PUT_MEM(34, 21);

	score_delta += points;
}

// kb/codegen/0112 trap 3: this file's scope is merged into th05/gather.cpp's,
// so every macro it defines has to be taken back out again.
#undef BONUS_LATCH_MISS_AND_BOMB
#undef HUD_POINTS_PUT_EAX
#undef HUD_POINTS_PUT_MEM
#undef HUD_5_DIGIT_PUT_SI
