#pragma option -zCSHOT_INV_TEXT -zPmain_01

// bb_playchar_put() was the whole of this segment's dump contribution, so it
// goes FIRST here: the object grows backwards into the hole the dump leaves and
// every byte keeps its address (kb/codegen/0114). Same shape as th05/main010.cpp,
// which adopted the identical file for TH05's `mai_TEXT` — this is the ADOPTION
// of that file, not a second implementation of the function.
#include "th04/main/player/bb_playchar_put.cpp"

#include "th04/main/player/shots_inv.cpp"
