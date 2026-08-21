/* ReC98
 * -----
 * 3rd part of code segment #3 of TH05's MAIN.EXE
 */

// The group is kb/codegen/0104, and it is needed as of the parcel that put
// item_collected() at the front of this object: that function's dense
// `switch` emits a `cs:`-relative jump table, and the assembler frames that
// on the segment unless a GROUP line names it -- which would leave the body
// byte-identical and every table entry low by main_03's base. It sits here
// rather than in an included body because Turbo C++ rejects `-zP` once a TU
// has emitted any code (kb/codegen/0138).
#pragma option -zCmain_033_TEXT -zPmain_03

#include "th04/main/item/update.cpp"
