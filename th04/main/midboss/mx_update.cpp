/// Extra Stage midboss, update function
/// ------------------------------------
/// TWO bodies, like th04/main/midboss/mx.cpp's renderer, but they share
/// nothing: TH04's Extra midboss is a scripted fly-by in ENM_POS_TEXT, TH05's
/// is a two-phase danmaku fight in BX_UPDATE_TEXT. Only TH05's is decompiled
/// so far; TH04's keeps the `#else` branch free for the parcel that lifts it.

#if (GAME == 5)

// This translation unit is shared with th05/main/boss/bx.cpp (kb/codegen/0112),
// so every header below must be included exactly once across both files.
// th04/main/gather.hpp brings in th04/main/bullet/bullet.hpp, and
// th04/main/boss/boss.hpp brings in th04/main/phase.hpp; neither of those two
// has an include guard, so neither may be listed here as well.
// th02/hardware/pages.hpp has no include guard either, and is listed here on
// the measured ground that it is in neither file's #include closure otherwise:
// bx.cpp reaches 23 headers and this file 47, and pages.hpp is in neither set.
#include "th02/hardware/pages.hpp"
#include "th04/snd/snd.h"
#include "th04/main/pattern.hpp"
#include "th04/math/randring.hpp"
#include "th04/main/homing.hpp"
#include "th04/main/gather.hpp"
#include "th04/main/bullet/clearzap.hpp"
#include "th04/main/spark.hpp"
#include "th04/main/item/item.hpp"
#include "th04/main/midboss/midboss.hpp"
#include "th04/main/boss/boss.hpp"
#include "th04/main/hud/hud.hpp"
#include "th05/sprites/main_pat.h"

// Constants
// ---------

static const pixel_t MIDBOSSX_W = 64;
static const pixel_t MIDBOSSX_H = 64;
static const int HP_TOTAL = 3000;
// ---------

// Still ZUN's assembly in th05_main.asm's BX_UPDATE_TEXT, reached through the
// zero-byte `public` aliases in front of the dump's own labels
// (kb/codegen/0123). The dump's own labels keep their IDA spellings, which is
// what 0123 prescribes; the C++ side is named, because a `public` a parcel
// wrote itself is not a precedent that parcel may then defer to
// (NAMING_REVIEW_VERDICTS_9 section 7, applied by round 16 once this dump was
// free).
//
// Round 9 kept the phase-1 patterns at their placeholder spellings on the
// narrower ground that their bodies were still unexamined. Round 16 voided that
// for the one below: the paragraph above
// pattern_symmetric_turning_spread_stacks(), written by the very commit that
// claimed the licence, describes all three remaining bodies in the vocabulary
// the tree's own pattern names use -- so the examination had happened, and a
// search that examines the body does not fail. sub_1E5FC and sub_1E66F appear
// here only inside comments, never as identifiers, which is round 4's licensed
// way to record a dump's own spelling; naming them belongs to whichever parcel
// lifts them, and NAMING_REVIEW_VERDICTS_16 carries the ruling.
// -----

extern "C" {

// [frame] is relative to the start of the current phase.
//
// Sets [midboss.sprite] from [frame] (221 while [frame] < 0 or during the
// 30…39 window, 222/223 alternating during the 0…29 approach, 220 from 40 on),
// steps [midboss.pos] along [midboss.angle] at a [frame]-dependent speed for
// [frame]s 0…29, and plays se 8 on [frame] 0 and se 15 on [frame] 30. From
// [frame] 40 on it calls the current pattern and returns *its* result; every
// earlier path returns false.
//
// [inferred] name, from the behaviour above and this tree's own vocabulary.
//
// `flystep` is what this tree calls the movement half of a routine that steps
// a boss's or midboss's flight and reports whether that flight is done. The
// family is defined by that ROLE, not by a signature: its four unqualified
// members are boss_flystep_random, boss_flystep_towards,
// marisa_flystep_pointreflected and mai_yuki_flystep_random, all `bool pascal
// near`, but they agree only in the return. Only boss_flystep_random and
// mai_yuki_flystep_random take an `int frame`;
// marisa_flystep_pointreflected takes an `int duration`, and
// boss_flystep_towards takes two subpixel_t coordinates and no counter at all.
// (Naming round 16 stated this census as a four-member
// `bool pascal near f(int frame)` family; round 17 measured it and two of the
// four do not have that shape. The stem survives, the signature claim does
// not.) Eight further identifiers carry `flystep` as helpers or ticks.
//
// `_and_pattern` is the half none of the four has. th05/main/boss/b1.cpp's
// phase_2_3_wait_fly_and_select_pattern -- a `#define`, not a function -- is
// the same compound shape in the same role, one level up.
bool pascal near midbossx_flystep_and_pattern(int frame);

// The pattern that phase 1 starts from. Its address is taken by the dump
// and stored into [midbossx_flystep_and_pattern]'s callback.
//
// On [midboss.phase_frame] 94 it picks +1 or -1 with randring2_next16_and(1)
// and parks it in [boss_statebyte[13]]; on every frame up to 114 it fires one
// BG_RING of BSM_SPEEDUP blue crosses whose angle has drifted by that constant
// since the last, and it reports done at 128. `[inferred]` name, by the
// solved-twin rule: pattern_curved_rings() in th05/main/boss/b6.cpp is the same
// construct field for field -- BG_RING, blue, a randomly signed angle delta
// chosen with randring2_next16_and(1) and parked in a [boss_statebyte] slot --
// so `curved` is this tree's own token for that drift.
//
// `speedup` is BSM_SPEEDUP's own token, and it is NOT coined: cheeto_flag_t's
// CF_SPEEDUP (th05/main/bullet/cheeto.hpp) already spells the same word for the
// same physical behaviour in the same subsystem. One prior, not zero.
//
// What `speedup` distinguishes needs saying precisely, because the obvious
// reading is wrong. It is NOT "this one speeds up and the twin does not":
// pattern_curved_rings ramps bullet_template.speed by 0.5f every fourth frame,
// which is a rising per-RING speed, exactly what b1.cpp's
// pattern_accelerating_rings means by `accelerating`. The split is over WHICH
// speed changes -- the twin raises the template's speed once per ring, this one
// sets BSM_SPEEDUP, a per-BULLET special motion that the twin spells
// BSM_EXACT_LINEAR. So `accelerating` and `speedup` are not synonyms competing
// for one slot; they name different layers, and the tree does not yet mark the
// per-ring layer consistently. 28 characters, so nothing truncates.
// Defined below, once it was lifted; the declaration stays here because
// this `extern "C"` block is what gives it the `_`-prefixed symbol the
// dump's own pattern table references.
bool near pattern_curved_speedup_rings(void);

}

// The pattern [midbossx_flystep_and_pattern] calls once the approach is over.
// Initialised to sub_1E5FC in the dump's own data. Mirrors its table exactly as
// shinki_phase_2_3_pattern and sara_phase_2_3_pattern mirror theirs.
extern pattern_oneshot_func_t midbossx_phase_1_pattern;

// The pattern picked on every phase-1 cycle, indexed by [boss_statebyte[12]]
// (0 before the score bonus below, 1 after) and by the low bit of the cycle
// counter [boss_statebyte[14]]. Same shape as SHINKI_PATTERNS_PHASE_2_3 in
// th05/main/boss/b6.cpp — but note that the column index is spelled `% 2`
// rather than `& 1`, because [boss_statebyte] promotes to a *signed* int and
// only the remainder emits the original's `cwd`/`idiv` pair.
extern const pattern_oneshot_func_t MIDBOSSX_PATTERNS_PHASE_1[2][2];

// [midboss.angle] for each of the first 8 phase-1 cycles, indexed by
// [boss_statebyte[14]] & 7. Cycle 8 onwards would read 0 twice before the
// timeout condition ends the fight at cycle 20 — except that the fight is
// unwinnable that long, see below. Named for MIDBOSS3_FLY_ANGLES and
// YUUKA6_PHASE2_FLY_ANGLES, the tree's only other angle tables of this shape:
// every `*_ANGLES` token in the tree is a `*_FLY_ANGLES`, and a bare `_ANGLES`
// spelling has no members at all.
extern const unsigned char MIDBOSSX_FLY_ANGLES[8];
// -----

// The pattern phase 1 starts from, and the first of the four in address
// order. Its address is taken by [midbossx_phase_1_pattern]'s initialiser
// and by [MIDBOSSX_PATTERNS_PHASE_1], both still the dump's own data, so
// this may not be `static` and th05_main.asm's BX_UPDATE_TEXT block
// declares it as a `procdesc near`.
//
// On frame 94 it picks +1 or -1 and parks it in [boss_statebyte[13]]; on
// every frame up to 114 it fires one BG_RING of BSM_SPEEDUP blue crosses
// whose angle has drifted by that constant since the last, which is the
// `curved` in the name. The cycle ends at 128, like all four of them.
//
// The name and its whole justification were already written above, by the
// parcel that could not yet lift the body; this one only moves the body
// under it. Naming cost zero.
bool near pattern_curved_speedup_rings(void)
{
	if(midboss.phase_frame == 94) {
		// One store, so a conditional expression rather than two
		// assignments: the original computes the value in AL and writes
		// [boss_statebyte[13]] exactly once.
		boss_statebyte[13] = (randring2_next16_and(1) ? 1 : -1);
	}
	if(midboss.phase_frame <= 114) {
		bullet_template.spawn_type = BST_NO_DECELERATE;
		bullet_special.speed_delta.set(0.125f);
		bullet_template.special_motion = BSM_SPEEDUP;
		bullet_template.set_spread(18, 16);
		bullet_template.group = BG_RING;
		bullet_template.speed.set(0.5f);
		bullet_template.patnum = PAT_BULLET16_N_CROSS_BLUE;
		bullet_template.angle += boss_statebyte[13];
		bullets_add_special();
	}

	// Two separate returns, for the same reason the two patterns below
	// give.
	if(midboss.phase_frame >= 128) {
		return true;
	}
	return false;
}

// The second of the Extra Stage midboss's four phase-1 patterns, taken on the
// odd-numbered cycles before the half-HP score bonus:
// [MIDBOSSX_PATTERNS_PHASE_1] holds it at [0][1] and nowhere else. Its address
// is taken by that table, which is still the dump's own data, so this may not
// be `static`, and th05_main.asm's BX_UPDATE_TEXT block declares it as a
// `procdesc near`.
//
// On every frame with [page_back] set -- so on every other frame, since the
// shared stage loop in th04/main/stage/loop.cpp flips it once per frame -- it
// fires one BG_RANDOM_ANGLE_AND_SPEED group of 12 pellets at a base speed of
// 1.0, which th04/main/bullet/add.cpp then gives an independent
// randring2_next16() angle and a random speed bonus of 0.0 to 2.0 each. The
// cycle ends after 128 frames, like all four of them.
//
// [spread_angle_delta] is written along with [spread] by the single word store
// the original emits, and is dead for this group: add.cpp reads the delta half
// only on the BG_SPREAD families, never on the path this group takes. That is
// why the spelling is set_spread(), whose reinterpret_cast is what emits one
// word store rather than two byte stores.
//
// [inferred] name. Solved twin, field for field:
// pattern_dense_spreads_and_random_balls_within_laser_walls() in
// th05/main/boss/b1.cpp sets the same BG_RANDOM_ANGLE_AND_SPEED group, the
// same speed.set(1.0f), the same [spread]-as-count and the same
// bullets_add_regular(), and names that half `random_balls` after its
// PAT_BULLET16_N_BALL_BLUE [patnum]. This [patnum] is 0, which BulletTemplate
// documents as TH05's pellet, so the same construction yields
// `random_pellets`. The rate gate is deliberately left out of the name: no
// pattern_* identifier in either game encodes a firing interval, and
// pattern_symmetric_turning_spread_stacks() below fires on a `% 16` gate that
// its own name likewise omits. 22 characters, so nothing truncates.
bool near pattern_random_pellets(void)
{
	if(page_back) {
		bullet_template.spawn_type = (BST_CLOUD_FORWARDS | BST_NO_DECELERATE);
		bullet_template.group = BG_RANDOM_ANGLE_AND_SPEED;
		bullet_template.speed.set(1.0f);
		bullet_template.set_spread(12, 10);
		bullet_template.patnum = 0;
		bullets_add_regular();
	}

	// Two separate returns rather than one returning the comparison itself,
	// for the reason spelled out at the end of the function below.
	if(midboss.phase_frame >= 128) {
		return true;
	}
	return false;
}

// The last of the Extra Stage midboss's four phase-1 patterns, and the only
// one taken once the half-HP score bonus has been given:
// [MIDBOSSX_PATTERNS_PHASE_1] holds it in *both* columns of its second row, so
// it repeats for every remaining cycle regardless of the parity index. Its
// address is taken by that table, which is still the dump's own data, so this
// may not be `static`, and th05_main.asm's BX_UPDATE_TEXT block declares it as
// a `procdesc near`.
//
// Every 16th frame of the cycle it fires two BSM_DECELERATE_THEN_TURN spread
// stacks of blue cross bullets in mirrored directions (angle 0x80 and 0x00),
// each turning by 0x38 towards the other after the single deceleration that
// [bullet_special.turns_max] allows -- an exact mirror pair, since mapping
// angle to (0x80 - angle) and turn_by to -turn_by takes one onto the other.
// The cycle ends after 128 frames.
//
// Named rather than left at the IDA placeholder the dump carried for it,
// because the search does not fail (kb/conventions/naming-precedents.md
// section 3, and NAMING_REVIEW_VERDICTS_9 section 7, which holds that a
// placeholder may be kept only while the body has genuinely not been read).
// The other three patterns are each distinguishable from this one in the same
// terms the existing TH05 pattern names use -- [sub_1E5FC] fires nothing at
// all, [pattern_curved_speedup_rings] is a BG_RING of BSM_SPEEDUP crosses, and
// [pattern_random_pellets] is a BG_RANDOM_ANGLE_AND_SPEED pellet spray. Round
// 16 read that sentence as the record of a search that did NOT fail and named
// the second of them; the parcel that lifted the third named it above, so
// [sub_1E5FC] is now the only one left at its dump spelling, and it appears
// here inside comments only, never as an identifier. The `pattern_`
// prefix and the adjective-then-noun shape follow th05/main/boss/b1.cpp and
// th05/main/boss/b6.cpp, whose pattern functions are reached from dump tables
// through exactly this `dw offset @pattern_...$qv` route.
//
// `symmetric` is th05/main/boss/b6.cpp's own token for a mirrored volley
// (pattern_aimed_b6balls_and_symmetric_spreads) and `spread_stack` is
// th05/main/boss/b1.cpp's for this BG_ group (pattern_aimed_red_spread_stack,
// whose `_aimed` this one correctly lacks). Turbo C++ truncates the
// identifier to 32 characters, so the linker and the dump both spell it
// `@pattern_symmetric_turning_spread$qv`, the same way
// `pattern_pellet_arcs_at_expanding_random_angles` is spelled there.
bool near pattern_symmetric_turning_spread_stacks(void)
{
	if((midboss.phase_frame % 16) == 0) {
		bullet_template.spawn_type = (BST_CLOUD_BACKWARDS | BST_NO_DECELERATE);
		bullet_special.turns_max = 1;
		bullet_template.special_motion = BSM_DECELERATE_THEN_TURN;
		bullet_template.group = BG_SPREAD_STACK;
		bullet_template.set_spread_stack(5, 0x08, 4, 1.0f);
		bullet_template.speed.set(1.5f);
		bullet_template.patnum = PAT_BULLET16_N_CROSS_BLUE;

		bullet_template.angle = 0x80;
		bullet_template_special_angle.turn_by = -0x38;
		bullets_add_special();

		bullet_template.angle = 0x00;
		bullet_template_special_angle.turn_by = 0x38;
		bullets_add_special();

		snd_se_play(3);
	}

	// Two separate returns rather than one returning the comparison itself.
	// The comparison is an `int` expression, so the single-expression spelling
	// evaluates it into AX and converts on the way out
	// (`mov ax, 1` / `jmp` / `xor ax, ax`); the original computes the 1-byte
	// [bool] directly in AL and duplicates the epilogue, which is what a pair
	// of constant returns emits. The other three patterns in this table end
	// the same way.
	if(midboss.phase_frame >= 128) {
		return true;
	}
	return false;
}

// [boss_statebyte] slots used here. Not #defined to names: [12] is both the
// "score bonus already given" latch and the pattern table's row index, [14] is
// both the angle table's index and the cycle counter the timeout condition
// reads, and [15] is written (0 in phase 0, 1 on every phase-1 cycle) but
// never read in this function or anywhere else in BX_UPDATE_TEXT.

void pascal midbossx_update(void)
{
	bullet_template.origin = midboss.pos.cur;
	gather_template.center = midboss.pos.cur;

	midboss.phase_frame++;

	switch(midboss.phase) {
	case 0:
		midboss_hittest_shots_invincible(
			TO_SP((MIDBOSSX_W / 2) - (MIDBOSSX_W / 8)),
			TO_SP((MIDBOSSX_H / 2) - (MIDBOSSX_H / 8))
		);
		midboss.angle = 0x40;
		if(midbossx_flystep_and_pattern(midboss.phase_frame)) {
			midboss.phase++;
			midboss.phase_frame = 0;
			midboss.angle = 0x00;
			boss_statebyte[15] = 0;
			boss_statebyte[14] = 0;
			boss_statebyte[12] = 0;
			midbossx_phase_1_pattern = pattern_curved_speedup_rings;
		}
		break;

	case 1:
		if(midbossx_flystep_and_pattern(midboss.phase_frame - 64)) {
			if((boss_statebyte[12] == 0) && (midboss.hp < 1000)) {
				midboss_score_bonus(10);
				bullets_clear();
				snd_se_play(15);
				boss_statebyte[12]++;
			}
			boss_statebyte[15] = 1;
			midboss.phase_frame = 0;
			midboss.angle = MIDBOSSX_FLY_ANGLES[boss_statebyte[14] & 7];
			boss_statebyte[14]++;
			midbossx_phase_1_pattern =
				MIDBOSSX_PATTERNS_PHASE_1[boss_statebyte[12]][boss_statebyte[14] % 2];
		}
		midboss_hittest_shots(
			TO_SP((MIDBOSSX_W / 2) - (MIDBOSSX_W / 8)),
			TO_SP((MIDBOSSX_H / 2) - (MIDBOSSX_H / 8))
		);

		// Timeout condition
		if(boss_statebyte[14] < 20) {
			if(midboss.hp > 0) {
				break;
			}
			bullet_zap.active = true;
			midboss_score_bonus(30);
			items_add(midboss.pos.cur.x, midboss.pos.cur.y, IT_1UP);
		}
		midboss.phase = PHASE_EXPLODE_BIG;
		midboss.sprite = PAT_ENEMY_KILL;
		midboss.phase_frame = 0;
		sparks_add_circle(
			midboss.pos.cur.x, midboss.pos.cur.y, TO_SP(MIDBOSSX_W / 8), 48
		);
		snd_se_play(12);
		break;

	default:
		midboss_defeat_update();
		hud_hp_update_and_render(midboss.hp, HP_TOTAL);
		return;
	}

	hud_hp_update_and_render(midboss.hp, HP_TOTAL);
	homing_target.x = midboss.pos.cur.x;
	homing_target.y = midboss.pos.cur.y;
}

#endif
