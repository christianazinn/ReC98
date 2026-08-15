/// .MPN loading, and initialization of the VRAM tile area
/// ------------------------------------------------------
/// Loads a stage's .MPN into slot 0, blits every one of its tile images into
/// the reserved tile area in VRAM on both pages, and frees the slot again. The
/// tile area *is* the copy the game blits from for the rest of the stage, so
/// the .MPN itself is only needed for the length of this function.
///
/// TH02 splits the same work across mpn_load_palette_show()
/// (th02/formats/mpn.hpp:29) and tile_area_init_and_put_both()
/// (th02/main/tile/tile.cpp:78), and rolls the two VRAM pages into a `do` loop
/// with a page toggle. TH04 fused those two, kept the .MPN one's name, and
/// unrolled the pages. NOT TH02's own mpn_load() (th02/main02_1.cpp:12-21):
/// that one exists precisely to *suppress* the palette show which this
/// function performs, so it is the one TH02 function this is not built from.
///
/// TH05's mpn_load() is ZUN's hand-written assembly and stays in
/// th05_main.asm. See state/notes/mpn_load.md, and
/// state/notes/_map_free_qv.md:161-179, :308-309 for the carve. The short
/// version is that it uses BP as its tile counter after establishing a standard
/// frame and reading its own parameter through it, which Turbo C++ never does.
/// That tell is the decisive one on its own. Its helpers corroborate but do not
/// carry it: all four are frameless, yet only two (mpn_load_inner,
/// th05_main.asm:177, and sub_4226, :251) are `MOV BX, SP` procs, and both of
/// those do push and pop SI/DI — so upstream's three-condition exclusion
/// (kb/conventions/handwritten-asm-tells.md, which also warns that one
/// frameless proc means little because `-k-` exists) does not fire on them.
///
/// This file is compiled as the first half of th04/map.cpp (kb/codegen/0112):
/// mpn_load() was the LAST proc of END_TEXT's root contribution and
/// th04/formats/map.cpp already owned everything after it (kb/codegen/0114),
/// so the `#include` order below is the original address order, and neither a
/// carve nor a Tupfile.lua line was needed.

#include "libs/master.lib/master.hpp"
#include "th04/formats/mpn.hpp"
#include "th04/main/tile/tile.hpp"

// ZUN's object for this code segment also held the tile ring scroller,
// immediately ahead of mpn_load(). #included from here rather than from the
// th04/map.cpp wrapper because it wants this file's header set
// (kb/codegen/0129).
#include "th04/main/tile/scroll.cpp"

extern "C" void pascal near mpn_load(const char *fn)
{
	int tile_x;
	int tile_y;
	int image;
	register screen_x_t left;
	register vram_y_t top;

	mpn_load_palette_show(0, fn);

	image = 0;
	for(
		tile_x = 0, left = TILE_AREA_LEFT;
		tile_x < TILE_AREA_COLUMNS;
		tile_x++, left += TILE_W
	) {
		for(
			tile_y = 0, top = TILE_AREA_TOP;
			tile_y < TILE_AREA_ROWS;
			tile_y++, top += TILE_H
		) {
			// Both pages, because tiles_render_all() blits from the tile area
			// on whichever page it happens to be writing to.
			graph_accesspage(1);
			mpn_put_8(left, top, 0, image);
			graph_accesspage(0);
			mpn_put_8(left, top, 0, image);
			image++;
		}
	}
	mpn_free(0);
}
