// TH02 Stage 3 actor checkpoint and clean-Practice ownership. This patch-only
// segment follows the earlier patch tails and leaves every native contribution
// byte-stable.
#pragma option -zCT2S3ACT_TEXT -G-

#include "platform.h"
#include "pc98.h"
#include "th01/rank.h"
#include "th02/core/globals.hpp"
#include "th02/hardware/pages.hpp"
#include "th02/main/playfld.hpp"
#include "th02/main/scroll.hpp"
#include "th02/main/s3_actor.hpp"
#include "th02/main/boss/boss.hpp"
#include "th02/main/bullet/bullet.hpp"
#include "th02/main/explode.hpp"
#include "th02/main/laser.hpp"
#include "th02/sprites/main_pat.h"
#include "th01/sprites/pellet.h"

extern "C" int boss_pos_x;
extern "C" int boss_pos_y;
extern "C" uint8_t boss_rank_param[5];

extern "C" int16_t midboss3_kill_frame[MIDBOSS3_COUNT];
extern "C" screen_x_t midboss3_left_on_page[STONE_COUNT][PAGE_COUNT];
extern "C" screen_y_t midboss3_top_on_page[STONE_COUNT][PAGE_COUNT];
extern "C" screen_x_t *midboss3_left_on_back_page[STONE_COUNT];
extern "C" screen_y_t *midboss3_top_on_back_page[STONE_COUNT];
extern "C" uint8_t midboss3_angle[MIDBOSS3_COUNT];
extern "C" uint8_t midboss3_loops;

extern "C" uint16_t stage3_effect_frame;
extern "C" int16_t stage3_ring_radius;
extern "C" uint8_t stage3_ring_angle;
extern "C" screen_x_t stage3_ring_center_x;
extern "C" screen_y_t stage3_ring_center_y;
extern "C" screen_point_t stage3_ring_dot[PAGE_COUNT][TH02_S3_RING_DOTS];

extern "C" screen_x_t stone_left[STONE_COUNT];
extern "C" screen_y_t stone_top[STONE_COUNT];
extern "C" uint8_t stone_hit_flash[STONE_COUNT];
extern "C" int16_t stone_kill_frame[STONE_COUNT];
extern "C" int16_t stone_patnum[STONE_COUNT];
extern "C" uint8_t angle_22FAF;
extern "C" uint8_t angle_22FB0;
extern "C" uint8_t angle_1EB28;
extern "C" uint8_t angle_22FB1[STONE_COUNT];
extern "C" screen_x_t stones_laser_left;
extern "C" uint8_t stones_laser_return_delay;
extern "C" uint8_t stones_tile_pending[TILES_X];
extern "C" uint8_t stones_tile_pass;
extern "C" uint8_t stones_tile_pair;
extern "C" uint8_t stones_tile_cols_done;
extern "C" int tile_image_22FD2;
extern "C" int tile_image_22FD4;
extern "C" int tile_image_22FD6;
extern "C" uint8_t stones_phase;
extern "C" uint8_t stones_pattern;
extern "C" int32_t stones_timeout_frame;
extern "C" int16_t stones_phase_frame_unused;
extern "C" screen_x_t left_22D98;
extern "C" screen_y_t top_22D9A;
extern "C" vram_y_t y_22D9C;

static void near th02_s3_rank_params_set(void);

static bool16 near th02_s3_midboss_native_valid(void)
{
	int i;

	if(
		(midboss3_loops > 4) ||
		!(((stone_patnum[0] >= 10) && (stone_patnum[0] <= 17)) ||
		  ((stone_patnum[0] >= 144) && (stone_patnum[0] <= 147)))
	) {
		return false;
	}
	for(i = 0; i < MIDBOSS3_COUNT; i++) {
		if(
			(midboss3_flag[i] > M3F_REMOVED) ||
			(midboss3_damage[i] < 0) ||
			(midboss3_kill_frame[i] < 0) ||
			(midboss3_kill_frame[i] >= 48) ||
			(midboss3_left_on_page[i][0] < 0) ||
			(midboss3_left_on_page[i][0] > (RES_X - 64)) ||
			(midboss3_left_on_page[i][1] < 0) ||
			(midboss3_left_on_page[i][1] > (RES_X - 64)) ||
			(midboss3_top_on_page[i][0] < -32) ||
			(midboss3_top_on_page[i][0] >= RES_Y) ||
			(midboss3_top_on_page[i][1] < -32) ||
			(midboss3_top_on_page[i][1] >= RES_Y)
		) {
			return false;
		}
	}
	return true;
}

static bool16 near th02_s3_midboss_state_validate(
	const th02_s3_midboss_state_t *state
)
{
	int i;
	int page;

	if(
		(state == 0) ||
		(state->loops > 4) ||
		!(((state->patnum >= 10) && (state->patnum <= 17)) ||
		  ((state->patnum >= 144) && (state->patnum <= 147)))
	) {
		return false;
	}
	for(i = 0; i < MIDBOSS3_COUNT; i++) {
		if(
			(state->flag[i] > M3F_REMOVED) ||
			(state->damage[i] < 0) ||
			(state->kill_frame[i] < 0) ||
			(state->kill_frame[i] >= 48)
		) {
			return false;
		}
		for(page = 0; page < PAGE_COUNT; page++) {
			if(
				(state->left_on_page[i][page] < 0) ||
				(state->left_on_page[i][page] > (RES_X - 64)) ||
				(state->top_on_page[i][page] < -32) ||
				(state->top_on_page[i][page] >= RES_Y)
			) {
				return false;
			}
		}
	}
	return true;
}

bool16 far th02_s3_midboss_state_capture(th02_s3_midboss_state_t *state)
{
	int i;
	int page;

	if((state == 0) || !th02_s3_midboss_native_valid()) {
		return false;
	}
	for(i = 0; i < MIDBOSS3_COUNT; i++) {
		state->flag[i] = static_cast<uint8_t>(midboss3_flag[i]);
		state->damage[i] = midboss3_damage[i];
		state->kill_frame[i] = midboss3_kill_frame[i];
		for(page = 0; page < PAGE_COUNT; page++) {
			state->left_on_page[i][page] = midboss3_left_on_page[i][page];
			state->top_on_page[i][page] = midboss3_top_on_page[i][page];
		}
		state->angle[i] = midboss3_angle[i];
	}
	state->loops = midboss3_loops;
	state->patnum = stone_patnum[0];
	return true;
}

bool16 far th02_s3_midboss_state_apply(
	const th02_s3_midboss_state_t *state
)
{
	int i;
	int page;

	if(!th02_s3_midboss_state_validate(state)) {
		return false;
	}
	for(i = 0; i < MIDBOSS3_COUNT; i++) {
		midboss3_flag[i] = static_cast<midboss3_flag_t>(state->flag[i]);
		midboss3_damage[i] = state->damage[i];
		midboss3_kill_frame[i] = state->kill_frame[i];
		for(page = 0; page < PAGE_COUNT; page++) {
			midboss3_left_on_page[i][page] = state->left_on_page[i][page];
			midboss3_top_on_page[i][page] = state->top_on_page[i][page];
		}
		midboss3_left_on_back_page[i] =
			&midboss3_left_on_page[i][page_back];
		midboss3_top_on_back_page[i] =
			&midboss3_top_on_page[i][page_back];
		midboss3_angle[i] = state->angle[i];
	}
	midboss3_loops = state->loops;
	stone_patnum[0] = state->patnum;
	return true;
}

static bool16 near th02_s3_field_state_validate(
	const th02_s3_field_state_t *state
)
{
	int i;
	int page;

	if(
		(state == 0) ||
		(state->effect_frame >= 1000) ||
		(state->ring_radius < 0) ||
		(state->ring_radius >= 200) ||
		(state->ring_center_x < 0) ||
		(state->ring_center_x >= RES_X) ||
		(state->ring_center_y < 0) ||
		(state->ring_center_y >= RES_Y)
	) {
		return false;
	}
	for(page = 0; page < PAGE_COUNT; page++) {
		for(i = 0; i < TH02_S3_RING_DOTS; i++) {
			if(
				(state->ring_dot[page][i].x < -256) ||
				(state->ring_dot[page][i].x >= (RES_X + 256)) ||
				(state->ring_dot[page][i].y < -256) ||
				(state->ring_dot[page][i].y >= (RES_Y + 256))
			) {
				return false;
			}
		}
	}
	return true;
}

static bool16 near th02_s3_field_native_valid(void)
{
	int i;
	int page;

	if(
		(stage3_effect_frame >= 1000) ||
		(stage3_ring_radius < 0) ||
		(stage3_ring_radius >= 200) ||
		(stage3_ring_center_x < 0) ||
		(stage3_ring_center_x >= RES_X) ||
		(stage3_ring_center_y < 0) ||
		(stage3_ring_center_y >= RES_Y)
	) {
		return false;
	}
	for(page = 0; page < PAGE_COUNT; page++) {
		for(i = 0; i < TH02_S3_RING_DOTS; i++) {
			if(
				(stage3_ring_dot[page][i].x < -256) ||
				(stage3_ring_dot[page][i].x >= (RES_X + 256)) ||
				(stage3_ring_dot[page][i].y < -256) ||
				(stage3_ring_dot[page][i].y >= (RES_Y + 256))
			) {
				return false;
			}
		}
	}
	return true;
}

bool16 far th02_s3_field_state_capture(th02_s3_field_state_t *state)
{
	int i;
	int page;

	if((state == 0) || !th02_s3_field_native_valid()) {
		return false;
	}
	state->effect_frame = stage3_effect_frame;
	state->ring_radius = stage3_ring_radius;
	state->ring_angle = stage3_ring_angle;
	state->ring_center_x = stage3_ring_center_x;
	state->ring_center_y = stage3_ring_center_y;
	for(page = 0; page < PAGE_COUNT; page++) {
		for(i = 0; i < TH02_S3_RING_DOTS; i++) {
			state->ring_dot[page][i] = stage3_ring_dot[page][i];
		}
	}
	return true;
}

bool16 far th02_s3_field_state_apply(const th02_s3_field_state_t *state)
{
	int i;
	int page;

	if(!th02_s3_field_state_validate(state)) {
		return false;
	}
	stage3_effect_frame = state->effect_frame;
	stage3_ring_radius = state->ring_radius;
	stage3_ring_angle = state->ring_angle;
	stage3_ring_center_x = state->ring_center_x;
	stage3_ring_center_y = state->ring_center_y;
	for(page = 0; page < PAGE_COUNT; page++) {
		for(i = 0; i < TH02_S3_RING_DOTS; i++) {
			stage3_ring_dot[page][i] = state->ring_dot[page][i];
		}
	}
	return true;
}

static bool16 near th02_s3_stones_phase_valid(
	const th02_s3_stones_state_t *state
)
{
	int i;

	if(state->phase == 0) {
		for(i = 0; i < STONE_COUNT; i++) {
			if(state->flag[i] != SF_DORMANT) {
				return false;
			}
		}
		return true;
	}
	if(state->phase == 1) {
		return (
			(state->flag[STONE_OUTER_WEST] == SF_DORMANT) &&
			(state->flag[STONE_OUTER_EAST] == SF_DORMANT) &&
			(state->flag[STONE_NORTH] == SF_DORMANT)
		);
	}
	if(state->phase == 2) {
		return (
			(state->flag[STONE_INNER_WEST] == SF_REMOVED) &&
			(state->flag[STONE_INNER_EAST] == SF_REMOVED) &&
			(state->flag[STONE_NORTH] == SF_DORMANT)
		);
	}
	if(
		(state->flag[STONE_INNER_WEST] != SF_REMOVED) ||
		(state->flag[STONE_INNER_EAST] != SF_REMOVED) ||
		(state->flag[STONE_OUTER_WEST] != SF_REMOVED) ||
		(state->flag[STONE_OUTER_EAST] != SF_REMOVED)
	) {
		return false;
	}
	if((state->phase == 3) || (state->phase == 6) || (state->phase == 7)) {
		return (state->flag[STONE_NORTH] == SF_DORMANT);
	}
	if((state->phase == 4) || (state->phase == 8)) {
		return (
			(state->flag[STONE_NORTH] == SF_ACTIVE) ||
			(state->flag[STONE_NORTH] == SF_KILL_ANIM) ||
			(state->flag[STONE_NORTH] == SF_REMOVE)
		);
	}
	// Phase 5 transitions the north stone from active to dormant.
	return (
		(state->flag[STONE_NORTH] == SF_ACTIVE) ||
		(state->flag[STONE_NORTH] == SF_DORMANT)
	);
}

static bool16 near th02_s3_stones_state_validate(
	const th02_s3_stones_state_t *state
)
{
	int i;

	if(
		(state == 0) ||
		(state->phase > 8) ||
		(state->pattern > 5) ||
		(state->timeout_frame < 0) ||
		(state->phase_frame_unused < 0) ||
		(state->laser_left < 0) ||
		(state->laser_left >= RES_X) ||
		(state->tile_pass > 3) ||
		(state->tile_pair > (TILES_X / 2)) ||
		(state->tile_cols_done > TILES_X) ||
		(state->muzzle_left < 0) ||
		(state->muzzle_left >= RES_X) ||
		(state->muzzle_top < 0) ||
		(state->muzzle_top >= RES_Y) ||
		(state->mutation_y < 0) ||
		(state->mutation_y >= RES_Y)
	) {
		return false;
	}
	for(i = 0; i < STONE_COUNT; i++) {
		if(
			(state->flag[i] > SF_REMOVED) ||
			(state->damage[i] < 0) ||
			(state->patnum[i] < 10) ||
			(state->patnum[i] > 163) ||
			(state->hit_flash[i] > 1) ||
			(state->kill_frame[i] < 0) ||
			(state->kill_frame[i] >= 96) ||
			(state->left[i] < 0) ||
			(state->left[i] > (RES_X - 32)) ||
			(state->top[i] < 0) ||
			(state->top[i] >= RES_Y)
		) {
			return false;
		}
	}
	if(!th02_s3_stones_phase_valid(state)) {
		return false;
	}
	for(i = 0; i < TILES_X; i++) {
		if(state->tile_pending[i] > 3) {
			return false;
		}
	}
	for(i = 0; i < 3; i++) {
		if((state->tile_image[i] < 0) || (state->tile_image[i] >= 64)) {
			return false;
		}
	}
	return true;
}

static bool16 near th02_s3_stones_native_valid(void)
{
	int i;

	if(
		(stones_phase > 8) ||
		(stones_pattern > 5) ||
		(stones_timeout_frame < 0) ||
		(stones_phase_frame_unused < 0) ||
		(stones_laser_left < 0) ||
		(stones_laser_left >= RES_X) ||
		(stones_tile_pass > 3) ||
		(stones_tile_pair > (TILES_X / 2)) ||
		(stones_tile_cols_done > TILES_X) ||
		(left_22D98 < 0) ||
		(left_22D98 >= RES_X) ||
		(top_22D9A < 0) ||
		(top_22D9A >= RES_Y) ||
		(y_22D9C >= RES_Y)
	) {
		return false;
	}
	for(i = 0; i < STONE_COUNT; i++) {
		if(
			(stone_flag[i] > SF_REMOVED) ||
			(stone_damage[i] < 0) ||
			(stone_patnum[i] < 10) ||
			(stone_patnum[i] > 163) ||
			(stone_hit_flash[i] > 1) ||
			(stone_kill_frame[i] < 0) ||
			(stone_kill_frame[i] >= 96) ||
			(stone_left[i] < 0) ||
			(stone_left[i] > (RES_X - 32)) ||
			(stone_top[i] < 0) ||
			(stone_top[i] >= RES_Y)
		) {
			return false;
		}
	}
	for(i = 0; i < TILES_X; i++) {
		if(stones_tile_pending[i] > 3) {
			return false;
		}
	}
	if(
		(tile_image_22FD2 < 0) || (tile_image_22FD2 >= 64) ||
		(tile_image_22FD4 < 0) || (tile_image_22FD4 >= 64) ||
		(tile_image_22FD6 < 0) || (tile_image_22FD6 >= 64)
	) {
		return false;
	}
	if(stones_phase == 0) {
		for(i = 0; i < STONE_COUNT; i++) {
			if(stone_flag[i] != SF_DORMANT) {
				return false;
			}
		}
		return true;
	}
	if(stones_phase == 1) {
		return (
			(stone_flag[STONE_OUTER_WEST] == SF_DORMANT) &&
			(stone_flag[STONE_OUTER_EAST] == SF_DORMANT) &&
			(stone_flag[STONE_NORTH] == SF_DORMANT)
		);
	}
	if(stones_phase == 2) {
		return (
			(stone_flag[STONE_INNER_WEST] == SF_REMOVED) &&
			(stone_flag[STONE_INNER_EAST] == SF_REMOVED) &&
			(stone_flag[STONE_NORTH] == SF_DORMANT)
		);
	}
	if(
		(stone_flag[STONE_INNER_WEST] != SF_REMOVED) ||
		(stone_flag[STONE_INNER_EAST] != SF_REMOVED) ||
		(stone_flag[STONE_OUTER_WEST] != SF_REMOVED) ||
		(stone_flag[STONE_OUTER_EAST] != SF_REMOVED)
	) {
		return false;
	}
	if((stones_phase == 3) || (stones_phase == 6) || (stones_phase == 7)) {
		return (stone_flag[STONE_NORTH] == SF_DORMANT);
	}
	if((stones_phase == 4) || (stones_phase == 8)) {
		return (
			(stone_flag[STONE_NORTH] == SF_ACTIVE) ||
			(stone_flag[STONE_NORTH] == SF_KILL_ANIM) ||
			(stone_flag[STONE_NORTH] == SF_REMOVE)
		);
	}
	return (
		(stone_flag[STONE_NORTH] == SF_ACTIVE) ||
		(stone_flag[STONE_NORTH] == SF_DORMANT)
	);
}

bool16 far th02_s3_stones_state_capture(th02_s3_stones_state_t *state)
{
	int i;

	if((state == 0) || !th02_s3_stones_native_valid()) {
		return false;
	}
	for(i = 0; i < STONE_COUNT; i++) {
		state->flag[i] = static_cast<uint8_t>(stone_flag[i]);
		state->damage[i] = stone_damage[i];
		state->patnum[i] = stone_patnum[i];
		state->hit_flash[i] = stone_hit_flash[i];
		state->kill_frame[i] = stone_kill_frame[i];
		state->left[i] = stone_left[i];
		state->top[i] = stone_top[i];
		state->aimed_angle[i] = angle_22FB1[i];
	}
	state->phase = stones_phase;
	state->pattern = stones_pattern;
	state->timeout_frame = stones_timeout_frame;
	state->phase_frame_unused = stones_phase_frame_unused;
	state->angle_phase6_a = angle_22FAF;
	state->angle_phase6_b = angle_22FB0;
	state->angle_sweep = angle_1EB28;
	state->laser_left = stones_laser_left;
	state->laser_return_delay = stones_laser_return_delay;
	for(i = 0; i < TILES_X; i++) {
		state->tile_pending[i] = stones_tile_pending[i];
	}
	state->tile_pass = stones_tile_pass;
	state->tile_pair = stones_tile_pair;
	state->tile_cols_done = stones_tile_cols_done;
	state->tile_image[0] = tile_image_22FD2;
	state->tile_image[1] = tile_image_22FD4;
	state->tile_image[2] = tile_image_22FD6;
	state->muzzle_left = left_22D98;
	state->muzzle_top = top_22D9A;
	state->mutation_y = y_22D9C;
	return true;
}

bool16 far th02_s3_stones_state_apply(const th02_s3_stones_state_t *state)
{
	int i;

	if(!th02_s3_stones_state_validate(state)) {
		return false;
	}
	for(i = 0; i < STONE_COUNT; i++) {
		stone_flag[i] = static_cast<stone_flag_t>(state->flag[i]);
		stone_damage[i] = state->damage[i];
		stone_patnum[i] = state->patnum[i];
		stone_hit_flash[i] = state->hit_flash[i];
		stone_kill_frame[i] = state->kill_frame[i];
		stone_left[i] = state->left[i];
		stone_top[i] = state->top[i];
		angle_22FB1[i] = state->aimed_angle[i];
	}
	stones_phase = state->phase;
	stones_pattern = state->pattern;
	stones_timeout_frame = state->timeout_frame;
	stones_phase_frame_unused = state->phase_frame_unused;
	angle_22FAF = state->angle_phase6_a;
	angle_22FB0 = state->angle_phase6_b;
	angle_1EB28 = state->angle_sweep;
	stones_laser_left = state->laser_left;
	stones_laser_return_delay = state->laser_return_delay;
	for(i = 0; i < TILES_X; i++) {
		stones_tile_pending[i] = state->tile_pending[i];
	}
	stones_tile_pass = state->tile_pass;
	stones_tile_pair = state->tile_pair;
	stones_tile_cols_done = state->tile_cols_done;
	tile_image_22FD2 = state->tile_image[0];
	tile_image_22FD4 = state->tile_image[1];
	tile_image_22FD6 = state->tile_image[2];
	left_22D98 = state->muzzle_left;
	top_22D9A = state->muzzle_top;
	y_22D9C = state->mutation_y;
	th02_s3_rank_params_set();
	return true;
}

static void near th02_s3_rank_params_set(void)
{
	if(rank != RANK_EASY) {
		boss_rank_param[0] = BG_5_SPREAD_WIDE_AIMED;
		boss_rank_param[1] = 0;
		boss_rank_param[2] = BG_8_RING;
		boss_rank_param[3] = BG_4_RING;
	} else {
		boss_rank_param[0] = BG_3_SPREAD_WIDE_AIMED;
		boss_rank_param[1] = 2;
		boss_rank_param[2] = BG_4_RING;
		boss_rank_param[3] = BG_2_RING;
	}
}

void far th02_s3_field_clean_init(void)
{
	int page;
	int i;

	// The next native update enters the stable repeating ring cycle at 200.
	stage3_effect_frame = 199;
	stage3_ring_radius = 0;
	stage3_ring_angle = 0;
	stage3_ring_center_x = 224;
	stage3_ring_center_y = 200;
	for(page = 0; page < PAGE_COUNT; page++) {
		for(i = 0; i < TH02_S3_RING_DOTS; i++) {
			stage3_ring_dot[page][i].x = stage3_ring_center_x;
			stage3_ring_dot[page][i].y = stage3_ring_center_y;
		}
	}
}

void far th02_s3_midboss_clean_init(void)
{
	int i;

	for(i = 0; i < MIDBOSS3_COUNT; i++) {
		midboss3_flag[i] = M3F_ALIVE;
		midboss3_damage[i] = 0;
		midboss3_kill_frame[i] = 0;
		midboss3_angle[i] = 0;
		midboss3_top_on_page[i][0] = -16;
		midboss3_top_on_page[i][1] = -16;
	}
	midboss3_left_on_page[0][0] = PLAYFIELD_LEFT;
	midboss3_left_on_page[0][1] = PLAYFIELD_LEFT;
	midboss3_left_on_page[1][0] = (PLAYFIELD_RIGHT - 64);
	midboss3_left_on_page[1][1] = (PLAYFIELD_RIGHT - 64);
	for(i = 0; i < MIDBOSS3_COUNT; i++) {
		midboss3_left_on_back_page[i] =
			&midboss3_left_on_page[i][page_back];
		midboss3_top_on_back_page[i] =
			&midboss3_top_on_page[i][page_back];
	}
	midboss3_loops = 0;
	stone_patnum[0] = (144 + (scroll_line & 3));
	boss_phase_frame = 1;
	boss_damage = 0;
	boss_pos_x = (PLAYFIELD_LEFT + 24);
	boss_pos_y = 0;
}

void far th02_s3_stones_clean_init(void)
{
	int i;

	boss_phase_frame = 0;
	for(i = 0; i < STONE_COUNT; i++) {
		stone_flag[i] = SF_DORMANT;
		stone_damage[i] = 0;
		stone_patnum[i] = 148;
		stone_hit_flash[i] = 0;
		stone_kill_frame[i] = 0;
		angle_22FB1[i] = 0;
	}
	stone_left[STONE_INNER_WEST] = (PLAYFIELD_LEFT + 16 + (1 * 80));
	stone_top[STONE_INNER_WEST] = (PLAYFIELD_TOP + 16);
	stone_left[STONE_INNER_EAST] = (PLAYFIELD_LEFT + 16 + (3 * 80));
	stone_top[STONE_INNER_EAST] = (PLAYFIELD_TOP + 16);
	stone_left[STONE_OUTER_WEST] = (PLAYFIELD_LEFT + 16 + (0 * 80));
	stone_top[STONE_OUTER_WEST] = (PLAYFIELD_TOP + 32);
	stone_left[STONE_OUTER_EAST] = (PLAYFIELD_LEFT + 16 + (4 * 80));
	stone_top[STONE_OUTER_EAST] = (PLAYFIELD_TOP + 32);
	stone_left[STONE_NORTH] = (PLAYFIELD_LEFT + 16 + (2 * 80));
	stone_top[STONE_NORTH] = (PLAYFIELD_TOP + 16);
	top_22D9A = (PLAYFIELD_TOP + 24);
	left_22D98 = (PLAYFIELD_LEFT + (PLAYFIELD_W / 2) - (PELLET_W / 2));
	laser_wait_frames = 12;
	stones_phase_frame_unused = 0;
	stones_timeout_frame = 0;
	stones_phase = 0;
	stones_pattern = 0;
	boss_explode_angle_offset = 0x20;
	y_22D9C = (96 + scroll_line);
	if(y_22D9C >= RES_Y) {
		y_22D9C -= RES_Y;
	}
	angle_22FAF = 0;
	angle_22FB0 = 0;
	angle_1EB28 = 0;
	stones_laser_left = 32;
	stones_laser_return_delay = 0;
	for(i = 0; i < TILES_X; i++) {
		stones_tile_pending[i] = 0;
	}
	stones_tile_pass = 0;
	stones_tile_pair = 0;
	stones_tile_cols_done = 0;
	tile_image_22FD2 = 40;
	tile_image_22FD4 = 41;
	tile_image_22FD6 = 42;
	boss_pos_x = (stone_left[STONE_INNER_WEST] + 8);
	boss_pos_y = (stone_top[STONE_INNER_WEST] + 8);
	th02_s3_rank_params_set();
	lasers_callbacks_set();
}
