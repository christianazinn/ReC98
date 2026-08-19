// Moved here out of th04/main/player/bomb.cpp, which is shared with TH04 and
// is no longer the first file in TH04's object. Same two values it always set.
#pragma option -zCPLAYER_B_TEXT -zPmain_01

// Address order inside PLAYER_B_TEXT, which is what TLINK reproduces from the
// order of these #includes: tile_ring_set_vo() was the last thing
// th05_main.asm contributed to this segment, so lifting it puts it at the
// FRONT of this object, ahead of the two functions that were already here
// (kb/codegen 0099 + 0112 + 0114).
//
// TH04 compiles the same file into an object of its own, th04/tile_set.cpp,
// where it lands in TILE_SET_TEXT instead. TH05's copy is the same 0x4F bytes
// and is never called, which is why IDA emitted it without a proc wrapper.
#include "th04/main/tile/set.cpp"
#include "th04/main/player/bomb.cpp"
