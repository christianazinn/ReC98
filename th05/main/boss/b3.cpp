/// Stage 3 Boss - Alice
/// --------------------
/// Compiled into th05/boss_4.cpp, *ahead* of th05/main/boss/b4_both.cpp.
/// alice_update() is the last emitting proc of th05_main.asm's contribution to
/// B4_UPDATE_TEXT -- the `dw offset loc_...` run below it is the jump table its
/// own `switch` compiles to, not data the dump owns -- so the C++ object that
/// already follows the root inside that segment simply grows backwards into the
/// hole (kb/codegen 0099 + 0112 + 0114 + 0129, state/re/JUMP_TABLE_TAILS.md).
///
/// The segment is called B4_UPDATE_TEXT because it is one module of ZUN's, and
/// that module holds the Stage 2 and 3 midbosses, Louise, Alice's puppets and
/// Alice as well as the Stage 4 pair -- of which only the pair had been
/// decompiled before this file existed.
///
/// The `-zCB4_UPDATE_TEXT -zPmain_03` pragma lives in th05/boss_4.cpp, which
/// compiles this file ahead of b4_both.cpp: only the first file compiled into
/// an object may name its segment (kb/codegen/0112 trap 0).

// For iatan2(). Guarded, unlike the rest of this block, so this is a hoist and
// not a move; the OBJ was read before and after to confirm it changed no
// codegen below it.
#include "libs/master.lib/master.hpp"
// Also supplies th04/math/randring.hpp and th05/sprites/main_pat.h, neither of
// which has an include guard; b4_both.cpp reaches both through this same
// header, so naming either of them here would be a compile error.
#include "th04/main/player/shot.hpp"
#include "th04/snd/snd.h"
#include "th04/main/bg.hpp"
#include "th04/main/gather.hpp"
#include "th04/main/homing.hpp"
#include "th04/main/pattern.hpp"
#include "th04/main/hud/hud.hpp"
#include "th04/main/bullet/clearzap.hpp"
// Supplies th05/main/boss/boss.hpp, which b4_both.cpp used to include itself.
// Unguarded, and this file compiles ahead of that one in the same object, so
// the include moved to the earliest file that needs it (kb/codegen 0129).
#include "th05/main/boss/bosses.hpp"
// Unguarded, like every header here, and reached from nowhere else in this
// object -- th05/main/boss/b4_both.cpp fires no lasers. It carries two
// balanced `#pragma codeseg` pairs, each closed by a bare `#pragma codeseg`
// that restores th05/boss_4.cpp's `-zC` default, so nothing below it moves
// segment; b1.cpp and b6.cpp include it the same way.
#include "th05/main/bullet/laser.hpp"
#include "th05/main/boss/b3puppet.hpp"

// ZUN's remaining assembly in this segment
// ----------------------------------------
// Every symbol below keeps IDA's address-derived spelling, and each one is
// linkable through a zero-byte alias in th05_main.asm (kb/codegen 0123).
//
// THE SEARCH THAT FAILED, run once for the whole group: none of these is
// published under any name by any of the five games (`git grep` over ReC98 for
// each address and for every candidate spelling), none has a TH04 counterpart
// -- TH04's Stage 3 boss is Marisa and shares no code with Alice -- and
// `state/notes/` holds no prior naming ruling for the Alice module. Naming them
// needs the bodies themselves classified, which is a naming round rather than
// a lift; this parcel only makes them referenceable. What is measured about
// each one is recorded beside it, and nothing here is a claim about intent.
//
// [measured] `void near f(void)`: `retn`, no parameter equates, no return
// register live at any call site.
extern "C" void near sub_19B04(void);  // Alice's own gather + cloud attack
extern "C" void near sub_19634(void);  // steps both puppets along a shrinking spiral
extern "C" void near sub_19CB0(void);  // the barrier phase, driven by [word_2CE2E]

// [measured] `bool pascal near f(puppet_t near *)`: one word parameter equate,
// `retn 2`, and the result is read out of AL by puppets_update(), which
// respawns the puppet and reseeds its callback whenever one returns true.
typedef bool (pascal near *near puppet_func_t)(puppet_t near *puppet);

extern "C" bool pascal near sub_198B7(puppet_t near *puppet);
extern "C" bool pascal near sub_19928(puppet_t near *puppet);
extern "C" bool pascal near sub_19A84(puppet_t near *puppet);
extern "C" bool pascal near sub_19AFB(puppet_t near *puppet);

// [measured] puppets_update() calls [fp_2CE2A] for puppet 0 and [fp_2CE2C] for
// puppet 1, selecting between them with an `if` on the loop index rather than
// by indexing, so these are two variables and not a two-element array.
extern "C" puppet_func_t fp_2CE2A;
extern "C" puppet_func_t fp_2CE2C;

// [measured] Both are read by sub_19CB0() only. [word_2CE2E] counts frames
// once both puppets have come to rest; [word_2CE30] widens the window inside
// which the barrier phase adds bullets.
extern "C" int word_2CE2E;
extern "C" int word_2CE30;

// [measured] The structural twin of [sara_phase_2_3_pattern] and
// SARA_PATTERNS_PHASE_2_3 in b1.cpp: a currently selected pattern function,
// and the table it is picked from. 4 rows of 3, indexed by [boss_statebyte[7]]
// -- which counts pattern phases survived -- and a random number below 3.
extern "C" pattern_loop_func_t fp_2CE32;
extern "C" const pattern_loop_func_t off_22770[4][3];
// ----------------------------------------

// Constants
// ---------

// [inferred] Alice's own sprites start at the same PAT_STAGE base every stage's
// sprites do; this is the cel the puppets show while a pattern runs. Kept
// file-local rather than added to th05/sprites/main_pat.h, which has no Alice
// block yet and is shared with every other TU in this binary.
static const int PAT_PUPPET = (PAT_STAGE + 10);

// [measured] Alice's own two cels, from the same base, spelled after
// PAT_SHINKI_STILL / PAT_SHINKI_CAST -- the attested formula for exactly this
// pair one boss over. Kept file-local for the same reason PAT_PUPPET is.
static const int PAT_ALICE_STILL = (PAT_STAGE + 0);
static const int PAT_ALICE_CAST = (PAT_STAGE + 4);

// The two gather circle colors every TH05 boss charges with; b1.cpp spells the
// same two values under the same two names.
enum alice_col_t {
	COL_GATHER_1 = 9,
	COL_GATHER_2 = 8,
};

enum alice_hp_t {
	HP_TOTAL = 9600,

	// Where the first pattern phase ends. Every later one ends 2200 lower,
	// until phase 12 leaves nothing.
	HP_PHASE_2_END = 7400,
	HP_PER_PHASE = 2200,
};

enum alice_phase_t {
	// The first of the four (pattern, rest, barrier) triples. Phases 3, 6, 9
	// and 12 run a pattern, 4, 7 and 10 fly back to the top, 5, 8 and 11 run
	// the barrier.
	PHASE_PATTERN_1 = 3,

	// The phase the last triple would have started at. Reaching it means
	// there is no HP left to hand to the next one.
	PHASE_AFTER_LAST_PATTERN = 12,
};

// The amount of patterns Alice survives before a phase ends on its own.
static const int PATTERNS_PER_PHASE = 24;
// ---------

// Six of the twelve entries of [off_22770], Alice's danmaku pattern table,
// across the three functions below. The table itself and its other three
// distinct bodies are still in th05_main.asm's _DATA and B4_UPDATE_TEXT
// contributions, so these three keep an address-suffixed hand name rather than
// a descriptive one: the family's stem is settled (`alice_…_pattern`) but
// which of the six bodies gets which descriptive name is one decision over the
// whole table, and it is only payable once every body has landed.
// state/re/NAMING_REVIEW_VERDICTS_19.md §10.1 measured the address-suffixed
// form against naming_precheck's own PLACEHOLDER_RE and it is not a
// placeholder. The table reaches all three through procdesc lines in
// B4_UPDATE_TEXT.

// Charges a 3-stack gather for 64 frames, then, on frame 64 alone, fires three
// shoot-out lasers -- one aimed, one 16 units clockwise, one 16 counter-
// clockwise -- together with a single random-angle blue ball spread.
void near alice_pattern_19E12(void)
{
	if(boss.phase_frame < 64) {
		gather_add_only_3stack(
			(boss.phase_frame - 40), COL_GATHER_1, COL_GATHER_2
		);
		boss.sprite = PAT_ALICE_CAST;
		if(boss.phase_frame == 40) {
			snd_se_play(8);
		}
		return;
	}
	if(boss.phase_frame == 64) {
		laser_template.col = 6;
		laser_template.coords.width.nonshrink = 8;
		laser_template.coords.origin = bullet_template.origin;

		// The dump spells this field `grow_at_age`, but `grow_at_age` and
		// `moveout_at_age` are two labels on the same word of `laser_t`
		// (th05/main/bullet/lasers[bss].asm), and it is the shoot-out reading
		// that applies: lasers_shootout_add() is what consumes this template.
		laser_template.active_at_age.moveout = 40;

		laser_template.shootout_speed.set(5.0f);
		laser_template.coords.angle = iatan2(
			(player_pos.cur.y.v - laser_template.coords.origin.y.v),
			(player_pos.cur.x.v - laser_template.coords.origin.x.v)
		);
		lasers_shootout_add();
		laser_template.coords.angle += 0x10;
		lasers_shootout_add();
		laser_template.coords.angle -= 0x20;
		lasers_shootout_add();

		bullet_template.spawn_type = (BST_CLOUD_FORWARDS | BST_NO_DECELERATE);
		bullet_template.group = BG_RANDOM_ANGLE_AND_SPEED;
		bullet_template.patnum = PAT_BULLET16_N_BALL_BLUE;
		bullet_template.set_spread(20, 8);
		bullet_template.speed.set(1.5f);
		bullet_template.angle = 0x00;
		bullet_template_tune();
		bullets_add_regular();
		boss.sprite = PAT_ALICE_STILL;
		return;
	}
	if(boss.phase_frame == 96) {
		boss.phase_frame = 0;
		boss.mode = 0;
	}
}

// Charges a 3-stack gather for 64 frames, then fires a mirrored pair of aimed
// blue ball stacks every 4th frame, walking the pair's angle 5 units per shot.
void near alice_pattern_19EDA(void)
{
	if(boss.phase_frame < 64) {
		gather_add_only_3stack(
			(boss.phase_frame - 40), COL_GATHER_1, COL_GATHER_2
		);
		boss.sprite = PAT_ALICE_CAST;
		boss_statebyte[15] = 8;
		if(boss.phase_frame == 40) {
			snd_se_play(8);
		}
		return;
	}
	if((boss.phase_frame % 4) == 0) {
		bullet_template.spawn_type = (BST_CLOUD_FORWARDS | BST_NO_DECELERATE);
		bullet_template.group = BG_STACK_AIMED;
		bullet_template.patnum = PAT_BULLET16_N_BALL_BLUE;
		bullet_template.set_stack(8, 0.4375f);
		bullet_template.speed.set(1.5f);
		bullet_template_tune();
		bullet_template.angle = boss_statebyte[15];
		bullets_add_regular();
		bullet_template.angle = -bullet_template.angle;
		bullets_add_regular();
		boss_statebyte[15] += 5;
		boss.sprite = PAT_ALICE_STILL;
		snd_se_play(15);
	}
	if(boss.phase_frame == 96) {
		boss.phase_frame = 0;
		boss.mode = 0;
	}
}

// The same charge, then a red ball ring every 4th frame, rotated 2 units per
// ring.
void near alice_pattern_19F75(void)
{
	if(boss.phase_frame < 64) {
		gather_add_only_3stack(
			(boss.phase_frame - 40), COL_GATHER_1, COL_GATHER_2
		);
		boss.sprite = PAT_ALICE_CAST;
		boss_statebyte[15] = 0;
		if(boss.phase_frame == 40) {
			snd_se_play(8);
		}
		return;
	}
	if((boss.phase_frame % 4) == 0) {
		bullet_template.spawn_type = (BST_CLOUD_FORWARDS | BST_NO_DECELERATE);
		bullet_template.group = BG_RING;
		bullet_template.patnum = PAT_BULLET16_N_BALL_RED;
		bullet_template.set_spread(20, 7);
		bullet_template.speed.set(2.5f);
		bullet_template_tune();
		bullet_template.angle = boss_statebyte[15];
		bullets_add_regular();
		boss_statebyte[15] += 2;
		boss.sprite = PAT_ALICE_STILL;
		snd_se_play(3);
	}
	if(boss.phase_frame == 96) {
		boss.phase_frame = 0;
		boss.mode = 0;
	}
}

// Phase 14 only, and called directly rather than through [off_22770]: a
// straight-down spread of red arrowheads, every 32nd frame. The template
// writes ahead of the frame test are ZUN bloat -- they are re-done on every
// frame and only read on every 32nd.
void near pattern_red_spreads(void)
{
	bullet_template.spawn_type = BST_NO_DECELERATE;
	bullet_template.group = BG_SPREAD;
	bullet_template.angle = 0x40;
	bullet_template.patnum = PAT_BULLET16_V_RED;
	if((boss.phase_frame % 32) == 0) {
		bullet_template.speed.set(2.0f);
		bullet_template.set_spread(13, 10);
		bullet_template_tune();
		bullets_add_regular();
		snd_se_play(3);
	}
}

#pragma option -a2

void pascal alice_update(void)
{
	homing_target.x = boss.pos.cur.x;
	homing_target.y = boss.pos.cur.y;
	boss.phase_frame++;
	bullet_template.spawn_type = BST_NORMAL; // ZUN bloat
	bullet_template.origin.x.v = boss.pos.cur.x.v;
	bullet_template.origin.y.v = (boss.pos.cur.y.v + to_sp(-8.0f));
	gather_template.center = bullet_template.origin;

	switch(boss.phase) {
	case PHASE_HP_FILL:
		if(boss.phase_frame == 1) {
			boss.hp = HP_TOTAL;
			boss.phase_end_hp = HP_PHASE_2_END;
			gather_template.radius.set(BOSS_W / 1.0f);
			gather_template.angle_delta = 0x02;
			gather_template.ring_points = 8;
		}
		boss_hittest_shots_invincible();

		// Timeout condition
		if(boss.phase_frame >= 128) {
			// Next phase
			boss.phase++;
			boss.phase_frame = 0;
			snd_se_play(13);
			bg_render_bombing_func = alice_bg_render;
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
			boss_statebyte[10] = 0x18;
			boss_statebyte[9] = 0;
		}
		break;

	case 2: {
		puppet_t near *puppet;

		sub_19B04();
		boss_hittest_shots();
		if(boss.phase_frame == 128) {
			puppet = puppets;
			puppet->flag = F_ALIVE;
			puppet->patnum = PAT_PUPPET;
			puppet->radius.motion = to_sp(256.0f);
			puppet->angle = 0x60;
			puppet->hp = PUPPET_HP;
			puppet->pos.cur.x.v = Subpixel::None();
			puppet++;
			puppet->flag = F_ALIVE;
			puppet->patnum = PAT_PUPPET;
			puppet->radius.motion = to_sp(256.0f);
			puppet->angle = 0x20;
			puppet->hp = PUPPET_HP;
			puppet->pos.cur.x.v = Subpixel::None();
			fp_2CE2A = sub_198B7;
			fp_2CE2C = sub_198B7;
		} else if(boss.phase_frame > 128) {
			sub_19634();

			// Timeout condition
			if(puppets[0].radius.motion == 0) {
				// Next phase
				boss.phase++;
				boss.mode = 0;
				boss.phase_frame = 0;
				boss.phase_state.patterns_seen = 0;
				boss_statebyte[7] = 0;
				word_2CE30 = 160;
			}
		}
		break;
	}

	case PHASE_PATTERN_1:
	case (PHASE_PATTERN_1 + 3):
	case (PHASE_PATTERN_1 + 6):
	case (PHASE_PATTERN_1 + 9):
		switch(boss.mode) {
		case 0:
			if(boss_flystep_random(boss.phase_frame - 8)) {
				boss.mode = 1;
				boss.phase_frame = 0;
				boss.phase_state.patterns_seen++;

				// Timeout condition
				if(boss.phase_state.patterns_seen >= PATTERNS_PER_PHASE) {
					goto phase_timed_out;
				}
				fp_2CE32 = off_22770
					[boss_statebyte[7]][randring2_next16_mod(3)];
			}
			break;

		case 1:
			fp_2CE32();
			break;
		}
		if(!boss_hittest_shots()) {
			break;
		}

		// Next phase
		boss_score_bonus(7);
		boss_items_drop();
	phase_timed_out:
		boss_statebyte[7]++;
		bullets_clear();
		boss_explode_small(ET_NW_SE);
		boss.phase++;
		boss.phase_frame = 0;
		boss.phase_state.patterns_seen = 0;
		boss.mode = 0;
		boss.hp = boss.phase_end_hp;
		if(boss.phase < PHASE_AFTER_LAST_PATTERN) {
			boss.phase_end_hp -= HP_PER_PHASE;
		} else {
			boss.phase_end_hp = 0;
		}
		word_2CE2E = 0;
		break;

	case (PHASE_PATTERN_1 + 1):
	case (PHASE_PATTERN_1 + 4):
	case (PHASE_PATTERN_1 + 7):
		puppets[0].pos.prev.x.v = to_sp(128.0f);
		puppets[0].pos.prev.y.v = to_sp(128.0f);
		puppets[1].pos.prev.x.v = to_sp(256.0f);
		puppets[1].pos.prev.y.v = to_sp(128.0f);
		puppets[0].hp = PUPPET_HP;
		puppets[1].hp = PUPPET_HP;
		fp_2CE2A = fp_2CE2C = sub_19AFB;
		// Both this phase and phase 13 fly back to the same point and end the
		// same way, and the original shares that ending: the branch below
		// jumps into phase 13's copy rather than carrying one of its own.
		// [measured] An ordinary `if` here does NOT reach that shape -- it
		// compiles to a second inline copy, ten bytes the original does not
		// have. `-O` does cross-jump identical suffixes elsewhere (TH05
		// MAIN's item_collected() is a landed case), so this is a fact about
		// this shape and not a rule about the optimizer: measure, do not
		// predict. A `goto` written where the optimizer needed none costs two
		// instructions of its own somewhere else.
		if(boss_flystep_towards(to_sp(PLAYFIELD_W / 2), to_sp(64.0f))) {
			goto flystep_next_phase;
		}
		goto flystep_hittest;

	case (PHASE_PATTERN_1 + 2):
	case (PHASE_PATTERN_1 + 5):
	case (PHASE_PATTERN_1 + 8):
		puppets[0].pos.prev.x.v = to_sp(128.0f);
		puppets[0].pos.prev.y.v = to_sp(128.0f);
		puppets[1].pos.prev.x.v = to_sp(256.0f);
		puppets[1].pos.prev.y.v = to_sp(128.0f);
		puppets[0].hp = PUPPET_HP;
		puppets[1].hp = PUPPET_HP;
		fp_2CE2A = fp_2CE2C = sub_19AFB;
		sub_19CB0();
		boss_hittest_shots();

		// Timeout condition
		if(boss.phase_frame >= 600) {
			// Next phase
			boss_score_bonus(5);
			boss_explode_small(ET_NW_SE);
			bullets_clear();
			boss.phase++;
			boss.phase_frame = 0;
			boss_statebyte[9] = 0;
			puppets[0].pos.prev.x.v = to_sp(320.0f);
			puppets[0].pos.prev.y.v = to_sp(96.0f);
			puppets[1].pos.prev.x.v = to_sp(64.0f);
			puppets[1].pos.prev.y.v = to_sp(96.0f);
			puppets[0].phase_frame = puppets[1].phase_frame = 0;
			fp_2CE2A = sub_19928;
			fp_2CE2C = sub_19928;
		}
		break;

	case 13:
		puppets[0].pos.prev.x.v = to_sp(64.0f);
		puppets[0].pos.prev.y.v = to_sp(128.0f);
		puppets[1].pos.prev.x.v = to_sp(320.0f);
		puppets[1].pos.prev.y.v = to_sp(128.0f);
		puppets[0].hp = PUPPET_HP;
		puppets[1].hp = PUPPET_HP;
		fp_2CE2A = fp_2CE2C = sub_19A84;
		if(!boss_flystep_towards(to_sp(PLAYFIELD_W / 2), to_sp(64.0f))) {
			goto flystep_hittest;
		}
	flystep_next_phase:
		// Next phase
		boss.phase_frame = 0;
		boss.phase++;
	flystep_hittest:
		boss_hittest_shots();
		break;

	case 14:
		puppets[0].pos.prev.x.v = to_sp(64.0f);
		puppets[0].pos.prev.y.v = to_sp(128.0f);
		puppets[1].pos.prev.x.v = to_sp(320.0f);
		puppets[1].pos.prev.y.v = to_sp(128.0f);
		fp_2CE2A = fp_2CE2C = sub_19A84;
		pattern_red_spreads();

		// Timeout condition
		if(boss.phase_frame < 1000) {
			// Boss defeated?
			if(!boss_hittest_shots()) {
				break;
			}
			boss.phase_state.defeat_bonus = true;
		}

		// Boss defeated
		boss.phase_frame = 0;
		boss.phase = PHASE_BOSS_EXPLODE_SMALL;
		puppets[0].flag = F_REMOVE;
		puppets[1].flag = F_REMOVE;
		break;

	default:
		boss_defeat_update(10);
		break;
	}
	hud_hp_update_and_render(boss.hp, HP_TOTAL);
	if(
		(boss.phase >= PHASE_PATTERN_1) &&
		(boss.phase < PHASE_BOSS_EXPLODE_BIG)
	) {
		puppets_update();
	}
}

// `-a2` is left in force past this point rather than restored, and that is a
// non-change, not a requirement. It was originally written that way on
// kb/codegen/0159's claim that an option pragma between the table and the next
// function discards the pad; that model has since been refuted, twice and
// independently (`state/notes/th05-main-item-collected.md` measures the
// two-sided control). The pad depends on `-a2` and on the parity of the table's
// object-relative offset, and on nothing else. What keeps the restore out is
// simply that the GREEN build does not have one, and that nothing following in
// this object -- th05/main/boss/b4_both.cpp -- defines a structure or emits
// data, so `-a2` staying on changes nothing there either way.
