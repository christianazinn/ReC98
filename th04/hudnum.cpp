/* ReC98
 * -----
 * The head of `IT_UPDT_TEXT` in TH04's MAIN.EXE: both bold-gaiji number
 * renderers, and the stage bonus tally's two multiplier functions.
 */

// ITS OWN OBJECT rather than four #includes at the front of th04/itminit.cpp,
// and the reason is the header hoist, not the addresses. The two bodies that
// object already carries -- stage_clear_bonus() and stage_allclear_bonus() --
// are byte-exact today against the header set th04/main/stage/bonus.cpp
// declares, and the pair here needs th04/main/boss/boss.hpp on top of it.
// Parsing one more header ahead of a function that this parcel does not touch
// has been measured to move it (state/notes/th05-main-tail-lifts.md, the
// enemies_update lift: th01/math/subpixel.hpp alone, guarded and already in
// the object, shortened boss_explode_small() by two bytes). A new object plus
// one Tupfile.lua line cannot perturb another translation unit at all.
//
// It is listed immediately BEFORE th04/itminit.cpp, which is the address order
// the seven bodies have in this segment. The dump's IT_UPDT_TEXT block is
// empty as of this parcel, so TLINK hands this object the segment start and
// every byte keeps its address (kb/codegen 0098 + 0112 + 0114) -- no carve, no
// new segment name, no group-list edit.
//
// The segment name is spelled out because the wrapper's basename would
// otherwise supply it (kb/codegen/0105), and the group with it, because
// stage_clear_bonus_multipliers_apply()'s rank dispatch frames its `jmp cs:`
// jump table on the GROUP (kb/codegen/0104). NO `-a2`: that table sits at an
// ODD offset in this object and the original carries no pad byte in front of
// it, so aligning it would make this object one byte too long
// (kb/codegen/0119, checked rather than assumed).
#pragma option -zCIT_UPDT_TEXT -zPmain_03

// Every header both included bodies need. They carry none of their own, for
// the reason th04/main/hud/lives.cpp states in full: several in this closure
// have no include guard, so a second expansion from the second body would be a
// hard error (kb/codegen/0129).
// th04/gaiji/gaiji.h is deliberately NOT listed: it has no include guard, and
// th04/main/boss/boss.hpp already reaches it through th04/main/hud/overlay.hpp.
// Spelling it out as well is a `Multiple declaration` error on all 26 of its
// enumerators (kb/codegen/0129), measured.
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th04/main/boss/boss.hpp"
#include "th04/main/hud/hud.hpp"
#include "th04/main/rank.hpp"
#include "th04/main/score.hpp"
#include "th04/main/stage/bonus.hpp"
#include "th04/resident.hpp"

#include "th04/main/hud/number_p.cpp"
#include "th04/main/stage/bonus_m.cpp"
