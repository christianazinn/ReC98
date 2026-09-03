#pragma option -zCMIDI_TEXT -zPMIDI_TEXT

#include "libs/master.lib/master.hpp"

extern unsigned mem_TopSeg;

int pascal th03_midi_mem_assign_dos(unsigned paras)
{
	if(!mem_assign_dos(paras)) {
		return 0;
	}

	// Replay Patch: MMD's compact resident shifts PMD and the game upward by
	// 22,576 bytes. Preserve the original arena whenever DOS can provide it.
	// int 21h, AH=48h leaves no allocation behind on failure, so the fallback
	// can safely reserve the maximum remaining block. MASTER.LIB retains its
	// standard 4 KiB DOS reserve while giving gameplay every other available
	// paragraph instead of imposing another fixed, potentially unsafe ceiling.
	mem_assign_all();
	return (mem_TopSeg == 0);
}

// Keep the following compiler runtime segment at its accepted paragraph
// phase. These bytes live entirely in this patch-owned segment.
#pragma codestring "\x90\x90\x90\x90"
