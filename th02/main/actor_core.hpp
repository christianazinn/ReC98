#ifndef TH02_MAIN_ACTOR_CORE_HPP
#define TH02_MAIN_ACTOR_CORE_HPP

#include "platform.h"
#include "pc98.h"

#define TH02_ACTOR_CORE_WIRE_SIZE 23

// Pointer-free generic boss and midboss state. Actor-private progression,
// stage effects, callbacks, and replay tags belong to separate owners.
struct th02_actor_core_state_t {
	int16_t patnum;
	int16_t phase_frame;
	screen_x_t left_on_page[PAGE_COUNT];
	screen_y_t top_on_page[PAGE_COUNT];
	int16_t damage;
	uint8_t phase;
	bool16 hit_flash;
	uint8_t rank_param[5];
	uint8_t explode_angle_offset;
	uint8_t bomb_damage_frame_mask;
};

bool16 far th02_actor_core_state_capture(th02_actor_core_state_t *state);
bool16 far th02_actor_core_state_apply(
	const th02_actor_core_state_t *state
);

// Fieldwise private exact-checkpoint codec. It deliberately has no apply
// entry point; the later coordinator owns the all-group transaction.
bool16 far th02_actor_core_state_wire_capture(
	uint8_t far *wire, uint16_t wire_size
);
bool16 far th02_actor_core_state_wire_valid(
	const uint8_t far *wire, uint16_t wire_size
);

#endif /* TH02_MAIN_ACTOR_CORE_HPP */
