// Its own object, for the same two reasons th04/gameover.cpp is one — and it
// has to come before that one in the link list, because thicklasers_render()
// sits ahead of the GAME OVER screen in EXECL_TEXT's original layout and TLINK
// concatenates a segment's contributions in link order with th04_main.asm
// first.
//
// The segment is named here rather than left to Turbo C++'s basename default
// (kb/codegen/0105), which would open a LASER_R_TEXT of its own. No `-zP`: the
// two sibling objects in this segment carry no group pragma and the map still
// places them in MAIN_01, because th04_main.asm's own `main_01 group` line
// already lists EXECL_TEXT.
//
// `-G` because the original sets its frame pointer up with three separate
// instructions rather than the one-instruction form the size-optimizing
// default emits (kb/codegen/0011). That switch is per-object, which is why
// this body is not simply appended to th04/main/gameover.cpp: every function
// there wants the default.
#pragma option -zCEXECL_TEXT -G

#include "th04/main/bullet/laser_render.cpp"
