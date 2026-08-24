// The code segment name is left to Turbo C++'s basename default, which makes
// this object append to th04_main.asm's mai_TEXT contribution - the original
// object's own segment. ZUN's object began with the tile renderer and ended
// with midboss2_render(). carve_free_tails.py offers PLAYFLD_TEXT /
// th04/playfld.cpp as
// a neighbour host instead, and that route works (MATCH-TH04-MAIN-SCROLL-
// UPDATE took it for this very segment), but it would put a midboss renderer
// at the front of the playfield object, in a segment that is not ZUN's. A new
// object needs no neighbour: TLINK concatenates a segment's contributions in
// link order with th04_main.asm first. (kb/codegen/0105 + 0112 + 0114.)
//
// The file basename is ZUN's, not ours — mai_TEXT is what the original object
// was called, and Turbo C++ derives MAI_TEXT from `mai.cpp`. Segment names are
// case-insensitive to TASM and TLINK, so the dump's lower-case spelling and
// this one are the same segment.
//
// The group pragma lives here rather than in the included file: it only takes
// effect before any code is generated. (kb/codegen/0112, trap 0)
#pragma option -zPmain_01

void near egc_start_copy_noframe(void);
#pragma codeseg TILE_REDRAW_TEXT main_01
#include "th04/main/tile/redraw.cpp"
#pragma codeseg MAI_TEXT main_01

#include "th04/main/tile/render_a.cpp"
#include "th04/main/midboss/m2.cpp"
