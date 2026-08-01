#ifndef TH03_OP_T3CASE_HPP
#define TH03_OP_T3CASE_HPP

#include "platform.h"

// Demo-driven T3CASE playback has no Replay Patch menu to restore the replay
// scenario before MAINL. If T3CASE.CFG selects playback, validate T3CASE.BIN
// and apply its resident startup fields before MAINL stages gameplay sprites.
void far t3case_op_scenario_apply(void);

#endif /* TH03_OP_T3CASE_HPP */
