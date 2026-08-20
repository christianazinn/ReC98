// Same arrangement as th04/demo.cpp, and for the same reason: stage_transition()
// and then pause() were the last two things th05_main.asm contributed to
// DEMO_TEXT, so their files are listed ahead of the rest of the object, in
// that order (kb/codegen 0099 + 0114). Both games' halves are the one shared
// th04/main/stage/transition.cpp and th04/main/pause.cpp.
#include "th04/main/stage/transition.cpp"
#include "th04/main/pause.cpp"
#include "th04/main/demo.cpp"
