// TH02 clean Practice construction. This patch-only segment deliberately
// follows every native and replay segment so its code cannot move either.
#pragma option -zCT2PRACT_TEXT -G-

#include "platform.h"
#include "pc98.h"
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "platform/x86real/pc98/page.hpp"
#include "th02/formats/map.hpp"
#include "th02/hardware/pages.hpp"
#include "th02/main/frames.hpp"
#include "th02/main/playfld.hpp"
#include "th02/main/practice.hpp"
#include "th02/main/replay.hpp"
#include "th02/main/scroll.hpp"
#include "th02/main/enemy/enemy.hpp"
#include "th02/main/stage/stage.hpp"
#include "th02/main/tile/tile.hpp"

extern "C" int spawn_row_cur;

extern pixel_delta_8_t tile_line_at_top;
extern tile_image_id_t tiles_for_new_row[TILES_X];
extern tile_image_id_t tile_ring[TILES_Y][TILES_X];
extern bool tile_dirty[TILES_X][TILES_Y];
extern bool tile_column_dirty[TILES_X];

static bool16 near practice_map_target_validate(
	int target_scroll_step, int *top_map_row
)
{
	int first_map_row;
	int map_rows;
	int max_scroll_step;
	int row;
	int section;

	if(
		(top_map_row == NULL) ||
		(map_length <= 0) ||
		(map_length > MAP_LENGTH_MAX)
	) {
		return false;
	}

	map_rows = (map_length * MAP_ROWS_PER_SECTION);
	max_scroll_step = (
		(map_rows - ((PLAYFIELD_H / TILE_H) + 1)) * 2
	);
	if((target_scroll_step <= 0) || (target_scroll_step > max_scroll_step)) {
		return false;
	}

	*top_map_row = (
		(PLAYFIELD_H / TILE_H) + ((target_scroll_step + 1) / 2)
	);
	first_map_row = (*top_map_row - (TILES_Y - 1));
	if((first_map_row < 0) || (*top_map_row >= map_rows)) {
		return false;
	}

	// Validate every section ID before the caller mutates any stage state.
	for(row = first_map_row; row <= *top_map_row; row++) {
		section = (row / MAP_ROWS_PER_SECTION);
		if(
			(section >= map_length) ||
			(map[section] >= MAP_SECTION_COUNT)
		) {
			return false;
		}
	}
	return true;
}

static tile_image_id_t near practice_map_tile_at(int row, int x)
{
	int section = (row / MAP_ROWS_PER_SECTION);
	return map_section_tiles[map[section]].row[
		(row % MAP_ROWS_PER_SECTION)
	][x];
}

bool16 far practice_spawn_row_upper_bound(
	int target_scroll_step, int *spawn_row
)
{
	int i;
	int first_after_target;
	int previous_trigger;
	int trigger;

	if(
		(spawn_row == NULL) ||
		(target_scroll_step < 0) ||
		(spawn_rows < 0)
	) {
		return false;
	}
	if(spawn_rows == 0) {
		*spawn_row = 0;
		return true;
	}
	if(spawn_grid[0] == NULL) {
		return false;
	}

	previous_trigger = spawn_grid[0][0];
	if(previous_trigger < 0) {
		return false;
	}
	first_after_target = 0;
	if(previous_trigger > target_scroll_step) {
		*spawn_row = first_after_target;
		return true;
	}
	for(i = 1; i < spawn_rows; i++) {
		trigger = spawn_grid[0][i];
		if((trigger < 0) || (trigger < previous_trigger)) {
			return false;
		}
		if(trigger > target_scroll_step) {
			first_after_target = i;
			*spawn_row = first_after_target;
			return true;
		}
		previous_trigger = trigger;
	}

	*spawn_row = spawn_rows;
	return true;
}

bool16 far practice_chapter_field_build(int target_scroll_step)
{
	int top_map_row;
	int top_ring_y;
	int ring_y;
	int map_row;
	int row_offset;
	int tile_x;
	int tile_y;
	int derived_spawn_row;
	int scrolled_lines;
	vram_y_t target_scroll_line;

	if(
		!practice_map_target_validate(target_scroll_step, &top_map_row) ||
		!practice_spawn_row_upper_bound(
			target_scroll_step, &derived_spawn_row
		)
	) {
		return false;
	}

	scrolled_lines = ((target_scroll_step % (RES_Y / 8)) * 8);
	target_scroll_line = (
		scrolled_lines ? (RES_Y - scrolled_lines) : 0
	);
	top_ring_y = (target_scroll_line >> TILE_BITS_H);

	for(row_offset = 0; row_offset < TILES_Y; row_offset++) {
		ring_y = (top_ring_y + row_offset);
		if(ring_y >= TILES_Y) {
			ring_y -= TILES_Y;
		}
		map_row = (top_map_row - row_offset);
		for(tile_x = 0; tile_x < TILES_X; tile_x++) {
			tile_ring[ring_y][tile_x] = practice_map_tile_at(
				map_row, tile_x
			);
		}
	}
	for(tile_x = 0; tile_x < TILES_X; tile_x++) {
		tiles_for_new_row[tile_x] = practice_map_tile_at(
			top_map_row, tile_x
		);
		tile_column_dirty[tile_x] = false;
		for(tile_y = 0; tile_y < TILES_Y; tile_y++) {
			tile_dirty[tile_x][tile_y] = false;
		}
	}

	stage_progression = SP_STAGE;
	stage_frame = 0;
	scroll_step = target_scroll_step;
	scroll_step_advanced = false;
	scroll_done = false;
	scroll_cycle = 0;
	scroll_delta = 0;
	scroll_line = target_scroll_line;
	scroll_sad = (target_scroll_line * (RES_X / 16));
	map_full_row_at_top_of_screen = top_map_row;
	tile_line_at_top = (
		(target_scroll_step & 1) ? (TILE_H / 2) : 0
	);
	spawn_row_cur = derived_spawn_row;
	tile_mode = TM_TILES;
	tiles_egc_render_all = false;
	page_front = 1;
	page_back = 0;
	replay_scroll_page_line_set(0, target_scroll_line);
	replay_scroll_page_line_set(1, target_scroll_line);

	graph_accesspage(page_back);
	tiles_render_all();
	graph_accesspage(page_front);
	tiles_render_all();
	graph_scrollup(target_scroll_line);
	page_show(page_front);
	graph_accesspage(page_back);
	return true;
}

bool16 far practice_terminal_field_build(void)
{
	int map_rows;
	int target_scroll_step;

	if((map_length <= 0) || (map_length > MAP_LENGTH_MAX)) {
		return false;
	}
	map_rows = (map_length * MAP_ROWS_PER_SECTION);
	target_scroll_step = (
		(map_rows - ((PLAYFIELD_H / TILE_H) + 1)) * 2
	);
	if(!practice_chapter_field_build(target_scroll_step)) {
		return false;
	}
	scroll_done = true;
	scroll_delta = 0;
	scroll_step_advanced = false;
	return true;
}
