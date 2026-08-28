// TH02 Stage 3 North Stone Phase 4 direct-Practice constructor. This final
// patch-only tail leaves native and prior replay contributions byte-stable.
#pragma option -zCT2S3NORTH_TEXT -G-

#include "platform.h"
#include "pc98.h"
#include "libs/master.lib/pc98_gfx.hpp"
#include "platform/x86real/pc98/page.hpp"
#include "th02/core/globals.hpp"
#include "th02/hardware/pages.hpp"
#include "th02/main/boss/boss.hpp"
#include "th02/main/playfld.hpp"
#include "th02/main/scroll.hpp"
#include "th02/main/s3_actor.hpp"
#include "th02/main/s3_north.hpp"
#include "th02/main/stage/stage.hpp"
#include "th02/main/tile/tile.hpp"

extern "C" screen_x_t stone_left[STONE_COUNT];
extern "C" screen_y_t stone_top[STONE_COUNT];
extern "C" uint8_t stone_hit_flash[STONE_COUNT];
extern "C" int16_t stone_kill_frame[STONE_COUNT];
extern "C" int16_t stone_patnum[STONE_COUNT];
extern "C" uint8_t stones_phase;
extern "C" uint8_t stones_pattern;
extern "C" int32_t stones_timeout_frame;
extern "C" int16_t stones_phase_frame_unused;
extern "C" uint8_t stones_tile_pending[TILES_X];
extern "C" uint8_t stones_tile_pass;
extern "C" uint8_t stones_tile_pair;
extern "C" uint8_t stones_tile_cols_done;
extern "C" int tile_image_22FD2;
extern "C" int tile_image_22FD4;
extern "C" int tile_image_22FD6;
extern "C" vram_y_t y_22D9C;
extern "C" int boss_pos_x;
extern "C" int boss_pos_y;

static void near th02_s3_north_takeover_row_put_both(void)
{
	int i;
	screen_x_t left = PLAYFIELD_LEFT;

	for(i = 0; i < TILES_X; i++) {
		tile_ring_set_and_put_both_8(left, y_22D9C, 43);
		left += TILE_W;
	}
}

static void near th02_s3_north_put_both(void)
{
	int page;
	vram_y_t top = (stone_top[STONE_NORTH] + scroll_line);

	if(top >= RES_Y) {
		top -= RES_Y;
	}
	for(page = 0; page < PAGE_COUNT; page++) {
		graph_accesspage(page);
		super_roll_put(
			stone_left[STONE_NORTH], top, stone_patnum[STONE_NORTH]
		);
	}
	page_front = 1;
	page_back = 0;
	graph_accesspage(page_back);
}

bool16 far th02_s3_stones_north_phase4_clean_init(void)
{
	int i;

	if(stage_id != 2) {
		return false;
	}

	th02_s3_stones_clean_base_init();
	for(i = STONE_INNER_WEST; i < STONE_NORTH; i++) {
		stone_flag[i] = SF_REMOVED;
		stone_damage[i] = 0;
		stone_hit_flash[i] = 0;
		stone_kill_frame[i] = 0;
	}
	stone_patnum[STONE_INNER_WEST] = 155;
	stone_patnum[STONE_INNER_EAST] = 155;
	stone_patnum[STONE_OUTER_WEST] = 159;
	stone_patnum[STONE_OUTER_EAST] = 159;
	stone_flag[STONE_NORTH] = SF_ACTIVE;
	stone_damage[STONE_NORTH] = 0;
	stone_patnum[STONE_NORTH] = 163;
	stone_hit_flash[STONE_NORTH] = 0;
	stone_kill_frame[STONE_NORTH] = 0;
	stones_phase = 4;
	stones_pattern = 0;
	boss_phase_frame = 0;
	stones_timeout_frame = 0;
	stones_phase_frame_unused = 0;
	for(i = 0; i < TILES_X; i++) {
		stones_tile_pending[i] = 0;
	}
	stones_tile_pass = 3;
	stones_tile_pair = 0;
	stones_tile_cols_done = TILES_X;
	tile_image_22FD2 = 41;
	tile_image_22FD4 = 42;
	tile_image_22FD6 = 43;
	boss_damage = 0;
	boss_pos_x = (stone_left[STONE_NORTH] + 8);
	boss_pos_y = (stone_top[STONE_NORTH] + 8);

	th02_s3_north_takeover_row_put_both();
	th02_s3_north_put_both();
	return true;
}
