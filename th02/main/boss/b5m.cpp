/// Stage 5 boss - Mima
/// -------------------
/// Her per-frame update, the pattern-advance and vertical-drift helper it is
/// the only caller of, and the seven patterns above them. Together they are
/// the carve-free tail of th02_main.asm's BOSS_5_TEXT contribution, so this
/// file needs no new segment - th02/boss_5.cpp includes it directly ahead of
/// skill_calculate(), which is the one body at the address after
/// mima_update()'s generated jump table. (kb/codegen/0099)
///
/// Everything here is prepended in dump order, so a later lift out of the same
/// block goes at the TOP of this file, not the bottom. The parity that lift
/// has to preserve is stated at the seam in th02_main.asm and measured off
/// obj/th02/boss_5.obj's PUBDEFs. (kb/codegen/0160)
///
/// mima_193A4() is `static`: mima_update() is its only caller, and the dump no
/// longer holds one.

#include "platform.h"
#include "pc98.h"
// For mima_17F27()'s hand-shifted flash blit, in the same position
// th02/main/boss/b4.cpp puts it for marisa_1AE98()'s.
#include "planar.h"
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/math/randring.hpp"
#include "th02/main/frames.hpp"
#include "th02/main/bg_particle.hpp"
#include "th02/main/bullet/bullet.hpp"
#include "th02/main/tile/tile.hpp"
#include "th02/main/boss/boss.hpp"
#include "th02/main/player/player.hpp"
#include "th02/main/player/shot.hpp"
#include "th02/main/stage/stage.hpp"
#include "th02/hardware/pages.hpp"
#include "th02/main/item/item.hpp"
#include "th02/math/vector.hpp"
#include "th01/rank.h"
#include "th02/sprites/main_pat.h"

// Declared the way th02/main/boss/b4.cpp already declares them, rather than by
// including th02/resident.hpp and th02/core/globals.hpp - th02/main/boss/b5_.cpp
// is included into this same translation unit below and owns those two
// unguarded headers.
extern "C" bool reduce_effects;
extern "C" bool boss_hit_flash;
extern "C" uint8_t boss_phase;
extern "C" int boss_pos_x;
extern "C" int boss_pos_y;
extern char rank;

// Spelled with the underlying type rather than by including th02/score.h,
// which is UNGUARDED and which th02/main/boss/b5_.cpp already reaches through
// th02/core/globals.hpp further down this same translation unit. C++ linkage,
// because that is how globals.hpp declares it.
extern int32_t score;
extern "C" uint8_t boss_rank_param[5];
extern "C" void __cdecl snd_se_play(int new_se);

// The sprite the boss and midboss renderers blit. Declared exactly the way
// th02/main/boss/b4.cpp and th02/main/boss/b5.cpp already declare it.
extern "C" int patnum_2064E;

/// Mima's other procs
/// ------------------
/// Her patterns, renderers, hit test, and update are all C++ now. The only
/// forward declaration needed for this file's own source order is
/// mima_17E91(), called by mima_bg_render() before its definition. mima_19C8D()
/// and mima_end() are defined in th02/main/boss/b5.cpp and therefore require
/// cross-object declarations here. mima_180AC() goes the other direction: its
/// definition below retains C linkage because b5.cpp calls it near through the
/// MAIN_03 group.

// Defined further down this file, but mima_bg_render() is above it.
extern "C" void near mima_17E91(void);

extern "C" void near mima_18B4B(void);
extern "C" void near mima_18BA6(void);
extern "C" void near mima_18C4A(void);
extern "C" void near mima_18DE0(void);
extern "C" void near mima_18EB8(void);
extern "C" bool16 near mima_19C8D(void);

// th02/main/boss/b5.cpp, in main_03__TEXT. Reached with the same
// `nop; push cs; call near ptr` same-segment far call as everywhere else in
// this group. (kb/codegen/0014)
extern "C" void far mima_end(void);

// Turbo C++ compiled ZUN's far calls to same-code-group functions as
// `nop; push cs; call near ptr`; a plain C++ far call is `call far ptr`
// instead, so only inline ASM still emits those bytes. Spelled exactly the way
// th02/main/stage/init.cpp and th02/main/stage/loop.cpp already spell it, and
// naming no register, so [i] below stays in SI. (kb/codegen/0014,
// kb/codegen/0083)
#define nopcall_same_group(func) _asm { \
	nop; \
	push	cs; \
	call	near ptr func; \
}
/// ----------------------

/// Mima's state
/// ------------
/// The six left_*/x_*/top_*/y_* slots below are the muzzle and anchor points
/// her patterns fire from; they keep the hand names the dump already gave them.
/// [mima_muzzle_left] and [mima_muzzle_top] are the pair mima_180EC() and
/// mima_181B3() use, and are the only two of the eight this parcel renames.

extern "C" screen_x_t left_26C56;
extern "C" screen_x_t mima_muzzle_left;
extern "C" screen_x_t left_26C5A;
extern "C" screen_x_t x_26C5C;
extern "C" screen_y_t top_26C5E;
extern "C" screen_y_t mima_muzzle_top;
extern "C" screen_y_t top_26C62;
extern "C" screen_y_t y_26C64;

// 0 until [mima_patterns_until_vulnerable] patterns of the current step have
// run, 1 afterwards. mima_17C92() multiplies the frame's shot damage by it, so
// Mima cannot be damaged during the opening patterns of any step.
// [marisa_damage_multiplier]'s mechanism, one boss up.
extern "C" int mima_damage_multiplier;

// The eleven-step machine the whole fight runs on. Even steps are the
// charge-ups, odd ones run patterns, 8 is her first defeat and 10 the end.
extern "C" int mima_phase;

// Which of her patterns is running, and how many have run in this step.
extern "C" int mima_pattern;
extern "C" int mima_patterns_this_phase;

// Raised in step 7, and never lowered. Widens the pattern cycle from
// [mima_pattern_count] to all 8, and switches six of the pattern functions to
// their harder variant.
extern "C" bool mima_all_patterns;

// The ring and the filled circle mima_17A7F() draws behind her, and their
// colors. th02/main/boss/b5.cpp named the ring's two colors _head and _tail;
// only the lagging one is ever reassigned after mima_init().
extern "C" uint8_t mima_bg_ring_radius;
extern "C" uint8_t mima_bg_circle_radius;
extern "C" uint8_t mima_bg_ring_col_tail;
extern "C" uint8_t mima_bg_circle_col;

// The four parameters each charge-up step hands the odd step that follows it.
// The step ends on either [mima_phase_damage_max] damage or
// [mima_patterns_max] patterns; steps 7 and 9 set the latter to 200 and so end
// on damage alone.
extern "C" int mima_phase_damage_max;
extern "C" int mima_patterns_max;
extern "C" int mima_pattern_count;
extern "C" int mima_patterns_until_vulnerable;

// The two expanding dot-square rings mima_update() blits itself, at this
// radius and this radius + 128 around a fixed (224, 200).
extern "C" uint8_t mima_ring_radius;

// The circle's radius pulses down from this base on a 64-frame counter.
extern "C" uint8_t mima_bg_circle_radius_base;
extern "C" uint8_t mima_bg_circle_pulse_frame;

// Her vertical drift, re-rolled once per pattern by mima_193A4().
extern "C" int mima_velocity_y;

// mima_19173()'s running spread angle, advanced by boss_rank_param[4] per shot.
extern "C" unsigned char mima_spiral_angle;

// `[measured]` mima_191CC() aims this at the player once, on the pattern's
// frame 50, and NOTHING ever reads it - not this proc, not any other in the
// binary. A dead store, spelled the way the dump already spells
// [boss_pos_x_unused].
extern "C" unsigned char mima_aim_angle_unused;

// The radius of mima_191CC()'s charge-up rings. Starts at 30 and shrinks by 2
// every other frame; the rings are blitted at this radius, twice it and four
// times it.
extern "C" unsigned char mima_charge_ring_radius;
/// ------------

static const int MIMA_RING_CENTER_X = 224;
static const int MIMA_RING_CENTER_Y = 200;
static const int MIMA_RING_COUNT = 2;
static const int MIMA_RING_ANGLE_STEP = 16;
static const int MIMA_RING_DISTANCE = 128;

static const int MIMA_CIRCLE_PULSE_FRAMES = 64;

// How long each charge-up step's palette fade runs for.
static const int MIMA_CHARGE_FRAMES = 100;

// Her drift only re-aims while a pattern is this young, and only inside this
// vertical band.
static const int MIMA_DRIFT_FRAMES = 10;
static const int MIMA_TOP_MIN = 48;
static const int MIMA_TOP_MAX = 64;

// mima_19353()'s window and cadence.
static const int MIMA_SPREAD_FIRST_FRAME = 50;
static const int MIMA_SPREAD_LAST_FRAME = 190;
static const int MIMA_SPREAD_INTERVAL = 32;

// mima_19173()'s.
static const int MIMA_SPIRAL_FIRST_FRAME = 50;
static const int MIMA_SPIRAL_LAST_FRAME = 150;

// mima_191CC()'s two halves.
static const int MIMA_CHARGE_FIRST_FRAME = 50;
static const int MIMA_CHARGE_LAST_FRAME = 80;
static const int MIMA_CHARGE_RING_RADIUS_INITIAL = 30;
static const int MIMA_SPRAY_LAST_FRAME = 200;


/// Her four orbs
/// -------------
/// Shared by mima_183D0() below and by mima_18EB8() further down. Those two
/// patterns keep SEPARATE centre, radius and angle slots; only the position
/// arrays, the per-orb flag and the two shape constants are common, so they
/// are declared up here and mima_18EB8()'s own state stays with that function.

// The orbs' positions, per page. `[measured]` The same 80-byte run as
// [mima_orb_flag], which th02/main/boss/b5.cpp already published: five 8-entry
// word arrays 16 bytes apart, of which these two are the left/top pair. Only
// the lower four slots are ever written. (kb/codegen/0123)
extern "C" screen_x_t mima_orb_left_on_page[PAGE_COUNT][8];
extern "C" screen_y_t mima_orb_top_on_page[PAGE_COUNT][8];
extern "C" int16_t mima_orb_flag[8];

// Which of two patterns mima_183D0() is running. `[measured]` mima_188AA()
// clears it and mima_18B4B() sets it, and the flag picks between the two
// halves of that function's body: the cleared one detonates each orb into two
// 16-pellet rings once the formation has closed back in, the set one fires a
// single 3-pellet fan out of each and then re-expands.
extern "C" bool mima_orb_variant;

static const int MIMA_ORB_COUNT = 4;

// A quarter turn, so the four orbs sit on the corners of a square.
static const unsigned char MIMA_ORB_ANGLE_STEP = 0x40;


/// Her background, and her damage and hit test
/// -------------------------------------------

// The ring's rotation phase, advanced once per frame by mima_bg_render().
// `[measured]` mima_17A7F() adds it to the angle BETWEEN the cosine and the
// sine of each dot, so it skews the ring into a rotating ellipse rather than
// simply turning a circle. This proc held its last six references, so it is a
// rename rather than a kb/codegen/0123 alias.
extern "C" uint8_t mima_bg_ring_phase;

// The lead color of that ring; th02/main/boss/b5.cpp named the pair.
extern "C" uint8_t mima_bg_ring_col_head;

// The two sweeps mima_bg_render() runs the ring through, and the three passes
// inside each: the lagging color twice, then the leading one.
static const int MIMA_BG_SWEEP_1 = 0x80;
static const int MIMA_BG_SWEEP_2 = 256;
static const int MIMA_BG_PHASE_LAG_1 = 16;
static const int MIMA_BG_PHASE_LAG_2 = 8;

// Mima is defeated at this much damage, which is worth this many points.
static const int MIMA_DAMAGE_MAX = 6000;
static const int32_t MIMA_DEFEAT_SCORE = 500000;

// Her hitbox, as an offset from her top-left corner and a size, and the
// separate box the player is tested against. `[measured]` The two do not
// agree: the shots box is 64x64 centered on (+48, +32), the player box runs
// from (+16, +0) to (+112, +80). marisa_1AA60() has the same disagreement.
static const pixel_t MIMA_SHOTS_HITBOX_LEFT = 48;
static const pixel_t MIMA_SHOTS_HITBOX_TOP = 32;
static const pixel_t MIMA_SHOTS_HITBOX_W = 64;
static const pixel_t MIMA_SHOTS_HITBOX_H = 64;


// One pass of the dot-square ring behind her: a dot every [angle_step] over
// the 0x80-wide sweep starting at [angle_start], on a circle of
// [mima_bg_ring_radius] around her fixed centre, in [col]. [with_circle]
// additionally fills the circle inside it, once, on the sweep that starts at
// 256.
//
// `[measured]` The cosine is taken at the angle and the sine at [angle_skew]
// past it, so what this draws is a rotating ELLIPSE rather than a turning
// circle - the skew, not the start, is what makes it move. The dots also grow
// with the radius, one pixel of edge per 64 of it.
//
// `static`: mima_bg_render() below is its only caller, and the dump no longer
// holds one. It remains `pascal`: before the lift, its five word arguments
// ended in `retn 0Ah` (`b718d36d:th02_main.asm:8685`).
static void pascal near mima_17A7F(
	int col, uint8_t angle_skew, int angle_step, uint8_t with_circle,
	int angle_start
)
{
	unsigned angle_counter;
	unsigned char angle;
	uint8_t dot_edge;

	dot_edge = ((mima_bg_ring_radius >> 6) + 1);
	grcg_setcolor(GC_RMW, col);
	for(
		angle_counter = angle_start;
		(angle_start + 0x80) > angle_counter;
		angle_counter += angle_step
	) {
		angle = angle_counter;
		dot_square_left = (
			(((long)(mima_bg_ring_radius) * CosTable8[angle]) >> 8) +
			MIMA_RING_CENTER_X
		);
		angle += angle_skew;
		dot_square_top = (
			(((long)(mima_bg_ring_radius) * SinTable8[angle]) >> 8) +
			MIMA_RING_CENTER_Y
		);
		if(
			(dot_square_left < PLAYFIELD_RIGHT) &&
			(dot_square_left > 24) &&
			(dot_square_top > 8) &&
			(dot_square_top < PLAYFIELD_BOTTOM)
		) {
			grcg_dot_square_put(dot_edge);
		}
		if(with_circle && (angle_counter == MIMA_BG_SWEEP_2)) {
			grcg_setcolor(GC_RMW, mima_bg_circle_col);
			grcg_circlefill(
				MIMA_RING_CENTER_X, MIMA_RING_CENTER_Y, mima_bg_circle_radius
			);
			grcg_setcolor(GC_RMW, col);
		}
	}
	grcg_off();
}


// Redraws the entire playfield behind her: the orbs' invalidation pass, then
// a black fill, then the six passes of the dot-square ring.
//
// Also the one place [boss_left_on_back_page] and [boss_top_on_back_page] are
// re-pointed at the back page, and where the front page's position is copied
// forward into it - so every pattern in this file that moves her through
// those pointers starts from where she was drawn last frame.
extern "C" void far mima_bg_render(void)
{
	uint8_t angle_step;

	mima_17E91();
	boss_left_on_back_page = &boss_left_on_page[page_back];
	boss_top_on_back_page = &boss_top_on_page[page_back];
	*boss_left_on_back_page = boss_left_on_page[page_front];
	*boss_top_on_back_page = boss_top_on_page[page_front];
	egc_off();
	grcg_setcolor(GC_RMW, 0);
	grcg_byteboxfill_x(
		PLAYFIELD_VRAM_LEFT,
		PLAYFIELD_TOP,
		(PLAYFIELD_VRAM_RIGHT - 1),
		(PLAYFIELD_BOTTOM - 1)
	);
	angle_step = ((reduce_effects * 3) + 1);
	mima_17A7F(
		mima_bg_ring_col_tail, (mima_bg_ring_phase - MIMA_BG_PHASE_LAG_1),
		angle_step, false, MIMA_BG_SWEEP_1
	);
	mima_17A7F(
		mima_bg_ring_col_tail, (mima_bg_ring_phase - MIMA_BG_PHASE_LAG_2),
		angle_step, false, MIMA_BG_SWEEP_1
	);
	mima_17A7F(
		mima_bg_ring_col_head, mima_bg_ring_phase, angle_step, false,
		MIMA_BG_SWEEP_1
	);
	mima_17A7F(
		mima_bg_ring_col_tail, (mima_bg_ring_phase - MIMA_BG_PHASE_LAG_1),
		angle_step, true, MIMA_BG_SWEEP_2
	);
	mima_17A7F(
		mima_bg_ring_col_tail, (mima_bg_ring_phase - MIMA_BG_PHASE_LAG_2),
		angle_step, false, MIMA_BG_SWEEP_2
	);
	mima_17A7F(
		mima_bg_ring_col_head, mima_bg_ring_phase++, angle_step, false,
		MIMA_BG_SWEEP_2
	);
	grcg_off();
}


// Her damage and hit test, and her defeat. marisa_1AA60() one boss up is the
// same function against different constants, including the assign-and-test in
// one expression that keeps [damage] on the frame (kb/codegen/0143).
extern "C" void near mima_17C92(void)
{
	int damage;

	if(boss_phase != 0) {
		return;
	}
	if((damage = shots_hittest(
		(*boss_left_on_back_page + MIMA_SHOTS_HITBOX_LEFT),
		(*boss_top_on_back_page + MIMA_SHOTS_HITBOX_TOP),
		MIMA_SHOTS_HITBOX_W,
		MIMA_SHOTS_HITBOX_H
	)) != 0) {
		boss_hit_flash = true;
		boss_damage += (mima_damage_multiplier * damage);
		if(boss_damage >= MIMA_DAMAGE_MAX) {
			snd_se_play(2);
			boss_phase = 1;
			score += MIMA_DEFEAT_SCORE;
			player_invincibility_time = BOSS_DEFEAT_INVINCIBILITY_FRAMES;
		}
	}
	if(
		(player_left_on_page[page_front] > (*boss_left_on_back_page + 16)) &&
		(player_left_on_page[page_front] < (*boss_left_on_back_page + 112)) &&
		(player_top_on_page[page_front] > *boss_top_on_back_page) &&
		(player_top_on_page[page_front] < (*boss_top_on_back_page + 80))
	) {
		player_is_hit = PLAYER_HIT;
	}
}


/// Her orbs' renderer and their invalidation pass
/// ----------------------------------------------
/// Both walk all EIGHT slots of the shared arrays. `[measured]` Every pattern
/// that fills them only ever writes the lower four, so the upper four are
/// permanently flag 0 and get skipped - but they are still walked, twice, on
/// every frame of the fight.

// `[measured]` Counted up once per slot that is NOT alive, reset to 0 at the
// top of every mima_17D59() call, and read by NOTHING in the binary - the
// census over th02_main.asm found exactly two occurrences, both writes, plus
// the `dw ?`. A dead store, and this proc holds its last two references, so
// it is a rename rather than a kb/codegen/0123 alias.
extern "C" int mima_orbs_gone_unused;

static const int MIMA_ORB_SLOT_COUNT = 8;

// The box mima_17D59() hands to shots_hittest() for each alive orb, offset by
// 4 pixels to the right of that orb's own top-left corner. `[measured]` The
// same box at the same offset b4.hpp measures for Marisa's orbs.
static const pixel_t MIMA_ORB_HITTEST_W = 24;
static const pixel_t MIMA_ORB_HITTEST_H = 32;

// The sprite itself, which is also what mima_17E91() invalidates.
static const pixel_t MIMA_ORB_W = 32;
static const pixel_t MIMA_ORB_H = 32;

// Half of that, on both axes, is the box mima_17D59() tests the player
// against - centered on the orb's top-left CORNER rather than on the sprite.
static const pixel_t MIMA_ORB_PLAYER_REACH = 16;

// The four orb sprites, picked by the low two bits of the slot index, so the
// upper four slots would repeat them.
static const int MIMA_ORB_PATNUM = 137;


// Blits the orbs, and runs both of their hit tests: shots against a 24x32 box
// offset into the orb, and the player against a 32x32 box centered on its
// top-left corner.
//
// `[measured]` ZUN quirk: the orb is read on the BACK page and the player on
// the FRONT one, so the player collision is a frame stale relative to the
// thing it is drawn against.
extern "C" void near mima_17D59(void)
{
	register int i;

	mima_orbs_gone_unused = 0;
	for(i = 0; i < MIMA_ORB_SLOT_COUNT; i++) {
		if(mima_orb_flag[i] != 1) {
			mima_orbs_gone_unused++;
			continue;
		}
		shots_hittest(
			(mima_orb_left_on_page[page_back][i] + 4),
			mima_orb_top_on_page[page_back][i],
			MIMA_ORB_HITTEST_W,
			MIMA_ORB_HITTEST_H
		);
		if(
			((mima_orb_left_on_page[page_back][i] - MIMA_ORB_PLAYER_REACH) <
				player_left_on_page[page_front]) &&
			((mima_orb_left_on_page[page_back][i] + MIMA_ORB_PLAYER_REACH) >
				player_left_on_page[page_front]) &&
			((mima_orb_top_on_page[page_back][i] - MIMA_ORB_PLAYER_REACH) <
				player_top_on_page[page_front]) &&
			((mima_orb_top_on_page[page_back][i] + MIMA_ORB_PLAYER_REACH) >
				player_top_on_page[page_front])
		) {
			player_is_hit = PLAYER_HIT;
		}
		super_put_rect(
			mima_orb_left_on_page[page_back][i],
			mima_orb_top_on_page[page_back][i],
			((i & 3) + MIMA_ORB_PATNUM)
		);
	}
}


// Invalidates the tiles behind every orb that is on screen, and retires the
// ones that were flagged for removal. th02/main/boss/b3.cpp's
// stones_bg_render() is the same shape one boss down.
//
// mima_bg_render() above is its only caller now. The forward declaration keeps
// this source order; before that caller's lift, the root reached this body via
// `extrn _mima_17E91:near` (`4c235f13:th02_main.asm:619`).
extern "C" void near mima_17E91(void)
{
	register int i;

	for(i = 0; i < MIMA_ORB_SLOT_COUNT; i++) {
		if(
			(mima_orb_flag[i] != 0) &&
			(mima_orb_top_on_page[page_back][i] > -MIMA_ORB_PLAYER_REACH) &&
			(mima_orb_top_on_page[page_back][i] < (PLAYFIELD_BOTTOM - 2))
		) {
			tiles_invalidate_rect(
				mima_orb_left_on_page[page_back][i],
				mima_orb_top_on_page[page_back][i],
				MIMA_ORB_W,
				MIMA_ORB_H
			);
			if(mima_orb_flag[i] == 2) {
				mima_orb_flag[i] = 0;
			}
		}
	}
}


/// Her renderer, her rank table, and her horizontal sweep
/// ------------------------------------------------------

// How many frames of the hit flash mima_17F27() actually blits: it counts
// every frame [boss_hit_flash] is raised and hand-blits on every fourth one,
// so three flash frames out of four fall back to the ordinary sprite.
// `[measured]` This proc holds the last two ASM references to the slot, so it
// is a rename rather than a kb/codegen/0123 alias.
extern "C" uint8_t mima_flash_frame;

// `[measured]` from mima_17F27()'s hand-written flash blit, which walks
// super_patdata[] linearly at 6 VRAM bytes (48 pixels) per row for 0x80 rows
// and then repeats that at x + 48 and x + 96. So she is drawn as three 48x128
// patterns and is 144x128 in total - the same shape b4.hpp measures for
// Marisa, who is two of them.
static const int MIMA_PATTERN_COUNT = 3;
static const pixel_t MIMA_PATTERN_W = 48;
static const pixel_t MIMA_H = 128;

// The three legs of her horizontal sweep, and the X each one stops at. Every
// leg clamps by snapping [boss_phase_frame] forward to the next leg's first
// frame rather than by clamping the position, so a leg that finishes early
// simply hands its remaining frames to the one after it.
static const int MIMA_SWEEP_LEG_2_FRAME = 66;
static const int MIMA_SWEEP_LEG_3_FRAME = 178;
static const int MIMA_SWEEP_END_FRAME = 234;
static const screen_x_t MIMA_SWEEP_LEFT_1 = 32;
static const screen_x_t MIMA_SWEEP_RIGHT = 256;
static const screen_x_t MIMA_SWEEP_LEFT_2 = 144;
static const int MIMA_SWEEP_SPEED = 2;
static const int MIMA_SWEEP_INTERVAL = 32;


// Blits Mima herself, as the three 48x128 patterns she is drawn from. Regular
// frames go through super_put_rect(); every fourth frame she has been hit on
// is instead blitted by hand into the GRCG, one 16-dot chunk at a time, from
// the raw superimpose pattern data.
//
// This is marisa_1AE98() one boss down, and it is the same function: same
// hand-shifted chunk, same three bounds tests, same fall back to
// super_put_rect(). The differences are that Mima is three patterns rather
// than two, that hers are driven by a loop rather than written out twice, and
// that the flash only fires on one frame in four.
extern "C" void near mima_17F27(void)
{
	register int col;
	screen_x_t x;
	int row;
	screen_y_t y;
	vram_offset_t vram_offset;
	vram_offset_t vram_offset_first;
	int i;
	dots8_t far *p;
	uint8_t shift_r;
	int chunk;

	if(boss_hit_flash) {
		mima_flash_frame++;
		// `== 0` rather than `!`: on a byte-sized global the explicit
		// comparison keeps the mask test in memory, where the negation
		// operator first loads the byte into AL and zero-extends it.
		// (kb/codegen/0028)
		if((mima_flash_frame & 3) == 0) {
			shift_r = (*boss_left_on_back_page & (BYTE_DOTS - 1));
			boss_hit_flash = false;
			vram_offset_first = vram_offset_shift(
				*boss_left_on_back_page, *boss_top_on_back_page
			);
			grcg_setcolor(GC_RMW, 4);
			for(i = 0; i < MIMA_PATTERN_COUNT; i++) {
				vram_offset = (
					vram_offset_first + (i * (MIMA_PATTERN_W / BYTE_DOTS))
				);
				p = reinterpret_cast<dots8_t far *>(
					MK_FP(super_patdata[patnum_2064E + i], 0)
				);
				row = 0;
				y = *boss_top_on_back_page;
				while(row < MIMA_H) {
					if(y >= PLAYFIELD_BOTTOM) {
						break;
					}
					col = 0;
					x = ((i * MIMA_PATTERN_W) + *boss_left_on_back_page);
					while(col < (MIMA_PATTERN_W / BYTE_DOTS)) {
						if(
							(x > 0) && (x < PLAYFIELD_RIGHT) &&
							(y >= PLAYFIELD_TOP)
						) {
							// The two shifts, spelled out rather than reached
							// for as a rotate: at shift_r == 0 the left half
							// shifts by 16, which a 286 masks to 0, so the
							// pair is NOT a rotate at every shift.
							// marisa_1AE98() keeps its left shift in its own
							// local; this frame has no room for one.
							chunk = (
								(*p << (16 - shift_r)) + (*p >> shift_r)
							);
							grcg_chunk(vram_offset + col, 16) = chunk;
						}
						col++;
						x += BYTE_DOTS;
						p++;
					}
					row++;
					vram_offset += ROW_SIZE;
					y++;
				}
			}
			grcg_off();
			return;
		}
	}
	super_put_rect(
		boss_left_on_page[page_back], boss_top_on_page[page_back],
		patnum_2064E
	);
	super_put_rect(
		(boss_left_on_page[page_back] + MIMA_PATTERN_W),
		boss_top_on_page[page_back],
		(patnum_2064E + 1)
	);
	super_put_rect(
		(boss_left_on_page[page_back] + (MIMA_PATTERN_W * 2)),
		boss_top_on_page[page_back],
		(patnum_2064E + 2)
	);
}


// The whole of [boss_rank_param] for her fight, in one of two sets. Easy gets
// a 2-spread and slower, wider bullets; everything above it gets the 32-ring
// mima_181B3() drops and a tighter aim.
extern "C" void near mima_180AC(void)
{
	if(rank != RANK_EASY) {
		boss_rank_param[0] = BG_16_RING;
		boss_rank_param[1] = 0x17;
		boss_rank_param[2] = BG_32_RING;
		boss_rank_param[3] = 0x21;
		boss_rank_param[4] = 6;
		return;
	}
	boss_rank_param[0] = BG_16_RING;
	boss_rank_param[1] = 0x19;
	boss_rank_param[2] = BG_2_SPREAD_MEDIUM_AIMED;
	boss_rank_param[3] = 0x22;
	boss_rank_param[4] = 8;
}


// Her horizontal sweep: three legs - left to 32, right to 256, left again to
// 144 - at 2 pixels a frame, firing two bullets every 32nd frame from her
// muzzle. One is a random angle in the lower left quadrant, in
// [boss_rank_param][0]'s group; the other goes straight down, in the group her
// harder variant swaps for a different one.
extern "C" void near mima_180EC(void)
{
	if(boss_phase_frame < 10) {
		return;
	}
	if(boss_phase_frame == 10) {
		bullets_set_stack_multiplier(0);
		patnum_2064E = 131;
	}
	if(boss_phase_frame < MIMA_SWEEP_LEG_2_FRAME) {
		*boss_left_on_back_page -= MIMA_SWEEP_SPEED;
		if(*boss_left_on_back_page <= MIMA_SWEEP_LEFT_1) {
			boss_phase_frame = MIMA_SWEEP_LEG_2_FRAME;
		}
	} else if(boss_phase_frame < MIMA_SWEEP_LEG_3_FRAME) {
		*boss_left_on_back_page += MIMA_SWEEP_SPEED;
		if(*boss_left_on_back_page >= MIMA_SWEEP_RIGHT) {
			boss_phase_frame = MIMA_SWEEP_LEG_3_FRAME;
		}
	} else if(boss_phase_frame < MIMA_SWEEP_END_FRAME) {
		*boss_left_on_back_page -= MIMA_SWEEP_SPEED;
		if(*boss_left_on_back_page <= MIMA_SWEEP_LEFT_2) {
			boss_phase_frame = MIMA_SWEEP_END_FRAME;
		}
	} else {
		boss_phase_frame = 0;
		patnum_2064E = 128;
		bullets_set_stack_multiplier(1);
	}
	if((boss_phase_frame % MIMA_SWEEP_INTERVAL) != 0) {
		return;
	}
	bullets_add_pellet(
		mima_muzzle_left, mima_muzzle_top,
		(randring2_next8_and(0x0F) + 0x38), boss_rank_param[0],
		((3 << 4) + 12)
	);
	bullets_add_pellet(
		mima_muzzle_left, mima_muzzle_top, 0x00,
		boss_rank_param[1 + mima_all_patterns], ((2 << 4) + 8)
	);
}


/// Her charge-and-column pattern
/// -----------------------------
/// `[measured]` This proc held the LAST ASM references to all three slots
/// below, so all three are renames rather than kb/codegen/0123 aliases. The
/// [mima_charge_*] name family is NOT available for them: it already belongs
/// to mima_191CC()'s own charge-up, further down this file.

// `[measured]` Written 0x30 on the frame the charge starts, and read by
// NOTHING in the binary - the census over th02_main.asm found exactly two
// occurrences, this write and the `db ?`. Spelled the way the dump already
// spells [boss_pos_x_unused] and [mima_aim_angle_unused].
extern "C" unsigned char mima_ray_unused;

// The base angle of the nine-ray fan, and the sole driver of its spread: the
// step between rays is ((0x40 - this) >> 2) in 8-bit UNSIGNED arithmetic, so
// once it walks past 0x40 the subtraction wraps and the fan turns into a
// near-full circle.
extern "C" unsigned char mima_ray_angle;

// The palette tone this pattern drives, straight into master.lib's
// [PaletteTone]. Starts at 100, climbs by 1 every other frame of the charge.
extern "C" uint8_t mima_ray_tone;

// The nine rays, and how far out they are drawn from her muzzle.
static const int MIMA_RAY_COUNT = 9;
static const int MIMA_RAY_LENGTH = 400;


/// Her flying orb formation
/// ------------------------
/// mima_183D0()'s state. `[measured]` Six more renames on the same terms as
/// the three above - every ASM reference to each was inside this proc.

// Where the formation is, in 1/16th pixels. Both mima_188AA() and mima_18B4B()
// seed it from her sprite and then let the velocity vector move it.
extern "C" int mima_orb_flight_center_x;
extern "C" int mima_orb_flight_center_y;

// How far the four orbs sit from that centre, and the per-frame step applied
// to it. The step is signed and gets rewritten by both halves of the pattern.
extern "C" int mima_orb_flight_radius;
extern "C" signed char mima_orb_flight_radius_step;

// The angle of the first orb; the other three are [MIMA_ORB_ANGLE_STEP] apart.
// Advances by 5 every frame the pattern does anything at all.
extern "C" unsigned char mima_orb_flight_angle;

// Raised by the [mima_orb_variant] half once its orbs have fired, so that the
// formation's rotation reverses for the rest of the pattern.
extern "C" bool mima_orb_flight_detonated;

// The velocity vector each of the two callers owns. Both keep the dump's own
// hand names: they are a matched pair whose only difference is which caller
// writes them, and giving that pair a name needs a ruling about the two
// patterns' identities that lifting them does not.
extern "C" screen_point_t point_26CD6;
extern "C" screen_point_t point_26CDE;


// Her charge-and-column pattern: a nine-ray fan out of her muzzle for a
// hundred frames while the palette brightens, then a vertical column at a
// random offset beside her for 28 frames, and finally a drop back to the top
// of the playfield that fires a ring every 16th frame on anything above Easy.
extern "C" void near mima_181B3(void)
{
	register int i;

	// One variable for the ray fan's end X and for the column's X: ZUN reused
	// the slot, and the frame the original allocates has room for neither a
	// second register variable nor a fourth stack local.
	register int x;

	// The same three-role reuse on the stack: the ray angle, then the column's
	// random offset, then the bullet group of the ring at the bottom.
	unsigned char angle;

	unsigned char angle_step;
	int ray_end_y;

	if(boss_phase_frame < 20) {
		return;
	}
	if(boss_phase_frame < 70) {
		patnum_2064E = 128;
		*boss_left_on_back_page += (
			((*boss_left_on_back_page + 64) < player_topleft.x) ? 2 : -2
		);
		return;
	}
	if(boss_phase_frame == 70) {
		snd_se_play(9);
		patnum_2064E = 131;
		mima_ray_unused = 0x30;
		mima_ray_angle = 0;
		mima_ray_tone = 100;
		return;
	}
	if(boss_phase_frame < 170) {
		grcg_setcolor(GC_RMW, 13);
		angle_step = (0x40 - mima_ray_angle);
		angle_step >>= 2;
		for(
			i = 0, angle = mima_ray_angle;
			i < MIMA_RAY_COUNT;
			angle += angle_step, i++
		) {
			x = (
				(((long)(MIMA_RAY_LENGTH) * CosTable8[angle]) >> 8) +
				mima_muzzle_left
			);
			ray_end_y = (
				(((long)(MIMA_RAY_LENGTH) * SinTable8[angle]) >> 8) +
				mima_muzzle_top
			);
			grcg_line(mima_muzzle_left, mima_muzzle_top, x, ray_end_y);
		}
		// ZUN quirk: this arm only runs from phase frame 71 onwards, so the
		// test is always true and the angle always advances.
		if(boss_phase_frame >= 36) {
			mima_ray_angle++;
		}
		grcg_off();
		if(boss_phase_frame & 1) {
			mima_ray_tone++;
		}
		palette_settone(mima_ray_tone);
		return;
	}
	if(boss_phase_frame == 170) {
		snd_se_play(5);
		patnum_2064E = 134;
		return;
	}
	if(boss_phase_frame <= 198) {
		*boss_top_on_back_page = (272 - ((boss_phase_frame & 3) * 60));
		grcg_setcolor(GC_RMW, 13);
		angle = randring2_next8_and(0x7F);
		x = (angle + *boss_left_on_back_page + 8);
		grcg_line(x, PLAYFIELD_TOP, x, (PLAYFIELD_BOTTOM - 1));
		grcg_off();
		// ZUN quirk: the column's hit test uses HER left edge rather than the
		// column's X, so the hurtbox and the thing you can see are in
		// different places for all 28 frames.
		if(
			((*boss_left_on_back_page + 16) < player_topleft.x) &&
			((*boss_left_on_back_page + 96) > player_topleft.x)
		) {
			player_is_hit = PLAYER_HIT;
		}
		// ZUN quirk: the mask is ZERO, so the test can never be true and the
		// 150 is unreachable. Before the lift, the listing used
		// `test byte ptr _boss_phase_frame, 0`
		// (`0c59da76:th02_main.asm:9549`).
		if(boss_phase_frame & 0) {
			mima_ray_tone = 150;
		} else {
			mima_ray_tone = 100;
		}
		palette_settone(mima_ray_tone);
		return;
	}
	if(boss_phase_frame <= 262) {
		patnum_2064E = 128;
		*boss_top_on_back_page -= 4;
		if(*boss_top_on_back_page <= 64) {
			boss_phase_frame = 263;
		}
		if(rank == RANK_EASY) {
			return;
		}
		if((boss_phase_frame % 16) != 0) {
			return;
		}
		// Two assignment statements rather than a conditional expression: the
		// original stores each constant straight into the slot, where a
		// ternary would compute it in AL and store once at the join. The same
		// shape mima_18EB8()'s [pellet_angle] needs, further down.
		if(!mima_all_patterns) {
			angle = BG_16_RING;
		} else {
			angle = BG_32_RING;
		}
		bullets_add_pellet(
			left_26C56, top_26C5E, 0x00, angle, ((3 << 4) + 12)
		);
		return;
	}
	boss_phase_frame = 0;
}


// The worker both of her flying-orb patterns hand their velocity vector to.
// Seeds the four orbs on a square around her at phase frame 80, expands the
// formation for 16 frames, and then flies it along the vector while the radius
// walks by [mima_orb_flight_radius_step]. What happens once the formation has
// closed back in is [mima_orb_variant]'s decision.
//
// `[measured]` ZUN quirk: the seeding pass at frame 80 divides the centre by
// 16 twice - once into the array and once through the `>> 4` below - because
// it runs on the same frame the centre is still in whole pixels. Every later
// frame reads it as 1/16ths.
static void pascal near mima_183D0(int *velocity_x, int *velocity_y)
{
	int i;

	// Both a pellet counter and a pellet speed, in the two detonation loops.
	int speed;

	unsigned char angle;

	// The orb being detonated, and later the formation's own centre.
	register int left;
	register int top;

	if(boss_phase_frame == 40) {
		snd_se_play(9);
		patnum_2064E = 131;
		mima_orb_flight_center_x = (*boss_left_on_back_page + 80);
		mima_orb_flight_center_y = (*boss_top_on_back_page + 64);
		mima_orb_flight_center_x <<= 4;
		mima_orb_flight_center_y <<= 4;
		mima_orb_flight_radius = 4;
		mima_orb_flight_radius_step = -1;
		mima_orb_flight_detonated = false;
	} else if(boss_phase_frame < 80) {
		return;
	} else if(boss_phase_frame == 80) {
		snd_se_play(10);
		patnum_2064E = 134;
		for(
			i = 0, angle = mima_orb_flight_angle;
			i < MIMA_ORB_COUNT;
			i++, angle = (angle + MIMA_ORB_ANGLE_STEP)
		) {
			mima_orb_left_on_page[0][i] = mima_orb_left_on_page[1][i] = (
				(((long)(mima_orb_flight_radius) * CosTable8[angle]) >> 8) +
				(mima_orb_flight_center_x >> 4)
			);
			mima_orb_top_on_page[0][i] = mima_orb_top_on_page[1][i] = (
				(((long)(mima_orb_flight_radius) * SinTable8[angle]) >> 8) +
				(mima_orb_flight_center_y >> 4)
			);
			mima_orb_flag[i] = 1;
		}
	} else if(boss_phase_frame < 96) {
		mima_orb_flight_radius += 4;
		for(
			i = 0, angle = mima_orb_flight_angle;
			i < MIMA_ORB_COUNT;
			i++, angle = (angle + MIMA_ORB_ANGLE_STEP)
		) {
			mima_orb_left_on_page[page_back][i] = (
				(((long)(mima_orb_flight_radius) * CosTable8[angle]) >> 8) +
				(mima_orb_flight_center_x >> 4)
			);
			mima_orb_top_on_page[page_back][i] = (
				(((long)(mima_orb_flight_radius) * SinTable8[angle]) >> 8) +
				(mima_orb_flight_center_y >> 4)
			);
		}
	} else {
		mima_orb_flight_center_x += *velocity_x;
		mima_orb_flight_center_y += *velocity_y;
		mima_orb_flight_radius += mima_orb_flight_radius_step;
		// Two forward jumps rather than a conditional chain, because the
		// original tests BOTH flags before any of the three arms and then
		// emits them in the order below. `[measured]` with kb/codegen/0152's
		// probe over four shapes: a three-armed conditional chain puts the
		// [mima_orb_variant] arm FIRST, and hoisting the two tests into one
		// conjunction re-tests [mima_orb_variant] at the join. Only this
		// shape emits the two branches back to back and then the three arms
		// in this order, with no re-test.
		if(mima_orb_variant) {
			goto orbs_flying_back;
		}
		if(!mima_all_patterns) {
			goto radius_walk;
		}
		if(mima_orb_flight_radius == 196) {
			boss_phase_frame = 0;
		}
		if(mima_orb_flight_radius < 20) {
			mima_orb_flight_detonated = false;
			snd_se_play(3);
			bullets_set_stack_multiplier(0);
			for(i = 0; i < MIMA_ORB_COUNT; i++) {
				left = (mima_orb_left_on_page[page_back][i] + 12);
				top = (mima_orb_top_on_page[page_back][i] + 12);
				for(
					speed = 0,
					angle = ((i << 6) + mima_orb_flight_angle - 0x40);
					speed < (1 << 4);
					speed++, angle = (angle + 0x08)
				) {
					bullets_add_pellet(
						left, top, angle, BG_1, ((2 << 4) + 3)
					);
				}
				for(
					speed = 0,
					angle = ((i << 6) + mima_orb_flight_angle - 0x3C);
					speed < (1 << 4);
					speed++, angle = (angle + 0x08)
				) {
					bullets_add_pellet(
						left, top, angle, BG_1, (((2 << 4) + 3) - speed)
					);
				}
				mima_orb_flag[i] = 2;
			}
			bullets_set_stack_multiplier(1);
			mima_orb_flight_radius = 320;
			patnum_2064E = 128;
		}
		goto reposition;

orbs_flying_back:
		if(mima_orb_flight_detonated) {
			mima_orb_flight_angle = (mima_all_patterns
				? (mima_orb_flight_angle - 3)
				: (mima_orb_flight_angle - 5)
			);
		}
		if(mima_orb_flight_radius >= 400) {
			goto retire;
		}
		if(mima_orb_flight_radius < 20) {
			*velocity_x = 0;
			*velocity_y = 0;
			mima_orb_flight_detonated = true;
			snd_se_play(3);
			angle = (mima_orb_flight_angle + 0x08);
			for(
				i = 0;
				i < MIMA_ORB_COUNT;
				i++, angle = (angle + MIMA_ORB_ANGLE_STEP)
			) {
				left = (mima_orb_left_on_page[page_back][i] + 12);
				top = (mima_orb_top_on_page[page_back][i] + 12);
				for(speed = 0; speed < (3 << 4); speed += 12) {
					bullets_add_pellet(
						left, top, (angle + speed), BG_1,
						(speed + ((2 << 4) + 3))
					);
				}
			}
			patnum_2064E = 128;
			mima_orb_flight_radius_step = 5;
		}
		goto reposition;

radius_walk:
		if(mima_orb_flight_radius < 0x20) {
			mima_orb_flight_radius_step = 2;
		} else if(mima_orb_flight_radius > 0x80) {
			mima_orb_flight_radius_step = -2;
		}

reposition:
		left = (mima_orb_flight_center_x >> 4);
		top = (mima_orb_flight_center_y >> 4);
		if(
			((left + mima_orb_flight_radius) < 0) ||
			((left - mima_orb_flight_radius) > 416) ||
			((top + mima_orb_flight_radius) < -16) ||
			((top - mima_orb_flight_radius) > 386) ||
			(boss_phase_frame > 500)
		) {
retire:
			for(i = 0; i < MIMA_ORB_COUNT; i++) {
				mima_orb_flag[i] = 2;
			}
			boss_phase_frame = 0;
			patnum_2064E = 128;
		}
		for(
			i = 0, angle = mima_orb_flight_angle;
			i < MIMA_ORB_COUNT;
			i++, angle = (angle + MIMA_ORB_ANGLE_STEP)
		) {
			mima_orb_left_on_page[page_back][i] = (
				(((long)(mima_orb_flight_radius) * CosTable8[angle]) >> 8) +
				(mima_orb_flight_center_x >> 4)
			);
			mima_orb_top_on_page[page_back][i] = (
				(((long)(mima_orb_flight_radius) * SinTable8[angle]) >> 8) +
				(mima_orb_flight_center_y >> 4)
			);
		}
	}
	mima_orb_flight_angle = (mima_orb_flight_angle + 5);
}


// The other half of her orb pattern's setup, and the one that leaves
// [mima_orb_variant] clear: on phase frame 40, aim a 48-long vector from her
// middle at the player and hand it to mima_183D0(). mima_18B4B() below is the
// same function with 52 and a set flag.
extern "C" void near mima_188AA(void)
{
	int x1;
	int y1;

	if(boss_phase_frame < 40) {
		return;
	}
	if(boss_phase_frame == 40) {
		mima_orb_variant = false;
		x1 = (*boss_left_on_back_page + 80);
		y1 = (*boss_top_on_back_page + 64);
		vector2_between_plus(
			x1, y1, player_topleft.x, player_topleft.y, 0,
			point_26CD6.x, point_26CD6.y, 48
		);
	}
	mima_183D0(&point_26CD6.x, &point_26CD6.y);
}


/// Her two pellet streams
/// ----------------------
/// Both open with the same 60-frame approach her bouncing stars and her
/// pellet fan open with, then fire from the same muzzle on an angle they walk
/// one step at a time. `[measured]` These two procs held the LAST ASM
/// references to all three slots below, so all three are renames rather than
/// kb/codegen/0123 aliases.

// mima_18905()'s aim angle and its speed. The angle is seeded off the random
// ring rather than aimed at the player, so where the stream starts depends on
// what the ring happened to be at. Both walk together: the angle sweeps down
// while the speed holds, then both climb.
extern "C" unsigned char mima_stream_angle;
extern "C" uint8_t mima_stream_speed;

// mima_18A1B()'s. Starts a sixth of a turn below 0 and walks 3 steps a frame,
// up for 32 frames and back down for 18.
extern "C" unsigned char mima_pair_angle;


// Her single-pellet stream: one pellet every (8 - [rank]) frames from phase
// frame 100, on an angle that walks down to frame 160 and then back up to 240
// while the speed ramps with it. Her harder variant gets a second sweep out
// to frame 400; the easier one ends the pattern the moment frame 240 passes.
//
// ZUN quirk: the fire test below runs on the SAME frame that resets
// [boss_phase_frame] to 0, and reads the angle and speed the resetting arm
// left behind. The reset is what stops it firing, not a guard of its own.
extern "C" void near mima_18905(void)
{
	if(boss_phase_frame < 10) {
		return;
	}
	if(boss_phase_frame < 70) {
		patnum_2064E = 128;
		// The same conditional expression her other approaches use: the
		// original picks the step in AX and adds it through the pointer once.
		*boss_left_on_back_page += (
			((*boss_left_on_back_page + 64) < player_topleft.x) ? 2 : -2
		);
	} else if(boss_phase_frame == 70) {
		snd_se_play(9);
		patnum_2064E = 131;
		mima_stream_speed = ((1 << 4) + 4);
	} else if(boss_phase_frame < 100) {
		return;
	} else if(boss_phase_frame == 100) {
		snd_se_play(10);
		patnum_2064E = 134;
		mima_stream_angle = randring2_next8();
	} else if(boss_phase_frame <= 160) {
		mima_stream_angle--;
	} else if(boss_phase_frame <= 240) {
		mima_stream_angle++;
		mima_stream_speed++;
	} else if(mima_all_patterns) {
		if(boss_phase_frame <= 320) {
			mima_stream_angle++;
			mima_stream_speed--;
		} else if(boss_phase_frame <= 400) {
			mima_stream_angle--;
			mima_stream_speed++;
		} else {
			boss_phase_frame = 0;
			patnum_2064E = 128;
		}
	} else {
		boss_phase_frame = 0;
		patnum_2064E = 128;
	}
	if(boss_phase_frame <= 100) {
		return;
	}
	if((boss_phase_frame % (8 - rank)) != 0) {
		return;
	}
	bullets_add_pellet(
		left_26C5A, top_26C62, mima_stream_angle, boss_rank_param[3],
		mima_stream_speed
	);
	snd_se_play(3);
}


// Her horizontally symmetric pellet pairs: one mirrored 2-spread every frame
// from phase frame 100, on an angle that walks +3 out to frame 132 and -3 back
// to 150. From there to 226 she fires one narrow 3-spread every 8th frame
// instead - or, in her harder variant, two pellets on independent random
// angles across the lower half of the turn.
extern "C" void near mima_18A1B(void)
{
	if(boss_phase_frame < 10) {
		return;
	}
	if(boss_phase_frame < 70) {
		patnum_2064E = 128;
		*boss_left_on_back_page += (
			((*boss_left_on_back_page + 64) < player_topleft.x) ? 2 : -2
		);
		return;
	}
	if(boss_phase_frame == 70) {
		snd_se_play(9);
		patnum_2064E = 131;
		mima_pair_angle = -0x28;
		return;
	}
	if(boss_phase_frame < 100) {
		return;
	}
	if(boss_phase_frame == 100) {
		snd_se_play(10);
		patnum_2064E = 134;
		return;
	}
	if(boss_phase_frame <= 132) {
		bullets_add_pellet(
			left_26C5A, top_26C62, mima_pair_angle,
			BG_2_SPREAD_HORIZONTALLY_SYMMETRIC, ((5 << 4) + 10)
		);
		mima_pair_angle += 0x03;
		return;
	}
	if(boss_phase_frame <= 150) {
		bullets_add_pellet(
			left_26C5A, top_26C62, mima_pair_angle,
			BG_2_SPREAD_HORIZONTALLY_SYMMETRIC, ((5 << 4) + 10)
		);
		mima_pair_angle -= 0x03;
		return;
	}
	if(boss_phase_frame <= 226) {
		if((boss_phase_frame % 8) != 1) {
			return;
		}
		if(!mima_all_patterns) {
			bullets_add_pellet(
				left_26C5A, top_26C62, 0x40, BG_3_SPREAD_NARROW,
				((5 << 4) + 10)
			);
			return;
		}
		bullets_add_pellet(
			left_26C5A, top_26C62, (randring2_next8_and(0x3F) + 0x20), BG_1,
			(5 << 4)
		);
		bullets_add_pellet(
			left_26C5A, top_26C62, (randring2_next8_and(0x3F) + 0x20), BG_1,
			(5 << 4)
		);
		return;
	}
	boss_phase_frame = 0;
	patnum_2064E = 128;
}


/// The five patterns that were the tail of BOSS_5_TEXT
/// ---------------------------------------------------

// Where mima_18EB8()'s orbs orbit, and how far out they are - a set of slots
// separate from mima_183D0()'s [mima_orb_flight_*] above, even though both
// patterns move the same four orbs. `[measured]` ZUN quirk: the seeding pass
// below divides the centre by 16 and the per-frame pass does not, so the four
// orbs spend their first frame at a sixteenth of the distance from the origin
// that every later frame puts them at.
extern "C" screen_x_t mima_orb_center_x;
extern "C" screen_y_t mima_orb_center_y;
extern "C" int16_t mima_orb_radius;

// The angle of the first orb; the other three are a quarter-turn apart. Walks
// back by 3 every frame, so the formation counter-rotates as it expands.
extern "C" unsigned char mima_orb_angle;

// The aim angle of her bouncing-star pattern, and the direction it sweeps in.
extern "C" unsigned char mima_star_angle;
extern "C" signed char mima_star_direction;

// The running angle of her symmetric pellet fan.
extern "C" unsigned char mima_fan_angle;

// One mirrored pair of her pellet fan, and the 10 steps the angle then walks.
#define mima_fan_fire() { \
	bullets_add_pellet( \
		left_26C5A, top_26C62, (mima_fan_angle + 0x08), \
		BG_3_SPREAD_NARROW, (5 << 4) \
	); \
	bullets_add_pellet( \
		left_26C5A, top_26C62, (0x78 - mima_fan_angle), \
		BG_3_SPREAD_NARROW, (5 << 4) \
	); \
	mima_fan_angle = (mima_fan_angle + 0x0A); }

// One star of the bouncing-star pattern. Every one of the six is fired from
// the same muzzle at the same speed, so only the angle differs.
#define mima_star_add(angle) bullets_add_16x16( \
	left_26C5A, \
	top_26C62, \
	(angle), \
	BSM_BOUNCE_TOP_BOTTOM, \
	PAT_BULLET16_STAR, \
	(5 << 4) \
)


// Her orb pattern's setup, and the one that leaves [mima_orb_variant] set: on
// phase frame 40, aim a 52-long vector from her middle at the player and hand
// it to mima_183D0(), which flies the four orbs along it from then on.
extern "C" void near mima_18B4B(void)
{
	int x1;
	int y1;

	if(boss_phase_frame < 40) {
		return;
	}
	if(boss_phase_frame == 40) {
		mima_orb_variant = true;
		x1 = (*boss_left_on_back_page + 80);
		y1 = (*boss_top_on_back_page + 64);
		vector2_between_plus(
			x1, y1, player_topleft.x, player_topleft.y, 0,
			point_26CDE.x, point_26CDE.y, 52
		);
	}
	mima_183D0(&point_26CDE.x, &point_26CDE.y);
}


// The three sound cues of her defeat, and the item it drops: a bomb if she was
// killed in step 6, an extra life in step 2, and a big power item otherwise.
extern "C" void near mima_18BA6(void)
{
	if(boss_phase_frame < 10) {
		return;
	}
	if(boss_phase_frame == 10) {
		snd_se_play(9);
		patnum_2064E = 131;
		return;
	}
	if(boss_phase_frame < 40) {
		return;
	}
	// items_add()'s parameters are (left, top, type), which is th02/item.cpp's
	// definition order rather than th02/main/item/item.hpp's declaration
	// order. th02/main/boss/b3.cpp carries the same note.
	if(boss_phase_frame == 40) {
		snd_se_play(10);
		patnum_2064E = 134;
		if(mima_phase == 6) {
			items_add(left_26C5A, top_26C62, IT_BOMB);
			return;
		}
	} else if(boss_phase_frame == 60) {
		snd_se_play(10);
	} else if(boss_phase_frame == 80) {
		snd_se_play(10);
		patnum_2064E = 128;
		if(mima_phase == 2) {
			items_add(left_26C5A, top_26C62, IT_1UP);
			return;
		}
	} else {
		return;
	}
	items_add(left_26C5A, top_26C62, IT_BIGPOWER);
}


// Her bouncing-star pattern: she closes in on the player for 60 frames, then
// fires four stars - two on Easy - every 8th frame from phase frame 100, on an
// aim angle that sweeps one step per odd frame and sweeps back again at 210.
extern "C" void near mima_18C4A(void)
{
	if(boss_phase_frame < 10) {
		return;
	}
	if(boss_phase_frame < 70) {
		patnum_2064E = 128;
		// A conditional expression rather than two assignments: the original
		// picks the step in AX and adds it through the pointer once, at the
		// join.
		*boss_left_on_back_page += (
			((*boss_left_on_back_page + 64) < player_topleft.x) ? 2 : -2
		);
		return;
	}
	if(boss_phase_frame == 70) {
		snd_se_play(9);
		patnum_2064E = 131;
		return;
	}
	if(boss_phase_frame < 100) {
		return;
	}
	if(boss_phase_frame == 100) {
		snd_se_play(10);
		patnum_2064E = 134;
		mima_star_angle = iatan2(
			(player_topleft.y - top_26C62), (player_topleft.x - left_26C5A)
		);
		// Spelled as the LEFT half's condition, which is what puts the +1 into
		// the branch and the -1 into the fall-through.
		mima_star_direction = (
			(player_topleft.x <= PLAYER_LEFT_START) ? -1 : 1
		);
		bullet_special.u3.turns_max = mima_all_patterns;
		return;
	}
	if(boss_phase_frame <= 300) {
		if((boss_phase_frame % 8) == 0) {
			snd_se_play(10);
			if(rank != RANK_EASY) {
				mima_star_add(mima_star_angle + 0x0A);
				mima_star_add(mima_star_angle - 0x0A);
				mima_star_add(mima_star_angle + 0x1E);
				mima_star_add(mima_star_angle - 0x1E);
			} else {
				mima_star_add(mima_star_angle + 0x0F);
				mima_star_add(mima_star_angle - 0x0F);
			}
		}
		if(boss_phase_frame < 150) {
			return;
		}
		if(!(boss_phase_frame & 1)) {
			return;
		}
		if(boss_phase_frame < 210) {
			mima_star_angle += mima_star_direction;
			return;
		}
		mima_star_angle -= mima_star_direction;
		return;
	}
	boss_phase_frame = 0;
	patnum_2064E = 128;
}


// Her symmetric pellet fan: the same 60-frame approach, then a mirrored pair
// of narrow 3-spreads every 10th frame from phase frame 120, with the angle
// walking 10 steps per pair.
extern "C" void near mima_18DE0(void)
{
	if(boss_phase_frame < 30) {
		return;
	}
	if(boss_phase_frame < 70) {
		patnum_2064E = 128;
		*boss_left_on_back_page += (
			((*boss_left_on_back_page + 64) < player_topleft.x) ? 2 : -2
		);
		return;
	}
	if(boss_phase_frame == 70) {
		snd_se_play(9);
		patnum_2064E = 131;
		return;
	}
	if(boss_phase_frame < 120) {
		return;
	}
	// Written out in both arms rather than after the `if`: the original falls
	// straight from the frame-120 arm into this block and jumps BACKWARD into
	// it from the other, which is Turbo C++ 4.02's -O cross-jump on two
	// identical tails. Hoisting it below the `if` puts it after the whole
	// chain and turns the first arm's fall-through into a forward jump.
	if(boss_phase_frame == 120) {
		snd_se_play(3);
		patnum_2064E = 134;
		mima_fan_angle = 0x04;
		mima_fan_fire();
		return;
	} else if(boss_phase_frame <= 160) {
		if((boss_phase_frame % 10) != 0) {
			return;
		}
		snd_se_play(3);
		mima_fan_fire();
		return;
	}
	boss_phase_frame = 0;
	patnum_2064E = 128;
}


// Her orb pattern proper: four orbs seeded on a square around her at phase
// frame 80, expanding by 2 a frame and counter-rotating, each firing a pellet
// outward every 4th frame once the formation is wide enough for the rank.
// Retires at radius 460.
extern "C" void near mima_18EB8(void)
{
	int i;
	unsigned char pellet_angle;
	unsigned char angle;

	if(boss_phase_frame < 40) {
		return;
	}
	if(boss_phase_frame == 40) {
		snd_se_play(9);
		patnum_2064E = 131;
		mima_orb_center_x = (*boss_left_on_back_page + 80);
		mima_orb_center_y = (*boss_top_on_back_page + 80);
		mima_orb_radius = 4;
		mima_orb_angle = randring2_next8();
	} else if(boss_phase_frame < 80) {
		return;
	} else if(boss_phase_frame == 80) {
		snd_se_play(10);
		patnum_2064E = 134;
		for(
			i = 0, angle = mima_orb_angle;
			i < MIMA_ORB_COUNT;
			i++, angle = (angle + MIMA_ORB_ANGLE_STEP)
		) {
			mima_orb_left_on_page[0][i] = mima_orb_left_on_page[1][i] = (
				(((long)(mima_orb_radius) * CosTable8[angle]) >> 8) +
				(mima_orb_center_x >> 4)
			);
			mima_orb_top_on_page[0][i] = mima_orb_top_on_page[1][i] = (
				(((long)(mima_orb_radius) * SinTable8[angle]) >> 8) +
				(mima_orb_center_y >> 4)
			);
			mima_orb_flag[i] = 1;
		}
	} else {
		mima_orb_radius += 2;
		for(
			i = 0, angle = mima_orb_angle;
			i < MIMA_ORB_COUNT;
			i++, angle = (angle + MIMA_ORB_ANGLE_STEP)
		) {
			mima_orb_left_on_page[page_back][i] = (
				(((long)(mima_orb_radius) * CosTable8[angle]) >> 8) +
				mima_orb_center_x
			);
			mima_orb_top_on_page[page_back][i] = (
				(((long)(mima_orb_radius) * SinTable8[angle]) >> 8) +
				mima_orb_center_y
			);
			// ZUN quirk: the sound effect plays before the four bounds tests,
			// so an orb that has left the screen is still audible.
			if(
				((180 - (rank << 4)) < mima_orb_radius) &&
				!(boss_phase_frame & 3)
			) {
				snd_se_play(3);
				if(
					(mima_orb_left_on_page[page_back][i] > 0) &&
					(mima_orb_left_on_page[page_back][i] < PLAYFIELD_RIGHT) &&
					(mima_orb_top_on_page[page_back][i] > 0) &&
					(mima_orb_top_on_page[page_back][i] < RES_Y)
				) {
					if(!mima_all_patterns) {
						pellet_angle = ((i << 6) + mima_orb_angle + 0x40);
					} else {
						pellet_angle = ((i << 6) + mima_orb_angle + 0x60);
					}
					bullets_add_pellet(
						mima_orb_left_on_page[page_back][i],
						mima_orb_top_on_page[page_back][i],
						pellet_angle,
						BG_1,
						(8 << 4)
					);
				}
			}
		}
		if(mima_orb_radius > 460) {
			for(i = 0; i < MIMA_ORB_COUNT; i++) {
				mima_orb_flag[i] = 0;
			}
			boss_phase_frame = 0;
			patnum_2064E = 128;
		}
	}
	mima_orb_angle = (mima_orb_angle - 3);
}


// The first of her three step-9 patterns: a symmetric spread every other frame
// between phase frames 50 and 150, with the angle walking by
// boss_rank_param[4] each time. The block-order shapes below are the same two
// mima_19353() needed - the restart FOLLOWS the window if-statement, and every
// arm ends in an explicit return statement.
static void near mima_19173(void)
{
	if(boss_phase_frame < MIMA_SPIRAL_FIRST_FRAME) {
		return;
	}
	if(boss_phase_frame == MIMA_SPIRAL_FIRST_FRAME) {
		mima_spiral_angle = 0;
		return;
	}
	if(boss_phase_frame < MIMA_SPIRAL_LAST_FRAME) {
		if((boss_phase_frame & 1) != 0) {
			snd_se_play(3);
			bullets_add_16x16(
				x_26C5C,
				y_26C64,
				mima_spiral_angle,
				BG_2_SPREAD_HORIZONTALLY_SYMMETRIC,
				PAT_BULLET16_BALL,
				((3 << 4) + 2)
			);
			mima_spiral_angle += boss_rank_param[4];
		}
		return;
	}
	boss_phase_frame = 0;
}


// The second: a charge-up that shrinks three concentric rings around her for 30
// frames, then sprays randomly aimed bullets and pellets for another 120.
static void near mima_191CC(void)
{
	if(boss_phase_frame < MIMA_CHARGE_FIRST_FRAME) {
		return;
	}
	if(boss_phase_frame == MIMA_CHARGE_FIRST_FRAME) {
		// 12 on both axes, which is neither the player's center
		// ((PLAYER_W / 2) is 16, (PLAYER_H / 2) is 24) nor any other named
		// offset in this game - so it stays a literal, the way
		// th02/main/player/shot.cpp keeps its own aiming offsets.
		mima_aim_angle_unused = iatan2(
			((player_topleft.y + 12) - y_26C64),
			((player_topleft.x + 12) - x_26C5C)
		);
		mima_charge_ring_radius = MIMA_CHARGE_RING_RADIUS_INITIAL;
		snd_se_play(9);
		return;
	}
	if(boss_phase_frame <= MIMA_CHARGE_LAST_FRAME) {
		if((boss_phase_frame & 1) == 0) {
			return;
		}
		// Unblit at the old radius, shrink, blit at the new one - except on
		// the last frame, which only unblits.
		grcg_setcolor(GC_RMW, 0);
		grcg_circle(x_26C5C, y_26C64, mima_charge_ring_radius);
		grcg_circle(x_26C5C, y_26C64, (mima_charge_ring_radius * 2));
		grcg_circle(x_26C5C, y_26C64, (mima_charge_ring_radius * 4));
		mima_charge_ring_radius -= 2;
		if(boss_phase_frame == MIMA_CHARGE_LAST_FRAME) {
			return;
		}
		grcg_setcolor(GC_RMW, 13);
		grcg_circle(x_26C5C, y_26C64, mima_charge_ring_radius);
		grcg_circle(x_26C5C, y_26C64, (mima_charge_ring_radius * 2));
		grcg_circle(x_26C5C, y_26C64, (mima_charge_ring_radius * 4));
		grcg_off();
		return;
	}
	if(boss_phase_frame < MIMA_SPRAY_LAST_FRAME) {
		if((boss_phase_frame & 7) == 0) {
			bullets_add_16x16(
				x_26C5C,
				y_26C64,
				2,
				BG_RANDOM_ANGLE_AND_SPEED,
				PAT_BULLET16_BALL,
				((3 << 4) + 12)
			);
			bullets_add_pellet(
				x_26C5C, y_26C64, 2, BG_RANDOM_ANGLE_AND_SPEED, ((3 << 4) + 12)
			);
		}
		// A second spray on the higher ranks, offset by half the interval.
		if((rank > 0) && ((boss_phase_frame & 7) == 4)) {
			bullets_add_16x16(
				x_26C5C,
				y_26C64,
				2,
				BG_RANDOM_ANGLE_AND_SPEED,
				PAT_BULLET16_BALL,
				((3 << 4) + 12)
			);
			bullets_add_pellet(
				x_26C5C, y_26C64, 2, BG_RANDOM_ANGLE_AND_SPEED, ((3 << 4) + 12)
			);
		}
		if((boss_phase_frame & 7) == 0) {
			snd_se_play(3);
		}
		return;
	}
	boss_phase_frame = 0;
}


// The third of her step-9 patterns: a single aimed spread every 16 frames, on
// a 32-frame cycle, alternating between the 5-bullet and the 4-bullet group.
// The pattern only fires between phase frames 50 and 190, and restarts the
// phase at 190.
static void near mima_19353(void)
{
	if(boss_phase_frame < MIMA_SPREAD_FIRST_FRAME) {
		return;
	}
	// `[measured]` The window is the OUTER if-statement, the restart is what
	// follows it, and each spread arm ends in an explicit return statement.
	// Both are load-bearing: written as an early return, the restart lands
	// ABOVE the spreads instead of between them and the epilogue; and without
	// the explicit return in the first arm, -O hosts the two calls' merged
	// tail at that arm and makes the second jump BACKWARD into it. Five other
	// spellings of the same control flow were screened; only this one gives
	// the original's block order. (kb/codegen/0152)
	if(boss_phase_frame < MIMA_SPREAD_LAST_FRAME) {
		if((boss_phase_frame & (MIMA_SPREAD_INTERVAL - 1)) == 0) {
			bullets_add_16x16(
				x_26C5C,
				y_26C64,
				0,
				BG_5_SPREAD_MEDIUM_AIMED,
				PAT_BULLET16_BALL,
				((5 << 4) + 10)
			);
			return;
		}
		if(
			(boss_phase_frame & (MIMA_SPREAD_INTERVAL - 1)) ==
			(MIMA_SPREAD_INTERVAL / 2)
		) {
			bullets_add_16x16(
				x_26C5C,
				y_26C64,
				0,
				BG_4_SPREAD_MEDIUM_AIMED,
				PAT_BULLET16_BALL,
				((5 << 4) + 10)
			);
		}
		return;
	}
	boss_phase_frame = 0;
}


// Ends the current pattern: advances the step if it is over, and otherwise
// picks the next pattern and re-rolls Mima's vertical drift.
static void near mima_193A4(void)
{
	register int pattern_next;

	if(boss_phase_frame == 0) {
		if(
			(boss_damage >= mima_phase_damage_max) ||
			(mima_patterns_this_phase > mima_patterns_max)
		) {
			mima_phase++;
			mima_damage_multiplier = 0;
			mima_patterns_this_phase = 0;
		} else {
			mima_patterns_this_phase++;
			if(mima_patterns_this_phase >= mima_patterns_until_vulnerable) {
				mima_damage_multiplier = 1;
			}
			// The `+ 1` is one expression in each arm, because the original
			// computes it in AX and moves it to SI - the opposite of
			// kb/codegen/0146's split-statement case.
			if(!mima_all_patterns) {
				pattern_next = (mima_pattern + 1);
				if(pattern_next >= mima_pattern_count) {
					pattern_next = 0;
				}
			} else {
				pattern_next = (mima_pattern + 1);
				if(pattern_next > 7) {
					pattern_next = 0;
				}
			}
			mima_pattern = pattern_next;
			if(*boss_top_on_back_page < MIMA_TOP_MIN) {
				mima_velocity_y = 1;
			} else if(*boss_top_on_back_page > MIMA_TOP_MAX) {
				mima_velocity_y = -1;
			} else {
				mima_velocity_y = (1 - (randring2_next8() % 3));
			}
		}
	}
	if(boss_phase_frame < MIMA_DRIFT_FRAMES) {
		*boss_top_on_back_page += mima_velocity_y;
	}
}


// Her per-frame update. Installed into [boss_update] by stage_init().
extern "C" int far mima_update(void)
{
	// [radius] first: Turbo C++ allocates stack locals in declaration order,
	// and it is the original's only one, at [bp-1]. (kb/codegen/0010)
	unsigned char radius;
	register int i;

	boss_phase_frame++;
	boss_pos_x = (*boss_left_on_back_page + 72);
	boss_pos_y = (*boss_top_on_back_page + 56);
	left_26C56 = (*boss_left_on_back_page + 32);
	mima_muzzle_left = (*boss_left_on_back_page + 40);
	left_26C5A = (*boss_left_on_back_page + 64);
	x_26C5C = (*boss_left_on_back_page + 64);
	top_26C5E = (*boss_top_on_back_page + 96);
	mima_muzzle_top = (*boss_top_on_back_page + 16);
	top_26C62 = (*boss_top_on_back_page + 114);
	y_26C64 = (*boss_top_on_back_page + 44);
	if((stage_frame & 1) == 0) {
		if(!reduce_effects) {
			mima_ring_radius += 8;
			grcg_setcolor(GC_RMW, 3);

			// [i] is initialized ahead of [radius], and the increment is a
			// statement of its own rather than a loop-header increment,
			// because that is what puts `inc si` ahead of the radius update
			// and lets -O merge both stores to [radius] into the shared test
			// block, as the original has them.
			i = 0;
			radius = mima_ring_radius;
			while(i < MIMA_RING_COUNT) {
				dot_square_ring_put(
					MIMA_RING_CENTER_X,
					MIMA_RING_CENTER_Y,
					radius,
					MIMA_RING_ANGLE_STEP
				);
				i++;
				radius += MIMA_RING_DISTANCE;
			}
			grcg_off();
		}
		if(mima_phase & 1) {
			mima_bg_circle_pulse_frame++;
			if(mima_bg_circle_pulse_frame >= MIMA_CIRCLE_PULSE_FRAMES) {
				mima_bg_circle_pulse_frame = 0;
			}
			mima_bg_circle_radius = (
				mima_bg_circle_radius_base - (mima_bg_circle_pulse_frame / 4)
			);
		}
	}

	if(mima_phase == 0) {
		mima_bg_ring_radius = (boss_phase_frame / 2);
		mima_bg_circle_radius = (boss_phase_frame / 3);
		Palettes[0].c.r = (boss_phase_frame >> 1);
		Palettes[0].c.g = 0;
		Palettes[0].c.b = (boss_phase_frame >> 1);
		palette_show();
		if(boss_phase_frame > MIMA_CHARGE_FRAMES) {
			mima_bg_circle_radius_base = mima_bg_circle_radius;
			mima_bg_circle_pulse_frame = 0;
			mima_phase = 1;
			boss_phase_frame = 0;
			mima_pattern = (randring2_next8() % 3);
			mima_patterns_this_phase = 0;
			mima_phase_damage_max = 600;
			mima_patterns_until_vulnerable = 2;
			mima_patterns_max = 10;
			mima_pattern_count = 3;
			boss_damage = 0;
		}
	} else if(mima_phase == 1) {
		switch(mima_pattern) {
		case 0:
			mima_180EC();
			break;
		case 1:
			mima_181B3();
			break;
		case 2:
			mima_188AA();
			break;
		}
		mima_193A4();
	} else if(mima_phase == 2) {
		mima_bg_ring_radius = ((boss_phase_frame / 4) + 50);
		mima_bg_circle_radius = ((boss_phase_frame / 5) + 30);
		mima_18BA6();
		Palettes[0].c.r = (51 - (boss_phase_frame >> 1));
		Palettes[0].c.g = (boss_phase_frame >> 1);
		Palettes[0].c.b = 50;
		palette_show();
		if(boss_phase_frame > MIMA_CHARGE_FRAMES) {
			mima_phase = 3;
			boss_phase_frame = 0;
			mima_pattern = (randring2_next8() % 3);
			mima_patterns_this_phase = 0;
			mima_bg_circle_radius_base = mima_bg_circle_radius;
			mima_phase_damage_max = 700;
			mima_patterns_until_vulnerable = 2;
			mima_patterns_max = 12;
			mima_pattern_count = 3;
			boss_damage = 0;
		}
	} else if(mima_phase == 3) {
		switch(mima_pattern) {
		case 0:
			mima_18905();
			break;
		case 1:
			mima_18A1B();
			break;
		case 2:
			mima_18B4B();
			break;
		}
		mima_193A4();
	} else if(mima_phase == 4) {
		mima_bg_ring_radius = ((boss_phase_frame / 4) + 75);
		mima_bg_circle_radius = ((boss_phase_frame / 5) + 50);
		mima_18BA6();
		Palettes[0].c.r = (boss_phase_frame >> 1);
		Palettes[0].c.g = (51 - (boss_phase_frame >> 1));
		Palettes[0].c.b = (51 - (boss_phase_frame >> 1));
		palette_show();
		if(boss_phase_frame > MIMA_CHARGE_FRAMES) {
			mima_phase = 5;
			boss_phase_frame = 0;
			mima_pattern = (randring2_next8() % 3);
			mima_bg_circle_radius_base = mima_bg_circle_radius;
			mima_patterns_this_phase = 0;
			mima_phase_damage_max = 800;
			mima_patterns_until_vulnerable = 2;
			mima_patterns_max = 12;
			mima_pattern_count = 3;
			boss_damage = 0;
		}
	} else if(mima_phase == 5) {
		switch(mima_pattern) {
		case 0:
			mima_18C4A();
			break;
		case 1:
			mima_18DE0();
			break;
		case 2:
			mima_18EB8();
			break;
		}
		mima_193A4();
	} else if(mima_phase == 6) {
		mima_bg_ring_radius = ((boss_phase_frame / 4) + 100);
		mima_bg_circle_radius = ((boss_phase_frame / 5) + 70);
		mima_18BA6();
		Palettes[0].c.r = (51 - (boss_phase_frame >> 1));
		Palettes[0].c.g = 0;
		Palettes[0].c.b = 0;
		palette_show();
		if(boss_phase_frame > MIMA_CHARGE_FRAMES) {
			mima_bg_circle_radius_base = mima_bg_circle_radius;
			mima_phase = 7;
			boss_phase_frame = 0;
			mima_pattern = randring2_next8_and(7);
			mima_patterns_this_phase = 0;
			mima_phase_damage_max = 1500;
			mima_patterns_until_vulnerable = 3;
			mima_patterns_max = 200;
			mima_pattern_count = 9;
			mima_all_patterns = 1;
			boss_damage = 0;
			mima_bg_ring_col_tail = 2;
		}
	} else if(mima_phase == 7) {
		switch(mima_pattern) {
		case 0:
			mima_18C4A();
			break;
		case 1:
			mima_188AA();
			break;
		case 2:
			mima_18905();
			break;
		case 3:
			mima_181B3();
			break;
		case 4:
			mima_18A1B();
			break;
		case 5:
			mima_18B4B();
			break;
		case 6:
			mima_180EC();
			break;
		case 7:
			mima_18DE0();
			break;
		case 8:
			mima_18EB8();
			break;
		}
		mima_193A4();
	} else if(mima_phase == 9) {
		switch(mima_pattern) {
		case 0:
			mima_19173();
			break;
		case 1:
			mima_191CC();
			break;
		case 2:
			mima_19353();
			break;
		}
		mima_193A4();
		if(boss_phase_frame == 1) {
			*boss_top_on_back_page = 64;
		}
		if(boss_phase_frame < 30) {
			*boss_left_on_back_page += (
				(x_26C5C < player_topleft.x) ? 1 : -1
			);
		}
	}

	mima_17C92();
	mima_17F27();
	mima_17D59();
	if(mima_phase == 8) {
		if(!mima_19C8D()) {
			mima_phase++;
			patnum_2064E = 128;
			mima_patterns_this_phase = 0;
			boss_phase_frame = 0;
			mima_pattern = (randring2_next8() % 3);
			mima_patterns_this_phase = 0;
			boss_damage = 0;
			mima_phase_damage_max = 1100;
			mima_patterns_until_vulnerable = 2;
			mima_patterns_max = 200;
			mima_pattern_count = 3;
			mima_bg_circle_col = 1;
			mima_bg_ring_col_tail = 2;
			mima_bg_ring_radius = 200;
			mima_bg_circle_radius = 150;
			mima_bg_circle_radius_base = mima_bg_circle_radius;
		}
	} else if(mima_phase == 10) {
		nopcall_same_group(mima_end);
	}
	// Through an `int` rather than [stage_progression_t]: -b- sizes a
	// three-value enum as a `char`, and the original returns in AX.
	// th02/main/boss/b4.cpp declares marisa_update() the same way.
	return SP_BOSS;
}
