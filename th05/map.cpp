// [-zPmain_01] moved up here from th04/formats/map.cpp so that TH04's wrapper
// can #include a second file ahead of it (kb/codegen/0112 trap 0). TH05's
// mpn_load() is hand-written assembly and stays in th05_main.asm, so this
// wrapper still builds one file only.
#pragma option -zCEND_TEXT -zPmain_01 -k-

#include "th04/formats/map.cpp"

// This segment ended on an `EVEN` in the original object, before the tile
// functions in the next one.
#pragma codestring "\x90"
