#ifndef TH01_REPLAY_FUUIN_HPP
#define TH01_REPLAY_FUUIN_HPP

#include "platform.h"
#include "th01/replay_format.hpp"

// Returns false when the launch marker and resident continuation disagree, or
// when continuation validation fails. This must run before end_init() destroys
// the resident handoff fields.
bool16 far t1replay_fuuin_entry(bool16 continuation_expected);
void far t1replay_fuuin_abort_to_op(void);

void far t1replay_fuuin_input_reset(void);
void far t1replay_fuuin_phase_begin(uint8_t phase);
void far t1replay_fuuin_frame_io(void);
int far t1replay_fuuin_key_sense(int keygroup);
void far t1replay_fuuin_terminal(void);
bool16 far t1replay_fuuin_active(void);
bool16 far t1replay_fuuin_playback(void);

#if T1REPLAY_FUUIN_SCORE_PROOF
void far t1replay_fuuin_score_table_unavailable(void);
void far t1replay_fuuin_score_table_before(
	const void far *names,
	const void far *scores,
	const void far *stages,
	const void far *routes
);
void far t1replay_fuuin_score_table_after(
	const void far *names,
	const void far *scores,
	const void far *stages,
	const void far *routes
);
#endif

#endif /* TH01_REPLAY_FUUIN_HPP */
