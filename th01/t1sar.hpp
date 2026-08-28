#ifndef TH01_T1SAR_HPP
#define TH01_T1SAR_HPP

#include "platform.h"
#include "th01/hardware/graph.h"
#include "th01/replay_format.hpp"

#if T1SAR_TRACE

class CBossEntity;
class CBossAnim;

// Private Sariel first-combat paired witness. It observes the source-owned
// post-entrance Phase 1 seam only; it is not a replay checkpoint.
void t1sar_trace_reset(void);
void t1sar_natural_prepare(void);
bool16 t1sar_direct_prepare(void);
bool16 t1sar_direct_ready(void);
void t1sar_visible_page_set(page_t page);
void t1sar_accessed_page_set(page_t page);
void t1sar_owner_set(
	int boss_phase,
	int boss_phase_frame,
	int boss_hp,
	int hud_hp_first_white,
	int hud_hp_first_redwhite,
	int pattern_state,
	int invincibility_frame,
	bool16 invincible,
	int phase_pattern,
	int patterns_done,
	int patterns_until_next,
	bool16 initial_hp_rendered,
	const CBossEntity& shield,
	const CBossAnim& dress,
	const CBossAnim& wand
);
void t1sar_pre_input(uint8_t process_seq);

#endif

#endif /* TH01_T1SAR_HPP */
