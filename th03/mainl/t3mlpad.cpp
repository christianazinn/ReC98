#pragma option -zCT3CASE_MAINL_PAD_TEXT
#pragma option -zPgroup_01

// Validation-only linker padding. The T3CASE cutscene hooks have different
// instruction-size deltas in the original and combined Anniversary builds.
// Round either delta to whole paragraphs without adding verifier code to an
// original segment contribution.
#include "th03/t3case_build.hpp"

#if (T3CASE_PRODUCER == T3CASE_PRODUCER_ANNIVERSARY_MOD)
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90"
#endif
#pragma codestring "\x90"
