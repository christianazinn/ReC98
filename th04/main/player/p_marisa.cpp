/// Marisa's shot control functions, shared parts and levels 0-1
/// ------------------------------------------------------------
/// The head of what th04_main.asm contributed to EXECL_TEXT: the two levels
/// that are the same for shottype A and B (SHOT_FUNCS_MARISA_A and
/// SHOT_FUNCS_MARISA_B both point their first two entries here). Both are
/// byte-for-byte the same construct as Reimu's shot_reimu_l0() and
/// shot_reimu_l1() in th04/main/player/shot_reimu.cpp, with Marisa's sprite
/// bank substituted -- the two shottypes only diverge from level 2 onwards.
///
/// (#included from th04/p_marisa.cpp, which is the object that occupies the
/// kb/codegen/0080 P_MARISA_TEXT anchor carved off the HEAD of EXECL_TEXT's
/// root contribution. Further lifts APPEND to that file's include list rather
/// than prepending, because this anchor sits at the segment's head where
/// th04/player_b.cpp's sits at its tail. kb/codegen 0080 + 0099 + 0114.)

#include "th03/math/randring.hpp"
#include "th04/main/player/shot.hpp"
#include "th04/sprites/main_pat.h"

void pascal near shot_laser_update(
	unsigned int frames, shot_laser_style_t style
)
;

extern "C" {

void pascal near shot_marisa_l0(void)
{
	Shot near *shot;

	shot_ptr = shots;
	shot_last_id = 0;
	if(( shot = shots_add() ) != nullptr) {
		shot->patnum_base = PAT_SHOT_MARISA;
		shot->damage = 10;
	}
}

void pascal near shot_marisa_l1(void)
{
	Shot near *shot;

	shot_ptr = shots;
	shot_last_id = 0;
	if(( shot = shots_add() ) != nullptr) {
		shot_velocity_set(
			&shot->pos.velocity, randring1_next8_ge_lt(-0x44, -0x3C)
		);
		shot->patnum_base = PAT_SHOT_MARISA;
		shot->damage = 10;
	}
}

}
