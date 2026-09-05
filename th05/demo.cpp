// Same arrangement as th04/demo.cpp, and for the same reason: stage_init(),
// stage_transition() and then pause() were the last three things th05_main.asm
// contributed to DEMO_TEXT, so their files are listed ahead of the rest of the
// object, in that order (kb/codegen 0099 + 0114). All three games' halves are
// the one shared th04/main/stage/init.cpp, th04/main/stage/transition.cpp and
// th04/main/pause.cpp.
//
// stage_init() is the proc stage_transition() left exposed at the dump's tail,
// which is the same order th04/demo.cpp's list records for TH04. The file it
// comes from had said this lift "would need a kb/codegen/0080 carve first" --
// true while sub_B55A still sat mid-segment, false once the two procs below it
// left, and corrected at its head.
#include "th04/main/stage/init.cpp"
#include "th04/main/stage/transition.cpp"
#include "th04/main/pause.cpp"
#include "th04/main/demo.cpp"

// Fill the exact stock DEMO_TEXT extent after moving demo policy to the tail.
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90"
