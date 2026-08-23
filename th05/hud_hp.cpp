// hud_hp_put() was the FIRST item in th05_main.asm's MIDBOSSX_TEXT block --
// the one deleted include that used to sit under th04's hud directory, with
// hud_put() and the rest of the root's 0x1E9 behind it. A C++ contribution can
// only ever be a SUFFIX of a root block, because the dump is that segment's
// first object, so it needs the kb/codegen/0080 carve that MIDBOSSX_A_TEXT is:
// the head keeps the bytes, the tail keeps the ORIGINAL name and all four of
// the C++ objects that already append to it, per 0080's "prefer the half with
// no C++ contribution".
//
// An object of its own rather than a `#pragma codeseg` block inside an
// existing one. th04/main/hud/hp_put.cpp owns the #includes for TH04's
// th04/hud_bar.cpp translation unit, and two of the three reach UNGUARDED
// headers, so it cannot be pulled into a TU that already has them -- and
// hoisting them into one that does not is the case where a guarded, already-
// present header still moves later codegen. The wrapper route costs one
// Tupfile.lua line and cannot do either.
//
// The segment name has to be spelled out because it does not match this
// file's basename (kb/codegen/0105), `-zPmain_01` because the body reaches
// TH05's near hud_bar_put() in MAIN_01_TEXT through a group-relative near
// call, and `-G` is what turns `ENTER 10h, 0` into the original's `push bp` /
// `mov bp, sp` / `sub sp, 10h` (kb/codegen/0011).
#pragma option -zCMIDBOSSX_A_TEXT -zPmain_01 -G

#include "th04/main/hud/hp_put.cpp"
