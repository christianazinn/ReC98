/* ReC98
 * -----
 * Head half of code segment #3's ENM_POS_TEXT in TH04's MAIN.EXE
 */

// MB_UPD_TEXT is th04_main.asm's own new name for the head of what an earlier
// kb/codegen/0080 carve had already split off `B4M_UPDATE_TEXT` and called
// ENM_POS_TEXT: the midboss 1 and 3 update functions and their pattern
// helpers. ENM_POS_TEXT keeps the tail -- midbossx_update(), two generated
// switch table pairs and the hand-written gather-point render `include` -- so
// th04/enm_pos1.cpp and th04/enm_pos.cpp, that segment's two existing C++
// contributions, are not re-pointed and every byte keeps its address.
//
// This is the SECOND of the segment's two objects, and th04/mb_upd1.cpp is the
// first; the root dump contributes nothing here any more. That file records why
// the Stage 1 midboss's half cannot simply be folded in ahead of this one.
//
// WHY THIS ROW EXISTS AT ALL. `state/progress/th04.md` recorded these thirteen
// procs, and main_033_TEXT's seventeen, as blocked because their kb/codegen/0148
// boundary pushes are blocked -- the root is not the last contribution in either
// segment. That is true of the PUSH and says nothing about the segment: 0148 is
// one route of several, and a head rename reaches a root block whose tail is an
// `include` no matter who that module was written by. The authorship of the
// module at the bottom of the tail never mattered here.
//
// `-zC` because the basename does not name the segment (kb/codegen/0105), and
// `-zPmain_03` because the body's near calls -- bullet_template_tune() through
// its function pointer, and bullets_add_regular() -- leave this segment and
// resolve against the group (kb/codegen/0104).
#pragma option -zCMB_UPD_TEXT -zPmain_03

#include "th04/main/midboss/mb_upd.cpp"
