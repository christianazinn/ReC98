/// Foreground rendering code for TH04's Stage 4 and Extra Stage bosses
/// -------------------------------------------------------------------
/// (#included from th04/boss_fg.cpp. ZUN's object for this code segment held
/// bullets_render(), reimu_orbs_render(), reimu_fg_render() and finally
/// gengetsu_fg_render(), in that address order; that an original object held
/// several unrelated sources is kb/codegen/0112. This file is appended to
/// that object's dump contribution and grows upwards, one tail at a time.)
///
/// Reimu keeps the three-way [boss_fg_render] contract that
/// th04/main/boss/render.cpp documents for Orange and Kurumi, with two
/// additions: a one-frame afterimage of her previous position while
/// [reimu_afterimage] is set, and her orbs, which she is the only TH04 boss
/// to have. Unlike Orange and Kurumi she *does* reset
/// [boss.damage_this_frame] after the white flash.
///
/// Gengetsu keeps that contract too, but is the only boss to differ in all
/// four of these ways at once:
///
/// • She is two sprites wide. Every blit is a pair, GENGETSU_W / 2 pixels
///   apart, with [boss.sprite] and its successor — which is why her sprite
///   box is GENGETSU_W × GENGETSU_H rather than BOSS_W × BOSS_H.
/// • The teleport animation replaces the blit entirely, through master.lib's
///   super_wave_put(). It is selected by [gengetsu_wave_amp] alone, not by a
///   phase, so it can interrupt any frame of the fight.
/// • The white damage flash only fires on every *other* damage frame, via
///   [gengetsu_damage_frames]. mugetsu_fg_render() does the same thing with
///   its own counter; no other TH04 boss halves the flash rate.
/// • There are no entrance circles, and no entrance branch at all — a zero
///   [boss.sprite] skips the whole blit instead.
///
/// Like yuuka5_fg_render() (th04/main/boss/b5r.cpp), it renders the thick
/// lasers after the explosions, and it ends by telegraphing her column
/// bullet spawn lines.

#include "platform.h"
#include "pc98.h"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/v_colors.hpp"
#include "th04/hardware/grcg.hpp"
#include "th04/main/frames.h"
#include "th04/main/boss/boss.hpp"
#include "th04/main/boss/bosses.hpp"
#include "th04/main/boss/b4r.hpp"
#include "th04/main/boss/bx2.hpp"
#include "th04/main/bullet/laser_t.hpp"
#include "th04/sprites/main_pat.h"

// Blits the barrier sprite pair over both Extra Stage bosses while a player
// bomb keeps them invincible. Defined in th04/main/boss/shield.cpp, which is
// a different code segment (main_01_TEXT) but the same group, so the call
// stays near. `extern "C"` because the mangled spelling would be 35
// characters (kb/codegen/0123).
extern "C" void near mugetsu_gengetsu_shield_render(void);

/// Still ASM
/// ---------
// Counts the frames on which Gengetsu took damage, and nothing else — its
// parity is the only thing ever read out of it, and this function is its only
// writer. A th04_main.asm `.data?` label with no `public` of ZUN's, so it
// needed a zero-byte `label` alias to become linkable (kb/codegen/0123).
// [inferred] name.
extern "C" unsigned char gengetsu_damage_frames;

// While set, reimu_fg_render() blits one extra copy of the boss sprite at
// [boss.pos.prev] in a single color, i.e. a one-frame motion trail. Set on
// frame 1 of two of Reimu's attack patterns and cleared when each ends, so it
// covers exactly the patterns that move her fast enough for the trail to be
// visible. A th04_main.asm `.data?` label with no `public` of ZUN's
// (kb/codegen/0123). [inferred] name.
extern "C" unsigned char reimu_afterimage;
/// ---------

/// Stage 4 Boss - Reimu
/// --------------------

// Reimu's cels are absolute patnums, like Orange's: stage4_setup() seeds
// [boss.sprite] with PAT_STAGE rather than with 0, the way stage2_setup()
// does for Kurumi.
static const int REIMU_FRAMES_PER_CEL = 4;

// The color the afterimage is blitted in. Not V_WHITE, which is what the
// damage flash below uses; this is master.lib's GC_BI plane pair.
static const vc_t REIMU_AFTERIMAGE_COL = 9;

// The orb cel animation runs at half the speed of its 8-frame cycle, giving
// REIMU_ORB_CELS cels of 2 frames each.
static const int REIMU_ORB_CYCLE_FRAMES = (REIMU_ORB_CELS * 2);

// Blits every orb that is alive and has entered the playfield from the top.
// Each orb's cel is offset by its own index, so a ring of them ripples rather
// than pulsing in lockstep.
void near reimu_orbs_render(void)
{
	// [bp-2], [bp-4] and [bp-6] in declaration order. (kb/codegen/0010)
	screen_x_t x;
	vram_y_t y;
	int patnum;

	// The two register variables: the orb pointer earns SI by how often it is
	// dereferenced as a base (kb/codegen/0117), the counter takes DI.
	reimu_orb_t near *orb = reimu_orbs;
	int i;

	for(i = 0; i < REIMU_ORB_COUNT; i++, orb++) {
		if(orb->flag == OF_FREE) {
			continue;
		}

		// Orbs are spawned above the playfield and fly in, so this skips the
		// ones that would still be blitted entirely off its top edge.
		if(orb->center.y <= -TO_SP(REIMU_ORB_H / 2)) {
			continue;
		}

		x = orb->center.to_screen_left(REIMU_ORB_W);
		y = orb->center.to_screen_top(REIMU_ORB_H);
		patnum = (
			orb_patnum_base +
			(((stage_frame + i) % REIMU_ORB_CYCLE_FRAMES) / 2)
		);
		super_roll_put(x, y, patnum);
	}
}

void pascal near reimu_fg_render(void)
{
	// Declared first so that it takes the original's single [bp-2] stack
	// slot; the two coordinates below are the register variables.
	// (kb/codegen/0010)
	int patnum;

	screen_x_t left;
	vram_y_t top;

	if(boss.phase < PHASE_EXPLODE_BIG) {
		// The afterimage is the *previous* position, so it can't reuse the
		// coordinates computed for the blit below — ZUN computes both.
		if(reimu_afterimage) {
			left = boss.pos.prev.to_screen_left(BOSS_W);
			top = boss.pos.prev.to_screen_top(BOSS_H);
			super_put_1plane(
				left, top, boss.sprite, 0, super_plane(REIMU_AFTERIMAGE_COL)
			);
		}
		left = boss.pos.cur.to_screen_left(BOSS_W);
		top = boss.pos.cur.to_screen_top(BOSS_H);

		// One cel range animates; every other [boss.sprite] value is a single
		// pose that is blitted as-is.
		if(boss.sprite == PAT_REIMU_ANIMATED) {
			patnum = (
				(stage_frame_mod16 / REIMU_FRAMES_PER_CEL) + PAT_REIMU_ANIMATED
			);
		} else {
			patnum = boss.sprite;
		}

		if(boss.damage_this_frame == 0) {
			super_put(left, top, patnum);
		} else {
			super_put_1plane(left, top, patnum, 0, super_plane(V_WHITE));
			boss.damage_this_frame = 0;
		}
		reimu_orbs_render();
	} else if(boss.phase == PHASE_EXPLODE_BIG) {
		left = boss.pos.cur.to_screen_left(BOSS_W);
		top = boss.pos.cur.to_screen_top(BOSS_H);
		super_large_put(left, top, boss.sprite);
	}
	explosions_small_update_and_render();
	explosions_big_update_and_render();
}
/// --------------------

/// Teleport animation
/// ------------------
// super_wave_put()'s [len] is the wavelength in pixels, so a *rising*
// amplitude also tightens the wave.
static const int GENGETSU_WAVE_LEN_MAX = 80;

// Added to [boss.angle] per rendered frame, which doubles as the wave phase
// for the whole animation.
// `int` rather than the `unsigned char` this really is, on purpose: an
// int-typed constant forces the AL round trip the original has, where a
// byte-typed one would fold into `ADD byte ptr [mem], 4`. (kb/codegen/0094)
static const int GENGETSU_WAVE_PHASE_SPEED = 4;
/// ------------------

// Same zoom level as Yuuka's (th04/main/boss/b5r.cpp).
static const int GENGETSU_EXPLODE_ZOOM = 3;

/// Column bullet spawn lines
/// -------------------------
// [inferred] The lines are telegraphed for the 64 frames in the middle of
// this one (phase, mode) pair; the original only shows the comparisons.
static const unsigned char GENGETSU_PHASE_SPAWNCOLUMNS = 5;
static const unsigned char GENGETSU_MODE_SPAWNCOLUMNS = 1;

static const int GENGETSU_SPAWNCOLUMN_FIRST_FRAME = 32;
static const int GENGETSU_SPAWNCOLUMN_LAST_FRAME = 96;

// Alternated every other frame to make the lines blink.
static const vc_t GENGETSU_SPAWNCOLUMN_COL = 9;
/// -------------------------

void pascal near gengetsu_fg_render(void)
{
	gengetsu_spawncolumn_t near *spawncolumn;

	// The two register variables, and the only two: the original's frame is
	// `ENTER 2, 0`, with [spawncolumn] as its single stack slot. That leaves
	// no room for a separate spawn line loop counter, so [top] doubles as one
	// — the same one-slot reuse that kurumi_fg_render() does with [patnum]
	// and its entrance circle radius. [left] is reassigned inside that loop
	// as well, which is why neither can be recomputed per use.
	screen_y_t top;
	screen_x_t left;

	if(boss.sprite != 0) {
		left = boss.pos.cur.to_screen_left(GENGETSU_W);
		top = boss.pos.cur.to_screen_top(GENGETSU_H);
		if(boss.phase < PHASE_EXPLODE_BIG) {
			if(gengetsu_wave_amp != 0) {
				super_wave_put(
					left, top, boss.sprite,
					(GENGETSU_WAVE_LEN_MAX - gengetsu_wave_amp),
					gengetsu_wave_amp, boss.angle
				);
				super_wave_put(
					(left + (GENGETSU_W / 2)), top, (boss.sprite + 1),
					(GENGETSU_WAVE_LEN_MAX - gengetsu_wave_amp),
					gengetsu_wave_amp, boss.angle
				);
				boss.angle += GENGETSU_WAVE_PHASE_SPEED;
			} else if(boss.damage_this_frame == 0) {
				super_put(left, top, boss.sprite);
				super_put((left + (GENGETSU_W / 2)), top, (boss.sprite + 1));
				mugetsu_gengetsu_shield_render();
			} else {
				gengetsu_damage_frames++;
				if(gengetsu_damage_frames & 1) {
					super_put(left, top, boss.sprite);
					super_put(
						(left + (GENGETSU_W / 2)), top, (boss.sprite + 1)
					);
				} else {
					super_put_1plane(
						left, top, boss.sprite, 0, super_plane(V_WHITE)
					);
					super_put_1plane(
						(left + (GENGETSU_W / 2)), top, (boss.sprite + 1), 0,
						super_plane(V_WHITE)
					);
				}
				boss.damage_this_frame = 0;
			}
		} else if(boss.phase == PHASE_EXPLODE_BIG) {
			super_zoom(left, top, boss.sprite, GENGETSU_EXPLODE_ZOOM);
		}
	}
	explosions_small_update_and_render();
	explosions_big_update_and_render();
	thicklasers_render();

	if(
		(boss.phase == GENGETSU_PHASE_SPAWNCOLUMNS) &&
		(boss.mode == GENGETSU_MODE_SPAWNCOLUMNS) &&
		(boss.phase_frame >= GENGETSU_SPAWNCOLUMN_FIRST_FRAME) &&
		(boss.phase_frame < GENGETSU_SPAWNCOLUMN_LAST_FRAME)
	) {
		grcg_setmode_rmw();

		// Two calls, not one with a ternary color: `-O` cross-jumping merges
		// the shared grcg_setcolor_direct_raw() tail and leaves exactly the
		// original's `MOV AH, imm8` / `JMP` / `MOV AH, imm8` island
		// (kb/codegen/0097). A ternary would materialize the color in AL
		// first and cost the extra `MOV AH, AL` (kb/codegen/0120).
		if(stage_frame_mod2) {
			grcg_setcolor_direct(GENGETSU_SPAWNCOLUMN_COL);
		} else {
			grcg_setcolor_direct(V_WHITE);
		}
		spawncolumn = gengetsu_spawncolumns;
		for(top = 0; top < GENGETSU_SPAWNCOLUMN_COUNT; top++, spawncolumn++) {
			// A division, not a TO_PIXEL() shift, exactly as in
			// kurumi_fg_render()'s spawn ray loop: the original really does
			// emit `MOV BX, SUBPIXEL_FACTOR` / `CWD` / `IDIV BX` here.
			left = ((spawncolumn->pos.x / SUBPIXEL_FACTOR) + PLAYFIELD_LEFT);
			grcg_vline(left, PLAYFIELD_TOP, (PLAYFIELD_BOTTOM - 1));
		}
	}
}
