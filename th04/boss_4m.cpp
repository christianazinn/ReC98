// The segment and group pragma moved up here from th04/main/boss/b4m.cpp,
// because `-zC` only takes effect before any code is generated
// (kb/codegen/0105) and this object now has a file ahead of that one.
// Unchanged in both values.
#pragma option -zCB4M_UPDATE_TEXT -zPmain_03

// POSITION-CRITICAL: boss_explode_big() was the last thing th04_main.asm
// contributed to B4M_UPDATE_TEXT, immediately above this object, so it has to
// come first here (kb/codegen/0112 + 0114).
#include "th04/main/boss/explode_big.cpp"
#include "th04/main/boss/b4m.cpp"
