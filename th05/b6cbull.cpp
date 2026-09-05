// Its OWN object rather than an `#include` at the front of th05/stages.cpp,
// which is MIDBOSSX_TEXT's next contribution and would otherwise be the host.
// That route costs one Tupfile.lua line less and five hoisted headers more:
// th05/main/boss/b6_fg.cpp and th05/main/stage/stages.cpp compile after it in
// that object and are already matched, and putting declarations in scope
// earlier for already-matched code has moved a switch dispatch before.
//
// Everything th05/stages.cpp says about MIDBOSSX_TEXT applies here too: it is
// a harness carve of the head of the original main_0_TEXT (kb/codegen/0080),
// so the segment has to be named explicitly rather than left to the basename
// default (kb/codegen/0105), and TLINK lays a segment's contributions out in
// link order with th05_main.asm first. This object is listed ahead of
// th05/stages.cpp, so the lifted body lands back at the address it had.
#pragma option -zCMIDBOSSX_TEXT -zPmain_01

#include "th05/main/midboss/m4.cpp"
#include "th05/main/bullet/b4balls_render.cpp"
#include "th05/main/boss/b4_solo_fg.cpp"
#include "th05/main/bullet/swords_render.cpp"
#include "th05/main/boss/b5_fg.cpp"
#include "th05/main/bullet/b6_custombullets_render.cpp"
