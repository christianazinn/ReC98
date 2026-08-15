/// Invalidating the background tiles behind the player's shots
/// -----------------------------------------------------------
/// ONE body with three `#if (GAME == 5)` sites, but the two halves diverge
/// completely after the shot loop: TH05 invalidates its separate [hitshots]
/// array, while TH04 invalidates the option laser that TH05 kept in the code
/// but never fires.
///
/// The two loops in TH05 share one counter, which is why it is the game with a
/// stack local here: [shot] and [hitshot] are both dereferenced three times per
/// iteration and take SI and DI, leaving [i] at [bp-2]. TH04 has only one
/// pointer, so [i] gets DI and the function needs no stack frame at all.
/// (kb/codegen/0117)

#include "th04/main/player/shot.hpp"
#include "th04/main/tile/tile.hpp"
#include "th02/main/entity.hpp"

// See th04/main/tile/tile.hpp for why this declaration has to be repeated in
// every translation unit that calls the function. This one needs the two
// separate coordinates rather than the SPPoint form, because the laser half
// below passes computed values rather than a struct.
extern "C" void pascal near tiles_invalidate_around(
	subpixel_t center_y, subpixel_t center_x
);

#if (GAME == 5)
	#define SHOT_FLAG_FREE F_FREE
#else
	#define SHOT_FLAG_FREE SF_FREE
#endif

void near shots_invalidate(void)
{
	int i;
	Shot near *shot;
	#if (GAME == 5)
		HitShot near *hitshot;
	#endif

	tile_invalidate_box.x = SHOT_W;
	tile_invalidate_box.y = SHOT_H;
	shot = shots;
	#if (GAME == 5)
		hitshot = hitshots;
	#endif
	for(i = 0; i < SHOT_COUNT; (i++, shot++)) {
		if(shot->flag != SHOT_FLAG_FREE) {
			tiles_invalidate_around_xy(shot->pos.prev.x, shot->pos.prev.y);
		}
	}

	#if (GAME == 5)
		for(i = 0; i < HITSHOT_COUNT; (i++, hitshot++)) {
			if(hitshot->age != 0) {
				tiles_invalidate_around_xy(
					hitshot->pos.prev.x, hitshot->pos.prev.y
				);
			}
		}
	#else
		// The laser reaches from the player to the top of the playfield, so
		// its height in tiles is its own Y coordinate, and its center is at
		// half of that. The division by 16 is a real IDIV rather than a shift
		// because Turbo C++ 4.0J does not strength-reduce signed division by
		// anything but 2 - which is exactly why to_pixel_slow() exists.
		if(shot_laser_time >= SHOT_LASER_COOLDOWN_FRAMES) {
			tile_invalidate_box.x = SHOT_LASER_W;
			tile_invalidate_box.y =
				shot_laser_bottomcenter.prev.y.to_pixel_slow();
			tiles_invalidate_around_xy(
				(shot_laser_bottomcenter.prev.x.v -
					to_sp(PLAYER_OPTION_DISTANCE)),
				(shot_laser_bottomcenter.prev.y.v / 2)
			);
			tiles_invalidate_around_xy(
				(shot_laser_bottomcenter.prev.x.v +
					to_sp(PLAYER_OPTION_DISTANCE)),
				(shot_laser_bottomcenter.prev.y.v / 2)
			);
		}
	#endif
}
