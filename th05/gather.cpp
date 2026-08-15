// kb/codegen/0104: -zC only names the segment; -zP is what puts it into the
// GROUP, and the group is the frame TASM uses for `cs:`-relative references.
// This TU went without -zP for as long as nothing in it emitted one — the
// first `switch` jump table in bonus_multipliers_put() is what made the
// difference observable, as a dispatch displacement and four table entries all
// low by (group base - segment base) = 0xDF0.
#pragma option -zCmain_032_TEXT -zPmain_03

// Address order is source order within one object, and everything this file
// lifts originally sat immediately before this object's contribution to
// main_032_TEXT — so it has to come first. It cannot go into
// th04/main/gather.cpp: Tupfile.lua compiles that source for TH04 as well, and
// the two games' bonus tallies share nothing.
#include "th05/main/stage/bonus.cpp"

#include "th04/main/gather.cpp"
