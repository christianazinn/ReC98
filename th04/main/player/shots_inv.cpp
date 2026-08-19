/// Invalidating the background tiles behind the player's shots
/// -----------------------------------------------------------
/// ONE body with three `#if (GAME == 5)` sites, but the two halves diverge
/// completely after the shot loop: TH05 invalidates its separate [hitshots]
/// array, while TH04 invalidates the option laser that TH05 kept in the code
/// but never fires.
///
/// The two loops in TH05 share one counter, which is why it is the game with a
/// stack local here: [shot] and [hitshot] are both dereferenced three times per
/// iteration and take SI and DI, leaving [i] at [bp-2]. TH04 has only one
/// pointer, so [i] gets DI and the function needs no stack frame at all.
/// (kb/codegen/0117)

#include "th04/main/player/shot.hpp"
#include "th04/main/tile/tile.hpp"
#include "th02/main/entity.hpp"

#if (GAME != 5)
	// ZUN's object for SHOT_INV_TEXT opened with the playchar-specific bomb
	// animations, then the bomb driver, then the bomb star animation, so all
	// three have to be emitted from this translation unit and ahead of both
	// functions below, in that order (kb/codegen/0129). These four headers
	// exist only for the star animation and are the four its closure needs
	// that the three above do not already bring: verified one-by-one, because
	// 15 of the 30 headers in this TU's closure have no include guard and a
	// second inclusion of any of them is a hard error.
	#include "th04/main/player/bomb.hpp"
	#include "th04/playchar.h"
	#include "th04/math/vector.hpp"
	#include "th04/formats/super.h"

	// And these five exist only for bombupd.cpp, checked the same way: none
	// of them is in this TU's closure yet, and none of them re-includes any
	// of its unguarded members. libs/master.lib/pc98_gfx.hpp is the one that
	// FAILS that check -- it would pull libs/master.lib/func.hpp in a second
	// time -- so the three master.lib symbols are declared in bombupd.cpp
	// itself instead.
	// Macro-only and idempotent (its whole body is guarded by
	// `#if !defined(MASTER_NEAR) && ...`), so it is safe here even though
	// pc98_gfx.hpp would also bring it. bombupd.cpp needs MASTER_RET.
	#include "libs/master.lib/func.hpp"
	#include "th03/hardware/palette.hpp"
	#include "th04/snd/snd.h"
	#include "th04/main/bg.hpp"
	#include "th04/main/circle.hpp"
	#include "th04/main/null.hpp"
	#include "th04/main/item/item.hpp"

	// And these exist only for bombchar.cpp, checked the same way. None of
	// them is in this TU's closure yet, and the only unguarded ones —
	// th04/hardware/grcg.hpp, th04/main/frames.h and th04/main/drawp.hpp —
	// are each pulled in exactly once from here.
	// th04/hardware/grcg.hpp brings nothing but pc98.h and x86real.h, and
	// th04/main/drawp.hpp nothing but th04/main/playfld.hpp; all three are
	// guarded and already present.
	//
	// th02/v_colors.hpp used to be on this list and is NOT any more. It is
	// unguarded, and as of TH04's adoption of
	// th04/main/player/bb_playchar_put.cpp that file reaches it first in this
	// same object — which is the only object in the tree where both files are
	// present. Naming it twice is a redefinition of `enum th02_vram_colors_t`.
	// If this file is ever compiled without that one ahead of it, this is the
	// line to put back.
	#include "th04/hardware/grcg.hpp"
	#include "th04/main/frames.h"
	#include "th04/main/drawp.hpp"
	#include "th03/formats/cdg.h"
	#include "th04/sprites/main_cdg.h"

	#include "th04/main/player/bombchar.cpp"
	#include "th04/main/player/bombupd.cpp"
	#include "th04/main/player/bombanim.cpp"
#endif

// See th04/main/tile/tile.hpp for why this declaration has to be repeated in
// every translation unit that calls the function, and why the parameter list is
// a per-TU choice: it is decided by the code the original generated, not by what
// the call sites would find convenient. Both games push two separate words here,
// so both take this form — note that TH05 does too, even though the laser half
// below, the only place that passes computed values rather than struct members,
// is TH04-only. th04/main/midboss/inv.cpp is the SPPoint form of the same call.
extern "C" void pascal near tiles_invalidate_around(
	subpixel_t center_y, subpixel_t center_x
);

#if (GAME == 5)
	#define SHOT_FLAG_FREE F_FREE
#else
	#define SHOT_FLAG_FREE SF_FREE

	// Written by shot_reset() and never read anywhere in MAIN.EXE.
	// ZUN bloat: one store with no effect. The evidence is a whole-binary
	// symbol search, not an emulator run — the placeholder the dump gives it
	// occurs exactly twice in the tree, that store and its `db ?`, and its
	// address occurs nowhere else at all.
	//
	// Named for what was measured about it rather than for what it might have
	// meant, which is the only honest option for a variable with no reader to
	// argue from. The spelling and the zero-byte alias behind it
	// (kb/codegen/0123) are both TH02's, for the identical situation:
	// th02/main/scroll.cpp declares [scroll_unused_2] this way and
	// th02_main.asm aliases its byte_2034E to match. No numeric suffix here,
	// because TH02's suffixes are IDA's discovery order across a family of
	// three and this subsystem has exactly one.
	extern uint8_t shot_unused;
#endif

#if (GAME == 4)
// Clears the shot-firing state at the start of every stage: the regular shot's
// cooldown, and both halves of the option laser's. stage_init() calls this
// four statements before bomb_reset(), which resets the other half of the
// player's per-stage state.
//
// TH04-only, and NOT for lifting-order reasons: TH05 has no such helper. Two of
// the three counters belong to the option laser, which TH05 keeps in its code
// but never fires, so TH05's stage-init proc simply sets [shot_time] inline in
// the same run of statements that calls bomb_reset(). Singular, per the
// convention th02/main/player/bomb.hpp spells out: scalar-state resets are
// singular (bomb_reset(), scroll_reset(), score_reset(), player_reset()) and
// only the array ones are plural — which is also why this sits next to
// shots_invalidate() without matching its plural.
void near shot_reset(void)
{
	shot_laser_time = 0;
	shot_laser_style = SLS_2;
	shot_time = 0;

	// ZUN bloat, declared above: a byte in this MAIN.EXE's own BSS, not a
	// field of the resident structure, so no other binary can observe it
	// either. Kept because removing it would change the bytes.
	shot_unused = 0;
}
#endif

void near shots_invalidate(void)
{
	int i;
	Shot near *shot;
	#if (GAME == 5)
		HitShot near *hitshot;
	#endif

	tile_invalidate_box.x = SHOT_W;
	tile_invalidate_box.y = SHOT_H;
	shot = shots;
	#if (GAME == 5)
		hitshot = hitshots;
	#endif
	for(i = 0; i < SHOT_COUNT; (i++, shot++)) {
		if(shot->flag != SHOT_FLAG_FREE) {
			tiles_invalidate_around_xy(shot->pos.prev.x, shot->pos.prev.y);
		}
	}

	#if (GAME == 5)
		for(i = 0; i < HITSHOT_COUNT; (i++, hitshot++)) {
			if(hitshot->age != 0) {
				tiles_invalidate_around_xy(
					hitshot->pos.prev.x, hitshot->pos.prev.y
				);
			}
		}
	#else
		// The laser reaches from the player to the top of the playfield, so
		// its height in tiles is its own Y coordinate, and its center is at
		// half of that. The division by 16 is a real IDIV rather than a shift
		// because Turbo C++ 4.0J does not strength-reduce signed division by
		// anything but 2 - which is exactly why to_pixel_slow() exists.
		if(shot_laser_time >= SHOT_LASER_COOLDOWN_FRAMES) {
			tile_invalidate_box.x = SHOT_LASER_W;
			tile_invalidate_box.y =
				shot_laser_bottomcenter.prev.y.to_pixel_slow();
			tiles_invalidate_around_xy(
				(shot_laser_bottomcenter.prev.x.v -
					to_sp(PLAYER_OPTION_DISTANCE)),
				(shot_laser_bottomcenter.prev.y.v / 2)
			);
			tiles_invalidate_around_xy(
				(shot_laser_bottomcenter.prev.x.v +
					to_sp(PLAYER_OPTION_DISTANCE)),
				(shot_laser_bottomcenter.prev.y.v / 2)
			);
		}
	#endif
}
