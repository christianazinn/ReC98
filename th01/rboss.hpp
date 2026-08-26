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

#endif /* TH01_RBOSS_HPP */
