/// Yumeko's swords: the renderer
/// -----------------------------
/// Blits every live sword. The state half of the same loop is still ZUN's
/// assembly; this is the only one of the three functions
/// th05/main/bullet/sword.hpp declares that has been decompiled.
///
/// Unlike Shinki's ball bullets, which wrap their Y coordinate by hand, the
/// swords go through scroll_subpixel_y_to_vram_seg1() — they are rendered in
/// a scrolling stage, and the sprite's row therefore has to follow the
/// scroll offset.
///
/// (#included from th05/b6cbull.cpp, ahead of th05/main/boss/b5_fg.cpp. The
/// module this replaces was the last thing th05_main.asm contributed to
/// MIDBOSSX_TEXT, and this object is the segment's next contribution, so the
/// C++ side grows backwards into the hole and every byte above it keeps its
/// address. kb/codegen 0112 + 0114.)

#include "th04/formats/super.h"
#include "th04/main/custom.hpp"
#include "th05/main/bullet/sword.hpp"

// `extern "C"` + `pascal`, because the module published the undecorated
// upper-case `SWORDS_RENDER` and th05_main.asm still takes its address
// (`mov _boss_custombullets_render, offset swords_render`). Plain C++ linkage
// would emit `@SWORDS_RENDER$QV` instead and leave that `offset` unresolved
// (kb/codegen/0081); sword.hpp's declaration is corrected to match.
extern "C" void pascal near swords_render(void)
{
	#define left	_AX
	#define top 	_DX

	sword_t near *sword;
	int i;
	int patnum_tiny;

	_ES = SEG_PLANE_B;
	sword = swords;
	for(i = 1; (i < (1 + SWORD_COUNT)); (i++, sword++)) {
		if(sword->flag == F_FREE) {
			continue;
		}
		patnum_tiny = sword->patnum_tiny;
		top = scroll_subpixel_y_to_vram_seg1(sword->pos.cur.y);
		left = sword->pos.cur.to_screen_left(SWORD_W);
		z_super_roll_put_tiny_32x32_raw(patnum_tiny);
	}

	#undef top
	#undef left
}
