// MB_UPD_TEXT's FIRST C++ object, ahead of th04/mb_upd.cpp -- the `1` is the
// link position, not a sequel, exactly as in th04/enm_pos1.cpp. The root dump
// contributes nothing to this segment any more, so TLINK gives the first
// object handed to it offset 0 (kb/codegen/0114) and the Tupfile.lua line for
// this file is POSITION-CRITICAL: it must stay immediately before
// th04/mb_upd.cpp's.
//
// WHY THIS IS NOT SIMPLY ANOTHER #include AT THE TOP OF th04/mb_upd.cpp, which
// is what it was until the whole segment was in C++.
// midboss3_update() carries `#pragma option -a2` for the one padding byte
// between its epilogue and its generated value/jump table pair, and that pad is
// a pure function of the table's offset parity inside the compiling object
// (kb/codegen/0160 + 0154). `[measured]` on this object, both parities read off
// the OBJ's PUBDEFs rather than off a `tcc -S` listing:
//
//   prefix ahead of midboss3_update()  natural table offset   OBJ
//   0x4D9 (this file's code folded in)  0x6E8, even           NO pad, 0x223
//   0x19A (the four patterns alone)     0x3A9, odd            pad, 0x224
//
// The second is the original. So this file's 0x33F bytes cannot live in that
// object, and the escape hatches do not apply either: `#pragma codestring`
// lands at the top of the object's code (kb/codegen/0070), which here is the
// segment's first byte, and there is no ASM contribution left above to borrow a
// `retn` from.
//
// `-zC` because the basename does not name the segment (kb/codegen/0105), and
// `-zPmain_03` because midboss1_update()'s near calls -- bullet_template_tune()
// and bullets_add_special() through their function pointers, and
// midboss_defeat_update() -- leave this segment and resolve against the group
// (kb/codegen/0104).
#pragma option -zCMB_UPD_TEXT -zPmain_03

#include "th04/main/midboss/m1_updt.cpp"
