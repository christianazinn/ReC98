// hud_bar_put() sat at the tail of th04_main.asm's HUD_PUT_TEXT root
// contribution, so a C++ object linked immediately before th04/hud_put.cpp
// lands at exactly the address it already had -- no carve (kb/codegen/0080),
// no new segment name, and every byte after it unmoved
// (kb/codegen 0099 + 0114).
//
// hud_hp_put() was the include ahead of it inside that same root
// contribution, and now grows this object backwards into the hole it left,
// taking th04_main.asm's contribution to this segment to zero. The two files
// are listed in the order their bodies had in the original, and that order is
// what puts each at its own address.
//
// The segment name has to be spelled out because it does not match this
// file's basename (kb/codegen/0105), and `-G` is what turns `ENTER 16h, 0`
// into the original's `push bp` / `mov bp, sp` / `sub sp, 16h`
// (kb/codegen/0011). Both bodies need it.
#pragma option -zCHUD_PUT_TEXT -zPmain_01 -G

#include "th04/main/hud/hp_put.cpp"
#include "th04/main/hud/bar_put.cpp"
