// pause() was the last thing th04_main.asm contributed to DEMO_TEXT and this
// object is the next contribution behind it, so listing its file FIRST grows
// this object backwards into the hole and puts the body back at its original
// address (kb/codegen 0099 + 0114).
//
// Listed here rather than at the head of th04/main/demo.cpp, which would have
// done the same job in one edit for both games: an insertion there renumbers
// every line-anchored citation into that file, and seven of them are live
// across state/ and upstream-leads/. Two one-line wrapper edits cost nothing
// and move no line anyone cites.
#include "th04/main/pause.cpp"
#include "th04/main/demo.cpp"
