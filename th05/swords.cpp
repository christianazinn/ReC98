/* ReC98
 * -----
 * swords_add() and swords_update() out of the 3rd part of code segment #3 of
 * TH05's MAIN.EXE
 */

// ITS OWN OBJECT, on the th05/itmadd.cpp model but for a different reason.
// The two bodies were the tail `include` of main_035_TEXT's dump
// contribution, and th05/main035.cpp is the segment's next contribution, so
// kb/codegen/0112's cheaper wrapper route -- an `#include` at the front of
// that object -- would land them at the same address. kb/codegen/0119 is
// satisfied for it too: 0x194 is EVEN, so the `-a2` pad under
// yumeko_update()'s jump table keeps its parity either way.
//
// What rules it out is th05/main/bullet/sword.hpp. It is unguarded, and
// th05/main/boss/b5.cpp -- the whole of th05/main035.cpp -- already includes
// it. Reaching it from a body at the front of that object either collides on
// `struct sword_t` or forces the header, and the four it pulls in, up the
// include order of a translation unit that is already matched. A separate
// object costs one Tupfile.lua line, which must come BEFORE
// th05/main035.cpp because TLINK lays a segment's contributions out in link
// order.
//
// The segment name is spelled out because this wrapper's basename would
// otherwise supply it (kb/codegen/0105), and the group with it, because every
// call in here leaves the segment and Turbo C++ rejects `-zP` once a TU has
// emitted any code (kb/codegen 0104 + 0138).
#pragma option -zCmain_035_TEXT -zPmain_03

#include "th05/main/bullet/swords_add_update.cpp"
