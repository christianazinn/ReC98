/// Marisa's shottype B, levels 2 to 9
/// ----------------------------------
/// The last eight of the sixteen functions installed into [playchar_shot_func]
/// out of [playchar_shot_funcs]; reached only through that pointer, which is
/// why the dump publishes none of them.
///
/// Where shottype A is the option laser (th04/main/player/shot_marisa_a.cpp),
/// B replaces it with a second kind of *shot*: a 16×16 one out of the 0x24
/// sprite bank, fired from an offset to either side of the player and — from
/// level 6 up — at a spread that widens with the level. `[measured]`, and it
/// is the whole difference between Marisa's two shottypes: not one of these
/// eight calls shot_laser_update(), which is why lifting shottype A took its
/// last call site with it. The mirror of the homing/fixed-angle split between
/// Reimu's two (th04/main/player/shot_reimu_b.cpp).
///
/// Levels 6 to 9 pick each option shot's x offset with a switch statement on
/// the remaining count, and that statement is what compiles to the `db 0` plus
/// `dw offset loc_...` run behind four of the eight `endp`s in the original.
/// Levels 2 to 5 use a plain conditional and compile no table at all.
///
/// (#included from th04/p_marisa.cpp, last of the three bodies that object
/// compiles — the address order all three have in P_MARISA_TEXT. This object
/// occupies a kb/codegen/0080 anchor at the HEAD of its segment, so its
/// include list runs in ASCENDING address order and each lift appends to the
/// end of it, the mirror of th04/player_b.cpp. kb/codegen 0080 + 0114 + 0129.)
///
/// state/re/JUMP_TABLE_TAILS.md's class: FOUR tables in one object, all four
/// preceded by exactly one `db 0` pad. Their natural (unpadded) object-local
/// offsets are 0x592, 0x626, 0x6D8 and 0x798 — every one EVEN, and pairwise an
/// even distance apart, so kb/codegen/0157's corollary applies and a single
/// parity bit decides all four together. This object's base is the original
/// segment start and every body ahead of these eight is pinned to its own
/// original address, so there was no parity freedom to spend here at all:
/// either the compiler pads at this parity or the lift is impossible. See the
/// `-a2` comment below for what it actually does.

#include "th03/math/randring.hpp"
#include "th04/main/player/shot.hpp"
#include "th04/sprites/main_pat.h"

// th04/math/vector.hpp is deliberately NOT named here: it reaches
// th02/math/vector.hpp and th01/math/subpixel.hpp, neither of which has an
// include guard, and both are already expanded in this object through
// th04/main/player/shot.hpp above (kb/codegen 0110 + 0129). One declaration
// costs one line; re-expanding that closure is a compile error.
// (Copied from th02/math/vector.hpp, which is where the shared one lives —
// together with the `extern "C"` that th04/math/vector.hpp wraps it in. That
// wrapper is not decoration: without it TLINK reports this function as
// undefined under its C++-mangled parameter-list spelling, which is the
// spelling a master.lib routine with no mangled name at all can never have.)
extern "C" {
	void pascal vector2(
		int &ret_x, int &ret_y, unsigned char angle, int length
	);
}

// The four `db 0` pads the original puts in front of levels 6 to 9's jump
// tables. `[measured]` on the OBJ, before the first build, with
// tools/pi-audit/obj_probe.py — never off a `tcc -S` listing, which
// kb/codegen/0154's own correction records as wrong in BOTH directions for
// this question:
//
//   no -a : B_L6 0x93, B_L7 0x93, B_L8 0xB1, B_L9 0xC3, SEGDEF 0x7A1
//   -a2   : B_L6 0x94, B_L7 0x94, B_L8 0xB2, B_L9 0xC4, SEGDEF 0x7A5
//
// and 0x7A5 is exactly the original contribution. Every one of the eight
// BODIES is byte-identical either way, which is kb/codegen/0119's whole point:
// without this line the lift is four bytes short and no per-function funcdiff
// can see it.
//
// This is a THIRD data point for the direction 0154 and kb/codegen/0096
// disagree about, and it lands on 0096's side as originally written: all four
// natural offsets here are EVEN and `-a2` pads all four. It also settles
// 0157's corollary in the affirmative on a four-table object — one option, one
// parity, four pads, no per-table exception.
//
// Placed after every #include on purpose: `-a2` is data alignment as well as
// code alignment, so a struct declared while it is in force would be padded.
// Nothing below declares one, and every struct this file uses — Shot,
// PlayfieldMotion, SPPoint — was laid out by the headers above. Nothing in the
// object after this file, either: shot_marisa_b.cpp is the last body
// th04/p_marisa.cpp compiles. `[measured]`: the OBJ offsets and lengths of all
// eleven functions ahead of these eight are bit-identical with and without it.
#pragma option -a2

extern "C" {

// Center-to-center distance of an option from the player, which levels 2 to 5
// fire their two option shots at. Levels 6 and up widen the spread instead and
// spell their own offsets out in the switch statement.
static const subpixel_t SHOT_OPTION_X = TO_SP(PLAYER_OPTION_DISTANCE);

// The random forward angle every option shot that is *aimed* rather than fired
// straight up takes: the same 8-unit window shottype A's level 2 uses.
#define shot_marisa_b_angle_random randring1_next8_ge_lt(-0x44, -0x3C)

void pascal near shot_marisa_b_l2(void)
{
	Shot near *shot;
	int i = 3;

	shot_ptr = shots;
	shot_last_id = 0;
	while(( shot = shots_add() ) != nullptr) {
		if(i <= 1) {
			shot->patnum_base = PAT_SHOT_MARISA;
			shot_velocity_set(
				&shot->pos.velocity, shot_marisa_b_angle_random
			);
			shot->damage = 10;
		} else {
			if(i == 3) {
				shot->pos.cur.x.v -= SHOT_OPTION_X;
			} else {
				shot->pos.cur.x.v += SHOT_OPTION_X;
			}
			shot->patnum_base = PAT_SHOT_MARISA_SUB_B;
			shot->pos.velocity.y.v = TO_SP(-16);
			shot->damage = 6;
		}
		if(--i <= 0) {
			break;
		}
	}
}

void pascal near shot_marisa_b_l3(void)
{
	Shot near *shot;
	int i = 4;

	shot_ptr = shots;
	shot_last_id = 0;
	while(( shot = shots_add() ) != nullptr) {
		if(i <= 2) {
			if(i == 2) {
				shot->pos.cur.x.v -= TO_SP(8);
			} else {
				shot->pos.cur.x.v += TO_SP(8);
			}
			shot->patnum_base = PAT_SHOT_MARISA;
			shot->damage = 9;
		} else {
			if(i == 4) {
				shot->pos.cur.x.v -= SHOT_OPTION_X;
			} else {
				shot->pos.cur.x.v += SHOT_OPTION_X;
			}
			shot->patnum_base = PAT_SHOT_MARISA_SUB_B;
			shot->pos.velocity.y.v = TO_SP(-16);
			shot->damage = 6;
		}
		if(--i <= 0) {
			break;
		}
	}
}

// Level 3 with the option shots aimed at a random forward angle rather than
// fired straight up.
void pascal near shot_marisa_b_l4(void)
{
	Shot near *shot;
	int i = 4;

	shot_ptr = shots;
	shot_last_id = 0;
	while(( shot = shots_add() ) != nullptr) {
		if(i <= 2) {
			if(i == 2) {
				shot->pos.cur.x.v -= TO_SP(8);
			} else {
				shot->pos.cur.x.v += TO_SP(8);
			}
			shot->patnum_base = PAT_SHOT_MARISA;
			shot->damage = 9;
		} else {
			if(i == 4) {
				shot->pos.cur.x.v -= SHOT_OPTION_X;
			} else {
				shot->pos.cur.x.v += SHOT_OPTION_X;
			}
			shot->patnum_base = PAT_SHOT_MARISA_SUB_B;
			vector2(
				shot->pos.velocity.x.v,
				shot->pos.velocity.y.v,
				shot_marisa_b_angle_random,
				TO_SP(16)
			);
			shot->damage = 6;
		}
		if(--i <= 0) {
			break;
		}
	}
}

void pascal near shot_marisa_b_l5(void)
{
	Shot near *shot;
	int i = 5;
	unsigned char angle = -0x48;

	shot_ptr = shots;
	shot_last_id = 0;
	while(( shot = shots_add() ) != nullptr) {
		if(i <= 3) {
			shot->patnum_base = PAT_SHOT_MARISA;
			shot_velocity_set(&shot->pos.velocity, angle);
			shot->damage = 9;
			angle += 0x08;
		} else {
			if(i == 5) {
				shot->pos.cur.x.v -= SHOT_OPTION_X;
			} else {
				shot->pos.cur.x.v += SHOT_OPTION_X;
			}
			vector2(
				shot->pos.velocity.x.v,
				shot->pos.velocity.y.v,
				shot_marisa_b_angle_random,
				TO_SP(16)
			);
			shot->patnum_base = PAT_SHOT_MARISA_SUB_B;
			shot->damage = 5;
		}
		if(--i <= 0) {
			break;
		}
	}
}

// Levels 6 to 9 all share this shape: a fan of the player's own shots that
// walks [angle] by a fixed step, and then four (six at level 9) option shots
// fired straight up from an x offset the switch statement picks out of the
// remaining count. The offset is SUBTRACTED, so a positive case sends its shot
// to the LEFT.
//
// The cases run in DESCENDING order because that is the order the original's
// bodies sit in memory, and Turbo C++ emits a switch's bodies in source order
// (the same reason th04/main/player/shot_reimu_b.cpp lists its four that way).
// The dispatcher is dense — `sub bx, N` / `cmp bx, count - 1` / `ja` / `add
// bx, bx` / `jmp cs:table[bx]` — and the missing default is ZUN's: [x] stays
// uninitialized for a count outside the cases, which no level can reach.
#define shot_marisa_b_fan_init(shot, i, x, angle, count, angle_first) \
	Shot near *shot; \
	int i = count; \
	subpixel_t x; \
	unsigned char angle = angle_first; \
	\
	shot_ptr = shots; \
	shot_last_id = 0;

#define shot_marisa_b_fan_option(shot, x, dmg) \
	shot->pos.cur.x.v -= x; \
	shot->patnum_base = PAT_SHOT_MARISA_SUB_B; \
	shot->pos.velocity.y.v = TO_SP(-16); \
	shot->damage = dmg;

void pascal near shot_marisa_b_l6(void)
{
	shot_marisa_b_fan_init(shot, i, x, angle, 7, -0x48);
	while(( shot = shots_add() ) != nullptr) {
		if(i <= 3) {
			shot->patnum_base = PAT_SHOT_MARISA;
			shot_velocity_set(&shot->pos.velocity, angle);
			shot->damage = 9;
			angle += 0x08;
		} else {
			switch(i) {
			case 7: x = TO_SP(-32); break;
			case 6: x = TO_SP(-16); break;
			case 5: x = TO_SP(32); break;
			case 4: x = TO_SP(16); break;
			}
			shot_marisa_b_fan_option(shot, x, 5);
		}
		if(--i <= 0) {
			break;
		}
	}
}

// Level 6 with a wider step, one less damage on the player's own shots, and
// the first angle two units further left.
void pascal near shot_marisa_b_l7(void)
{
	shot_marisa_b_fan_init(shot, i, x, angle, 7, -0x4A);
	while(( shot = shots_add() ) != nullptr) {
		if(i <= 3) {
			shot->patnum_base = PAT_SHOT_MARISA;
			shot_velocity_set(&shot->pos.velocity, angle);
			shot->damage = 8;
			angle += 0x0A;
		} else {
			switch(i) {
			case 7: x = TO_SP(-32); break;
			case 6: x = TO_SP(-16); break;
			case 5: x = TO_SP(32); break;
			case 4: x = TO_SP(16); break;
			}
			shot_marisa_b_fan_option(shot, x, 5);
		}
		if(--i <= 0) {
			break;
		}
	}
}

// Level 7 plus one more shot in the player's own fan, which is offset to one
// side or the other rather than fired from the center — and the count at which
// that happens is also the one round whose angle is NOT stepped afterwards, so
// two of the fan's shots leave at the same angle.
void pascal near shot_marisa_b_l8(void)
{
	shot_marisa_b_fan_init(shot, i, x, angle, 8, -0x4A);
	while(( shot = shots_add() ) != nullptr) {
		if(i <= 4) {
			if(i == 3) {
				shot->pos.cur.x.v -= TO_SP(8);
			} else if(i == 2) {
				shot->pos.cur.x.v += TO_SP(8);
			}
			shot->patnum_base = PAT_SHOT_MARISA;
			shot_velocity_set(&shot->pos.velocity, angle);
			shot->damage = 8;
			if(i != 3) {
				angle += 0x0A;
			}
		} else {
			switch(i) {
			case 8: x = TO_SP(-32); break;
			case 7: x = TO_SP(-16); break;
			case 6: x = TO_SP(32); break;
			case 5: x = TO_SP(16); break;
			}
			shot_marisa_b_fan_option(shot, x, 5);
		}
		if(--i <= 0) {
			break;
		}
	}
}

// Level 8 with six option shots rather than four, spread out to 48 pixels, and
// one less damage on each of them.
void pascal near shot_marisa_b_l9(void)
{
	shot_marisa_b_fan_init(shot, i, x, angle, 10, -0x4A);
	while(( shot = shots_add() ) != nullptr) {
		if(i <= 4) {
			if(i == 3) {
				shot->pos.cur.x.v -= TO_SP(8);
			} else if(i == 2) {
				shot->pos.cur.x.v += TO_SP(8);
			}
			shot->patnum_base = PAT_SHOT_MARISA;
			shot_velocity_set(&shot->pos.velocity, angle);
			shot->damage = 8;
			if(i != 3) {
				angle += 0x0A;
			}
		} else {
			switch(i) {
			case 10: x = TO_SP(-48); break;
			case  9: x = TO_SP(-32); break;
			case  8: x = TO_SP(-16); break;
			case  7: x = TO_SP(48); break;
			case  6: x = TO_SP(32); break;
			case  5: x = TO_SP(16); break;
			}
			shot_marisa_b_fan_option(shot, x, 4);
		}
		if(--i <= 0) {
			break;
		}
	}
}

}
