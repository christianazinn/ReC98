/// Stage 5 boss - Mima
/// -------------------
/// Her final defeat, and the Stage 4 background scroller that sits directly
/// below it. Both are the tail of th02_main.asm's contribution to the nameless
/// code segment that th02/main/midboss/m4.cpp and th02/main/boss/b4.cpp
/// already share, so this translation unit needs no split and no new segment -
/// it just contributes to that segment between the dump's block and m4.cpp's.
/// (kb/codegen/0099)
///
/// stage4_update_and_render() is NOT hers: it is Stage 4's own [stage_update_and_render], and
/// it lives here for the same reason the boss-entrance helpers live in b4.cpp -
/// this is the object that reaches its address.

// -G, because stage4_update_and_render()'s prolog is `push bp; mov bp, sp; sub sp, 4` rather
// than an `ENTER`. (kb/codegen/0011) No -a2: `[measured]` nothing in this
// object emits a generated jump table, so there is no alignment to pin.
#pragma option -zCmain_03__TEXT -zPmain_03 -G

#include "platform.h"
#include "pc98.h"
#include "decomp.hpp"
#include "planar.h"
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/resident.hpp"
#include "th02/core/globals.hpp"
#include "th02/hardware/pages.hpp"
#include "th02/hardware/frmdelay.h"
#include "th02/hardware/egc.hpp"
#include "th02/main/playfld.hpp"
#include "th02/main/scroll.hpp"
#include "th02/main/tile/tile.hpp"
#include "th02/main/frames.hpp"
#include "th02/main/boss/boss.hpp"
#include "th02/main/hiscore.hpp"
#include "th02/main/dialog/dialog.hpp"
#include "th02/main/player/player.hpp"
#include "th02/sprites/main_pat.h"

// The sprite the boss and midboss renderers blit. Declared exactly the way
// th02/main/boss/b4.cpp already declares it.
extern "C" int patnum_2064E;

// The last thing this binary does, th02_main.asm: commit the run to the
// resident structure and launch MAINE.EXE over this process. mima_end() below
// and mima_19C8D() are its only two callers, and both reach it as a plain near
// call inside this one physical segment.
extern "C" void near mima_19C1D(void);

// The top of the first row of tiles stage4_update_and_render() copies, wrapped around the
// 400-line VRAM. A rename; that function holds every reference.
extern "C" vram_y_t stage4_tile_top;


// Her second and final defeat, and the end of the game.
extern "C" void far mima_end(void)
{
	// ZUN bug: This renders Mima's sprite to [page_back] on top of the boss
	// background rendered earlier, right before animating the defeat dialog on
	// [page_front] and launching into MAINE.EXE, which means that none of this
	// will ever show up. Given the fact that this code exists, it probably was
	// ZUN's intention to render the defeat dialog on top of what's rendered
	// here - i.e, before rendering any player shot, item, bullet, or spark
	// sprites - rather than blindly printing the text on top of whatever white
	// VRAM pixels that may have been in the text box area in the previous full
	// frame.
	super_put_rect(
		boss_left_on_page[page_back], boss_top_on_page[page_back], patnum_2064E
	);
	super_put_rect(
		(boss_left_on_page[page_back] + 48),
		boss_top_on_page[page_back],
		(patnum_2064E + 1)
	);
	super_put_rect(
		(boss_left_on_page[page_back] + 96),
		boss_top_on_page[page_back],
		(patnum_2064E + 2)
	);
	dialog_script_stage5_post_animate();
	score += 100000;
	if(!resident->continues_used) {
		scoredat_cleared_set();
	}
	mima_19C1D();
}


// Stage 5's background: the 6x6 block of tiles that scrolls behind the fight,
// copied through the EGC on [page_back] frames only.
//
// `[measured]` Stage 4 installs this as [stage_update_and_render] and only if
// [reduce_effects] is clear, so on a slow machine the stage runs without it.
extern "C" void far stage4_update_and_render(void)
{
	// [tile_image] first: Turbo C++ allocates stack locals in declaration
	// order, and the original's frame is `tile_image` at [bp-2] and `top` at
	// [bp-4]. (kb/codegen/0010)
	int tile_image;
	vram_y_t top;
	register screen_x_t x;
	int image;

	if(page_back != 0) {
		return;
	}
	egc_on();
	outport2(EGC_ACTIVEPLANEREG, 0xFFF7);
	outport2(EGC_READPLANEREG, 0x03FF);
	outport2(EGC_MASKREG, 0xFFFF);
	outport2(EGC_ADDRRESSREG, 0);
	outport2(EGC_BITLENGTHREG, 0x000F);
	// Literals rather than the constant names, because outport2() puts its
	// value straight into an `_asm` block where `|` is not an operator. This is
	// how th01/hardware/egc_impl.hpp already spells the same register.
	/* EGC_WS_PATREG | EGC_RL_MEMREAD */
	outport2(EGC_MODE_ROP_REG, 0x1100);
	tiles_invalidate_rect(176, 156, 96, 96);
	tiles_egc_render();
	/* EGC_WS_ROP | 11111100b */
	outport2(EGC_MODE_ROP_REG, 0x08FC);
	stage4_tile_top = 152;
	stage4_tile_top += scroll_line;
	if(stage4_tile_top >= RES_Y) {
		stage4_tile_top -= RES_Y;
	}
	top = stage4_tile_top;
	for(tile_image = 0; tile_image < 0x60; tile_image += 0x10) {
		for(x = 176, image = tile_image; x < 272; x += TILE_W, image++) {
			tile_egc_roll_copy_8(x, top, image);
		}
		top += TILE_H;
		if(top >= RES_Y) {
			top -= RES_Y;
		}
	}
	egc_off();
}
