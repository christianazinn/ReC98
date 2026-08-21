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
extern "C" void near sub_1A005(void);  // the final phase's red spread

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
		sub_1A005();

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
