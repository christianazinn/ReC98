// mima_update()'s generated jump table needs an ODD number of bytes ahead of
// it inside this object (kb/codegen/0154), and a plain lift leaves it at an
// even 0x592. That one byte is mima_19353()'s own final `retn` - the proc
// directly above mima_193A4() in the dump - handed to this object as a one-byte
// codestring, which is how th02/main/boss/b4.cpp bought marisa_update()'s pad
// while marisa_1BC43() was still in the dump. (kb/codegen/0070, kb/codegen/0159)
//
// -zPmain_03 for the sixteen near calls into Mima's still-ASM patterns and for
// the `nop; push cs; call near ptr` far call into mima_end(); -G for
// mima_update()'s `push bp; mov bp, sp; sub sp, 2` prolog (kb/codegen/0011);
// -a2 for the pad itself. The segment name still comes from this wrapper's own
// basename. (kb/codegen/0105)
#pragma option -zPmain_03 -G -a2
#pragma codestring "\xC3"

#include "th02/main/boss/b5m.cpp"

// skill_calculate() is NOT -G: the option strength-reduces both of its `* 3`
// multiplies from `imul ax, ax, 3` into add chains and costs it 7 bytes.
// `[measured]` The pad above survives this pragma - whether it reaches the
// object is a function of this object's prefix parity and -a2 alone, and of
// nothing else. (kb/codegen/0159)
#pragma option -G-

#include "th02/main/boss/b5_.cpp"
