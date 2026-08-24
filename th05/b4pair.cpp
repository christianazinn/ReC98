/* ReC98
 * -----
 * mai_yuki_update() and the two cheeto patterns above it, out of the 3rd part
 * of code segment #3 of TH05's MAIN.EXE
 */

// ITS OWN OBJECT, and not a backwards prepend into th05/b4mai.cpp, because of
// the one-byte `-a2` pad in front of @mai_yuki_update$qv's two switch tables.
// That function takes its pad on the OPPOSITE parity from the three jump
// tables already inside b4mai.obj -- measured, both directions, on that very
// object -- so no single object can reproduce all four pads at once. The
// Tupfile.lua line therefore comes BEFORE th05/b4mai.cpp's: TLINK lays a
// segment's contributions out in link order (kb/codegen 0112 + 0114), and the
// two patterns in front of the tail proc are what puts it at the ODD object
// offset its pad needs. Full measurement in
// state/notes/th05-main-mai-update.md.
//
// The segment name is spelled out because this wrapper's basename would
// otherwise supply it (kb/codegen/0105), and the group with it, because the
// body reaches BOSS_BG_TEXT, PLAYFLD_TEXT, STD_TEXT and MIDBOSSX_TEXT, and
// Turbo C++ rejects `-zP` once a TU has emitted any code
// (kb/codegen 0104 + 0138).
#pragma option -zCmain_035_TEXT -zPmain_03

#include "th05/main/boss/b4_pair.cpp"
