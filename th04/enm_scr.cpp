// The enemy .STD script VM, as its OWN object, linked at the head of
// B4M_UPDATE_TEXT's C++ half and immediately ahead of th04/expl_sm.cpp.
//
// WHY ANOTHER OBJECT RATHER THAN ONE MORE `#include` IN THAT ONE: the include
// sets are incompatible, not the addresses. th04/main/boss/explode_small.cpp
// includes the UNGUARDED `th02/snd/snd.h` directly and this file needs
// snd_se_play() too, which it reaches through the guarded `th04/snd/snd.h`;
// whichever of the two comes first in an object, the other expands that header
// a second time and Turbo C++ answers with `kb/codegen/0129`'s
// `Multiple declaration` storm. The alternative is to edit a file TH05 also
// compiles, for TH04's benefit only. One `.cpp` and one Tupfile.lua line is
// cheaper, and it perturbs no other translation unit at all.
//
// The body is `0x570` plus `0x120` of generated jump table, so `0x690` and
// EVEN either way; nothing behind it in this segment moves by an odd amount
// and kb/codegen/0119 does not arise.
//
// The segment name cannot come from Turbo C++'s basename default
// (kb/codegen/0105) and is named explicitly; `-zPmain_03` because the VM's
// dense `switch` frames its `jmp cs:` table on the GROUP (kb/codegen/0104).
#pragma option -zCB4M_UPDATE_TEXT -zPmain_03

#include "th04/main/enemy/script.cpp"
