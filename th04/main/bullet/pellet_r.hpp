#include "th04/main/bullet/bullet.hpp"
#include "planar.h"

union pellet_render_t {
	struct {
		screen_x_t left;
		vram_y_t top;
	} top;
	struct {
		vram_offset_t vram_offset;
		uint16_t sprite_offset;
	} bottom;
};

#if (GAME == 5)
// Separate render list for pellets during their delay cloud stage.
extern int pellet_clouds_render_count;
extern bullet_t near *pellet_clouds_render[PELLET_COUNT];

// GRCG color for pellets_render_bottom(), set per stage.
extern int pellet_bottom_col;
#endif

extern int pellets_render_count;
extern pellet_render_t pellets_render[PELLET_COUNT];

// Renders the top and bottom part of all pellets, as per [pellets_render] and
// [pellets_render_count]. Assumptions:
// • ES is already set to the beginning of a VRAM segment
// • The GRCG is active, and set to the intended color
// • pellets_render_top() is called before pellets_render_bottom()
//
// th04/main/bullet/pellet_r.asm publishes both of these UNDECORATED
// (`public _pellets_render_top`), so the declarations need C linkage. They sat
// outside an `extern "C"` for as long as this header had no C++ caller, which
// is the same latent defect th04/formats/super.h records for
// z_super_put_16x16_mono_raw().
extern "C" {
void near pellets_render_top();
void near pellets_render_bottom();
}
