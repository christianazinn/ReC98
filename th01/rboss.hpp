#ifndef TH01_RBOSS_HPP
#define TH01_RBOSS_HPP

#include "platform.h"
#include "th01/replay_format.hpp"

bool16 t1replay_checkpoint_boss_valid(
	const t1replay_checkpoint_boss_t far *boss
);
bool16 t1replay_checkpoint_world_valid(
	const t1replay_checkpoint_t far *checkpoint
);
bool16 t1replay_checkpoint_boss_capture(
	t1replay_checkpoint_boss_t far *boss
);
bool16 t1replay_checkpoint_boss_apply(
	const t1replay_checkpoint_boss_t far *boss
);

#if T1REPLAY_CHECKPOINT_RESTORE || T1REPLAY_PIXEL_TRACE
bool16 t1replay_ckpt_present_valid(
	const t1replay_checkpoint_t far *checkpoint
);
bool16 t1replay_ckpt_present_apply(
	const t1replay_checkpoint_t far *checkpoint
);
#endif

#endif /* TH01_RBOSS_HPP */
