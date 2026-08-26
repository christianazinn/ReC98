/// The verdict screen's body
/// -------------------------
/// Twelve labels, their values, the [skill] computation and its clamp, then
/// the two `_ude.txt` comment records. TH05 stops there: the wait and the
/// fade-out live in verdict_animate(), which is why this is
/// verdict_stats_put() and TH04's is verdict_stats_put_and_wait().
///
/// `[measured]` th04/end/verdict_stats.cpp predicted a shared body was "the
/// pairing to try when that copy is lifted". **Measured, it is not.** The two
/// share their label list, their row order and the shape of the five
/// percentage rows, and nothing else:
///
/// - every coordinate is `[verdict_left]`/`[verdict_top]`-relative here and a
///   hardcoded immediate in TH04, because TH05 runs these same renderers a
///   second time from staffroll_animate() against a different origin;
/// - TH05 zeroes [skill] on entry and divides by **12**, not 5, and then adds
///   [skill_quarter] back — the mechanism th04/end/verdict_digits.cpp
///   documents as finished only in TH05;
/// - the score bonus reads **`score_highest`**, not `score_last`, tests `>= 10`
///   rather than `>= 9`, and has no `- 3` offset;
/// - three of the five rank cases scale [skill] by a fraction before adding,
///   which TH04 has nowhere;
/// - the completion row keys on `end_sequence >= ES_BAD`, not `== ES_GOOD`;
/// - every constant in the credits/bombs/Turbo/misses/bombs-used chain
///   differs, and the Turbo penalty moves [skill_max] too;
/// - TH05 reads TWO `_ude.txt` records and TH04 reads one.
///
/// An `#if (GAME == 5)` body would be a different function inside a shared
/// wrapper. Both files now say so from their own side.
///
/// `[measured]` This was the LAST proc of th05_maine.asm's SCORE_TEXT block
/// and, after skill_apply_and_graph_guts() and verdict_comment_2_num() came
/// out of the head, its ONLY one — so it comes out through the head as well,
/// appended to the tail of th05/regist.cpp's contribution
/// (kb/codegen/0098 + 0114). **That emptied the block.** No carve, no new
/// segment, no group-list edit and no Tupfile.lua line.
///
/// `[measured]` The two `jmp cs:` tables Turbo C++ generates from the rank and
/// continues switches are emitted after the epilogue and came out of the dump
/// with the body. They need NO `#pragma option -a2`, unlike TH04's pair: the
/// original puts the first at an ODD offset, so the `-a1` state
/// th05/regist.cpp restores is what reproduces it, and `-a2` would insert a
/// pad the original does not have.
///
/// `[measured]` Lifting this body also removed one of upstream's own hand-
/// encoded instructions: a three-byte `db` stood in for the near jump out of
/// the RANK_NORMAL arm, under a comment reading "No idea why TASM can't
/// assemble this properly after scoredat_load_for() was decompiled". Turbo
/// C++ emits those three bytes itself. One more of them is still live inside
/// staffroll_animate(), for a backward conditional jump.
///
/// No `#include`s of its own; th05/regist.cpp's chain provides all of
/// th05/resident.hpp, th01/rank.h, th04/end/end.h, th04/common.h,
/// th04/gaiji/gaiji.h, th04/hardware/grppsafx.h and master.lib, and the two
/// files ahead of this one in that chain already declare [verdict_left],
/// [verdict_top], [skill], VERDICT_COL and every renderer called below.

/// `[measured]` The colour of the free-text lines, distinct from
/// [verdict_col], which the numbers use. Published beside it by a zero-byte
/// `label word` alias in th05_maine.asm (kb/codegen/0123).
extern "C" vc2 verdict_comment_col;

/// `[measured]` Held in its own `.data?` byte rather than recomputed, because
/// the rank-dependent bonus below needs it again after RANK_EXTRA has been
/// substituted for the Extra Stage — and because staffroll_animate(), still
/// ASM, reads it too, which is why the dump keeps the storage and merely
/// publishes it.
extern "C" unsigned char verdict_rank;

/// The five 7-cell, right-aligned gaiji rank names, defined in
/// th04/gaiji/verdict[data].asm and reached through the zero-byte
/// `label byte` alias that file publishes over them (kb/codegen/0123). The
/// alias exists because TH04's dump still spells the table by its IDA name;
/// this parcel removed TH05's last reference to that spelling, so on this
/// side the alias is now the only name in use.
extern "C" const unsigned char gbRANKS[RANK_COUNT][8];

/// The two 30-byte `_ude.txt` records, and the file they come out of. The
/// records stay `.data?` bytes of the dump because th05/end/verdict_comment.cpp
/// renders them from the OTHER object growing into this segment.
extern "C" shiftjis_t verdict_comment_1[];
extern "C" shiftjis_t verdict_comment_2[];
extern "C" const char ude_txt[];

/// `[measured]` All of these stay `_DATA` bytes of the root dump rather than
/// becoming literals of this translation unit, for the reason
/// th04/end/verdict_digits.cpp documents at length: the build compiles with
/// `-d`, and the dump holds byte-identical duplicate pairs — 「回」 twice
/// (TIMES_MSG / TIMES_MSG_0), 「点」 twice (POINT_MSG, still read by the
/// nine-digit score renderer, and POINT_MSG_0 here) and 「最終得点」 twice
/// (this one and FINAL_SCORE_MSG_0, which th05/end/verdict_scores.cpp reads).
/// Moving any of them out would collapse a pair into one literal and shift
/// every following byte of that contribution. th05/end/verdict_scores.cpp
/// reserved the name FINAL_SCORE_MSG for this copy; this parcel publishes it.
extern "C" const shiftjis_t VERDICT_TITLE[];
extern "C" const shiftjis_t LABEL_RANK[];
extern "C" const shiftjis_t FINAL_SCORE_MSG[];
extern "C" const shiftjis_t LABEL_MISSES[];
extern "C" const shiftjis_t LABEL_BOMBS[];
extern "C" const shiftjis_t LABEL_GAME_COMPLETION[];
extern "C" const shiftjis_t LABEL_ENEMIES_KILLED[];
extern "C" const shiftjis_t LABEL_ITEMS_COLLECTED[];
extern "C" const shiftjis_t LABEL_POINT_ITEMS_MAXED[];
extern "C" const shiftjis_t LABEL_GUTS[];
extern "C" const shiftjis_t LABEL_SLOWDOWN[];
extern "C" const shiftjis_t LABEL_YOUR_SKILL[];
extern "C" const shiftjis_t TIMES_MSG[];
extern "C" const shiftjis_t TIMES_MSG_0[];
extern "C" const shiftjis_t POINT_MSG_0[];
extern "C" const shiftjis_t SKILL_UNKNOWN_MSG[];
extern "C" const shiftjis_t SLOWDOWN_NO_VERDICT_MSG[];

/// `[measured]` Rows are 24 pixels apart, except for the 32-pixel gap between
/// the slowdown row and the verdict itself. TH04's equivalent gap is 48.
#define VERDICT_ROW(n) (verdict_top + (n))

/// `[measured]` Plain C++ linkage, NOT `extern "C"`, and this is the flip
/// state/notes/verdict_comment_put.md predicted for this exact symbol: the
/// `extern "C"` + `label near` alias existed so th04/end/verdict_animate.cpp
/// could call INTO the dump. Now that C++ defines the body, the alias would be
/// a duplicate symbol — and staffroll_animate(), still ASM, calls it too, so
/// the dump reaches it through the mangled `@verdict_stats_put$qv` the same way
/// it already reaches verdict_comment_put() and verdict_stage_scores_put().
void near verdict_stats_put(void)
{
	// `[measured]` Declaration order is load-bearing: Turbo C++ gives the
	// first-declared local the slot closest to BP, and the original holds
	// [skill_max] at `bp-4`. [line_id] is an explicit `register`, which is why
	// the prologue's only push is SI.
	uint32_t skill_max;
	register int line_id;

	skill = 0;
	graph_putsa_fx_func = FX_WEIGHT_BOLD;

	graph_putsa_fx(verdict_left, VERDICT_ROW(  0), verdict_col, VERDICT_TITLE);
	graph_putsa_fx(verdict_left, VERDICT_ROW( 24), verdict_col, LABEL_RANK);
	graph_putsa_fx(verdict_left, VERDICT_ROW( 48), verdict_col, FINAL_SCORE_MSG);
	graph_putsa_fx(verdict_left, VERDICT_ROW( 72), verdict_col, LABEL_MISSES);
	graph_putsa_fx(verdict_left, VERDICT_ROW( 96), verdict_col, LABEL_BOMBS);
	graph_putsa_fx(
		verdict_left, VERDICT_ROW(120), verdict_col, LABEL_GAME_COMPLETION
	);
	graph_putsa_fx(
		verdict_left, VERDICT_ROW(144), verdict_col, LABEL_ENEMIES_KILLED
	);
	graph_putsa_fx(
		verdict_left, VERDICT_ROW(168), verdict_col, LABEL_ITEMS_COLLECTED
	);
	graph_putsa_fx(
		verdict_left, VERDICT_ROW(192), verdict_col, LABEL_POINT_ITEMS_MAXED
	);
	graph_putsa_fx(verdict_left, VERDICT_ROW(216), verdict_col, LABEL_GUTS);
	graph_putsa_fx(verdict_left, VERDICT_ROW(240), verdict_col, LABEL_SLOWDOWN);
	graph_putsa_fx(
		verdict_left, VERDICT_ROW(272), verdict_col, LABEL_YOUR_SKILL
	);

	// `[measured]` A ternary, not an if/else: the original computes both arms
	// into AL and stores once at the merge point.
	verdict_rank = (
		(resident->stage == STAGE_EXTRA) ? RANK_EXTRA : resident->rank
	);
	graph_gaiji_puts(
		(verdict_left + 160), VERDICT_ROW(24), GAIJI_W, gbRANKS[verdict_rank],
		verdict_col
	);
	graph_score_and_ten_put(
		(verdict_left + 128), VERDICT_ROW(48), &resident->score_last
	);
	graph_3_digit_put(
		(verdict_left + 224), VERDICT_ROW(72), resident->miss_count
	);
	graph_3_digit_put(
		(verdict_left + 224), VERDICT_ROW(96), resident->bombs_used
	);
	graph_putsa_fx(
		(verdict_left + 272), VERDICT_ROW(72), verdict_col, TIMES_MSG
	);
	graph_putsa_fx(
		(verdict_left + 272), VERDICT_ROW(96), verdict_col, TIMES_MSG_0
	);

	skill_stash_quarter = true;

	// `[measured]` Two complete calls, not one call with a selected [total]:
	// the original's two argument pushes jump into a shared tail, which is
	// Turbo C++'s cross-jumping, not a source-level merge. Also `!=` rather
	// than `==`, which is what puts the non-Extra body first.
	//
	// ZUN quirk: finishing ANY route counts as full completion regardless of
	// how far the run actually got, because the target is written into the
	// counter it is about to be compared with. TH05 goes further than TH04
	// here — `>= ES_BAD` catches the bad ending as well as the good one.
	if(resident->stage != STAGE_EXTRA) {
		if(resident->end_sequence >= ES_BAD) {
			resident->std_frames = 46000;
		}
		skill_apply_and_graph_percentage(
			(verdict_left + 176), VERDICT_ROW(120), 46000, resident->std_frames
		);
	} else {
		if(resident->end_sequence == ES_EXTRA) {
			resident->std_frames = 12800;
		}
		skill_apply_and_graph_percentage(
			(verdict_left + 176), VERDICT_ROW(120), 12800, resident->std_frames
		);
	}
	skill_stash_quarter = false;

	skill_apply_and_graph_percentage(
		(verdict_left + 176), VERDICT_ROW(144),
		resident->enemies_gone, resident->enemies_killed
	);
	skill_apply_and_graph_percentage(
		(verdict_left + 176), VERDICT_ROW(168),
		resident->items_spawned, resident->items_collected
	);
	skill_apply_and_graph_percentage(
		(verdict_left + 176), VERDICT_ROW(192),
		resident->point_items_collected,
		resident->max_valued_point_items_collected
	);
	skill_apply_and_graph_guts();

	skill_subtract = true;
	skill_apply_and_graph_percentage(
		(verdict_left + 176), VERDICT_ROW(240),
		(resident->frames / 10), (resident->slow_frames / 10)
	);
	skill_subtract = false;

	// kb/codegen/0145: `/=`, for the divisor-first load order.
	skill /= 12;

	// kb/codegen/0147: the cast is load-bearing and the parcel paid two build
	// cycles for it. [skill_quarter] is `uint32_t` and [skill] is `int32_t`,
	// and a compound assignment whose sides differ in signedness falls back to
	// the general expression path — where `-Z` then sees that EAX still holds
	// [skill] from the division above and drops the reload, giving
	// `add eax, [skill_quarter]` / `mov [skill], eax` instead of the original's
	// `mov eax, [skill_quarter]` / `add [skill], eax`. Casting to the
	// left-hand side's exact type restores the memory read-modify-write.
	skill += static_cast<int32_t>(skill_quarter);

	// The topmost two LEBCD digits of the run's best score. `digits[7] >= 10`
	// is unreachable — the packing tops out at 9 — so the ceiling bonus is
	// ZUN bloat, and every real run takes the branch below it.
	if(resident->score_highest.digits[7] >= 10) {
		skill += 500000;
	} else {
		// `[measured]` Two statements, not one sum: the original stores to
		// [skill] after each term.
		skill += (resident->score_highest.digits[6] * 5000L);
		skill += (resident->score_highest.digits[7] * 50000L);
	}

	switch(verdict_rank) {
	case RANK_EASY:
		skill -= 50000;
		skill_max = 800000;
		break;
	case RANK_NORMAL:
		skill_max = 1000000;
		break;
	case RANK_HARD:
		// `[measured]` Two statements again — `skill = ((skill * 5) / 4)`
		// would keep the product in a register.
		skill *= 5;
		skill /= 4;
		skill += 150000;
		skill_max = 1200000;
		break;
	case RANK_LUNATIC:
		skill *= 3;
		skill /= 2;
		skill += 300000;
		skill_max = 1400000;
		break;
	case RANK_EXTRA:
		skill *= 3;
		skill /= 2;
		skill += 250000;
		skill_max = 2000000;
		break;
	}

	// kb/codegen/0077: the generated table is dense and sorted, but the case
	// bodies follow source order — and `credit_lives == 3` has no body at all,
	// so its slot holds the default label.
	switch(resident->credit_lives) {
	case 1:
		skill += 50000;
		skill_max += 100000;
		break;
	case 2:
		skill += 25000;
		skill_max += 50000;
		break;
	case 4:
		skill_max -= 25000;
		break;
	case 5:
		skill_max -= 50000;
		break;
	case 6:
		skill_max -= 75000;
		break;
	}

	// kb/codegen/0135: three sparse cases, so a `cmp`/`jz` chain rather than a
	// table.
	switch(resident->credit_bombs) {
	case 0:
		skill += 50000;
		skill_max += 100000;
		break;
	case 1:
		skill += 30000;
		skill_max += 50000;
		break;
	case 2:
		skill += 20000;
		skill_max += 25000;
		break;
	}

	if(!resident->turbo_mode) {
		skill -= 200000;
		skill_max -= 100000;
	}

	if(resident->miss_count >= 10) {
		skill -= 300000;
	} else {
		skill -= (resident->miss_count * 30000L);
	}
	if(resident->bombs_used >= 15) {
		skill -= 225000;
	} else {
		skill -= (resident->bombs_used * 15000L);
	}

	// i.e., ES_INGAME or ES_SCORE: the player never finished a route, so they
	// keep seven eighths of the accumulated skill. TH04 halves it instead.
	if(resident->end_sequence < ES_EXTRA) {
		skill *= 7;
		skill /= 8;
	}

	// `[measured]` The lower bound is signed and the upper one is not, because
	// [skill] is `int32_t` and [skill_max] is `uint32_t`. Both halves of that
	// asymmetry are in the original's `jge`/`jbe` pair.
	if(skill < 0) {
		skill = 0;
	} else if(skill > skill_max) {
		skill = skill_max;
	}

	verdict_comment_1[0] = '\0';
	verdict_comment_2[0] = '\0';

	// ZUN quirk: spending more than half the run in slow mode voids the
	// verdict entirely — the number is not even shown, and [skill] keeps
	// whatever it was clamped to.
	if((resident->frames / 2) > resident->slow_frames) {
		graph_fraction_of_million_put(
			(verdict_left + 176), VERDICT_ROW(272), skill
		);
		graph_putsa_fx(
			(verdict_left + 272), VERDICT_ROW(272), verdict_col, POINT_MSG_0
		);

		language_asset_file_ropen(ude_txt);

		// Record 0 is the best one, and the file is left at it if [skill] hit
		// the cap.
		if(skill < 1500000) {
			if(skill == 0) {
				line_id = 25;
			} else if(skill < 1050000) {
				line_id = (24 - (skill / 50000));
			} else if(skill < 1200000) {
				line_id = 3;
			} else if(skill < 1350000) {
				line_id = 2;
			} else {
				line_id = 1;
			}
			file_seek((line_id * 30), 0);
		}
		file_read(verdict_comment_1, 30);

		// The second block of records starts at record 26.
		line_id = verdict_comment_2_num();
		file_seek(((line_id * 30) + 780), 0);
		file_read(verdict_comment_2, 30);
		language_asset_file_close();
		verdict_comment_1[28] = '\0';
		verdict_comment_2[28] = '\0';
	} else {
		graph_putsa_fx(
			(verdict_left + 176), VERDICT_ROW(272), verdict_col,
			SKILL_UNKNOWN_MSG
		);
		graph_putsa_fx(
			(verdict_left + 48), VERDICT_ROW(296), verdict_comment_col,
			SLOWDOWN_NO_VERDICT_MSG
		);
	}
}
