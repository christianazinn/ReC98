/// Stage 5 Boss - Yumeko
/// ---------------------
/// yumeko_update() was the last emitting proc of th05_main.asm's contribution
/// to main_035_TEXT: the `dw offset loc_…` run below it is the jump table its
/// own switch statement compiles to, not data the dump owns
/// (state/re/JUMP_TABLE_TAILS.md), so the C++ emits the alignment pad and the
/// table along with the function (kb/codegen 0099 + 0104).
///
/// That contribution had **no C++ successor at all**, and this file is the
/// answer to it rather than a block on it: TLINK lays a segment's
/// contributions out in link order and the root dump is the first object it is
/// handed, so a new object naming the segment lands at that segment's tail by
/// construction (kb/codegen 0112). One new translation unit and one
/// Tupfile.lua line -- no carve (kb/codegen/0080), no new segment name, no
/// group-list edit. This is the first lift out of the 0x2FE4 bytes of ZUN
/// assembly that block still holds, and every later one grows this object
/// backwards into the hole the same way th05/boss_4.cpp grew.
///
/// The `-a2` below is what puts the one-byte pad under the jump table. With
/// this object holding nothing else, the table's natural offset is the
/// function's own length, and the pad was read out of the OBJ's PUBDEF
/// records, never out of a `tcc -S` listing (kb/codegen 0159 + 0160).
///
/// What this file still reaches in th05_main.asm
/// --------------------------------------------
/// Everything Yumeko has except the two functions below: eight more bodies are
/// still ZUN assembly directly above, and so are the two pattern tables in
/// _DATA and the function pointer in _BSS that select between them. Six of
/// those bodies are reached from here, the other two only from the tables, and
/// none of the nine things this file names was published -- so the parcel adds
/// a zero-byte kb/codegen/0123 alias beside each of the nine. Every one of them
/// goes away again with the body or table it names, as the chain above is
/// lifted.

#pragma option -zCmain_035_TEXT -zPmain_03

#include "libs/master.lib/pc98_gfx.hpp"
#include "th04/main/pattern.hpp"
#include "th04/snd/snd.h"
#include "th04/main/bg.hpp"
#include "th04/main/gather.hpp"
#include "th04/main/homing.hpp"
#include "th04/main/hud/hud.hpp"
#include "th04/main/custom.hpp"
#include "th04/main/player/shot.hpp"
#include "th05/main/boss/bosses.hpp"

// The two near function pointers this file stores point OUT of group main_03,
// and Turbo C++ frames every near code reference on the object's own `-zP`
// group unless the declaration says which segment the target is in
// (kb/codegen/0162). Declaring each one inside its real segment is what
// th05/main/boss/bosses.hpp already does for the *_bg_render() family, whose
// BOSS_BG_TEXT is in main_01 exactly like these two -- so this is that same
// balanced `#pragma codeseg` pair, not a new device. The bare pragma restores
// this file's own `-zC` default.
//
// th04/main/null.hpp is therefore NOT included: it declares nullfunc_near()
// with no segment at all, and TH04's copy of it is in a different one anyway.
#pragma codeseg PLAYER_B_TEXT
extern "C" void pascal near nullfunc_near(void);
#pragma codeseg

#pragma codeseg MIDBOSSX_TEXT
extern "C" void pascal near swords_render(void);
#pragma codeseg

#include "th05/main/bullet/sword.hpp"
#include "th05/sprites/main_pat.h"

// What this file still reaches in th05_main.asm
// ---------------------------------------------

// Yumeko's currently selected pattern, and the two tables it is picked from --
// the structural twin of [phase_2_3_pattern] and SARA_PATTERNS_PHASE_2_3 in
// b1.cpp, and of [shinki_phase_2_3_pattern] and SHINKI_PATTERNS_PHASE_2_3 in
// b6.cpp. Named on the same formula those two follow: the phase that draws
// from the table qualifies its name. Both are indexed by
// [boss.phase_state.patterns_seen] & 1, which is what sizes them at 2; the
// `dw 0` words behind each one in the dump are not indexed from anywhere and
// stay unnamed data.
extern "C" pattern_loop_func_t yumeko_pattern;
extern "C" const pattern_loop_func_t YUMEKO_PATTERNS_PHASE_2[2];
extern "C" const pattern_loop_func_t YUMEKO_PATTERNS_PHASE_5[2];

// [measured] Flies the boss at a random angle and the given [speed], bouncing
// it off a box of its own inside the playfield, and returns true once
// [boss.phase_frame] has reached [frames]. `pascal`, therefore published and
// declared UPPERCASE (kb/codegen/0102); the other five below are `near`
// void(void) and carry the ordinary underscore alias.
extern "C" bool pascal near yumeko_flystep_bounce(
	subpixel_t speed, int frames
);

// The four bodies this function assigns to [yumeko_pattern] directly. The two
// that only the tables above reach keep no declaration here. Address-suffixed
// hand names, not placeholders: what each one shoots is measurable, but
// nothing in the fight names the patterns, and the parcel that lifts each body
// is the one that can describe it.
extern "C" void near yumeko_1CA42(void);
extern "C" void near yumeko_1CB71(void);
extern "C" void near yumeko_1CCD3(void);
extern "C" void near yumeko_1CED9(void);

// Phase 8's body, called directly rather than through [yumeko_pattern]. The
// one name of the ten the dump already carried; phase 10's body is below,
// because this object needed its length.
extern "C" void near yumeko_1D085(void);
// ---------------------------------------------

// Constants
// ---------

// [inferred] Yumeko's cels, from the same PAT_STAGE base that every stage's
// own sprites start at, and spelled after PAT_SHINKI_STILL / PAT_SHINKI_CAST,
// the attested formula for this pair one boss over. Kept file-local for the
// reason th05/main/boss/b3.cpp gives for Alice's: main_pat.h has no Stage 5
// boss block yet and is shared with every other TU in this binary.
static const int PAT_YUMEKO_STILL = (PAT_STAGE + 0);
static const int PAT_YUMEKO_CAST = (PAT_STAGE + 4);

// [inferred] Only ever shown while she is moving: during her entrance, and
// while yumeko_flystep_bounce() is carrying her.
static const int PAT_YUMEKO_FLY = (PAT_STAGE + 8);

// The sword sprites this fight blits through master.lib's *_tiny*() functions,
// converted on the first frame of the HP fill because the dialog script did
// not do it.
//
// [measured] The range is [PAT_SWORD, 229), one *below* the
// PAT_DECAY_SWORD_last that th05/sprites/main_pat.h computes: its
// `PAT_SWORD_last = (PAT_SWORD + BULLET_V_CELS)` line is missing the `- 1`
// that every sibling `_last` in that file carries, which pushes PAT_DECAY_SWORD
// and everything after it up by one. Spelled as a literal here rather than
// reconciled, because moving PAT_DECAY_SWORD moves a patnum other code already
// resolves, and that is not this parcel's call to make.
static const int TINY_SWORD_END = 229;

// Always denotes the last phase that ends with that amount of HP, exactly as
// shinki_hp_t in b6.cpp does. Phases 2/5, 3/6 and 4/7 share one arm each here,
// so the numbers are the only thing that tells the two halves of the fight
// apart.
enum yumeko_hp_t {
	HP_TOTAL = 8300,
	HP_PHASE_2_END = 7500,
	HP_PHASE_4_END = 5700,
	HP_PHASE_5_END = 4500,
	HP_PHASE_7_END = 2700,
	HP_PHASE_8_END = 1200,
	HP_PHASE_10_END = 0,
};
// ---------

// Game logic
// ----------

// Phase 10: a 3-stack gather, then one BSM_DECELERATE_THEN_TURN ring every
// 16th frame, alternately turning clockwise and counter-clockwise and mirroring
// its base angle around 0x80 on the clockwise ones.
//
// This body is in the parcel for a second, purely mechanical reason: it is
// 0xA5 bytes long, and the ODD prefix is what puts the `-a2` pad in front of
// yumeko_update()'s jump table. At a zero prefix the pad is absent -- read off
// this object's own SEGDEF, not off a listing, per kb/codegen 0159 + 0160,
// which is also why the direction was probed rather than predicted.
//
// [boss_statebyte] slots used here are left as raw indices, the way
// th04/main/midboss/mx_update.cpp leaves its own: [14] and [15] are shared
// with the six Yumeko bodies still in th05_main.asm, and naming them from one
// of the seven is how a name gets contradicted by the next lift.
void near yumeko_1D1C6(void)
{
	if(boss.phase_frame < 32) {
		gather_add_only_3stack((boss.phase_frame - 16), 7, 6);
		if(boss.phase_frame == 16) {
			boss.sprite = PAT_YUMEKO_CAST;
			bullet_template.spawn_type = BST_NO_DECELERATE;
			bullet_template.patnum = PAT_BULLET16_V_BLUE;
			bullet_template.group = BG_RING;
			bullet_template.spread = 18;
			bullet_template.special_motion = BSM_DECELERATE_THEN_TURN;
			bullet_template.speed.set(2.375f);
			bullet_template_tune();
			snd_se_play(8);
			boss_statebyte[15] = 0;
			bullet_special.turns_max = 2;
			boss_statebyte[14] = 0;
		}
	} else if((boss.phase_frame % 16) == 0) {
		bullet_template.angle = boss_statebyte[14];
		if(boss_statebyte[15] & 1) {
			bullet_template_special_angle.turn_by = 0x40;
			bullet_template.angle = (0x80 - bullet_template.angle);
		} else {
			bullet_template_special_angle.turn_by = -0x40;
		}
		bullets_add_special_fixedspeed();
		boss_statebyte[14]++;
		boss_statebyte[15]++;
		snd_se_play(3);
	}
}

#pragma option -a2

void pascal yumeko_update(void)
{
	int i;

	homing_target = boss.pos.cur;
	bullet_template.origin = boss.pos.cur;
	gather_template.center = boss.pos.cur;
	sword_template.origin = boss.pos.cur;
	boss.phase_frame++;

	switch(boss.phase) {
	case PHASE_HP_FILL:
		if(boss.phase_frame == 1) {
			boss.hp = HP_TOTAL;
			boss.phase_end_hp = HP_PHASE_2_END;
			gather_template.radius.set(BOSS_W / 1.0f);
			gather_template.angle_delta = 0x02;
			gather_template.ring_points = 8;
			boss.sprite = PAT_YUMEKO_STILL;
			boss.pos.velocity.x.set(4.0f);
			for(i = PAT_SWORD; i < TINY_SWORD_END; i++) {
				super_convert_tiny(i);
			}
			boss_sprite_left = PAT_YUMEKO_STILL;
			boss_sprite_right = PAT_YUMEKO_STILL;
			boss_sprite_stay = PAT_YUMEKO_STILL;
		}

		// Yuki's sprite is still on screen from the Stage 4 fight, and gets
		// pushed off the top of the playfield here rather than by the stage
		// transition. She stays shootable the whole way up.
		if(boss2.pos.cur.y >= to_sp(-32.0f)) {
			boss2.pos.cur.y.v -= to_sp(1.0f);

			// Spelled out rather than through the shots_hittest() overload
			// that takes the box: that one copies the two radii through stack
			// temporaries, and this function has no stack frame at all.
			shot_hitbox_radius.x.v = to_sp(24.0f);
			shot_hitbox_radius.y.v = to_sp(24.0f);
			shot_hitbox_center = boss2.pos.cur;
			if(shots_hittest()) {
				snd_se_play(10);
			}
		}

		if(boss.phase_frame < 64) {
			boss_hittest_shots_invincible();
		} else {
			boss.sprite = PAT_YUMEKO_FLY;
			boss.pos.cur.x.v += to_sp(2.0f);

			// Timeout condition
			if(boss.pos.cur.x.v >= to_sp(192.0f)) {
				// Next phase
				boss.sprite = PAT_YUMEKO_STILL;
				boss.phase++;
				boss.phase_frame = 0;
				snd_se_play(13);
				bg_render_bombing_func = yumeko_bg_render;
			}
		}
		break;

	case PHASE_BOSS_ENTRANCE_BB:
		boss_hittest_shots_invincible();

		// Timeout condition
		if(boss.phase_frame >= 64) {
			// Next phase
			boss.phase++;
			boss.phase_frame = 0;
			boss.mode = 1;
			boss.phase_state.patterns_seen = 0;
			yumeko_pattern = yumeko_1CA42;
			boss_custombullets_render = swords_render;
		}
		break;

	case 2:
	case 5:
		switch(boss.mode) {
		case 0:
			if(yumeko_flystep_bounce(to_sp(2.0f), 64)) {
				boss.phase_frame = 0;
				boss.mode++;
				boss.phase_state.patterns_seen++;

				// Timeout condition
				if(boss.phase_state.patterns_seen >= 20) {
					goto phase_2_5_timed_out;
				}
				if(boss.phase == 2) {
					yumeko_pattern = YUMEKO_PATTERNS_PHASE_2[
						boss.phase_state.patterns_seen & 1
					];
				} else {
					yumeko_pattern = YUMEKO_PATTERNS_PHASE_5[
						boss.phase_state.patterns_seen & 1
					];
				}
			}
			break;

		// [measured] a second labelled arm, not a default one: the dispatch
		// compiles to a zero test, a compare against 1, and a fallthrough
		// jump, which is a two-label switch with no default arm rather than a
		// one-label one.
		case 1:
			yumeko_pattern();
			break;
		}
		if((boss.mode != 0) && boss_hittest_shots()) {
			boss_score_bonus(10);
phase_2_5_timed_out:
			// Next phase
			if(boss.phase == 2) {
				boss_phase_next(ET_CIRCLE, HP_PHASE_4_END);
			} else {
				boss_phase_next(ET_HORIZONTAL, HP_PHASE_7_END);
			}
		}
		break;

	case 3:
	case 6:
		boss_hittest_shots();
		if(boss_flystep_towards(to_sp(PLAYFIELD_W / 2), to_sp(64.0f))) {
			// Next phase
			boss.phase++;
			boss.phase_frame = 0;
			if(boss.phase == 4) {
				yumeko_pattern = yumeko_1CB71;
			} else {
				yumeko_pattern = yumeko_1CED9;
			}
		}
		break;

	case 4:
	case 7:
		yumeko_pattern();
		if(boss.phase_frame < 2000) {
			if(!boss_hittest_shots()) {
				break;
			}
			boss_score_bonus(10);
		}

		// Next phase
		if(boss.phase == 4) {
			boss_phase_next(ET_NW_SE, HP_PHASE_5_END);
			yumeko_pattern = yumeko_1CCD3;
		} else {
			boss_phase_next(ET_NW_SE, HP_PHASE_8_END);
		}
		boss.mode = 1;
		break;

	case 8:
		yumeko_1D085();
		if(boss.phase_frame < 2000) {
			if(!boss_hittest_shots()) {
				break;
			}
			boss_score_bonus(10);
		}

		// Next phase
		boss_phase_next(ET_NW_SE, HP_PHASE_10_END);
		break;

	case 9:
		boss_hittest_shots();
		if(boss_flystep_towards(to_sp(PLAYFIELD_W / 2), to_sp(96.0f))) {
			// Next phase
			boss.phase++;
			boss.phase_frame = 0;
			boss.phase_state.patterns_seen = 0;
		}
		break;

	case 10:
		yumeko_1D1C6();
		if(boss.phase_frame < 1200) {
			if(!boss_hittest_shots()) {
				break;
			}
			boss.phase_state.defeat_bonus = true;
		}

		// Next phase
		boss_explode_small(ET_VERTICAL);
		boss.phase_frame = 0;
		boss.phase = PHASE_BOSS_EXPLODE_SMALL;
		boss_custombullets_render = nullfunc_near;
		break;

	default:
		boss_defeat_update(65);
		return;
	}

	swords_update();
	hud_hp_update_and_render(boss.hp, HP_TOTAL);
}

#pragma option -a1
// ----------
