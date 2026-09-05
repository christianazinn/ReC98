// The code segment name is left to Turbo C++'s basename default, which makes
// this object contribute to th04_main.asm's main_01_TEXT — but unlike every
// other wrapper in this binary, **this one's Tupfile.lua line is
// position-critical**. main_01_TEXT already has a second contribution,
// th04\scoreupd.asm, and TLINK concatenates a segment's contributions in link
// order. Listed after that module, this object would land at the far end of
// the segment and move scoreupd.asm's 0x101 bytes; listed before it, the
// layout is [th04_main.asm][this][scoreupd.asm], which is the original's and
// leaves every byte where it is.
//
// So: keep this entry immediately before "th04/scoreupd.asm" in the link
// list. carve_free_tails.py prints `MUST be listed before th04\scoreupd.asm`
// for exactly this reason, and it is the only TH04 MAIN row for which it does.
// (kb/codegen/0105 + 0112 + 0114.)
//
// The group pragma lives here rather than in the included file: it only takes
// effect before any code is generated. (kb/codegen/0112, trap 0)
#pragma option -zPmain_01

// Address order inside ZUN's own object for this segment, which is what TLINK
// reproduces from the order of these #includes: the .BB text loader first,
// then mugetsu_fg_render(), then the barrier it calls.
// th04/main/boss/bx1_fg.cpp still owns every unguarded header the last two
// share — the loader's own two headers are both guarded, so it adds nothing
// to that set and takes nothing away from it.
//
// The loader was the last thing th04_main.asm contributed to this segment, so
// putting it at the FRONT of this object grows the object backwards into that
// hole and takes the dump's contribution to zero (kb/codegen 0099 + 0114).
#include "th04/formats/bb_txt_load.cpp"
#include "th04/main/boss/bx1_fg.cpp"
#include "th04/main/boss/shield.cpp"
