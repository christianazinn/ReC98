// continue_prompt() comes first: it is where this object's contribution to
// EXECL_TEXT starts, so it has to be appended ahead of score_last_commit() and
// GameExecl() to leave every byte at its original address. It cannot live in
// th04/main/execl.cpp, which th05/execl.cpp #includes as a shared body.
// (kb/codegen/0129)
//
// What ends the segment's ROOT contribution is now gameover(), which th04/
// gameover.cpp supplies from its own object, listed ahead of this one in
// Tupfile.lua. It is not #included here because it needs th04/gaiji/gaiji.h,
// and th04/main/continue.cpp below owns this translation unit's only copy of
// that unguarded header.
#include "th04/main/continue.cpp"
#include "th04/main/execl.cpp"
