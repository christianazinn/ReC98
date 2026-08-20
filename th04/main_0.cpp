// The code segment name is left to Turbo C++'s basename default, which makes
// this object contribute to th04_main.asm's main_0_TEXT (kb/codegen/0105).
// The segment's only other contribution is th04\scoreupd.asm's, which is
// 0 bytes long, so TLINK -- which lays a segment's contributions out in link
// order, with the root dump first -- puts this one at the tail of the root's
// contribution by construction, which is exactly where the `include` this
// parcel deleted used to end. (kb/codegen 0112 + 0114.)
//
// main_0_TEXT is in group main_01, and every other root-level wrapper in this
// binary that names that group declares it, so this one does too. The group
// pragma lives here rather than in the included file: it only takes effect
// before any code is generated. (kb/codegen/0112, trap 0; kb/codegen/0138.)
#pragma option -zPmain_01

// Address order inside ZUN's own object for this segment, which is what TLINK
// reproduces from the order of these #includes: sub_10988() first, then
// sub_10ABF(), then player_render(). Each was the tail of th04_main.asm's own
// contribution when it left, in reverse of that order, and the C++ grew
// backwards into each hole. sub_10988() was the third and last: THE ROOT
// DUMP'S CONTRIBUTION TO main_0_TEXT IS NOW EMPTY, and th04\scoreupd.asm's is
// 0 bytes long, so this object is the segment's only source of bytes.
//
// All three files reach th04/main/player/player.hpp -- update.cpp through
// th04/main/player/shot.hpp -- and none has to decline it, because
// MATCH-TH05-MAIN-TAILS-1 guarded that header for the same collision in
// th05/shot_inv.cpp. The rest of the three include sets is disjoint, and
// miss.cpp keeps it that way DELIBERATELY: it declares its own externs for
// th02/snd/snd.h and libs/master.lib/pc98_gfx.hpp rather than including
// either, since update.cpp and render.cpp expand those two below it and
// neither is guarded (kb/codegen/0129).
#include "th04/main/player/miss.cpp"
#include "th04/main/player/update.cpp"
#include "th04/main/player/render.cpp"
