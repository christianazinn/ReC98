// ZUN's object for this code segment held yuuka5_fg_render() followed by
// kurumi_backdrop_colorfill(). Only the former is C++: the latter is a
// GRCG_FILL_PLAYFIELD_ROWS pair with an ES:DI __usercall callee, the same
// hand-written shape as th04/hardware/fillm64-56_256-256.asm, so it stays in
// the ASM tail that kept the original `main_TEXT` name. (kb/codegen/0112)
//
// The group pragma lives here rather than in the included file: it only takes
// effect before any code is generated. (kb/codegen/0112, trap 0)
#pragma option -zPmain_01

#include "th04/main/boss/b5r.cpp"
