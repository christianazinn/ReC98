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

// Direct Stage 6 Practice is constructed from REPLAY_CK_TEXT, outside the
// main_01 code group that owns Yuuka's NEAR background renderer. This FAR
// bridge lives in DEMO_TEXT's existing patch reserve, where the callback is a
// valid same-group NEAR call and no stock segment has to grow.
void pascal near yuuka6_bg_render(void);
void pascal far replay_practice_yuuka6_bg_render(void)
{
	yuuka6_bg_render();
}

// Fill the exact stock DEMO_TEXT extent after moving demo policy to the tail.
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
