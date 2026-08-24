#include "th02/main/tile/tile.hpp"
#include "th01/math/subpixel.hpp"

static const int TILE_ROWS_PER_SECTION = 5;

// An absolutely pointless lookup table mapping tile section IDs to offsets in
// [map_seg], which, through its very existence, pointlessly limits the amount
// of tile sections contained in a .MAP file.
static const int TILE_SECTION_COUNT_MAX = 32;
extern const uint16_t TILE_SECTION_OFFSETS[TILE_SECTION_COUNT_MAX];

// TH04 starts addressing individual tiles directly via their 16-bit offset
// in VRAM.
extern vram_offset_t tile_ring[TILES_Y][TILES_MEMORY_X];

extern int8_t tile_row_in_section;

// The [tile_ring] row most recently refilled from [map_seg], as a
// [scroll_line] divided by [TILE_H]. Guards the refill in
// tiles_scroll_and_egc_render() so that it runs once per crossed tile row
// rather than once per frame. [inferred]: the binary only shows the
// comparison. TH05 has the same variable, and th05_main.asm now publishes it
// under this same name.
extern int tile_ring_row_filled;

// Advances the map cursor by one tile row whenever [scroll_line] has crossed
// into a new one, refills the [tile_ring] row that exposed, and EGC-copies
// the lines that scrolled in since the page currently being drawn was last
// rendered. Assumes nothing about the EGC; it starts and stops the copy
// itself. Called once per frame, from scroll_update_and_render() in both
// games (th04/main/scroll.cpp). TH05 shares this C++ body; see
// th04/main/tile/scroll.cpp for the three places the two games differ.
void near tiles_scroll_and_egc_render(void);

// Copies the topmost [scroll_lines_pending] lines of every one of the
// [TILES_X] playfield columns from the tile source area to the playfield,
// starting at [scroll_line] and wrapping through the [tile_ring] as it goes.
// Assumes the EGC to be active and initialized for a copy — the caller above
// is the one that does that. Still ZUN's hand-written assembly, in
// th04_main.asm's CIRCLE_TEXT and th05_main.asm's carved STD_B_TEXT: it pushes
// BP and then uses it as the scratch register for the tile word being copied
// without ever establishing a frame (kb/conventions/handwritten-asm-tells.md).
// TH05's twin is instruction-for-instruction the same proc, so th05_main.asm
// publishes it under this same name.
extern "C" void near tiles_egc_copy_scrolled_lines(void);

// Loads the .MPN file with the given [fn] into slot 0, blits all of its tile
// images to the tile area in VRAM on both pages, and frees the slot again.
// (TH02 splits this into mpn_load() and tile_area_init_and_put_both().)
// TH05's is hand-written assembly and has no C++ body; see
// th04/main/tile/mpn_load.cpp.
#if (GAME != 5)
extern "C" void pascal near mpn_load(const char *fn);
#endif

// Completely fills [tile_ring] with the initial screen of a stage, by loading
// the section IDs from [std_seg], and the tiles themselves from [map_seg].
void pascal near tiles_fill_initial(void);

// Blits all tiles in the ring buffer to the playfield in VRAM.
void pascal near tiles_render_all(void);

// Sets the [tile_ring] tile at (x, y) to the given VRAM offset.
void pascal tile_ring_set_vo(
	subpixel_t x, subpixel_t y, vram_offset_t image_vo
);

// Sets the [tile_ring] tile at (x, y) to the given tile_image_id_t.
#define tile_ring_set(x, y, id) ( \
	tile_ring_set_vo(x, y, tile_image_vo(id)) \
)

/// Redraw
/// ------

// Subdivides each 16×16 tile into two 16×8 halves and marks whether that half
// should be redrawn by the next call to tiles_redraw_invalidated() if its
// entry is nonzero.
extern bool halftiles_dirty[TILE_FLAGS_Y][TILES_MEMORY_X];

void near tiles_invalidate_reset(void);
void near tiles_invalidate_all(void);

// ---------------------------------------------------------------------------
// tiles_invalidate_around() marks all stage background tiles for redrawing
// that lie in the area covered by [tile_invalidate_box] around [center].
// Inconsistencies in the originally generated code revealed that ZUN must
// have used at least two different parameter lists for the same function. To
// use it, the respective prototype has to be declared separately in each
// translation unit, depending on the expected code generation:
//
// • Passing separate X and Y coordinates (including hardcoded constants
//   combined to form a single 32-bit immediate via the -3 compiler option):
//
//   extern "C" void pascal near tiles_invalidate_around(
//   	subpixel_t center_y, subpixel_t center_x
//   );
//
//   Use the tiles_invalidate_around_xy() macro declared below for a more
//   natural parameter order. (Yes, Borland/Turbo C++ only supports __stdcall
//   for Windows targets.)
//
// • Passing SPPoint instances:
//
//   extern "C" void pascal near tiles_invalidate_around(const SPPoint center);

#define tiles_invalidate_around_xy(center_x, center_y) \
	tiles_invalidate_around(center_y, center_x)

#define tiles_invalidate_around_vram_xy(center_x, center_y) \
	tiles_invalidate_around_xy(to_sp(center_x), to_sp(center_y))
// ---------------------------------------------------------------------------

// Width and height, in screen pixels, of a box around the center passed to
// tiles_invalidate_around(). *Not* the radius.
extern point_t tile_invalidate_box;

void pascal near tiles_redraw_invalidated(void);

// Invalidates all entity types, then redraws the invalidated tiles.
void pascal near tiles_render(void);

// Sets [bg_render_not_bombing] to [tiles_render].
void tiles_activate(void);

// Sets [bg_render_not_bombing] to a function that calls [tiles_render_all] for
// the next [n] frames, and then sets the function pointer back to
// [tiles_render]. With [n] = 2, this removes the remnants of in-game dialog
// graphics from both VRAM pages.
// This is only needed for TH04's post-boss dialog; it's also called for the
// pre-boss ones, but [bg_render_not_bombing] is immediately overwritten with
// the boss-specific background render function which does the same job via
// tiles_render_after_custom(). In TH05, the boss-specific functions remove the
// graphics of both the pre-boss and post-boss dialogs, and this function is
// unused.
void pascal tiles_activate_and_render_all_for_next_N_frames(uint8_t n);

// Used for switching back to a tiled background after rendering anything else,
// like in-game dialog, or a custom background. Makes sure to first render all
// tiles to both VRAM pages, then performs regular redrawing of only the
// invalidated tiles.
inline void tiles_render_after_custom(const int& frame) {
	if(frame <= 2) {
		tiles_render_all();
	} else {
		tiles_render();
	}
}
/// ------
