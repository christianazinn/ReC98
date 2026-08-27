// TH02 Stage 3 direct-Practice target construction. This patch-only tail
// keeps the native boss object and every preceding patch segment stable.
#pragma option -zCT2S3PRACT_TEXT -G-

#include "platform.h"
#include "pc98.h"
#include "th02/core/globals.hpp"
#include "th02/main/boss/boss.hpp"
#include "th02/main/s3_actor.hpp"
#include "th02/main/s3_pract.hpp"
#include "th02/main/stage/stage.hpp"

extern "C" int boss_pos_x;
extern "C" int boss_pos_y;

bool16 far th02_s3_stones_clean_init(
	th02_s3_stones_clean_target_t target
)
{
	th02_s3_stones_state_t state;

	if(stage_id != 2) {
		return false;
	}
	switch(target) {
	case T2S3_STONES_BOSS_START:
	case T2S3_STONES_INNER_PAIR:
	case T2S3_STONES_OUTER_PAIR:
		break;
	default:
		return false;
	}

	th02_s3_stones_clean_base_init();
	if(target == T2S3_STONES_BOSS_START) {
		return true;
	}
	if(!th02_s3_stones_state_capture(&state)) {
		return false;
	}

	state.flag[STONE_INNER_WEST] = SF_ACTIVE;
	state.flag[STONE_INNER_EAST] = SF_ACTIVE;
	state.patnum[STONE_INNER_WEST] = 155;
	state.patnum[STONE_INNER_EAST] = 155;
	state.phase = 1;
	state.pattern = 0;
	state.timeout_frame = 64;
	state.phase_frame_unused = 0;
	if(target == T2S3_STONES_OUTER_PAIR) {
		state.flag[STONE_INNER_WEST] = SF_REMOVED;
		state.flag[STONE_INNER_EAST] = SF_REMOVED;
		state.flag[STONE_OUTER_WEST] = SF_ACTIVE;
		state.flag[STONE_OUTER_EAST] = SF_ACTIVE;
		state.patnum[STONE_OUTER_WEST] = 159;
		state.patnum[STONE_OUTER_EAST] = 159;
		state.phase = 2;
		state.timeout_frame = 0;
	}
	if(!th02_s3_stones_state_apply(&state)) {
		return false;
	}

	boss_phase_frame = 0;
	boss_damage = 0;
	if(target == T2S3_STONES_INNER_PAIR) {
		boss_pos_x = (state.left[STONE_INNER_WEST] + 8);
		boss_pos_y = (state.top[STONE_INNER_WEST] + 8);
	} else {
		boss_pos_x = (state.left[STONE_OUTER_WEST] + 8);
		boss_pos_y = (state.top[STONE_OUTER_WEST] + 8);
	}
	return true;
}
