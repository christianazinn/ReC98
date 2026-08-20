// Stage 4 boss state
// ------------------

#include "pc98.h"

/// Marisa herself
/// --------------

// Top-left corner of the sprite, in screen space. Every blit in Marisa's
// still-ASM code takes .x and .y directly.
//
// `[measured]` The same storage is *also* the stage-4 midboss's top-left:
// midboss4_19F52() and midboss4_19FAF() blit from it too, and derive their own
// [boss_pos_x] / [boss_pos_y] from it with +24 rather than the +40 below. The
// `marisa_` prefix names the half this header covers, not exclusive ownership.
extern screen_point_t marisa_topleft;

// The per-frame movement delta marisa_1BE72() adds to Marisa's position on
// every frame of the first 50 of a pattern. Only ever -1, 0 or 1; picked once,
// on frame 2, and then held for the rest of the drift.
//
// `[measured]` Both slots are `dw` and both are read back with a signed
// compare, and nothing outside marisa_1BE72() touched either of them in
// th02_main.asm, so this pair is the whole of Marisa's movement state.
extern pixel_t marisa_velocity_x;
extern pixel_t marisa_velocity_y;

// `[measured]` from marisa_1AE98()'s hand-written flash blit, which walks
// super_patdata[] linearly at 6 VRAM bytes (48 pixels) per row for 0x60 rows,
// then repeats the same pattern at x + 48. So each of the two patterns is
// 48x96 and the pair Marisa is drawn as is 96x96.
static const pixel_t MARISA_W = 96;
static const pixel_t MARISA_H = 96;

// What marisa_update() adds to both axes of [marisa_topleft] to reach the
// point shottype B's homing shots aim at. Deliberately NOT (MARISA_W / 2):
// `[measured]` marisa_1AC7B() passes the same +40 pair to
// boss_explode_render(), whose parameters are named [center_x] / [center_y],
// while marisa_1AA60()'s hitbox is centered on +48 instead, and
// marisa_1BE72()'s drift direction turns on +32. ZUN's four notions of
// Marisa's center do not agree, so this one gets its own name.
static const pixel_t MARISA_CENTER_OFFSET = 40;

/// --------------

/// Orbs
/// ----
/// The four orbs Marisa summons around herself, spins outwards at a growing
/// radius, and connects with lines into a rotating ring. Each one is shot down
/// separately, and drops an item when it is.

static const int MARISA_ORB_COUNT = 4;

enum marisa_orb_flag_t {
	MOF_ALIVE = 0,
	MOF_KILL_ANIM = 1,
	MOF_REMOVED = 2,

	_marisa_orb_flag_t_FORCE_INT16 = 0x7FFF,
};

extern marisa_orb_flag_t marisa_orb_flag[MARISA_ORB_COUNT];

// Same page-indexed indirection as used for the boss itself (boss.hpp), except
// that the cached pointers are far ones, and that the two arrays *are*
// contiguous in memory. Two of the remaining per-orb arrays are still ASM-only:
// [marisa_orb_damage] (raised by 1 per frame in which any player shot overlaps
// the orb, and the orb is removed at 110) and [marisa_orb_hit_flash] (set on
// the same frame, and cleared by the renderer after one white blit).
extern screen_x_t marisa_orb_left_on_page[PAGE_COUNT][MARISA_ORB_COUNT];
extern screen_y_t marisa_orb_top_on_page[PAGE_COUNT][MARISA_ORB_COUNT];
extern screen_x_t* marisa_orb_left_on_back_page[MARISA_ORB_COUNT];
extern screen_y_t* marisa_orb_top_on_back_page[MARISA_ORB_COUNT];

// Together with [marisa_orb_angle] below, the polar placement of each orb
// relative to Marisa's own center. marisa_1B477() spawns the orbs at radius 8
// and grows it by 2 per frame; three of the patterns then breathe the ring in
// and out by walking this array directly.
//
// `[measured]` state/notes/marisa_bg_render.md already recorded the name and
// the evidence - marisa_1B3DE() places each orb at
// `(radius * _CosTable8[angle]) >> 8` from that center - and th02_main.asm
// already carried the kb/codegen/0123 alias unpublished. This parcel only adds
// the `public`.
extern int marisa_orb_radius[MARISA_ORB_COUNT];

// The angle half of that pair, as a full-circle 0-255 byte. marisa_1B477()
// spawns orb `i` at `(i << 6)`, i.e. the four cardinal directions, and
// marisa_1B3DE() advances each one by [marisa_orb_angle_delta] every frame.
// Three patterns also fire *along* this angle, so it doubles as each orb's
// muzzle direction.
extern uint8_t marisa_orb_angle[MARISA_ORB_COUNT];

// How much [marisa_orb_angle] advances per frame, per orb. Usually set in ±
// pairs - orbs 0 and 2 one way, 1 and 3 the other - so the ring
// counter-rotates against itself. marisa_1BC43() widens the pairs from ±2 to
// ±4 over the first 30 frames of its pattern, marisa_1BAFF() flips the sign of
// all four at once, marisa_1B996() drives all four the *same* way, and
// marisa_1B7D3() sets the one pair that is not symmetric.
//
// `[measured]` The dump still spells this array as four separate `word_26D42`
// through `word_26D48` definitions, because eight sites in main_03__TEXT
// address the elements by their own names; every indexed access
// (`word_26D42[bx]`, bx = orb * 2) proves it is one four-element array, and
// kb/codegen/0123's alias is what gives it this name.
extern int marisa_orb_angle_delta[MARISA_ORB_COUNT];

// Recomputed at the top of every marisa_update() as the plain sum of all four
// [marisa_orb_flag] values, and only ever compared against
// (MARISA_ORB_COUNT * MOF_REMOVED) - i.e. it is a "have all four orbs gone"
// test that ZUN spelled as an arithmetic one. Marisa has no hittable center
// while any orb is left.
extern int marisa_orb_flag_sum;

// [marisa_pattern] case 3's own two slots, cleared together on frame 50 of
// every run of it and touched by nothing else in th02_main.asm.
//
// [marisa_orb_volley_angle] is the offset that case 3 adds to each orb's
// [marisa_orb_angle] to aim that orb's shot, and it grows by twice that orb's
// [marisa_orb_angle_delta] on every volley - so each stream sweeps away from
// the ring at double the ring's own rate. [marisa_volleys_fired] counts the
// volleys, and is only ever read for its low bit, which halves the rate of the
// ring Marisa fires herself once the orbs are gone.
extern uint8_t marisa_volleys_fired;
extern uint8_t marisa_orb_volley_angle[MARISA_ORB_COUNT];
/// ----

/// Fight progression
/// -----------------

// Which of the three stages of the fight Marisa is in. Not [boss_phase],
// which stays 0 for all three of these and only turns 1 once she is defeated.
//
// `[measured]` Reused as its own 0/1 state by the still-ASM stage-4 midboss
// (@midboss4_update_and_render$qv), the same way [marisa_topleft] is. Every
// access in either is an equality test, so nothing in the dump says whether
// the slot is signed.
extern int marisa_intro_step;

// The pattern marisa_update() dispatches on, as a *signed* char: 1 to 6 are
// the regular patterns, -1 to -3 the ones she uses once all four orbs are
// gone, and MP_UNSTARTED the gap between two patterns during which the next
// one is picked.
extern int8_t marisa_pattern;

static const int8_t MP_UNSTARTED = 0x7F;

// Which way pattern 4's star rain leans: +1 or -1, flipped on frame 50 of
// every run of it, and initialized to +1 in the dump's own `_DATA`.
//
// `[measured]` marisa_1BC43() held every reference to this slot in
// th02_main.asm, and it decides three things there at once - the sign of
// [bullet_special]'s drift angle, which of the playfield's two sides the paired
// stars spawn on, and the ±22.5° the whole rain is angled by. Nothing resets
// it between rounds, so the lean alternates across the entire fight.
extern int8_t marisa_pattern_side;

// Frames MP_UNSTARTED waits before picking the next pattern.
static const int MARISA_PATTERN_GAP_FRAMES = 30;

// Regular patterns fired since the last reset; at
// MARISA_PATTERNS_PER_ROUND, pattern 6 is forced instead of a random one.
extern uint8_t marisa_patterns_seen;
static const uint8_t MARISA_PATTERNS_PER_ROUND = 4;

// The same counter for the all-orbs-gone patterns, which run two at a time
// before the round ends.
extern int8_t marisa_orbless_patterns_seen;

// Completed rounds. Marisa is defeated at MARISA_ROUNDS.
extern uint8_t marisa_rounds_done;
static const uint8_t MARISA_ROUNDS = 7;
/// -----------------

/// Background
/// ----------

// Color ramp that [bg_particle_col] is cycled through, one step every
// MARISA_BG_PARTICLE_COL_FRAMES frames.
static const int MARISA_BG_PARTICLE_COLS_COUNT = 6;
static const int MARISA_BG_PARTICLE_COL_FRAMES = 32;
extern const vc_t MARISA_BG_PARTICLE_COLS[MARISA_BG_PARTICLE_COLS_COUNT];
extern int8_t marisa_bg_particle_col_i;
/// ----------

/// Invulnerability
/// ---------------

// `[measured]` The factor marisa_1AA60() multiplies every frame's shot damage
// by before adding it to [boss_damage]:
//
//	boss_damage += (marisa_damage_multiplier * shots_hittest(…))
//
// marisa_init() clears it and marisa_update() sets it to 1 once
// [marisa_rounds_done] reaches 2 - so Marisa takes literally no damage during
// her first two rounds, while still playing the hit sound and the white flash.
// It is only ever 0 or 1; "multiplier" is what the code does with it, and the
// dump gives no evidence that any other value was intended.
extern uint8_t marisa_damage_multiplier;
/// ---------------
// ------------------
