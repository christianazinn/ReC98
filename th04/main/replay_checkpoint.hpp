#ifndef TH04_MAIN_REPLAY_CHECKPOINT_HPP
#define TH04_MAIN_REPLAY_CHECKPOINT_HPP

#include "platform.h"
#include "th04/replay_format.hpp"

// Internal field-codec and container interface. Container restore validates
// every byte before mutating live state, applies groups in dependency order,
// restores RNG last, and verifies the resulting semantic digest. A failure
// after mutation is an internal restore defect and must abort the run; this
// real-mode implementation cannot retain a second full container for rollback.
enum replay_ck_mode_t {
	RCK_ENCODE = 0,
	RCK_VALIDATE = 1,
	RCK_APPLY = 2,
};

typedef bool (pascal far *replay_ck_io_func_t)(
	uint16_t context, void far *data, uint16_t size
);

struct replay_ck_stream_t {
	uint8_t far *data;
	uint32_t limit;
	uint32_t pos;
	uint32_t checksum;
	replay_ck_mode_t mode;
	bool failed;
	replay_ck_io_func_t io_func;
	uint16_t io_context;
	uint16_t window_capacity;
	uint16_t window_pos;
	uint16_t window_len;
};

struct replay_ck_identity_t {
	uint8_t start_kind;
	uint8_t stage;
	uint8_t section;
	uint8_t phase;
	uint32_t source_fingerprint;
};

// Runtime-only description of one serialized container. Keeping this separate
// from the on-disk structs lets MAIN stream one group at a time without
// requiring a contiguous copy of the full checkpoint.
struct replay_ck_plan_t {
	uint16_t total_size;
	uint16_t prefix_size;
	uint32_t decoded_size;
	uint32_t state_digest;
	uint32_t container_checksum;
	uint16_t group_sizes[REPLAY_CKPT_GROUPS_MAX];
	uint32_t group_checksums[REPLAY_CKPT_GROUPS_MAX];
};

#define REPLAY_CK_BOSS_SECTION_NONE 0xFF

struct replay_ck_actor_probe_t {
	bool midboss_active;
	bool midboss_finished;
	uint8_t boss_section;
	uint8_t boss_phase;
};

void replay_ck_measure_init(replay_ck_stream_t far *stream);
void replay_ck_encode_init(
	replay_ck_stream_t far *stream, void far *data, uint32_t size
);
void replay_ck_validate_init(
	replay_ck_stream_t far *stream, const void far *data, uint32_t size
);
void replay_ck_apply_init(
	replay_ck_stream_t far *stream, const void far *data, uint32_t size
);
bool replay_ck_finish(replay_ck_stream_t far *stream);

// Groups 0 through 11 are independently available for both games. TH05
// additionally provides group 12.
bool replay_ck_group_codec(
	uint8_t group_id, replay_ck_stream_t far *stream
);
extern uint8_t replay_ck_failure_group_value;
extern uint16_t replay_ck_failure_field_value;
uint8_t replay_ck_failure_group(void);
uint16_t replay_ck_failure_field(void);

// Returns normalized live actor ownership for hidden-Practice boundary
// detection. An unknown callback tuple fails instead of guessing a section.
bool replay_ck_actor_probe(replay_ck_actor_probe_t far *probe);

// All sizes are below one real-mode segment.
bool replay_ck_container_measure(
	const replay_ck_identity_t far *identity,
	uint16_t far *total_size,
	uint32_t far *state_digest
);

// Streaming container interface. Capture writes the prefix followed by each
// encoded group. Restore validates every group before a second, dependency-
// ordered apply pass, preserving the full-container validate-before-mutation
// contract without a 30 KiB conventional-memory allocation.
bool replay_ck_container_plan(
	const replay_ck_identity_t far *identity,
	replay_ck_plan_t far *plan
);
bool replay_ck_container_prefix_encode(
	const replay_ck_identity_t far *identity,
	const replay_ck_plan_t far *plan,
	void far *data,
	uint16_t capacity
);
bool replay_ck_container_prefix_validate(
	const replay_ck_identity_t far *identity,
	const void far *data,
	uint16_t prefix_size,
	uint16_t total_size,
	replay_ck_plan_t far *plan
);
void replay_ck_container_prefix_checksum_set(
	void far *data, uint32_t checksum
);
bool replay_ck_group_encode_stream(
	uint8_t group_id,
	void far *buffer,
	uint16_t buffer_size,
	uint16_t group_size,
	uint32_t expected_checksum,
	replay_ck_io_func_t write_func,
	uint16_t context
);
bool replay_ck_group_validate_stream(
	uint8_t group_id,
	void far *buffer,
	uint16_t buffer_size,
	uint16_t group_size,
	uint32_t expected_checksum,
	replay_ck_io_func_t read_func,
	uint16_t context
);
bool replay_ck_group_apply_stream(
	uint8_t group_id,
	void far *buffer,
	uint16_t buffer_size,
	uint16_t group_size,
	uint32_t expected_checksum,
	replay_ck_io_func_t read_func,
	uint16_t context
);
uint32_t replay_ck_group_digest_begin(
	uint32_t digest, uint8_t group_id, uint16_t size
);

#endif /* TH04_MAIN_REPLAY_CHECKPOINT_HPP */
