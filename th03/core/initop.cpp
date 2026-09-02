#pragma option -zCSHARED -3

#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th01/hardware/vplanset.h"
#include "th03/core/initexit.h"

#if (GAME <= 3)
#if (GAME == 3)
// Replay Patch: Leave conventional memory for MMD's compact resident song
// buffer. The original 352,000-byte master.lib arena is overprovisioned
// relative to observed OP allocation demand; release boot and intro-skip
// probes exercise the process transition against this 12,000-byte reduction.
#define mem_assign_paras (340000 >> 4)
#else
#define mem_assign_paras (352000 >> 4)
#endif
#endif

int game_init_op(const unsigned char *pf_fn)
{
	if(mem_assign_dos(mem_assign_paras)) {
		return 1;
	}
#if (GAME <= 4)
	vram_planes_set();
#endif
	graph_start();
	graph_clear_both();
#if (GAME >= 4)
	pfsetbufsiz(8192);
#endif
	vsync_start();
	key_beep_off();
	text_systemline_hide();
	text_cursor_hide();
	egc_start();
	js_start();

#if (GAME >= 4)
	if(pf_fn[0]) {
		pfstart(pf_fn);
	}
	bgm_init(1024);
#else
	pfstart(pf_fn);
#endif
	return 0;
}
