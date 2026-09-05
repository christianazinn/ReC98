/// Item tile invalidation
/// ----------------------
/// TH04 emits this body from circle.cpp into CIRCLE_C_TEXT. TH05 emits it from
/// null.cpp into SCORE_I_TEXT.

#include "platform.h"
#include "th04/main/item/item.hpp"
#include "th04/main/item/splash.hpp"
#include "th04/main/tile/tile.hpp"

#if (GAME == 5)
// scroll.hpp restores the default segment after its inline functions.
#pragma codeseg SCORE_I_TEXT main_01
#endif

// See th04/main/tile/tile.hpp for why this declaration is a per-TU choice.
// Both call sites pass an SPPoint as one packed dword.
extern "C" void pascal near tiles_invalidate_around(const SPPoint center);

void near items_invalidate(void)
{
	// Separate scopes let Turbo C++ assign SI to both pointers and DI to both
	// counters. Their lifetimes do not overlap, matching the original's two
	// post-test loops without introducing a stack local.
	{
		register item_t near *item;
		register int i;

		// One packed 32-bit store, as in the original. Width and height are both
		// 16 here, so the two halves cannot be distinguished by this constant.
		reinterpret_cast<uint32_t &>(tile_invalidate_box) = (
			ITEM_W | (static_cast<uint32_t>(ITEM_H) << 16)
		);

		item = items;
		#if (GAME == 5)
		// ZUN bug: TH05 has 40 item slots, but only invalidates the first 32.
		i = 32;
		#else
		i = ITEM_COUNT;
		#endif
		do {
			if(item->flag != F_FREE) {
				tiles_invalidate_around(item->pos.prev);
			}
			item++;
		} while(--i);
	}
	{
		register item_splash_t near *splash;
		register int i;

		splash = item_splashes;
		i = ITEM_SPLASH_COUNT;
		do {
			if(splash->flag != F_FREE) {
				// Convert the Q12.4 radius to half-pixels, round it up, and use
				// that value for both dimensions of the invalidation box.
				tile_invalidate_box.y = tile_invalidate_box.x = (
					(static_cast<unsigned>(splash->radius_prev.v) >> 3) + 1
				);
				tiles_invalidate_around(splash->center);
			}
			splash++;
		} while(--i);
	}
}

// invalidate.asm's trailing `nop`, which word-aligned the next function.
#pragma codestring "\x90"
