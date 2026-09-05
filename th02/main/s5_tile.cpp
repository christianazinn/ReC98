// TH02 Stage 5 Mima logical tile-state exact-checkpoint ownership. This
// ungrouped patch segment only captures and validates typed state. It must not
// grow the original tile module or attempt a partial live restore.
#pragma option -zCT2S5TILE_TEXT -G-

#include "platform.h"
#include "pc98.h"
#include "th02/formats/map.hpp"
#include "th02/main/playfld.hpp"
#include "th02/main/s5_tile.hpp"
#include "th02/main/tile/tile.hpp"

extern pixel_delta_8_t tile_line_at_top;
extern tile_image_id_t tile_ring[TILES_Y][TILES_X];
extern bool tile_dirty[TILES_X][TILES_Y];
extern bool tile_column_dirty[TILES_X];
extern tile_mode_t tile_mode;
extern bool tiles_egc_render_all;
extern "C" vram_y_t stage4_tile_top;

#define T2S5TL_MODE_OFFSET 0
#define T2S5TL_RENDER_ALL_OFFSET 1
#define T2S5TL_LINE_AT_TOP_OFFSET 2
#define T2S5TL_MAP_ROW_OFFSET 3
#define T2S5TL_STAGE4_TILE_TOP_OFFSET 5
#define T2S5TL_RING_OFFSET 7
#define T2S5TL_RING_SIZE (TILES_X * TILES_Y)
#define T2S5TL_DIRTY_OFFSET (T2S5TL_RING_OFFSET + T2S5TL_RING_SIZE)
#define T2S5TL_DIRTY_SIZE ((TILES_X * TILES_Y) / 8)
#define T2S5TL_COLUMN_DIRTY_OFFSET (T2S5TL_DIRTY_OFFSET + T2S5TL_DIRTY_SIZE)
#define T2S5TL_COLUMN_DIRTY_SIZE (TILES_X / 8)
#define T2S5TL_MAP_ROW_MAX (MAP_LENGTH_MAX * MAP_ROWS_PER_SECTION)

typedef char th02_s5_tile_logic_wire_size_check[
	((T2S5TL_COLUMN_DIRTY_OFFSET + T2S5TL_COLUMN_DIRTY_SIZE) ==
	 TH02_S5_TILE_LOGIC_WIRE_SIZE) ? 1 : -1
];

static void near t2s5tile_wire_put_u16(
	uint8_t far *wire, unsigned offset, uint16_t value
)
{
	wire[offset] = static_cast<uint8_t>(value & 0xFF);
	wire[offset + 1] = static_cast<uint8_t>(value >> 8);
}

static uint16_t near t2s5tile_wire_get_u16(
	const uint8_t far *wire, unsigned offset
)
{
	return static_cast<uint16_t>(
		wire[offset] | (static_cast<uint16_t>(wire[offset + 1]) << 8)
	);
}

static bool16 near t2s5tile_live_state_valid(void)
{
	int tile_x;
	int tile_y;

	if(
		(tile_mode > TM_NONE) ||
		((tiles_egc_render_all != false) && (tiles_egc_render_all != true)) ||
		(tile_line_at_top < 0) || (tile_line_at_top >= TILE_H) ||
		(map_full_row_at_top_of_screen > T2S5TL_MAP_ROW_MAX) ||
		(stage4_tile_top >= RES_Y)
	) {
		return false;
	}
	for(tile_x = 0; tile_x < TILES_X; tile_x++) {
		if(
			(tile_column_dirty[tile_x] != false) &&
			(tile_column_dirty[tile_x] != true)
		) {
			return false;
		}
		for(tile_y = 0; tile_y < TILES_Y; tile_y++) {
			if(
				(tile_dirty[tile_x][tile_y] != false) &&
				(tile_dirty[tile_x][tile_y] != true)
			) {
				return false;
			}
			if(tile_ring[tile_y][tile_x] >= TILE_IMAGE_COUNT) {
				return false;
			}
		}
	}
	return true;
}

bool16 far th02_s5_tile_logic_wire_valid(
	const uint8_t far *wire, uint16_t wire_size
)
{
	int line_at_top;
	unsigned offset;

	if(
		(wire == 0) ||
		(wire_size != TH02_S5_TILE_LOGIC_WIRE_SIZE) ||
		(wire[T2S5TL_MODE_OFFSET] > TM_NONE) ||
		(wire[T2S5TL_RENDER_ALL_OFFSET] > 1) ||
		(t2s5tile_wire_get_u16(wire, T2S5TL_MAP_ROW_OFFSET) >
		 T2S5TL_MAP_ROW_MAX) ||
		(t2s5tile_wire_get_u16(wire, T2S5TL_STAGE4_TILE_TOP_OFFSET) >=
		 RES_Y)
	) {
		return false;
	}
	for(offset = T2S5TL_RING_OFFSET;
		offset < (T2S5TL_RING_OFFSET + T2S5TL_RING_SIZE); offset++) {
		if(wire[offset] >= TILE_IMAGE_COUNT) {
			return false;
		}
	}
	line_at_top = static_cast<int>(
		static_cast<int8_t>(wire[T2S5TL_LINE_AT_TOP_OFFSET])
	);
	return ((line_at_top >= 0) && (line_at_top < TILE_H));
}

bool16 far th02_s5_tile_logic_wire_capture(
	uint8_t far *wire, uint16_t wire_size
)
{
	unsigned offset;
	uint8_t bits;
	uint8_t bit;
	int tile_x;
	int tile_y;

	if(
		(wire == 0) ||
		(wire_size != TH02_S5_TILE_LOGIC_WIRE_SIZE) ||
		!t2s5tile_live_state_valid()
	) {
		return false;
	}
	wire[T2S5TL_MODE_OFFSET] = static_cast<uint8_t>(tile_mode);
	wire[T2S5TL_RENDER_ALL_OFFSET] = (tiles_egc_render_all ? 1 : 0);
	wire[T2S5TL_LINE_AT_TOP_OFFSET] = static_cast<uint8_t>(tile_line_at_top);
	t2s5tile_wire_put_u16(
		wire, T2S5TL_MAP_ROW_OFFSET, map_full_row_at_top_of_screen
	);
	t2s5tile_wire_put_u16(
		wire, T2S5TL_STAGE4_TILE_TOP_OFFSET, stage4_tile_top
	);
	offset = T2S5TL_RING_OFFSET;
	for(tile_y = 0; tile_y < TILES_Y; tile_y++) {
		for(tile_x = 0; tile_x < TILES_X; tile_x++) {
			wire[offset++] = tile_ring[tile_y][tile_x];
		}
	}
	offset = T2S5TL_DIRTY_OFFSET;
	bits = 0;
	bit = 0;
	for(tile_x = 0; tile_x < TILES_X; tile_x++) {
		for(tile_y = 0; tile_y < TILES_Y; tile_y++) {
			if(tile_dirty[tile_x][tile_y]) {
				bits |= static_cast<uint8_t>(1 << bit);
			}
			bit++;
			if(bit == 8) {
				wire[offset++] = bits;
				bits = 0;
				bit = 0;
			}
		}
	}
	offset = T2S5TL_COLUMN_DIRTY_OFFSET;
	bits = 0;
	bit = 0;
	for(tile_x = 0; tile_x < TILES_X; tile_x++) {
		if(tile_column_dirty[tile_x]) {
			bits |= static_cast<uint8_t>(1 << bit);
		}
		bit++;
		if(bit == 8) {
			wire[offset++] = bits;
			bits = 0;
			bit = 0;
		}
	}
	return (
		(offset == TH02_S5_TILE_LOGIC_WIRE_SIZE) &&
		th02_s5_tile_logic_wire_valid(wire, wire_size)
	);
}

#if T2REPLAY_EXACT_APPLY
bool16 far th02_s5_tile_logic_wire_prepare(
	th02_s5_tile_apply_plan_t *plan,
	const uint8_t far *wire, uint16_t wire_size
)
{
	if((plan == 0) || !th02_s5_tile_logic_wire_valid(wire, wire_size)) {
		return false;
	}
	plan->wire = wire;
	return true;
}

void far th02_s5_tile_logic_commit_prepared(
	const th02_s5_tile_apply_plan_t *plan
)
{
	const uint8_t far *wire = plan->wire;
	unsigned offset;
	uint8_t bits;
	uint8_t bit;
	int tile_x;
	int tile_y;

	tile_mode = static_cast<tile_mode_t>(wire[T2S5TL_MODE_OFFSET]);
	tiles_egc_render_all = (wire[T2S5TL_RENDER_ALL_OFFSET] != 0);
	tile_line_at_top = static_cast<int8_t>(wire[T2S5TL_LINE_AT_TOP_OFFSET]);
	map_full_row_at_top_of_screen = t2s5tile_wire_get_u16(
		wire, T2S5TL_MAP_ROW_OFFSET
	);
	stage4_tile_top = t2s5tile_wire_get_u16(
		wire, T2S5TL_STAGE4_TILE_TOP_OFFSET
	);
	offset = T2S5TL_RING_OFFSET;
	for(tile_y = 0; tile_y < TILES_Y; tile_y++) {
		for(tile_x = 0; tile_x < TILES_X; tile_x++) {
			tile_ring[tile_y][tile_x] = wire[offset++];
		}
	}
	offset = T2S5TL_DIRTY_OFFSET;
	bits = wire[offset++];
	bit = 0;
	for(tile_x = 0; tile_x < TILES_X; tile_x++) {
		for(tile_y = 0; tile_y < TILES_Y; tile_y++) {
			tile_dirty[tile_x][tile_y] = ((bits & (1 << bit)) != 0);
			bit++;
			if(bit == 8) {
				bit = 0;
				if((tile_x != (TILES_X - 1)) || (tile_y != (TILES_Y - 1))) {
					bits = wire[offset++];
				}
			}
		}
	}
	offset = T2S5TL_COLUMN_DIRTY_OFFSET;
	bits = wire[offset++];
	bit = 0;
	for(tile_x = 0; tile_x < TILES_X; tile_x++) {
		tile_column_dirty[tile_x] = ((bits & (1 << bit)) != 0);
		bit++;
		if((bit == 8) && (tile_x != (TILES_X - 1))) {
			bit = 0;
			bits = wire[offset++];
		}
	}
}
#endif
