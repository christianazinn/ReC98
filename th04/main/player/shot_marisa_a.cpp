/// Marisa's shottype A, levels 2 to 9
/// ----------------------------------
/// Eight of the sixteen functions installed into [playchar_shot_func] out of
/// [playchar_shot_funcs]; reached only through that pointer, which is why the
/// dump publishes none of them.
///
/// Shottype A IS the option laser: these eight are the only callers of
/// shot_laser_update() in either dump, and they differ from each other in
/// nothing but the laser's duration and style and the fan of the player's own
/// shots fired underneath it. `[measured]` by a census, before the lift, of
/// every call to it anywhere in th04_main.asm: eight, all inside these bodies,
/// none anywhere else. Shottype B fires fixed-angle option shots
/// instead and never starts a laser, which is the whole difference between
/// Marisa's two shottypes, and the mirror of the homing/fixed-angle split
/// between Reimu's.
///
/// Not one of these eight compiles a switch statement, so none has a jump
/// table behind its `endp` and kb/codegen/0119's parity question does not
/// arise until shottype B.
///
/// (#included from th04/p_marisa.cpp, after p_marisa.cpp -- the address order
/// the original has. This object occupies a kb/codegen/0080 anchor at the HEAD
/// of its segment, so its include list runs in ASCENDING address order and
/// each further lift appends to the end of it, the mirror of
/// th04/player_b.cpp.)

#include "th03/math/randring.hpp"
#include "th04/main/player/shot.hpp"
#include "th04/sprites/main_pat.h"

void pascal near shot_laser_update(
	unsigned int frames, shot_laser_style_t style
);

extern "C" {

// Levels 3 and 4 fire their two shots half a sprite to either side of the
// position shots_add() already gave them.
static const subpixel_t SHOT_FAN_X = TO_SP(8);

// Levels 3 and 4 differ in nothing but the laser they start.
#define shot_marisa_a_pair(shot, i, laser_frames, laser_style) \
	Shot near *shot; \
	int i = 2; \
	\
	shot_laser_update(laser_frames, laser_style); \
	shot_ptr = shots; \
	shot_last_id = 0; \
	while(( shot = shots_add() ) != nullptr) { \
		if(i == 2) { \
			shot->pos.cur.x.v -= SHOT_FAN_X; \
		} else { \
			shot->pos.cur.x.v += SHOT_FAN_X; \
		} \
		shot->patnum_base = PAT_SHOT_MARISA; \
		shot->damage = 9; \
		if(--i <= 0) { \
			break; \
		} \
	}

// Levels 5 to 9 walk one angle across their whole fan by a fixed step. The
// count, the first angle, the step, the damage and the laser are the only
// things that change between the five.
#define shot_marisa_a_fan( \
	shot, i, angle, count, laser_frames, laser_style, angle_first, dmg, step \
) \
	Shot near *shot; \
	int i = count; \
	unsigned char angle; \
	\
	shot_laser_update(laser_frames, laser_style); \
	angle = angle_first; \
	shot_ptr = shots; \
	shot_last_id = 0; \
	while(( shot = shots_add() ) != nullptr) { \
		shot_velocity_set(&shot->pos.velocity, angle); \
		shot->patnum_base = PAT_SHOT_MARISA; \
		shot->damage = dmg; \
		angle += step; \
		if(--i <= 0) { \
			break; \
		} \
	}

void pascal near shot_marisa_a_l2(void)
{
	Shot near *shot;

	shot_laser_update(64, SLS_2);
	shot_ptr = shots;
	shot_last_id = 0;
	while(( shot = shots_add() ) != nullptr) {
		shot_velocity_set(
			&shot->pos.velocity, randring1_next8_ge_lt(-0x44, -0x3C)
		);
		shot->patnum_base = PAT_SHOT_MARISA;
		shot->damage = 9;
		break;
	}
}

void pascal near shot_marisa_a_l3(void)
{
	shot_marisa_a_pair(shot, i, 72, SLS_2);
}

void pascal near shot_marisa_a_l4(void)
{
	shot_marisa_a_pair(shot, i, 88, SLS_4);
}

void pascal near shot_marisa_a_l5(void)
{
	shot_marisa_a_fan(shot, i, angle, 3, 104, SLS_4, -0x48, 8, 8);
}

void pascal near shot_marisa_a_l6(void)
{
	shot_marisa_a_fan(shot, i, angle, 3, 128, SLS_6, -0x48, 8, 8);
}

void pascal near shot_marisa_a_l7(void)
{
	shot_marisa_a_fan(shot, i, angle, 3, 144, SLS_1_4_1, -0x48, 8, 8);
}

void pascal near shot_marisa_a_l8(void)
{
	shot_marisa_a_fan(shot, i, angle, 5, 168, SLS_1_4_1, -0x4C, 7, 6);
}

void pascal near shot_marisa_a_l9(void)
{
	shot_marisa_a_fan(shot, i, angle, 5, 192, SLS_8, -0x4C, 7, 6);
}

}
