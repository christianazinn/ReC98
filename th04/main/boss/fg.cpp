/// Foreground rendering code for TH04's Extra Stage boss
/// -----------------------------------------------------
/// (#included from th04/boss_fg.cpp. ZUN's object for this code segment held
/// bullets_render(), a bullet helper, reimu_fg_render() and finally this
/// function; that an original object held several unrelated sources is
/// kb/codegen/0112. This file is appended to that object's dump contribution
/// and grows upwards, one tail at a time.)
///
/// Gengetsu keeps the [boss_fg_render] contract that
/// th04/main/boss/render.cpp documents for Orange and Kurumi, but is the only
/// boss to differ in all four of these ways at once:
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
#include "th04/main/boss/bx2.hpp"
#include "th04/main/bullet/laser_t.hpp"

/// Still ASM
/// ---------
// Blits the barrier sprite pair over both Extra Stage bosses while a player
// bomb keeps them invincible. th04_main.asm's sub_11647 in main_01_TEXT,
// published under this name for its one C++ caller; mugetsu_fg_render() is
// still ASM and keeps the dump's spelling.
//
// [inferred] name: the byte it branches on is set to 32 by both
// mugetsu_update() and gengetsu_update() on every frame [bombing] is nonzero
// and decremented from there, and both bosses' hittest functions swap
// boss_hittest_shots() for the invincibility-sound variant for exactly as
// long as it stays nonzero. So the sprite pair it draws is the visual for
// that window. What the sprites themselves depict is not recoverable from the
// binary; the evidence is in state/notes/_gengetsu_fg_render_qv.md.
extern "C" void near mugetsu_gengetsu_shield_render(void);

// Counts the frames on which Gengetsu took damage, and nothing else — its
// parity is the only thing ever read out of it, and this function is its only
// writer. A th04_main.asm `.data?` label with no `public` of ZUN's, so it
// needed a zero-byte `label` alias to become linkable (kb/codegen/0123).
// [inferred] name.
extern "C" unsigned char gengetsu_damage_frames;
/// ---------

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
