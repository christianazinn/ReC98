#ifndef TH01_REPLAY_MILESTONE_HPP
#define TH01_REPLAY_MILESTONE_HPP

#include "th01/replay_format.hpp"

// A private, cross-process witness for the release-blocker runtime gates.
// T1RB is intentionally separate from the numeric T1RP exact-trace selector:
// it exercises ordinary OP-to-REIIDEN handoff without changing replay data.
#ifndef T1RB
	#define T1RB 0
#endif
#define T1REPLAY_PROCESS_MILESTONES (T1RB == 1)

enum t1replay_process_milestone_t {
	T1RPM_PRACTICE_CARRIER_COMMITTED = 1,
	T1RPM_REIIDEN_ENTRY = 2,
	T1RPM_CARRIER_ACCEPTED = 3,
	T1RPM_REIIDEN_TEARDOWN = 4,
	T1RPM_STAGE5_BOSS_INIT = 5,
	// These diagnostic-only outcomes keep the Practice runtime gate from
	// conflating command loss, a missing native resident, an initial .TMP
	// write failure, resident allocation, and guard creation.
	T1RPM_COMMAND_REJECTED = 6,
	T1RPM_COMMAND_ACCEPTED = 7,
	T1RPM_RESIDENT_MISSING = 8,
	T1RPM_RESIDENT_FOUND = 9,
	T1RPM_HEADER_CAPTURED = 10,
	T1RPM_HEADER_WRITE_FAILED = 11,
	T1RPM_HEADER_WRITTEN = 12,
	T1RPM_REPLAY_RES_CREATE_FAILED = 13,
	T1RPM_REPLAY_RES_CREATED = 14,
	T1RPM_GUARD_BEGIN_FAILED = 15,
	T1RPM_GUARD_READY = 16,
	T1RPM_BOOTSTRAP_CONFIG_ACCEPTED = 17,
	T1RPM_RECORD_COMMAND_WRITE_FAILED = 18,
	T1RPM_RECORD_COMMAND_WRITTEN = 19,
	T1RPM_PRACTICE_CARRIER_CREATE_FAILED = 20,
	T1RPM_HANDOFF_PROBE_REACHED = 21,
	T1RPM_STAGE4_CLEAR_BEGIN = 22,
	T1RPM_STAGE4_BONUS_COMPLETE = 23,
	T1RPM_STAGE4_SPLIT_COMPLETE = 24,
	T1RPM_STAGE4_HANDOFF_COMMITTED = 25,
	T1RPM_STAGE4_EXECL_RETURNED = 26,
	T1RPM_STAGE4_BONUS_ENTERED = 27,
	T1RPM_STAGE4_BONUS_BOX_OPENED = 28,
	T1RPM_STAGE4_BONUS_RENDERED = 29,
	T1RPM_STAGE4_BONUS_INPUT_RELEASED = 30,
	T1RPM_STAGE4_GAMEPLAY_LOOP_LEFT = 31,
	T1RPM_STAGE4_SCROLLUP_COMPLETE = 32,
	T1RPM_STAGE4_LIFE_LOOP_LEFT = 33,
	T1RPM_STAGE5_ARCHIVE_LOADED = 34,
	T1RPM_STAGE5_SCENE_LOADED = 35,
	T1RPM_STAGE5_BOSS_LOADED = 36,
	T1RPM_STAGE5_ENTRANCE_COMPLETE = 37,
};

#if T1REPLAY_PROCESS_MILESTONES
void far t1replay_process_milestone_reset(void);
void far t1replay_process_milestone(t1replay_process_milestone_t milestone);
// H is a private, one-shot source route which reaches the normal REIIDEN
// teardown/handoff path after the first boss has loaded. It is not gameplay
// state and does not exist outside the T1RB diagnostic profile.
bool far t1replay_process_milestone_handoff_probe(void);
bool far t1replay_process_milestone_stage4_clear_probe(void);
#endif

#endif
