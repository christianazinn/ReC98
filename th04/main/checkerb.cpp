#include "planar.h"
#include "platform/x86real/flags.hpp"
#include "th04/hardware/grcg.hpp"
#include "th04/main/playfld.hpp"
#include "th04/main/checkerb.hpp"

// Constants
// ---------

#define CHECKERBOARD_W 32
#define CHECKERBOARD_H 32
static const pixel_t CHECKERBOARD_SPEED = 4;

// In the very unusual unit of "VRAM bytes per frame".
static const vram_byte_amount_t CHECKERBOARD_VO_SPEED = (
	CHECKERBOARD_SPEED * ROW_SIZE
);

static const vram_byte_amount_t CHECKERBOARD_VRAM_W = (
	CHECKERBOARD_W / BYTE_DOTS
);
// ---------

struct checkerboard_t {
	// Points to the top row of the bottommost square.
	// Should technically be the unsigned `seg_t` type, *especially* since it's
	// used in comparisons. Luckily, all values we compare remain in the
	// "negative" range from 0x8000 to 0xFFFF and don't cross over into
	// positive values, which makes signed comparisons still correct.
	int16_t seg_bottom;

	// Not vram_offset_t, as they're relative to the custom segments calculated
	// from [seg_bottom].
	vram_byte_amount_t off_bottom;
	vram_byte_amount_t off_top;

	union {
		struct {
			uint8_t vo_x_of_dark; // ACTUAL TYPE: vram_offset_t

			// ZUN bloat: Doesn't make sense with any value other than 2.
			uint8_t loops;
		} var;
		uint16_t both;
	} u1;
};

extern checkerboard_t checkerboard;

#define checkerboard_vo_x_flip(v) { \
	/* Overly clever trick that relies on the position for the first and \
	 * second square not sharing any bits. */ \
	static_assert((PLAYFIELD_LEFT & (PLAYFIELD_LEFT + CHECKERBOARD_W)) == 0); \
	v ^= (PLAYFIELD_VRAM_LEFT | (PLAYFIELD_VRAM_LEFT + CHECKERBOARD_VRAM_W)); \
}

// ZUN bloat: Turbo C++ 4.0J even warns that the result should be unsigned
// because [SEG_PLANE_B] doesn't fit into a signed variable, and then
// automatically treats it as unsigned. Therefore, we need this additional cast
// to retain the signed comparisons from ZUN's original code.
#define grcg_segment_(x, y) static_cast<int16_t>(grcg_segment(x, y))

void near playfield_checkerboard_grcg_tdw_update_and_render(void)
{
	#define loops_and_vo_x	_BX
	#define loops         	_BH
	#define vo_x          	_BL

	// Not vram_offset_t, as it's relative to the custom segments calculated
	// from [seg_bottom].
	#define vram_off static_cast<int16_t>(_DI)

	// Same signedness issue as with [seg_bottom].
	#define vram_seg static_cast<int16_t>(_DX)

	grcg_setcolor_direct_constant(0);
	loops_and_vo_x = checkerboard.u1.both;

	// Bottom row
	while(1) {
		vram_seg = checkerboard.seg_bottom;
		vram_off = loops_and_vo_x;
		vram_off &= 0xFF; // Isolate [vo_x], ignoring the loop count
		vram_off += checkerboard.off_bottom;
		goto put;

		// Top row
		do {
			vram_seg = grcg_segment(0, PLAYFIELD_TOP);
			vram_off = loops_and_vo_x;
			vram_off &= 0xFF; // Isolate [vo_x], ignoring the loop count
			vram_off += checkerboard.off_top;
			goto put;

			// Fully visible, regular rows within the playfield
			do {
				vram_off = loops_and_vo_x;
				vram_off &= 0xFF; // Isolate [vo_x], ignoring the loop count
				vram_off += ((CHECKERBOARD_H - 1) * ROW_SIZE);

			put:
				_ES = vram_seg;
				do {
					_CX = ((PLAYFIELD_W / CHECKERBOARD_W) / 2);
					put_loop: {
						*reinterpret_cast<dots_t(CHECKERBOARD_W) __es *>(
							vram_off
						) = _EAX;
						vram_off += (CHECKERBOARD_VRAM_W * 2);
						asm { loop put_loop; }
					}
				} while((vram_off -= (ROW_SIZE + PLAYFIELD_VRAM_W)) >= 0);

				// Move [vram_seg] to the next column and row, and continue if
				// we haven't reached the top of the playfield yet.
				checkerboard_vo_x_flip(vo_x);
				vram_seg -= ((CHECKERBOARD_H * ROW_SIZE) / 16);
			} while(vram_seg > grcg_segment_(0, PLAYFIELD_TOP));

			// If we came here from the top row, [vram_seg] is in fact exactly
			// equal to this value. Otherwise, jump there.
		} while(vram_seg != grcg_segment_(0, (PLAYFIELD_TOP - CHECKERBOARD_H)));

		loops--;
		if(FLAGS_ZERO) {
			break;
		}

		grcg_setcolor_direct_constant(1);
		vo_x = checkerboard.u1.var.vo_x_of_dark;
		checkerboard_vo_x_flip(vo_x);
	}

	checkerboard.seg_bottom -= (CHECKERBOARD_VO_SPEED / 16);
	checkerboard.off_bottom += CHECKERBOARD_VO_SPEED;
	if(checkerboard.seg_bottom < grcg_segment_(
		0, (PLAYFIELD_BOTTOM - CHECKERBOARD_H)
	)) {
		// The bottom row would be (CHECKERBOARD_H + CHECKERBOARD_SPEED) pixels
		// high on the next frame, which means that we've fully scrolled the
		// bottommost square onto the playfield during this frame. Start the
		// new frame at the CHECKERBOARD_SPEED offset, and flip the colors
		// accordingly.
		checkerboard.seg_bottom = grcg_segment(
			0, (PLAYFIELD_BOTTOM - CHECKERBOARD_SPEED)
		);
		checkerboard.off_bottom = CHECKERBOARD_VO_SPEED;
		checkerboard_vo_x_flip(checkerboard.u1.var.vo_x_of_dark);
	}
	checkerboard.off_top -= CHECKERBOARD_VO_SPEED;
	if(FLAGS_SIGN) {
		// Scrolled the top row off the playfield, so we start a new one at the
		// bottom of a full square in the next frame. No color flip necessary
		// here, since we render from bottom to top. (There's also always a
		// half-scrolled square at the bottom whenever we get here.)
		checkerboard.off_top = ((CHECKERBOARD_H - 1) * ROW_SIZE);
	}

	#undef vram_seg
	#undef vram_off
	#undef vo_x
	#undef loops
	#undef loops_and_vo_x
}

// ZUN's object for this code segment also held the two playfield fills at the
// end of main_013_TEXT, and this one is the segment's new tail (kb/codegen
// 0148 pushed the th04/hardware/grcg_fill_rows.asm include that used to sit
// behind it into CHECKERB_TEXT). #pragma codeseg puts it back into
// main_013_TEXT from this object, which costs no new translation unit and no
// Tupfile.lua line; the group has to be named because the call below is NEAR
// into a segment this object does not otherwise touch.
//
// THIS FILE MUST NOT DECLARE playfield_fill() ABOVE THIS POINT, and must not
// include a header that does: #pragma codeseg binds a function to a segment at
// its FIRST DECLARATION, so a declaration read under the default CHECKERB_TEXT
// would emit the definition 0xE bytes late, into CHECKERB_TEXT, and the build
// would still link and run (kb/codegen 0155). th04/main/boss/bg.cpp declares
// it locally for its own callers, which is a different translation unit.
//
// -k- is already in force from the top of this file: the original has no stack
// frame at all, `push di` is its first instruction, and that push is Turbo
// C++'s own, inserted because the grcg_fill_playfield_rows_at() macro writes
// _DI (kb/codegen 0050).
#pragma codeseg main_013_TEXT main_01

// The bomb backdrop's playfield fill, and the FIRST of this segment's three
// C++ functions: it fills the 40 rows above and the 54 rows below the 274-row
// band that th04/main/player/bombchar.cpp paints the bomb character portrait
// into, which is what its hand name records. Both callers are already C++, in
// bombchar.cpp, which is also where it is declared -- so the kb/codegen/0123
// zero-byte alias the dump needed for the linker's underscore-decorated
// spelling goes away with the body: `extern "C"` publishes exactly that name
// from here.
extern "C" void near playfield_fillm_0_40_384_274(void)
{
	grcg_fill_playfield_rows_at(  0, 40);
	grcg_fill_playfield_rows_at(314, 54);
}

// Alignment padding after the function, in the original. main_013_TEXT is
// `word public`, and this is the byte that kept the next function even. It has
// to be emitted here rather than left to the assembler, and it lands in source
// order (kb/codegen/0161) -- same device as th04/main/boss/colorfill.cpp.
#pragma codestring "\x90"

// The whole-screen fill that stage_setup() in th04/main/stage/setup_main.cpp
// calls twice while setting a stage up. Its body, and the five macros it needs,
// are shared with TH05 verbatim: the same function sits at 0xCFEE in
// th05_main.asm's BOMB_BG_TEXT, byte-identical, and is compiled from this
// same file by th05/main/player/bombchar.cpp. Nothing here selects a game.
//
// `-k-` is already in force from the top of this file, which is what the
// shared body needs; TH05 brackets its own include with it.
#include "th04/main/graph2pg.cpp"

// Fills the entire playfield with the current GRCG tile register, assuming TDW
// mode. Unlike TH05's boss_bg_fill_col_0(), it neither enables nor disables the
// GRCG; both are the caller's job.
extern "C" void near playfield_fill(void)
{
	grcg_fill_playfield_rows_at(0, PLAYFIELD_H);
}

#pragma codeseg
