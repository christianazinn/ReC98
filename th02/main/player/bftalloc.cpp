#pragma option -zCT2BFT_TEXT -zPT2BFT_TEXT -G-

#include "th02/main/player/bomb_bft.hpp"

static const unsigned BOMB_BFT_CEL_SIZE = 72;

void __seg * MASTER_RET bomb_bft_alloc(unsigned bytesize)
{
	uint8_t __seg *block = reinterpret_cast<uint8_t __seg *>(
		hmem_allocbyte(bytesize + BOMB_BFT_CEL_SIZE)
	);
	if(block) {
		uint8_t far *tail = block;
		tail += bytesize;
		// ZUN bug: The original 15-cel allocation is read at cel 15, drawing
		// the next heap block's metadata. Retain the final full-field mask.
		for(unsigned i = 0; i < BOMB_BFT_CEL_SIZE; i++) {
			tail[i] = 0xFF;
		}
	}
	return block;
}
