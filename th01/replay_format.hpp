#ifndef TH01_REPLAY_FORMAT_HPP
#define TH01_REPLAY_FORMAT_HPP

/*
 * TH01's user replay format. V4 carries a run from REIIDEN into FUUIN while
 * retaining the compact packet geometry, native replay name, and fieldwise
 * terminal/stage summary.
 */

#include "platform.h"
#include <stddef.h>
#include "th01/common.h"

#define T1REPLAY_VERSION 4
#define T1REPLAY_HEADER_SIZE 258
#define T1REPLAY_START_SIZE 64
#define T1REPLAY_SUMMARY_SIZE 128
#define T1REPLAY_STAGE_SUMMARY_SIZE 6
#define T1REPLAY_PACKET_SIZE 8
#define T1REPLAY_NAME_KANJI 8
#define T1REPLAY_NAME_BYTES (T1REPLAY_NAME_KANJI * 2)
#define T1REPLAY_SLOT_COUNT 100
#define T1REPLAY_SLOT_PENDING T1REPLAY_SLOT_COUNT
#define T1REPLAY_INPUT_SIZE_MAX 0x00400000UL

// Private TH01 semantic checkpoint sidecars are intentionally separate from
// T1RPY4. Each T1CxxYY.CKP
// is keyed by replay slot xx and REIIDEN process yy.
#define T1REPLAY_CHECKPOINT_SCHEMA 4
#define T1REPLAY_CHECKPOINT_HEADER_SIZE 32
#define T1REPLAY_CHECKPOINT_GROUP_SIZE 16
#define T1REPLAY_CHECKPOINT_GROUP_COUNT 14
#define T1REPLAY_CHECKPOINT_SIZE 11236
// Measured from every shipped STAGE?.DAT by th01_stageobj_capacity.py:
// STAGE7 has 76 cards, STAGE4 has 25 obstacles. Capture rejects larger
// modded/live owners rather than silently truncating them.
#define T1REPLAY_CHECKPOINT_CARD_COUNT_MAX 76
#define T1REPLAY_CHECKPOINT_OBSTACLE_COUNT_MAX 25
#define T1REPLAY_CHECKPOINT_PELLET_COUNT 100
#define T1REPLAY_CHECKPOINT_SHOT_COUNT 8
#define T1REPLAY_CHECKPOINT_MISSILE_COUNT 50
#define T1REPLAY_CHECKPOINT_LASER_COUNT 10
#define T1REPLAY_CHECKPOINT_PARTICLE_COUNT 40
#define T1REPLAY_CHECKPOINT_ITEM_BOMB_COUNT 4
#define T1REPLAY_CHECKPOINT_ITEM_POINT_COUNT 10
#define T1REPLAY_CHECKPOINT_BOSS_PAYLOAD_SIZE 264
#define T1REPLAY_CHECKPOINT_FLAG_CAPTURE_ONLY 0x0001
#define T1REPLAY_CHECKPOINT_FLAGS_KNOWN T1REPLAY_CHECKPOINT_FLAG_CAPTURE_ONLY
#define T1REPLAY_CHECKPOINT_PROCESS_MAX 99
#define T1REPLAY_CHECKPOINT_CODEC_RAW 0

// Private FUUIN validation can bind the decoded score table seen before
// registration and the in-memory result afterward to a finalized replay.
// T1RPY4 stays unchanged; release builds neither read nor write this sidecar.
#define T1REPLAY_SCORE_PROOF_SCHEMA 1
#define T1REPLAY_SCORE_PROOF_SIZE 48
#ifndef T1REPLAY_FUUIN_SCORE_PROOF
#define T1REPLAY_FUUIN_SCORE_PROOF 0
#endif

// Release recording captures the first semantic boundary in BSS only. Private
// capture builds may opt into a process-end sidecar flush; ordinary users must
// never take a DOS create/write on that first gameplay frame.
#ifndef T1REPLAY_CHECKPOINT_EMIT
#define T1REPLAY_CHECKPOINT_EMIT 0
#endif

// Private validation builds can begin each REIIDEN process at its semantic
// sidecar. Release builds keep this disabled and perform no checkpoint reads.
#ifndef T1REPLAY_CHECKPOINT_RESTORE
#define T1REPLAY_CHECKPOINT_RESTORE 0
#endif

// Kept deliberately short because the 16-bit compiler receives this switch
// through DOS's command tail. Values 1 through 3 select the private capture,
// sequential, and direct exact-restore profiles. Values 4 and 5 add the
// measurement-only SinGyoku pixel probe to sequential and direct playback.
// Value 6 captures the natural Konngara phase-1-frame-0 witness only.
#ifndef T1RP
#define T1RP 0
#endif

#if T1RP
	#undef T1REPLAY_CHECKPOINT_EMIT
	#undef T1REPLAY_CHECKPOINT_RESTORE
	#define T1REPLAY_CHECKPOINT_EMIT (T1RP == 1)
	#define T1REPLAY_CHECKPOINT_RESTORE ((T1RP == 3) || (T1RP == 5))
#endif

#define T1REPLAY_PIXEL_TRACE ((T1RP == 4) || (T1RP == 5))
#define T1REPLAY_KONNGARA_PHASE1_TRACE (T1RP == 6)
#define T1REPLAY_PRIVATE_PIXEL_TRACE ( \
	T1REPLAY_PIXEL_TRACE || T1REPLAY_KONNGARA_PHASE1_TRACE \
)

// Private semantic tracing for direct-versus-sequential checkpoint evidence.
// This is not part of T1RPY4 or T1CKP1 and stays absent from release builds.
// T1RP6 is a bounded pixel witness, not an exact-trace producer.
#ifndef T1REPLAY_EXACT_TRACE
	#define T1REPLAY_EXACT_TRACE ((T1RP >= 1) && (T1RP <= 5))
#endif
#define T1REPLAY_WORLD_CAPTURE ( \
	T1REPLAY_EXACT_TRACE || T1REPLAY_KONNGARA_PHASE1_TRACE \
)

#define T1REPLAY_STATUS_RECORDING 1
#define T1REPLAY_STATUS_FINALIZED 2
#define T1REPLAY_STATUS_ERROR 3

#define T1REPLAY_COMMAND_RECORD 1
#define T1REPLAY_COMMAND_PLAYBACK 2

#define T1REPLAY_SAVE_REQUEST_SCHEMA 1
#define T1REPLAY_SAVE_REQUEST_SIZE 20
#define T1REPLAY_RESTART_REQUEST_SCHEMA 1
#define T1REPLAY_RESTART_REQUEST_SIZE 20

enum t1replay_save_request_source_t {
	T1RSRS_POSTGAME = 0,
	T1RSRS_PAUSE = 1,
};

enum t1replay_practice_section_t {
	T1RPS_STAGE_START,
	T1RPS_CHAPTER,
	T1RPS_BOSS_START,
};

// OP's transient Practice choice is also the semantic restart configuration.
// It contains no pointers or live-world state and is deliberately separate
// from the user replay ABI.
struct t1replay_practice_start_t {
	uint8_t scene;
	uint8_t route;
	uint8_t section;
	uint8_t chapter;
	int8_t rank;
	int32_t score;
	int8_t lives;
	int8_t bombs;
	uint16_t point_value;
	int16_t pellet_speed;
	uint32_t rand;
};

inline bool t1replay_slot_is_numbered(uint8_t slot)
{
	return (slot < T1REPLAY_SLOT_COUNT);
}

inline bool t1replay_slot_is_pending(uint8_t slot)
{
	return (slot == T1REPLAY_SLOT_PENDING);
}

// The pending sentinel is a process-local record target, never a playable
// numbered slot. It keeps the T1RPY4 header and resident carrier ABI stable
// while OP decides whether a finalized capture should become permanent.
inline bool t1replay_slot_valid_for_mode(uint8_t mode, uint8_t slot)
{
	return (
		t1replay_slot_is_numbered(slot) ||
		((mode == T1REPLAY_COMMAND_RECORD) && t1replay_slot_is_pending(slot))
	);
}

#define T1REPLAY_FLAG_RLE 0x0001
#define T1REPLAY_FLAG_KEY_LATCH 0x0002
#define T1REPLAY_FLAGS_KNOWN (T1REPLAY_FLAG_RLE | T1REPLAY_FLAG_KEY_LATCH)

#define T1REPLAY_INPUT_SEMANTICS_LATCHED_GROUPS 1

#define T1REPLAY_PACKET_CONTROL 0x80
#define T1REPLAY_PACKET_RUN_MASK 0x7F
#define T1REPLAY_PACKET_RUN_MAX (T1REPLAY_PACKET_RUN_MASK + 1)
#define T1REPLAY_CONTROL_PROCESS_END 1
#define T1REPLAY_CONTROL_TERMINAL 2
#define T1REPLAY_CONTROL_PHASE 3

#define T1REPLAY_END_MENU 1
#define T1REPLAY_END_CLEAR 2

#define T1REPLAY_PROCESS_NONE 0
#define T1REPLAY_PROCESS_REIIDEN 1
#define T1REPLAY_PROCESS_FUUIN 2

#define T1REPLAY_FUUIN_PHASE_NONE 0
#define T1REPLAY_FUUIN_PHASE_VERDICT 1
#define T1REPLAY_FUUIN_PHASE_SCORE_NAME 2
#define T1REPLAY_FUUIN_PHASE_SCORE_RELEASE 3

#define T1REPLAY_RES_ID "T1ReplayState"
#define T1REPLAY_RES_VERSION 2
#define T1REPLAY_RESTART_RES_ID "T1ReplayRestart"
#define T1REPLAY_RESTART_RES_VERSION 1

#define T1REPLAY_FNV1A_BASIS 0x811C9DC5UL
#define T1REPLAY_FNV1A_PRIME 0x01000193UL

enum t1replay_input_group_t {
	T1RIG_0 = 0,
	T1RIG_3,
	T1RIG_5,
	T1RIG_6,
	T1RIG_7,
	T1RIG_8,
	T1RIG_9,
	T1REPLAY_INPUT_GROUP_COUNT,
};

enum t1replay_fuuin_input_index_t {
	T1RFIG_0 = 0,
	T1RFIG_3,
	T1RFIG_5,
	T1RFIG_7,
	T1REPLAY_FUUIN_INPUT_GROUP_COUNT,
};

enum t1replay_mode_t {
	T1RM_DISABLED = 0,
	T1RM_RECORD = 1,
	T1RM_PLAYBACK = 2,
};

enum t1replay_restart_kind_t {
	T1RRK_NORMAL = 1,
	T1RRK_PRACTICE = 2,
};

enum t1replay_checkpoint_group_id_t {
	T1RCGI_SCENARIO = 0,
	T1RCGI_RNG = 1,
	T1RCGI_INPUT = 2,
	T1RCGI_PACING = 3,
	T1RCGI_PLAYER = 4,
	T1RCGI_ORB = 5,
	T1RCGI_STAGE = 6,
	T1RCGI_ITEMS = 7,
	T1RCGI_PELLETS = 8,
	T1RCGI_SHOTS = 9,
	T1RCGI_MISSILES = 10,
	T1RCGI_LASERS = 11,
	T1RCGI_PARTICLES = 12,
	T1RCGI_BOSS = 13,
};

// Only bits consumed by TH01's REIIDEN input path are replayed. Keeping the
// stream canonical prevents unrelated keyboard state from becoming format ABI.
#define T1REPLAY_INPUT_MASK_0 0x01
#define T1REPLAY_INPUT_MASK_3 0x10
#define T1REPLAY_INPUT_MASK_5 0x06
#define T1REPLAY_INPUT_MASK_6 0xC0
#define T1REPLAY_INPUT_MASK_7 0x3C
#define T1REPLAY_INPUT_MASK_8 0x48
#define T1REPLAY_INPUT_MASK_9 0x09

// This is intentionally an explicit field list, not a byte-copy of
// resident_t. It is the cross-EXE state REIIDEN actually consumes before the
// first input sample. Its exact shape is shared with the host parser.
struct t1replay_start_t {
	uint32_t resident_rand;
	int32_t score;
	int32_t continues_total;
	uint32_t hiscore;
	int32_t score_highest;
	int32_t bonus_per_stage[4];
	uint16_t continues_per_scene[4];
	uint16_t stage_id;
	uint16_t point_value;
	int16_t pellet_speed;
	int8_t rank;
	int8_t bgm_mode;
	int8_t rem_bombs;
	int8_t credit_lives_extra;
	int8_t rem_lives;
	int8_t route;
	int8_t end_flag;
	int8_t debug_mode;
	int8_t snd_need_init;
	int8_t mode_test;
	int8_t start_binary;
	uint8_t reserved[3];
};

#define T1REPLAY_STAGE_FLAG_REACHED 0x01
#define T1REPLAY_STAGE_FLAG_COMPLETE 0x02
#define T1REPLAY_STAGE_FLAGS_KNOWN (T1REPLAY_STAGE_FLAG_REACHED | T1REPLAY_STAGE_FLAG_COMPLETE)
#define T1REPLAY_FINAL_STAGE_NONE 0xFF

// Records are stored in route order, not indexed by stage ID. Keeping the ID
// explicit lets every reader reject duplicates and ordering corruption.
struct t1replay_stage_summary_t {
	int32_t score;
	uint8_t stage_id;
	uint8_t flags;
};

struct t1replay_summary_t {
	int32_t final_score;
	uint8_t final_stage_id;
	uint8_t terminal_reason;
	uint8_t split_count;
	uint8_t reserved;
	t1replay_stage_summary_t splits[STAGE_COUNT];
};

struct t1replay_header_t {
	char magic[8]; // "T1RPY4\\0\\0"
	uint16_t version;
	uint16_t header_size;
	uint16_t packet_size;
	uint16_t flags;
	uint8_t status;
	uint8_t end_reason;
	uint8_t game_id;
	uint8_t input_semantics;
	uint8_t process_count;
	uint8_t reserved_0;
	uint32_t sample_count;
	uint32_t packet_count;
	uint32_t input_offset;
	uint32_t input_size;
	uint32_t payload_checksum;
	uint32_t start_checksum;
	uint32_t header_checksum;
	t1replay_start_t start;
	uint8_t name[T1REPLAY_NAME_BYTES];
	t1replay_summary_t summary;
};

// The native score-registration keyboard accepts full-width ASCII letters and
// numerals plus exactly these eighteen symbols. T1RPY4 uses the same cells;
// unentered trailing cells are pairs of ordinary ASCII spaces.
inline bool t1replay_name_cell_valid(uint8_t lead, uint8_t trail)
{
	if(lead == 0x82) {
		return (
			((trail >= 0x4F) && (trail <= 0x58)) ||
			((trail >= 0x60) && (trail <= 0x79)) ||
			((trail >= 0x81) && (trail <= 0x9A))
		);
	}
	if(lead != 0x81) {
		return false;
	}
	switch(trail) {
	case 0x40: // Full-width space, distinct from trailing padding.
	case 0x44: case 0x45: case 0x48: case 0x49:
	case 0x5E: case 0x63: case 0x67: case 0x68:
	case 0x87: case 0x88: case 0x89: case 0x8A:
	case 0x94: case 0x95: case 0x96: case 0x98:
	case 0x99: case 0x9F:
		return true;
	}
	return false;
}

inline bool t1replay_name_valid(const uint8_t far *name)
{
	bool padding_seen = false;
	uint8_t cell;

	for(cell = 0; cell < T1REPLAY_NAME_KANJI; cell++) {
		uint8_t lead = name[(cell * 2) + 0];
		uint8_t trail = name[(cell * 2) + 1];

		if((lead == ' ') && (trail == ' ')) {
			padding_seen = true;
		} else if(padding_seen || !t1replay_name_cell_valid(lead, trail)) {
			return false;
		}
	}
	return true;
}

inline bool t1replay_summary_valid(
	const t1replay_summary_t far *summary,
	const t1replay_start_t far *start,
	bool finalized,
	uint8_t header_terminal_reason
)
{
	uint8_t i;
	const t1replay_stage_summary_t far *split;

	if(
		(start->stage_id >= STAGE_COUNT) ||
		(summary->reserved != 0) ||
		(summary->split_count > STAGE_COUNT) ||
		(summary->split_count > (STAGE_COUNT - start->stage_id))
	) {
		return false;
	}
	for(i = 0; i < summary->split_count; i++) {
		split = &summary->splits[i];
		if(
			(split->stage_id != (start->stage_id + i)) ||
			(split->flags & ~T1REPLAY_STAGE_FLAGS_KNOWN) ||
			!(split->flags & T1REPLAY_STAGE_FLAG_REACHED) ||
			((i < (summary->split_count - 1)) &&
			 !(split->flags & T1REPLAY_STAGE_FLAG_COMPLETE))
		) {
			return false;
		}
	}
	for(; i < STAGE_COUNT; i++) {
		split = &summary->splits[i];
		if((split->score != 0) || (split->stage_id != 0) || (split->flags != 0)) {
			return false;
		}
	}
	if(!finalized) {
		if(
			(summary->final_score != 0) ||
			(summary->final_stage_id != T1REPLAY_FINAL_STAGE_NONE) ||
			(summary->terminal_reason != 0) ||
			((summary->split_count != 0) &&
			 !(summary->splits[summary->split_count - 1].flags &
			   T1REPLAY_STAGE_FLAG_COMPLETE))
		) {
			return false;
		}
		return true;
	}
	if(
		(summary->split_count == 0) ||
		(summary->terminal_reason != header_terminal_reason) ||
		((header_terminal_reason != T1REPLAY_END_MENU) &&
		 (header_terminal_reason != T1REPLAY_END_CLEAR))
	) {
		return false;
	}
	split = &summary->splits[summary->split_count - 1];
	if(
		(summary->final_stage_id != split->stage_id) ||
		(summary->final_score != split->score)
	) {
		return false;
	}
	if(header_terminal_reason == T1REPLAY_END_CLEAR) {
		return (
			(split->stage_id == (STAGE_COUNT - 1)) &&
			(split->flags == T1REPLAY_STAGE_FLAGS_KNOWN)
		);
	}
	return (split->flags == T1REPLAY_STAGE_FLAG_REACHED);
}

struct t1replay_packet_t {
	uint8_t tag;
	uint8_t keys[T1REPLAY_INPUT_GROUP_COUNT];
};

struct t1replay_command_t {
	char magic[8]; // "T1RPYC\\0\\0"
	uint8_t mode;
	uint8_t slot;
	uint8_t reserved[6];
};

struct t1replay_save_request_t {
	char magic[8]; // "T1RSAV1\\0"
	uint8_t schema;
	uint8_t source;
	uint16_t reserved;
	uint32_t replay_header_checksum;
	uint32_t checksum;
};

// A restart request is deliberately tiny. The complete launch description
// remains in a checksummed ResData block, so a stale on-disk request cannot
// recreate a run after its matching resident state has gone away.
struct t1replay_restart_request_t {
	char magic[8]; // "T1RRST1\0"
	uint8_t schema;
	uint8_t reserved_0;
	uint16_t reserved_1;
	uint32_t restart_state_checksum;
	uint32_t checksum;
};

struct t1replay_restart_state_t {
	char id[sizeof(T1REPLAY_RESTART_RES_ID)];
	char magic[4];
	uint8_t version;
	uint8_t kind;
	uint8_t reserved[2];
	t1replay_practice_start_t practice;
	uint32_t checksum;
};

// Cross-process replay state. [source_process] owns the committed handoff at
// the replay prefix, while [target_process] is the only executable permitted
// to resume it. No process pointers or interrupt-visible state cross this ABI.
struct t1replay_res_t {
	char id[sizeof(T1REPLAY_RES_ID)];
	char magic[4];
	uint8_t version;
	uint8_t mode;
	uint8_t slot;
	uint8_t process_seq;
	uint8_t source_process;
	uint8_t target_process;
	uint8_t reserved[2];
	uint32_t sample_count;
	uint32_t packet_count;
	uint32_t input_size;
	uint32_t payload_checksum;
	uint32_t start_checksum;
	uint32_t handoff_checksum;
	uint32_t checksum;
};

// Exactly the resident fields FUUIN consumes before end_init() destroys them.
struct t1replay_fuuin_handoff_t {
	uint32_t resident_rand;
	int32_t score;
	int32_t score_highest;
	int32_t continues_total;
	uint16_t continues_per_scene[4];
	int8_t rank;
	int8_t credit_lives_extra;
	int8_t end_flag;
	uint8_t reserved;
};

struct t1replay_score_proof_t {
	char magic[8]; // "T1SDG1\0\0"
	uint16_t schema;
	uint16_t size;
	uint8_t game_id;
	uint8_t slot;
	uint8_t rank;
	uint8_t phase;
	uint32_t replay_start_checksum;
	uint32_t replay_payload_checksum;
	uint32_t replay_sample_count;
	uint32_t replay_packet_count;
	uint32_t before_digest;
	uint32_t after_digest;
	uint32_t container_checksum;
	uint8_t reserved[4];
};

// This envelope is a capture and validation substrate only. It never stores
// resident pointers, heap addresses, VRAM, function pointers, or raw BSS.
// Exact restore stays unavailable until the world-pool and boss codecs exist.
struct t1replay_checkpoint_header_t {
	char magic[8]; // "T1CKP1\\0\\0"
	uint16_t schema;
	uint16_t header_size;
	uint8_t game_id;
	uint8_t group_count;
	uint16_t flags;
	uint32_t total_size;
	uint32_t replay_start_checksum;
	uint32_t state_digest;
	uint32_t container_checksum;
};

struct t1replay_checkpoint_group_t {
	uint8_t id;
	uint8_t schema;
	uint8_t codec;
	uint8_t flags;
	uint32_t offset;
	uint16_t stored_size;
	uint16_t decoded_size;
	uint32_t checksum;
};

// `resident_*` names are fields from resident_t; `game_*` names are the
// separately-owned REIIDEN mirrors that affect later native game logic.
struct t1replay_checkpoint_scenario_t {
	uint32_t resident_rand;
	int32_t resident_score;
	int32_t resident_continues_total;
	uint32_t resident_hiscore;
	int32_t resident_score_highest;
	int32_t resident_bonus_per_stage[4];
	uint16_t resident_continues_per_scene[4];
	int32_t game_score;
	int32_t game_continues_total;
	uint32_t reserved_0;
	uint16_t resident_stage_id;
	uint16_t resident_point_value;
	int16_t resident_pellet_speed;
	int8_t resident_rank;
	int8_t resident_bgm_mode;
	int8_t resident_rem_bombs;
	int8_t resident_credit_lives_extra;
	int8_t resident_end_flag;
	int8_t resident_route;
	int8_t resident_rem_lives;
	int8_t resident_snd_need_init;
	int8_t resident_debug_mode;
	int8_t game_rank;
	int8_t game_bgm_mode;
	int8_t game_rem_bombs;
	int8_t game_credit_lives_extra;
	int8_t game_route;
	int8_t game_rem_lives;
	int8_t mode_test;
	uint8_t reserved[2];
};

// random_seed is master.lib's current irand() state. frame_rand is the
// independent REIIDEN gameplay clock from which native code also derives RNG.
struct t1replay_checkpoint_rng_t {
	uint32_t frame_rand;
	uint32_t random_seed;
};

// input_history is the edge-detection state formerly hidden inside
// input_sense(). It is semantic state, not a physical keyboard snapshot.
struct t1replay_checkpoint_input_t {
	uint8_t input_history[16];
	uint8_t input_lr;
	uint8_t input_shot;
	uint8_t input_ok;
	uint8_t input_strike;
	uint8_t input_up;
	uint8_t input_down;
	uint8_t input_bomb;
	uint8_t paused;
	uint8_t player_is_hit;
	uint8_t input_mem_enter;
	uint8_t input_mem_leave;
	uint8_t reserved_0;
	int16_t bomb_doubletap_frame;
	int16_t bomb_doubletap_frame_unused;
};

struct t1replay_checkpoint_pacing_t {
	uint32_t frame_since_start_of_binary;
	uint32_t bomb_frame;
	uint32_t replay_sample_anchor;
	uint32_t replay_packet_anchor;
	uint32_t replay_input_anchor;
	uint32_t replay_prefix_checksum;
	uint16_t stage_timer;
	uint16_t frame_since_harryup;
	int16_t pellet_speed_raise_cycle;
	uint8_t process_seq;
	uint8_t harryup_cycle;
	uint8_t timer_initialized;
	uint8_t first_stage_in_scene;
	uint8_t stage_wait_for_shot_to_begin;
	uint8_t reserved;
};

// Player animation resource handles, heap pointers, and VRAM are deliberately
// absent. This is the scalar state consumed at the next gameplay-loop update.
struct t1replay_checkpoint_player_t {
	int16_t player_left;
	int16_t player_invincibility_time;
	int16_t cardcombo_cur;
	int16_t cardcombo_max;
	int8_t swing_deflection_frames;
	int8_t dash_cycle;
	int8_t mode_frame;
	int8_t mode;
	int8_t dash_direction;
	int8_t bomb_flag;
	int8_t bombing;
	int8_t combo_enabled;
	int8_t submode;
	int8_t ptn_id_prev;
	uint8_t player_deflecting;
	uint8_t player_sliding;
	uint8_t player_is_hit;
	uint8_t player_invincible;
	uint8_t player_invincible_against_orb;
	uint8_t bomb_damaging;
	uint8_t reserved[2];
};

struct t1replay_checkpoint_orb_t {
	int16_t cur_left;
	int16_t cur_top;
	int16_t prev_left;
	int16_t prev_top;
	int16_t frames_outside_portal;
	int16_t rotation_frame;
	int16_t force_frame;
	int16_t velocity_x;
	uint16_t in_portal;
	double force;
	double velocity_y;
	uint8_t reserved[2];
};

// Heap-backed stage objects are encoded as stable slots. Their page-1 VRAM
// snapshots and backing allocations are intentionally not part of this codec.
struct t1replay_checkpoint_card_t {
	int16_t left;
	int16_t top;
	int16_t flip_frame;
	uint32_t score;
	int8_t hp;
	uint8_t flag;
};

struct t1replay_checkpoint_obstacle_t {
	int16_t left;
	int16_t top;
	int16_t frame;
	uint8_t type;
	uint8_t turret_flag;
};

struct t1replay_checkpoint_stage_t {
	uint16_t cards_count;
	uint16_t obstacles_count;
	int16_t entered_portal_slot;
	int16_t portal_dst_left;
	int16_t portal_dst_top;
	uint8_t vertical_bars_blocked;
	uint8_t portals_blocked;
	uint8_t card_flip_cycle;
	uint8_t reserved;
	t1replay_checkpoint_card_t cards[T1REPLAY_CHECKPOINT_CARD_COUNT_MAX];
	t1replay_checkpoint_obstacle_t obstacles[
		T1REPLAY_CHECKPOINT_OBSTACLE_COUNT_MAX
	];
};

struct t1replay_checkpoint_item_t {
	int16_t left;
	int16_t top;
	int16_t unknown_zero;
	int16_t velocity_y;
	uint8_t flag;
	uint8_t state;
};

struct t1replay_checkpoint_items_t {
	t1replay_checkpoint_item_t bombs[T1REPLAY_CHECKPOINT_ITEM_BOMB_COUNT];
	t1replay_checkpoint_item_t points[T1REPLAY_CHECKPOINT_ITEM_POINT_COUNT];
};

struct t1replay_checkpoint_pellet_t {
	int32_t cur_left;
	int32_t cur_top;
	int32_t spin_center_left;
	int32_t spin_center_top;
	int32_t prev_left;
	int32_t prev_top;
	int32_t velocity_x;
	int32_t velocity_y;
	int32_t spin_velocity_x;
	int32_t spin_velocity_y;
	int32_t speed;
	int16_t from_group;
	int16_t age;
	int16_t decay_frame;
	int16_t cloud_frame;
	int16_t cloud_left;
	int16_t cloud_top;
	int16_t angle;
	int16_t sling_direction;
	uint8_t moving;
	uint8_t motion_type;
	uint16_t not_rendered;
};

struct t1replay_checkpoint_pellets_t {
	int16_t alive_count_excluding_cloud_pellets_after_reset;
	int16_t unknown_seven;
	uint16_t interlace_field;
	uint8_t spawn_with_cloud;
	uint8_t reserved;
	t1replay_checkpoint_pellet_t pellets[T1REPLAY_CHECKPOINT_PELLET_COUNT];
};

struct t1replay_checkpoint_shot_t {
	int16_t left;
	int16_t top;
	int16_t unknown;
	uint8_t moving;
	uint8_t decay_frame;
};

struct t1replay_checkpoint_shots_t {
	t1replay_checkpoint_shot_t shots[T1REPLAY_CHECKPOINT_SHOT_COUNT];
};

struct t1replay_checkpoint_missile_t {
	int32_t cur_left;
	int32_t cur_top;
	int32_t prev_left;
	int32_t prev_top;
	int32_t velocity_x;
	int32_t velocity_y;
	int8_t unknown;
	uint8_t flag;
};

struct t1replay_checkpoint_missiles_t {
	uint8_t ptn_id_base;
	uint8_t reserved[3];
	t1replay_checkpoint_missile_t missiles[
		T1REPLAY_CHECKPOINT_MISSILE_COUNT
	];
};

struct t1replay_checkpoint_laser_t {
	int32_t origin_left;
	int32_t origin_y;
	int32_t ray_start_left;
	int32_t ray_start_y;
	int32_t ray_i_left;
	int32_t ray_i_y;
	int16_t ray_length;
	int16_t ray_moveout_speed;
	int16_t target_left;
	int16_t target_y;
	int16_t unknown;
	int32_t velocity_y;
	int32_t step_y;
	int32_t velocity_x;
	int32_t step_x;
	int16_t ray_extend_speed;
	uint16_t alive;
	int16_t age;
	int16_t moveout_at_age;
	uint8_t col;
	uint8_t width_cel;
	uint8_t damaging;
	uint8_t id;
	uint8_t put_flag;
	uint8_t reserved;
};

struct t1replay_checkpoint_lasers_t {
	t1replay_checkpoint_laser_t lasers[T1REPLAY_CHECKPOINT_LASER_COUNT];
};

struct t1replay_checkpoint_particle_t {
	int32_t x;
	int32_t y;
	int32_t velocity_x;
	int32_t velocity_y;
	uint8_t alive;
	uint8_t velocity_base;
	uint8_t reserved[2];
};

struct t1replay_checkpoint_particles_t {
	int16_t spawn_interval;
	int16_t velocity_base_max;
	uint8_t spawn_cycle;
	uint8_t reserved[3];
	t1replay_checkpoint_particle_t particles[
		T1REPLAY_CHECKPOINT_PARTICLE_COUNT
	];
};

// The tagged payload is sized for the largest owner record already derived
// from the current boss corpus (Kikuri). Unsupported owners fail capture
// closed; the public replay stream does not depend on this private envelope.
struct t1replay_checkpoint_boss_t {
	int8_t boss_id;
	uint8_t owner;
	uint8_t owner_schema;
	uint8_t flags;
	uint16_t payload_size;
	uint16_t reserved_0;
	uint8_t payload[T1REPLAY_CHECKPOINT_BOSS_PAYLOAD_SIZE];
};

struct t1replay_checkpoint_t {
	t1replay_checkpoint_header_t header;
	t1replay_checkpoint_group_t groups[T1REPLAY_CHECKPOINT_GROUP_COUNT];
	t1replay_checkpoint_scenario_t scenario;
	t1replay_checkpoint_rng_t rng;
	t1replay_checkpoint_input_t input;
	t1replay_checkpoint_pacing_t pacing;
	t1replay_checkpoint_player_t player;
	t1replay_checkpoint_orb_t orb;
	t1replay_checkpoint_stage_t stage;
	t1replay_checkpoint_items_t items;
	t1replay_checkpoint_pellets_t pellets;
	t1replay_checkpoint_shots_t shots;
	t1replay_checkpoint_missiles_t missiles;
	t1replay_checkpoint_lasers_t lasers;
	t1replay_checkpoint_particles_t particles;
	t1replay_checkpoint_boss_t boss;
};

typedef char t1replay_start_size_check[
	(sizeof(t1replay_start_t) == T1REPLAY_START_SIZE) ? 1 : -1
];
typedef char t1replay_header_size_check[
	(sizeof(t1replay_header_t) == T1REPLAY_HEADER_SIZE) ? 1 : -1
];
typedef char t1replay_stage_summary_size_check[
	(sizeof(t1replay_stage_summary_t) == T1REPLAY_STAGE_SUMMARY_SIZE) ? 1 : -1
];
typedef char t1replay_summary_size_check[
	(sizeof(t1replay_summary_t) == T1REPLAY_SUMMARY_SIZE) ? 1 : -1
];
typedef char t1replay_packet_size_check[
	(sizeof(t1replay_packet_t) == T1REPLAY_PACKET_SIZE) ? 1 : -1
];
typedef char t1replay_command_size_check[
	(sizeof(t1replay_command_t) == 16) ? 1 : -1
];
typedef char t1replay_save_request_size_check[
	(sizeof(t1replay_save_request_t) == T1REPLAY_SAVE_REQUEST_SIZE) ? 1 : -1
];
typedef char t1replay_restart_request_size_check[
	(sizeof(t1replay_restart_request_t) == T1REPLAY_RESTART_REQUEST_SIZE) ? 1 : -1
];
typedef char t1replay_res_size_check[
	(sizeof(t1replay_res_t) == 54) ? 1 : -1
];
typedef char t1replay_fuuin_handoff_size_check[
	(sizeof(t1replay_fuuin_handoff_t) == 28) ? 1 : -1
];
typedef char t1replay_score_proof_size_check[
	(sizeof(t1replay_score_proof_t) == T1REPLAY_SCORE_PROOF_SIZE) ? 1 : -1
];
typedef char t1replay_checkpoint_header_size_check[
	(sizeof(t1replay_checkpoint_header_t) == T1REPLAY_CHECKPOINT_HEADER_SIZE) ? 1 : -1
];
typedef char t1replay_checkpoint_group_size_check[
	(sizeof(t1replay_checkpoint_group_t) == T1REPLAY_CHECKPOINT_GROUP_SIZE) ? 1 : -1
];
typedef char t1replay_checkpoint_scenario_size_check[
	(sizeof(t1replay_checkpoint_scenario_t) == 80) ? 1 : -1
];
typedef char t1replay_checkpoint_rng_size_check[
	(sizeof(t1replay_checkpoint_rng_t) == 8) ? 1 : -1
];
typedef char t1replay_checkpoint_input_size_check[
	(sizeof(t1replay_checkpoint_input_t) == 32) ? 1 : -1
];
typedef char t1replay_checkpoint_pacing_size_check[
	(sizeof(t1replay_checkpoint_pacing_t) == 36) ? 1 : -1
];
typedef char t1replay_checkpoint_player_size_check[
	(sizeof(t1replay_checkpoint_player_t) == 26) ? 1 : -1
];
typedef char t1replay_checkpoint_orb_size_check[
	(sizeof(t1replay_checkpoint_orb_t) == 36) ? 1 : -1
];
typedef char t1replay_checkpoint_card_size_check[
	(sizeof(t1replay_checkpoint_card_t) == 12) ? 1 : -1
];
typedef char t1replay_checkpoint_obstacle_size_check[
	(sizeof(t1replay_checkpoint_obstacle_t) == 8) ? 1 : -1
];
typedef char t1replay_checkpoint_stage_size_check[
	(sizeof(t1replay_checkpoint_stage_t) == 1126) ? 1 : -1
];
typedef char t1replay_checkpoint_item_size_check[
	(sizeof(t1replay_checkpoint_item_t) == 10) ? 1 : -1
];
typedef char t1replay_checkpoint_items_size_check[
	(sizeof(t1replay_checkpoint_items_t) == 140) ? 1 : -1
];
typedef char t1replay_checkpoint_pellet_size_check[
	(sizeof(t1replay_checkpoint_pellet_t) == 64) ? 1 : -1
];
typedef char t1replay_checkpoint_pellets_size_check[
	(sizeof(t1replay_checkpoint_pellets_t) == 6408) ? 1 : -1
];
typedef char t1replay_checkpoint_shot_size_check[
	(sizeof(t1replay_checkpoint_shot_t) == 8) ? 1 : -1
];
typedef char t1replay_checkpoint_shots_size_check[
	(sizeof(t1replay_checkpoint_shots_t) == 64) ? 1 : -1
];
typedef char t1replay_checkpoint_missile_size_check[
	(sizeof(t1replay_checkpoint_missile_t) == 26) ? 1 : -1
];
typedef char t1replay_checkpoint_missiles_size_check[
	(sizeof(t1replay_checkpoint_missiles_t) == 1304) ? 1 : -1
];
typedef char t1replay_checkpoint_laser_size_check[
	(sizeof(t1replay_checkpoint_laser_t) == 64) ? 1 : -1
];
typedef char t1replay_checkpoint_lasers_size_check[
	(sizeof(t1replay_checkpoint_lasers_t) == 640) ? 1 : -1
];
typedef char t1replay_checkpoint_particle_size_check[
	(sizeof(t1replay_checkpoint_particle_t) == 20) ? 1 : -1
];
typedef char t1replay_checkpoint_particles_size_check[
	(sizeof(t1replay_checkpoint_particles_t) == 808) ? 1 : -1
];
typedef char t1replay_checkpoint_boss_size_check[
	(sizeof(t1replay_checkpoint_boss_t) == 272) ? 1 : -1
];
typedef char t1replay_checkpoint_size_check[
	(sizeof(t1replay_checkpoint_t) == T1REPLAY_CHECKPOINT_SIZE) ? 1 : -1
];
typedef char t1replay_header_start_offset_check[
	(offsetof(t1replay_header_t, start) == 50) ? 1 : -1
];
typedef char t1replay_header_checksum_offset_check[
	(offsetof(t1replay_header_t, header_checksum) == 46) ? 1 : -1
];
typedef char t1replay_header_name_offset_check[
	(offsetof(t1replay_header_t, name) == 114) ? 1 : -1
];
typedef char t1replay_header_summary_offset_check[
	(offsetof(t1replay_header_t, summary) == 130) ? 1 : -1
];
typedef char t1replay_checkpoint_groups_offset_check[
	(offsetof(t1replay_checkpoint_t, groups) == T1REPLAY_CHECKPOINT_HEADER_SIZE) ? 1 : -1
];

#endif /* TH01_REPLAY_FORMAT_HPP */
