// MUGETSU_TEXT's THIRD of FOUR C++ objects. That segment is th04_main.asm's
// kb/codegen/0080 carve off main_033_TEXT's head, taken by
// MATCH-TH04-MAIN-033-HEAD-CARVE so that the hand-written
// th04/main/pointnum/digits.asm `include` would stop blocking the fifteen procs
// above it; state/notes/th04-main-033-head.md records why the head took the new
// name. TLINK lays a segment's contributions out in link order with the root
// dump first (kb/codegen/0114), and the root's contribution to this segment is
// now enemies_update() and nothing else, so these four objects land behind it
// in the address order these functions already had.
//
// ALL FOUR Tupfile.lua lines are POSITION-CRITICAL and must stay in the order
// bx1_gath, bx1_pose, bx1_ptn, bx1_upd. th04/main/boss/bx1_gath.cpp's header
// comment carries the measurement: the pose pair must be compiled without
// th04/main/bullet/bullet.hpp in the translation unit or its dense `switch`
// heads stage their selector through AX, and the `-a2` table parities of
// mugetsu_1812A(), mugetsu_1821E() and mugetsu_update() then fix the other two
// boundaries. 0x8C + 0x259 + 0x3D7 + 0x303 = 0x9BF, unchanged.
//
// FOUR OBJECTS rather than one `#pragma codeseg` section inside
// th04/main_033.cpp (kb/codegen/0155), and the reason is measured: that object
// already emits kurumi_update()'s `-a2` padding byte, which
// th04/main/boss/b2_updt.cpp records as depending on an ODD prefix ahead of it,
// and folding 0x9BF of Mugetsu in would be kb/codegen/0119 aimed at 0x14A8 of
// already-matched code.
//
//
// This object: the four `switch(mugetsu_pose_func())` patterns and the fight's
// helpers, mugetsu_18314() through mugetsu_186B9().
// `-zC` because the basename does not name the segment (kb/codegen/0105), and
// `-zPmain_03` because the generated `cs:` tables in this segment would
// otherwise be framed on the segment rather than on the group
// (kb/codegen/0104), and because every near call this fight makes leaves this
// segment.
#pragma option -zCMUGETSU_TEXT -zPmain_03

#include "th04/main/boss/bx1_ptn.cpp"
