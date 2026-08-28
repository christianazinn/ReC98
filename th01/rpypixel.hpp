#ifndef TH01_RPYPIXEL_HPP
#define TH01_RPYPIXEL_HPP

#include "platform.h"
#include "th01/replay_format.hpp"

#if T1REPLAY_PRIVATE_PIXEL_TRACE

void t1replay_pixel_probe_reset(void);

#endif

#if T1REPLAY_PIXEL_TRACE

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

#if T1REPLAY_KONNGARA_PHASE1_TRACE

class CBossEntity;

// Pointer-free summary of the shared world at the natural Konngara seam. The
// private trace writes its values fieldwise; it is not a checkpoint record.
struct t1replay_pixel_world_t {
	uint32_t semantic_digest;
	uint16_t cards;
	uint16_t obstacles;
	uint16_t bomb_items;
	uint16_t point_items;
	uint16_t pellets;
	uint16_t shots;
	uint16_t missiles;
	uint16_t lasers;
	uint16_t particles;
};

void t1replay_pixel_probe_konngara_phase1_arm(
	int8_t phase,
	int phase_frame,
	int hp,
	int hp_first_white,
	int hp_first_redwhite,
	int pattern_state,
	int face_direction,
	int face_expression,
	bool16 face_direction_can_change,
	bool16 hit_invincible,
	int hit_invincibility_frame,
	int pattern_prev,
	int pattern_cur,
	int patterns_done,
	bool16 initial_hp_rendered,
	const CBossEntity& head,
	const CBossEntity& face_closed_or_glare,
	const CBossEntity& face_aim
);
bool16 t1kpx_direct_prepare(void);
bool16 t1kpx_direct_ready(void);
void t1replay_pixel_probe_konngara_phase1_pre_input(
	uint8_t process_seq,
	uint32_t sample_cursor,
	uint32_t packet_cursor,
	uint32_t input_cursor,
	int pellet_speed_raise_cycle,
	uint32_t resource_digest
);

#endif

#if T1REPLAY_YUUGENMAGAN_FIRST_COMBAT_TRACE

// Captures the natural source-proven seam only. No direct state is applied and
// no public replay path can invoke this private witness.
void t1ymx_pre_input(void);

// Records the page-select ports through the graph wrappers. The latches are
// probe-owned far data so the stock graph module's BSS remains unchanged.
void t1ymx_visible_page_set(page_t page);
uint8_t t1ymx_visible_page_get(void);
void t1ymx_accessed_page_set(page_t page);
uint8_t t1ymx_accessed_page_get(void);

#endif

#endif /* TH01_RPYPIXEL_HPP */
