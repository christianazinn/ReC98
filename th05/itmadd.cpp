/* ReC98
 * -----
 * items_add() out of the 3rd part of code segment #3 of TH05's MAIN.EXE
 */

// ITS OWN OBJECT, and kb/codegen/0119 is why -- measured on this parcel, in
// both directions. items_add() is 0xA5 = 165 bytes, ODD, and the object that
// already follows the dump here, th05/main033.cpp, emits an `-a2`-aligned
// jump table: the seven-entry one item_collected() compiles to. Folding this
// body into the FRONT of that object through kb/codegen/0112's wrapper
// #include moves every byte of it to an odd object-local offset, the table
// then needs no pad, and MAIN_033_TEXT comes out 0x597 where the original has
// 0x598 -- with the lifted range itself byte-perfect and the missing byte over
// a thousand bytes further in, under a function this parcel never touched.
// That was cycle 1 here, `14 ok, 1 fail`, and it is the entry's own case.
//
// A separate object restores ZUN's layout: this object's contribution begins
// where the dump's ends, th05/main033.cpp's begins where this one's ends, and
// the offsets INSIDE that object are exactly what they were, so its pad
// survives. Cost is one Tupfile.lua line, which must come BEFORE
// th05/main033.cpp because TLINK lays a segment's contributions out in link
// order.
//
// TH04's half of the same parcel needed none of this: its body is 0x96 = 150,
// even, so kb/codegen/0112's cheaper wrapper route holds there and the lift is
// an #include at the front of th04/it_updt.cpp. Two games, one body, two
// routes -- the route is a property of the length and the host, not of the
// function.
//
// The segment name is spelled out because the wrapper's basename would
// otherwise supply it (kb/codegen/0105), and the group with it, because
// item_splashes_add() is a cross-segment near call and Turbo C++ rejects `-zP`
// once a TU has emitted any code (kb/codegen 0104 + 0138).
#pragma option -zCmain_033_TEXT -zPmain_03

#include "th04/main/item/add.cpp"
