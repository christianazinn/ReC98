// ZUN's object for this code segment held the Extra Stage *midboss*'s update
// function as well as EX-Alice's own code, so both are compiled into this one
// translation unit, in their original address order (kb/codegen/0112). The
// segment pragma lives here rather than in either included file, because only
// the first one to be compiled may name the segment (0112 trap 0).
#pragma option -zCBX_UPDATE_TEXT -zPmain_03

#include "th04/main/midboss/mx_update.cpp"
#include "th05/main/boss/bx.cpp"
