// Its OWN object, not an `#include` into th04/execl.cpp, which is the next
// contribution to this segment and already hosts continue_prompt(),
// score_last_commit() and GameExecl(). Two reasons, and the first is decisive:
// th04/main/continue.cpp is #included into that translation unit and owns its
// only copy of th04/gaiji/gaiji.h, th04/hardware/input.h and
// th04/main/quit.hpp -- none of which carries an include guard, and each of
// which defines an enum or a namespace-scope constant -- so a body appended
// AHEAD of it there could not reach [gb_G] or OVERLAY_FADE_CELS at all.
// Second, kb/codegen/0119: this body is 0x139 bytes, an odd length, and a
// separate object cannot re-roll a host object's -a2 parity.
//
// The segment is named here rather than left to the basename default
// (kb/codegen/0105), which would open a GAMEOVER_TEXT of its own. No `-zP`:
// the sibling object in this segment, th04/execl.cpp, carries no group pragma
// and the map still places it in MAIN_01, because th04_main.asm's own
// `main_01 group` line already lists EXECL_TEXT. BOOTSTRAP records six
// `Fixup overflow` errors caused by adding a `-zP` that was not derived.
//
// TLINK concatenates a segment's contributions in link order with
// th04_main.asm first, and this object is listed ahead of th04/execl.cpp, so
// gameover() lands back at the tail of the root contribution where it started
// and continue_prompt() does not move. Same shape as th05/gameover.cpp.
#pragma option -zCEXECL_TEXT

#include "th04/main/gameover.cpp"
