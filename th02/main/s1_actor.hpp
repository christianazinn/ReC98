#ifndef TH02_MAIN_S1_ACTOR_HPP
#define TH02_MAIN_S1_ACTOR_HPP

#include "platform.h"
#include "pc98.h"

// Pointer-free Stage 1 actor state. Generic boss state and callback IDs belong
// to the checkpoint core and are deliberately not duplicated here.
struct th02_s1_midboss_state_t {
	int16_t defeat_frame;
	int16_t patnum;
	bool16 active;
};

struct th02_s1_rika_state_t {
	screen_point_t topleft;
	int16_t move_state;
	int16_t defeat_frame;
	uint8_t pattern_angle;
	uint16_t pattern_frame;
};

enum th02_s1_rika_clean_target_t {
	T2S1_RIKA_START = 0,
	T2S1_RIKA_DAMAGE_700 = 1,
	T2S1_RIKA_DAMAGE_1400 = 2,
};

bool16 far th02_s1_midboss_state_capture(th02_s1_midboss_state_t *state);
bool16 far th02_s1_midboss_state_apply(
	const th02_s1_midboss_state_t *state
);

bool16 far th02_s1_rika_state_capture(th02_s1_rika_state_t *state);
bool16 far th02_s1_rika_state_apply(const th02_s1_rika_state_t *state);

// These two constructors must run after stage_init(). They initialize actor
// state only; the Practice dispatcher still owns field reconstruction,
// callback promotion, enemy retirement, BGM, and the first redraw.
void far th02_s1_midboss_clean_init(void);
bool16 far th02_s1_rika_clean_init(th02_s1_rika_clean_target_t target);

#endif /* TH02_MAIN_S1_ACTOR_HPP */
