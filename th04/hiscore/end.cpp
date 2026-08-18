// Has to sit here rather than in recreate.cpp, which sets the -zC for this
// whole chain: th04/hiscore/view.cpp includes that file too, from OP.EXE's
// -zPop_01 translation unit, and a -zP inside it would override op_01 there.
// This file is only ever the very first thing in th0N/hi_end.cpp, so the
// pragma still precedes every byte of emitted code (kb/codegen/0138).
// Needed since regist_view.cpp: score_put() and stage_put() dispatch through
// `jmp cs:` switch tables, and without the group the compiler frames those
// displacements on SCORE_TEXT instead of group_01.
#pragma option -zPgroup_01

#include "th04/formats/scoredat/recreate.cpp"
#include "th04/hiscore/score_ld.cpp"

extern playchar_t playchar;

#include "th04/hiscore/score_sv.cpp"

// regist_score_enter_from_resident() lands at the very end of this object's
// SCORE_TEXT contribution, which is exactly where the root dump's own
// contribution used to begin — TH04 `0A05:2362`, TH05 `0A54:11F0`. A
// kb/codegen/0098 head lift into the object that already sat immediately
// before it, in both games: no carve, no new segment, no Tupfile.lua line.
#include "th04/hiscore/regist_enter.cpp"

// …and so do score_put() and stage_put(), which were the next two procs of
// that same root dump block in both games. Same kb/codegen/0098 head lift,
// one step further down.
#include "th04/hiscore/regist_view.cpp"

// …and finally regist_menu() itself, which was the last proc of that block.
// TH05 needs its copy one object later, after th05/regist.cpp's glyph ball
// code that it calls, so that game includes the same file from there instead.
#if (GAME != 5)
#include "th04/hiscore/regist_menu.cpp"

// …and, after it, the `near` EGC rectangle copy that regist_menu() unblits
// through. It was the last proc of the root dump's block, so this include
// empties th04_maine.asm's SCORE_TEXT contribution entirely. TH05 has no
// counterpart — it unblits through bgimage_put_rect_16().
#include "th04/hiscore/regist_unblit.cpp"
#endif
