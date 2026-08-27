#ifndef TH01_RPYPIXEL_HPP
#define TH01_RPYPIXEL_HPP

#include "platform.h"
#include "th01/replay_format.hpp"

#if T1REPLAY_PIXEL_TRACE

void t1replay_pixel_probe_reset(void);
bool16 t1replay_pixel_probe_arm(
	const t1replay_checkpoint_t far *checkpoint
);
void t1replay_pixel_probe_restored(
	uint8_t process_seq, uint32_t sample_cursor, uint32_t packet_cursor,
	uint32_t input_cursor, uint32_t semantic_digest
);
void t1replay_pixel_probe_pre_input(
	uint8_t process_seq, uint32_t sample_cursor, uint32_t packet_cursor,
	uint32_t input_cursor, uint32_t semantic_digest
);

#endif

#endif /* TH01_RPYPIXEL_HPP */
