/// Thick lasers
/// ------------

#include "th01/math/subpixel.hpp"

#define THICKLASER_COUNT 2

enum thicklaser_flag_t {
	TF_FREE = 0,
	TF_LINE = 1,
	TF_GROW = 2,
	TF_STATIC = 3,
	TF_SHRINK = 4,

	// Forces the underlying type UNSIGNED, and it is load-bearing rather
	// than tidy. `[measured]` Turbo C++ 4.02 gives an enum whose values are
	// 0 to 4 a SIGNED char, and thicklasers_update_and_hittest()'s
	// `flag <= TF_LINE` then comes out `CMP byte ptr [si], 1` / `JLE` where
	// the original has `JBE`. Same instruction, same length -- so no length
	// check can see it, which is kb/codegen/0163's lesson in a second
	// currency. Every other reader compares with `==`, which is
	// sign-agnostic, and `sizeof()` is 1 either way.
	_thicklaser_flag_t_FORCE_UINT8 = 0xFF
};

struct thicklaser_t {
	thicklaser_flag_t flag;
	/* ------------------------- */ int8_t unused_1;
	SPPoint origin;

	// FOUR bytes, not one. th04_main.asm's own `thicklaser_t struc` has four
	// unnamed `db ?` between [origin] and [cur_flag_frame], which puts
	// [radius_cur] at +0x14 and makes the whole structure 24 bytes wide.
	// Modelling this hole as a single byte -- as this file did until
	// thicklasers_render() became the first C++ to read past it -- silently
	// moves every field below by 3 and shrinks `sizeof()` by 3.
	/* ------------------------- */ int8_t unused_2[4];
	int cur_flag_frame;

	// Frames to spend in TF_LINE before transitioning to TF_GROW.
	int line_frames;

	// Frames to spend in TF_STATIC before transitioning to TF_SHRINK.
	int static_frames;

	// Hardware palette color of the outermost layer of the laser. The second
	// layer is rendered with the hardware palette color at this index + 1.
	vc_t col_outline;

	/* ------------------------- */ int8_t unused_3;

	// Target radius during TF_GROW, and fixed radius during TF_STATIC.
	pixel_t radius_max;

	pixel_t radius_cur;

	// Per-frame radius delta during both TF_GROW and TF_SHRINK.
	pixel_t radius_speed;
};

extern thicklaser_t thicklaser_template;
extern thicklaser_t thicklasers[THICKLASER_COUNT];

// Renders every non-TF_FREE thick laser, and nothing else -- the function
// writes no thicklaser_t field, which is why the name ends in _render rather
// than _update_and_render. Lifted, in th04/main/bullet/laser_render.cpp; the
// one remaining ASM caller reaches it through a procdesc in th04_main.asm.
extern "C" void near thicklasers_render(void);

// Advances every non-TF_FREE thick laser through the flag state machine above
// -- TF_LINE for [line_frames], then TF_GROW until [radius_cur] reaches
// [radius_max], then TF_STATIC for [static_frames], then TF_SHRINK back to
// TF_FREE -- and hittests the player against the ones that have a body, i.e.
// everything past TF_LINE.
//
// [inferred] name: it is the only writer of every thicklaser_t field outside
// the spawn path, and its second half sets [player_is_hit], which is what the
// tree's other `_update_and_hittest` symbols do. A naming round is owed.
extern "C" void near thicklasers_update_and_hittest(void);

// Copies [thicklaser_template] over [thicklaser]. Private to ZUN's object --
// thicklaser_add() is its only caller in either dump -- but NOT `static`,
// because the original publishes the mangled name. Moved here from
// th04/main/bullet/laser_t.cpp, which is upstream's structure-only RE file
// and is in no Tupfile, so no compiled translation unit could see it there.
void near pascal thicklaser_template_pull(thicklaser_t near& thicklaser);
