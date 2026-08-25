#ifndef TH04_MAIN_REPLAY_CHECKPOINT_HPP
#define TH04_MAIN_REPLAY_CHECKPOINT_HPP

#include "platform.h"

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

struct replay_ck_stream_t {
	uint8_t far *data;
	uint32_t limit;
	uint32_t pos;
	uint32_t checksum;
	replay_ck_mode_t mode;
	bool failed;
};

struct replay_ck_identity_t {
	uint8_t start_kind;
	uint8_t stage;
	uint8_t section;
	uint8_t phase;
	uint32_t source_fingerprint;
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
bool replay_ck_finish(const replay_ck_stream_t far *stream);

// Groups 0 through 11 are independently available for both games. TH05
// additionally provides group 12.
bool replay_ck_group_codec(
	uint8_t group_id, replay_ck_stream_t far *stream
);

// All sizes are below one real-mode segment. [capacity] may exceed the
// measured size; the encoded container itself remains tightly packed.
bool replay_ck_container_measure(
	const replay_ck_identity_t far *identity,
	uint16_t far *total_size,
	uint32_t far *state_digest
);
bool replay_ck_container_encode(
	const replay_ck_identity_t far *identity,
	void far *data,
	uint16_t capacity,
	uint16_t far *total_size,
	uint32_t far *state_digest
);
bool replay_ck_container_validate(
	const replay_ck_identity_t far *identity,
	const void far *data,
	uint16_t total_size,
	uint32_t far *state_digest
);
bool replay_ck_container_apply(
	const replay_ck_identity_t far *identity,
	const void far *data,
	uint16_t total_size
);

#endif /* TH04_MAIN_REPLAY_CHECKPOINT_HPP */
