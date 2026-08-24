#include "th04/main/null.hpp"

// TASM's `even` after each original function emitted a NOP. Keep those bytes
// in this object so neither following symbol nor segment moves.
#pragma option -k-
extern "C" {
void pascal near nullfunc_near(void) {}
#pragma codestring "\x90"

void pascal far nullfunc_far(void) {}
#pragma codestring "\x90"
}
#pragma option -k.
