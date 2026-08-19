// Moved here out of th04/main/player/bomb.cpp, which is shared with TH05 and
// is no longer the first file in this object. Same two values it always set:
// `-zC` and `-zP` take effect only before any code is generated, and a second
// `-zC` after that is a hard error rather than a no-op, so the pragma has to
// live in the wrapper once a second file is #included ahead of the old one.
// (kb/codegen/0112 trap 0, kb/codegen/0138.)
#pragma option -zCPLAYER_B_TEXT -zPmain_01

// Address order inside PLAYER_B_TEXT, which is what TLINK reproduces from the
// order of these #includes: the playchar .BB/.CDG lifecycle was the last thing
// th04_main.asm contributed to this segment, so lifting it puts it at the FRONT
// of this object, ahead of the two functions that were already here
// (kb/codegen 0099 + 0112 + 0114).
#include "th04/formats/bb_playchar.cpp"
#include "th04/main/player/bomb.cpp"
