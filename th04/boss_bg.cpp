// The code segment name is left to Turbo C++'s basename default, which makes
// this object append to th04_main.asm's BOSS_BG_TEXT contribution — the
// original object's own segment, and the position mugetsu_gengetsu_bg_render()
// already occupied at its tail. No carve, no new segment name, no group-list
// edit. (kb/codegen/0112 + 0114; th05/boss_bg.cpp is the same one-liner for
// TH05's half.)
//
// The group pragma lives here rather than in the included file: it only takes
// effect before any code is generated. (kb/codegen/0112, trap 0)
#pragma option -zPmain_01

#include "th04/main/boss/bg.cpp"
