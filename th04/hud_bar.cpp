// hud_bar_put() sat at the tail of th04_main.asm's HUD_PUT_TEXT root
// contribution, so a C++ object linked immediately before th04/hud_put.cpp
// lands at exactly the address it already had -- no carve (kb/codegen/0080),
// no new segment name, and every byte after it unmoved
// (kb/codegen 0099 + 0114).
//
// The segment name has to be spelled out because it does not match this
// file's basename (kb/codegen/0105), and `-G` is what turns `ENTER 16h, 0`
// into the original's `push bp` / `mov bp, sp` / `sub sp, 16h`
// (kb/codegen/0011).
#pragma option -zCHUD_PUT_TEXT -zPmain_01 -G

#include "th04/main/hud/bar_put.cpp"
