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

// items_miss_add() was the tail `include` of this segment's dump
// contribution, and this object is the segment's only other contribution, so
// it goes FIRST here and every byte keeps its address (kb/codegen/0114).
// Shared verbatim with th04/it_updt.cpp, which hosts the same body one
// binary over.
#include "th04/main/item/miss_add.cpp"
#include "th04/main/item/update.cpp"
