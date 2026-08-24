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
