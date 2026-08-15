#include "th04/formats/scoredat/recreate.cpp"
#include "th04/hiscore/score_ld.cpp"

extern playchar_t playchar;

#include "th04/hiscore/score_sv.cpp"

// TH04's regist_score_enter_from_resident() lands at the very end of this
// object's SCORE_TEXT contribution, which is exactly where the root dump's own
// contribution used to begin (`0A05:2362`) — a kb/codegen/0098 head lift into
// the object that already sat immediately before it, so no carve, no new
// segment and no Tupfile.lua line. TH05's copy is still ASM; see the file's
// own docblock for why the guard is a lifting-order artifact.
#if (GAME == 4)
	#include "th04/hiscore/regist_enter.cpp"
#endif
