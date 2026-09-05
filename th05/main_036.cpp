/* ReC98
 * -----
 * 6th part of code segment #3 of TH05's MAIN.EXE
 */

// The name is TH04's for the same block: th04/main_036.cpp carries
// `-zPmain_03` and boss-EXTRA update code, and this segment holds
// exalice_update() and midboss5_update(), the same role in the same binary
// position. `-zC` is spelled out rather than left to the basename default
// (kb/codegen/0105) because that is what this binary's three other main_03
// wrappers do.
//
// The group is kb/codegen/0104 + 0080's step 2, and it is not decorative:
// midboss5_update() below stores `offset pattern_blue_spreads` into a near
// function pointer and reads three more `offset`s out of the dump's own
// [MIDBOSS5_PATTERNS_PHASE_1]. An `offset` only resolves against the group
// base when the object NAMES the group; without this, the code links and puts
// the wrong word into the callback.
#pragma option -zCmain_036_TEXT -zPmain_03

// The tail of the segment's middle block, which the kb/codegen/0080 three-way
// carve of main_036_TEXT left with no C++ contribution at all. TLINK lays a
// segment's contributions out in link order with the root dump first, so this
// object lands at the middle block's tail by construction -- which is where
// these six functions already were (kb/codegen 0112 + 0114). Its Tupfile.lua
// line is therefore append-anywhere.
#include "th05/main/midboss/m5_updt.cpp"
