// ZUN's object for this code segment holds the Stage 3 midboss's update
// function and Alice's as well as the Stage 4 pair's shared code, so all three
// are compiled into this one translation unit, in their original address order
// (kb/codegen/0112). The segment pragma lives here rather than in any included
// file, because only the first one to be compiled may name the segment (0112
// trap 0) -- which is now th05/main/midboss/m3_updt.cpp, and which therefore
// also owns the unguarded headers this object shares.
#pragma option -zCB4_UPDATE_TEXT -zPmain_03

#include "th05/main/midboss/m3_updt.cpp"
#include "th05/main/boss/b3.cpp"
#include "th05/main/boss/b4_both.cpp"
