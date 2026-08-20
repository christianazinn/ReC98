/// Reimu's shottype B, levels 5 to 9
/// ---------------------------------
/// Five of the sixteen functions installed into [playchar_shot_func] out of
/// [playchar_shot_funcs]; reached only through that pointer, which is why the
/// dump publishes none of them.
///
/// Every one of the five fires the same two kinds of shot in the same scan:
/// a fan of the player's own shots that walks [angle_1] by a fixed step, and
/// then a pair of option shots, one per side, whose angle comes out of a
/// switch statement on the remaining count, and that statement is what
/// compiles to the `dw offset loc_...` run behind each `endp` in the
/// original.
///
/// (#included from th04/player_b.cpp at the very FRONT of it, ahead of
/// bb_playchar.cpp and bomb.cpp -- the address order all three bodies have in
/// PLAYER_B_TEXT. These five procs and their five jump tables were the last
/// thing th04_main.asm contributed to that segment, so the object grows
/// backwards into the hole and every byte above it keeps its address
/// (kb/codegen 0099 + 0112 + 0114).
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

#include "th04/main/player/shot.hpp"
#include "th04/sprites/main_pat.h"

extern "C" {

// Counts the rounds of shots already fired within the current shot cycle: 0
// on the round at [shot_time] == SHOT_CYCLE_FRAMES, which is where it is
// reset, then 1 and 2 on the two further rounds at the cycle's ⅓ and ⅔ points.
// Reimu's patterns divide it to fire their option shots less often than every
// round -- `% 3` for once per cycle, `% 2` for twice.
//
// [inferred] name, [measured] role and population: the sixteen
// shot_reimu_{a,b}_l{2..9} procs are the only code in either dump that touches
// this byte, Marisa's sixteen use lasers instead, and the spelling follows
// TH02's [shot_c_cycle] (th02/main/player/shot.cpp), the identical construct
// -- a uint8_t bumped once per shot-control call and read `% N` to gate the
// option half. Evidence and the full population: state/notes/th04-shot-cycle-counter.md.
//
// ZUN quirk, worth knowing before reading the other fifteen: shot_reimu_a_l8
// increments this byte and neither resets nor reads it, so its rounds all fire
// the same shots and it only shifts the phase the next pattern sees.
extern uint8_t shot_reimu_cycle;

// The x offset of an option shot from the player's center, which these
// patterns apply only after the shot has been given its velocity.
static const subpixel_t SHOT_OPTION_X = TO_SP(PLAYER_OPTION_DISTANCE);

#define shot_reimu_b_init(shot, i, count, cycle_divisor) \
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
		i += 4; \
	} \
	shot_reimu_cycle++;

void pascal near shot_reimu_b_l5(void)
{
	shot_reimu_b_init(shot, i, 3, 2);
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
	shot_reimu_b_init(shot, i, 3, 2);
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
	shot_reimu_b_init(shot, i, 3, 2);
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
	shot_reimu_b_init(shot, i, 5, 2);
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
	shot_reimu_b_init(shot, i, 7, 2);
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
