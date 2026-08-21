/// Stage 1 Boss - Orange: the fight's own update function
/// ------------------------------------------------------
/// (#included from th04/main_033.cpp. main_033_TEXT has no other C++
/// contribution, and TLINK lays a segment's contributions out in link order
/// with the root dump first, so this object lands at the segment's tail by
/// construction — which is where this function already was.
/// kb/codegen/0112 + 0114.)
///
/// orange_bg_render() and orange_fg_render() are th04/main/boss/bg.cpp and
/// th04/main/boss/render.cpp, both in main_01 and both different objects.

#include "platform.h"
#include "pc98.h"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/v_colors.hpp"
#include "th02/main/player/player.hpp"
#include "th03/hardware/palette.hpp"
#include "th04/snd/snd.h"
#include "th04/sprites/main_pat.h"
#include "th04/math/randring.hpp"
#include "th04/main/bg.hpp"
#include "th04/main/circle.hpp"
#include "th04/main/gather.hpp"
#include "th04/main/homing.hpp"
#include "th04/main/spark.hpp"
#include "th04/main/hud/hud.hpp"
#include "th04/main/bullet/clearzap.hpp"
#include "th04/main/tile/bb.hpp"
#include "th04/main/boss/boss.hpp"

/// Still ASM
/// ---------
// Orange's five patterns, all of them in this same segment and all of them
// private to ZUN's object, so each needed a zero-byte `label` alias in
// th04_main.asm to become linkable (kb/codegen/0123). The address-suffixed
// names are the dump's own; naming them belongs to whoever lifts them.
extern "C" {
	void near orange_19686(void);  // [boss.mode] 1
	void near orange_19720(void);  // 2
	void near orange_197BB(void);  // 3
	void near orange_19814(void);  // 4
	void near orange_19878(void);  // phase 3's only pattern
	void near orange_1998B(void);  // phase 4's second half
}

// Declared FAR here, and only here: th04/main/boss/bosses.hpp declares the
// same function `near`, which is what it is, and that header is deliberately
// not included. A near reference under this object's `-zPmain_03` frames its
// offset on main_03, and orange_bg_render() lives in main_01.
// kb/codegen/0162.
void pascal far orange_bg_render(void);
/// ---------

/// State
/// -----
// The two `boss_statebyte` slots Orange uses, spelled the way th04_main.asm's
// own `boss_statebyte_t` overlay already names them.
#define patterns_done     boss_statebyte[15]
#define pattern_num_prev  boss_statebyte[14]
/// -----

/// Constants
/// ---------
static const int ORANGE_HP = 3050;

// Orange hovers around this point during her last two patterns, nudged back
// with a one-pixel dead zone on each axis.
static const subpixel_t ORANGE_HOVER_X = TO_SP(192);
static const subpixel_t ORANGE_HOVER_Y = TO_SP(80);
/// ---------

// Nudges Orange back towards ([ORANGE_HOVER_X], [ORANGE_HOVER_Y]). Written as
// a macro rather than a function because the original inlines it into both of
// the phases that use it, in full, twice.
#define orange_hover() { \
	if(boss.pos.cur.x.v < (ORANGE_HOVER_X - TO_SP(1))) { \
		boss.pos.velocity.x.v = 24; \
	} else if(boss.pos.cur.x.v > (ORANGE_HOVER_X + TO_SP(1))) { \
		boss.pos.velocity.x.v = -24; \
	} \
	if(boss.pos.cur.y.v < (ORANGE_HOVER_Y - TO_SP(1))) { \
		boss.pos.velocity.y.v = 12; \
	} else if(boss.pos.cur.y.v > (ORANGE_HOVER_Y + TO_SP(1))) { \
		boss.pos.velocity.y.v = -12; \
	} \
	boss.pos.update_seg3(); \
}

void pascal far orange_update(void)
{
	int i;

	gather_template.center.x.v = boss.pos.cur.x.v;
	gather_template.center.y.v = boss.pos.cur.y.v;

	switch(boss.phase) {
	case 0:
		if(boss.phase_frame == 0) {
			boss.hp = ORANGE_HP;
			boss.phase_end_hp = 1950;
			Palettes[0].c.r = 0;
			Palettes[0].c.g = 0;
			Palettes[0].c.b = 96;
			palette_changed = true;
		}
		boss.phase_frame++;
		if(boss.phase_frame == 192) {
			// The entrance gather: one huge ring that shrinks onto her for
			// the rest of the phase.
			boss.sprite += 2;
			snd_se_play(8);
			gather_template.center.x.v = (boss.pos.cur.x.v + TO_SP(8));
			gather_template.center.y.v = (boss.pos.cur.y.v - TO_SP(40));
			gather_template.radius.v = TO_SP(320);
			gather_template.ring_points = 32;
			gather_template.angle_delta = 3;
			gather_template.col = 7;
		} else if(boss.phase_frame > 320) {
			if(boss.phase_frame == 336) {
				gather_template.col = 6;
			}
			if((boss.phase_frame & 7) == 0) {
				gather_add_only();
			}
			if(boss.phase_frame >= 352) {
				boss.phase++;
				boss.phase_frame = 0;
				snd_se_play(13);
				patterns_done = 0;
				pattern_num_prev = -1;
				_asm mov word ptr bg_render_bombing_func, offset orange_bg_render
				tiles_bb_col = 0;
			}
		}
		boss_hittest_shots_damage(TO_SP(16), TO_SP(16), 10);
		break;

	case 1:
		boss.phase_frame++;
		if(boss.phase_frame >= 32) {
			// Back to the small gather rings the patterns use…
			gather_template.radius.v = TO_SP(64);
			gather_template.angle_delta = 2;
			gather_template.ring_points = 8;

			boss.phase = 2;
			boss.phase_frame = 0;
			boss.mode = 0;
			boss.sprite += 2;

			// …and one three-ring opening volley, each ring slower than the
			// one before it.
			bullet_template.spawn_type = BST_PELLET;
			bullet_template.origin.x.v = boss.pos.cur.x.v;
			bullet_template.origin.y.v = boss.pos.cur.y.v;
			bullet_template.group = BG_RING;
			bullet_template.count = 16;
			bullet_template.speed.v = TO_SP(4);
			bullet_template.angle = 0;
			bullet_template_tune();
			for(i = 0; i < 3; i++) {
				bullets_add_regular();
				bullet_template.speed.v -= TO_SP(1);
			}
			snd_se_play(6);
		}
		boss_hittest_shots_damage(TO_SP(16), TO_SP(16), 10);
		break;

	case 2:
		switch(boss.mode) {
		case 0:
			boss.phase_frame = 0;

			// Rerolled until it differs from the last one.
			do {
				// kb/codegen/0032: the original increments the returned byte
				// in AL and stores it, where `and(3) + 1` widens to `INC AX`.
				_AL = randring2_next16_and(3);
				_AL++;
				boss.mode = _AL;
			} while(pattern_num_prev == boss.mode);
			pattern_num_prev = boss.mode;

			patterns_done++;
			if(patterns_done >= 16) {
				goto phase_2_over;
			}
			break;
		case 1:
			orange_19686();
			break;
		case 2:
			orange_19720();
			break;
		case 3:
			orange_197BB();
			break;
		case 4:
			orange_19814();
			break;
		}
		if(!boss_hittest_shots()) {
			break;
		}
		boss_score_bonus(5);
phase_2_over:
		boss_phase_next(ET_NW_SE, 450);
		Palettes[0].c.r = 112;
		Palettes[0].c.b = 112;
		palette_changed = true;
		break;

	case 3:
		switch(boss.mode) {
		case 0:
			if(boss.phase_frame > 128) {
				boss.phase_frame = 0;
				boss.mode = 1;
			}
			break;
		case 1:
			orange_19878();
			break;
		}
		if(boss.phase_frame <= 1500) {
			if(!boss_hittest_shots()) {
				break;
			}
			boss_score_bonus(5);
		}
		boss_phase_next(ET_NW_SE, 0);
		boss.sprite += 4;
		Palettes[0].c.r = 144;
		Palettes[0].c.b = 32;
		palette_changed = true;
		gather_template.col = 9;
		break;

	case 4:
		bullet_template.origin.x.v = (boss.pos.cur.x.v + TO_SP(8));
		bullet_template.origin.y.v = (boss.pos.cur.y.v - TO_SP(16));
		switch(boss.mode) {
		case 0:
			if(boss.phase_frame == 96) {
				gather_template.center.x.v = bullet_template.origin.x.v;
				gather_template.center.y.v = bullet_template.origin.y.v;
				gather_add_only();
			}
			if(boss.phase_frame == 112) {
				circles_add_shrinking(
					bullet_template.origin.x.v, bullet_template.origin.y.v
				);
				circles_color = V_WHITE;
			}
			if(boss.phase_frame > 128) {
				boss.phase_frame = 0;
				boss.mode = 1;
			}
			orange_hover();
			break;
		case 1:
			orange_1998B();
			break;
		}
		if(boss.phase_frame <= 600) {
			if(!boss_hittest_shots()) {
				break;
			}
		}

		// The defeat bonus is the one thing that distinguishes killing Orange
		// from surviving her: the timeout takes the same branch.
		if(boss.phase_frame <= 600) {
			boss.phase_state.defeat_bonus = true;
		} else {
			boss.phase_state.defeat_bonus = false;
		}
		boss_explode_small(ET_HORIZONTAL);
		boss.phase++;
		boss.phase_frame = 0;
		boss.mode = 0;
		sparks_add_circle(
			boss.pos.cur.x, boss.pos.cur.y, TO_SP(8), 48
		);
		break;

	case 5:
		orange_hover();
		boss.phase_frame++;
		if(boss.phase_frame == 16) {
			boss_explode_small(ET_VERTICAL);
		}
		if(boss.phase_frame == 32) {
			boss_defeat_explode_big(ET_CIRCLE, 10);
			snd_se_play(12);

			// Only two of the three components, like Marisa's.
			Palettes[0].c.r = 0;
			Palettes[0].c.b = 0;
			palette_changed = true;
			player_invincibility_time = BOSS_DEFEAT_INVINCIBILITY_FRAMES;
		}
		break;

	default:
		boss_defeat_update();
		return;
	}

	homing_target.x.v = boss.pos.cur.x.v;
	homing_target.y.v = boss.pos.cur.y.v;
	hud_hp_update_and_render(boss.hp, ORANGE_HP);
}
/// ------------------------------------------------------
