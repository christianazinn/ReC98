/* ReC98
 * -----
 * TH02's per-stage reset of the map scrolling state. stage_init() calls this
 * once per stage, through a `nopcall` alias.
 */

// The original's prolog is a plain `push bp; mov bp, sp` with no locals, which
// is -G. -G- would emit `ENTER 0, 0` even with no frame to set up.
// (kb/codegen/0011)
#pragma option -zCSCROLL_TEXT -zPmain_01 -G

#include "platform.h"
#include "pc98.h"
#include "th02/main/scroll.hpp"

// Written by scroll_reset() and never read anywhere in MAIN.EXE.
// ZUN bloat: Three stores with no effect. [static] — the evidence is a
// whole-binary symbol search, not an emulator run.
//
// The numbering implies a contiguous family that the layout does not have:
// [scroll_unused] and [scroll_unused_2] are `_BSS` (the latter at 2034Eh),
// but [scroll_unused_3] is `_DATA` with an initializer (1E502h, `db 0`) —
// 7,756 bytes below [scroll_unused_2], in the other segment. The suffixes
// are IDA's discovery order, not an array or a struct.
extern int scroll_unused;
extern uint8_t scroll_unused_2;
extern uint8_t scroll_unused_3;

void far scroll_reset(void)
{
	scroll_speed = 1;
	scroll_cycle = 0;
	scroll_line = 0;
	scroll_unused = 0;
	scroll_step = 0;
	scroll_interval = 4;
	scroll_done = false;
	scroll_unused_2 = 0;
	scroll_unused_3 = 0;
	scroll_sad = 0;
	scroll_step_advanced = false;
	scroll_delta = 0;
}
