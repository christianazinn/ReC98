/// Stage 4 Boss - Marisa, foreground rendering
/// -------------------------------------------
/// (#included from th04/main/hud/points.cpp, at the very FRONT of it, ahead of
/// th04/main/boss/b1_cfill.cpp, th04/main/score_extend.cpp, score_reset.cpp,
/// lives.cpp, bombs.cpp and that file's own function -- the address order all
/// seven bodies have in HUD_PNT_TEXT. That an original object held several
/// unrelated sources is kb/codegen/0112; that a prepended body arrives as an
/// #include rather than as more of points.cpp is kb/codegen/0129, because that
/// translation unit's header closure has unguarded members. With these two
/// bodies and Orange's backdrop fill below them, th04_main.asm's contribution
/// to HUD_PNT_TEXT is ZERO bytes, so the object simply grows backwards into
/// the hole and every byte above it keeps its address: no carve, no new
/// segment name, no group-list edit and no Tupfile.lua line
/// (kb/codegen 0099 + 0114).
///
/// Marisa keeps the three-way [boss_fg_render] contract that
/// th04/main/boss/render.cpp documents for Orange and Kurumi, in the same
/// reduced form elly_fg_render() (th04/main/boss/b3_fg.cpp) uses: no entrance
/// branch, and no cel animation, so [boss.sprite] is blitted as-is on every
/// frame. Like Reimu, Mugetsu and Gengetsu -- and unlike Orange, Kurumi and
/// Elly -- she does reset [boss.damage_this_frame] after the white flash.
///
/// What is hers alone among the TH04 bosses: the bits, rendered by the helper
/// below. Reimu's orbs (th04/main/boss/fg.cpp) are the other per-boss entity
/// swarm a TH04 renderer walks, and the two loops are the same shape -- but
/// nothing draws lines between the orbs, and only the bits carry a per-bit
/// damage flash.

// th04/main/boss/boss.hpp reaches the unguarded th04/main/hud/overlay.hpp,
// which re-expands th04/gaiji/gaiji.h -- already in this translation unit
// through points.cpp, and a `typedef enum` rather than macros, so a second
// expansion would be a hard error (kb/codegen/0129). That is why
// th04/main/score_extend.cpp below declines to include the same header and
// spells its three declarations by hand instead. gaiji.h HAS AN INCLUDE GUARD
// NOW, added by this parcel, which discharges the blocker for the whole class.
// The guard is codegen-neutral by construction: gaiji.h defines a
// `typedef enum` at file scope, so no translation unit in the tree can be
// expanding it twice today and still compiling, and a guard therefore cannot
// suppress an expansion that happens.
#include "th02/v_colors.hpp"
#include "th04/main/boss/boss.hpp"
#include "th04/main/boss/bosses.hpp"
#include "th04/main/boss/b4m.hpp"
#include "th04/hardware/grcg.hpp"

// th04/main/boss/b4m.cpp's own two constants, repeated because that file is a
// different segment and therefore a different translation unit. [inferred] the
// same way they are there: only the folded constants survive into the binary,
// so the split between "sprite box" and "extra offset" is not recoverable.
// Reading the whole of `PLAYFIELD_LEFT - (BIT_W / 2)` and
// `PLAYFIELD_TOP - (BIT_H / 2)` as a sprite box needs no leftover in either
// axis and gives the bits the 32x32 box PAT_MARISA_BIT is drawn in.
static const pixel_t BIT_W = 32;
static const pixel_t BIT_H = 32;

// The color the lines between the bits are drawn in: master.lib's GC_BI plane
// pair, the same one reimu_fg_render()'s afterimage uses.
static const vc_t BIT_LINE_COL = 9;

// Draws the ring of lines that connects the live bits, then blits every bit
// that is on screen.
//
// The two halves are indexed differently, on purpose. [bit_center_*] are the
// PACKED arrays marisa_bits_update_and_hittest() (th04/main/boss/b4m_upd.cpp)
// rebuilds every frame, so the line loop walks 0 to [bits_alive] and a dead
// bit leaves no gap in the ring. The blit loop walks the [bits] slots
// themselves, because it needs each bit's own [flag], [patnum] and damage
// flag.
//
// [i] survives the first loop into the ring-closing line below it, which is
// why it is one function-scope variable rather than two loop-local ones: that
// line joins the LAST live bit back to the first, and "the last live bit" is
// only expressible as the counter the loop left behind.
static void near marisa_bits_render(void)
{
	// [bp-2] and [bp-4] in declaration order (kb/codegen/0010); the frame is
	// `ENTER 4, 0`. [bit] and [i] are the two register variables, [bit] first
	// because it earns SI by how often it is dereferenced as a base
	// (kb/codegen/0117).
	screen_x_t left;
	vram_y_t top;

	bit_t near *bit;
	int i;

	grcg_setmode_rmw();
	grcg_setcolor_direct(BIT_LINE_COL);
	for(i = 1; i < bits_alive; i++) {
		grcg_line(
			bit_center_x[i - 1], bit_center_y[i - 1],
			bit_center_x[i], bit_center_y[i]
		);
	}

	// Closes the ring, but only from three bits on: with two, this would draw
	// the single line above a second time, and with one, a point.
	if(bits_alive >= 3) {
		grcg_line(
			bit_center_x[i - 1], bit_center_y[i - 1],
			bit_center_x[0], bit_center_y[0]
		);
	}

	// The inline 3-instruction form the original has here rather than
	// master.lib's GRCG_OFF: `MOV DX, 7Ch` / `MOV AL, 0` / `OUT DX, AL`. Same
	// spelling as th04/end/staff_dissolve.cpp's and
	// th04/hiscore/regist_view.cpp's grcg_off_clobbering_dx().
	outportb(0x7C, GC_OFF);

	bit = bits;
	for(i = 0; i < BIT_COUNT; i++, bit++) {
		if(bit->flag == BF_FREE) {
			continue;
		}

		// ZUN bug: Clipped at the right and bottom edges 16 pixels too early.
		// The four bounds are spelled out rather than reached through
		// th02/main/playfld.hpp's playfield_encloses(), which is this same
		// comparison done correctly: that macro adds half the sprite extent to
		// both upper bounds, and the original adds it to neither.
		if(bit->center.x <= -TO_SP(BIT_W / 2)) {
			continue;
		}
		if(bit->center.x >= TO_SP(PLAYFIELD_W)) {
			continue;
		}
		if(bit->center.y <= -TO_SP(BIT_H / 2)) {
			continue;
		}
		if(bit->center.y >= TO_SP(PLAYFIELD_H)) {
			continue;
		}

		left = bit->center.to_screen_left(BIT_W);
		top = bit->center.to_screen_top(BIT_H);
		if(bit->damage_this_frame == 0) {
			// [top] is pushed out of AX, where the assignment above left it,
			// rather than out of its stack slot: this call site is reached in
			// a straight line, so the compiler's register tracking is still
			// valid here (kb/codegen/0126). The other one is reached through a
			// label, i.e. a basic-block join, and reloads it.
			super_roll_put(left, top, bit->patnum);
		} else {
			super_roll_put_1plane(
				left, top, bit->patnum, 0, super_plane(V_WHITE)
			);
			bit->damage_this_frame = 0;
		}
	}
}

void pascal near marisa_fg_render(void)
{
	// The two register variables, and the only two: the original's prolog is
	// `PUSH BP` / `MOV BP, SP` / `PUSH SI` / `PUSH DI` with no `SUB SP` at
	// all, so this function has no stack locals for a third one to live in.
	// [left] is declared first because it takes SI. (kb/codegen/0010, 0117)
	screen_x_t left;
	vram_y_t top;

	left = boss.pos.cur.to_screen_left(BOSS_W);
	top = boss.pos.cur.to_screen_top(BOSS_H);
	if(boss.phase < PHASE_EXPLODE_BIG) {
		if(boss.damage_this_frame == 0) {
			// Pushed out of AX for the same reason as in the bit loop above
			// (kb/codegen/0126).
			super_put(left, top, boss.sprite);
		} else {
			super_put_1plane(left, top, boss.sprite, 0, super_plane(V_WHITE));
			boss.damage_this_frame = 0;
		}
		marisa_bits_render();
	} else if(boss.phase == PHASE_EXPLODE_BIG) {
		super_large_put(left, top, boss.sprite);
	}
	explosions_small_update_and_render();
	explosions_big_update_and_render();
}

// The byte of alignment padding between this function and
// @orange_backdrop_colorfill$qv, which th04_main.asm used to carry as a `db 0`
// between the two `proc`s. HUD_PNT_TEXT is `byte public`, so nothing pads it
// automatically and the byte has to be emitted here. A file-scope codestring
// lands where it stands in source order, i.e. after the function above and
// nowhere else (kb/codegen/0161); same device as
// th04/main/boss/colorfill.cpp's, with the original's byte value rather than a
// NOP.
#pragma codestring "\x00"
