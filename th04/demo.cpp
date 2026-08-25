#pragma option -zPmain_01

// stage_setup(), stage_init(), stage_transition(), and pause() were the final
// roots th04_main.asm contributed to DEMO_TEXT. This object follows that root
// contribution, so listing them first in original order grows it backwards
// over each vacated body (kb/codegen 0099 + 0114).
//
// Listed here rather than at the head of th04/main/demo.cpp, which would have
// done the same job in one edit for both games: an insertion there renumbers
// every line-anchored citation into that file, and seven of them are live
// across state/ and upstream-leads/. Two one-line wrapper edits cost nothing
// and move no line anyone cites.
#include "th04/main/stage/setup_main.cpp"
#include "th04/main/stage/init.cpp"
#include "th04/main/stage/transition.cpp"
#include "th04/main/pause.cpp"
#include "th04/main/demo.cpp"

// Fill the exact stock DEMO_TEXT extent after moving demo policy to the tail.
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90"
