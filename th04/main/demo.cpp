#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/main/execl.hpp"
#include "th04/hardware/inputvar.h"
#include "th04/main/frames.h"
#include "th04/main/demo.hpp"
#include "th04/main/oracle.hpp"
#if (GAME == 5)
#include "th05/resident.hpp"
#else
#include "th04/resident.hpp"
#endif

void near demo_load(void)
{
#if (GAME == 5)
	size_t size = ((resident->demo_num <= 4)
		? sizeof(REC<DEMO_N>)
		: sizeof(REC<DEMO_N_EXTRA>)
	);
#else
	#define size sizeof(REC<DEMO_N>)
#endif

	extern char near demo_fn[];
	DemoBuf = static_cast<uint8_t *>(hmem_allocbyte(size));
	char* fn = demo_fn;
	fn[4] = ('0' - (GAME == 5) + resident->demo_num);

	file_ropen(fn);
	file_read(DemoBuf, size);
	file_close();
}

void near DemoPlay(void)
{
	#undef BINARY_OP
	#define BINARY_OP DEMOPLAY_BINARY_OP
	extern const char BINARY_OP[];

#if (GAME == 5)
	size_t shift_offset = (resident->demo_num <= 4) ? DEMO_N : DEMO_N_EXTRA;
#else
	#define shift_offset DEMO_N
#endif

	if(oracle_or_demo_frame(shift_offset)) {
		return;
	}
	demo_end();
}
