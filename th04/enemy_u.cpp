// enemies_update() as its own object at the head of MUGETSU_TEXT's C++
// contributions. Keeping this header set out of the four switch-bearing
// Mugetsu objects preserves their measured object-level codegen
// (kb/codegen/0171).
//
// `-zC` names the dump segment, and `-zPmain_03` keeps generated same-group
// references framed on the group.
#pragma option -zCMUGETSU_TEXT -zPmain_03

#include "th04/main/enemy/update.cpp"
