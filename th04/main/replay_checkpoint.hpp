#ifndef TH04_MAIN_REPLAY_CHECKPOINT_HPP
#define TH04_MAIN_REPLAY_CHECKPOINT_HPP

#include "platform.h"

// Internal field-codec interface. A complete user checkpoint is not emitted
// until every mandatory group has a codec and the restore transaction passes
// its oracle gate.
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

// Groups 0 through 4, 6 through 8, and 11 are independently available for both
// games.
// TH04 also provides group 5; TH05 keeps it unavailable until its stage-local
// actor inventory is complete. All later groups fail. This function does not build
// a checkpoint container, so it cannot expose an incomplete restore to the game.
bool replay_ck_group_codec(
	uint8_t group_id, replay_ck_stream_t far *stream
);

#endif /* TH04_MAIN_REPLAY_CHECKPOINT_HPP */
