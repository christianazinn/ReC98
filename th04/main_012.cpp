// The code segment name is left to Turbo C++'s basename default, which makes
// this object contribute to th04_main.asm's main_012_TEXT (kb/codegen/0105).
// That segment has no other contribution, so TLINK -- which lays a segment's
// contributions out in link order, with the root dump first -- puts this one
// at its tail by construction, which is where the function below already was.
// Its Tupfile.lua line is therefore append-anywhere, unlike th04/main_01.cpp's.
// (kb/codegen/0112 + 0114.)
//
// main_012_TEXT is in group main_01, and every other root-level wrapper in
// this binary that names that group declares it, so this one does too. There
// are `[measured]` EIGHT of them, not the two this comment used to claim:
// th04/boss_5r.cpp, th04/boss_bg.cpp, th04/boss_fg.cpp, th04/mai.cpp,
// th04/main_01.cpp, th04/map.cpp, th04/mb_dfr.cpp and th04/shot_inv.cpp.
// (Naming review round 16 section 6.4. Two of the eight are easy to miss with
// a grep for `option -zPmain_01`: th04/map.cpp and th04/shot_inv.cpp spell it
// after a -zC on the same #pragma line.)
//
// The group pragma lives here rather than in the included file: it only takes
// effect before any code is generated. (kb/codegen/0112, trap 0;
// kb/codegen/0138.)
#pragma option -zPmain_01

// Address order inside ZUN's own object for this segment, which is what TLINK
// reproduces from the order of these #includes: player_shot_level_update(),
// elly_fg_render(), then stage_state_reset(). The first body is frameless,
// unlike the two following ones, so restore the command-line frame option
// immediately after it (kb/codegen/0011 + 0112).
#pragma option -k-
#include "th04/main/player/shot_level.cpp"
#pragma option -k.

#include "th04/main/boss/b3_fg.cpp"
#include "th04/main/stage/reset.cpp"
