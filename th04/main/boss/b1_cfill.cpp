/// Single-color playfield fill for Orange's boss backdrop
/// ------------------------------------------------------
/// Orange's [boss_backdrop_colorfill] callback, and the last of the three
/// bodies that made up th04_main.asm's contribution to HUD_PNT_TEXT. Like
/// every other member of that callback family it is called with the GRCG
/// already set up, and it fills whole playfield rows through the GRCG's TDW
/// mode -- which is why it goes through th04/hardware/grcg_fill_rows.asm
/// rather than any of master.lib's rectangle blitters.
///
/// (#included from th04/main/hud/points.cpp, between
/// th04/main/boss/b4m_fg.cpp and th04/main/score_extend.cpp, which is this
/// function's original address order. See b4m_fg.cpp's docblock for the shape
/// of the whole lift.)
///
/// THE HAND-WRITTEN SCREEN ON THIS ROW WAS A FALSE POSITIVE, and that is worth
/// recording where the body is. tools/pi-audit/carve_free_tails.py filed
/// HUD_PNT_TEXT as free-but-hand-written and "NOT LIFTABLE, do not rank this row",
/// on the single tell "out to a port -- direct PC-98 port programming". The
/// tell fires on a MACRO EXPANSION, not on hand-written code: the four
/// `OUT DX, AL`s are what th04/hardware/grcg.hpp's
/// grcg_setcolor_direct_constant(0) emits, character for character, and the
/// two row fills are its grcg_fill_playfield_rows_at(). The same two-call
/// skeleton is oracle-verified twice over already, in
/// kurumi_backdrop_colorfill() (th04/main/boss/colorfill.cpp) and in three
/// functions in th05/main/boss/colorfill.cpp. The screen's own instructions
/// say silence is an ABSTENTION rather than a clearance; this row shows that a
/// single port-write tell is not evidence either, when the port write has a
/// spelling in a header the tree already uses.
///
/// The two fills leave the 128 rows between them for Orange's stage backdrop
/// image.

// `-k-`: the original has no stack frame at all -- `PUSH DI` is the first
// instruction, not `PUSH BP`. BRACKETED and restored at the bottom, because
// this file is #included in the middle of a translation unit whose remaining
// functions are already matched WITH frames, and a #pragma option keeps
// applying to everything generated after it (kb/codegen/0011 + 0112 trap 1).
// Exactly th04/main/boss/colorfill.cpp's trap 1.
//
// Note what -k- does NOT have to hide here, and what
// th05/main/boss/colorfill.cpp's two hand-rolled fills do: that file keeps DI
// invisible to the compiler with raw `db 0x57` / `db 0x5F` bytes, because its
// port writes come BEFORE the save and Turbo C++ will only ever emit its own
// `PUSH DI` at the very top of a function (kb/codegen/0050). Orange saves DI
// first, which is precisely where the compiler puts it, so the ordinary
// spelling below is enough.
#pragma option -k-

// As th04/hardware/grcg.hpp's grcg_fill_playfield_rows_at(), but staging the
// segment value through DX rather than letting Turbo C++ pick the scratch
// register for `_ES = <constant>` -- which it does by picking AX.
//
// `[measured]`, and worth stating because it is invisible to a length check:
// the two spellings are the SAME SIX BYTES LONG, `B8 imm16` / `8E C0` against
// `BA imm16` / `8E C2`. An object whose every function came out the right
// length can still be wrong here, which is the per-function-diff blind spot
// kb/codegen/0119 warns about, approached from the other side. The original's
// FIRST fill goes through DX and its second through AX;
// th04/hardware/grcg.inc's ASM macro takes the register as its third argument
// for exactly this reason, and the dump spelled this call with dx as that
// third argument (the invocation itself is gone from th04_main.asm now: this
// commit is what replaced it).
//
// [inferred] What ZUN's source did to get DX here is NOT recoverable from the
// binary. The grcg_setcolor_direct_constant(0) between the two fills writes DX
// as well, but that is not what causes it: a plain pair of
// grcg_fill_playfield_rows_at() calls -- the spelling that matched
// kurumi_backdrop_colorfill() and all three of th05/main/boss/colorfill.cpp's
// fills -- picks AX for both, with the same setcolor in between. So this macro
// reproduces the bytes rather than reconstructing the source. Two statements
// rather than the shorter `_ES = (_DX = ...)`: the chained form materializes
// the constant in AX first and costs a third `MOV`.
#define grcg_fill_playfield_rows_at_via_dx(y, num_rows) { \
	_DX = (SEG_PLANE_B + ((((y) + PLAYFIELD_TOP) * ROW_SIZE) / 16)); \
	_ES = _DX; \
	_DI = ((((num_rows) - 1) * ROW_SIZE) + PLAYFIELD_VRAM_LEFT); \
	grcg_fill_playfield_rows(); \
}

void pascal near orange_backdrop_colorfill(void)
{
	grcg_fill_playfield_rows_at_via_dx(0, 120);

	// Re-zeroes all four tile registers between the two fills, keeping the
	// current (TDW) mode and reenabling interrupts on the way out -- so it is
	// grcg_setcolor_direct_constant(), not master.lib's grcg_setcolor() and
	// not th05/main/boss/colorfill.cpp's GRCG_TDW_COL_0_NOINT(), which
	// restores the previous interrupt state instead.
	grcg_setcolor_direct_constant(0);

	grcg_fill_playfield_rows_at(248, 120);
}

#undef grcg_fill_playfield_rows_at_via_dx

// Restores the translation unit's baseline for everything after this point:
// th04/main/score_extend.cpp, score_reset.cpp, lives.cpp, bombs.cpp and
// points.cpp's own function are all already matched with a standard frame, and
// the base cflags pass no -k.
#pragma option -k
