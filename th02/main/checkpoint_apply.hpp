#ifndef TH02_MAIN_CHECKPOINT_APPLY_HPP
#define TH02_MAIN_CHECKPOINT_APPLY_HPP

#include "platform.h"
#include "th02/replay_format.hpp"

enum t2checkpoint_common_reject_t {
	T2CCAR_OK = 0,
	T2CCAR_NULL,
	T2CCAR_SIZE,
	T2CCAR_CONTAINER,
	T2CCAR_GROUP,
	T2CCAR_PAYLOAD,
	T2CCAR_BOUNDARY,
	T2CCAR_ENVIRONMENT,
};

// The schema-4 capture owner keeps the canonical validator private except for
// this narrow read-only bridge. The common apply tail must not duplicate its
// large semantic validator merely to obtain transactional admission.
bool16 far replay_checkpoint_schema4_valid(
	const uint8_t far *container, uint32_t container_size
);

// This is a caller-owned statement about the native setup that preceded a
// later exact restore. It carries no live continuation or hardware state.
struct t2checkpoint_common_boundary_t {
	uint8_t at_ordinary_stage_loop_top;
	uint8_t stage_init_complete;
	uint8_t input_sampled;
	uint8_t pause_or_debounce_active;
	uint8_t blocking_presentation_active;
	uint8_t restore_or_redraw_active;
};

// Pointers refer only to the caller-owned validated container. They are never
// serialized and they must not outlive that container.
struct t2checkpoint_common_plan_t {
	const uint8_t far *container;
	uint32_t container_size;
	uint32_t semantic_digest;
	struct t2checkpoint_common_boundary_t boundary;
	const uint8_t far *group[T2REPLAY_CHECKPOINT_GROUP_COUNT];
	uint8_t page_front;
	uint8_t page_back;
	uint8_t laser_callback_id;
	uint8_t enemy_callback_id;
	uint8_t resource_id;
	uint8_t palette_required;
	uint8_t redraw_required;
};

// Decodes no gameplay state. Later owners use this to validate every planned
// common and actor resource before beginning one mutation phase.
bool16 far t2checkpoint_common_plan_prepare(
	struct t2checkpoint_common_plan_t *plan,
	const uint8_t far *container,
	uint32_t container_size,
	const struct t2checkpoint_common_boundary_t *boundary,
	enum t2checkpoint_common_reject_t *reason
);

// Commits a previously prepared common plan. It restores no actor, palette,
// tile, stage-FX, framebuffer, or stage-specific callback state.
bool16 far t2checkpoint_common_apply_prepared(
	const struct t2checkpoint_common_plan_t *plan
);

// Standalone safe entry point. It repeats preparation immediately before its
// first gameplay write, preventing a stale plan from becoming a partial apply.
bool16 far t2checkpoint_common_apply(
	const uint8_t far *container,
	uint32_t container_size,
	const struct t2checkpoint_common_boundary_t *boundary,
	enum t2checkpoint_common_reject_t *reason
);

#endif /* TH02_MAIN_CHECKPOINT_APPLY_HPP */
