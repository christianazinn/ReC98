#ifndef TH01_LANGUAGE_FUUIN_HPP
#define TH01_LANGUAGE_FUUIN_HPP

#include "pc98.h"
#include "shiftjis.hpp"

enum t1lang_fuuin_regist_text_t {
	T1LFRT_ALPHABET_END = 0,
	T1LFRT_HEADER_PLACE,
	T1LFRT_HEADER_NAME,
	T1LFRT_HEADER_SCORE,
	T1LFRT_HEADER_STAGE_ROUTE,
	T1LFRT_PLACE_0,
	T1LFRT_PLACE_1,
	T1LFRT_PLACE_2,
	T1LFRT_PLACE_3,
	T1LFRT_PLACE_4,
	T1LFRT_PLACE_5,
	T1LFRT_PLACE_6,
	T1LFRT_PLACE_7,
	T1LFRT_PLACE_8,
	T1LFRT_PLACE_9,
	T1LFRT_STAGE_MAKAI,
	T1LFRT_STAGE_JIGOKU,
};

enum t1lang_fuuin_verdict_text_t {
	T1LFVT_RANK = 0,
	T1LFVT_SCORE_HIGHEST,
	T1LFVT_SCORE,
	T1LFVT_CONTINUES,
	T1LFVT_SHRINE,
	T1LFVT_MAKAI_1,
	T1LFVT_MAKAI_2,
	T1LFVT_MAKAI_3,
	T1LFVT_JIGOKU_1,
	T1LFVT_JIGOKU_2,
	T1LFVT_JIGOKU_3,
	T1LFVT_MAKAI_TOTAL,
	T1LFVT_JIGOKU_TOTAL,
	T1LFVT_HEADING,
};

// Renders one registration label, falling back to [japanese] unless the
// process-entry preference selected the validated English donor text.
void far t1lang_fuuin_regist_put(
	screen_x_t left, screen_y_t top, int col_and_fx,
	const shiftjis_t *japanese, t1lang_fuuin_regist_text_t text
);

// Preserves the original Japanese title path and emits the English donor's
// rank heading without adding an initialized translated table to DGROUP.
bool far t1lang_fuuin_regist_title_put(
	screen_x_t left, screen_y_t top, int col_and_fx,
	const shiftjis_t *japanese_format, int rank
);

// Materializes the validated English v1.00 verdict donor strings without
// adding translated initialized data to FUUIN's DGROUP.
bool far t1lang_fuuin_verdict_english_build(
	shiftjis_t *out, t1lang_fuuin_verdict_text_t text
);
bool far t1lang_fuuin_verdict_title_english_build(
	shiftjis_t *out, int group, int level
);
pixel_t far t1lang_fuuin_verdict_max_w(
	const shiftjis_t *rank, unsigned long score_highest, unsigned long score,
	const int32_t *continues_per_scene, unsigned long continues_total,
	bool16 makai_route
);

#endif /* TH01_LANGUAGE_FUUIN_HPP */
