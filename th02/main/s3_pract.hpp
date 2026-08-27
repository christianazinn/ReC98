#ifndef TH02_MAIN_S3_PRACT_HPP
#define TH02_MAIN_S3_PRACT_HPP

#include "platform.h"

enum th02_s3_stones_clean_target_t {
	T2S3_STONES_BOSS_START = 0,
	T2S3_STONES_INNER_PAIR = 1,
	T2S3_STONES_OUTER_PAIR = 2,
};

// Constructs one source-derived stable Stage 3 boss target. Invalid targets
// and calls outside Stage 3 fail before touching live state.
bool16 far th02_s3_stones_clean_init(
	th02_s3_stones_clean_target_t target
);

#endif /* TH02_MAIN_S3_PRACT_HPP */
