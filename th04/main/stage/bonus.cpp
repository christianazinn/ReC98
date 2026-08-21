/// TH04's all-clear bonus tally
/// ----------------------------
/// TH04-ONLY, and that is measured rather than assumed. The `kb/codegen/0115`
/// compare in `state/notes/_stage_allclear_bonus_qv.md` found that this
/// function and TH05's function of the same name share their opening, their
/// accumulator idiom and their closing and nothing else: TH04 forks on
/// RANK_EXTRA twice where TH05 never mentions rank, reads [dream_score] where
/// TH05 computes [dream] * 10, has no no-miss/no-bomb bonus and no extend-item
/// term, and reaches its tally printer through six plain near calls where TH05
/// nopcalls a four-argument renderer nine times. th05/main/stage/bonus.cpp
/// therefore holds a different body under the same name, and this file is not
/// shared with it.
///
/// Not compiled on its own: th04/itminit.cpp #includes it AHEAD of
/// th04/main/item/init.cpp, which is the address order the two have in
/// `IT_UPDT_TEXT`. This proc was the last thing th04_main.asm contributed there
/// once items_init() left, so the object grows backwards into the hole and
/// every byte above it keeps its address (kb/codegen 0099 + 0112 + 0114) --
/// no carve, no new segment, no Tupfile.lua line.
///
/// `kb/codegen/0119` was checked rather than assumed, and does not bite: the
/// body is 0x185 = 389 bytes and ODD, but th04/itminit.cpp emits no
/// `-a2`-aligned data at all, which is the entry's first safe condition. The
/// items_init() lift one parcel earlier could NOT use th04/it_updt.cpp for the
/// mirrored reason -- that object emits two `switch` jump tables.

#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th04/gaiji/gaiji.h"
#include "th04/main/hud/hud.hpp"
#include "th04/main/item/item.hpp"
#include "th04/main/player/player.hpp"
#include "th04/main/rank.hpp"
#include "th04/main/score.hpp"
#include "th04/main/stage/bonus.hpp"
#include "th04/resident.hpp"

extern "C" {
	// The eight tally labels are the strings themselves, not pointers to
	// them -- TH04's call sites push `offset a…` immediates where TH05's push
	// the contents of a `dw offset` table, which is the entire reason the
	// Shift-JIS work in this parcel is on this game's side. Renamed in place
	// in th04_main.asm from IDA's `a…` spellings; the `_2` suffixes are IDA's
	// and are kept because they are true, four of these strings being byte
	// identical to ones the stage-clear tally uses out of separate storage.
	extern const char ALL_CLEAR[];
	extern const char POWERX50_2[];
	extern const char BONUS_DREAM_2[];
	extern const char GRAZEX50_2[];
	extern const char PLAYER_REM_10000[];
	extern const char PLAYER_REM_30000[];
	extern const char BONUS_POINT_2[];
	extern const char BONUS_TOTAL_2[];

	extern const gaiji_th04_t gpCONGRATULATION[];

	// [placeholder names] Both stay in th04_main.asm and both keep IDA's
	// spelling, each published with the one-line `public` form
	// (kb/codegen/0081). sub_1D48E prints one row of the tally and sub_1D5E9
	// applies the setting-dependent multipliers to the finished total -- so
	// TH05's counterpart of the second is already
	// stage_clear_bonus_multipliers_apply(). Naming either means reading a
	// body this parcel does not lift, and both are called by
	// stage_clear_bonus() above as well, which is the parcel that should name
	// them. Searched th04/ and every *.hpp for an existing name for either:
	// none exists, and neither is referenced outside this dump.
	void pascal near sub_1D48E(int y, unsigned long points);
	void pascal near sub_1D5E9(unsigned long far *points);
}

/// The tally printer takes the running total in EAX, which is where the term
/// just added still is; a plain call reloads it from the frame instead, so the
/// two pushes are emitted by hand. `66 50` is `push eax` -- Turbo C++ 4.02's
/// inline assembler has no 32-bit register names and rejects `push eax`
/// outright. `6A yy` is the `push` of the row's Y coordinate, which the
/// original encodes as an imm8. (kb/codegen/0122, one game over.)
#define TALLY_PUT_EAX(y) _asm { \
	db	0x6A, y; \
	db	0x66, 0x50; \
	call	near ptr sub_1D48E; \
}

/// The closing row reads the total back out of the frame instead, so a bare
/// `66` operand-size prefix widens BASM's 16-bit memory push to
/// `push dword ptr` -- which keeps BASM computing [points]'s own frame offset
/// rather than us hardcoding one.
#define TALLY_PUT_MEM(y) _asm { \
	db	0x6A, y; \
	db	0x66; \
	push	word ptr points; \
	call	near ptr sub_1D48E; \
}

/// hud_5_digit_put() is a `far` proc that happens to live in THIS segment, so
/// TASM shortens the dump's call to `push cs` + `call near` -- four bytes, and
/// with no `nop`, because the `nop` in that idiom only appears when TASM cannot
/// see the target's segment (kb/codegen 0014 + 0082). Turbo C++ cannot see it
/// either way and emits five bytes however the call is spelled, so the four go
/// in by hand. `66 68` pushes the folded 32-bit (left, y) pair the way the
/// original does, and `56` is `push si` written as a byte so that naming SI to
/// the inline assembler does not evict [bonus] into DI (kb/codegen/0083).
#define HUD_5_DIGIT_PUT_SI(left, y) _asm { \
	db	0x66, 0x68, y, 0x00, left, 0x00; \
	db	0x56; \
	push	cs; \
	call	near ptr hud_5_digit_put; \
}

void near stage_allclear_bonus(void)
{
	unsigned long points;
	unsigned int bonus;

	PaletteTone = 60;
	palette_show();
	extends_gained = 10;

	gaiji_putsa(
		19, 4, reinterpret_cast<const char near *>(gpCONGRATULATION), TX_WHITE
	);
	text_putsa( 6,  6, ALL_CLEAR,     TX_WHITE);
	text_putsa( 6,  8, POWERX50_2,    TX_WHITE);
	text_putsa( 6, 10, BONUS_DREAM_2, TX_WHITE);
	text_putsa( 6, 12, GRAZEX50_2,    TX_WHITE);

	// Extra's lives are worth three times as much, and the label has to say so.
	if(rank != RANK_EXTRA) {
		text_putsa(6, 14, PLAYER_REM_10000, TX_WHITE);
	} else {
		text_putsa(6, 14, PLAYER_REM_30000, TX_WHITE);
	}

	text_putsa( 6, 17, BONUS_POINT_2, TX_WHITE);
	text_putsa( 6, 21, BONUS_TOTAL_2, TX_WHITE);

	bonus = 1000;
	points = bonus;
	TALLY_PUT_EAX(6);

	// Every row is printed ×10 by the tally printer, which is why the labels
	// say × 50 for a term that multiplies by 5.
	bonus = (power * 5);
	points += bonus;
	TALLY_PUT_EAX(8);

	bonus = dream_score;
	points += bonus;
	TALLY_PUT_EAX(10);

	bonus = (stage_graze * 5);
	points += bonus;
	TALLY_PUT_EAX(12);

	// The life you are still playing on does not count, hence the subtraction.
	if(rank != RANK_EXTRA) {
		bonus = ((resident->rem_lives * 1000) - 1000);
	} else {
		bonus = ((resident->rem_lives * 3000) - 3000);
	}
	points += bonus;
	TALLY_PUT_EAX(14);

	// ZUN bloat: the point item count *multiplies* the running total rather
	// than adding to it, so an all-clear that collected no point items at all
	// scores nothing whatever the terms above came to.
	bonus = stage_point_items_collected;
	points = (bonus * points);
	HUD_5_DIGIT_PUT_SI(40, 17);

	sub_1D5E9(&points);
	TALLY_PUT_MEM(21);

	score_delta += points;
}

// kb/codegen/0112 trap 3: this file's scope is merged into th04/itminit.cpp's,
// so every macro it defines has to be taken back out again.
#undef TALLY_PUT_EAX
#undef TALLY_PUT_MEM
#undef HUD_5_DIGIT_PUT_SI
