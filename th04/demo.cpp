// stage_init(), stage_transition(), and then pause(), were the last three
// things th04_main.asm contributed to DEMO_TEXT and this object is the next
// contribution behind them, so listing their files FIRST, in that order,
// grows this object backwards into the hole and puts all three bodies back at
// their original addresses (kb/codegen 0099 + 0114). pause() got here first;
// stage_transition() is the proc that was left exposed at the dump's tail
// once it did, and stage_init() is the one that stage_transition() exposed in
// turn. The dump's contribution to this segment is now sub_AD03, sub_AED0 and
// the latter's jump table alone.
//
// Listed here rather than at the head of th04/main/demo.cpp, which would have
// done the same job in one edit for both games: an insertion there renumbers
// every line-anchored citation into that file, and seven of them are live
// across state/ and upstream-leads/. Two one-line wrapper edits cost nothing
// and move no line anyone cites.
#include "th04/main/stage/init.cpp"
#include "th04/main/stage/transition.cpp"
#include "th04/main/pause.cpp"
#include "th04/main/demo.cpp"
