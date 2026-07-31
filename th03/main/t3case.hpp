#ifndef TH03_MAIN_T3CASE_HPP
#define TH03_MAIN_T3CASE_HPP

#include "platform.h"

// Everything the verifier does happens at or after this hook, which sits
// exactly where the Replay Patch puts `replay_session_start()`: after
// `round_startup()` and `farfp_20F20()`. Selects the mode from the resident
// handoff, falling back to T3CASE.CFG, resumes the cursor across MAIN
// processes, and captures or applies the startup description.
//
// Earlier hook positions were tried and are unsafe. Before `game_init_main()`
// master.lib and the game are not initialized; between
// `cfg_load_resident_ptr()` and `round_startup()` the packfile is open and
// master.lib's single-handle file state is shared with `pfint21`.
void far t3case_session_start(void);

// Normal round initialization has completed and no logical input has been
// consumed yet. Emits the reference split row.
void far t3case_round_start(void);

// Per frame, after `input_mode()` and before both `player_update()` calls.
void far t3case_frame_io(void);

void far t3case_route(uint8_t route);
void far t3case_finish(void);

#endif /* TH03_MAIN_T3CASE_HPP */
