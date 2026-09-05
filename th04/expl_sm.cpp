// The last two things th04_main.asm contributed to B4M_UPDATE_TEXT, as their
// OWN object, linked immediately ahead of th04/boss_4m.cpp.
//
// WHY A SECOND OBJECT RATHER THAN TWO MORE `#include`s IN THAT ONE, and both
// halves are `[measured]`:
//
// * The two bodies are `0xF` and `0x80` bytes, `0x8F` together, which is ODD.
//   th04/main/boss/b4m.cpp -- the tail of th04/boss_4m.cpp's object -- turns
//   `#pragma option -a2` on over two of its functions, and `-a2` aligns a
//   generated jump table against the offset its own OBJECT starts at. Folding
//   an odd-length body in front of that object flips those two tables'
//   object-local parity and silently moves the padding byte in front of them,
//   hundreds of bytes from anything this parcel wrote (kb/codegen/0119).
//   With the bodies here instead, root and this object only ever exchange
//   bytes: th04/boss_4m.cpp's base is pinned for this lift and every future
//   one out of the same root contribution.
// * th04/main/boss/explode_small.cpp includes the UNGUARDED `th02/snd/snd.h`
//   directly, while th04/main/boss/b4m.cpp includes the guarded
//   `th04/snd/snd.h`, which reaches the same file through `th03/snd/snd.h`.
//   In one translation unit those two are a `Multiple declaration` storm, and
//   the only fixes inside a shared object are to edit a file TH05 also
//   compiles or to hoist a header past b4m.cpp's own list. Separate objects
//   have separate include sets and neither is needed.
//
// The segment name therefore cannot come from Turbo C++'s basename default
// (kb/codegen/0105) and is named explicitly; `-zPmain_03` because
// boss_explode_small()'s dense `switch` frames its `jmp cs:` table on the
// GROUP (kb/codegen/0104).
#pragma option -zCB4M_UPDATE_TEXT -zPmain_03

// Address order is source order within one object, and this is the order the
// two had in the dump (kb/codegen/0112 + 0114). Both files are the ones TH05
// already compiles, unchanged: the bodies are identical in the two games, and
// until this parcel TH04 was the only reason the two ASM modules still
// existed.
#include "th04/main/boss/explosions_reset.cpp"
#include "th04/main/boss/explode_small.cpp"
