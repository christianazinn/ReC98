/* ReC98
 * -----
 * TH02's per-frame player invalidation pass. ZUN's object placed it in the
 * same code segment as the point number code, which is why the two are
 * compiled into a single translation unit here - see th02/pointnum.cpp.
 */

#include "th02/hardware/pages.hpp"
#include "th02/main/player/player.hpp"
#include "th02/main/player/shot.hpp"
#include "th02/main/tile/tile.hpp"

void pascal near player_invalidate(void)
{
	// ZUN bloat: Assigned once, then never read. The value happens to equal
	// PAGE_COUNT, but nothing else in this function corroborates that
	// meaning, so it is spelled as the literal it is.
	int unused = 2;

	shots_invalidate();

	// Cache the back page's element of each page-indexed position array, so
	// that the rest of the frame can reach it through a single near pointer.
	player_left_on_back_page = &player_left_on_page[page_back];
	player_top_on_back_page = &player_top_on_page[page_back];
	player_option_left_left_on_back_page = (
		&player_option_left_topleft[page_back].x
	);
	player_option_left_top_on_back_page = (
		&player_option_left_topleft[page_back].y
	);

	tiles_invalidate_rect(
		*player_left_on_back_page,
		*player_top_on_back_page,
		PLAYER_W,
		PLAYER_H
	);

	register screen_x_t option_left_left = (
		*player_option_left_left_on_back_page
	);
	tiles_invalidate_rect(
		option_left_left,
		*player_option_left_top_on_back_page,
		PLAYER_OPTION_W,
		PLAYER_OPTION_H
	);
	tiles_invalidate_rect(
		(option_left_left + PLAYER_OPTION_TO_OPTION_DISTANCE),
		*player_option_left_top_on_back_page,
		PLAYER_OPTION_W,
		PLAYER_OPTION_H
	);

	// Carry the front page's position over to the back page, which the rest
	// of the frame is then free to move.
	*player_left_on_back_page = player_left_on_page[page_front];
	*player_top_on_back_page = player_top_on_page[page_front];
}
