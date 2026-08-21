/* ReC98
 * -----
 * mai_update() out of the 3rd part of code segment #3 of TH05's MAIN.EXE
 */

// ITS OWN OBJECT, on the th05/swords.cpp model. th05/swords.cpp is the
// segment's first contribution after the root dump, so growing that object
// backwards -- kb/codegen/0112's cheaper wrapper route -- would put this
// function at swords_add()'s address rather than at its own. The Tupfile.lua
// line therefore comes BEFORE th05/swords.cpp: TLINK lays a segment's
// contributions out in link order (kb/codegen 0112 + 0114). Every later lift
// out of the block above grows THIS object backwards instead, at no further
// Tupfile.lua cost.
//
// The segment name is spelled out because this wrapper's basename would
// otherwise supply it (kb/codegen/0105), and the group with it, because the
// body reaches BOSS_TEXT, LASER_SC_TEXT and three main_01 segments, and Turbo
// C++ rejects `-zP` once a TU has emitted any code (kb/codegen 0104 + 0138).
#pragma option -zCmain_035_TEXT -zPmain_03

#include "th05/main/boss/b4_mai.cpp"
