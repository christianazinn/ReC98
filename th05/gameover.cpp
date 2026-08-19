// Its OWN object, not an `#include` into th05/laser.cpp, which is the next
// contribution to this segment and already hosts continue_prompt().
// carve_free_tails.py flags that seam `PARITY RISK (kb/codegen/0119)`: the
// body below is 0x20D bytes, an ODD length, and folding it in front of a host
// that emits `-a2`-aligned data re-rolls that object's own offsets and can
// delete the pad byte under a `switch` table in a function nobody touched. A
// separate object is 0119's own prescribed fix and cannot have that effect:
// every object aligns its data against its own contribution's offset 0.
//
// The segment is named here rather than left to the basename default
// (kb/codegen/0105), because `gameover.cpp` would otherwise open a
// GAMEOVER_TEXT of its own. Naming it also keeps the group pragma in the one
// place -zC/-zP take effect at all, before any code is generated
// (kb/codegen/0112, trap 0).
//
// TLINK concatenates a segment's contributions in link order with
// th05_main.asm first, and this object is listed ahead of th05/laser_rh.cpp
// and th05/laser.cpp, so the three lifted procs land back at the head of
// main__TEXT where they started. th05_main.asm's contribution to that segment
// is now zero-length; it was exactly these three procs.
#pragma option -zCmain__TEXT -zPmain_01

#include "th05/main/gameover.cpp"
