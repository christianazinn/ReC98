// Append the loader to the carved head segment. The original END_EXT_A_TEXT
// reopens immediately afterward with the hand-written cheeto_put().
#pragma option -zCEND_EXT_H_TEXT -zPmain_01 -k-

#include "th05/formats/bb_cheeto.cpp"

// Original padding before cheeto_put().
#pragma codestring "\x90"
