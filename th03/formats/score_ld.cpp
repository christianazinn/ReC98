#include "th03/scorefile.hpp"

#if (BINARY == 'L')
#define recreated
#define loaded
void pascal near scoredat_load_and_decode(rank_t rank)
#else
#define recreated true
#define loaded false
bool16 pascal near scoredat_load_and_decode(rank_t rank)
#endif
{
	#if (BINARY == 'L')
	scorefile_compat_load(rank);
	return;
	#else
	return scorefile_compat_load(rank);
	#endif
}
#undef loaded
#undef recreated

#if (BINARY == 'O')
// Keep the following original OP contribution at its accepted phase after
// replacing the legacy loader with the expanded score-store call.
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#endif
