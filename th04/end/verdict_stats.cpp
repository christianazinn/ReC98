/// The verdict screen's body
/// -------------------------
/// Twelve labels, their values, the [skill] computation and its clamp, the
/// `_ude.txt` verdict line, then the wait and the fade-out.
///
/// `[measured]` This was the LAST proc of th04_maine.asm's MAINE_01_TEXT
/// contribution once verdict_animate() had left it, and it comes out through
/// the same seam: th04/staff.cpp is the LAST object in TH04's MAINE.EXE link
/// list, so a `-zCMAINE_01_TEXT -zPgroup_01` body placed AHEAD of
/// verdict_animate.cpp in that wrapper lands immediately behind the dump's
/// shrunken contribution, i.e. at the address this proc already had. A
/// kb/codegen/0098 tail lift: no carve, no new segment, no group-list edit and
/// no Tupfile.lua line. MAINE_01_TEXT's tail is the only free end this dump
/// has, so every further lift out of it grows this file's chain backwards.
///
/// The two cs-relative jump tables at the end of this function are Turbo C++'s
/// own, generated after the epilogue, so they came out of the dump with the
/// body. `-zPgroup_01` (kb/codegen/0104) is what frames their operands on the
/// group rather than on MAINE_01_TEXT, and it is already set by the
/// th04/staff.cpp wrapper.
///
/// TH05's counterpart is verdict_stats_put() in
/// th05/end/verdict_stats.cpp. It has the same label list in the same order
/// and the same five percentage rows, but reads every coordinate and colour
/// out of `_DATA` where TH04 hardcodes immediates, and its scoring constants
/// and clamps differ throughout. Those differences keep the two C++ bodies
/// separate.

#include "th01/rank.h"
#include "th02/v_colors.hpp"
#include "th02/hardware/frmdelay.h"
#include "th04/common.h"
#include "th04/resident.hpp"
#include "th04/end/end.h"
#include "th04/hardware/input.h"
/// th04/hardware/grppsafx.h has no include guard and th04/end/verdict_guts.cpp,
/// which now precedes this file in the same translation unit, already needs it.
/// Relying on the host is the idiom for every other .cpp fragment in this
/// chain.
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"

/// `[measured]` Every string below stays a `_DATA` byte of the root dump
/// rather than becoming a literal of this translation unit, for the reason
/// th04/hiscore/regist_menu.cpp documents at length: the build compiles with
/// `-d`, and 「回」 appears twice byte-identically (TIMES_MSG / TIMES_MSG_0),
/// as does 「点」 (POINT_MSG, still read by the ASM score renderer, and
/// POINT_MSG_0 here). Moving any of them out would both collapse a pair into
/// one and shift every following byte of the dump's `_DATA` contribution.
/// Published there under these names; the `_0` suffix is the dump's own
/// convention for such a duplicate.
extern "C" const shiftjis_t VERDICT_TITLE[];
extern "C" const shiftjis_t LABEL_RANK[];
extern "C" const shiftjis_t LABEL_SCORE[];
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

/// The 28-character verdict lines, one per 30-byte record (28 + CR + LF).
extern "C" const char ude_txt[];

/// The record read out of [ude_txt]. Stays in the dump's `.data?`, published
/// by renaming its IDA label (kb/codegen/0123); nothing else references it.
extern "C" shiftjis_t verdict_line[];

/// The five 7-cell, right-aligned gaiji rank names of
/// th04/gaiji/verdict[data].asm, reached there through a zero-byte
/// `label byte` alias because th05_maine.asm still references the original
/// `grEASY` spelling (kb/codegen/0123).
extern "C" const unsigned char gbRANKS[RANK_COUNT][8];

/// `[measured]` A `long`, not the `unsigned long` its name suggests: the clamp
/// below tests it against 0 with a signed compare, and both divisions of it
/// sign-extend, while the same clamp's upper bound comes out unsigned because
/// [skill_max] is. Both halves of that asymmetry are needed and neither is
/// guessable.
extern "C" int32_t skill;

/// ZUN bloat: written 1/0 around exactly one row here and never read anywhere
/// in TH04 — the game has no [skill_quarter] for it to feed. TH05 finished
/// the mechanism; see th04/end/verdict_digits.cpp.
extern "C" bool skill_stash_quarter;

/// `[measured]` Held in its own `.data?` byte rather than recomputed, because
/// the rank-dependent bonus below needs it again after RANK_EXTRA has been
/// substituted for the Extra Stage.
extern "C" unsigned char verdict_rank;

/// `[measured]` The dump spelled this one I short, ending its mangled argument
/// list in a bare unsigned modifier with no base type. Turbo C++ 4.02 cannot
/// produce that: a probe object built with the build's own flags mangles
/// plain unsigned, unsigned int and a typedef of it all the same way, and the
/// short, char and long widths each get their own two-letter code. The dump's
/// two siblings below agree, and TH05's already-matched copy of this very
/// function carries the longer spelling. Corrected there; symbol spellings
/// never reach the load image.
void pascal near graph_3_digit_put(
	screen_x_t left, screen_y_t top, uint16_t num
)
;

/// Renders `resident->score_last` as eight right-aligned boldface gaiji digits
/// at a hardcoded (160, 96), followed by 「点」. Defined in
/// th04/end/verdict_digits.cpp, alongside TH05's parameterised nine-digit
/// sibling.
extern "C" void near graph_score_and_ten_put(void);

void pascal near skill_apply_and_graph_percentage(
	screen_x_t left, screen_y_t top, uint16_t total, uint16_t share
)
;

/// The 「気合い」 row: a pseudo-random bonus seeded from `resident->rand` and
/// weighted by the continues, the Turbo Mode flag and the graze count. Defined
/// in th04/end/verdict_guts.cpp, the file this one's chain includes
/// immediately ahead of itself.
extern "C" void near skill_apply_and_graph_guts(void);

void pascal near graph_fraction_of_million_put(
	screen_x_t left, screen_y_t top, uint32_t num
)
;

/// Rows are 24 pixels apart, except for the 24-pixel gap between the last
/// percentage row and the verdict itself.
#define VERDICT_LABEL_LEFT 16
#define VERDICT_VALUE_LEFT 192

/// kb/codegen/0096 + 0139: the original pads the byte before the first of the
/// two generated jump tables, putting them at `0A05:209E` and `0A05:20AA`.
/// Without this the body is byte-identical and both tables land one byte
/// early. Restored to `-a1` immediately after, so that nothing appended to
/// this translation unit inherits the padding silently.
#pragma option -a2

extern "C" void near verdict_stats_put_and_wait(void)
{
	uint32_t skill_max;
	register int line_id;

	graph_putsa_fx_func = FX_WEIGHT_BOLD;
	graph_accesspage(0);
	graph_showpage(0);

	graph_putsa_fx(VERDICT_LABEL_LEFT,  48, V_WHITE, VERDICT_TITLE);
	graph_putsa_fx(VERDICT_LABEL_LEFT,  72, V_WHITE, LABEL_RANK);
	graph_putsa_fx(VERDICT_LABEL_LEFT,  96, V_WHITE, LABEL_SCORE);
	graph_putsa_fx(VERDICT_LABEL_LEFT, 120, V_WHITE, LABEL_MISSES);
	graph_putsa_fx(VERDICT_LABEL_LEFT, 144, V_WHITE, LABEL_BOMBS);
	graph_putsa_fx(VERDICT_LABEL_LEFT, 168, V_WHITE, LABEL_GAME_COMPLETION);
	graph_putsa_fx(VERDICT_LABEL_LEFT, 192, V_WHITE, LABEL_ENEMIES_KILLED);
	graph_putsa_fx(VERDICT_LABEL_LEFT, 216, V_WHITE, LABEL_ITEMS_COLLECTED);
	graph_putsa_fx(VERDICT_LABEL_LEFT, 240, V_WHITE, LABEL_POINT_ITEMS_MAXED);
	graph_putsa_fx(VERDICT_LABEL_LEFT, 264, V_WHITE, LABEL_GUTS);
	graph_putsa_fx(VERDICT_LABEL_LEFT, 288, V_WHITE, LABEL_SLOWDOWN);
	graph_putsa_fx(VERDICT_LABEL_LEFT, 336, V_WHITE, LABEL_YOUR_SKILL);

	// `[measured]` A ternary, not an if/else: the original computes both arms
	// into AL and stores once at the merge point.
	verdict_rank = (
		(resident->stage == STAGE_EXTRA) ? RANK_EXTRA : resident->rank
	);
	graph_gaiji_puts(176, 72, GAIJI_W, gbRANKS[verdict_rank], VERDICT_COL);

	graph_score_and_ten_put();
	graph_3_digit_put(240, 120, resident->miss_count);
	graph_3_digit_put(240, 144, resident->bombs_used);
	graph_putsa_fx(288, 120, VERDICT_COL, TIMES_MSG);
	graph_putsa_fx(288, 144, VERDICT_COL, TIMES_MSG_0);

	skill_stash_quarter = true;

	// `[measured]` Two complete calls, not one call with a selected [total]:
	// the original's two argument pushes jump into a shared tail, which is
	// Turbo C++'s cross-jumping, not a source-level merge. Also `!=` rather
	// than `==`, which is what puts the non-Extra body first.
	//
	// ZUN quirk: reaching the Extra Stage's or the Good Ending's *end* counts
	// as full completion regardless of how far the run actually got, because
	// the target is written into the counter it is about to be compared with.
	if(resident->stage != STAGE_EXTRA) {
		if(resident->end_sequence == ES_GOOD) {
			resident->std_frames = 44000;
		}
		skill_apply_and_graph_percentage(
			VERDICT_VALUE_LEFT, 168, 44000, resident->std_frames
		);
	} else {
		if(resident->end_sequence == ES_EXTRA) {
			resident->std_frames = 12000;
		}
		skill_apply_and_graph_percentage(
			VERDICT_VALUE_LEFT, 168, 12000, resident->std_frames
		);
	}
	skill_stash_quarter = false;

	skill_apply_and_graph_percentage(
		VERDICT_VALUE_LEFT, 192,
		resident->enemies_gone, resident->enemies_killed
	);
	skill_apply_and_graph_percentage(
		VERDICT_VALUE_LEFT, 216,
		resident->items_spawned, resident->items_collected
	);
	skill_apply_and_graph_percentage(
		VERDICT_VALUE_LEFT, 240,
		resident->point_items_collected,
		resident->max_valued_point_items_collected
	);
	skill_apply_and_graph_guts();
	skill_apply_and_graph_percentage(
		VERDICT_VALUE_LEFT, 288,
		(resident->frames / 10), (resident->slow_frames / 10)
	);

	skill /= 5;

	// The topmost two LEBCD digits of the final score, i.e. everything from
	// 10,000,000 upwards. `digits[7] >= 9` is the 900,000,000 ceiling the
	// packing allows at all.
	if(resident->score_last.digits[7] >= 9) {
		skill += 600000;
	} else {
		skill += (resident->score_last.digits[6] * 10000L);
		if(resident->score_last.digits[7] > 3) {
			skill += ((resident->score_last.digits[7] - 3) * 100000L);
		}
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
		skill += 150000;
		skill_max = 1200000;
		break;
	case RANK_LUNATIC:
		skill += 300000;
		skill_max = 1400000;
		break;
	case RANK_EXTRA:
		skill += 450000;
		skill_max = 1500000;
		break;
	}

	// kb/codegen/0077: the generated table is dense and sorted, but the case
	// bodies follow source order — and `credit_lives == 3` has no body at
	// all, so its slot holds the default label.
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

	// kb/codegen/0135: two sparse cases, so a `cmp`/`jz` chain rather than a
	// table.
	switch(resident->credit_bombs) {
	case 0:
		skill += 50000;
		skill_max += 100000;
		break;
	case 1:
		skill += 20000;
		skill_max += 50000;
		break;
	}

	if(!resident->turbo_mode) {
		skill -= 100000;
	}

	if(resident->miss_count >= 15) {
		skill -= 300000;
	} else {
		skill -= (resident->miss_count * 20000L);
	}
	if(resident->bombs_used >= 30) {
		skill -= 90000;
	} else {
		skill -= (resident->bombs_used * 3000L);
	}

	static_assert(ES_EXTRA < ES_BAD);
	if(resident->end_sequence < ES_EXTRA) {
		// i.e., ES_INGAME or ES_SCORE: the player never finished a route, so
		// half the accumulated skill is all they get.
		if(verdict_rank != RANK_EXTRA) {
			skill /= 2;
		} else {
			skill -= 200000;
		}
	} else if(resident->end_sequence == ES_BAD) {
		skill_max -= 100000;
	}

	if(skill < 0) {
		skill = 0;
	} else if(skill > skill_max) {
		skill = skill_max;
	}

	// ZUN quirk: spending more than half the run in slow mode voids the
	// verdict line entirely — the number is not even shown, and [skill] keeps
	// whatever it was clamped to.
	if((resident->frames / 2) > resident->slow_frames) {
		graph_fraction_of_million_put(VERDICT_VALUE_LEFT, 336, skill);
		graph_putsa_fx(288, 336, VERDICT_COL, POINT_MSG_0);

		file_ropen(ude_txt);

		// Line 0 is the best one, and the file is left at it if [skill] hit
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
		file_read(verdict_line, 30);
		verdict_line[28] = '\0';
		file_close();

		frame_delay(64);
		graph_putsa_fx(64, 360, V_WHITE, verdict_line);
	} else {
		graph_putsa_fx(
			VERDICT_VALUE_LEFT, 336, VERDICT_COL, SKILL_UNKNOWN_MSG
		);
		graph_putsa_fx(64, 360, V_WHITE, SLOWDOWN_NO_VERDICT_MSG);
	}

	input_wait_for_change(0);
	palette_black_out(2);
}

#pragma option -a1
