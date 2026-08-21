// One object for marisa_update(), enemies_add() and std_run(), which sit
// back-to-back at the end of what the dump still contributes to
// ENM_BTPL_TEXT. Code is emitted in
// source order within an object, so this #include order *is* the original
// address order (kb/codegen/0112 + 0114). The segment pragma has to live here
// rather than in either included file, because it only takes effect before any
// code is generated.
#pragma option -zCENM_BTPL_TEXT -zPmain_03

#include "th04/main/boss/b4m_upd.cpp"
#include "th04/main/enemy/add.cpp"
#include "th04/formats/std_run.cpp"
