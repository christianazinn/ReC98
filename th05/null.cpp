#pragma option -zCRANDRING_NEXT_TEXT -zPmain_01

#pragma codeseg SCORE_I_TEXT main_01
void near items_invalidate(void);
#pragma codeseg RANDRING_NEXT_TEXT main_01

#include "th03/math/randring_fill.cpp"

#pragma option -k-
#pragma codeseg SCORE_I_TEXT main_01
#include "th04/main/item/invalidate.cpp"
#pragma option -k.
#pragma codeseg RANDRING_NEXT_TEXT main_01

#include "th04/main/null.cpp"

// END_EXT_B_TEXT's 0x38-byte root loader precedes these shared C++ bodies.
#pragma codeseg END_EXT_B_TEXT main_01
#include "th04/main/tile/bb_put_a.cpp"
#pragma codeseg RANDRING_NEXT_TEXT main_01
