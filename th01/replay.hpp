#ifndef TH01_REPLAY_HPP
#define TH01_REPLAY_HPP

#include "platform.h"
#include "th01/replay_format.hpp"

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

// The two accessors keep hidden owner state semantic and pointer-free. Import
// remains intentionally unused by the capture-only substrate.
void t1replay_input_checkpoint_export(t1replay_checkpoint_input_t *out);
void t1replay_input_checkpoint_import(const t1replay_checkpoint_input_t *in);

// Called immediately before REIIDEN hands control to another executable. A
// nonterminal handoff is always REIIDEN-to-REIIDEN; terminal paths remain
// native after their final replay control has been validated or written.
void far t1replay_process_end(bool16 terminal, uint8_t end_reason);

bool16 far t1replay_active(void);

#endif /* TH01_REPLAY_HPP */
