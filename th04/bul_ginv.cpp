// Its OWN object rather than an `#include` at the head of th04/tile.cpp, which
// is the next contribution to this segment and would have been the cheaper
// wiring by one Tupfile.lua line.
//
// [measured] th04/tile.cpp CANNOT host it. Tup compiles that file exactly once,
// with -DGAME=4 into obj/th04/tile.obj, and links that SAME object into both
// th04/main.exe and th05/main.exe -- so `#if (GAME != 5)` around the include is
// always true there and TH05's link failed with `bullets_and_gather_invalidate()
// defined in module th05_main.asm is duplicated in module th04/tile.cpp`. A
// game-conditional inside a wrapper only works when each game has its own
// wrapper, the way th04/demo.cpp and th05/demo.cpp do; this segment has one
// wrapper for two binaries, and the shared module is still ASM in TH05.
//
// TLINK concatenates a segment's contributions in link order with
// th04_main.asm first, and this object is listed ahead of th04/tile.cpp, so the
// body lands back at the tail of the root contribution where it started and
// nothing in th04/main/tile/tile.cpp moves.
//
// The segment is named here rather than left to Turbo C++'s basename default
// (kb/codegen/0105), which would open a BULLETS_GATHER_INV_TEXT of its own.
// `-zP` matches the sibling object in this segment, th04/tile.cpp, which sets
// it inside th04/main/tile/tile.cpp.
#pragma option -zCTILE_TEXT -zPmain_01

#include "th04/main/bullets_gather_inv.cpp"
