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
// iatan2(), which the fourth pattern aims with.
#include "libs/master.lib/master.hpp"
#include "th04/sprites/main_pat.h"
#include "th04/main/player/player.hpp"
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
/// All four sat directly above midboss2_update() in ZUN's object, and every
/// one of them is reached from its `switch(midboss2_pattern)` and from nowhere
/// else, so all four are `static` here and the four zero-byte `label` aliases
/// th04_main.asm carried for them are gone with the bodies. They keep the
/// dump's address-suffixed names; **a naming round is owed for all four**, on
/// the same terms as the Stage 4 midboss's.
///
/// All four share one skeleton: a modulo on the phase frame gates the volley,
/// and a compare against the pattern's own length hands control back to
/// midboss2_update() by setting [midboss2_pattern] to 255. Three of them aim
/// off [midboss2_255B3], the direction byte, rather than off the player.

// A ball on every other volley and a 6-way spread on all of them, both aimed
// away from whichever wall the midboss is heading for.
static void near midboss2_14AF2(void)
{
	register int frame = midboss.phase_frame;

	if((frame % 8) == 0) {
		snd_se_play(3);
		_AL = midboss2_255B3;
		_AL <<= 5;
		_AL += 0x20;
		bullet_template.angle = _AL;
		if((frame / 8) & 1) {
			bullet_template.spawn_type = BST_BULLET16_CLOUD_FORWARDS;
			bullet_template.patnum = PAT_BULLET16_N_BALL_BLUE;
			bullet_template.group = BG_SINGLE;
			bullet_template.speed.v = (TO_SP(2) + 10);
			bullet_template_tune();
			bullets_add_regular();
			bullet_template.delta.spread_angle = 0x0F;
		} else {
			bullet_template.delta.spread_angle = 0x0A;
		}
		bullet_template.group = BG_SPREAD;
		bullet_template.count = 6;
		bullet_template.speed.v = (TO_SP(2) + 4);
		bullet_template_tune();
		bullets_add_regular();
	}
	if(frame >= 64) {
		midboss.phase_frame = 0;
		midboss2_pattern = 255;
	}
}

// A 32-way ring every 8th frame, each one faster than the last.
static void near midboss2_14B76(void)
{
	register int frame = midboss.phase_frame;

	if((frame % 8) == 0) {
		snd_se_play(3);
		_AL = midboss2_255B3;
		_AL <<= 5;
		_AL += 0x20;
		bullet_template.angle = _AL;
		_AL = (frame / 2);
		_AL += TO_SP(2);
		bullet_template.speed.v = _AL;
		bullet_template.group = BG_RING;
		bullet_template.count = 32;
		bullet_template_tune();
		bullets_add_regular();
	}
	if(frame >= 32) {
		midboss.phase_frame = 0;
		midboss2_pattern = 255;
	}
}

// Four single bullets every 4th frame, at four fixed angles off the direction
// byte — two on each side of it, and the outer pair fired first.
static void near midboss2_14BCD(void)
{
	register int frame = midboss.phase_frame;

	if((frame % 4) == 0) {
		snd_se_play(3);
		bullet_template.speed.v = TO_SP(4);
		bullet_template.group = BG_SINGLE;
		_AL = midboss2_255B3;
		_AL <<= 5;
		_AL += 0x08;
		bullet_template.angle = _AL;
		bullet_template_tune();
		bullets_add_regular();
		_AL = midboss2_255B3;
		_AL <<= 5;
		_AL += 0x10;
		bullet_template.angle = _AL;
		bullets_add_regular();
		_AL = midboss2_255B3;
		_AL <<= 5;
		_AL += 0x38;
		bullet_template.angle = _AL;
		bullets_add_regular();
		_AL = midboss2_255B3;
		_AL <<= 5;
		_AL += 0x30;
		bullet_template.angle = _AL;
		bullets_add_regular();
	}
	if(frame >= 32) {
		midboss.phase_frame = 0;
		midboss2_pattern = 255;
	}
}

// The only one that aims at the player: a 5-way spread on the angle latched at
// the start, an aimed ball beside it, and a cloud of specials over the last 8
// frames of every 32.
static void near midboss2_14C45(void)
{
	register int frame = (midboss.phase_frame - 1);

	if(frame == 0) {
		midboss.angle = iatan2(
			(player_pos.cur.y.v - midboss.pos.cur.y.v),
			(player_pos.cur.x.v - midboss.pos.cur.x.v)
		);
	}
	if((frame % 8) == 0) {
		snd_se_play(3);
		bullet_template.speed.v = (TO_SP(2) + 8);
		bullet_template.group = BG_SPREAD;
		bullet_template.count = 5;
		bullet_template.delta.spread_angle = 0x10;
		bullet_template.angle = midboss.angle;
		bullet_template_tune();
		bullets_add_regular();
		bullet_template.speed.v = 10;
		bullet_template.angle = iatan2(
			(player_pos.cur.y.v - midboss.pos.cur.y.v),
			(player_pos.cur.x.v - midboss.pos.cur.x.v)
		);
		bullet_template.patnum = PAT_BULLET16_D_BLUE;
		bullet_template.group = BG_SINGLE;
		bullet_template.special_motion = BSM_NONE;
	}
	if((frame % 32) >= 24) {
		bullet_template.spawn_type = BST_BULLET16_CLOUD_FORWARDS;
		_AL = bullet_template.speed.v;
		_AL += 10;
		bullet_template.speed.v = _AL;
		bullets_add_special();
	}
	if(frame >= 64) {
		midboss.phase_frame = 0;
		midboss2_pattern = 255;
	}
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
