// The code segment name is left to Turbo C++'s basename default, which makes
// this object contribute to th04_main.asm's main_012_TEXT (kb/codegen/0105).
// That segment has no other contribution, so TLINK -- which lays a segment's
// contributions out in link order, with the root dump first -- puts this one
// at its tail by construction, which is where the function below already was.
// Its Tupfile.lua line is therefore append-anywhere, unlike th04/main_01.cpp's.
// (kb/codegen/0112 + 0114.)
//
// main_012_TEXT is in group main_01, and the two other main_01 wrappers in
// this binary (th04/boss_bg.cpp, th04/main_01.cpp) both declare it, so this
// one does too. The group pragma lives here rather than in the included file:
// it only takes effect before any code is generated. (kb/codegen/0112, trap 0;
// kb/codegen/0138.)
#pragma option -zPmain_01

#include "th04/main/stage/reset.cpp"
