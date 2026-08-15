// ZUN's object for this code segment held the Stage 1 and Stage 2 boss
// foreground renderers and the midboss defeat animation, so all three are
// compiled into this one translation unit, in their original address order.
// (kb/codegen/0112)
//
// The group pragma lives here rather than in either included file: it only
// takes effect before any code is generated, so a second identical
// `#pragma option` in the file included last is rejected outright.
// (kb/codegen/0112, trap 0)
#pragma option -zPmain_01

#include "th04/main/boss/render.cpp"
#include "th04/main/midboss/defeat_render.cpp"
