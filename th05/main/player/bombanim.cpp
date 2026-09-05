#include "libs/master.lib/pc98_gfx.hpp"
#include "th04/math/vector.hpp"
#include "th04/math/randring.hpp"
#include "th04/main/playfld.hpp"
#include "th04/main/frames.h"
#include "th04/main/circle.hpp"
#include "th04/main/hud/hud.hpp"
#include "th04/main/item/item.hpp"
#include "th04/main/boss/boss.hpp"
#include "th05/sprites/main_pat.h"
#include "th05/main/player/bomb.hpp"

#include "th05/main/player/bombanim.hpp"

#define reimu_star_put(star, col) \
	super_roll_put_1plane( \
		star->topleft.screen_x.to_pixel_slow(), \
		star->topleft.screen_y.to_pixel_slow(), \
		PAT_PLAYCHAR_BOMB_SHAPE, \
		0, \
		super_plane(col, true) \
	);

/// Dream meter drain
/// -----------------
/// Called at the end of every frame of all four playchars' bomb animations —
/// bombing costs Dream. The rate is halved outside the [PHASE_HP_FILL] window:
/// the meter loses a unit every 2nd frame while [boss.phase] is 0, and only
/// every 4th frame otherwise. The same `(phase == PHASE_HP_FILL) || (frame is
/// odd-ish)` gate guards the -2 drain in the miss animation, which is still
/// ZUN's assembly. [inferred; the dump left this function unnamed]
///
/// The floor is 1, not 0: a meter that reached 0 here would be re-raised by
/// the next point item from a different base than the miss path's floor, which
/// also clamps to 1.

void near bomb_dream_decay(void)
{
	// `% 2` and `% 4`, not `& 1` and `& 3`: [bomb_frame] is an
	// `unsigned char` that promotes to a *signed* `int` before `%`, and
	// Turbo C++ 4.0J only lowers the modulo to a mask for an unsigned
	// operand. (kb/codegen/0128; the remainder is consumed out of DX, which
	// is what proves this is `%` and not `/`.)
	if((bomb_frame % 2) == 0) {
		if((boss.phase == PHASE_HP_FILL) || ((bomb_frame % 4) != 0)) {
			if(dream > 1) {
				dream--;
			}

			// kb/codegen/0083: a natural C++ call would be a plain
			// far `9A`, but the original reaches this same-group
			// callee through `nop` / `push cs` / `call near`.
			_asm { nop; push cs; call near ptr hud_dream_put; }
		}
	}
}
/// -----------------

void near reimu_stars_update_and_render(void)
{
	reimu_star_t near *head;
	reimu_star_t near *trail;
	Subpixel speed;
	int i;
	unsigned char angle;

	if(bomb_frame == BOMB_CIRCLE_FRAMES) {
		head = &bomb_anim.reimu[0][0];
		trail = &bomb_anim.reimu[0][REIMU_STAR_NODE_COUNT - 1];
		i = 0;
		while(i < REIMU_STAR_TRAILS) {
			//  head == [0 |  8 | 16 | 24 | 30 | 36]
			// trail == [7 | 11 | 15 | 19 | 23 | 27]
			head->topleft.screen_x.v = randring1_next16_mod_ge_lt_sp(
				PLAYFIELD_LEFT, (PLAYFIELD_LEFT + 320.0f)
			);
			head->topleft.screen_y.v = randring1_next16_mod_ge_lt_sp(
				0.0f, (RES_Y - (REIMU_STAR_H / 2))
			);

			// MODDERS: REIMU_STAR_NODE_COUNT times
			/*    */	trail->topleft = head->topleft;
			trail--;	trail->topleft = head->topleft;
			trail--;	trail->topleft = head->topleft;
			trail--;	trail->topleft = head->topleft;
			trail--;	trail->topleft = head->topleft;
			trail--;	trail->topleft = head->topleft;
			trail--;	trail->topleft = head->topleft;
			trail += 10; // ZUN quirk

			speed.v = randring1_next16_ge_lt_sp(10.0f, 18.0f);
			angle = (0x40 + randring1_next8_mod_ge_lt(-0x18, +0x18));

			vector2(head->velocity.x.v, head->velocity.y.v, angle, speed);

			i++;
			head += REIMU_STAR_NODE_COUNT;
		}
	}
	head = &bomb_anim.reimu[0][REIMU_STAR_NODE_COUNT - 2];
	trail = &bomb_anim.reimu[0][REIMU_STAR_NODE_COUNT - 1];
	for(i = 0; i < REIMU_STAR_TRAILS; i++) {
		//  head == [6 | 14 | 22 | 30 | 38 | 46]
		// trail == [7 | 15 | 23 | 31 | 39 | 47]

		// MODDERS: (REIMU_STAR_NODE_COUNT - 1) times
		/*            */	trail->topleft = head->topleft;
		head--; trail--;	trail->topleft = head->topleft;
		head--; trail--;	trail->topleft = head->topleft;
		head--; trail--;	trail->topleft = head->topleft;
		head--; trail--;	trail->topleft = head->topleft;
		head--; trail--;	trail->topleft = head->topleft;
		head--; trail--;	trail->topleft = head->topleft;
		//  head == [0 |  8 | 16 | 24 | 32 | 40]
		// trail == [1 |  9 | 17 | 25 | 33 | 41]

		head->topleft.screen_x.v += head->velocity.x.v;
		head->topleft.screen_y.v += head->velocity.y.v;

		if(
			head->topleft.screen_x.v <= to_sp(PLAYFIELD_LEFT) ||
			head->topleft.screen_x.v >= to_sp(PLAYFIELD_RIGHT - REIMU_STAR_W)
		) {
			head->velocity.x.v = -head->velocity.x.v;
		}
		if(head->topleft.screen_y.v >= to_sp(RES_Y)) {
			head->topleft.screen_y.v -= to_sp(RES_Y);
		}

		trail += ((REIMU_STAR_NODE_COUNT - 1) - 2);
		// trail == [6 | 14 | 22 | 30 | 38 | 46]

		/*       */	reimu_star_put(trail, 6);
		trail -= 2;	reimu_star_put(trail, 6);
		trail -= 2;	reimu_star_put(trail, 6);
		/*       */	reimu_star_put(head, 7);

		// trail == [2 | 10 | 18 | 26 | 34 | 42]

		// ZUN bug: Should perhaps be done after the conditional branch below,
		// to avoid the out-of-bounds memory access on the last iteration of
		// this loop...
		trail += (((REIMU_STAR_NODE_COUNT * 2) - 2) - 1);
		head += ((REIMU_STAR_NODE_COUNT * 2) - 2);

		if((bomb_frame <= 112) && (stage_frame_mod8 == i)) {
			/* TODO: Replace with the decompiled call
			 * 	circles_add_growing(
			 * 		head->topleft.screen_x,
			 * 		(head->topleft.screen_y + PLAYFIELD_TOP)
			 * 	);
			 * once that function is part of this translation unit */
			_asm {
				db	0xFF, 0x34;
				db	0x8B, 0x44, 0x02;
				add 	ax, (PLAYFIELD_TOP * 16);
				push	ax;
				nop;
				push	cs;
				call	near ptr circles_add_growing;
			}
		}
	}
}
