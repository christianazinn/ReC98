/// Text dissolve circles: loading and freeing
/// ------------------------------------------
/// (#included from th04/main_01.cpp, AHEAD of th04/main/boss/bx1_fg.cpp,
/// which is the address order ZUN's own object for main_01_TEXT had — that
/// object held this file, the Mugetsu renderer and then the barrier
/// (kb/codegen/0112). Both headers below are guarded, so bx1_fg.cpp keeps its
/// own #includes and this file adds no unguarded header to that translation
/// unit.)
///
/// TH04 only. TH05's implementation in th05/formats/bb_txt_load.cpp shares
/// the two names and nothing else: it is frameless, reads through DOS directly
/// instead of master.lib, and rewrites its own filename string in place, so
/// there is no shared body here to fence with `#if (GAME == 5)`.

#include "libs/master.lib/master.hpp"
#include "th04/formats/bb.h"

// The two files, in th04/formats/bb_txt_load[data].asm.
extern "C" const char BB_TXT_FN[];
extern "C" const char BB_TXT2_FN[];

extern "C" void pascal near bb_txt_load(void)
{
	// One allocation for both files, so the cel index the blitter uses runs
	// straight across the two: `sizeof(bb_txt_t)` is BB_SIZE + (BB_SIZE / 2),
	// which is the size the original passes.
	bb_txt_seg = HMem<bb_txt_t>::alloc(1);

	file_ropen(BB_TXT_FN);
	file_read(bb_txt_seg, BB_SIZE);
	file_close();

	file_ropen(BB_TXT2_FN);
	file_read(bb_txt_seg->in, (BB_SIZE / 2));
	file_close();
}

extern "C" void pascal near bb_txt_free(void)
{
	if(bb_txt_seg) {
		HMem<bb_txt_t>::free(bb_txt_seg);
		bb_txt_seg = 0;
	}
}
