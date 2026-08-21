/// Stage 4 midboss - update function
/// ---------------------------------
/// (#included from th04/enm_pos.cpp, ahead of enemy_pos_update() and
/// enemy_velocity_set(), which is this function's original address order at
/// the head of ENM_POS_TEXT's C++ object. That ZUN's object for this segment
/// held the four midboss update functions and the two enemy position helpers
/// is kb/codegen/0112.)
///
/// midboss4_render() is th04/main/midboss/m4.cpp, in a different segment and
/// therefore a different object.

#include "platform.h"
#include "pc98.h"
#include "th04/snd/snd.h"
#include "th04/main/homing.hpp"
#include "th04/main/scroll.hpp"
#include "th04/main/spark.hpp"
#include "th04/main/hud/hud.hpp"
#include "th04/main/item/item.hpp"
#include "th04/main/bullet/bullet.hpp"
#include "th04/main/bullet/clearzap.hpp"
// MIDBOSS_DEC(4) in th04/main/midboss/midboss.hpp declares midboss4_render()
// `near`, which is what it is. This object needs a FAR view of it, because a
// near reference under `-zPmain_03` frames its offset on main_03 and that
// function is main_01 (kb/codegen/0162) -- and Turbo C++ 4.02 rejects both a
// second file-scope declaration and a block-scope one. So the header's
// declaration is renamed out of the way as the header is expanded. Nothing in
// this translation unit calls midboss4_render() at all, so the renamed
// declaration is never referenced and emits no EXTDEF.
#define midboss4_render midboss4_render_near_unused
#include "th04/main/midboss/midboss.hpp"
#undef midboss4_render
#include "th04/main/phase.hpp"

void pascal far midboss4_render(void);

/// Still ASM
/// ---------
// The Stage 4 midboss's four bullet patterns, dispatched by [midboss4_pattern]
// below and called from nowhere else. All four are in this same segment and
// all four are private to ZUN's object, so each needed a zero-byte `label`
// alias in th04_main.asm to become linkable (kb/codegen/0123). The
// address-suffixed names are this parcel's, and are deliberately not semantic:
// nothing here measures what any of the four fires, and naming them belongs to
// whoever lifts them.
extern "C" {
	void near midboss4_14F78(void);  // [midboss4_pattern] 0
	void near midboss4_1511D(void);  // 1
	void near midboss4_15027(void);  // 2
	void near midboss4_15202(void);  // 3
}

// The midboss's own state, both of them th04_main.asm `.data?` bytes with no
// `public` of ZUN's. `[inferred]`, and **a naming round is owed**: every read
// and write of both is inside this function and the four patterns above, and
// the four are what set [midboss4_pattern] back to 255.
extern "C" {
	// Which of the four patterns is running, or 255 while the midboss is
	// traversing the playfield between two of them.
	extern unsigned char midboss4_pattern;

	// Traversals completed. The eighth one is the last: after it the midboss
	// leaves through the side instead of turning around, and the score bonus
	// for killing it is `30 - this`.
	extern unsigned char midboss4_passes;

	// Written 0 exactly once, here, and read by nothing in any of the five
	// binaries. ZUN bloat, and the reason it keeps an address-suffixed name.
	extern unsigned char midboss4_255C8;
}

/// ---------

// Both halves of the fight test the same box.
#define midboss4_hittest(se_on_hit) \
	midboss_hittest_shots_damage(TO_SP(24), TO_SP(24), se_on_hit)

// `#pragma option -a2` is the one padding byte between this function's
// epilogue and its generated value/jump table pair. kb/codegen/0160 for the
// instrument: read the OBJ's PUBDEF offsets, not the `tcc -S` listing.
// `[measured]` at a zero prefix -- which is what this function being the first
// thing the object emits gives it -- the object is 0x29B bytes to
// enemy_pos_update() and carries the pad; without `-a2` it is 0x29A and does
// not. That is the same direction as BOSS_BG_TEXT's and the opposite of
// B4M_UPDATE_TEXT's, which is why 0160 says to probe rather than compute.
#pragma option -a2
void pascal far midboss4_update(void)
{
	int damage;

	homing_target.x.v = midboss.pos.cur.x.v;
	homing_target.y.v = midboss.pos.cur.y.v;

	if(midboss.phase == 0) {
		// The entrance: it drifts in from the top right and is invincible
		// until it stops, but the hittest still runs.
		midboss.pos.update_seg3();
		midboss.phase_frame++;
		damage = midboss4_hittest(10); // ZUN bloat: never read
		if(midboss.phase_frame >= 48) {
			midboss.phase++;
			midboss.phase_frame = 0;
			midboss.pos.velocity.x.v = 0;
			midboss.pos.velocity.y.v = 0;
			midboss4_255C8 = 0;
			midboss4_pattern = 0;
			midboss4_passes = 0;
		}
	} else if(midboss.phase == 1) {
		midboss.pos.update_seg3();
		midboss.phase_frame++;
		bullet_template.spawn_type = BST_PELLET;
		bullet_template.origin.x.v = midboss.pos.cur.x.v;
		bullet_template.origin.y.v = (midboss.pos.cur.y.v - TO_SP(16));

		switch(midboss4_pattern) {
		case 0:
			midboss4_14F78();
			break;
		case 1:
			midboss4_1511D();
			break;
		case 2:
			midboss4_15027();
			break;
		case 3:
			midboss4_15202();
			break;
		case 255:
			if(midboss.phase_frame == 1) {
				// Away from whichever half of the playfield it stopped in.
				midboss.pos.velocity.x.v = ((
					midboss.pos.cur.x.v >= TO_SP(180)
				) ? -TO_SP(4) : TO_SP(4));
			} else if(midboss4_passes < 8) {
				if(
					(midboss.pos.cur.x.v <= TO_SP(48)) ||
					(midboss.pos.cur.x.v >= TO_SP(336))
				) {
					midboss.pos.velocity.x.v = 0;
					midboss.phase_frame = 0;
					midboss4_passes++;
					midboss4_pattern = (midboss4_passes & 3);
				}
			} else if(
				// A whole sprite past either edge, since this traversal is
				// the one it never comes back from.
				(midboss.pos.cur.x.v <= TO_SP(-32)) ||
				(midboss.pos.cur.x.v >= TO_SP(416))
			) {
				midboss.phase = 3;
			}
			break;
		}

		if(
			(midboss.pos.cur.y.v >= TO_SP(368)) ||
			(midboss.pos.cur.x.v <= 0) ||
			(midboss.pos.cur.x.v >= TO_SP(384))
		) {
			midboss.phase = PHASE_NONE;
		}
		damage = midboss4_hittest(4);
		if(damage) {
			midboss.hp -= damage;
			if(midboss.hp > 0) {
				midboss.damage_this_frame = 1;
			} else {
				midboss.damage_this_frame = 1;

				// ZUN bloat: the return value is never read, and neither is
				// the assignment below it.
				damage = scroll_subpixel_y_to_vram_always(
					midboss.pos.cur.y.v - TO_SP(16)
				);

				bullet_zap.active = true;
				midboss_score_bonus(30 - midboss4_passes);
				playfield_shake_anim_time = 12;
				midboss.phase = PHASE_EXPLODE_BIG;
				midboss.sprite = 4;
				midboss.phase_frame = 0;
				midboss.pos.velocity.x.v = 0;
				sparks_add_circle(
					midboss.pos.cur.x, midboss.pos.cur.y, TO_SP(6), 48
				);
				snd_se_play(12);

				// The first of the stage's two midbosses drops the bomb.
				if(midboss.frames_until == 2800) {
					items_add(
						midboss.pos.cur.x.v, midboss.pos.cur.y.v, IT_BOMB
					);
				} else {
					items_add(
						midboss.pos.cur.x.v, midboss.pos.cur.y.v, IT_1UP
					);
				}
			}
		}
	} else if(midboss.phase == PHASE_EXPLODE_BIG) {
		midboss.pos.velocity.x.v = 0;
		midboss.pos.velocity.y.v = 0;
		midboss.pos.update_seg3();
		midboss.phase_frame++;
		if((midboss.phase_frame % 16) == 0) {
			midboss.sprite++;
			if(midboss.sprite >= 12) {
				midboss.phase++;
				midboss.hp = 0;
			}
		}
	} else {
		// kb/codegen/0014: a far call to a function in the same GROUP is a
		// near call with CS pushed by hand, not the inter-segment call Turbo
		// C++ emits for a far target on its own.
		_asm { nop; push cs; call near ptr midboss_reset }

		// …and the second one re-arms itself as the same midboss, which is
		// the only reason this function is ever installed twice.
		if(midboss.frames_until == 2800) {
			midboss.frames_until = 5600;
			midboss_update_func = midboss4_update;
			_asm mov word ptr midboss_render_func, offset midboss4_render
			midboss.pos.cur.x.v = TO_SP(240);
			midboss.pos.cur.y.v = TO_SP(-32);
			midboss.pos.prev.x.v = TO_SP(240);
			midboss.pos.prev.y.v = TO_SP(-32);
			midboss.pos.velocity.x.v = -TO_SP(4);
			midboss.pos.velocity.y.v = TO_SP(2);
			midboss.hp = 1200;
			midboss.sprite = 0;
			midboss.phase_frame = 0;
			return;
		}
	}
	hud_hp_update_and_render(midboss.hp, 1200);
}
#pragma option -a1
/// ---------------------------------
