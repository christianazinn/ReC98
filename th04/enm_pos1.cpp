// ENM_POS_TEXT's FIRST C++ object, ahead of th04/enm_pos.cpp — the `1` is the
// link position, not a sequel. TLINK lays a segment's contributions out in
// link order with the root dump first (kb/codegen/0114), so the Tupfile.lua
// line for this file is POSITION-CRITICAL: it must stay immediately before
// th04/enm_pos.cpp's.
//
// WHY THIS IS NOT SIMPLY ANOTHER #include AT THE TOP OF th04/enm_pos.cpp.
// midboss4_update() carries `#pragma option -a2` for the one padding byte
// between its epilogue and its generated value/jump table pair, and `-a2` pads
// exactly when the natural table offset is EVEN, measured from the compiling
// object's own base (kb/codegen/0154 + 0096). th04/enm_pos.cpp's note records
// the measurement: at a zero prefix — which is what midboss4_update() being
// the first thing that object emits gives it — the object is 0x29B bytes to
// enemy_pos_update() and carries the pad.
//
// midboss2_update() plus its own value/jump table pair is 0x27B bytes, which
// is ODD. Prepending it inside that object would flip the parity, `-a2` would
// stop padding, and every byte from midboss4_update()'s table onwards would
// move by one — kb/codegen/0119, under a function this parcel never touched.
// A second object keeps th04/enm_pos.cpp's prefix at zero, which is the only
// thing its measurement depends on.
//
// `-zC` because the basename does not name the segment (kb/codegen/0105), and
// `-zPmain_03` because midboss2_update()'s sparse `switch` emits a `cs:`
// value/jump table pair that the assembler otherwise frames on the segment
// rather than the group (kb/codegen/0104).
#pragma option -zCENM_POS_TEXT -zPmain_03

#include "th04/main/midboss/m2_updt.cpp"
