#include "th02/formats/mpn.hpp"
#include <stddef.h>

// Internal .MPN slot structure
struct mpn_t {
	mpn_image_t far *images;
	size_t count;
	Palette8 palette;
	int8_t unused[10]; // ZUN bloat
};

// TH04 reserves memory for 8 slots, but only actually uses the first one.
static const int MPN_COUNT = 8;

extern mpn_t mpn_slots[MPN_COUNT];

extern "C" {

// Frees the .MPN images in the given [slot].
void pascal mpn_free(int slot);

// Sets the hardware color palette to the one in the given .MPN [slot].
void pascal mpn_palette_show(int slot);

// Frees the images in the given .MPN [slot], then loads the file with the
// given name into the same slot, and sets the hardware color palette to the
// one in this slot. Returns 0 if allocation succeeded and the tiles were read
// into the given [slot], -1 otherwise.
int pascal mpn_load_palette_show(int slot, const char *fn);

// Blits the given [image] from the .MPN in the given [slot] to
// (⌊left/8⌋*8, top).
//
// [measured 2026-08-15] Unlike TH02's, this one *does* bail out for an [image]
// past ([mpn_slots[slot]].count), so TH02's `ZUN landmine` about
// tile_area_init_and_put_both() blitting undefined data past the loaded tile
// count does not carry over to TH04. mpn_load() below still walks all
// TILE_IMAGE_COUNT positions; the surplus ones are simply left untouched.
void pascal mpn_put_8(screen_x_t left, vram_y_t top, int slot, int image);

}
