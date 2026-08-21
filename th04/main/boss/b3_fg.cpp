/// Stage 3 Boss - Elly, foreground rendering
/// -----------------------------------------
/// (#included from th04/main_012.cpp, ahead of th04/main/stage/reset.cpp and
/// therefore in its original address order. ZUN's object for main_012_TEXT
/// held this renderer and then stage_state_reset(); that an original object
/// held several unrelated sources is kb/codegen/0112. That wrapper's
/// Tupfile.lua line is append-anywhere, and it already exists, so this lift
/// costs no build-system change at all.)
///
/// Elly keeps the three-way [boss_fg_render] contract that
/// th04/main/boss/render.cpp documents for Orange and Kurumi, in the same
/// reduced form mugetsu_fg_render() (th04/main/boss/bx1_fg.cpp) uses: no
/// entrance branch, and no cel animation, so [boss.sprite] is blitted as-is
/// on every frame. Unlike Mugetsu she has no zero-sprite guard, and like
/// Orange and Kurumi — but unlike Reimu, Mugetsu and Gengetsu — she does
/// *not* reset [boss.damage_this_frame] after the white flash.
///
/// What is hers alone among the TH04 bosses: the second entity blitted after
/// the boss, below. Both coordinate pairs fold to the same 64x64 sprite box.

#include "platform.h"
#include "pc98.h"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/v_colors.hpp"
#include "th04/main/frames.h"
#include "th04/main/boss/boss.hpp"
#include "th04/main/boss/bosses.hpp"

/// Elly's boomerang
/// ----------------
/// The one entity Elly owns, and the byte that tracks it. Both are
/// th04_main.asm `.data?` labels with no `public` of ZUN's, so they needed
/// zero-byte `label` aliases to become linkable (kb/codegen/0123). This
/// function is their only C++ reader; every writer is still ASM.
///
/// [inferred] The STEM only. "boomerang" names the out-and-back trajectory
/// below, not a reading of the artwork — nothing in the dump ties patnums
/// 142…145 to an asset. th04/sprites/main_pat.h does the same thing for its
/// own two unattributable ranges, and note what it actually withholds: it
/// NAMES both (PAT_KURUMI, PAT_MIDBOSSX) and omits only the source-FILE
/// comment every other block there carries, "because nothing in the dump ties
/// this patnum range to a specific .bmt/.bb?, and naming one would be a
/// guess." The tree names sprites from their behaviour freely.
///
/// [measured] Everything below is read out of `elly_1B95C`, Elly's pattern
/// driver, and none of it is inferred:
///
/// • thrown from Elly's own position on one frame of her pattern
///   (snd_se_play(9); flag ← EBF_THROWN; position ← [boss.pos.cur]);
/// • it travels under a velocity that `elly_1B95C` recomputes *every frame*
///   with vector2() from its own angle and speed bytes, so its
///   update_seg3() only integrates — the reflection off the
///   playfield edges is that driver's own state dispatch on the edge
///   comparisons, each arm of which also takes 4 off the speed;
/// • it hurts the player within a ±24-PIXEL box, and the player's shots
///   *deflect* rather than destroy it: shots_hittest() with a 32-PIXEL
///   radius, half the hit count subtracted from its downward velocity;
/// • it is caught inside a ±16-PIXEL box around Elly, which sets
///   EBF_CAUGHT and advances her pattern.
///
/// All three of those are pixels, not subpixels: the driver spells them
/// `(±24 shl 4)`, `(32 shl 4)` and `(±16 shl 4)`, and `N shl 4` is to_sp(N),
/// i.e. N pixels converted INTO subpixels.
enum elly_boomerang_flag_t {
	EBF_FREE = 0,
	EBF_THROWN = 1,
	// One more tile invalidation, then transitions to EBF_FREE.
	// elly_invalidate() (th04/main/boss/bg.cpp) invalidates while the flag
	// is *nonzero*, but this function only blits while it is EBF_THROWN, so
	// this state exists to buy that one frame —
	// the mechanism th02/main/boss/b3.hpp's SF_REMOVE documents.
	EBF_CAUGHT = 2,
};

extern "C" unsigned char elly_boomerang_flag;
extern "C" PlayfieldMotion elly_boomerang_pos;

// = PAT_STAGE + 14, four cels at two frames each.
//
// Still spelled as the literal rather than through th04/sprites/main_pat.h,
// where naming round 4's ruling would put it — but NOT for the reason this
// comment used to give. That reason was the double-`typedef enum` this
// translation unit would get, because th04/main/stage/reset.cpp, which
// th04/main_012.cpp compiles into the same object after this file, already
// reaches that header through th04/main/enemy/enemy.hpp and the header had no
// include guard. IT HAS ONE NOW, so that blocker is discharged and this is no
// longer a KEEP-WITH-LICENCE resting on a build constraint.
//
// What is left is a structural decision, not a constraint: main_pat.h has no
// Stage 3 block at all, so honouring round 4 here means opening a new section
// in a header that 16 sources and one shared header include directly. That
// belongs to the parcel that next edits that enum, not to the one that added
// the guard.
static const int PAT_ELLY_BOOMERANG = 142;
static const int ELLY_BOOMERANG_FRAMES_PER_CEL = 2;

// [inferred] Only the folded constants 0 and -16 survive into the binary, so
// the split between "sprite box" and "extra offset" is not recoverable — the
// same note orange_fg_render() and mugetsu_fg_render() carry. Reading the
// whole of it as a sprite box needs no leftover in either axis, and gives the
// boomerang the same 64x64 box as Elly herself.
static const pixel_t ELLY_BOOMERANG_W = 64;
static const pixel_t ELLY_BOOMERANG_H = 64;
/// ----------------

void pascal near elly_fg_render(void)
{
	// Declared first so that it takes the original's single [bp-2] stack
	// slot — the frame is `ENTER 2, 0` — with the two coordinates below as
	// the register variables, [left] first because it takes SI.
	// (kb/codegen/0010, 0117)
	int patnum;

	screen_x_t left;
	vram_y_t top;

	// Computed once, ahead of the phase chain, rather than per branch the way
	// render.cpp's two bosses do it: all three branches below use the same
	// pair. yuuka5_fg_render() (th04/main/boss/b5r.cpp) is the other boss
	// whose renderer hoists them.
	left = boss.pos.cur.to_screen_left(BOSS_W);
	top = boss.pos.cur.to_screen_top(BOSS_H);

	if(boss.phase < PHASE_EXPLODE_BIG) {
		if(boss.damage_this_frame == 0) {
			// [top] is pushed out of AX, where the assignment above left it,
			// rather than out of the DI it lives in — this is the one call
			// site of the three that the compiler reaches in a straight line,
			// so its register tracking is still valid here and it needs no
			// source shape. The other two are reached through a label, i.e. a
			// basic-block join, and push DI. (kb/codegen/0126)
			super_put(left, top, boss.sprite);
		} else {
			super_put_1plane(
				left, top, boss.sprite, 0, super_plane(V_WHITE)
			);
		}
	} else if(boss.phase == PHASE_EXPLODE_BIG) {
		super_large_put(left, top, boss.sprite);
	}

	// Clipped as a point, against the playfield in subpixels, before any
	// coordinate conversion — so the entity vanishes as its *center* leaves
	// the playfield rather than being clipped at the edge.
	if(
		(elly_boomerang_flag == EBF_THROWN) &&
		(elly_boomerang_pos.cur.x >= 0) &&
		(elly_boomerang_pos.cur.x < TO_SP(PLAYFIELD_W)) &&
		(elly_boomerang_pos.cur.y >= 0) &&
		(elly_boomerang_pos.cur.y < TO_SP(PLAYFIELD_H))
	) {
		left = elly_boomerang_pos.cur.to_screen_left(ELLY_BOOMERANG_W);
		top = elly_boomerang_pos.cur.to_screen_top(ELLY_BOOMERANG_H);

		// A signed division, not a shift: [stage_frame_mod8] is an
		// `unsigned char` that promotes to `int`, and the original really
		// does emit the `CWD` / `SUB AX, DX` / `SAR AX, 1` idiom that only
		// `/ 2` produces. (kb/codegen/0128)
		patnum = (
			(stage_frame_mod8 / ELLY_BOOMERANG_FRAMES_PER_CEL) +
			PAT_ELLY_BOOMERANG
		);
		super_put(left, top, patnum);
	}

	explosions_small_update_and_render();
	explosions_big_update_and_render();
}
