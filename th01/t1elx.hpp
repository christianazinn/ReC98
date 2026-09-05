#ifndef TH01_T1ELX_HPP
#define TH01_T1ELX_HPP

#include "platform.h"
#include "th01/hardware/graph.h"
#include "th01/replay_format.hpp"

#if T1ELX_TRACE

class CBossEntity;

// Starts a profile-only natural witness. It accepts only the normal Makai
// Scene 2 boss-start handoff and does not alter that handoff.
void t1elx_natural_prepare(void);

// Arms the profile-only direct witness after the normal stage loader and the
// existing Elis owner have both succeeded.
bool16 t1elx_direct_prepare(void);

// Clears private far-state between REIIDEN processes.
void t1elx_trace_reset(void);

// Source-owned seam in elis_main(), reached before the first ordinary phase-1
// update. The parameters expose only stack-local owner state for hashing.
void t1elx_pre_input(
	int boss_id, int boss_phase, int boss_phase_frame, int boss_hp,
	int pattern_state, int form, int hit_invincibility_frame,
	bool16 hit_invincible, int phase_pattern, bool16 teleport_done,
	int bat_velocity_x, int bat_velocity_y, bool16 initial_hp_rendered,
	const CBossEntity& still_or_wave, const CBossEntity& attack,
	const CBossEntity& bat
);

// These latches are owned by the tail probe so graph.cpp retains its release
// BSS layout. They are valid only in T1RP10/T1RP11 REIIDEN profiles.
void t1elx_visible_page_set(page_t page);
void t1elx_accessed_page_set(page_t page);

#endif

#endif /* TH01_T1ELX_HPP */
