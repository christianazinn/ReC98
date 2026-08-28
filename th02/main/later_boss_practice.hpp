#ifndef TH02_MAIN_LATER_BOSS_PRACTICE_HPP
#define TH02_MAIN_LATER_BOSS_PRACTICE_HPP

#include "platform.h"

// Clean public Practice entries. These identify semantic phase boundaries, not
// historical replay positions or native transition frames.
enum th02_later_boss_target_t {
	T2LBPT_MARISA_PHASE1 = 0,
	T2LBPT_MIMA_PHASE1 = 1,
	T2LBPT_SIGMA_PHASE1 = 2,
	T2LBPT_MARISA_ROUND2 = 3,
	T2LBPT_MIMA_PHASE3 = 4,
	T2LBPT_SIGMA_PHASE3 = 5,
	T2LBPT_MARISA_ROUND3 = 6,
	T2LBPT_MIMA_PHASE5 = 7,
	T2LBPT_SIGMA_PHASE5 = 8,
	T2LBPT_MIMA_PHASE7 = 9,
};

bool16 far th02_later_boss_clean_init(
	th02_later_boss_target_t target
);

#endif /* TH02_MAIN_LATER_BOSS_PRACTICE_HPP */
