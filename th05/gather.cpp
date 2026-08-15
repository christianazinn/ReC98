#pragma option -zCmain_032_TEXT

// Address order is source order within one object, and stage_allclear_bonus()
// originally sat immediately before this object's contribution to
// main_032_TEXT — so it has to come first. It cannot go into
// th04/main/gather.cpp: Tupfile.lua compiles that source for TH04 as well, and
// the two games' bonus tallies share nothing.
#include "th05/main/stage/bonus.cpp"

#include "th04/main/gather.cpp"
