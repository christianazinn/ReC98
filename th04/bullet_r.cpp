// Its OWN object rather than an `#include` at the front of th04/boss_fg.cpp,
// which is the next contribution to this segment and which carve_free_tails.py
// therefore names as the kb/codegen/0114 host. Two reasons, both measured, and
// the first is decisive:
//
// ReC98's game headers are unguarded, and three of the ones bullets_render()
// needs are already in th04/boss_fg.cpp's include closure — th02/v_colors.hpp
// (an enum) and th04/hardware/grcg.hpp (an inline function) through
// th04/main/boss/fg.cpp, and th04/formats/super.h through
// th04/main/item/render.cpp. bullets_render() has to come FIRST in that object
// to keep its original address, so no reordering of the three #includes turns
// the second inclusion into a first one.
//
// Second, kb/codegen/0119: this body is 0x10B = 267 bytes, an odd length, and
// prepending it would re-roll every object-relative offset in th04/boss_fg.cpp
// by an odd amount. th04/main/item/render.cpp measured that host to emit no
// `-a2`-aligned data, so the risk is currently zero — but that measurement
// covers the closure the object has today, not the seven headers this file
// would add to it. A separate object is 0119's own prescribed fix and cannot
// have the effect at all: every object aligns its data against its own
// contribution's offset 0.
//
// The segment is named here rather than left to Turbo C++'s basename default
// (kb/codegen/0105), which would open a BULLET_R_TEXT of its own. `-zP` matches
// the sibling object in this segment, th04/boss_fg.cpp, which carries
// `-zPmain_01`; th04_main.asm's own `main_01 group` line lists BOSS_FG_TEXT as
// well, so this is belt-and-braces rather than load-bearing. Both live here
// rather than in the included file because -zC/-zP only take effect before any
// code is generated (kb/codegen/0112, trap 0) — and because that file is shared
// with TH05, whose own wrapper needs different ones.
//
// TLINK concatenates a segment's contributions in link order with th04_main.asm
// first, and this object is listed ahead of th04/boss_fg.cpp, so bullets_render()
// lands back at the head of BOSS_FG_TEXT where it started. th04_main.asm's
// contribution to that segment is now zero-length; it was exactly this one proc.
#pragma option -zCBOSS_FG_TEXT -zPmain_01

#include "th04/main/bullet/render.cpp"
