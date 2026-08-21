/// Stage 2 midboss - update function
/// ---------------------------------
/// (#included from th04/enm_pos1.cpp, which is a SEPARATE object from
/// th04/enm_pos.cpp on purpose. See that file.)
///
/// midboss2_render() is th04/main/midboss/m2.cpp, in a different segment and
/// therefore a different object.

#include "platform.h"
#include "pc98.h"
#include "th02/v_colors.hpp"
#include "th04/snd/snd.h"
#include "th04/math/randring.hpp"
#include "th04/main/frames.h"
#include "th04/main/homing.hpp"
#include "th04/main/gather.hpp"
#include "th04/main/spark.hpp"
#include "th04/main/hud/hud.hpp"
#include "th04/main/item/item.hpp"
#include "th04/main/bullet/bullet.hpp"
#include "th04/main/bullet/clearzap.hpp"
#include "th04/main/midboss/midboss.hpp"

/// The midboss's own state
/// -----------------------
/// All three are th04_main.asm `.data?` bytes with no `public` of ZUN's, and
/// this function plus the four patterns above it are the only readers or
/// writers of any of them in any of the five binaries.
///
/// Two of the three are the Stage 4 midboss's two exactly: same role, same
/// `& 3` pattern cycle, same `N - passes` score bonus, so they take
/// th04/main/midboss/m4_updt.cpp's already-ruled spellings rather than a new
/// pair of names. The third has no counterpart there and keeps the dump's
/// address-suffixed spelling; **a naming round is owed** for it.
extern "C" {
	// Which of the four patterns is running, or 255 during the gather-and-
	// reposition interlude between two of them.
	extern unsigned char midboss2_pattern;

	// Which way the midboss is facing, and therefore which way it moves next:
	// 0 left, 2 right, 1 stopped in the middle. Also gates the sprite reset
	// at the end of each interlude. `[inferred]`.
	extern unsigned char midboss2_255B3;

	// Interludes completed. The score bonus for killing it is `18 - this`,
	// and the 17th ends the fight whether or not the player got there.
	extern unsigned char midboss2_passes;
}
/// -----------------------

// Both phases test the same box, and both pass the same `se_on_hit` — unlike
// the Stage 4 midboss, whose entrance passes 10 and whose fight passes 4.
#define midboss2_hittest() \
	midboss_hittest_shots_damage(TO_SP(24), TO_SP(24), 10)

/// The Stage 2 midboss's four bullet patterns
/// ------------------------------------------
/// All four sit directly above this function in ZUN's object, and every one
/// of them is reached from its `switch(midboss2_pattern)` and from nowhere
/// else — so all four are still ASM here only because they are not the tail,
/// and each needed a zero-byte `label` alias in th04_main.asm to become
/// linkable (kb/codegen/0123). They keep the dump's address-suffixed names;
/// **a naming round is owed for all four**, on the same terms as the Stage 4
/// midboss's.
extern "C" {
	void near midboss2_14AF2(void);
	void near midboss2_14B76(void);
	void near midboss2_14BCD(void);
	void near midboss2_14C45(void);
}
/// ------------------------------------------

void pascal far midboss2_update(void)
{
	int damage;

	homing_target.x.v = midboss.pos.cur.x.v;
	homing_target.y.v = midboss.pos.cur.y.v;

	if(midboss.phase == 0) {
		// The entrance: invincible until it stops, but the hittest still runs.
		midboss.pos.update_seg3();
		midboss.phase_frame++;
		damage = midboss2_hittest(); // ZUN bloat: never read
		if(midboss.phase_frame >= 96) {
			midboss.phase++;
			midboss.phase_frame = 0;
			midboss.pos.velocity.x.v = 0;
			midboss.pos.velocity.y.v = 0;
			midboss2_pattern = 0;
			midboss2_255B3 = 1;
			midboss2_passes = 0;
		}
	} else if(midboss.phase == 1) {
		midboss.pos.update_seg3();
		midboss.phase_frame++;
		bullet_template.spawn_type = BST_PELLET;
		bullet_template.origin.x.v = midboss.pos.cur.x.v;
		bullet_template.origin.y.v = (midboss.pos.cur.y.v - TO_SP(16));

		switch(midboss2_pattern) {
		case 0:
			midboss2_14AF2();
			break;
		case 1:
			midboss2_14B76();
			break;
		case 2:
			midboss2_14BCD();
			break;
		case 3:
			midboss2_14C45();
			break;
		case 255:
			// The interlude: a 52-frame gather circle, during 48 frames of
			// which the midboss slides to the other side of the playfield.
			gather_template.center.x.v = midboss.pos.cur.x.v;
			gather_template.center.y.v = midboss.pos.cur.y.v;
			gather_add_only_3stack(
				(midboss.phase_frame - 48), V_WHITE, 7
			);
			switch(midboss.phase_frame) {
			case 48:
				midboss.pos.velocity.x.v = 0;
				break;

			case 52:
				midboss.phase_frame = 0;
				if(midboss2_255B3 == 1) {
					midboss.sprite = 0;
				}
				midboss2_passes++;
				_AL = midboss2_passes;
				_AL &= 3;
				midboss2_pattern = _AL;
				if(midboss2_passes > 16) {
					goto leaves;
				}
				break;

			case 1:
				gather_template.ring_points = 8;
				gather_template.radius.v = TO_SP(96);
				if(midboss2_255B3 == 1) {
					// Stopped in the middle, so the direction is a coin flip.
					if(randring2_next16() & 1) {
						midboss.sprite = 1;
						midboss2_255B3 = 0;
						midboss.pos.velocity.x.v = -TO_SP(3);
					} else {
						midboss.sprite = 2;
						midboss2_255B3 = 2;
						midboss.pos.velocity.x.v = TO_SP(3);
					}
				} else if(midboss2_255B3 == 0) {
					midboss2_255B3 = 1;
					midboss.pos.velocity.x.v = TO_SP(3);
					midboss.sprite = 2;
				} else if(midboss2_255B3 == 2) {
					midboss2_255B3 = 1;
					midboss.pos.velocity.x.v = -TO_SP(3);
					midboss.sprite = 1;
				}
				break;
			}
			break;
		}

		damage = midboss2_hittest();
		if(damage) {
			midboss.hp -= damage;
			if(midboss.hp > 0) {
				midboss.damage_this_frame = 1;
				snd_se_play(4);
			} else {
				midboss.damage_this_frame = 1;
				bullet_zap.active = true;
				midboss_score_bonus(18 - midboss2_passes);
				items_add(
					midboss.pos.cur.x.v, midboss.pos.cur.y.v, IT_BOMB
				);
				playfield_shake_anim_time = 12;

				// …and it leaves the same way whether the player killed it or
				// simply survived all 17 interludes.
leaves:
				midboss.phase = 2;
				midboss.sprite = 0;
				midboss.phase_frame = 0;
				midboss.pos.velocity.x.v = 0;
				midboss.pos.velocity.y.v = -TO_SP(1);
				sparks_add_circle(
					midboss.pos.cur.x, midboss.pos.cur.y, TO_SP(8), 48
				);
				snd_se_play(12);
			}
		}
	} else if(midboss.phase == 2) {
		// Straight up and off the top of the playfield, sparking every 16th
		// stage frame.
		midboss.pos.update_seg3();
		midboss.phase_frame++;
		if(midboss.pos.cur.y.v <= 0) {
			midboss.phase++;
			midboss.hp = 0;
		}
		if(stage_frame_mod16 == 0) {
			sparks_add_circle(
				midboss.pos.cur.x, midboss.pos.cur.y, TO_SP(8), 16
			);
		}
	} else {
		// kb/codegen/0014: a far call to a function in the same GROUP is a
		// near call with CS pushed by hand, not the inter-segment call Turbo
		// C++ emits for a far target on its own.
		_asm { nop; push cs; call near ptr midboss_reset }
	}
	hud_hp_update_and_render(midboss.hp, 750);
}
