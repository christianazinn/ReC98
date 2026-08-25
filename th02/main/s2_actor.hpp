#ifndef TH02_MAIN_S2_ACTOR_HPP
#define TH02_MAIN_S2_ACTOR_HPP

#include "platform.h"
#include "pc98.h"
#include "th01/math/subpixel.hpp"
#include "th02/main/entity.hpp"

static const int TH02_S2_MEIRA_SLASH_COUNT = 40;
static const int TH02_S2_MEIRA_AFTERIMAGE_SLOTS = 3;

// Native Meira slashes have a 12-byte stride. The checkpoint codec must write
// these fields individually and must never serialize padding.
#pragma option -a2
struct th02_s2_meira_slash_state_t {
	entity_flag_t flag;
	uint8_t group;
	screen_x_t left;
	screen_y_t top;
	uint8_t angle;
	uint8_t age;
	uint8_t speed;
	uint8_t trail_patnum;
	uint8_t trail_frames;
};
#pragma option -a-

struct th02_s2_midboss_state_t {
	int16_t defeat_frame;
	bool16 active;
};

// Pointer-free Stage 2 actor state. ACTOR_CORE owns generic boss state and
// STAGE_FX owns scenery and palette flash state.
struct th02_s2_meira_state_t {
	uint8_t phase;
	uint8_t pattern;
	int16_t defeat_frame;
	bool16 afterimages_active;
	screen_x_t dash_origin_x;
	screen_y_t dash_origin_y;
	screen_x_t dash_target_x;
	screen_y_t dash_target_y;
	int16_t dash_step;
	screen_x_t afterimage_left[PAGE_COUNT][TH02_S2_MEIRA_AFTERIMAGE_SLOTS];
	screen_y_t afterimage_top[PAGE_COUNT][TH02_S2_MEIRA_AFTERIMAGE_SLOTS];
	uint8_t player_is_right;
	uint8_t slash_trail_patnum;
	int16_t slash_trail_frames;
	uint8_t slash_burst_i;
	uint8_t burst_group;
	uint8_t burst_speed;
	subpixel_t ramp_speed_a;
	subpixel_t ramp_speed_b;
	th02_s2_meira_slash_state_t slashes[TH02_S2_MEIRA_SLASH_COUNT];
};

enum th02_s2_meira_clean_target_t {
	T2S2_MEIRA_PHASE_1 = 0,
	T2S2_MEIRA_PHASE_2 = 1,
	T2S2_MEIRA_PHASE_3 = 2,
};

bool16 far th02_s2_midboss_state_capture(th02_s2_midboss_state_t *state);
bool16 far th02_s2_midboss_state_apply(
	const th02_s2_midboss_state_t *state
);

bool16 far th02_s2_meira_state_capture(th02_s2_meira_state_t *state);
bool16 far th02_s2_meira_state_apply(const th02_s2_meira_state_t *state);

// These constructors run after stage_init(). The future Practice dispatcher
// remains responsible for field construction, callback promotion, BGM/dialogue,
// enemy retirement, and redraw.
void far th02_s2_midboss_clean_init(void);
bool16 far th02_s2_meira_clean_init(th02_s2_meira_clean_target_t target);

#endif /* TH02_MAIN_S2_ACTOR_HPP */
