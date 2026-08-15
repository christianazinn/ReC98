/// Inserting the run's final score into the high score list
/// --------------------------------------------------------
/// MAINE.EXE's counterpart to MAIN.EXE's hiscore_continue_enter_raw()
/// (th04/main/hiscore.cpp): the same descending place search, the same
/// three-loop shift, and the same ZUN bug in the comparison — but reading the
/// finished run out of [resident] rather than the live [score], seeding the new
/// row's name with dots for regist_name_enter_menu() to overwrite instead of
/// "CONTINUE", taking the stage from [resident] too, and NOT saving: the caller
/// regist_menu() saves once the player has typed a name.
///
/// The name follows TH03's regist_score_enter_from_resident()
/// (th03/hiscore/regist.cpp), which is the same function in the same role and
/// with the same [resident] source. TH03's returns the place; this one stores
/// it in [entered_place], exactly as MAIN.EXE's version does.
///
/// `#if (GAME == 4)`-guarded ONLY because TH05's copy of this function is still
/// ASM, at the head of th05_maine.asm's own SCORE_TEXT root contribution
/// (`sub_B730`, `0A54:11F0`), immediately after th05/hi_end.cpp's object. An
/// unguarded body here would grow that object while that copy is still in the
/// dump, which is a guaranteed RED. Whether the two bodies are one source is
/// NOT measured yet — TH04 keeps the signed-promotion bug documented below and
/// th04/main/hiscore.cpp records that TH05's MAIN.EXE version fixes it by
/// comparing unsigned bytes, so expect at least one `#if (GAME == 5)` site
/// there rather than a clean rename.

#include "th04/end/end.h"
#include "th04/resident.hpp"

// ZUN bloat: Could have been local, exactly as in MAIN.EXE's version. Shared
// with regist_menu() and regist_row_put_at(), which is why it is not.
extern uint8_t entered_place;

void near regist_score_enter_from_resident(void)
{
	int c;
	int i;

	for(i = (SCOREDAT_PLACES - 1); i >= 0; i--) {
		for(c = (SCORE_DIGITS - 1); c >= 0; c--) {
			// ZUN bug: The same signed-promotion bug that
			// th04/main/hiscore.cpp documents at length for MAIN.EXE's
			// hiscore_continue_enter_raw(), in the same two comparisons: the
			// subtraction promotes the right-hand side to `int`, so a
			// gaiji-offsetted digit ≥96 that overflowed to 0 reads as
			// negative and sorts below every un-offsetted digit of the score
			// being entered. Reinforces the same soft limit of 959,999,999.
			if(resident->score_last.digits[c] >
				(hi.score.g_score[i].digits[c] - gb_0)
			) {
				break;
			}
			if(resident->score_last.digits[c] <
				(hi.score.g_score[i].digits[c] - gb_0)
			) {
				goto found_place;
			}
		}
	}
	entered_place = 0;
	goto shift;

found_place:
	if(i == (SCOREDAT_PLACES - 1)) {
		entered_place = -1; // ZUN bloat
		return;
	}
	entered_place = (i + 1);

	// ZUN bloat: memcpy(), again — the same three inner loops and the same
	// 24 multiplications and 16 bit shifts as MAIN.EXE's version.
shift:
	for(i = (SCOREDAT_PLACES - 2); i >= entered_place; i--) {
		for(c = (SCOREDAT_NAME_LEN - 1); c >= 0; c--) {
			hi.score.g_name[i + 1][c] = hi.score.g_name[i][c];
		}
		for(c = (SCORE_DIGITS - 1); c >= 0; c--) {
			hi.score.g_score[i + 1].digits[c] = hi.score.g_score[i].digits[c];
		}
		hi.score.g_stage[i + 1] = hi.score.g_stage[i];
	}

	// Blanked with dots rather than spaces, because regist_name_enter_menu()
	// starts the player on an all-dot name and unblits it a glyph at a time.
	for(c = (SCOREDAT_NAME_LEN - 1); c >= 0; c--) {
		hi.score.g_name[entered_place][c] = gs_DOT;
	}

	for(c = (SCORE_DIGITS - 1); c >= 0; c--) {
		hi.score.g_score[entered_place].digits[c] =
			(resident->score_last.digits[c] + gb_0);
	}

	// [end_sequence]'s ES_* values descend from 0xFF, so "≥ ES_EXTRA" is
	// "reached one of the three ending sequences", i.e. cleared the game.
	if(resident->end_sequence >= ES_EXTRA) {
		hi.score.g_stage[entered_place] = gs_ALL;
	} else {
		hi.score.g_stage[entered_place] = (gb_1 + resident->stage);
	}
}
