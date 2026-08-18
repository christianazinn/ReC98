/// Advancing the map scroll, once per frame
/// ----------------------------------------
/// Records the scroll position the VRAM page about to be drawn will be drawn
/// at, hardware-scrolls the display to the position the *previous* frame
/// computed, then advances [scroll_subpixel_line] by [scroll_speed] and turns
/// every whole pixel it crossed into a [scroll_line] step, a
/// [scroll_lines_pending] count and a [scroll_last_delta]. Ends by handing all
/// of that to tiles_scroll_and_egc_render(), which is the only thing that
/// actually redraws tiles.
///
/// (#included from th04/main/playfld.cpp in both games, which is
/// kb/codegen/0129's host-source form. On the TH05 side this function was the
/// last proc of th05_main.asm's PLAYFLD_TEXT root contribution and
/// th05/playfld.cpp already owned everything after it. TH04 put its copy in
/// mai_TEXT, whose own C++ host contributes 0 bytes -- but mai_TEXT is
/// immediately followed by PLAYFLD_TEXT in main_01, and th04/playfld.cpp owns
/// all of that segment, so the neighbour hosted the lift instead. Either way
/// the #include is the original address order and costs neither a carve nor a
/// Tupfile.lua line -- see state/notes/scroll_update_and_render.md.
///
/// Because this file shares a translation unit with th04/main/playfld.cpp, it
/// only includes headers that are #include-guarded; everything else comes
/// from the host.)

#include "x86real.h"
#include "th02/hardware/pages.hpp"
#if (GAME == 5)
	#include "th04/main/stage/stage.hpp"
#endif

void near scroll_update_and_render(void)
{
	#if (GAME == 5)
		// Stage 6 is Shinki's, and her background is not made of map tiles.
		// The test below repeats this one rather than sharing it, and
		// tiles_scroll_and_egc_render() then makes it a third time.
		if(stage_id == 5) {
			scroll_active = false;
		}
	#endif

	// The page that is about to be drawn is shown one frame from now, so
	// everything drawn onto it has to be placed at *this* frame's scroll
	// position, and hud/entity code reads it back from here.
	scroll_line_on_page[page_back] = scroll_line;

	// Scroll the display to the position the previous frame computed -- not
	// to the one computed below, which belongs to the page being drawn now.
	if(
		scroll_lines_prev_frame && scroll_active
		#if (GAME == 5)
			&& (stage_id != 5)
		#endif
	) {
		graph_scrollup(scroll_line);
	}

	scroll_last_delta.v = 0;

	// Kept in AL/AX all the way down: the original computes the whole-pixel
	// count once and then reads it back as a byte, as a word, and shifted
	// into subpixels, without ever spilling it. The function establishes a
	// frame but allocates no locals, so a C++ local here would have to be one
	// the optimizer never spills -- pseudo-registers say it outright.
	_AL = scroll_subpixel_line.v;
	_AL += scroll_speed.v;
	scroll_subpixel_line.v = _AL;
	if(_AL >= SUBPIXEL_FACTOR) {
		_AH = 0;
		_AX >>= SUBPIXEL_BITS;

		// The map scrolls upwards, so a line crossed *lowers* [scroll_line];
		// VRAM wraps at RES_Y.
		//
		// kb/codegen/0031, the same pin th04/main/tile/scroll.cpp needs one
		// function later, and for a sharper reason: the original fuses the
		// sign flag of `SUB [scroll_line], AX` straight into a JNS, and
		// writing that as `if((scroll_line -= _AX) < 0)` makes Turbo C++ load
		// [scroll_line] into AX first -- which clobbers the very count AX is
		// carrying, and compiles to `SUB AX, AX`. Pin the subtraction and its
		// branch, and nothing else; the target is an ordinary C label, so the
		// skipped statement stays ordinary C++.
		asm {
			sub 	word ptr scroll_line, ax;
			jns 	short scroll_line_still_on_page;
		}
		scroll_line += RES_Y;
scroll_line_still_on_page:

		scroll_lines_pending = _AL;
		scroll_subpixel_line.v &= (SUBPIXEL_FACTOR - 1);
		_AX <<= SUBPIXEL_BITS;
		scroll_last_delta.v = _AX;
	}

	tiles_scroll_and_egc_render();
}
