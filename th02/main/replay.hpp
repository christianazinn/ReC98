#ifndef TH02_MAIN_REPLAY_HPP
#define TH02_MAIN_REPLAY_HPP

#include "platform.h"
#include "th02/replay_format.hpp"

// Invoked after MAIN has loaded OP's resident block and before gameplay_init()
// derives any state from it. Consumes one strict, one-shot T2RCFG2 command.
void replay_entry(void);

// Called after stage_init() has fixed [stage_id] and before any frame can
// consume input. Emits or verifies the stage boundary control packet.
void replay_stage_start(void);

// Applies the one-shot clean-Practice recipe after stage_init() and physical
// scroll-page initialization. Returns false rather than approximating a
// malformed or unavailable target.
bool16 replay_practice_target_apply(void);

// Owns TH02's two physical-page scroll-line values. The source loop starts
// both from its original packed initializer and continues to address them by
// the physical back-page index.
void replay_scroll_pages_reset(long packed_initial_lines);
int16_t replay_scroll_page_line_get(uint8_t page);
void replay_scroll_page_line_set(uint8_t page, int16_t line);

// Captures and validates the private semantic checkpoint vocabulary at the
// first frame of a stage loop. It deliberately has no restore or file-write
// behavior until every actor and world group has an exact codec.
void replay_checkpoint_capture_validate(void);

enum t2rec_reject_t {
	T2REC_DEFERRED_CODECS = 0,
	T2REC_NULL_ENVELOPE,
	T2REC_BOUNDARY_NOT_LOOP_TOP,
	T2REC_BOUNDARY_STAGE_INIT,
	T2REC_BOUNDARY_INPUT_SAMPLED,
	T2REC_BOUNDARY_PAUSE,
	T2REC_BOUNDARY_PRESENTATION,
	T2REC_BOUNDARY_RESTORE_OR_REDRAW,
	T2REC_BOUNDARY_STAGE_PROGRESSION,
	T2REC_HEADER,
	T2REC_DIRECTORY,
	T2REC_TAG,
	T2REC_CHECKSUM,
};

// The future exact hook constructs this descriptor at the top of a normal
// stage_loop() iteration. It is intentionally caller-owned here: this parcel
// must not turn the diagnostic stage_loop() entry capture into a live seek
// boundary.
struct t2rec_boundary_t {
	uint8_t at_ordinary_stage_loop_top;
	uint8_t stage_init_complete;
	uint8_t input_sampled;
	uint8_t pause_or_debounce_active;
	uint8_t blocking_presentation_active;
	uint8_t restore_or_redraw_active;
	uint8_t stage_progression;
};

bool replay_exact_checkpoint_boundary_available(
	const struct t2rec_boundary_t *boundary,
	enum t2rec_reject_t *reason
);

// Validation is transactional: it reads only [envelope] and [boundary], never
// changes gameplay state, and always rejects schema 1 as codec work is absent.
enum t2rec_reject_t replay_exact_checkpoint_validate(
	const uint8_t far *envelope, uint32_t envelope_size,
	const struct t2rec_boundary_t *boundary
);

// Builds only the registered schema-2 Stage 5 Mima capture. Its stage-FX,
// tile-logic, palette, callback, and redraw groups remain explicitly deferred,
// so this is not a live seek hook or an apply entry point.
bool16 replay_exact_stage5_mima_capture(
	uint8_t far *envelope, uint32_t envelope_size,
	const struct t2rec_boundary_t *boundary
);

// Builds only the registered schema-3 Stage 5 Mima capture. It adds the
// typed logical TILE_LOGIC payload but still defers Stage FX, palette,
// callbacks, and redraw; it is not a live seek hook or an apply entry point.
bool16 replay_exact_stage5_mima_tile_capture(
	uint8_t far *envelope, uint32_t envelope_size,
	const struct t2rec_boundary_t *boundary
);

// Builds only the registered schema-4 Stage 5 Mima capture. It additionally
// proves the generic background-particle owner is quiescent, but palette,
// callbacks, and redraw remain deferred; it is not a live seek hook or an
// apply entry point.
bool16 replay_exact_stage5_mima_stage_fx_capture(
	uint8_t far *envelope, uint32_t envelope_size,
	const struct t2rec_boundary_t *boundary
);

// Builds only the registered schema-5 Stage 5 Mima capture. It adds the
// bounded semantic PALETTE payload but still defers callbacks and redraw; it
// is not a live seek hook or an apply entry point.
bool16 replay_exact_stage5_mima_palette_capture(
	uint8_t far *envelope, uint32_t envelope_size,
	const struct t2rec_boundary_t *boundary
);

// Builds only the registered schema-6 Stage 5 Mima capture. CALLBACKS and
// REDRAW are semantic recipe bytes, but apply and public seek remain deferred.
bool16 replay_exact_stage5_mima_callback_redraw_capture(
	uint8_t far *envelope, uint32_t envelope_size,
	const struct t2rec_boundary_t *boundary
);

#if T2REPLAY_EXACT_APPLY
// Read-only bridge used while preparing the common owner from the ordered
// payload pointers in a schema-6 envelope.
bool16 far replay_checkpoint_common_groups_valid(
	const uint8_t far *group[]
);
#endif

// Called immediately after one of MAIN's native input_reset_sense() calls.
// The phase identifies the existing consumer; it is not a synthetic frame.
void replay_input_sample(uint8_t phase);

// Restores Practice's fixed dynamic rank after native paths that assign or
// increment [playperf]. No-op outside Rank Lock sessions.
void replay_rank_lock_apply(void);

// Replay-aware equivalent of key_delay() for blocking gameplay presentation.
// Returns false when playback has failed or the host requested cancellation.
bool replay_input_wait_for_change(void);

// Pause terminal actions deliberately reuse T2RPY1's existing GAME_OVER
// terminal record. The pending-save sidecar is therefore unchanged.
bool replay_pause_save_available(void);
bool replay_pause_save_refresh(void);
bool replay_pause_restart_semantics(void);
bool replay_pause_restart_available(void);
bool replay_pause_restart(void);
bool replay_pause_save_and_exit(void);
void replay_pause_exit_without_saving(void);

// Terminalizes a non-clear attempt before the native GAME OVER / Continue
// screen. Returns true when replay playback must bypass that live UI.
bool replay_gameover(void);

// Terminalizes a clear or validates a playback terminal immediately before
// GameExecl() tears the current process down. Returns true to launch OP.
bool replay_process_end(const char *binary_fn);

// The post-registration wrapper asks at most once per MAIN process. A malformed
// sidecar is discarded without making its capture actionable.
bool replay_save_request_prompt_needed(void);
void replay_save_request_discard(void);

bool replay_playback_active(void);
bool replay_playback_exit_requested(void);

#endif /* TH02_MAIN_REPLAY_HPP */
