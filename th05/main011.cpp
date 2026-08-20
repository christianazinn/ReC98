/* ReC98
 * -----
 * 2nd part of code segment #1 of TH05's MAIN.EXE
 */

// The segment has to be named explicitly: the basename default would open a
// MAIN011_TEXT of its own (kb/codegen/0105), the same reason th05/main031.cpp
// gives. main_TEXT has no other C++ contribution, so TLINK -- which lays a
// segment's contributions out in link order, with the root dump first -- puts
// this object at the tail of the root's contribution by construction, which
// is exactly where the `include` this parcel deleted used to end
// (kb/codegen 0112 + 0114). The Tupfile.lua line is therefore
// append-anywhere.
//
// main_TEXT is in group main_01, and th05/main010.cpp — the binary's other
// root-level wrapper named after a part of code segment #1 — declares that
// group, so this one does too.
// The group pragma lives here rather than in the included file: it only takes
// effect before any code is generated (kb/codegen/0112 trap 0;
// kb/codegen/0138).
#pragma option -zCmain_TEXT -zPmain_01

#include "th04/main/enemy/render.cpp"
