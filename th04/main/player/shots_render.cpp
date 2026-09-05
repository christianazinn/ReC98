/// Rendering the player's shots
/// ----------------------------
/// (#included from th04/main_.cpp, at the very front of it, ahead of
/// shots_hittest(), enemies_render() and player_invalidate() -- the address
/// order all four bodies had in main__TEXT. This proc became that segment's
/// carve-free tail the moment shots_hittest() left, so the object grows
/// backwards into the hole again and every byte above it keeps its address
/// (kb/codegen 0099 + 0112 + 0114).)
///
/// TH05 has its own, different body for this function, still in th05_main.asm,
/// so this file is TH04-only for now rather than a shared one.
///
/// Every live shot, blitted back to front out of [shots], plus the option
/// laser ahead of them all. Hitshots -- shots in their SF_HIT decay animation
/// -- advance [patnum_base] themselves during the update, so only a live shot
/// gets the two-frame sprite alternation added here.

#include "platform.h"
#include "pc98.h"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th04/hardware/grcg.hpp"
#include "th04/formats/super.h"
#include "th04/main/player/shot.hpp"

// Marisa's option laser, blitted once per option. Its exact semantic C++ body
// cannot reproduce the original post-epilogue switch-table pad without moving
// bytes outside the function, so the proven-undecompilable body remains ASM at
// the tail of EXECL_TEXT; see state/notes/shot_laser_render.md. This same-group
// near call reaches the zero-byte C-linkage label published by th04_main.asm.
extern "C" void near shot_laser_render(void);

// master.lib's GRCG_OFF_CLOBBERING macro, which spills the port number to DX
// instead of using the immediate-port form that _outportb_() would emit. Same
// spelling as th04/main/stage/loop.cpp and th04/end/staff_dissolve.cpp.
#define grcg_off_clobbering_dx() outportb(0x7C, GC_OFF)

void pascal near shots_render(void)
{
	// SI and DI, in declaration order (kb/codegen/0146). The function needs
	// no stack frame at all: the sprite number lives in CX through the
	// pseudo-registers below, not in a local.
	register Shot near *shot;
	register int i;

	_ES = SEG_PLANE_B;
	grcg_setmode_rmw();
	if(shot_laser_time > SHOT_LASER_COOLDOWN_FRAMES) {
		shot_laser_render();
	}

	// Back to front, with a separate forward counter: the pointer walks down
	// from the last slot while [i] counts up. Same shape as
	// th04/main/bullet/render.cpp.
	shot = &shots[SHOT_COUNT - 1];
	for(i = 0; i < SHOT_COUNT; (i++, shot--)) {
		if((shot->flag == SF_FREE) || (shot->flag >= SF_REMOVE)) {
			continue;
		}

		// CX rather than a local, and it has to be live across the
		// scroll_subpixel_y_to_vram_seg1() call below, which no compiler
		// temporary could be. Only the pseudo-registers express that.
		_CH = 0;
		_CL = shot->patnum_base;
		if(shot->flag == SF_ALIVE) {
			// Hitshots increment [patnum_base] during the update, so this
			// two-frame alternation is for live shots only.
			_AL = shot->age;
			_AL &= 1;
			_AL += _CL;
			_CL = _AL;
		}
		z_super_roll_put_tiny_16x16(
			(shot->pos.cur.x.to_pixel() + (PLAYFIELD_LEFT - (SHOT_W / 2))),
			scroll_subpixel_y_to_vram_seg1(
				shot->pos.cur.y.v + TO_SP(PLAYFIELD_TOP - (SHOT_H / 2))
			),
			_CX
		);
	}
	grcg_off_clobbering_dx();
}
