// ZUN's own object for the Stage 2 midboss's update half, which is a separate
// translation unit from th05/boss_4.cpp rather than the front of it. That is
// measured, not assumed: B4_UPDATE_TEXT starts at the ODD group offset 0x3685,
// so a C++ object placed at the segment start puts its `-a2` word-aligned jump
// tables on ODD segment offsets. Folding this code into the front of
// th05/boss_4.cpp therefore deleted the pad under louise_update()'s table
// (segment 0x1C51 -> 0x1C50, dispatch `cs:[bx+0x3FF0]` -> `cs:[bx+0x3FEF]`),
// even though the moved code itself was byte-identical. As its own object of
// odd length 0x2B3, it restores the even base th05/boss_4.cpp needs.
#pragma option -zCB4_UPDATE_TEXT -zPmain_03

#include "th05/main/midboss/m2_updt.cpp"
