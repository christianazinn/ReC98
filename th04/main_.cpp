// The code segment name is left to Turbo C++'s basename default, which makes
// this object contribute to th04_main.asm's main__TEXT (kb/codegen/0105).
// That segment has no other contribution, so TLINK -- which lays a segment's
// contributions out in link order, with the root dump first -- puts this one
// at the tail of the root's contribution by construction, which is exactly
// where the two `include`s this parcel deleted used to end.
// (kb/codegen 0112 + 0114.)
//
// main__TEXT is in group main_01, and every other root-level wrapper in this
// binary that names that group declares it, so this one does too. The group
// pragma lives here rather than in the included files: it only takes effect
// before any code is generated. (kb/codegen/0112, trap 0; kb/codegen/0138.)
#pragma option -zPmain_01

// Address order inside ZUN's own object for this segment, which is what TLINK
// reproduces from the order of these #includes: enemies_render() first, then
// player_invalidate(). Neither file includes th04/main/tile/tile.hpp or
// th04/main/enemy/size.hpp for the other's sake -- both are unguarded, and a
// second expansion in one object rejects the `static const` objects they
// declare (kb/codegen/0129).
#include "th04/main/enemy/render.cpp"
#include "th04/main/player/invalidate.cpp"
