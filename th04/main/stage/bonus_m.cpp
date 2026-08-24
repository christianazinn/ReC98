/// The stage bonus tally's setting-dependent multipliers
/// -----------------------------------------------------
/// ONE BODY EACH WITH TH05, up to the two globals TH04 spells differently, and
/// the pair was named from th05/main/stage/bonus.cpp rather than coined here:
/// that game's stage_clear_bonus_multipliers_apply() and
/// stage_clear_bonus_multiplier_apply_and_put() are the same two functions in
/// the same slot, called by the same tally, over the same eleven
/// [STAGE_CLEAR_BONUS_DESC] labels in the same order. th04/main/stage/bonus.cpp
/// carried both under IDA placeholder names, and its own comment said that the
/// parcel which lifts them is the one that names them; this is it.
///
/// The two files are still separate, because the ROW ORDER differs: TH04 reads
/// [boss.phase_state] and [score.continues_used] out of flat MAIN globals where
/// TH05 goes through its own resident structure, and TH04's rank dispatch
/// compiles to a `cs:` jump table where TH05's does not.
///
/// This file deliberately has no #includes of its own: th04/hudnum.cpp holds
/// every header this object needs, and it shares a translation unit with
/// th04/main/hud/number_p.cpp (kb/codegen/0129).

extern "C" {
	// The label for each multiplier row, indexed by the `desc` parameter
	// below. Upstream's own name -- th04_main.asm already publishes
	// `_STAGE_CLEAR_BONUS_DESC`, so it is adopted verbatim rather than
	// coined, exactly as th05/main/stage/bonus.cpp adopted it. TH04's table is
	// `dd`, i.e. FAR pointers, where TH05's is `dw`; in the large model the
	// declaration below is already far, which is why the original's index
	// scales by 4 (`shl bx, 2`) and TH05's by 2.
	//
	//	0        aBOSS_FINAL_TIMEOUT       × 0.0
	//	1  - 3   aPENALTY_6 / _5 / _4      × 0.3 / 0.5 / 0.7  (starting lives)
	//	4  - 6   aPENALTY_CONT_1 / _2 / _3 × 0.8 / 0.6 / 0.4  (continues used)
	//	7  - 10  aBONUS_EASY / aBONUS_NORMAL / aBONUS_HARD / aBONUS_LUNATIC
	//	                                   × 0.5 / 1.0 / 1.2 / 1.4
	extern const char *STAGE_CLEAR_BONUS_DESC[];
}

/// *Multiplies* the running tally by one multiplier given in tenths -- it does
/// not add to it -- and renders the row that explains it. Below × 1.0 the row
/// is a penalty and prints red; at or above it, a bonus, and prints green.
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
	text_putsa(6, y, STAGE_CLEAR_BONUS_DESC[desc], col);
}

/// Applies every setting-dependent multiplier to a finished stage tally, in
/// three rows: the starting-lives penalty, the continue penalty, and the
/// per-rank scaling -- so `rank` is the difficulty one, and `credit_lives` is
/// not. Failing to defeat the final boss instead zeroes the tally with a × 0.0
/// row and skips all three. RANK_EXTRA falls out of the last dispatch entirely,
/// which is what the `ja` above the jump table is.
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
