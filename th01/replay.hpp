#ifndef TH01_REPLAY_HPP
#define TH01_REPLAY_HPP

#include "platform.h"
#include "th01/replay_format.hpp"
#include "th01/formats/scoredat.hpp"
#include "th01/score.h"
#include "shiftjis.hpp"

// Called before resident_stuff_get(). The one-shot command is consumed only in
// the first REIIDEN process; a valid resident carrier always takes precedence.
void far t1replay_entry(void);

// A playback validation failure is terminal for the current REIIDEN process.
// main() checks this before resident initialization; runtime callers transfer
// directly through the same safe OP cleanup path.
bool16 far t1replay_abort_requested(void);
void far t1replay_abort_to_op(void);

// The frame-input seam. input_sense(true) returns before this call and
// therefore consumes no replay sample.
void far t1replay_frame_io(void);
int far t1replay_key_sense(int keygroup);

// Called at the first active-gameplay input boundary of each REIIDEN process.
// The private sidecar is capture-only until all world codecs can restore it.
void far t1replay_checkpoint_capture(int pellet_speed_raise_cycle);

// Private exact-restore seam. The pending flag is set only after the complete
// sidecar and replay prefix validate before any gameplay mutation.
bool16 far t1replay_checkpoint_restore_pending(void);
bool16 far t1replay_checkpoint_restore_apply(int *pellet_speed_raise_cycle);

// The two accessors keep hidden owner state semantic and pointer-free. Import
// remains intentionally unused by the capture-only substrate.
void t1replay_input_checkpoint_export(t1replay_checkpoint_input_t *out);
void t1replay_input_checkpoint_import(const t1replay_checkpoint_input_t *in);

// Called immediately before REIIDEN hands control to another executable.
// Source and target are encoded explicitly in both stream and carrier.
bool16 far t1replay_process_handoff(uint8_t target_process);

// REIIDEN owns only the menu-return terminal. Clear finalization belongs to
// FUUIN after its deterministic input phases have completed.
#if T1REPLAY_EXACT_TRACE
// Preserve live world state before teardown; terminal acceptance writes it.
void far t1replay_exact_terminal_capture(uint8_t end_reason);
#endif
void far t1replay_terminal(uint8_t end_reason);
// Exact-width wrappers for two existing stock far-call sites. The first
// terminalizes before the native score menu; the second writes a postgame
// request only after Continue has returned, then performs the native BGM stop.
void far t1replay_gameover_regist_menu(
	score_t score, int16_t stage_num, sshiftjis_t route[SCOREDAT_ROUTE_LEN + 1]
);
void far t1replay_terminal_save_request(void);

bool16 far t1replay_active(void);

enum t1replay_pause_action_t {
	T1RPA_RESUME = 0,
	T1RPA_RESTART = 1,
	T1RPA_SAVE_EXIT = 2,
	T1RPA_DISCARD_EXIT = 3,
};

// Patch-owned four-choice Pause surface. It records through the existing
// input_sense(false) seam and returns true only for a terminal action.
bool16 far t1replay_pause_menu(void);
bool16 far t1replay_pause_save_available(void);
bool16 far t1replay_pause_restart_available(void);
void far t1replay_pause_action_set(t1replay_pause_action_t action);

#endif /* TH01_REPLAY_HPP */
