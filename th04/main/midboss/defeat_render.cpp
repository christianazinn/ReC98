/// Midboss defeat animation
/// ------------------------
/// One name, ONE body, with zero `#if (GAME == 5)` sites: the two dumps of
/// this function differ in nothing but the name IDA gave the ring's rotation
/// variable and TASM's group qualifier on the scroll call, and neither is a
/// source-level difference.
///
/// Renders the expanding ring of MIDBOSS_DEFEAT_SPRITE_COUNT explosion sprites
/// that midboss_defeat_update() animates. The ring grows with
/// [midboss.phase_frame] until it reaches MIDBOSS_DEFEAT_RADIUS_MAX, and only
/// starts rotating once it stops growing.

// `#pragma option -zPmain_01` now lives in th04/mb_dfr.cpp, which #includes
// this file after th04/main/boss/render.cpp. (kb/codegen/0112, trap 0)

#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th03/math/polar.hpp"
#include "th04/main/midboss/midboss.hpp"

// Rotation of the ring, in master.lib's 8-bit angle units. Advanced by one
// full sprite spacing between sprites, and by 1 per frame once the ring has
// stopped expanding.
extern uint8_t midboss_defeat_angle;

// The number of instances of the one explosion sprite placed around the ring,
// not the number of distinct cels in an animation — hence CONTRIBUTING's
// `<ENTITY>_COUNT` and not `*_CELS`.
static const int MIDBOSS_DEFEAT_SPRITE_COUNT = 16;

// `int` rather than the `uint8_t` this really is: a byte-typed constant
// makes Turbo C++ fold the compound assignment below into a single
// `ADD mem, imm8`, while an int-typed one keeps the original's round trip
// through AL. (kb/codegen/0094)
static const int MIDBOSS_DEFEAT_SPRITE_SPACING = (
	0x100 / MIDBOSS_DEFEAT_SPRITE_COUNT
);

static const subpixel_t MIDBOSS_DEFEAT_RADIUS_MAX = TO_SP(48);

// Sprites within this many pixels of the playfield edge are still rendered.
static const pixel_t MIDBOSS_DEFEAT_MARGIN = 16;

void near midboss_defeat_render(void)
{
	// Declared before [radius] to put it at [bp-2] rather than [bp-4], which
	// is where the original has it. (kb/codegen/0010)
	int i;

	subpixel_t radius = TO_SP(midboss.phase_frame);
	if(radius >= MIDBOSS_DEFEAT_RADIUS_MAX) {
		radius = MIDBOSS_DEFEAT_RADIUS_MAX;

		// `+= 1` rather than `++`: Turbo C++ compiles `++` on a byte global
		// to a single 4-byte `INC mem`, while a compound assignment goes
		// through AL, which is what the original does — here and at the end
		// of the loop below. (kb/codegen/0094)
		midboss_defeat_angle += 1;
	}

	// The rotation advance lives in the increment expression, not at the end
	// of the body: the original increments [i] first.
	for(
		i = 0;
		i < MIDBOSS_DEFEAT_SPRITE_COUNT;
		i++, midboss_defeat_angle += MIDBOSS_DEFEAT_SPRITE_SPACING
	) {
		subpixel_t x = polar_x(
			midboss.pos.cur.x, radius, midboss_defeat_angle
		);
		subpixel_t y = polar_y(
			midboss.pos.cur.y, radius, midboss_defeat_angle
		);
		if(
			(y > TO_SP(-MIDBOSS_DEFEAT_MARGIN)) &&
			(y < TO_SP(PLAYFIELD_H + MIDBOSS_DEFEAT_MARGIN)) &&
			(x > TO_SP(-MIDBOSS_DEFEAT_MARGIN)) &&
			(x < TO_SP(PLAYFIELD_W + MIDBOSS_DEFEAT_MARGIN))
		) {
			// Both coordinates are converted in place. Separate
			// screen_x_t / vram_y_t locals would be two more stack slots;
			// the original has exactly two, so [x] and [y] must be the
			// register variables that carry the converted values too.
			x = (TO_PIXEL(x) + (PLAYFIELD_LEFT - MIDBOSS_DEFEAT_MARGIN));
			y = scroll_subpixel_y_to_vram_seg1(y);
			super_roll_put(x, y, midboss.sprite);
		}
	}
}
