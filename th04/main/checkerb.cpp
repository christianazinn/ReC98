#pragma option -k-

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

// The whole-screen fill that sub_AED0 -- still ASM, in DEMO_TEXT -- calls twice
// while setting a stage up: once before the palette is loaded, once after it
// has been faded to black. It is the TH04/TH05 answer to TH01-TH03's
// graph_clear_both(): the same access-page-1-then-0 shape, but hand-rolled
// through the GRCG's TDW mode instead of two graph_clear() calls, which is why
// the color it leaves behind is 1 rather than 0.
//
// TH05's MAIN.EXE has a byte-identical twin at the end of its MB_INV_TEXT
// (sub_CFEE in th05_main.asm), still ASM. Whoever lifts that one should hoist
// this body into a shared file rather than copy it.

// DI IS HIDDEN FROM THE COMPILER HERE, for the reason
// th05/main/boss/colorfill.cpp spells out at length: the original saves DI
// *after* its port writes, and any mention of DI or _DI makes Turbo C++ put its
// own `PUSH DI` at the very top of the function instead (kb/codegen/0050). So
// the save, the restore and both `XOR DI, DI`s are raw bytes.
#define PUSH_DI() _asm { db 0x57; }
#define POP_DI()  _asm { db 0x5F; }
#define XOR_DI()  __emit__(0x33, 0xFF)

// Turbo C++ 4.0J's inline assembler is 16-bit only and can spell neither of
// these. th03/main/hitc_mrs.cpp emits the same `REP STOSD`.
#define REP_STOSD()  __emit__(0xF3, 0x66, 0xAB)
#define OUT_AX(port) __emit__(0xE7, port) /* OUT port, AX */

// grcg_setcolor_direct_constant(1), with a GC_TDW mode switch inside the same
// interrupt-free window. Neither of th04/hardware/grcg.hpp's helpers can be
// composed into this shape: the mode `out` sits between the `cli` and the tile
// register's port number, and grcg_setcolor_direct_constant() owns both the
// `cli` and the `sti`. 0x80 is GC_TDW, hardcoded the way
// grcg_setmode_rmw_inlined() hardcodes 0xC0 for GC_RMW -- naming it would mean
// pulling libs/master.lib/pc98_gfx.hpp into this translation unit for one
// constant.
#define grcg_setmode_tdw_and_setcolor_1() { \
	disable(); \
	_outportb_(0x7C, 0x80); \
	_DX = 0x7E; \
	outportb(_DX, 0xFF); \
	outportb(_DX, (_AL ^= _AL)); \
	outportb(_DX, _AL); \
	outportb(_DX, _AL); \
	enable(); \
}

// GRCG off, in the four-byte inline spelling the target uses rather than a call
// to master.lib's GRCG_OFF (kb/codegen/0061).
#define GRCG_OFF_INLINE() _asm { \
	db  	0x32, 0xC0; /* XOR AL, AL */ \
	out 	0x7C, al; \
}

void near graph_both_pages_fill_col_1(void)
{
	grcg_setmode_tdw_and_setcolor_1();
	PUSH_DI();
	_ES = SEG_PLANE_B;

	// A *word* OUT to a byte-wide port, in the original; the hardware ignores
	// AH. [inferred] a plain `int` page number in ZUN's source, which is what
	// master.lib's own graph_accesspage() macro takes.
	_AX = 1;
	OUT_AX(0xA6);
	XOR_DI();
	_CX = (PLANE_SIZE / 4);
	REP_STOSD();

	// The same two statements, in the other order -- ZUN wrote the page switch
	// and the counter the other way round for page 0.
	_AX ^= _AX;
	OUT_AX(0xA6);
	_CX = (PLANE_SIZE / 4);
	XOR_DI();
	REP_STOSD();

	POP_DI();
	GRCG_OFF_INLINE();
}

// Fills the entire playfield with the current GRCG tile register, assuming TDW
// mode. Unlike TH05's boss_bg_fill_col_0(), it neither enables nor disables the
// GRCG; both are the caller's job.
extern "C" void near playfield_fill(void)
{
	grcg_fill_playfield_rows_at(0, PLAYFIELD_H);
}

#pragma codeseg
