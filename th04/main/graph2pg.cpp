/// The whole-screen page fill, shared between TH04 and TH05
/// ---------------------------------------------------------
/// The TH04/TH05 answer to TH01-TH03's graph_clear_both(): the same
/// access-page-1-then-0 shape, but hand-rolled through the GRCG's TDW mode
/// instead of two graph_clear() calls, which is why the color it leaves behind
/// is 1 rather than 0.
///
/// The two binaries hold this function at two unrelated addresses, in two
/// segments with different names and different alignments, so it cannot be one
/// object -- but it IS one body, byte-identical in both originals. So this file
/// is the body and nothing else: it #includes NOTHING and declares nothing,
/// because both of its includers already have `disable`/`outportb` (through
/// platform/x86real/flags.hpp), SEG_PLANE_B and PLANE_SIZE (through planar.h),
/// and both put it inside their own `#pragma option -k-` -- the original has no
/// stack frame at all, and `push di` below is raw bytes rather than the
/// compiler's own.
///
///   TH04 MAIN.EXE  main_013_TEXT, group main_01, 0x12024  (th04/main/checkerb.cpp)
///   TH05 MAIN.EXE  BOMB_BG_TEXT,  group main_01, 0xCFEE   (th05/main/player/bombchar.cpp)
///
/// Its callers are the two call sites in the shared stage_setup() compiled from
/// th04/main/stage/setup_main.cpp for both games. They reach it through a
/// `procdesc near` in the segment above. It runs once
/// before the stage's palette is loaded and once after it has been faded to
/// black.

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
