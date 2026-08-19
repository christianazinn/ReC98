// The code segment name is left to Turbo C++'s basename default, which makes
// this object append to th04_main.asm's BOSS_FG_TEXT contribution — the
// original object's own segment, and the position gengetsu_fg_render()
// already occupied at its tail. carve_free_tails.py files that segment as
// BLOCKED, but that verdict is about the *neighbour* route only: the next
// contribution is th01/vplanset.cpp, one byte later and in no group. A new
// object needs neither, because TLINK concatenates a segment's contributions
// in link order with th04_main.asm first. No carve, no new segment name, no
// group-list edit. (kb/codegen/0112 + 0114; th04/boss_bg.cpp is the same
// one-liner for the background renderers.)
//
// The group pragma lives here rather than in the included file: it only takes
// effect before any code is generated. (kb/codegen/0112, trap 0)
#pragma option -zPmain_01

// items_render() was the tail `include` of this segment's dump contribution,
// so it goes FIRST here: the object grows backwards into the hole the dump
// leaves and every byte keeps its address (kb/codegen/0114).
#include "th04/main/item/render.cpp"
#include "th04/main/boss/fg.cpp"
