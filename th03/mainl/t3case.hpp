#ifndef TH03_MAINL_T3CASE_HPP
#define TH03_MAINL_T3CASE_HPP

// Validation-only T3CASE process adapter. MAINL polls physical input in tight
// presentation loops, but the Replay Patch records one logical phase-1 sample
// per hardware VSync. This wrapper retains that temporal boundary.
void far t3case_mainl_session_start(void);
void far t3case_mainl_input_mode_interface(void);

// Cutscene presentation timing intentionally differs across validation
// lineages. Consume the recorded phase-1 stream at the ordinary VSync cadence
// but expose Cancel continuously so the script interpreter fast-forwards
// instead of coupling gameplay validation to presentation-frame alignment.
void far t3case_mainl_cutscene_input_mode_interface(void);

// Replay-aware counterparts for the cutscene interpreter's nested blocking
// waits. These must sample through the same once-per-VSync adapter as the
// interpreter's top-level input poll.
void far t3case_mainl_input_wait_for_change(int frames);
bool16 far t3case_mainl_input_wait_for_ok(unsigned int frames);
bool16 far t3case_mainl_input_wait_for_ok_or_measure(
	int measure, unsigned int frames
);

// Drains the remainder of this MAINL process, consumes exactly MAINL_END, and
// stores the cumulative cursor for the next MAIN. Safe to call more than once.
void far t3case_mainl_transition_finish(void);

// Same process-control consumption for a natural MAINL-to-OP terminal route.
// Remaining payload is an error on this path.
void far t3case_mainl_terminal_finish(void);

// Latch a playback terminal before name registration blocks on physical input.
// Recording and ordinary game sessions are left untouched.
void far t3case_mainl_playback_terminal_latch(void);

#endif /* TH03_MAINL_T3CASE_HPP */
