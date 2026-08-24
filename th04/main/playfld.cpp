#include "libs/master.lib/pc98_gfx.hpp"
#include "th04/main/frames.h"
#include "th04/main/playfld.hpp"
#include "th04/main/tile/tile.hpp"
#if (GAME == 5)
#include "th02/main/scroll.hpp"
#endif

extern int8_t playfield_shake_redraw_time;

#if (GAME == 5)
// ... and ahead of even THAT: the cheeto bullet renderer, which was the first
// thing th05_main.asm's PLAYFLD_TEXT root contribution emitted and is now the
// front of this object. With it lifted, that contribution is zero bytes.
// TH04 has no cheeto bullets, so nothing is wired there.
#include "th05/main/bullet/cheetos_render.cpp"
#endif

#if (GAME == 5)
// ... and ahead of even that: the item splash renderer, which sat immediately
// before the bullet renderer in th05_main.asm's PLAYFLD_TEXT root
// contribution and is therefore the front of this object. TH04 emits the same
// source from circle.cpp into its carved IT_SPL_R_TEXT segment.
#include "th04/main/item/splashes_render.cpp"
#endif

#if (GAME == 5)
// TH05's bullet renderer was the last proc of th05_main.asm's PLAYFLD_TEXT
// root contribution, so it belongs at the very front of this object — ahead of
// the scroll advance below (kb/codegen/0114 + 0129). No carve, no new segment,
// no group-list edit and no Tupfile.lua line. TH04 keeps its own copy in
// BOSS_FG_TEXT, a different segment with a different host, so it is not
// #included here.
#include "th04/main/bullet/render.cpp"
#endif

// ZUN's object for this code segment also held the per-frame scroll advance,
// immediately ahead of playfield_shake_update_and_render(), so this #include
// is the original address order and needs neither a carve nor a Tupfile.lua
// line (kb/codegen/0114 + 0129).
// • TH05: it was the last proc of th05_main.asm's PLAYFLD_TEXT root
//   contribution, and this object already owned everything after it.
// • TH04: it was the last proc of th04_main.asm's *mai_TEXT*, the segment
//   immediately before this one in main_01, and this object owns all of
//   PLAYFLD_TEXT. Lifting it to the front of this object therefore moved only
//   the segment boundary, not a single byte of the image.
#include "th04/main/scroll.cpp"

inline void shift(
	egc_shift_func_t *func,
	vram_y_t top,
	vram_y_t bottom,
	bool negate,
	const pixel_t &dots
) {
	func(
		PLAYFIELD_LEFT,
		top,
		(PLAYFIELD_RIGHT - 1),
		bottom,
		(negate) ? -dots : dots
	);

	// Since these games use page flipping and we only ever shift the active
	// VRAM page, we need to force a redraw *on* the next frame after this one
	// (*for* the one two frames after this one) to undo the shift we just did.
	//
	// Forcing a redraw *on this* frame (*for* the next one) is technically
	// only necessary during the animation, where the other VRAM page will
	// still display its previous shifted state. It's just easier to always
	// unconditionally redraw the next two frames, though.
	playfield_shake_redraw_time = PAGE_COUNT;
}

inline void shift_x(egc_shift_func_t *func, bool negate) {
	shift(func, 0, (RES_Y - 1), negate, playfield_shake_x);
}

inline void shift_y(egc_shift_func_t *func, bool negate) {
#if (GAME == 5)
	// Micro-optimization to limit the amount of moved pixels during bosses.
	if(scroll_line == 0) {
		shift(
			func,
			PLAYFIELD_TOP,
			(PLAYFIELD_BOTTOM - 1),
			negate,
			playfield_shake_y
		);
		return;
	}
#endif
	shift(func, 0, (RES_Y - 1), negate, playfield_shake_y);
}

void near playfield_shake_update_and_render(void)
{
	if(playfield_shake_anim_time) {
		playfield_shake_x = (stage_frame_mod2 == 0) ? -2 : 2;
		playfield_shake_y = (stage_frame_mod4 <= 1) ? -2 : 2;
		playfield_shake_anim_time--;
	}

	if(playfield_shake_x < 0) {
#if (GAME == 5)
		// Doubly strong left shaking?
		egc_shift_left(
			PLAYFIELD_LEFT,
			0,
			(PLAYFIELD_RIGHT - 1),
			(RES_Y - 1),
			-playfield_shake_x
		);
#endif
		shift_x(egc_shift_left, true);
	} else if(playfield_shake_x > 0) {
		shift_x(egc_shift_right, false);
	}
	if(playfield_shake_y < 0) {
		shift_y(egc_shift_up, true);
	} else if(playfield_shake_y > 0) {
		shift_y(egc_shift_down, false);
	}

	if(playfield_shake_redraw_time) {
		playfield_shake_redraw_time--;
		tiles_invalidate_all();
		playfield_shake_x = 0;
		playfield_shake_y = 0;
	}

}
