/// Invalidating the tiles below the player
/// ---------------------------------------
/// (#included from th04/main_.cpp, behind th04/main/enemy/render.cpp and
/// therefore in its original address order. TH04-only: TH05's function of the
/// same name is a different, hand-written body in th05/player.asm, which is
/// why th04/main/player/invalidate.asm can be deleted outright.)
///
/// Either the player and its two options, or -- while the miss animation is
/// running -- the eight points of the miss explosion, which are placed on the
/// ring the PREVIOUS frame used. The ring is walked with the same
/// half-radius/reversed-angle switch at the halfway point that
/// player_render() (th04/main/player/render.cpp) uses to draw it.

#include "platform.h"
#include "pc98.h"
#include "th04/main/drawp.hpp"
#include "th04/main/player/player.hpp"
#include "th04/math/vector.hpp"

// th04/main/tile/tile.hpp's two declarations, spelled here rather than
// reached through that header: it has no include guard and declares
// `static const` objects and an `inline` function, which a second expansion
// in this multi-source object would reject (kb/codegen/0129).
// ---------------------------------------------------------------------
// Width and height, in screen pixels, of the box around the center passed to
// tiles_invalidate_around(). *Not* the radius.
extern point_t tile_invalidate_box;

// See th04/main/tile/tile.hpp for why this declaration is a per-TU choice.
// This is the SPPoint form, like th04/main/enemy/inv.cpp: the original pushes
// [pos.prev] as one dword rather than as two words.
extern "C" void pascal near tiles_invalidate_around(const SPPoint center);

// ...and this body is the first that needs the OTHER parameter list in the
// same translation unit. It cannot be spelled under the same name: Turbo C++
// 4.02 rejects a second `extern "C"` declaration of one identifier, and
// rejects a block-scoped one with "Linkage specification not allowed in
// function". th04/main/tile/inv.asm therefore publishes a second, zero-byte
// name for the very same entry point, and this is it. The proc takes one
// dword either way; what differs is only how the caller delivers it, because
// `-3` folds a pair of adjacent `pascal` arguments into one 32-bit push
// whenever it can prove the two are contiguous -- kb/codegen/0125 records
// that for two constants; two members of one struct fold as well.
extern "C" void pascal near tiles_invalidate_around_yx(
	subpixel_t center_y, subpixel_t center_x
);

// The two halves of [drawpoint], as the separate 16-bit variables ZUN's source
// had them in at this one call site. Passing `drawpoint.y, drawpoint.x`
// instead makes `-3` fold the pair into one 32-bit push, because it can prove
// two members of one object are contiguous; two distinct globals it cannot.
// Measured with `tcc -S` over five spellings; see
// th04/main/drawpoint[bss].asm, which publishes them.
extern "C" subpixel_t drawpoint_x;
extern "C" subpixel_t drawpoint_y;
// ---------------------------------------------------------------------

void near player_invalidate(void)
{
	unsigned char angle;

	// The two register variables. [i] is mentioned once more often than
	// [radius] and therefore takes SI; the two are STORED the other way
	// round, which is independent of the declaration order (kb/codegen/0146).
	register int i;
	register subpixel_t radius;

	tile_invalidate_box.y = PLAYER_H;
	if(miss_time != 0) {
		// The explosion is invalidated at the position it was RENDERED at,
		// which is one frame behind the parameters miss_update() has already
		// advanced.
		tile_invalidate_box.x = MISS_EXPLOSION_W;
		radius = (miss_explosion_radius - MISS_EXPLOSION_RADIUS_VELOCITY);
		i = 0;
		angle = (miss_explosion_angle - MISS_EXPLOSION_ANGLE_VELOCITY);
		while(i < MISS_EXPLOSION_COUNT) {
			if(i == (MISS_EXPLOSION_COUNT / 2)) {
				radius /= 2;
				angle = -angle;
			}
			vector2_at(
				drawpoint,
				player_pos.cur.x,
				player_pos.cur.y,
				radius,
				angle
			);
			if(
				(drawpoint.y >= TO_SP(
					PLAYFIELD_TOP - (MISS_EXPLOSION_H / 2)
				)) &&
				// ZUN bug: Both of these are wrong, exactly as they are in
				// player_render().
				(drawpoint.y < TO_SP(PLAYFIELD_BOTTOM - 8)) &&
				(drawpoint.x >= TO_SP(PLAYFIELD_LEFT - 40)) &&
				(drawpoint.x < TO_SP(
					PLAYFIELD_RIGHT - (MISS_EXPLOSION_W / 2)
				))
			) {
				// Two words, not one dword -- see the second declaration at
				// the top of this file.
				tiles_invalidate_around_yx(drawpoint_y, drawpoint_x);
			}
			i++;
			angle += (256 / (MISS_EXPLOSION_COUNT / 2));
		}
	} else {
		tile_invalidate_box.x = PLAYER_W;
		tiles_invalidate_around(player_pos.prev);

		// One box around both options and the player between them, rather
		// than one box per option.
		tile_invalidate_box.x = (
			PLAYER_OPTION_W + PLAYER_W + PLAYER_OPTION_W
		);
		tile_invalidate_box.y = PLAYER_OPTION_H;
		tiles_invalidate_around(player_option_pos_prev);
	}
}
