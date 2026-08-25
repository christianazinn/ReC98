#ifndef TH04_MAIN_REPLAY_HPP
#define TH04_MAIN_REPLAY_HPP

#include "platform.h"

// Called immediately after the validation oracle's early MAIN hook. Consumes
// the one-shot OP command, opens the selected replay, and applies playback's
// semantic start before native stage setup derives any state from it.
void replay_entry(void);

// True only while a direct Practice start still needs the first-run native
// initialization path. Ordinary stage progression never takes this branch.
bool replay_practice_run_start_requested(void);

// Applies the portable Practice resources after stage_init() has reset
// per-stage state and before shot-level and HUD derivation consume them.
void replay_practice_start_apply_after_reset(void);

// Finishes fields reset by items_init() and consumes the one-shot Practice
// startup request.
void replay_practice_items_ready(void);

// Captures the current arbitrary Practice boundary into the normalized
// checkpoint sidecar. The caller must still be in hidden native preroll; input
// recording begins only after this succeeds.
bool replay_practice_checkpoint_capture(void);

// Called after native stage setup and before the first frame. Emits or checks
// the stage-start control packet at a boundary where stage_id is final.
void replay_stage_start(void);

// Called after input_sense() and demo_update() at the canonical gameplay
// input seam. Record mode stores the resulting full input_t plus Shift;
// playback mode replaces them from the stream.
void replay_gameplay_input(void);

// The frame-loop tail samples the keyboard a second time for TH05's debug
// fast-forward toggle. It is not a logical replay sample, but playback must
// prevent host input from leaking through it.
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
