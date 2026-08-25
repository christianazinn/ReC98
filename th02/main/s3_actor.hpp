#ifndef TH02_MAIN_S3_ACTOR_HPP
#define TH02_MAIN_S3_ACTOR_HPP

#include "platform.h"
#include "pc98.h"
#include "th02/main/boss/b3.hpp"
#include "th02/main/tile/tile.hpp"

static const int TH02_S3_RING_DOTS = 64;

// Pointer-free Stage 3 actor state. Generic boss fields and the complete
// logical tile ring belong to their respective exact-checkpoint groups.
struct th02_s3_midboss_state_t {
	uint8_t flag[MIDBOSS3_COUNT];
	int16_t damage[MIDBOSS3_COUNT];
	int16_t kill_frame[MIDBOSS3_COUNT];
	screen_x_t left_on_page[MIDBOSS3_COUNT][PAGE_COUNT];
	screen_y_t top_on_page[MIDBOSS3_COUNT][PAGE_COUNT];
	uint8_t angle[MIDBOSS3_COUNT];
	uint8_t loops;
	int16_t patnum;
};

struct th02_s3_field_state_t {
	uint16_t effect_frame;
	int16_t ring_radius;
	uint8_t ring_angle;
	screen_x_t ring_center_x;
	screen_y_t ring_center_y;
	screen_point_t ring_dot[PAGE_COUNT][TH02_S3_RING_DOTS];
};

struct th02_s3_stones_state_t {
	uint8_t flag[STONE_COUNT];
	int16_t damage[STONE_COUNT];
	int16_t patnum[STONE_COUNT];
	uint8_t hit_flash[STONE_COUNT];
	int16_t kill_frame[STONE_COUNT];
	screen_x_t left[STONE_COUNT];
	screen_y_t top[STONE_COUNT];
	uint8_t phase;
	uint8_t pattern;
	int32_t timeout_frame;
	int16_t phase_frame_unused;
	uint8_t angle_phase6_a;
	uint8_t angle_phase6_b;
	uint8_t angle_sweep;
	uint8_t aimed_angle[STONE_COUNT];
	screen_x_t laser_left;
	uint8_t laser_return_delay;
	uint8_t tile_pending[TILES_X];
	uint8_t tile_pass;
	uint8_t tile_pair;
	uint8_t tile_cols_done;
	int16_t tile_image[3];
	screen_x_t muzzle_left;
	screen_y_t muzzle_top;
	vram_y_t mutation_y;
};

bool16 far th02_s3_midboss_state_capture(th02_s3_midboss_state_t *state);
bool16 far th02_s3_midboss_state_apply(
	const th02_s3_midboss_state_t *state
);
bool16 far th02_s3_field_state_capture(th02_s3_field_state_t *state);
bool16 far th02_s3_field_state_apply(const th02_s3_field_state_t *state);
bool16 far th02_s3_stones_state_capture(th02_s3_stones_state_t *state);
bool16 far th02_s3_stones_state_apply(const th02_s3_stones_state_t *state);

// These constructors run only after stage_init(). They own actor state, not
// field construction, callback promotion, BGM/dialog, or the first redraw.
void far th02_s3_midboss_clean_init(void);
void far th02_s3_stones_clean_init(void);

#endif /* TH02_MAIN_S3_ACTOR_HPP */
