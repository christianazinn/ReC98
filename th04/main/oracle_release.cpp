#pragma option -zCORACLE_TEXT

// Production facade for the validation oracle hooks. Distributable builds do
// not carry the T4CASE1 / T5CASE1 implementation, but DemoPlay() keeps its
// stock input policy behind the same far hook boundary.

#include "th04/hardware/inputvar.h"
#include "th04/main/frames.h"
#include "th04/main/demo.hpp"
#include "th04/main/oracle.hpp"
#if (GAME == 5)
#include "th05/resident.hpp"
#else
#include "th04/resident.hpp"
#endif

void oracle_entry(void)
{
}

bool oracle_active(void)
{
	return false;
}

bool oracle_frame(uint16_t)
{
	return false;
}

bool oracle_or_demo_frame(uint16_t shift_offset)
{
	if(
		((GAME == 5) && ((key_det & ~INPUT_MOVEMENT) == 0)) ||
		((GAME == 4) && (key_det == INPUT_NONE))
	) {
		key_det = DemoBuf[stage_frame];
		shiftkey = DemoBuf[stage_frame + shift_offset];
		if(
			((GAME == 5) && (resident->demo_num > 4)) ||
			(stage_frame < (DEMO_N - 4))
		) {
			return true;
		}
	}
	return false;
}

// Keep the following stock runtime segment at its original paragraph phase.
#if (GAME == 5)
	#pragma codestring "\x90\x90\x90\x90\x90\x90\x90"
#else
	#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#endif
