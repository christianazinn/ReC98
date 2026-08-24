// ZUN's object for this code segment held both the .MPN loading/tile area
// initialization and the .MAP loading code, so both are compiled into this one
// translation unit, in their original address order (kb/codegen/0112 + 0114).
// The segment pragma has to sit above both #includes, or the second file's
// identical pragma is rejected (kb/codegen/0112 trap 0).
#pragma option -zCEND_TEXT -zPmain_01

#include "th04/main/tile/mpn_load.cpp"
#include "th04/formats/map.cpp"
