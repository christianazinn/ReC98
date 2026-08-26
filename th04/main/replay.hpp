#ifndef TH04_MAIN_REPLAY_HPP
#define TH04_MAIN_REPLAY_HPP

#include "platform.h"
#include "th04/replay_format.hpp"

// Validates one semantic checkpoint target against the complete per-game
// chapter, actor, and phase domain. Checkpoint decode shares this validator
// with replay-header and command admission.
bool replay_checkpoint_identity_valid(
	const replay_start_config_t far *start
);

// Called immediately after the validation oracle's early MAIN hook. Consumes
// the one-shot OP command, opens the selected replay, and applies playback's
// semantic start before native stage setup derives any state from it.
void replay_entry(void);

// True only while a direct Practice start still needs the first-run native
// initialization path. Ordinary stage progression never takes this branch.
bool replay_practice_run_start_requested(void);
bool replay_stage_is_first(uint8_t stage);

// Applies the portable Practice resources after stage_init() has reset
// per-stage state and before shot-level and HUD derivation consume them.
void replay_practice_start_apply_after_reset(void);
void replay_practice_start_apply_and_stage_activate(void);

// Finishes fields reset by items_init() and consumes the one-shot Practice
// startup request.
void replay_practice_items_ready(void);

// Captures the current arbitrary Practice boundary into the normalized
// checkpoint sidecar. The caller must still be in hidden native preroll; input
// recording begins only after this succeeds.
bool replay_practice_checkpoint_capture(void);

// Arbitrary Practice target setup and its next-frame boundary detector.
// Chapters and midbosses reconstruct field state directly; boss phases retain
// hidden native preroll because their actor-private state has no generic
// constructor. The stage loop masks video only while either path is pending.
bool replay_practice_preroll_active(void);
bool replay_practice_preroll_boundary(void);
bool replay_practice_direct_redraw_take(void);

// Private emulator-test runs use a fixed gameplay suffix. Suppressing player
// hits keeps late-stage targets from entering post-game input polling before
// that suffix reaches its deterministic terminal.
bool replay_private_test_active(void);

// Called after native stage setup and before the first frame. Emits or checks
// the stage-start control packet at a boundary where stage_id is final.
void replay_stage_start(void);

// Tail helpers that make room for replay hooks without growing position-
// critical stock MAIN segments.
void replay_main_entry_setup(void);
void replay_game_init_main_or_exit(const unsigned char far *pf_fn);
bool replay_frame_pacing_should_delay(void);
bool replay_stage_frame_advance_should_raise(void);
void replay_metrics_commit(void);

// Called after input_sense() and demo_update() at the canonical gameplay
// input seam. Playback replaces the resulting full input_t plus Shift before
// Pause or gameplay can consume them. Recording is deferred to the frame-tail
// hook below so it captures the post-Pause input that gameplay actually used.
void replay_gameplay_input(void);

// Records the gameplay input after Pause has drained its own controls, then
// performs the native frame-tail keyboard sample. The latter is not a logical
// replay sample, and playback must prevent host input from leaking through it.
void replay_input_reset_sense_tail(void);

// Replay-aware forms for MAIN's blocking dialog and continue input. They
// preserve each game's native sensing function and then record or override
// the value at the point where the caller consumes it.
void replay_input_reset_sense_interstitial(void);
void replay_input_sense_interstitial(void);
int16_t replay_input_reset_sense_held_interstitial(void);
void pascal replay_input_wait_for_change(int frames);

// Called as the first operation of GameExecl(). Finalizes a recording or
// verifies playback's terminal control. Returns true when playback must go
// directly to OP instead of entering MAINE.
bool replay_process_end(void);

bool replay_active(void);
bool replay_playback_active(void);

#endif /* TH04_MAIN_REPLAY_HPP */
