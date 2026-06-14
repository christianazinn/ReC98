#pragma codeseg CUTSCENE_TEXT group_01

#define cdg_free_all cdg_free_all_default_distance
#include "th03/formats/cdg.h"
#undef cdg_free_all

extern "C" void near pascal cdg_free_all(void)
{
	for(register int slot = 0; slot < CDG_SLOT_COUNT; slot++) {
		cdg_free(slot);
	}
}

#pragma codeseg
