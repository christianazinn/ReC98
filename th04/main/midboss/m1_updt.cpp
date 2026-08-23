/// Stage 1 midboss - update function
/// ---------------------------------
/// (#included from th04/mb_upd1.cpp, MB_UPD_TEXT's FIRST C++ object. It is a
/// separate object from th04/mb_upd.cpp for a layout reason, not a stylistic
/// one; see that file.)
///
/// midboss1_render() is th04/main/midboss/m1.cpp, in a different segment and
/// therefore a different object.

#include "platform.h"
#include "pc98.h"
#include "th04/snd/snd.h"
#include "th04/sprites/main_pat.h"
#include "th04/main/homing.hpp"
#include "th04/main/spark.hpp"
#include "th04/main/hud/hud.hpp"
#include "th04/main/bullet/bullet.hpp"
#include "th04/main/bullet/clearzap.hpp"
#include "th04/main/player/shot.hpp"
#include "th04/main/tile/tile.hpp"
#include "th04/main/phase.hpp"
// Last, and that is load-bearing: th04/main/playfld.hpp behind it pulls
// th04/main/scroll.hpp, whose `#pragma codeseg mai_TEXT main_01` / bare
// `#pragma codeseg` pair has to run before this object emits any code. That
// path is also the only expansion of that unguarded header in this TU.
#include "th04/main/midboss/midboss.hpp"

/// The midboss's own state
/// -----------------------
/// Both are th04_main.asm `.data?` slots with no `public` of ZUN's, and this
/// function plus midboss1_update() below are the only readers or writers of
/// either in any of the five binaries. **A naming round is owed** for both
/// spellings: neither has a counterpart in the Stage 2, 3 or 4 midbosses'
/// already-ruled state, so there is nothing to take a name from.
extern "C" {
	// Angle of the left half of the pellet fan below, rotating by 0x0C on
	// every volley.
	extern unsigned char midboss1_25594;

	// Written twice, both times with the VRAM line the midboss just moved to,
	// and read by nothing in any of the five binaries. ZUN bloat, and the
	// reason it keeps an address-suffixed name.
	extern vram_y_t midboss1_25596;
}
/// -----------------------

// Two pellet fans, mirrored around straight down, on every eighth frame of the
// Stage 1 midboss's last phase -- and only while it is still in the upper two
// thirds of the playfield. [midboss1_25594] is the angle of the left one; the
// right one is its mirror, and the pair rotates by 0x0C every time it fires.
//
// midboss1_update()'s phase 3 is the only caller, so this is `static` and the
// zero-byte `label` alias th04_main.asm carried for it is gone with the body.
static void near midboss1_13FB2(void)
{
	if(midboss.pos.cur.y.v < TO_SP(256)) {
		register int frame = midboss.phase_frame;

		if(frame == 1) {
			midboss1_25594 = 1;
		}
		if((frame % 8) == 0) {
			bullet_template.spawn_type = BST_BULLET16;
			bullet_template.patnum = PAT_BULLET16_D_BLUE;
			bullet_template.angle = midboss1_25594;
			bullet_template.speed.v = TO_SP(2);
			bullet_template.group = BG_SINGLE;
			bullet_template.special_motion = BSM_NONE;
			bullet_template_tune();
			bullets_add_special();

			// kb/codegen/0032: both of these keep the angle live in AL across
			// the store, which is why they are spelled through the
			// pseudo-register rather than as byte arithmetic on the variable.
			_AL = 0x80;
			_AL -= midboss1_25594;
			bullet_template.angle = _AL;
			bullets_add_special();
			_AL = midboss1_25594;
			_AL += 0x0C;
			midboss1_25594 = _AL;
		}
	}
}

void pascal far midboss1_update(void)
{
	int damage;

	if(midboss.phase == 0) {
		// The entrance: the midboss flies up against the scroll, laying down
		// the four tiles of its 32x32 shadow as it goes, and the four of the
		// larger one when it stops.
		midboss.pos.velocity.y.v = -TO_SP(1);
		midboss.pos.update_seg3();
		tile_ring_set(
			(midboss.pos.cur.x.v - TO_SP(16)),
			(midboss.pos.cur.y.v - TO_SP(16)),
			40
		);
		tile_ring_set(
			midboss.pos.cur.x.v, (midboss.pos.cur.y.v - TO_SP(16)), 41
		);
		tile_ring_set(
			(midboss.pos.cur.x.v - TO_SP(16)), midboss.pos.cur.y.v, 56
		);
		tile_ring_set(midboss.pos.cur.x.v, midboss.pos.cur.y.v, 57);
		midboss.phase_frame++;
		if(midboss.phase_frame >= 288) {
			midboss.phase = 1;
			midboss.phase_frame = 0;
			midboss.pos.velocity.y.v = 2;
			tile_ring_set(
				(midboss.pos.cur.x.v - TO_SP(16)),
				(midboss.pos.cur.y.v - TO_SP(16)),
				42
			);
			tile_ring_set(
				midboss.pos.cur.x.v, (midboss.pos.cur.y.v - TO_SP(16)), 43
			);
			tile_ring_set(
				(midboss.pos.cur.x.v - TO_SP(16)), midboss.pos.cur.y.v, 58
			);
			tile_ring_set(midboss.pos.cur.x.v, midboss.pos.cur.y.v, 59);
			midboss.sprite = 136;
			midboss.pos.cur.y.v -= TO_SP(4);
			midboss.pos.cur.y.v += scroll_subpixel_line.v;
			midboss1_25596 = scroll_subpixel_y_to_vram_seg3(
				midboss.pos.cur.y.v
			);
			sparks_add_circle(
				midboss.pos.cur.x, midboss.pos.cur.y, TO_SP(3), 32
			);
			snd_se_play(9);
		}
	} else if(midboss.phase == 1) {
		// Hatching, part 1: cels 136…139, one every 8th frame, and invincible
		// throughout -- the hittest runs and only plays a sound.
		midboss.pos.update_seg3();
		homing_target.x.v = midboss.pos.cur.x.v;
		homing_target.y.v = midboss.pos.cur.y.v;
		midboss.phase_frame++;
		if(midboss.sprite < 139) {
			if((midboss.phase_frame % 8) == 0) {
				midboss.sprite++;
			}
		} else if(midboss.phase_frame >= 96) {
			midboss.phase = 2;
			midboss.phase_frame = 0;
			midboss.sprite = 140;
			midboss.pos.cur.y.v -= TO_SP(16);
			midboss1_25596 = scroll_subpixel_y_to_vram_seg3(
				(midboss.pos.cur.y.v - TO_SP(16))
			);
			sparks_add_circle(
				midboss.pos.cur.x, midboss.pos.cur.y, TO_SP(3), 32
			);
			snd_se_play(9);
		}
		// Spelled out rather than through shot.hpp's 3-argument overload, at
		// all three call sites below: binding its `const subpixel_t&` radii to
		// two literals materialises two stack temporaries per call, and the
		// original has no stack frame at all -- `push bp` / `mov bp, sp`, not
		// an `enter`. th04/main/boss/b3_upd.cpp records the same measurement.
		shot_hitbox_radius.x.v = TO_SP(16);
		shot_hitbox_radius.y.v = TO_SP(12);
		shot_hitbox_center.x.v = midboss.pos.cur.x.v;
		shot_hitbox_center.y.v = midboss.pos.cur.y.v;
		if(shots_hittest()) {
			snd_se_play(10);
		}
	} else if(midboss.phase == 2) {
		// Hatching, part 2: cels 140…146 in steps of two, still invincible.
		midboss.pos.update_seg3();
		homing_target.x.v = midboss.pos.cur.x.v;
		homing_target.y.v = midboss.pos.cur.y.v;
		midboss.phase_frame++;
		if(midboss.sprite < 146) {
			if((midboss.phase_frame % 8) == 0) {
				midboss.sprite += 2;
			}
		} else {
			midboss.phase = 3;
			midboss.phase_frame = 0;
		}
		shot_hitbox_radius.x.v = TO_SP(24);
		shot_hitbox_radius.y.v = TO_SP(16);
		shot_hitbox_center.x.v = midboss.pos.cur.x.v;
		shot_hitbox_center.y.v = midboss.pos.cur.y.v;
		if(shots_hittest()) {
			snd_se_play(10);
		}
	} else if(midboss.phase == 3) {
		// The fight itself, which ends either by damage or by the stage
		// scrolling on -- [scroll_speed] is raised to 4 by the stage script
		// once the fight has gone on long enough.
		midboss.pos.update_seg3();
		if(scroll_speed.v > 2) {
			goto leaves;
		}
		homing_target.x.v = midboss.pos.cur.x.v;
		homing_target.y.v = midboss.pos.cur.y.v;
		midboss.phase_frame++;
		shot_hitbox_radius.x.v = TO_SP(24);
		shot_hitbox_radius.y.v = TO_SP(16);
		shot_hitbox_center.x.v = midboss.pos.cur.x.v;
		shot_hitbox_center.y.v = midboss.pos.cur.y.v;
		damage = shots_hittest();
		bullet_template.spawn_type = BST_PELLET;
		bullet_template.origin.x.v = midboss.pos.cur.x.v;
		bullet_template.origin.y.v = (midboss.pos.cur.y.v - TO_SP(1));
		midboss1_13FB2();
		if(damage) {
			midboss.hp -= damage;
			if(midboss.hp > 0) {
				midboss.damage_this_frame = 1;
				snd_se_play(4);
			} else {
				bullet_zap.active = true;
				midboss_score_bonus(5);

				// …and it leaves the same way whether the player killed it or
				// the stage simply scrolled past it.
leaves:
				midboss.phase = PHASE_EXPLODE_BIG;
				midboss.sprite = 4;
				midboss.phase_frame = 0;
				midboss.pos.velocity.y.v = 0;
				sparks_add_circle(
					midboss.pos.cur.x, midboss.pos.cur.y, TO_SP(8), 48
				);
				snd_se_play(12);
				scroll_speed.v = 4;
			}
		}
	} else {
		midboss_defeat_update();
	}
	hud_hp_update_and_render(midboss.hp, 620);
}
