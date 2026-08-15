/// TH05's all-clear bonus tally
/// ---------------------------
/// Shares a skeleton with TH04's function of the same name and nothing else:
/// the frames are different sizes, every term between the opening and the
/// closing differs, and the two reach their tally printer through entirely
/// different mechanisms (TH05 nopcalls a 4-argument renderer that takes the
/// text attribute, TH04 plainly calls a 3-argument one that hardcodes it).
/// So this body is TH05-only and deliberately not shared.
///
/// Not compiled on its own: th05/gather.cpp #includes this file above
/// th04/main/gather.cpp. Within one object code is emitted in source order,
/// so that include position is what keeps this function at the front of that
/// object's contribution to main_032_TEXT, i.e. at its original address
/// immediately after the root dump's shrunken contribution. (kb/codegen/0112)

#include "libs/master.lib/pc98_gfx.hpp"
#include "th04/gaiji/gaiji.h"
#include "th04/main/hud/hud.hpp"
#include "th04/main/playperf.hpp"
#include "th04/main/score.hpp"
#include "th04/main/stage/bonus.hpp"
#include "th04/main/stage/stage.hpp"
#include "th04/main/item/item.hpp"
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

	// The previous stage's [miss_count] and [bombs_used], latched here and in
	// stage_clear_bonus() to decide the no-miss and no-bomb bonuses.
	// [placeholder names] Searched th04/, th05/ and every *.hpp for an existing
	// name for either: none exists, and neither is referenced outside these two
	// functions, so there is no call site to take a name from. stage_clear_bonus()
	// is still ASM and shares both, so they stay in the dump for now.
	extern uint8_t byte_22274;
	extern uint8_t byte_22275;

	// Adds the per-difficulty and per-continue penalty rows to the running
	// total, and renders each. [placeholder name] Its own body is still ASM and
	// reaches its rows through _STAGE_CLEAR_BONUS_DESC, which is equally
	// unnamed, so naming it is a separate parcel.
	void pascal near sub_16438(unsigned long far *points);
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
/// with `Undefined symbol 'eax'`. `66 50` is `push eax`; a bare `66` operand-
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
	text_putsa( 6,  8, BONUS_STAGE, TX_WHITE);
	text_putsa( 6, 10, BONUS_DREAM, TX_WHITE);
	text_putsa( 6, 12, GRAZEX50,    TX_WHITE);
	text_putsa( 6, 14, POINT_ITEMS, TX_WHITE);
	if(no_miss) {
		text_putsa(6, 16, BONUS_NOMISS, TX_CYAN);
	}
	if(no_bomb) {
		text_putsa(6, 17, BONUS_NOBOMB, TX_CYAN);
	}
	text_putsa( 6, 21, BONUS_TOTAL, TX_WHITE);

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

	sub_16438(&points);
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
	text_putsa( 6,  6, ALL_CLEAR,   TX_WHITE);
	text_putsa( 6,  8, BONUS_DREAM, TX_WHITE);
	text_putsa( 6, 10, GRAZEX50,    TX_WHITE);
	text_putsa( 6, 12, PLAYER_REM,  TX_WHITE);
	text_putsa( 6, 14, POINT_ITEMS, TX_WHITE);
	if(no_miss) {
		text_putsa(6, 16, BONUS_NOMISS, TX_CYAN);
	}
	if(no_bomb) {
		text_putsa(6, 17, BONUS_NOBOMB, TX_CYAN);
	}
	text_putsa( 6, 18, POINT_TOTAL, TX_CYAN);
	text_putsa( 6, 21, BONUS_TOTAL, TX_WHITE);

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

	sub_16438(&points);
	HUD_POINTS_PUT_MEM(34, 21);

	score_delta += points;
}

// kb/codegen/0112 trap 3: this file's scope is merged into th05/gather.cpp's,
// so every macro it defines has to be taken back out again.
#undef BONUS_LATCH_MISS_AND_BOMB
#undef HUD_POINTS_PUT_EAX
#undef HUD_POINTS_PUT_MEM
#undef HUD_5_DIGIT_PUT_SI
