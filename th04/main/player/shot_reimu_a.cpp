/// Reimu's shottype A, levels 2 to 9
/// ---------------------------------
/// Eight of the sixteen functions installed into [playchar_shot_func] out of
/// [playchar_shot_funcs]; reached only through that pointer, which is why the
/// dump publishes none of them.
///
/// Shottype A's option shots aim at [homing_target] whenever a boss or midboss
/// has published one, which is the whole difference from shottype B: the B
/// levels in shot_reimu_b.cpp fire theirs at fixed angles instead, and pay for
/// it with a generated jump table each. Not one of these eight compiles a
/// switch statement.
///
/// (#included from th04/player_b.cpp, between shot_reimu.cpp and
/// shot_reimu_b.cpp -- the address order the original has.
/// kb/codegen 0099 + 0112 + 0114.)

#include "th04/main/player/shot_reimu.hpp"

extern "C" {

// The shared prologue of all eight: one round of shots is [count] of the
// player's own, plus two option shots on the rounds where [shot_reimu_cycle]
// divides evenly. Declared as a macro because [i] and [shot] have to be the
// first two locals for the register allocation to come out right, and because
// the secondary count is added under a condition rather than in the
// initializer.
#define shot_reimu_a_init(shot, i, count, cycle_divisor) \
	Shot near *shot; \
	int i = count; \
	\
	if(shot_time == SHOT_CYCLE_FRAMES) { \
		shot_reimu_cycle = 0; \
	} \
	if((shot_reimu_cycle % cycle_divisor) == 0) { \
		i += 2; \
	} \
	shot_reimu_cycle++;

// The option half, shared by every level from 2 to 8. [i_left] is the count at
// which the shot goes to the left option; anything else goes to the right one.
#define shot_reimu_a_option(shot, i, i_left) \
	if(i == i_left) { \
		shot->from_option_l(); \
	} else { \
		shot->from_option_r(); \
	} \
	if(homing_target.y.v != Subpixel::None()) { \
		shot_velocity_set_homing(shot, (randring1_next16_and(7) - 4)); \
	} \
	shot->patnum_base = PAT_SHOT_REIMU_SUB_A;

void pascal near shot_reimu_a_l2(void)
{
	shot_reimu_a_init(shot, i, 1, 3);
	shot_ptr = shots;
	shot_last_id = 0;
	while(( shot = shots_add() ) != nullptr) {
		if(i == 1) {
			shot_velocity_set(
				&shot->pos.velocity, randring1_next8_ge_lt(-0x48, -0x38)
			);
			shot->patnum_base = PAT_SHOT_REIMU;
		} else {
			shot_reimu_a_option(shot, i, 3);
		}
		shot->damage = 10;
		if(--i <= 0) {
			break;
		}
	}
}

void pascal near shot_reimu_a_l3(void)
{
	shot_reimu_a_init(shot, i, 2, 3);
	shot_ptr = shots;
	shot_last_id = 0;
	while(( shot = shots_add() ) != nullptr) {
		if(i <= 2) {
			if(i == 2) {
				shot->pos.cur.x -= 8;
			} else {
				shot->pos.cur.x += 8;
			}
			shot->patnum_base = PAT_SHOT_REIMU;
			shot->damage = 9;
		} else {
			shot_reimu_a_option(shot, i, 4);
			shot->damage = 10;
		}
		if(--i <= 0) {
			break;
		}
	}
}

// Levels 4 to 6 are one shape: a fan of [i] own shots walking [angle] by a
// fixed step, and the option pair behind it.
#define SHOT_REIMU_A_FAN(angle_init, cycle_divisor, step, dmg_own, dmg_option) \
	shot_reimu_a_init(shot, i, 3, cycle_divisor); \
	unsigned char angle = angle_init; \
	\
	shot_ptr = shots; \
	shot_last_id = 0; \
	while(( shot = shots_add() ) != nullptr) { \
		if(i <= 3) { \
			shot_velocity_set(&shot->pos.velocity, angle); \
			shot->patnum_base = PAT_SHOT_REIMU; \
			shot->damage = dmg_own; \
			angle += step; \
		} else { \
			shot_reimu_a_option(shot, i, 5); \
			shot->damage = dmg_option; \
		} \
		if(--i <= 0) { \
			break; \
		} \
	}

void pascal near shot_reimu_a_l4(void)
{
	SHOT_REIMU_A_FAN(-0x46, 3, 0x06, 8, 9);
}

void pascal near shot_reimu_a_l5(void)
{
	SHOT_REIMU_A_FAN(-0x48, 2, 0x08, 8, 9);
}

void pascal near shot_reimu_a_l6(void)
{
	SHOT_REIMU_A_FAN(-0x48, 2, 0x08, 7, 8);
}

// Levels 7 and 8 add a middle pair of own shots at fixed angles, between the
// fan and the option pair.
#define SHOT_REIMU_A_MID(shot, i, angle_2) \
	if(i == 5) { \
		shot->from_option_l(); \
		angle_2 = -0x48; \
	} else { \
		shot->from_option_r(); \
		angle_2 = -0x38; \
	} \
	shot_velocity_set(&shot->pos.velocity, angle_2); \
	shot->patnum_base = PAT_SHOT_REIMU;

void pascal near shot_reimu_a_l7(void)
{
	shot_reimu_a_init(shot, i, 5, 2);
	unsigned char angle_1 = -0x46;
	unsigned char angle_2;

	shot_ptr = shots;
	shot_last_id = 0;
	while(( shot = shots_add() ) != nullptr) {
		if(i <= 3) {
			shot_velocity_set(&shot->pos.velocity, angle_1);
			shot->patnum_base = PAT_SHOT_REIMU;
			// ZUN bloat: immediately overwritten, and it is the only place in
			// the family where a damage value is written twice.
			shot->damage = 4;
			shot->damage = 7;
			angle_1 += 0x06;
		} else if(i <= 5) {
			SHOT_REIMU_A_MID(shot, i, angle_2);
			shot->damage = 7;
		} else {
			shot_reimu_a_option(shot, i, 7);
			shot->damage = 7;
		}
		if(--i <= 0) {
			break;
		}
	}
}

// ZUN quirk: the only one of the sixteen that neither resets nor reads
// [shot_reimu_cycle]. Both of the tests the other fifteen make are gone, so the
// secondary count is added unconditionally and every round of this level fires
// the same nine shots -- but the increment stayed, so it still shifts the phase
// the next pattern sees.
void pascal near shot_reimu_a_l8(void)
{
	Shot near *shot;
	int i = 5;
	unsigned char angle_1;
	unsigned char angle_2;

	// Assigned here rather than in the declaration above, which is where the
	// original puts it: Turbo C++ emits a declaration's initializer at the
	// declaration, so `unsigned char angle_1 = -0x46;` moves this store ahead
	// of the two statements below and costs three instruction slots.
	i += 2;
	shot_reimu_cycle++;
	angle_1 = -0x46;
	shot_ptr = shots;
	shot_last_id = 0;
	while(( shot = shots_add() ) != nullptr) {
		if(i <= 3) {
			shot_velocity_set(&shot->pos.velocity, angle_1);
			shot->patnum_base = PAT_SHOT_REIMU;
			shot->damage = 7;
			angle_1 += 0x06;
		} else if(i <= 5) {
			SHOT_REIMU_A_MID(shot, i, angle_2);
			shot->damage = 7;
		} else {
			shot_reimu_a_option(shot, i, 7);
			shot->damage = 7;
		}
		if(--i <= 0) {
			break;
		}
	}
}

// The widest of the eight: four kinds of shot in one round, and the only one
// whose option shots have a fixed angle to fall back on when no target exists.
void pascal near shot_reimu_a_l9(void)
{
	Shot near *shot;
	int i = 5;
	unsigned char angle_1;
	unsigned char angle_2;

	i += 2;
	if(shot_time == SHOT_CYCLE_FRAMES) {
		shot_reimu_cycle = 0;
	}
	if((shot_reimu_cycle % 2) == 0) {
		i += 2;
	}
	shot_reimu_cycle++;
	angle_1 = -0x46;
	shot_ptr = shots;
	shot_last_id = 0;
	while(( shot = shots_add() ) != nullptr) {
		if(i <= 3) {
			shot_velocity_set(&shot->pos.velocity, angle_1);
			shot->patnum_base = PAT_SHOT_REIMU;
			shot->damage = 6;
			angle_1 += 0x06;
		} else if(i <= 5) {
			SHOT_REIMU_A_MID(shot, i, angle_2);
			shot->damage = 7;
		} else if(i <= 7) {
			if(i == 7) {
				shot->from_option_l();
				angle_2 = -0x4C;
			} else {
				shot->from_option_r();
				angle_2 = -0x34;
			}
			if(homing_target.y.v != Subpixel::None()) {
				shot_velocity_set_homing(shot, (randring1_next16_and(7) - 4));
			} else {
				shot_velocity_set(&shot->pos.velocity, angle_2);
			}
			shot->patnum_base = PAT_SHOT_REIMU_SUB_A;
			shot->damage = 5;
		} else {
			if(i == 9) {
				shot->from_option_l();
			} else {
				shot->from_option_r();
			}
			if(homing_target.y.v != Subpixel::None()) {
				shot_velocity_set_homing(shot, 0);
			}
			shot->patnum_base = PAT_SHOT_REIMU_SUB_A;
			shot->damage = 7;
		}
		if(--i <= 0) {
			break;
		}
	}
}

}
