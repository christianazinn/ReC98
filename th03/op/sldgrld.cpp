#pragma option -zCSELCDGR_TEXT -zPSELCDGR_TEXT

#include "libs/master.lib/master.hpp"
#include "th03/formats/cdg.h"

void pascal far select_cdg_reload_selected(int slot, const char *fn, int n)
{
	CDG header;
	CDG near &allocated = cdg_slots[slot];
	long image_size;

	// Selection permanently reserves two full portrait slots. Reusing those
	// buffers avoids master.lib's unchecked allocation path, which can return
	// segment 0 after translated archive swaps fragment OP's smaller MIDI-era
	// heap.
	if(!allocated.seg_alpha() || !allocated.seg_colors()) {
		return;
	}
	if(!file_ropen(fn)) {
		return;
	}
	if(file_read(&header, sizeof(header)) != sizeof(header)) {
		file_close();
		return;
	}
	if(
		(header.bitplane_size != allocated.bitplane_size) ||
		(header.pixel_w != allocated.pixel_w) ||
		(header.pixel_h != allocated.pixel_h)
	) {
		file_close();
		return;
	}
	image_size = (header.bitplane_size * (PLANE_COUNT + 1L));
	file_seek((n * image_size), SEEK_CUR);
	file_read(allocated.seg_alpha(), header.bitplane_size);
	file_read(
		allocated.seg_colors(), (header.bitplane_size * PLANE_COUNT)
	);
	file_close();
}
