#ifndef TH01_RPRESENT_HPP
#define TH01_RPRESENT_HPP

#include "platform.h"
#include "th01/replay_format.hpp"

#if T1REPLAY_CHECKPOINT_RESTORE || T1REPLAY_PIXEL_TRACE
// These paint only the already-imported semantic checkpoint state. They never
// advance gameplay, sample input, allocate, or touch the input history.
bool16 t1replay_ckpt_player_paint_valid(
	const t1replay_checkpoint_player_t far *checkpoint
);
bool16 t1replay_ckpt_orb_paint_valid(
	const t1replay_checkpoint_orb_t far *checkpoint
);
bool16 t1replay_player_checkpoint_paint(
	const t1replay_checkpoint_player_t far *checkpoint
);
bool16 t1replay_orb_checkpoint_paint(
	const t1replay_checkpoint_orb_t far *checkpoint
);
#endif

#endif /* TH01_RPRESENT_HPP */
