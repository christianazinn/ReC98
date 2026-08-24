/// Reimu's shot control functions, shared parts and levels 0-1
/// -----------------------------------------------------------
/// The head of what th04_main.asm contributed to PLAYER_B_TEXT: the homing
/// helper both shottypes' option shots reach for, and the two levels that are
/// the same for shottype A and B (SHOT_FUNCS_REIMU_A and SHOT_FUNCS_REIMU_B
/// both point their first two entries here).
///
/// (#included from th04/player_b.cpp, first of the three shot_reimu*.cpp
/// bodies and therefore first in the object, which is the address order the
/// original has. kb/codegen 0099 + 0112 + 0114.)

#include "th04/main/player/shot_reimu.hpp"

extern "C" {

void pascal near shot_velocity_set_homing(
	Shot near *shot, unsigned char angle_offset
)
{
	angle_offset += iatan2(
		(homing_target.y - player_pos.cur.y),
		(homing_target.x - shot->pos.cur.x)
	);
	shot_velocity_set(&shot->pos.velocity, angle_offset);
}

void pascal near shot_reimu_l0(void)
{
	Shot near *shot;

	shot_ptr = shots;
	shot_last_id = 0;
	if(( shot = shots_add() ) != nullptr) {
		shot->patnum_base = PAT_SHOT_REIMU;
		shot->damage = 10;
	}
}

void pascal near shot_reimu_l1(void)
{
	Shot near *shot;

	shot_ptr = shots;
	shot_last_id = 0;
	if(( shot = shots_add() ) != nullptr) {
		shot_velocity_set(
			&shot->pos.velocity, randring1_next8_ge_lt(-0x44, -0x3C)
		);
		shot->patnum_base = PAT_SHOT_REIMU;
		shot->damage = 10;
	}
}

}
