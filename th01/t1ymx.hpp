#ifndef TH01_T1YMX_HPP
#define TH01_T1YMX_HPP

#include "th01/main/boss/b10m.hpp"
#include "th01/replay_format.hpp"

#if T1REPLAY_YUUGENMAGAN_FIRST_COMBAT_TRACE
// Builds the semantic record at phase 1's natural post-entrance input seam.
// This is private measurement support, never a replay or Practice command.
bool16 t1boss_yuugenmagan_first_combat_construct(
	t1boss_yuugenmagan_checkpoint_t *checkpoint
);
#endif

#endif /* TH01_T1YMX_HPP */
