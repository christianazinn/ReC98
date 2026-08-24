/// Extra Stage boss barrier
/// ------------------------
/// (#included from th04/main_01.cpp, after th04/main/boss/bx1_fg.cpp. ZUN's
/// object for this code segment held the loader now in
/// th04/formats/bb_txt_load.cpp, mugetsu_fg_render() and then this function;
/// that an original object held
/// several unrelated sources is kb/codegen/0112. The first of the three stays
/// in the dump, so this object is spliced *between* th04_main.asm and
/// th04\scoreupd.asm in the link list rather than appended after both — see
/// th04/main_01.cpp for why that line is position-critical.)
///
/// Both Extra Stage bosses become invincible for a fixed window whenever the
/// player bombs, and both of their foreground renderers call this from their
/// undamaged branch to draw the barrier that shows it.

// platform.h and th04/main/boss/bosses.hpp are NOT included here: neither has
// an include guard, and th04/main/boss/bx1_fg.cpp already supplies both to
// this object. Naming either again is a compile error. (The same arrangement
// as th05/main/midboss/m5.cpp, which sits behind th05/main/boss/bx_fg.cpp in
// its object.)
#include "pc98.h"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th04/main/boss/boss.hpp"
#include "th04/sprites/main_pat.h"

/// Still ASM
/// ---------
// Frames of bomb-triggered invincibility left. Re-armed to SHIELD_FRAMES on
// every frame [bombing] is nonzero and counted down from there by both
// mugetsu_update() and gengetsu_update(); while it is nonzero, both bosses'
// hittest helpers discard the damage they compute and play the invincibility
// sound instead of the hit one. A th04_main.asm `.data?` label with no
// `public` of ZUN's (kb/codegen/0123). [inferred] name.
extern "C" unsigned char mugetsu_gengetsu_shield_frames;
/// ---------

// The window is re-armed rather than merely started, so it lasts as long as
// the bomb does plus this many frames.
static const int SHIELD_FRAMES = 32;

// st06.bmt cels. Two halves, GENGETSU_W / 2 apart, exactly like Gengetsu's own
// sprite pair — which is why the barrier is drawn at her box rather than at
// Mugetsu's, even though Mugetsu gets it too.
static const int PAT_SHIELD_LEFT = (PAT_STAGE + 8);
static const int PAT_SHIELD_RIGHT = (PAT_SHIELD_LEFT + 1);

// [inferred] The original folds PLAYFIELD_LEFT and the sprite extent into one
// constant, so the split between "box width" and "extra offset" is not
// recoverable from the binary — the same note orange_fg_render()
// (th04/main/boss/render.cpp) carries. Expressing it against GENGETSU_W is the
// reading that needs the smallest leftover: the barrier sits one pixel to the
// right of Gengetsu's own sprite pair, and vertically on top of it exactly.
static const pixel_t SHIELD_OFFSET_X = 1;

// `extern "C"`, and not only because th04/main/boss/fg.cpp and th04_main.asm
// both already reference `_mugetsu_gengetsu_shield_render` (kb/codegen/0102):
// C++ mangling would ask TLINK for `@mugetsu_gengetsu_shield_render$qv`, 35
// characters, and Turbo C++ only keeps 32 before truncating — the second
// trigger in kb/codegen/0123.
extern "C" void near mugetsu_gengetsu_shield_render(void)
{
	screen_x_t left;
	vram_y_t top;

	// Blinks on the odd frames of the count, but is drawn unconditionally on
	// the frames the window is still being re-armed — so it appears solid for
	// as long as the bomb lasts and then flickers out.
	if(
		(mugetsu_gengetsu_shield_frames != 0) &&
		(
			(mugetsu_gengetsu_shield_frames >= SHIELD_FRAMES) ||
			(mugetsu_gengetsu_shield_frames & 1)
		)
	) {
		left = (
			boss.pos.cur.to_screen_left(GENGETSU_W) + SHIELD_OFFSET_X
		);
		top = boss.pos.cur.to_screen_top(GENGETSU_H);
		super_put(left, top, PAT_SHIELD_LEFT);
		super_put((left + (GENGETSU_W / 2)), top, PAT_SHIELD_RIGHT);
	}
}
