#ifndef TH02_MAIN_S4_ACTOR_HPP
#define TH02_MAIN_S4_ACTOR_HPP

#include "platform.h"
#include "pc98.h"
#include "th02/main/boss/b4.hpp"

// Pointer-free Stage 4 actor state. Generic boss, particle-pool, palette,
// tile-ring, and callback state belong to their respective exact groups.
struct th02_s4_midboss_state_t {
	screen_point_t topleft;
	int16_t intro_step;
	int16_t intro_direction;
	int16_t defeat_frame;
	screen_y_t pellet_top;
	bool16 active;
	uint8_t pattern;
	uint8_t patterns_seen;
};

struct th02_s4_marisa_state_t {
	screen_point_t topleft;
	pixel_t velocity_x;
	pixel_t velocity_y;
	int16_t intro_step;
	int16_t intro_direction;
	int8_t pattern;
	int8_t pattern_side;
	uint8_t star_drift_angle;
	uint8_t patterns_seen;
	int8_t orbless_patterns_seen;
	uint8_t rounds_done;
	int16_t defeat_frame;
	uint8_t damage_multiplier;
	int8_t bg_particle_col_i;
	bool16 spray_is_first_run;
	uint8_t pattern_angle[4];
	int8_t swoop_direction;
	screen_x_t swoop_center_x;
	screen_y_t swoop_center_y;
	uint8_t swoop_angle;
	uint8_t volleys_fired;
	uint8_t orb_volley_angle[MARISA_ORB_COUNT];
	int16_t orb_flag[MARISA_ORB_COUNT];
	int16_t orb_damage[MARISA_ORB_COUNT];
	bool16 orb_hit_flash[MARISA_ORB_COUNT];
	int16_t orb_kill_frame[MARISA_ORB_COUNT];
	screen_x_t orb_left_on_page[PAGE_COUNT][MARISA_ORB_COUNT];
	screen_y_t orb_top_on_page[PAGE_COUNT][MARISA_ORB_COUNT];
	int16_t orb_radius[MARISA_ORB_COUNT];
	uint8_t orb_angle[MARISA_ORB_COUNT];
	int16_t orb_angle_delta[MARISA_ORB_COUNT];
	int16_t orb_flag_sum;
};

struct th02_s4_field_state_t {
	vram_y_t tile_top;
};

enum th02_s4_midboss_clean_target_t {
	T2S4_MIDBOSS_FIRST = 0,
	T2S4_MIDBOSS_SECOND = 1,
};

bool16 far th02_s4_midboss_state_capture(th02_s4_midboss_state_t *state);
bool16 far th02_s4_midboss_state_apply(
	const th02_s4_midboss_state_t *state
);
bool16 far th02_s4_marisa_state_capture(th02_s4_marisa_state_t *state);
bool16 far th02_s4_marisa_state_apply(const th02_s4_marisa_state_t *state);
bool16 far th02_s4_field_state_capture(th02_s4_field_state_t *state);
bool16 far th02_s4_field_state_apply(const th02_s4_field_state_t *state);

// These constructors run only after stage_init(). They own actor state, not
// field construction, callback promotion, BGM/dialog, pools, or first redraw.
bool16 far th02_s4_midboss_clean_init(
	th02_s4_midboss_clean_target_t target
);
void far th02_s4_marisa_clean_init(void);

#endif /* TH02_MAIN_S4_ACTOR_HPP */
