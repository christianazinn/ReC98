/// Stage 4 Bosses - Mai & Yuki, PAIR update
/// ----------------------------------------
/// THE Stage 4 [boss_update_func]: the first half of the fight, with both
/// characters alive at once and sharing one HP bar, until one of them is
/// defeated. th05/main/boss/b4_pair_fg.cpp is the matching renderer and
/// documents who is who during the explosion phases;
/// th05/main/boss/b4_both.cpp holds the two-boss shot hittest and the paired
/// flystep this function drives.
///
/// The second half of the fight is a *different* [boss_update]. The last case
/// below loads the survivor's dialog and switches [boss_update] to
/// yuki_update() (th05/main/boss/b4_yuki.cpp) or mai_update()
/// (th05/main/boss/b4_mai.cpp).
///
/// (#included from th05/b4pair.cpp, which is its OWN object, so this file
/// names every header it needs and shares none of them -- kb/codegen/0129 has
/// nothing to collide. That object exists rather than growing th05/b4mai.cpp
/// backwards because of the `-a2` pad in front of the two tables at the end:
/// @mai_yuki_update$qv takes its pad on the OPPOSITE parity from the three
/// jump tables already in b4mai.obj, so no single object can hold all four.
/// The measurement is in state/notes/th05-main-mai-update.md.)

// CHEETOS_RENDER is in PLAYFLD_TEXT, tiles_render_all() in STD_TEXT and
// B4_SOLO_FG_RENDER in MIDBOSSX_TEXT -- all three group main_01, while this
// object is -zPmain_03. Turbo C++ frames every near code reference on the
// object's own group unless the declaration says which segment the target is
// in (kb/codegen/0162), and this function stores the address of all three.
//
// The cheetos_render() declaration has to come BEFORE
// th05/main/bullet/cheeto.hpp below, which declares the same function with no
// segment at all: the first declaration a translation unit sees is the one
// Turbo frames the reference on, measured in th05/main/boss/b4_mai.cpp on
// exactly this class of store. The other two are declared here rather than
// through th04/main/tile/tile.hpp and a header of b4_solo_fg_render()'s own
// (there is none), for the reason b4_mai.cpp gives for nullfunc_near(): a
// header that declares the function with no segment is worse than no header.
#pragma codeseg PLAYFLD_TEXT
extern "C" void pascal near cheetos_render(void);
#pragma codeseg

#pragma codeseg STD_TEXT
void pascal near tiles_render_all(void);
#pragma codeseg

#pragma codeseg MIDBOSSX_TEXT
extern "C" void pascal near b4_solo_fg_render(void);
#pragma codeseg

#include "libs/master.lib/pc98_gfx.hpp"
#include "th03/hardware/palette.hpp"
#include "th04/main/pattern.hpp"
#include "th04/snd/snd.h"
#include "th04/main/bg.hpp"
#include "th04/main/custom.hpp"
#include "th04/main/frames.h"
#include "th04/main/gather.hpp"
#include "th04/main/homing.hpp"
#include "th04/main/hud/hud.hpp"
#include "th04/main/slowdown.hpp"
#include "th04/main/bullet/clearzap.hpp"
#include "th04/main/dialog/dialog.hpp"
#include "th05/formats/dialog.hpp"
#include "th05/main/boss/bosses.hpp"
#include "th05/main/bullet/cheeto.hpp"
#include "th05/main/bullet/laser.hpp"
#include "th05/main/player/player.hpp"
#include "th05/sprites/main_pat.h"

// What this file still reaches in th05_main.asm
// ---------------------------------------------
// Zero-byte kb/codegen/0123 aliases, every one of them published off a
// `label` beside ZUN's own definition, on the model of [fp_2CE32] and
// [off_22770] one boss over (th05/main/boss/b3.cpp). The dump's own names are
// kept: everything here is either still ZUN's assembly, or a table pointing
// into assembly that is still there, so all of it belongs to the parcel that
// lifts the block above -- not to this one.

// The two currently selected danmaku patterns, one per character. ZUN's
// assembly right above this block calls them -- mai_yuki_1A556() steps
// [mai_pair_pattern] with Mai's gather animation, mai_yuki_1A5B3() steps [yuki_pair_pattern]
// with Yuki's -- and mai_yuki_update() is the only thing that ever assigns
// them. Two variables rather than a two-element array: every assignment below
// names one of them directly. Structural twin of [fp_2CE2A] / [fp_2CE2C] in
// b3.cpp.
extern "C" pattern_oneshot_func_t mai_pair_pattern;
extern "C" pattern_oneshot_func_t yuki_pair_pattern;

// The five pattern tables the assignments below draw from, in the dump's
// address order: Mai's set for the first and third pattern block, then Yuki's
// three. All five are indexed by [boss.phase_state.patterns_seen] & 3, which
// is what sizes them at 4 -- the dump's [MAI_PAIR_PATTERNS_3] block carries four more
// zero words behind its four entries, and nothing reads them.
extern "C" const pattern_oneshot_func_t MAI_PAIR_PATTERNS_1[4];
extern "C" const pattern_oneshot_func_t MAI_PAIR_PATTERNS_3[4];
extern "C" const pattern_oneshot_func_t YUKI_PAIR_PATTERNS_1[4];
extern "C" const pattern_oneshot_func_t YUKI_PAIR_PATTERNS_2[4];
extern "C" const pattern_oneshot_func_t YUKI_PAIR_PATTERNS_3[4];

// The two per-character pattern steps mai_yuki_update() calls once both are
// flying, and the three patterns it installs by name rather than through a
// table: Mai's first, Mai's laser, and Yuki's first. The two steps return
// nothing at all -- neither ever loads AL before `retn` -- while all three
// patterns are `pattern_oneshot_func_t`, which is what the tables above are
// made of.
extern "C" void near mai_yuki_1A556(void);
extern "C" void near mai_yuki_1A5B3(void);
extern "C" bool near mai_yuki_1A5EB(void);
extern "C" bool near mai_yuki_1A775(void);
extern "C" bool near mai_yuki_1A8C9(void);

// The two dialog scripts and the two BGM titles the last case picks between,
// all four still in the dump's _DATA.
extern "C" const char a_dm09_tx2[];
extern "C" const char a_dm08_tx2[];
extern "C" shiftjis_t aTH05_10[];
extern "C" shiftjis_t aTH05_11[];
// ---------------------------------------------

// The two second-half update functions, both in th05/b4mai.cpp. `far`,
// because [boss_update] is a `far` pointer and the last case below stores a
// segment and an offset into it. yuki_update() has C linkage and
// mai_update() does not, which is exactly the difference between the two
// symbols the dump used to declare at the head of this segment
// (kb/codegen 0081 + 0102).
void pascal mai_update(void);
extern "C" void pascal yuki_update(void);

// The three functions th05/main/boss/b4_both.cpp defines for this one, in
// B4_UPDATE_TEXT -- a different object, but the same group main_03, so these
// need no segment frame. No header declares them: that file is the only
// definition and this is the only C++ caller, so the three `procdesc`s
// th05_main.asm carried for exactly these call sites go with the lift.
int pascal near yuki_hittest_shots_damage(
	subpixel_t radius_x, subpixel_t radius_y, int se_on_hit
);
unsigned char near mai_yuki_hittest_shots(void);
bool pascal near mai_yuki_flystep_random(int frame);

// Both characters start with half of this, and the HUD bar mai_yuki_update()
// ends with shows their sum against it.
static const int MAI_YUKI_HP_TOTAL = 9000;
static const int MAI_YUKI_HP_EACH = (MAI_YUKI_HP_TOTAL / 2);

// Sums of both characters' HP that force the fight ahead to the two interlude
// patterns below, each of which warps the pair back to the top of the
// playfield behind an explosion.
static const int MAI_YUKI_HP_INTERLUDE_1 = 5500;
static const int MAI_YUKI_HP_INTERLUDE_2 = 2250;

// [measured] Pattern counts, all three read off the three-way selection in
// mai_yuki_update(): patterns 0 to INTERLUDE_1 come from the first pair of
// tables, INTERLUDE_1 + 1 to INTERLUDE_2 give Mai the laser and Yuki her
// second table, INTERLUDE_2 + 1 to TIMEOUT - 1 use the third pair, and the
// pair goes down on its own at TIMEOUT. The fight ends on HP long before that.
static const int MAI_YUKI_PATTERNS_INTERLUDE_1 = 9;
static const int MAI_YUKI_PATTERNS_INTERLUDE_2 = 14;
static const int MAI_YUKI_PATTERNS_TIMEOUT = 36;

// [measured] ZUN quirk: the survivor always enters the second half with
// *Yuki's* HP total, even when it is Mai who survived -- mai_update() then
// runs the rest of her fight against MAI_HP_TOTAL, which is 7800. Not a fix
// candidate on a match branch.
static const int B4_SOLO_HP_TOTAL = 7900;

// One curved cheeto out of each character every 48 frames, aimed a quarter
// turn to the side of the player that the OTHER character is not on. Yuki's
// third-block pattern and Mai's third-block one are the same body with the
// spawner, the colour and the comparison's sense swapped; they are
// [YUKI_PAIR_PATTERNS_3]'s and [MAI_PAIR_PATTERNS_3]'s first entries, and both are near-verbatim
// copies of pattern_aimed_cheetos() in th05/main/boss/bx_updt.cpp.
//
// Yuki's never ends -- mai_yuki_update()'s own HP check is what stops the
// block -- so it is the `bool` half of the pair.
bool near mai_yuki_1AB1F(void)
{
	unsigned char angle;

	if((boss.phase_frame == 48) || (boss.phase_frame == 96)) {
		cheeto_template.origin = yuki.pos.cur;
		cheeto_template.col = 11;
		cheeto_template.speed.set(2.0f);
		// An `if`/`else` rather than a `?:`: the ternary materializes the
		// constant into AL and then stores it into a compiler temp AND the
		// named local, which is two extra bytes and two stack words
		// (measured).
		if(boss.pos.cur.x.v >= yuki.pos.cur.x.v) {
			angle = 0x40;
		} else {
			angle = -0x40;
		}
		// The three-argument overload, NOT the PlayfieldPoint one: the
		// inline forwarding of [plus_angle] copies the byte local into a
		// second stack word, where the original pushes the local's own word
		// straight from the frame (kb/codegen/0024).
		cheeto_template.angle = player_angle_from(
			cheeto_template.origin.x, cheeto_template.origin.y, angle
		);
		cheetos_add();
		snd_se_play(15);
	}
	return false;
}

// Mai's half of the pair above. Returned as an expression rather than through
// an `if` with two `return` statements for the reason
// pattern_aimed_cheetos() gives: the `if` form gives each constant its own
// epilogue, and the original materializes the comparison into AX and falls
// into ONE epilogue through a `jmp short`.
int near mai_yuki_1AB76(void)
{
	unsigned char angle;

	if((boss.phase_frame == 48) || (boss.phase_frame == 96)) {
		cheeto_template.origin = boss.pos.cur;
		cheeto_template.col = 9;
		cheeto_template.speed.set(2.0f);
		if(boss.pos.cur.x.v < yuki.pos.cur.x.v) {
			angle = 0x40;
		} else {
			angle = -0x40;
		}
		// The three-argument overload, NOT the PlayfieldPoint one: the
		// inline forwarding of [plus_angle] copies the byte local into a
		// second stack word, where the original pushes the local's own word
		// straight from the frame (kb/codegen/0024).
		cheeto_template.angle = player_angle_from(
			cheeto_template.origin.x, cheeto_template.origin.y, angle
		);
		cheetos_add();
		snd_se_play(15);
	}
	return (boss.phase_frame == 128);
}

#pragma option -a2

void pascal mai_yuki_update(void)
{
	// ZUN reuses one stack word for two unrelated values -- the [frame]
	// handed to the paired flystep, and which of the two characters ended its
	// phase this frame. Two locals would be two stack words.
	int frame_or_defeated;
	int i;

	// Bullets home in on whichever of the two is further down the playfield.
	// Not a `?:` -- that yields a reference to a class type and copies the
	// Point through a pointer; Turbo's own cross-jumping merges the single
	// 32-bit store back out of these two branches.
	if(boss.pos.cur.y.v > yuki.pos.cur.y.v) {
		homing_target = boss.pos.cur;
	} else {
		homing_target = yuki.pos.cur;
	}
	boss.phase_frame++;

	switch(boss.phase) {
	case PHASE_HP_FILL:
		if(boss.phase_frame == 1) {
			boss.hp = MAI_YUKI_HP_EACH;
			boss.phase_end_hp = 0;
			yuki.hp = MAI_YUKI_HP_EACH;
			yuki.phase_end_hp = 0;
			gather_template.radius.set(GATHER_RADIUS_START);
			gather_template.angle_delta = 0x02;
			gather_template.ring_points = 8;
			for(i = TINY_B4BALL_START; i < TINY_B4BALL_END; i++) {
				super_convert_tiny(i);
			}
		}
		boss_hittest_shots_invincible();
		yuki_hittest_shots_damage(
			to_sp(BOSS_HITBOX_DEFAULT_W), to_sp(BOSS_HITBOX_DEFAULT_H), 10
		);
		if(boss.phase_frame >= 128) {
			boss.phase++;
			boss.phase_frame = 0;
			snd_se_play(13);
			bg_render_bombing_func = mai_yuki_bg_render;
		}
		break;

	case PHASE_BOSS_ENTRANCE_BB:
		boss_hittest_shots_invincible();
		yuki_hittest_shots_damage(
			to_sp(BOSS_HITBOX_DEFAULT_W), to_sp(BOSS_HITBOX_DEFAULT_H), 10
		);
		if(boss.phase_frame >= 64) {
			boss.phase++;
			boss.phase_frame = 0;
			boss.mode = 1;
			boss.phase_state.patterns_seen = 0;
			mai_pair_pattern = mai_yuki_1A5EB;
			yuki_pair_pattern = mai_yuki_1A8C9;
			boss_custombullets_render = cheetos_render;
		}
		break;

	// The whole fight, in one phase: [boss.mode] alternates between flying to
	// a new position and running the two patterns picked on arrival.
	case 2:
		switch(boss.mode) {
		case 0:
			if(
				(boss.phase_state.patterns_seen ==
					MAI_YUKI_PATTERNS_INTERLUDE_1) ||
				(boss.phase_state.patterns_seen ==
					MAI_YUKI_PATTERNS_INTERLUDE_2)
			) {
				// The interlude warp starts 48 frames later than a regular
				// one, which is what the explosion below fills.
				frame_or_defeated = (boss.phase_frame - 64);
				if(boss.phase_frame == 16) {
					bullets_clear();
					snd_se_play(15);
					boss_explode_small(ET_NW_SE);
					boss2_explode_small(ET_SW_NE);
				}
			} else {
				frame_or_defeated = (boss.phase_frame - 16);
			}
			if(mai_yuki_flystep_random(frame_or_defeated)) {
				boss.phase_frame = 0;
				boss.mode++;
				boss.phase_state.patterns_seen++;
				if(boss.phase_state.patterns_seen <
					(MAI_YUKI_PATTERNS_INTERLUDE_1 + 1)
				) {
					mai_pair_pattern = MAI_PAIR_PATTERNS_1[
						boss.phase_state.patterns_seen & 3
					];
					yuki_pair_pattern = YUKI_PAIR_PATTERNS_1[
						boss.phase_state.patterns_seen & 3
					];
				} else if(boss.phase_state.patterns_seen <
					(MAI_YUKI_PATTERNS_INTERLUDE_2 + 1)
				) {
					mai_pair_pattern = mai_yuki_1A775;
					yuki_pair_pattern = YUKI_PAIR_PATTERNS_2[
						boss.phase_state.patterns_seen & 3
					];
				} else if(
					boss.phase_state.patterns_seen < MAI_YUKI_PATTERNS_TIMEOUT
				) {
					mai_pair_pattern = MAI_PAIR_PATTERNS_3[
						boss.phase_state.patterns_seen & 3
					];
					yuki_pair_pattern = YUKI_PAIR_PATTERNS_3[
						boss.phase_state.patterns_seen & 3
					];
				} else {
					// Timeout condition
					boss.phase_state.defeat_bonus = false;
					frame_or_defeated = 1;
					goto defeated;
				}
			}
			break;

		case 1:
			mai_yuki_1A556();
			mai_yuki_1A5B3();
			break;
		}
		if(
			((boss.hp + yuki.hp) < MAI_YUKI_HP_INTERLUDE_1) &&
			(boss.phase_state.patterns_seen < MAI_YUKI_PATTERNS_INTERLUDE_1)
		) {
			boss.mode = 0;
			boss.phase_frame = 0;
			boss.phase_state.patterns_seen = MAI_YUKI_PATTERNS_INTERLUDE_1;
		} else if(
			((boss.hp + yuki.hp) < MAI_YUKI_HP_INTERLUDE_2) &&
			(boss.phase_state.patterns_seen < MAI_YUKI_PATTERNS_INTERLUDE_2)
		) {
			boss.mode = 0;
			boss.phase_frame = 0;
			boss.phase_state.patterns_seen = MAI_YUKI_PATTERNS_INTERLUDE_2;
		}
		frame_or_defeated = mai_yuki_hittest_shots();
		if(frame_or_defeated != 0) {
			boss.phase_state.defeat_bonus = true;
defeated:
			// 0 for Mai, 1 for Yuki. th05/main/boss/b4_pair_fg.cpp reads it
			// to decide which of the two to blit exploding.
			boss2.phase_state.patterns_seen = (frame_or_defeated - 1);
			boss.phase = PHASE_BOSS_EXPLODE_SMALL;
			boss.phase_frame = 0;
			if(lasers[0].flag != LF_FREE) {
				laser_stop(0);
				// The explosion has to start on THIS frame rather than on the
				// next one, so this jumps into the case below. It is a two-byte
				// jump to that arm's own three-byte jump to the switch end, not
				// a three-byte one of its own: see the label's comment for what
				// keeps it that way.
				goto explode_small_started;
			}
		}
		break;

	case PHASE_BOSS_EXPLODE_SMALL:
		// The `break` and the second `if` are not an `else if`, and neither
		// the optimization_barrier() below nor the position of the label in
		// front of it is decoration. [measured, 32 bytes]
		//
		// Without the barrier, Turbo threads the inner join through to the
		// switch end, which puts the near jump out of the `if` arm and the
		// short one out of the `else` arm -- the opposite of the original,
		// at the same total length.
		//
		// With the label BEHIND the barrier instead of in front of it, the
		// label resolves to the `break`'s own jump, and Turbo threads case
		// 2's `goto` straight through it to the switch end: a three-byte
		// near jump where the original has a two-byte short one into this
		// arm, which is one byte too many and kills the tail pad. In front
		// of the barrier the label is not a jump, so the `goto` stays short.
		if(boss.phase_frame == 16) {
			if(boss2.phase_state.patterns_seen == 0) {
				boss_explode_small(ET_VERTICAL);
			} else {
				boss2_explode_small(ET_VERTICAL);
			}
explode_small_started:
			optimization_barrier();
			break;
		}
		if(boss.phase_frame == 32) {
			// Whoever is left goes back to the shared still cel. Yuki's
			// patnums are Mai's with B4_CELS added inside the renderer, which
			// is why both branches spell the same number.
			if(boss2.phase_state.patterns_seen == 0) {
				boss_explode_big_circle();
				boss.sprite = PAT_ENEMY_KILL;
				yuki.sprite = PAT_B4_STILL;
			} else {
				boss2_explode_big_circle();
				yuki.sprite = PAT_ENEMY_KILL;
				boss.sprite = PAT_B4_STILL;
			}
			boss.phase++;
			bullet_zap.active = boss.phase_state.defeat_bonus;
			boss.phase_frame = 0;
			snd_se_play(12);
			player_invincibility_time = BOSS_DEFEAT_INVINCIBILITY_FRAMES;
		}
		break;

	// The same block boss_defeat_update() runs for every other boss
	// (th04/main/boss/boss.cpp), with the sprite advanced on whichever of the
	// two is the one exploding.
	case PHASE_BOSS_EXPLODE_BIG:
		if(boss.phase_frame < 12) {
			playfield_shake_x = ((stage_frame_mod2 == 0) ? -4 : +4);
			playfield_shake_y = ((stage_frame_mod4 <= 1) ? -4 : +4);
		}
		bg_render_bombing_func = tiles_render_all;
		slowdown_factor = 2;
		if((boss.phase_frame % 8) == 0) {
			if(boss2.phase_state.patterns_seen == 0) {
				boss.sprite++;
			} else {
				yuki.sprite++;
			}
			if(boss.phase_frame >= 64) {
				boss.phase++;
				boss.phase_frame = 0;
			}
		}
		break;

	case PHASE_NONE:
		palette_settone_deferred(60);
		if(boss.phase_frame == 1) {
			if(boss2.phase_state.patterns_seen == 0) {
				dialog_load(a_dm09_tx2);
				boss_bgm_title = aTH05_10;

				// Yuki survived, so she is the one that has to arrive in
				// [boss] for the second half. Mai's branch needs no such copy
				// because she already is [boss]; b4_solo_fg.cpp documents the
				// arrangement from the rendering side.
				boss.pos.cur = yuki.pos.cur;
				boss_update = yuki_update;
			} else {
				dialog_load(a_dm08_tx2);
				boss_bgm_title = aTH05_11;
				boss_update = mai_update;
			}
			dialog_animate();
			overlay1 = overlay_boss_bgm_update_and_render;
			boss.phase = PHASE_HP_FILL;
			boss.phase_frame = 0;
			boss_fg_render = b4_solo_fg_render;
			boss.hp = B4_SOLO_HP_TOTAL;
		}
		break;
	}
	cheetos_update();
	hud_hp_update_and_render((boss.hp + yuki.hp), MAI_YUKI_HP_TOTAL);
}

#pragma option -a1
