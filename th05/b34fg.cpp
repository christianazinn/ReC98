// Its OWN object rather than an `#include` at the front of th05/b6cbull.cpp,
// which is MIDBOSSX_TEXT's next contribution and would otherwise be the host.
// That route costs one Tupfile.lua line less and two hoisted headers more:
// th05/main/boss/bosses.hpp and th05/main/boss/b3puppet.hpp are needed here
// and are NOT in scope at the top of that object, where th05/main/midboss/
// m4.cpp and th05/main/bullet/b4balls_render.cpp compile first and are
// already matched. Putting declarations in scope earlier for already-matched
// code has moved a switch dispatch before, and a per-function funcdiff cannot
// see it.
//
// Everything th05/b6cbull.cpp says about MIDBOSSX_TEXT applies here too: it
// is a harness carve of the head of the original main_0_TEXT
// (kb/codegen/0080), so the segment has to be named explicitly rather than
// left to the basename default (kb/codegen/0105), and TLINK lays a segment's
// contributions out in link order with th05_main.asm first. This object is
// listed ahead of th05/b6cbull.cpp, so the lifted bodies land back at the
// addresses they had.
#pragma option -zCMIDBOSSX_TEXT -zPmain_01

#include "th05/main/midboss/m1_render.cpp"
#include "th05/main/boss/b1_fg.cpp"
#include "th05/main/midboss/m2.cpp"
#include "th05/main/boss/b2_fg.cpp"
#include "th05/main/midboss/m3.cpp"
#include "th05/main/boss/b3puppet_render.cpp"
#include "th05/main/boss/b3_fg.cpp"
#include "th05/main/boss/b4_pair_fg.cpp"
