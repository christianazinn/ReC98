// Its OWN object rather than an `#include` into th05/midbossx.cpp, which is
// the next contribution to this segment and the host carve_free_tails.py names
// for this row. That route would work, but it costs more than the Tupfile.lua
// line it saves: th04/main/midboss/mx.cpp carries its own
// `#pragma option -zPmain_01`, and Turbo C++ REJECTS -zP once the translation
// unit has emitted any code (kb/codegen/0112, trap 0 — th05/laser_rh.cpp
// carries the same note). Prepending a body there means hoisting that pragma
// out of a file TH04 shares, to fix a problem this object does not have.
//
// MIDBOSSX_TEXT is not one of ZUN's object names either. It is a harness carve
// of the head of the original main_0_TEXT (kb/codegen/0080; the comment sits
// above the segment in th05_main.asm), so "one object per segment" is not a
// fidelity argument here the way it would be for a segment ZUN's own linker
// named.
//
// The segment therefore has to be named explicitly rather than left to the
// basename default (kb/codegen/0105), which would open a STAGES_TEXT of its
// own. TLINK lays a segment's contributions out in link order with
// th05_main.asm first, and this object is listed ahead of th05/midbossx.cpp,
// so the lifted body lands back at the address it had.
#pragma option -zCMIDBOSSX_TEXT -zPmain_01

#include "th05/main/stage/stages.cpp"
