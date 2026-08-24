/// The verdict screen's stage score table
/// --------------------------------------
/// `[measured]` This was the LAST proc of th05_maine.asm's SCORE_TEXT block,
/// and th05/staff.cpp is the object immediately after it in TH05's MAINE.EXE
/// link order — a translation unit whose only contribution to that segment,
/// space_window_set(), sits directly behind the dump's. Emitting this body
/// ahead of that function appends it to the dump's block from the far end: a
/// tail lift with no carve, no new segment, no group-list edit and no
/// Tupfile.lua line, mirroring the head lift that th05/regist.cpp gives
/// th04/end/verdict_digits.cpp. The block is now consumed from both ends.
///
/// `[measured]` Nothing on the verdict screen itself calls this function.
/// staffroll_animate() does, once, after the staff roll — which is why TH05
/// hoists the overlay's origin and colour into _DATA (see
/// th04/end/verdict_digits.cpp) where TH04 hardcodes them: the same renderers
/// run twice, from two callers, against two different origins.

/// th04/hardware/grppsafx.h and th05/resident.hpp have no include guards and
/// th05/end/verdict_comment.cpp — which the host includes ahead of this file —
/// has already pulled both in. Including them again is a wall of "Multiple
/// declaration" errors, so this file relies on its host, which is the idiom
/// for every other .cpp fragment in the chain (see
/// th04/end/verdict_digits.cpp on th04/gaiji/gaiji.h).

/// `[measured]` The overlay origin, published by zero-byte `label word`
/// aliases in front of the private _DATA words they alias in th05_maine.asm
/// (kb/codegen/0123), exactly as [verdict_col] already is. The dump's own
/// remaining references keep IDA's original spelling, which is why these are
/// aliases and not renames.
extern "C" screen_x_t verdict_left;
extern "C" vram_y_t verdict_top;
extern "C" vc2 verdict_col;

/// `[measured]` All seven stay _DATA bytes of the root dump rather than
/// becoming literals of this translation unit, for the reason
/// th04/end/verdict_digits.cpp documents for POINT_MSG: the build compiles
/// with `-d`, and the dump holds a byte-identical SECOND copy of 「最終得点」
/// twenty-three lines above the one this function uses — the copy the
/// still-ASM screen body renders. Merging that pair into a single literal
/// would shift every following byte of the dump's _DATA contribution. The
/// `_0` suffix is the dump's own convention for such a duplicate (see its
/// four identical `_SCOREDAT_FN*` copies); the first copy gets published as
/// FINAL_SCORE_MSG when that body is lifted.
extern "C" const shiftjis_t STAGE_1_MSG[];
extern "C" const shiftjis_t STAGE_2_MSG[];
extern "C" const shiftjis_t STAGE_3_MSG[];
extern "C" const shiftjis_t STAGE_4_MSG[];
extern "C" const shiftjis_t STAGE_5_MSG[];
extern "C" const shiftjis_t STAGE_6_MSG[];
extern "C" const shiftjis_t FINAL_SCORE_MSG_0[];

/// The nine-digit LEBCD score renderer with its 点 label, still one object
/// ahead of this one in th04/end/verdict_digits.cpp.
extern "C" void pascal near graph_score_and_ten_put(
	screen_x_t left, vram_y_t top, const score_lebcd_t far *score
);

/// Renders the six per-stage scores under their 「Ｎ面」 labels, then the
/// run's final score under 「最終得点」 two rows further down.
void near verdict_stage_scores_put(void)
{
	screen_x_t left;
	register int i;
	register vram_y_t top;

	graph_putsa_fx(verdict_left, (verdict_top +  64), verdict_col, STAGE_1_MSG);
	graph_putsa_fx(verdict_left, (verdict_top +  96), verdict_col, STAGE_2_MSG);
	graph_putsa_fx(verdict_left, (verdict_top + 128), verdict_col, STAGE_3_MSG);
	graph_putsa_fx(verdict_left, (verdict_top + 160), verdict_col, STAGE_4_MSG);
	graph_putsa_fx(verdict_left, (verdict_top + 192), verdict_col, STAGE_5_MSG);
	graph_putsa_fx(verdict_left, (verdict_top + 224), verdict_col, STAGE_6_MSG);
	graph_putsa_fx(
		verdict_left, (verdict_top + 288), verdict_col, FINAL_SCORE_MSG_0
	);

	left = (verdict_left + 128);
	for(i = 0, top = (verdict_top + 64); i < MAIN_STAGE_COUNT; i++, top += 32) {
		graph_score_and_ten_put(left, top, &resident->stage_score[i]);
	}
	graph_score_and_ten_put(
		left, (verdict_top + 288), &resident->score_last
	);
}
