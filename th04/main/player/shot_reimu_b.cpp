/// Reimu's shottype B, levels 2 to 9
/// ---------------------------------
/// Eight of the sixteen functions installed into [playchar_shot_func] out of
/// [playchar_shot_funcs]; reached only through that pointer, which is why the
/// dump publishes none of them.
///
/// Levels 5 to 9 all fire the same two kinds of shot in the same scan: a fan
/// of the player's own shots that walks [angle_1] by a fixed step, and then a
/// pair of option shots, one per side, whose angle comes out of a switch
/// statement on the remaining count -- and that statement is what compiles to
/// the `dw offset loc_...` run behind each `endp` in the original. Levels 2 to
/// 4 pick their option angle with a plain conditional instead, and compile no
/// table at all.
///
/// Where shottype A's option shots aim at [homing_target]
/// (th04/main/player/shot_reimu_a.cpp), B's take a fixed angle. That is the
/// whole difference between the two shottypes.
///
/// (#included from th04/player_b.cpp, last of the three shot_reimu*.cpp bodies
/// and ahead of bb_playchar.cpp and bomb.cpp -- the address order all five
/// bodies have in PLAYER_B_TEXT. kb/codegen 0099 + 0112 + 0114.)
///
/// state/re/JUMP_TABLE_TAILS.md's class, third TH04 row and the first that is
/// a CHAIN: draining shot_reimu_b_l9 only uncovers shot_reimu_b_l8's own
/// table, four times over. kb/codegen/0129 is why this is a separate file
/// rather than more of player_b.cpp: th04/main/player/shot.hpp's closure has
/// unguarded members that bb_playchar.cpp behind it must not re-expand.)
///
/// Assembly in TH05, and NOT the same shape: th05/p_reimu.cpp's shot_reimu_l*
/// derive which sub-round of the shot cycle they are in from
/// shot_cycle_init()'s SC_* bitflags, where TH04 counts the rounds itself in
/// [shot_reimu_cycle]. Related idea, different code.

// NO -a option, and that is a decision rather than an omission. The original
// puts no alignment byte between any of these five epilogues and the jump
// table behind it, and at the build's own alignment Turbo C++ emits none
// either -- `[measured]` with tcc -S over this object, zero `db N dup` in the
// whole listing. kb/codegen/0119 is the failure that avoids: a wrong alignment
// leaves every body byte-identical while silently inventing or dropping a pad,
// which a per-function funcdiff cannot see. Diff the whole segment and the
// map's contribution length.
//
// `[measured]` and worth recording, because this is the first FIVE-table
// object the campaign has compiled: adding -a2 here cannot reproduce the
// original at ANY object parity. Probed with a 0-, 1-, 2- and 3-byte
// #pragma codestring prefix, -a2 pads l6, l7 and l8 in all four, l9 in none of
// the four, and l5 only at an even prefix -- so no prefix makes the object
// uniform, which is what the dump needs. What that does to kb/codegen/0157's
// two-table corollary is in state/notes/th04-main-shot-reimu-b.md; it is
// reported there rather than folded into the entry on one object's evidence.

#include "th04/main/player/shot_reimu.hpp"

extern "C" {

// The x offset of an option shot from the player's center, which levels 3 and
// up apply only after the shot has been given its velocity -- so they hold it
// in a local rather than calling the Shot member from_option_l() or
// from_option_r() the way shottype A and level 2 do.
static const subpixel_t SHOT_OPTION_X = TO_SP(PLAYER_OPTION_DISTANCE);

#define shot_reimu_b_init(shot, i, count, cycle_divisor, secondary) \
	Shot near *shot; \
	int i = count; \
	subpixel_t x; \
	unsigned char angle_1; \
	unsigned char angle_2; \
	\
	if(shot_time == SHOT_CYCLE_FRAMES) { \
		shot_reimu_cycle = 0; \
	} \
	if((shot_reimu_cycle % cycle_divisor) == 0) { \
		i += secondary; \
	} \
	shot_reimu_cycle++;

// The two option shots' side, shared by every level that picks their angle
// with a plain conditional rather than a switch statement. [i_left] is the
// count at which the shot goes to the left option; anything else goes to the
// right one.
#define shot_reimu_b_option(i, i_left, x, angle) \
	if(i == i_left) { \
		x = -SHOT_OPTION_X; \
		angle = -0x48; \
	} else { \
		x = SHOT_OPTION_X; \
		angle = -0x38; \
	}

void pascal near shot_reimu_b_l2(void)
{
	Shot near *shot;
	int i = 1;
	unsigned char angle;

	if(shot_time == SHOT_CYCLE_FRAMES) {
		shot_reimu_cycle = 0;
	}
	if((shot_reimu_cycle % 3) == 0) {
		i += 2;
	}
	shot_reimu_cycle++;
	shot_ptr = shots;
	shot_last_id = 0;
	while(( shot = shots_add() ) != nullptr) {
		if(i == 1) {
			shot_velocity_set(
				&shot->pos.velocity, randring1_next8_ge_lt(-0x48, -0x38)
			);
			shot->patnum_base = PAT_SHOT_REIMU;
		} else {
			if(i == 3) {
				shot->from_option_l();
				angle = -0x48;
			} else {
				shot->from_option_r();
				angle = -0x38;
			}
			shot_velocity_set(&shot->pos.velocity, angle);
			shot->patnum_base = PAT_SHOT_REIMU_SUB_B;
		}
		shot->damage = 10;
		if(--i <= 0) {
			break;
		}
	}
}

void pascal near shot_reimu_b_l3(void)
{
	Shot near *shot;
	int i = 2;
	subpixel_t x;
	unsigned char angle;

	if(shot_time == SHOT_CYCLE_FRAMES) {
		shot_reimu_cycle = 0;
	}
	if((shot_reimu_cycle % 3) == 0) {
		i += 2;
	}
	shot_reimu_cycle++;
	shot_ptr = shots;
	shot_last_id = 0;
	while(( shot = shots_add() ) != nullptr) {
		if(i <= 2) {
			if(i == 2) {
				x = -TO_SP(8);
			} else {
				x = TO_SP(8);
			}
			shot->patnum_base = PAT_SHOT_REIMU;
		} else {
			shot_reimu_b_option(i, 4, x, angle);
			shot_velocity_set(&shot->pos.velocity, angle);
			shot->patnum_base = PAT_SHOT_REIMU_SUB_B;
		}
		shot->damage = 9;
		shot->pos.cur.x.v += x;
		if(--i <= 0) {
			break;
		}
	}
}

void pascal near shot_reimu_b_l4(void)
{
	shot_reimu_b_init(shot, i, 3, 2, 2);
	angle_1 = -0x46;
	shot_ptr = shots;
	shot_last_id = 0;
	while(( shot = shots_add() ) != nullptr) {
		if(i <= 3) {
			x = 0;
			shot_velocity_set(&shot->pos.velocity, angle_1);
			shot->patnum_base = PAT_SHOT_REIMU;
			shot->damage = 9;
			angle_1 += 0x06;
		} else {
			shot_reimu_b_option(i, 5, x, angle_2);
			shot_velocity_set(&shot->pos.velocity, angle_2);
			shot->patnum_base = PAT_SHOT_REIMU_SUB_B;
			shot->damage = 9;
		}
		shot->pos.cur.x.v += x;
		if(--i <= 0) {
			break;
		}
	}
}

void pascal near shot_reimu_b_l5(void)
{
	shot_reimu_b_init(shot, i, 3, 2, 4);
	angle_1 = -0x46;
	shot_ptr = shots;
	shot_last_id = 0;
	while(( shot = shots_add() ) != nullptr) {
		if(i <= 3) {
			x = 0;
			shot_velocity_set(&shot->pos.velocity, angle_1);
			shot->patnum_base = PAT_SHOT_REIMU;
			shot->damage = 8;
			angle_1 += 0x07;
		} else {
			if(i >= 6) {
				x = -SHOT_OPTION_X;
			} else {
				x = SHOT_OPTION_X;
			}
			switch(i) {
			case 7: angle_2 = -0x4E; break;
			case 6: angle_2 = -0x47; break;
			case 5: angle_2 = -0x32; break;
			case 4: angle_2 = -0x39; break;
			}
			shot_velocity_set(&shot->pos.velocity, angle_2);
			shot->patnum_base = PAT_SHOT_REIMU_SUB_B;
			shot->damage = 9;
		}
		shot->pos.cur.x.v += x;
		if(--i <= 0) {
			break;
		}
	}
}

void pascal near shot_reimu_b_l6(void)
{
	shot_reimu_b_init(shot, i, 3, 2, 4);
	angle_1 = -0x46;
	shot_ptr = shots;
	shot_last_id = 0;
	while(( shot = shots_add() ) != nullptr) {
		if(i <= 3) {
			x = 0;
			shot_velocity_set(&shot->pos.velocity, angle_1);
			shot->patnum_base = PAT_SHOT_REIMU;
			shot->damage = 8;
			angle_1 += 0x06;
		} else {
			if(i >= 6) {
				x = -SHOT_OPTION_X;
			} else {
				x = SHOT_OPTION_X;
			}
			switch(i) {
			case 7: angle_2 = -0x4E; break;
			case 6: angle_2 = -0x47; break;
			case 5: angle_2 = -0x32; break;
			case 4: angle_2 = -0x39; break;
			}
			shot_velocity_set(&shot->pos.velocity, angle_2);
			shot->patnum_base = PAT_SHOT_REIMU_SUB_B;
			shot->damage = 9;
		}
		shot->pos.cur.x.v += x;
		if(--i <= 0) {
			break;
		}
	}
}

// ZUN bloat: byte-for-byte the same function as shot_reimu_b_l6() above --
// same count, same angles, same step. Level 7 of this shottype fires exactly
// what level 6 does. Written out rather than aliased, because the original
// has two separate procs with two separate jump tables.
void pascal near shot_reimu_b_l7(void)
{
	shot_reimu_b_init(shot, i, 3, 2, 4);
	angle_1 = -0x46;
	shot_ptr = shots;
	shot_last_id = 0;
	while(( shot = shots_add() ) != nullptr) {
		if(i <= 3) {
			x = 0;
			shot_velocity_set(&shot->pos.velocity, angle_1);
			shot->patnum_base = PAT_SHOT_REIMU;
			shot->damage = 8;
			angle_1 += 0x06;
		} else {
			if(i >= 6) {
				x = -SHOT_OPTION_X;
			} else {
				x = SHOT_OPTION_X;
			}
			switch(i) {
			case 7: angle_2 = -0x4E; break;
			case 6: angle_2 = -0x47; break;
			case 5: angle_2 = -0x32; break;
			case 4: angle_2 = -0x39; break;
			}
			shot_velocity_set(&shot->pos.velocity, angle_2);
			shot->patnum_base = PAT_SHOT_REIMU_SUB_B;
			shot->damage = 9;
		}
		shot->pos.cur.x.v += x;
		if(--i <= 0) {
			break;
		}
	}
}

void pascal near shot_reimu_b_l8(void)
{
	shot_reimu_b_init(shot, i, 5, 2, 4);
	angle_1 = -0x48;
	shot_ptr = shots;
	shot_last_id = 0;
	while(( shot = shots_add() ) != nullptr) {
		if(i <= 5) {
			x = 0;
			shot_velocity_set(&shot->pos.velocity, angle_1);
			shot->patnum_base = PAT_SHOT_REIMU;
			shot->damage = 8;
			angle_1 += 0x04;
		} else {
			if(i >= 8) {
				x = -SHOT_OPTION_X;
			} else {
				x = SHOT_OPTION_X;
			}
			switch(i) {
			case 9: angle_2 = -0x4E; break;
			case 8: angle_2 = -0x47; break;
			case 7: angle_2 = -0x32; break;
			case 6: angle_2 = -0x39; break;
			}
			shot_velocity_set(&shot->pos.velocity, angle_2);
			shot->patnum_base = PAT_SHOT_REIMU_SUB_B;
			shot->damage = 9;
		}
		shot->pos.cur.x.v += x;
		if(--i <= 0) {
			break;
		}
	}
}

// The one of the five that is not the same shape: six option shots rather than
// four, their sides alternating with the count's parity rather than splitting
// it in half, and two of the six sharing an angle.
void pascal near shot_reimu_b_l9(void)
{
	shot_reimu_b_init(shot, i, 7, 2, 4);
	angle_1 = -0x48;
	shot_ptr = shots;
	shot_last_id = 0;
	while(( shot = shots_add() ) != nullptr) {
		if(i <= 5) {
			x = 0;
			shot_velocity_set(&shot->pos.velocity, angle_1);
			shot->patnum_base = PAT_SHOT_REIMU;
			shot->damage = 8;
			angle_1 += 0x04;
		} else {
			switch(i) {
			case 11:
			case 10: angle_2 = -0x40; break;
			case  9: angle_2 = -0x54; break;
			case  8: angle_2 = -0x2C; break;
			case  7: angle_2 = -0x4A; break;
			case  6: angle_2 = -0x36; break;
			}
			if(i & 1) {
				x = -SHOT_OPTION_X;
			} else {
				x = SHOT_OPTION_X;
			}
			shot_velocity_set(&shot->pos.velocity, angle_2);
			shot->patnum_base = PAT_SHOT_REIMU_SUB_B;
			shot->damage = 9;
		}
		shot->pos.cur.x.v += x;
		if(--i <= 0) {
			break;
		}
	}
}

}
