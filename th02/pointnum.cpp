// ZUN's object for this code segment held both the player invalidation pass
// and the point number code, so both are compiled into this one translation
// unit here, in their original address order.

// -zC/-zP only take effect before any code is generated, so the group has to
// be named here rather than in an included file (kb/codegen/0112). The segment
// name still comes from this wrapper's own basename (kb/codegen/0105).
#pragma option -zPmain_01 -G

#include "th02/main/player/reset.cpp"
#include "th02/main/player/invalidate.cpp"
#include "th02/main/pointnum/pointnum.cpp"
