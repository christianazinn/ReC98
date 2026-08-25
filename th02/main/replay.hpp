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

// Called immediately after one of MAIN's native input_reset_sense() calls.
// The phase identifies the existing consumer; it is not a synthetic frame.
void replay_input_sample(uint8_t phase);

// Terminalizes a non-clear attempt before the native GAME OVER / Continue
// screen. Returns true when replay playback must bypass that live UI.
bool replay_gameover(void);

// Terminalizes a clear or validates a playback terminal immediately before
// GameExecl() tears the current process down. Returns true to launch OP.
bool replay_process_end(const char *binary_fn);

bool replay_playback_active(void);

#endif /* TH02_MAIN_REPLAY_HPP */
