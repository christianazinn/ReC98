#include "th02/main/scroll.hpp"
#include "th01/math/subpixel.hpp"

// [scroll_line] is advanced by 1 for every 16 units.
extern SubpixelLength8 scroll_subpixel_line;

// Amount to add to [scroll_subpixel_line] every frame. Replaces TH02's
// separate pixel-based [scroll_speed] and [scroll_interval] with a single
// variable.
extern SubpixelLength8 scroll_speed;

extern vram_y_t scroll_line_on_page[PAGE_COUNT];

// Playfield-space pixels scrolled in the last frame.
extern Subpixel scroll_last_delta;

// If false, the game doesn't draw stage tiles, assuming that someone else
// draws the background.
extern bool scroll_active;

// Playfield lines that scrolled in since the VRAM page currently being drawn
// was last rendered, and that tiles_scroll_and_egc_render() therefore still
// has to copy into it. Set to this frame's own scroll delta before that
// function is called, raised by [scroll_lines_prev_frame] inside it, and
// cleared once the copy is done. [inferred]: the binary only shows the
// arithmetic, so "still to be copied" is read off the two writers and the one
// consumer (tiles_egc_copy_scrolled_lines()), not off a name of ZUN's.
extern pixel_length_8_t scroll_lines_pending;

// The previous frame's [scroll_lines_pending], added to the current frame's
// on the next call. The two VRAM pages alternate, so the page being drawn
// missed the previous frame's scroll entirely and has to catch up on both.
// [inferred], same evidence as above.
//
// TH05 has both of these variables too, in the same roles, and th05_main.asm
// now publishes them under these same two names -- they used to be a pair of
// unnamed bytes in its _BSS. TH05's tiles_scroll_and_egc_render() shares this
// file's C++ body; see th04/main/tile/scroll.cpp.
extern pixel_length_8_t scroll_lines_prev_frame;

// Records this frame's [scroll_line] for the VRAM page about to be drawn,
// hardware-scrolls the display to the position the previous frame computed,
// advances [scroll_subpixel_line] by [scroll_speed] and turns every whole
// pixel it crosses into a [scroll_line] step plus a [scroll_lines_pending] /
// [scroll_last_delta] pair, then calls tiles_scroll_and_egc_render(). Called
// once per frame from the stage loop, just before the page flip.
// ONE body for both games; the two Stage 6 tests are TH05's alone. See
// th04/main/scroll.cpp.
void near scroll_update_and_render(void);

#pragma codeseg mai_TEXT main_01

// Transforms [y] to its corresponding VRAM line, adding the current
// [scroll_line] or 0 if scrolling is disabled.
extern "C" vram_y_t pascal near scroll_subpixel_y_to_vram_seg1(subpixel_t y);

#pragma codeseg

extern "C" {

// Transforms [y] to its corresponding VRAM line, adding the current
// [scroll_line] or 0 if scrolling is disabled.
vram_y_t pascal near scroll_subpixel_y_to_vram_seg3(subpixel_t y);

// Like the one above, but always adds [scroll_line], even if scrolling is
// disabled.
vram_y_t pascal near scroll_subpixel_y_to_vram_always(subpixel_t y);

}
