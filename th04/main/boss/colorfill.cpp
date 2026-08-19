/// Single-color playfield fill for a boss backdrop
/// -----------------------------------------------
/// Kurumi's [boss_backdrop_colorfill] callback, and the entire root
/// contribution to th04_main.asm's `main_TEXT`. Like every other member of that
/// callback family it fills whole playfield rows through the GRCG's TDW mode,
/// which is why it goes through th04/hardware/grcg_fill_rows.asm rather than
/// any of master.lib's rectangle blitters, and like every other member it is
/// called with the GRCG already set up.
///
/// TH05's twin is shinki_stage_backdrop_colorfill() in
/// th05/main/boss/colorfill.cpp, matched 2026-08-15. The two are NOT written as
/// one shared body: they fill different rows for different bosses and share
/// only the two-call skeleton, so a `#if (GAME == N)` would guard all four
/// arguments and leave nothing outside the guard. They ARE byte-comparable, and
/// that comparison is what cleared this function as compiler output rather than
/// hand-written assembly — see state/notes/kurumi_backdrop_colorfill.md.
///
/// (#included from th04/main/stage/stages.cpp, at the FRONT, which is
/// kb/codegen/0129's host-source form. `main_TEXT` held nothing but this
/// function and its trailing pad, and `STAGES_TEXT` begins in the same group at
/// the very next byte, so the C++ object grows backwards across the segment
/// boundary and every byte keeps its address: no carve, no new segment name, no
/// group-list edit and no Tupfile.lua line. `main_TEXT` is left behind as a
/// comment-only, ZERO-length segment, which keeps th04_main.asm's own
/// `main_01 group` line correct without an edit.)

// `-k-`: the original has no stack frame at all — `push di` is the first
// instruction, not `push bp`. BRACKETED and restored at the bottom, because
// this file is #included at the head of a translation unit whose remaining
// functions are already matched WITH frames, and a #pragma option keeps
// applying to everything generated after it (kb/codegen/0011 + 0112 trap 1).
// th05/main/boss/colorfill.cpp sets the same option unbracketed; it can,
// because nothing in its host follows it that wants the other setting.
#pragma option -k-

// The two fills leave the 192 rows between them for Kurumi's stage backdrop
// image — the same shape as Shinki's, with the filled and unfilled bands
// swapped end for end.
void pascal near kurumi_backdrop_colorfill(void)
{
	grcg_fill_playfield_rows_at(192, 192);
	grcg_fill_playfield_rows_at(  0,  80);
}

// Alignment padding after the function, in the original. It is the last byte of
// `main_TEXT`'s contribution, so it has to be emitted here rather than left to
// the assembler: `main_TEXT` is `byte public`, which means nothing pads it
// automatically. Same device as th05/main/boss/colorfill.cpp uses between its
// first two functions.
#pragma codestring "\x90"

// Restores the command line's setting for the rest of the host translation
// unit: the base cflags pass no -k, and every function after this point in
// th04/main/stage/stages.cpp is already matched with a standard frame.
#pragma option -k
