#ifndef TH03_MAINL_T3CASE_HPP
#define TH03_MAINL_T3CASE_HPP

// Validation-only T3CASE process adapter. MAINL polls physical input in tight
// presentation loops, but the Replay Patch records one logical phase-1 sample
// per hardware VSync. This wrapper retains that temporal boundary.
void far t3case_mainl_session_start(void);
void far t3case_mainl_input_mode_interface(void);

// Drains the remainder of this MAINL process, consumes exactly MAINL_END, and
// stores the cumulative cursor for the next MAIN. Safe to call more than once.
void far t3case_mainl_transition_finish(void);

// Same process-control consumption for a natural MAINL-to-OP terminal route.
// Remaining payload is an error on this path.
void far t3case_mainl_terminal_finish(void);

#endif /* TH03_MAINL_T3CASE_HPP */
