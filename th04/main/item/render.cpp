/// Item rendering
/// --------------
/// One name, ONE body, shared by both games with zero `#if (GAME == 5)` sites:
/// everything that differs between the two dumps' copies comes from a constant
/// their own headers already give this file (ITEM_COUNT, `sizeof(item_t)`).
///
/// This replaces th04/main/item/render.asm, which was the tail `include` of
/// TWO root contributions at once — th04_main.asm's BOSS_FG_TEXT and
/// th05_main.asm's MB_DFR_TEXT — so it is one parcel and not two
/// (state/re/INCLUDE_TAIL_CLASSES.md). In both binaries the module was the
/// LAST thing the dump contributed to its segment, so #including this file at
/// the FRONT of the C++ object that already follows there puts every byte back
/// at its original address: no carve (kb/codegen/0080), no new segment name,
/// no group-list edit and no Tupfile.lua line (kb/codegen/0112 + 0114).
///
/// The two hosts are th04/boss_fg.cpp and th05/mb_dfr.cpp, and they must
/// #include this file BEFORE their existing one. kb/codegen/0119 was checked
/// rather than assumed on both: the body is 0x47 = 71 bytes, which is ODD, and
/// neither host object emits `-a2`-aligned data — no `switch` and no
/// `#pragma option -a` anywhere in either object's 33- and 17-header include
/// closure, and both objects' `_DATA` and `_BSS` map rows read `0000`.
///
/// Neither this file nor either host may list platform.h: it has no include
/// guard, and both hosts already reach it through their own trees. The three
/// headers below are unguarded too, and are listed here on the measured ground
/// that neither host's closure reaches any of them.

#include "x86real.h"
#include "th04/formats/super.h"
#include "th04/main/item/item.hpp"
#include "th04/main/item/splash.hpp"

// `extern "C"`, and `pascal` with it, because the module this replaces
// published this name undecorated and upper-cased, which is Borland's
// decoration for exactly that pair (kb/codegen/0123). The prototype the
// one caller uses is still the local one in th04/main/stage/loop.cpp:
// moving it into th04/main/item/item.hpp would mean #including that unguarded
// header into th04/main/stage/loop.cpp's translation unit, which is a
// collision this parcel has no need to risk.
extern "C" void pascal near items_render(void)
{
	// [item] is declared first so that it wins SI and the counter takes DI,
	// which is the allocation the original has. Neither is a stack local: the
	// original's prolog is `push bp; mov bp, sp; push si; push di` with no
	// `sub sp`, and the two blit coordinates below never become variables
	// because z_super_roll_put_tiny_16x16() takes them through _DX and _AX.
	item_t near *item;
	int i;

	_ES = SEG_PLANE_B;
	item_splashes_render();

	item = items;
	for(i = 0; i < ITEM_COUNT; (i++, item++)) {
		if(item->flag != F_ALIVE) {
			continue;
		}

		// The only cull. It is one-sided and top-only: an item is skipped
		// while it is still at least half a sprite above the playfield, and
		// nothing else is tested. The bottom, left and right edges are left
		// to z_super_roll_put_tiny_16x16()'s own vertical wrapping and to the
		// fact that items_update() removes an item before it can get there.
		if(item->pos.cur.y <= TO_SP(-(ITEM_H / 2))) {
			continue;
		}

		z_super_roll_put_tiny_16x16(
			item->pos.cur.to_screen_left(ITEM_W),
			item->pos.cur.to_vram_top_scrolled_seg1(ITEM_H),
			item->patnum
		);
	}
}
