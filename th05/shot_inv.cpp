#pragma option -zCSHOT_INV_TEXT -zPmain_01

// player_render() was the tail `include` of th05_main.asm's contribution to
// this segment, and this object is the segment's only other contribution, so
// the lift has to be the FIRST code this translation unit emits for every
// byte to keep its address (kb/codegen 0112 + 0114). Same shape as the TH05
// arm of th04/main/end.cpp.
#include "th04/main/player/render.cpp"

#include "th04/main/player/shots_inv.cpp"
