// The code segment name is left to Turbo C++'s basename default, which makes
// this object contribute to th04_main.asm's main_035_TEXT (kb/codegen/0105).
// That segment has no other contribution, so TLINK -- which lays a segment's
// contributions out in link order, with the root dump first -- puts this one
// at its tail by construction, which is where the function below already was.
// Its Tupfile.lua line is therefore append-anywhere, unlike th04/main_01.cpp's.
// (kb/codegen/0112 + 0114.)
//
// There is deliberately NO `-zP` group pragma, and this is where a wrapper for
// a group main_03 segment differs from th04/boss_bg.cpp and its two siblings.
// Every near function pointer the code below stores -- midbossx_render(),
// mugetsu_gengetsu_bg_render(), mugetsu_fg_render(), the backdrop colorfill,
// and nullfunc_near() twice -- lives in group main_01, while this segment is
// in main_03. Turbo C++ frames an `&extern near function` fixup on the
// object's OWN declared group, so `#pragma option -zPmain_03` makes all six
// overflow at link time ("Fixup overflow at MAIN_035_TEXT:00BC, target =
// NULLFUNC_NEAR"). With no group pragma at all the group comes from
// th04_main.asm's own GRPDEF, and each fixup frames on its target's group --
// which is what the original does. th04/boss.cpp is the precedent: BOSS_TEXT
// is also group main_03, it also assigns main_01 renderers, and it also
// carries no `-zP`.

// Address order inside main_035_TEXT, which is what TLINK reproduces from the
// order of these #includes: the .BB boss-entrance lifecycle was the last thing
// th04_main.asm contributed to this segment, so it goes at the FRONT of this
// object, ahead of the seven stage setups.
//
// th04/formats/bb_boss.cpp is not self-contained -- it takes its declarations
// from whichever file includes it, exactly as it does from th05/main014.cpp --
// so the two headers it needs are pulled in here. th04/main/stage/setup.cpp
// includes th04/formats/bb.h a second time, which is why that header gained an
// include guard in the same parcel.
#include "libs/master.lib/master.hpp"
#include "th04/formats/bb.h"
#include "th04/main/null.hpp"
#include "th04/main/boss/boss.hpp"

#include "th04/main/boss/reset.cpp"
#include "th04/formats/bb_boss.cpp"
#include "th04/main/stage/setup.cpp"
