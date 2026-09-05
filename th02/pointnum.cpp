// Five contiguous runs of this code segment, compiled into one translation
// unit here in their original address order: the GAME OVER screen and its
// continue prompt, the cleanup/handoff function, the per-attempt player reset,
// the player invalidation pass, and the point number code. The order is the
// addresses' - each file's contribution begins exactly where the previous one
// ends.

// th02/main/cfg_load.cpp is linked ahead of this object into the same segment,
// as ZUN's own two objects were; see that file for why it cannot be folded in
// here as a sixth #include.

// -zC/-zP only take effect before any code is generated, so the group has to
// be named here rather than in an included file (kb/codegen/0112). The segment
// name still comes from this wrapper's own basename (kb/codegen/0105).
#pragma option -zPmain_01 -G

#pragma codeseg T2GOSAVE_TEXT
void far t2gosave_post_regist(void);
#pragma codeseg POINTNUM_TEXT main_01

#include "th02/main/continue.cpp"
#include "th02/main/gameexecl.cpp"
#include "th02/main/player/reset.cpp"
#include "th02/main/player/invalidate.cpp"
#include "th02/main/pointnum/pointnum.cpp"
#include "th02/main/gameover_save.cpp"
