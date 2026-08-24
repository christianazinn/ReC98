/// Extra Stage Boss - EX-Alice, foreground rendering
/// -------------------------------------------------
/// (#included from th05/midboss5.cpp. ZUN's object for main_0_TEXT held both
/// the Stage 5 midboss and this renderer; that an original object holds
/// several unrelated sources is kb/codegen/0112. This file is appended to the
/// front of that object's dump contribution, which is where the original had
/// it.)
///
/// EX-Alice keeps the three-way [boss_fg_render] contract that
/// th04/main/boss/fg.cpp documents for TH04's bosses, with two differences,
/// both of which are hers alone among TH05's seven:
///
/// • Her bomb-invincibility window is drawn by *flickering her own sprite*
///   between two parallel cel sets EXALICE_INVINCIBLE_CELS apart, rather than
///   by a separate shield blit the way TH04's Extra bosses do
///   (mugetsu_gengetsu_shield_render()).
/// • She carries a second, independent 4-cel animation blitted on top of her
///   still pose, whose base patnum the fight switches once mid-way through.

#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/v_colors.hpp"
#include "th04/main/frames.h"
#include "th04/main/boss/boss.hpp"
#include "th05/main/boss/bosses.hpp"

/// Still ZUN's assembly, named here for this file's two reads
/// ---------------------------------------------------------

// Counts down the bomb-triggered invincibility window. th05_main.asm sets it
// to 0x27 frames on every frame [bombing] is nonzero, clears it
// when phase 0 begins, decrements it once per frame in exalice_update(), and
// — the reason the name is a reading rather than a guess — skips
// boss_hittest_shots() entirely for exactly as long as it stays nonzero. This
// renderer is its only other reader, and only its parity. A th05_main.asm
// `.data?` label with no `public` of ZUN's (kb/codegen/0123). [inferred] name.
extern "C" unsigned char exalice_invincibility_frames;

// Base patnum of the 4-cel animation this function blits *on top of*
// EX-Alice while she is in her still pose. th05_main.asm writes it exactly
// twice, both times at a phase transition, and this is its only reader — so
// the fight has two of these animations and swaps between them once.
// **What the sprites depict is not recoverable from the binary**; "overlay"
// is the role this function gives them, not a reading of the artwork, the
// same caveat state/notes/_gengetsu_fg_render_qv.md records for
// mugetsu_gengetsu_shield_render(). A `.data?` label with no `public`
// (kb/codegen/0123). [inferred] name.
extern "C" unsigned int exalice_overlay_patnum;
/// ---------------------------------------------------------

// The distance between EX-Alice's regular cel set and the parallel one the
// invincibility flicker alternates with. The same trick, and the same
// spelling, as Yuki's `PAT_MAI + B4_CELS` in th05/sprites/main_pat.h.
static const int EXALICE_INVINCIBLE_CELS = 8;

// [inferred] The overlay is blitted only while [boss.sprite] is still one of
// the two cels of her stationary pose — [boss_sprite_left] and
// [boss_sprite_right] are seeded higher than this by exalice_update(), and so
// is every flickered patnum, which is why one comparison covers both
// exclusions.
static const int EXALICE_PAT_STILL_LAST = 181;

// [inferred] Phases 0 and 1 are the HP fill and the entrance; the overlay
// starts with the fight proper.
static const unsigned char EXALICE_PHASE_OVERLAY_FIRST = 2;

static const int EXALICE_OVERLAY_FRAMES_PER_CEL = 4;

void pascal near exalice_fg_render(void)
{
	// The original's frame is `ENTER 2, 0`: three 16-bit locals, two of them
	// enregistered and one on the stack. Declaration order decides which is
	// which, and for this shape it runs DI, SI, `[bp-2]` — so [left] has to
	// be declared FIRST to reach DI and [top] LAST to reach the stack slot.
	// Measured by swapping exactly these two declarations: the first spelling
	// put [left] in `[bp-2]` and [top] in DI, and moved nothing else in the
	// body. (kb/codegen/0010)
	screen_x_t left;
	int patnum;
	screen_y_t top;

	// Computed once, ahead of the phase branch, because both arms blit at
	// them — unlike TH04's renderers, which recompute inside each arm.
	left = boss.pos.cur.to_screen_left(BOSS_W);
	top = boss.pos.cur.to_screen_top(BOSS_H);

	if(boss.phase == PHASE_BOSS_EXPLODE_BIG) {
		super_large_put(left, top, boss.sprite);
	} else {
		patnum = boss.sprite;

		// `% 2`, not `& 1`: [exalice_invincibility_frames] is an
		// `unsigned char` that promotes to a *signed* `int`, and Turbo C++
		// 4.0J only masks for an unsigned operand. The remainder is consumed
		// out of DX, which is what proves this is `%`. (kb/codegen/0128)
		if(exalice_invincibility_frames % 2) {
			patnum += EXALICE_INVINCIBLE_CELS;
		}

		if(boss.damage_this_frame == 0) {
			super_put(left, top, patnum);
		} else {
			super_put_1plane(left, top, patnum, 0, super_plane(V_WHITE));
			boss.damage_this_frame = 0;
		}

		if(
			(patnum <= EXALICE_PAT_STILL_LAST) &&
			(boss.phase >= EXALICE_PHASE_OVERLAY_FIRST)
		) {
			// A division, not a mask: AX rather than DX survives the same
			// `MOV BX, 4` / `CWD` / `IDIV BX` as above. (kb/codegen/0128)
			patnum = (
				(stage_frame_mod16 / EXALICE_OVERLAY_FRAMES_PER_CEL) +
				exalice_overlay_patnum
			);
			super_put(left, top, patnum);
		}
	}
	explosions_small_update_and_render();
	explosions_big_update_and_render();
}
/// -------------------------------------------------
