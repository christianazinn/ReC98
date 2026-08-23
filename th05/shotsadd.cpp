// shots_add() was the FIRST of the eleven `include`s that made up
// th05_main.asm's SCORE_TEXT root contribution, so a C++ object could not
// reach it: the dump is the segment's first object and a C++ contribution can
// only ever be a SUFFIX of a root block. SCORE_A_TEXT is the kb/codegen/0080
// head carve that fixes that. The tail keeps the ORIGINAL name, because it is
// the half with the C++ contribution (th05/score_rm.cpp), which 0080 prefers.
//
// SCORE_TEXT is `word`-aligned, and this is the first carve in this dump where
// that matters: the body is 0x37, an ODD length, so the head has to carry the
// module's own `even` pad or the reopened tail would start one byte early and
// TLINK would then align it back with a byte of its own choosing. The pad is
// emitted by the body file, at the position it had.
//
// An object of its own rather than a `#pragma codeseg` block inside an
// existing one, and both candidates were rejected for measurable reasons
// rather than by preference: th05/score_rm.cpp contributes to the TAIL of the
// segment this carve splits, and th05/p_common.cpp holds three already-matched
// bodies that need frames, while this one needs `-k-` for the whole object.
//
// The segment name is spelled out because it does not match this file's
// basename (kb/codegen/0105); `-zPmain_01` because SCORE_A_TEXT is a new
// member of that group; and `-k-` because the original establishes no stack
// frame at all.
#pragma option -zCSCORE_A_TEXT -zPmain_01 -k-

#include "th05/main/player/shots_add.cpp"
