// kb/codegen/0104: -zC only names the segment; -zP is what puts it into the
// GROUP, and the group is the frame TASM uses for `cs:`-relative references.
// This TU went without -zP for as long as nothing in it emitted one — the first
// `switch` jump table in stage_clear_bonus_multipliers_apply() is what made the
// difference observable, as a dispatch displacement and four table entries all
// low by (group base - segment base) = 0xDF0.
#pragma option -zCmain_032_TEXT -zPmain_03

// Address order is source order within one object, and everything this file
// lifts originally sat immediately before this object's contribution to
// main_032_TEXT — so it has to come first. It cannot go into
// th04/main/gather.cpp: Tupfile.lua compiles that source for TH04 as well, and
// the two games' bonus tallies share nothing.
// ... and boss2_explode_big_circle() sat immediately before even that: it was
// the LAST thing th05_main.asm contributed to main_032_TEXT, an `include` of a
// module rather than a proc, so replacing the module with a C++ file put it at
// the front of this object (kb/codegen/0112 + 0114). It is deliberately not
// #included from th05/main/stage/bonus.cpp below, which would work but would
// bury a segment-order fact inside an unrelated file.
// ... and boss_explode_big_circle(), the function it wraps, sat immediately
// before *that*, as the new last item of the same root contribution once the
// wrapper had left it. Same route, one module further up, so this line has to
// stay above the one below it.
// ... and boss2_explode_small() above even that, by the same argument applied a
// third time. Draining a segment's `include` tail promotes whatever was above
// it, so each of these lines went in at the FRONT of the list, and the list is
// therefore in reverse lift order and in forward address order.
#include "th05/main/boss/2_explode_small.cpp"

#include "th05/main/boss/explode_big.cpp"

#include "th05/main/boss/2_explode_big.cpp"

#include "th05/main/stage/bonus.cpp"

#include "th04/main/gather.cpp"
