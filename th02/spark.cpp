// ZUN's object for this code segment held both the boss background particles
// and the sparks, so both are compiled into this one translation unit, in
// their original address order. (kb/codegen/0112)
//
// -zC/-zP only take effect before any code is generated, so the segment has to
// be named here rather than at the top of either included file.
#pragma option -zC_TEXT -zPTEXT -2

#include "th02/main/bg_particle.cpp"
#include "th02/main/spark.cpp"
