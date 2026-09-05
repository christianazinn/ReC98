/// MAINE.EXE's own page 1 → page 0 EGC rectangle copy
/// --------------------------------------------------
/// TH04's registration screen keeps its background on VRAM page 1 rather than
/// in a bgimage buffer — it links neither th04/hardware/bgimage.cpp's
/// unblitter nor th04/egcrect.cpp, which Tupfile.lua gives to TH04's OP.EXE
/// only — and unblits through this `near` function instead, which takes the
/// same four parameters as bgimage_put_rect_16().
///
/// `[measured]` Its *contract* is exactly th04/hardware/egcrect.cpp's
/// egc_copy_rect_1_to_0_16(): start the EGC, copy the rectangle 16 pixels at a
/// time through plane B alone, turn the EGC back off. Its *body* is not — that
/// one is a hand-tuned `register`-pseudovariable STOSW loop, this one a plain-C
/// double loop with a stack-allocated VRAM offset, closer to
/// th02/hardware/grp_rect.cpp's graph_copy_rect_1_to_0_16() (which in turn is
/// not this contract: it copies all four planes and never touches the EGC). So
/// TH04 has two different bodies for one operation, and the `_near` suffix is
/// what keeps them apart. TH05 has neither — its registration screen unblits
/// through bgimage_put_rect_16().
///
/// This was the last thing in th04_maine.asm's SCORE_TEXT contribution, right
/// after regist_menu(), which now sits at the tail of th04/hi_end.cpp's
/// contribution to the same segment. Lifting it there too therefore EMPTIES
/// the dump's block: kb/codegen/0098 head lift, no carve, no new segment, no
/// Tupfile.lua line.

#include "planar.h"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th01/hardware/egc_impl.hpp"

/// master.lib's egc_on(), inlined into its only caller, followed by
/// egc_setup_copy() - i.e. th01/hardware/egc.inc's EGC_START_COPY_INLINED,
/// matching the body now decompiled in th04/main/tile/render_a.cpp.
///
/// `[measured]` Every write here needs the two-byte immediate `OUT` form, so
/// `_outportb_()` rather than master.lib's `grcg_setmode()` /
/// `graph_mode_change()`, which go through outportb() and spill the port
/// number to DX (kb/codegen/0088). The six 16-bit register writes below need
/// the `mov ax` / `mov dx` / `out dx, ax` order of master.lib's `outw2` macro,
/// which is what decomp.hpp's outport2() — and hence egc_setup_copy() —
/// emits (kb/codegen/0051).
static void near egc_start_copy_inlined(void)
{
	_outportb_(0x7C, GC_OFF);	// GRCG off
	_outportb_(0x6A, 7);    	// graph_mode_change(true)
	_outportb_(0x6A, 5);    	// graph_mode_egc(true)
	_outportb_(0x7C, GC_TDW);	// The EGC requires an active GRCG in TDW mode
	_outportb_(0x6A, 6);    	// graph_mode_change(false)
	egc_setup_copy();
}

extern "C" void pascal near egc_copy_rect_1_to_0_16_near(
	screen_x_t left, vram_y_t top, pixel_t w, pixel_t h
)
{
	register vram_offset_t vo;
	register vram_word_amount_t w_16 = w;
	vram_word_amount_t col;
	pixel_t row;
	vram_offset_t vo_row;
	dots16_t tmp;

	egc_start_copy_inlined();
	vo_row = vram_offset_shift(left, top);
	w_16 /= EGC_REGISTER_DOTS;

	// `[measured]` [vo_row] is advanced in the increment clause, not at the
	// end of the body: the original increments [row] *first* and only then
	// adds ROW_SIZE. Moving the addition into the body swaps those two
	// instructions and nothing else.
	for(row = 0; row < h; (row++, vo_row += ROW_SIZE)) {
		for(
			(col = 0, vo = vo_row);
			col < w_16;
			(col++, vo += EGC_REGISTER_SIZE)
		) {
			graph_accesspage(1);	VRAM_SNAP(tmp, B, vo, 16);
			graph_accesspage(0);	VRAM_PUT(B, vo, tmp, 16);
		}
	}
	egc_off();
}

// `[measured]` The original object's SCORE_TEXT contribution ends on one pad
// byte past this function, which is also the segment's last byte. Without it
// th04/hi_end.cpp contributes 0xC9 where the dump contributed 0xCA.
#pragma codestring "\x00"
