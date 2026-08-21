// Extra Stage Boss Yuuka's yuuka6_phase_next(), as its OWN object, linked
// immediately ahead of th04/main_034.cpp.
//
// WHY A SECOND OBJECT RATHER THAN A SECOND #include IN THAT ONE, and it is
// `[measured]`, not a preference: this body is 0x4F bytes, ODD, and
// th04/main/boss/b3_upd.cpp -- the second half of th04/main_034.cpp's object
// -- turns `#pragma option -a2` on for elly_1BDB4()'s sparse `switch`, whose
// value/jump table pair takes a one-byte pad in front of it. `-a2` aligns a
// table against the offset its own OBJECT starts at, so folding an odd-length
// body in front of that object flips the table's object-local parity and the
// pad silently disappears. Measured here, exactly as kb/codegen/0119 predicts:
// the body came out IDENTICAL over its whole 0x4F bytes, and main_034_TEXT
// still ended up one byte short, with the missing byte 0x9B4 further into the
// object at load 0x1BE32:
//
//   original  ... b0 01 c9 c3 | 00 | 01 00 08 00 10 00 20 00 ...
//   built     ... b0 01 c9 c3 |    | 01 00 08 00 10 00 20 00 ...
//
// th04/main_36r.cpp is the same fix for the same reason one segment over, and
// records the arithmetic in more detail.
//
// The segment name therefore cannot come from Turbo C++'s basename default
// (kb/codegen/0105) and is named explicitly; `-zPmain_03` because the near
// call to boss_explode_small() leaves this segment and resolves against the
// group (kb/codegen/0104).
#pragma option -zCmain_034_TEXT -zPmain_03

#include "th04/main/boss/b6_next.cpp"
