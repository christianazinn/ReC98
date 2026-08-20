/// Single-color playfield fill for a boss backdrop
/// -----------------------------------------------
/// Kurumi's [boss_backdrop_colorfill] callback, and the entire root
/// contribution to th04_main.asm's `main_TEXT`. It fills whole playfield rows
/// through the GRCG's TDW mode, which is why it goes through
/// th04/hardware/grcg_fill_rows.asm rather than any of master.lib's rectangle
/// blitters — and like every other member of that callback family it is called
/// with the GRCG already set up.
///
/// The row-fill half is NOT a family property, and saying it was is the one
/// thing round 18 found wrong here. Of the 11 [boss_backdrop_colorfill]
/// members across both games, four never reach grcg_fill_rows.asm's macro at
/// all or reach it beside a hand-rolled `stosd` loop: reimu_marisa_ and
/// mai_yuki_ share th04/hardware/fillm64-56_256-256.asm, which fills AROUND a
/// 256x256 rect, and yuuka5_ and sara_ mix one GRCG_FILL_PLAYFIELD_ROWS with
/// loops of their own. fillm64-56_256-256.asm is the same module this file's
/// own note once cited as the reason kurumi_backdrop_colorfill could not be
/// lifted, so the counter-example was already in view.
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
