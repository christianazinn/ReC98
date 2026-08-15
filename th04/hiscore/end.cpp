#include "th04/formats/scoredat/recreate.cpp"
#include "th04/hiscore/score_ld.cpp"

extern playchar_t playchar;

#include "th04/hiscore/score_sv.cpp"

// regist_score_enter_from_resident() lands at the very end of this object's
// SCORE_TEXT contribution, which is exactly where the root dump's own
// contribution used to begin — TH04 `0A05:2362`, TH05 `0A54:11F0`. A
// kb/codegen/0098 head lift into the object that already sat immediately
// before it, in both games: no carve, no new segment, no Tupfile.lua line.
#include "th04/hiscore/regist_enter.cpp"
