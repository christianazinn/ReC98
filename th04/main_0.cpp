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
// reproduces from the order of these #includes: sub_10ABF() first -- it was
// the tail of th04_main.asm's own contribution and the C++ grows backwards
// into the hole it left -- then player_render(), which took the `include` that
// used to end the contribution. Both files reach th04/main/player/player.hpp
// -- update.cpp through th04/main/player/shot.hpp -- and neither has to
// decline it, because MATCH-TH05-MAIN-TAILS-1 guarded that header for the
// same collision in th05/shot_inv.cpp. Nothing else in the two include sets
// intersects (kb/codegen/0129).
#include "th04/main/player/update.cpp"
#include "th04/main/player/render.cpp"
