/* ReC98 replay mod -- TH02 compact native Story input stream.
 *
 * This translation unit deliberately owns a new trailing code segment. Its
 * state is all BSS, appended after every original contributor, so no original
 * TH02 data or BSS offset moves. The user file contract is in
 * th02/replay_format.hpp and mirrored by tools/replay/th02_user_replay.py.
 */

// This deliberately remains outside MAIN_01: that original group is already
// full. Tupfile.lua links this object after every game contributor and before
// the fixed C runtime tail, leaving every original data/BSS offset untouched.
#pragma option -zCT2REPLAY_TEXT -G-

#include "platform.h"
#include "libs/master.lib/master.hpp"
#include "th01/rank.h"
#include "th02/common.h"
#include "th02/replay_format.hpp"
#include "th02/resident.hpp"
#include "th02/core/globals.hpp"
#include "th02/hardware/input.hpp"
#include "th02/hardware/pages.hpp"
#include "th02/math/randring.hpp"
#include "th02/snd/snd.h"
#include "th02/main/frames.hpp"
#include "th02/main/main.hpp"
#include "th02/main/midboss/midboss.hpp"
#include "th02/main/playperf.hpp"
#include "th02/main/score.hpp"
#include "th02/main/scroll.hpp"
#include "th02/main/slowdown.hpp"
#include "th02/main/stage/stage.hpp"
#include "th02/main/player/player.hpp"
#include "th02/main/player/bomb.hpp"
#include "th02/main/tile/tile.hpp"

#define T2REPLAY_BUFFER_PACKET_COUNT 256
#define T2REPLAY_BUFFER_SIZE (T2REPLAY_BUFFER_PACKET_COUNT * T2REPLAY_PACKET_SIZE)
#define T2REPLAY_INPUT_KNOWN 0xF1FF
#define T2REPLAY_DOS_ACCESS_READ 0
#define T2REPLAY_DOS_ACCESS_RW 2
#define T2REPLAY_FP_SEG(p) ((unsigned)(((unsigned long)(void far *)(p)) >> 16))
#define T2REPLAY_FP_OFF(p) ((unsigned)((unsigned long)(void far *)(p)))

enum t2replay_mode_t {
	T2RM_DISABLED = 0,
	T2RM_RECORD = 1,
	T2RM_PLAYBACK = 2,
};

static char t2replay_command_fn[10];
static char t2replay_slot_fn[11];
static bool t2replay_paths_ready;
static t2replay_mode_t t2replay_mode;
static t2replay_header_t t2replay_header;
static t2replay_packet_t t2replay_buffer[T2REPLAY_BUFFER_PACKET_COUNT];
static uint16_t t2replay_buffer_len;
static uint16_t t2replay_buffer_pos;
static uint32_t t2replay_payload_written;
static uint32_t t2replay_packet_cursor;
static uint32_t t2replay_sample_cursor;
static uint32_t t2replay_payload_checksum;
static t2replay_packet_t t2replay_pending;
static uint8_t t2replay_pending_run;
static uint8_t t2replay_decode_run;
static bool t2replay_pending_valid;
static bool t2replay_failed;
static bool t2replay_finished;
static bool t2replay_playback_exit;
static bool t2replay_stage_seen;
static uint8_t t2replay_last_stage;

union t2replay_scroll_pages_t {
	uint32_t packed_initial_lines;
	int16_t line[2];
};

static t2replay_scroll_pages_t t2replay_scroll_pages;
static uint8_t t2replay_checkpoint_capture[T2REPLAY_CHECKPOINT_CAPTURE_SIZE];
static uint16_t t2replay_checkpoint_capture_size;
static bool t2replay_checkpoint_capture_is_valid;

// Owned by the native stage loader and enemy spawn VM. This narrow replay
// module reads them only while forming the capture-only checkpoint identity.
extern "C" int spawn_row_cur;
extern "C" uint8_t bgm_show_timer;
extern "C" uint8_t bgm_title_id;
extern "C" uint8_t boss_bgm_title_id;

static void t2replay_memclear(void far *buf, unsigned size)
{
	uint8_t far *p = reinterpret_cast<uint8_t far *>(buf);

	while(size != 0) {
		*p++ = 0;
		size--;
	}
}

static bool t2replay_bytes_zero(const uint8_t far *buf, unsigned size);
static uint32_t t2replay_fnv1a(uint32_t hash, const void far *buf, unsigned size);

#define T2RCK_HEADER_TOTAL_SIZE 0x10
#define T2RCK_HEADER_SOURCE_FINGERPRINT 0x14
#define T2RCK_HEADER_SEMANTIC_DIGEST 0x18
#define T2RCK_HEADER_DECODED_SIZE 0x1C
#define T2RCK_HEADER_CONTAINER_CHECKSUM 0x20
#define T2RCK_HEADER_GROUP_MASK 0x24

#define T2RCK_GROUP_ID 0
#define T2RCK_GROUP_SCHEMA 1
#define T2RCK_GROUP_CODEC 2
#define T2RCK_GROUP_FLAGS 3
#define T2RCK_GROUP_OFFSET 4
#define T2RCK_GROUP_STORED_SIZE 8
#define T2RCK_GROUP_DECODED_SIZE 12
#define T2RCK_GROUP_CHECKSUM 16

static uint16_t t2replay_checkpoint_get_u16(
	const uint8_t far *data, unsigned offset
)
{
	return static_cast<uint16_t>(
		static_cast<uint16_t>(data[offset + 0]) |
		(static_cast<uint16_t>(data[offset + 1]) << 8)
	);
}

static uint32_t t2replay_checkpoint_get_u32(
	const uint8_t far *data, unsigned offset
)
{
	return (
		static_cast<uint32_t>(data[offset + 0]) |
		(static_cast<uint32_t>(data[offset + 1]) << 8) |
		(static_cast<uint32_t>(data[offset + 2]) << 16) |
		(static_cast<uint32_t>(data[offset + 3]) << 24)
	);
}

static void t2replay_checkpoint_put_u16(
	uint8_t far *data, unsigned offset, uint16_t value
)
{
	data[offset + 0] = static_cast<uint8_t>(value);
	data[offset + 1] = static_cast<uint8_t>(value >> 8);
}

static void t2replay_checkpoint_put_u32(
	uint8_t far *data, unsigned offset, uint32_t value
)
{
	data[offset + 0] = static_cast<uint8_t>(value);
	data[offset + 1] = static_cast<uint8_t>(value >> 8);
	data[offset + 2] = static_cast<uint8_t>(value >> 16);
	data[offset + 3] = static_cast<uint8_t>(value >> 24);
}

static unsigned t2replay_checkpoint_group_offset(uint8_t id)
{
	return (
		T2REPLAY_CHECKPOINT_HEADER_SIZE +
		(static_cast<unsigned>(id) * T2REPLAY_CHECKPOINT_GROUP_SIZE)
	);
}

static unsigned t2replay_checkpoint_group_size(uint8_t id)
{
	switch(id) {
	case T2RCGI_IDENTITY:
		return T2REPLAY_CHECKPOINT_IDENTITY_SIZE;
	case T2RCGI_RNG:
		return T2REPLAY_CHECKPOINT_RNG_SIZE;
	case T2RCGI_RUN:
		return T2REPLAY_CHECKPOINT_RUN_SIZE;
	case T2RCGI_FIELD:
		return T2REPLAY_CHECKPOINT_FIELD_SIZE;
	case T2RCGI_STAGE_VM:
		return T2REPLAY_CHECKPOINT_STAGE_VM_SIZE;
	case T2RCGI_PACING:
		return T2REPLAY_CHECKPOINT_PACING_SIZE;
	default:
		return 0;
	}
}

static uint32_t t2replay_checkpoint_digest_u32(
	uint32_t digest, uint32_t value
)
{
	uint8_t bytes[4];

	t2replay_checkpoint_put_u32(bytes, 0, value);
	return t2replay_fnv1a(digest, bytes, sizeof(bytes));
}

static uint32_t t2replay_checkpoint_group_digest(
	uint32_t digest, uint8_t id, const uint8_t far *data, unsigned size
)
{
	digest = t2replay_fnv1a(digest, &id, sizeof(id));
	id = T2REPLAY_CHECKPOINT_GROUP_SCHEMA;
	digest = t2replay_fnv1a(digest, &id, sizeof(id));
	digest = t2replay_checkpoint_digest_u32(digest, size);
	return t2replay_fnv1a(digest, data, size);
}

static uint32_t t2replay_checkpoint_container_checksum(
	const uint8_t far *data, unsigned size
)
{
	uint32_t hash = T2REPLAY_FNV1A_BASIS;
	unsigned offset;
	uint8_t byte;

	for(offset = 0; offset < size; offset++) {
		byte = (
			(offset >= T2RCK_HEADER_CONTAINER_CHECKSUM) &&
			(offset < (T2RCK_HEADER_CONTAINER_CHECKSUM + 4))
		) ? 0 : data[offset];
		hash = t2replay_fnv1a(hash, &byte, sizeof(byte));
	}
	return hash;
}

static void t2replay_checkpoint_group_set(
	uint8_t far *container, uint8_t id, unsigned payload_offset,
	unsigned payload_size
)
{
	uint8_t far *group = (
		container + t2replay_checkpoint_group_offset(id)
	);

	group[T2RCK_GROUP_ID] = id;
	group[T2RCK_GROUP_SCHEMA] = T2REPLAY_CHECKPOINT_GROUP_SCHEMA;
	group[T2RCK_GROUP_CODEC] = T2RCC_RAW;
	group[T2RCK_GROUP_FLAGS] = 0;
	t2replay_checkpoint_put_u32(group, T2RCK_GROUP_OFFSET, payload_offset);
	t2replay_checkpoint_put_u32(group, T2RCK_GROUP_STORED_SIZE, payload_size);
	t2replay_checkpoint_put_u32(group, T2RCK_GROUP_DECODED_SIZE, payload_size);
	t2replay_checkpoint_put_u32(
		group, T2RCK_GROUP_CHECKSUM,
		t2replay_fnv1a(T2REPLAY_FNV1A_BASIS,
			container + payload_offset, payload_size)
	);
}

static bool t2replay_checkpoint_group_capture(
	uint8_t id, uint8_t far *data
)
{
	unsigned i;

	switch(id) {
	case T2RCGI_IDENTITY:
		data[0] = static_cast<uint8_t>(stage_id);
		data[1] = resident->shottype;
		data[2] = static_cast<uint8_t>(rank);
		data[3] = (reduce_effects ? 1 : 0);
		data[4] = T2REPLAY_INPUT_SEMANTICS_KEY_DET;
		data[5] = T2REPLAY_RULESET_STOCK;
		data[6] = 0;
		data[7] = 0;
		t2replay_checkpoint_put_u32(
			data, 8, T2REPLAY_CHECKPOINT_SOURCE_FINGERPRINT
		);
		return true;

	case T2RCGI_RNG:
		t2replay_checkpoint_put_u32(
			data, 0, static_cast<uint32_t>(random_seed)
		);
		for(i = 0; i < RANDRING_SIZE; i++) {
			data[4 + i] = randring[i];
		}
		data[260] = randring_p;
		data[261] = 0;
		data[262] = 0;
		data[263] = 0;
		return true;

	case T2RCGI_RUN:
		t2replay_checkpoint_put_u32(
			data, 0, static_cast<uint32_t>(resident->frame)
		);
		t2replay_checkpoint_put_u32(
			data, 4, static_cast<uint32_t>(resident->score)
		);
		t2replay_checkpoint_put_u32(data, 8, resident->score_highest);
		t2replay_checkpoint_put_u16(data, 12, resident->continues_used);
		t2replay_checkpoint_put_u16(
			data, 14, static_cast<uint16_t>(resident->skill)
		);
		data[16] = static_cast<uint8_t>(resident->rem_lives);
		data[17] = static_cast<uint8_t>(resident->rem_bombs);
		data[18] = resident->start_lives;
		data[19] = resident->start_bombs;
		data[20] = static_cast<uint8_t>(resident->start_power);
		data[21] = resident->bgm_mode;
		data[22] = static_cast<uint8_t>(resident->debug);
		data[23] = resident->op_main_retval;
		data[24] = resident->demo_num;
		t2replay_checkpoint_put_u32(data, 25, static_cast<uint32_t>(score));
		t2replay_checkpoint_put_u32(data, 29, static_cast<uint32_t>(hiscore));
		t2replay_checkpoint_put_u32(data, 33, static_cast<uint32_t>(score_delta));
		t2replay_checkpoint_put_u16(data, 37, extends_gained);
		data[39] = static_cast<uint8_t>(lives);
		data[40] = static_cast<uint8_t>(bombs);
		data[41] = power;
		data[42] = (quit ? 1 : 0);
		return true;

	case T2RCGI_FIELD:
		data[0] = static_cast<uint8_t>(page_front);
		data[1] = static_cast<uint8_t>(page_back);
		data[2] = (scroll_done ? 1 : 0);
		data[3] = static_cast<uint8_t>(tile_mode);
		t2replay_checkpoint_put_u16(
			data, 4, static_cast<uint16_t>(scroll_line)
		);
		data[6] = static_cast<uint8_t>(scroll_speed);
		data[7] = scroll_cycle;
		data[8] = scroll_interval;
		data[9] = static_cast<uint8_t>(scroll_delta);
		t2replay_checkpoint_put_u16(
			data, 10, static_cast<uint16_t>(scroll_step)
		);
		t2replay_checkpoint_put_u16(
			data, 12, static_cast<uint16_t>(scroll_step_advanced)
		);
		t2replay_checkpoint_put_u16(
			data, 14,
			static_cast<uint16_t>(t2replay_scroll_pages.line[0])
		);
		t2replay_checkpoint_put_u16(
			data, 16,
			static_cast<uint16_t>(t2replay_scroll_pages.line[1])
		);
		data[18] = (tiles_egc_render_all ? 1 : 0);
		data[19] = 0;
		return true;

	case T2RCGI_STAGE_VM:
		data[0] = static_cast<uint8_t>(stage_id);
		data[1] = static_cast<uint8_t>(stage_progression);
		data[2] = (midboss_active ? 1 : 0);
		data[3] = 0;
		t2replay_checkpoint_put_u16(
			data, 4, static_cast<uint16_t>(spawn_row_cur)
		);
		t2replay_checkpoint_put_u16(
			data, 6, static_cast<uint16_t>(midboss_scroll_step)
		);
		return true;

	case T2RCGI_PACING:
		t2replay_checkpoint_put_u32(data, 0, stage_frame);
		data[4] = slowdown_factor;
		t2replay_checkpoint_put_u16(
			data, 5, static_cast<uint16_t>(playperf)
		);
		data[7] = playperf_max;
		data[8] = bgm_show_timer;
		data[9] = bgm_title_id;
		data[10] = boss_bgm_title_id;
		data[11] = 0;
		return true;

	default:
		return false;
	}
}

static bool t2replay_checkpoint_group_payload_valid(
	uint8_t id, const uint8_t far *data
)
{
	int16_t value;

	switch(id) {
	case T2RCGI_IDENTITY:
		return (
			(data[0] < T2REPLAY_STAGE_COUNT) &&
			(data[1] < 3) &&
			(data[2] <= RANK_EXTRA) &&
			((data[0] == (T2REPLAY_STAGE_COUNT - 1)) ==
				(data[2] == RANK_EXTRA)) &&
			(data[3] <= 1) &&
			(data[4] == T2REPLAY_INPUT_SEMANTICS_KEY_DET) &&
			(data[5] == T2REPLAY_RULESET_STOCK) &&
			(data[6] == 0) && (data[7] == 0) &&
			(t2replay_checkpoint_get_u32(data, 8) ==
				T2REPLAY_CHECKPOINT_SOURCE_FINGERPRINT)
		);

	case T2RCGI_RNG:
		return (
			(data[261] == 0) && (data[262] == 0) && (data[263] == 0)
		);

	case T2RCGI_RUN:
		value = static_cast<int8_t>(data[16]);
		if((value < -1) || (value > 5)) {
			return false;
		}
		value = static_cast<int8_t>(data[17]);
		if((value < -1) || (value > 5)) {
			return false;
		}
		return (
			(data[18] <= 5) && (data[19] <= 5) &&
			(static_cast<int8_t>(data[20]) >= 0) &&
			(static_cast<int8_t>(data[20]) <= POWER_MAX) &&
			(data[21] <= SND_BGM_MIDI) && (data[22] <= 1) &&
			(data[24] <= 3) &&
			(static_cast<int8_t>(data[39]) >= -1) &&
			(static_cast<int8_t>(data[39]) <= 5) &&
			(static_cast<int8_t>(data[40]) >= -1) &&
			(static_cast<int8_t>(data[40]) <= 5) &&
			(data[41] <= POWER_MAX) && (data[42] <= 1)
		);

	case T2RCGI_FIELD:
		value = static_cast<int16_t>(
			t2replay_checkpoint_get_u16(data, 4)
		);
		if((value < 0) || (value >= RES_Y)) {
			return false;
		}
		value = static_cast<int16_t>(
			t2replay_checkpoint_get_u16(data, 10)
		);
		if(value < 0) {
			return false;
		}
		value = static_cast<int16_t>(
			t2replay_checkpoint_get_u16(data, 14)
		);
		if((value < 0) || (value >= RES_Y)) {
			return false;
		}
		value = static_cast<int16_t>(
			t2replay_checkpoint_get_u16(data, 16)
		);
		return (
			(data[0] <= 1) && (data[1] <= 1) && (data[0] != data[1]) &&
			(data[2] <= 1) && (data[3] <= TM_NONE) &&
			(data[8] != 0) && (data[12] <= 1) && (data[13] == 0) &&
			(value >= 0) && (value < RES_Y) &&
			(data[18] <= 1) && (data[19] == 0)
		);

	case T2RCGI_STAGE_VM:
		value = static_cast<int16_t>(
			t2replay_checkpoint_get_u16(data, 4)
		);
		if(value < 0) {
			return false;
		}
		value = static_cast<int16_t>(
			t2replay_checkpoint_get_u16(data, 6)
		);
		return (
			(data[0] < T2REPLAY_STAGE_COUNT) &&
			(data[1] <= SP_CLEAR) && (data[2] <= 1) && (data[3] == 0) &&
			(value >= -1)
		);

	case T2RCGI_PACING:
		value = static_cast<int16_t>(
			t2replay_checkpoint_get_u16(data, 5)
		);
		return (
			(data[4] != 0) &&
			(value >= playperf_min) && (value <= 16) &&
			(data[7] <= 16) && (value <= data[7]) &&
			(data[8] <= 160) && (data[9] < 12) && (data[10] < 12) &&
			(data[11] == 0)
		);

	default:
		return false;
	}
}

static bool t2replay_checkpoint_valid(
	const uint8_t far *container, unsigned total_size
)
{
	uint32_t digest = T2REPLAY_FNV1A_BASIS;
	uint32_t decoded_size = 0;
	unsigned payload_offset;
	unsigned group_offset;
	unsigned group_size;
	uint8_t group_id;

	if(
		(container == 0) ||
		(total_size != T2REPLAY_CHECKPOINT_CAPTURE_SIZE) ||
		(container[0] != 'T') || (container[1] != '2') ||
		(container[2] != 'C') || (container[3] != 'K') ||
		(container[4] != 'P') || (container[5] != '1') ||
		(container[6] != '\0') || (container[7] != '\0') ||
		(t2replay_checkpoint_get_u16(container, 8) !=
			T2REPLAY_CHECKPOINT_SCHEMA) ||
		(t2replay_checkpoint_get_u16(container, 10) !=
			T2REPLAY_CHECKPOINT_HEADER_SIZE) ||
		(container[12] != 2) ||
		(container[13] != T2REPLAY_CHECKPOINT_GROUP_COUNT) ||
		(t2replay_checkpoint_get_u16(container, 14) != 0) ||
		(t2replay_checkpoint_get_u32(container, T2RCK_HEADER_TOTAL_SIZE) !=
			T2REPLAY_CHECKPOINT_CAPTURE_SIZE) ||
		(t2replay_checkpoint_get_u32(
			container, T2RCK_HEADER_SOURCE_FINGERPRINT
		) != T2REPLAY_CHECKPOINT_SOURCE_FINGERPRINT) ||
		(t2replay_checkpoint_get_u32(container, T2RCK_HEADER_DECODED_SIZE) !=
			(T2REPLAY_CHECKPOINT_CAPTURE_SIZE -
				T2REPLAY_CHECKPOINT_HEADER_SIZE -
				(T2REPLAY_CHECKPOINT_GROUP_COUNT *
					T2REPLAY_CHECKPOINT_GROUP_SIZE))) ||
		(t2replay_checkpoint_get_u32(container, T2RCK_HEADER_GROUP_MASK) !=
			T2REPLAY_CHECKPOINT_GROUP_MASK)
	) {
		return false;
	}
	payload_offset = (
		T2REPLAY_CHECKPOINT_HEADER_SIZE +
		(T2REPLAY_CHECKPOINT_GROUP_COUNT * T2REPLAY_CHECKPOINT_GROUP_SIZE)
	);
	for(group_id = 0; group_id < T2REPLAY_CHECKPOINT_GROUP_COUNT; group_id++) {
		group_offset = t2replay_checkpoint_group_offset(group_id);
		group_size = t2replay_checkpoint_group_size(group_id);
		if(
			(group_size == 0) ||
			(container[group_offset + T2RCK_GROUP_ID] != group_id) ||
			(container[group_offset + T2RCK_GROUP_SCHEMA] !=
				T2REPLAY_CHECKPOINT_GROUP_SCHEMA) ||
			(container[group_offset + T2RCK_GROUP_CODEC] != T2RCC_RAW) ||
			(container[group_offset + T2RCK_GROUP_FLAGS] != 0) ||
			(t2replay_checkpoint_get_u32(
				container, group_offset + T2RCK_GROUP_OFFSET
			) != payload_offset) ||
			(t2replay_checkpoint_get_u32(
				container, group_offset + T2RCK_GROUP_STORED_SIZE
			) != group_size) ||
			(t2replay_checkpoint_get_u32(
				container, group_offset + T2RCK_GROUP_DECODED_SIZE
			) != group_size) ||
			(t2replay_checkpoint_get_u32(
				container, group_offset + T2RCK_GROUP_CHECKSUM
			) != t2replay_fnv1a(
				T2REPLAY_FNV1A_BASIS,
				container + payload_offset, group_size
			)) ||
			!t2replay_checkpoint_group_payload_valid(
				group_id, container + payload_offset
			)
		) {
			return false;
		}
		digest = t2replay_checkpoint_group_digest(
			digest, group_id, container + payload_offset, group_size
		);
		payload_offset += group_size;
		decoded_size += group_size;
	}
	return (
		(payload_offset == total_size) &&
		(decoded_size == t2replay_checkpoint_get_u32(
			container, T2RCK_HEADER_DECODED_SIZE
		)) &&
		(digest == t2replay_checkpoint_get_u32(
			container, T2RCK_HEADER_SEMANTIC_DIGEST
		)) &&
		(t2replay_checkpoint_container_checksum(container, total_size) ==
			t2replay_checkpoint_get_u32(
				container, T2RCK_HEADER_CONTAINER_CHECKSUM
			))
	);
}

void replay_scroll_pages_reset(long packed_initial_lines)
{
	t2replay_scroll_pages.packed_initial_lines =
		static_cast<uint32_t>(packed_initial_lines);
}

int16_t replay_scroll_page_line_get(uint8_t page)
{
	return t2replay_scroll_pages.line[page];
}

void replay_scroll_page_line_set(uint8_t page, int16_t line)
{
	t2replay_scroll_pages.line[page] = line;
}

void replay_checkpoint_capture_validate(void)
{
	uint8_t far *checkpoint = t2replay_checkpoint_capture;
	uint32_t state_digest = T2REPLAY_FNV1A_BASIS;
	unsigned payload_offset;
	unsigned group_size;
	uint8_t group_id;

	if(t2replay_mode == T2RM_DISABLED) {
		return;
	}
	t2replay_checkpoint_capture_is_valid = false;
	t2replay_checkpoint_capture_size = 0;
	t2replay_memclear(checkpoint, T2REPLAY_CHECKPOINT_CAPTURE_SIZE);
	checkpoint[0] = 'T'; checkpoint[1] = '2'; checkpoint[2] = 'C';
	checkpoint[3] = 'K'; checkpoint[4] = 'P'; checkpoint[5] = '1';
	t2replay_checkpoint_put_u16(checkpoint, 8, T2REPLAY_CHECKPOINT_SCHEMA);
	t2replay_checkpoint_put_u16(
		checkpoint, 10, T2REPLAY_CHECKPOINT_HEADER_SIZE
	);
	checkpoint[12] = 2;
	checkpoint[13] = T2REPLAY_CHECKPOINT_GROUP_COUNT;
	t2replay_checkpoint_put_u32(
		checkpoint, T2RCK_HEADER_TOTAL_SIZE,
		T2REPLAY_CHECKPOINT_CAPTURE_SIZE
	);
	t2replay_checkpoint_put_u32(
		checkpoint, T2RCK_HEADER_SOURCE_FINGERPRINT,
		T2REPLAY_CHECKPOINT_SOURCE_FINGERPRINT
	);
	t2replay_checkpoint_put_u32(
		checkpoint, T2RCK_HEADER_DECODED_SIZE,
		(T2REPLAY_CHECKPOINT_CAPTURE_SIZE -
			T2REPLAY_CHECKPOINT_HEADER_SIZE -
			(T2REPLAY_CHECKPOINT_GROUP_COUNT *
				T2REPLAY_CHECKPOINT_GROUP_SIZE))
	);
	t2replay_checkpoint_put_u32(
		checkpoint, T2RCK_HEADER_GROUP_MASK, T2REPLAY_CHECKPOINT_GROUP_MASK
	);
	payload_offset = (
		T2REPLAY_CHECKPOINT_HEADER_SIZE +
		(T2REPLAY_CHECKPOINT_GROUP_COUNT * T2REPLAY_CHECKPOINT_GROUP_SIZE)
	);
	for(group_id = 0; group_id < T2REPLAY_CHECKPOINT_GROUP_COUNT; group_id++) {
		group_size = t2replay_checkpoint_group_size(group_id);
		if(
			(group_size == 0) ||
			!t2replay_checkpoint_group_capture(
				group_id, checkpoint + payload_offset
			)
		) {
			return;
		}
		t2replay_checkpoint_group_set(
			checkpoint, group_id, payload_offset, group_size
		);
		state_digest = t2replay_checkpoint_group_digest(
			state_digest, group_id, checkpoint + payload_offset, group_size
		);
		payload_offset += group_size;
	}
	if(payload_offset != T2REPLAY_CHECKPOINT_CAPTURE_SIZE) {
		return;
	}
	t2replay_checkpoint_put_u32(
		checkpoint, T2RCK_HEADER_SEMANTIC_DIGEST, state_digest
	);
	t2replay_checkpoint_put_u32(
		checkpoint, T2RCK_HEADER_CONTAINER_CHECKSUM,
		t2replay_checkpoint_container_checksum(
			checkpoint, T2REPLAY_CHECKPOINT_CAPTURE_SIZE
		)
	);
	t2replay_checkpoint_capture_size = T2REPLAY_CHECKPOINT_CAPTURE_SIZE;
	t2replay_checkpoint_capture_is_valid = t2replay_checkpoint_valid(
		checkpoint, t2replay_checkpoint_capture_size
	);
}

static void t2replay_paths_init(void)
{
	if(t2replay_paths_ready) {
		return;
	}
	t2replay_command_fn[0] = 'T';
	t2replay_command_fn[1] = '2';
	t2replay_command_fn[2] = 'R';
	t2replay_command_fn[3] = 'P';
	t2replay_command_fn[4] = 'Y';
	t2replay_command_fn[5] = '.';
	t2replay_command_fn[6] = 'C';
	t2replay_command_fn[7] = 'F';
	t2replay_command_fn[8] = 'G';
	t2replay_command_fn[9] = '\0';
	t2replay_slot_fn[0] = 'T';
	t2replay_slot_fn[1] = 'H';
	t2replay_slot_fn[2] = '2';
	t2replay_slot_fn[3] = 'R';
	t2replay_slot_fn[4] = '0';
	t2replay_slot_fn[5] = '0';
	t2replay_slot_fn[6] = '.';
	t2replay_slot_fn[7] = 'R';
	t2replay_slot_fn[8] = 'P';
	t2replay_slot_fn[9] = 'Y';
	t2replay_slot_fn[10] = '\0';
	t2replay_paths_ready = true;
}

static void t2replay_slot_set(uint8_t slot)
{
	t2replay_slot_fn[4] = static_cast<char>('0' + (slot / 10));
	t2replay_slot_fn[5] = static_cast<char>('0' + (slot % 10));
}

static int t2replay_dos_open(const char far *fn, unsigned char access)
{
	unsigned fn_seg = T2REPLAY_FP_SEG(fn);
	unsigned fn_off = T2REPLAY_FP_OFF(fn);
	int result;

	_asm {
		push	ds
		mov	dx, fn_off
		mov	ds, fn_seg
		mov	ah, 3Dh
		mov	al, access
		int	21h
		pop	ds
		sbb	dx, dx
		or	ax, dx
		mov	result, ax
	}
	return result;
}

static int t2replay_dos_create(const char far *fn)
{
	unsigned fn_seg = T2REPLAY_FP_SEG(fn);
	unsigned fn_off = T2REPLAY_FP_OFF(fn);
	int result;

	_asm {
		push	ds
		mov	dx, fn_off
		mov	ds, fn_seg
		mov	ah, 3Ch
		xor	cx, cx
		int	21h
		pop	ds
		sbb	dx, dx
		or	ax, dx
		mov	result, ax
	}
	return result;
}

static void t2replay_dos_close(int fh)
{
	_asm {
		mov	bx, fh
		mov	ah, 3Eh
		int	21h
	}
}

static bool t2replay_dos_seek(int fh, uint32_t offset)
{
	unsigned offset_hi = static_cast<unsigned>(offset >> 16);
	unsigned offset_lo = static_cast<unsigned>(offset & 0xFFFFUL);
	unsigned failed;

	_asm {
		mov	bx, fh
		mov	cx, offset_hi
		mov	dx, offset_lo
		mov	ax, 4200h
		int	21h
		sbb	ax, ax
		neg	ax
		mov	failed, ax
	}
	return (failed == 0);
}

static bool t2replay_dos_size(int fh, uint32_t far *size)
{
	unsigned size_hi;
	unsigned size_lo;
	unsigned failed;

	_asm {
		mov	bx, fh
		xor	cx, cx
		xor	dx, dx
		mov	ax, 4202h
		int	21h
		mov	size_lo, ax
		mov	size_hi, dx
		sbb	ax, ax
		neg	ax
		mov	failed, ax
	}
	*size = (
		(static_cast<uint32_t>(size_hi) << 16) |
		static_cast<uint32_t>(size_lo)
	);
	return (failed == 0);
}

static unsigned t2replay_dos_read(int fh, void far *buf, unsigned size)
{
	unsigned buf_seg = T2REPLAY_FP_SEG(buf);
	unsigned buf_off = T2REPLAY_FP_OFF(buf);
	unsigned result;

	_asm {
		push	ds
		mov	bx, fh
		mov	cx, size
		mov	dx, buf_off
		mov	ds, buf_seg
		mov	ah, 3Fh
		int	21h
		pop	ds
		sbb	cx, cx
		not	cx
		and	ax, cx
		mov	result, ax
	}
	return result;
}

static unsigned t2replay_dos_write(int fh, const void far *buf, unsigned size)
{
	unsigned buf_seg = T2REPLAY_FP_SEG(buf);
	unsigned buf_off = T2REPLAY_FP_OFF(buf);
	unsigned result;

	_asm {
		push	ds
		mov	bx, fh
		mov	cx, size
		mov	dx, buf_off
		mov	ds, buf_seg
		mov	ah, 40h
		int	21h
		pop	ds
		sbb	cx, cx
		not	cx
		and	ax, cx
		mov	result, ax
	}
	return result;
}

static void t2replay_dos_delete(const char far *fn)
{
	unsigned fn_seg = T2REPLAY_FP_SEG(fn);
	unsigned fn_off = T2REPLAY_FP_OFF(fn);

	_asm {
		push	ds
		mov	dx, fn_off
		mov	ds, fn_seg
		mov	ah, 41h
		int	21h
		pop	ds
	}
}

static uint32_t t2replay_fnv1a(
	uint32_t hash, const void far *buf, unsigned size
)
{
	const uint8_t far *p = reinterpret_cast<const uint8_t far *>(buf);

	while(size != 0) {
		hash ^= static_cast<uint32_t>(*p++);
		hash *= T2REPLAY_FNV1A_PRIME;
		size--;
	}
	return hash;
}

static bool t2replay_bytes_zero(const uint8_t far *p, unsigned size)
{
	while(size != 0) {
		if(*p++ != 0) {
			return false;
		}
		size--;
	}
	return true;
}

static bool t2replay_magic_matches(const char far *magic, char last)
{
	return (
		(magic[0] == 'T') &&
		(magic[1] == '2') &&
		(magic[2] == 'R') &&
		(magic[3] == 'P') &&
		(magic[4] == 'Y') &&
		(magic[5] == last) &&
		(magic[6] == '\0') &&
		(magic[7] == '\0')
	);
}

static bool t2replay_command_magic_matches(const char far *magic)
{
	return (
		(magic[0] == 'T') &&
		(magic[1] == '2') &&
		(magic[2] == 'R') &&
		(magic[3] == 'C') &&
		(magic[4] == 'F') &&
		(magic[5] == 'G') &&
		(magic[6] == '2') &&
		(magic[7] == '\0')
	);
}

static void t2replay_header_checksum_set(void)
{
	t2replay_header.header_checksum = 0;
	t2replay_header.header_checksum = t2replay_fnv1a(
		T2REPLAY_FNV1A_BASIS, &t2replay_header, sizeof(t2replay_header)
	);
}

static bool t2replay_header_write(bool create)
{
	int fd = (create
		? t2replay_dos_create(t2replay_slot_fn)
		: t2replay_dos_open(t2replay_slot_fn, T2REPLAY_DOS_ACCESS_RW)
	);

	if(fd < 0) {
		return false;
	}
	t2replay_header.payload_checksum = t2replay_payload_checksum;
	t2replay_header_checksum_set();
	if(!t2replay_dos_seek(fd, 0) ||
		(t2replay_dos_write(fd, &t2replay_header, sizeof(t2replay_header)) !=
		 sizeof(t2replay_header))) {
		t2replay_dos_close(fd);
		return false;
	}
	t2replay_dos_close(fd);
	return true;
}

static bool t2replay_buffer_flush(void)
{
	unsigned len;
	int fd;

	if(t2replay_buffer_len == 0) {
		return true;
	}
	len = (t2replay_buffer_len * T2REPLAY_PACKET_SIZE);
	fd = t2replay_dos_open(t2replay_slot_fn, T2REPLAY_DOS_ACCESS_RW);
	if(fd < 0) {
		return false;
	}
	if(!t2replay_dos_seek(fd, t2replay_header.input_offset + t2replay_payload_written) ||
		(t2replay_dos_write(fd, t2replay_buffer, len) != len)) {
		t2replay_dos_close(fd);
		return false;
	}
	t2replay_dos_close(fd);
	t2replay_payload_written += len;
	t2replay_buffer_len = 0;
	return t2replay_header_write(false);
}

static bool t2replay_packet_commit(const t2replay_packet_t far *packet)
{
	t2replay_buffer[t2replay_buffer_len] = *packet;
	t2replay_buffer_len++;
	t2replay_header.packet_count++;
	t2replay_header.input_size += T2REPLAY_PACKET_SIZE;
	t2replay_payload_checksum = t2replay_fnv1a(
		t2replay_payload_checksum, packet, T2REPLAY_PACKET_SIZE
	);
	if(t2replay_buffer_len >= T2REPLAY_BUFFER_PACKET_COUNT) {
		return t2replay_buffer_flush();
	}
	return true;
}

static bool t2replay_pending_commit(void)
{
	if(!t2replay_pending_valid) {
		return true;
	}
	t2replay_pending.tag = static_cast<uint8_t>(
		(t2replay_pending.tag & 0xC0) | (t2replay_pending_run - 1)
	);
	t2replay_header.sample_count += t2replay_pending_run;
	if(!t2replay_packet_commit(&t2replay_pending)) {
		return false;
	}
	t2replay_pending_valid = false;
	t2replay_pending_run = 0;
	return true;
}

static bool t2replay_record_sample(uint8_t phase)
{
	uint8_t low = static_cast<uint8_t>(key_det & 0xFF);
	uint8_t high = static_cast<uint8_t>(key_det >> 8);

	if(
		t2replay_pending_valid &&
		((t2replay_pending.tag >> T2REPLAY_PACKET_PHASE_SHIFT) == phase) &&
		(t2replay_pending.input_low == low) &&
		(t2replay_pending.input_high == high) &&
		(t2replay_pending_run < T2REPLAY_PACKET_RUN_MAX)
	) {
		t2replay_pending_run++;
		return true;
	}
	if(!t2replay_pending_commit()) {
		return false;
	}
	t2replay_pending.tag = static_cast<uint8_t>(
		phase << T2REPLAY_PACKET_PHASE_SHIFT
	);
	t2replay_pending.input_low = low;
	t2replay_pending.input_high = high;
	t2replay_pending.arg = 0;
	t2replay_pending_run = 1;
	t2replay_pending_valid = true;
	return true;
}

static bool t2replay_record_control(uint8_t opcode, uint16_t value, uint8_t arg)
{
	t2replay_packet_t packet;

	if(!t2replay_pending_commit()) {
		return false;
	}
	packet.tag = static_cast<uint8_t>(
		(T2REPLAY_PHASE_CONTROL << T2REPLAY_PACKET_PHASE_SHIFT) | opcode
	);
	packet.input_low = static_cast<uint8_t>(value & 0xFF);
	packet.input_high = static_cast<uint8_t>(value >> 8);
	packet.arg = arg;
	return t2replay_packet_commit(&packet);
}

static bool t2replay_start_valid(const t2replay_start_t far *start)
{
	if(
		(start->stage < 0) ||
		(start->stage >= T2REPLAY_STAGE_COUNT) ||
		(start->rank > RANK_EXTRA) ||
		((start->stage == (T2REPLAY_STAGE_COUNT - 1)) !=
		 (start->rank == RANK_EXTRA)) ||
		(start->rem_lives < 0) ||
		(start->rem_lives > 5) ||
		(start->rem_bombs < 0) ||
		(start->rem_bombs > 5) ||
		(start->start_lives < 1) ||
		(start->start_lives > 5) ||
		(start->start_bombs < 1) ||
		(start->start_bombs > 5) ||
		(start->start_power < 0) ||
		(start->start_power > 80) ||
		(start->random_seed != start->resident_frame) ||
		(start->shottype >= SHOTTYPE_COUNT) ||
		(start->bgm_mode > SND_BGM_MIDI) ||
		(start->reduce_effects > 1) ||
		(start->debug != 0) ||
		!t2replay_bytes_zero(start->reserved, sizeof(start->reserved))
	) {
		return false;
	}
	return true;
}

static bool t2replay_practice_start_valid(const t2replay_start_t far *start)
{
	// A stored zero is native: cfg_load() maps it to the first live power unit.
	return (
		t2replay_start_valid(start) &&
		(start->score >= 0) &&
		(start->score_highest >= static_cast<uint32_t>(start->score)) &&
		(start->continues_used == 0) &&
		(start->rem_lives == static_cast<int8_t>(start->start_lives)) &&
		(start->rem_bombs == static_cast<int8_t>(start->start_bombs))
	);
}

static bool t2replay_stage_scores_valid(void)
{
	uint8_t first_stage = static_cast<uint8_t>(t2replay_header.start.stage);
	uint8_t stage;

	for(stage = 0; stage < T2REPLAY_STAGE_COUNT; stage++) {
		if(
			((stage < first_stage) || (stage > t2replay_header.stage_reached)) &&
			(t2replay_header.stage_scores[stage] != 0)
		) {
			return false;
		}
	}
	return true;
}

static bool t2replay_packet_is_valid(
	const t2replay_packet_t far *packet, uint32_t far *samples,
	bool far *terminal_seen
)
{
	uint8_t phase = static_cast<uint8_t>(
		packet->tag >> T2REPLAY_PACKET_PHASE_SHIFT
	);
	uint8_t low = static_cast<uint8_t>(packet->tag & T2REPLAY_PACKET_RUN_MASK);
	input_t input;

	if(*terminal_seen) {
		return false;
	}
	if(phase < T2REPLAY_PHASE_CONTROL) {
		input = static_cast<input_t>(
			packet->input_low | (static_cast<uint16_t>(packet->input_high) << 8)
		);
		if((packet->arg != 0) || (input & ~T2REPLAY_INPUT_KNOWN)) {
			return false;
		}
		*samples += static_cast<uint32_t>(low + 1);
		return (*samples >= static_cast<uint32_t>(low + 1));
	}
	if(phase != T2REPLAY_PHASE_CONTROL) {
		return false;
	}
	if((low == T2REPLAY_CONTROL_STAGE_START) && (packet->arg == 0)) {
		return (
			(packet->input_high == 0) &&
			(packet->input_low < T2REPLAY_STAGE_COUNT) &&
			!(*terminal_seen)
		);
	}
	if(low == T2REPLAY_CONTROL_TERMINAL) {
		if(
			(packet->input_high != 0) ||
			((packet->input_low != T2REPLAY_END_GAME_OVER) &&
			 (packet->input_low != T2REPLAY_END_CLEAR)) ||
			(packet->arg >= T2REPLAY_STAGE_COUNT) ||
			*terminal_seen
		) {
			return false;
		}
		*terminal_seen = true;
		return true;
	}
	return false;
}

static bool t2replay_payload_validate(int fd, uint32_t file_size)
{
	uint32_t hash = T2REPLAY_FNV1A_BASIS;
	uint32_t samples = 0;
	uint32_t packets_seen = 0;
	unsigned want;
	unsigned len;
	unsigned i;
	bool terminal_seen = false;
	bool stage_seen = false;
	uint8_t expected_stage = static_cast<uint8_t>(t2replay_header.start.stage);
	uint8_t terminal_reason = 0;
	uint8_t terminal_stage = 0;

	if(file_size != (t2replay_header.input_offset + t2replay_header.input_size)) {
		return false;
	}
	if(!t2replay_dos_seek(fd, t2replay_header.input_offset)) {
		return false;
	}
	while(packets_seen < t2replay_header.packet_count) {
		want = static_cast<unsigned>(
			((t2replay_header.packet_count - packets_seen) >
			 T2REPLAY_BUFFER_PACKET_COUNT)
				? T2REPLAY_BUFFER_PACKET_COUNT
				: (t2replay_header.packet_count - packets_seen)
		);
		len = (want * T2REPLAY_PACKET_SIZE);
		if(t2replay_dos_read(fd, t2replay_buffer, len) != len) {
			return false;
		}
		hash = t2replay_fnv1a(hash, t2replay_buffer, len);
		for(i = 0; i < want; i++) {
			if(!t2replay_packet_is_valid(
				&t2replay_buffer[i], &samples, &terminal_seen
			)) {
				return false;
			}
			if((t2replay_buffer[i].tag == static_cast<uint8_t>(
				(T2REPLAY_PHASE_CONTROL << T2REPLAY_PACKET_PHASE_SHIFT) |
				T2REPLAY_CONTROL_TERMINAL
			))) {
				if(
					!stage_seen ||
					(t2replay_buffer[i].arg != (expected_stage - 1))
				) {
					return false;
				}
				terminal_reason = t2replay_buffer[i].input_low;
				terminal_stage = t2replay_buffer[i].arg;
			} else if((t2replay_buffer[i].tag == static_cast<uint8_t>(
				(T2REPLAY_PHASE_CONTROL << T2REPLAY_PACKET_PHASE_SHIFT) |
				T2REPLAY_CONTROL_STAGE_START
			))) {
				if(
					(expected_stage >= T2REPLAY_STAGE_COUNT) ||
					(t2replay_buffer[i].input_low != expected_stage)
				) {
					return false;
				}
				stage_seen = true;
				expected_stage++;
			}
		}
		packets_seen += want;
	}
	return (
		(hash == t2replay_header.payload_checksum) &&
		(samples == t2replay_header.sample_count) &&
		terminal_seen &&
		(t2replay_header.stage_reached == (expected_stage - 1)) &&
		(terminal_reason == t2replay_header.end_reason) &&
		(terminal_stage == t2replay_header.terminal_stage)
	);
}

static bool t2replay_header_read(void)
{
	uint32_t file_size;
	uint32_t stored_checksum;
	uint32_t computed_checksum;
	int fd;

	fd = t2replay_dos_open(t2replay_slot_fn, T2REPLAY_DOS_ACCESS_READ);
	if(fd < 0) {
		return false;
	}
	if((t2replay_dos_read(fd, &t2replay_header, sizeof(t2replay_header)) !=
		 sizeof(t2replay_header)) || !t2replay_dos_size(fd, &file_size)) {
		t2replay_dos_close(fd);
		return false;
	}
	stored_checksum = t2replay_header.header_checksum;
	t2replay_header.header_checksum = 0;
	computed_checksum = t2replay_fnv1a(
		T2REPLAY_FNV1A_BASIS, &t2replay_header, sizeof(t2replay_header)
	);
	t2replay_header.header_checksum = stored_checksum;
	if(
		!t2replay_magic_matches(t2replay_header.magic, '1') ||
		(t2replay_header.version != T2REPLAY_VERSION) ||
		(t2replay_header.header_size != T2REPLAY_HEADER_SIZE) ||
		(t2replay_header.packet_size != T2REPLAY_PACKET_SIZE) ||
		(t2replay_header.flags != T2REPLAY_KNOWN_FLAGS) ||
		(t2replay_header.status != T2REPLAY_STATUS_FINALIZED) ||
		(t2replay_header.game_id != 2) ||
		(t2replay_header.ruleset != T2REPLAY_RULESET_STOCK) ||
		(t2replay_header.input_semantics != T2REPLAY_INPUT_SEMANTICS_KEY_DET) ||
		(t2replay_header.stage_count != T2REPLAY_STAGE_COUNT) ||
		(t2replay_header.stage_reached >= T2REPLAY_STAGE_COUNT) ||
		(t2replay_header.terminal_stage >= T2REPLAY_STAGE_COUNT) ||
		(t2replay_header.end_reason < T2REPLAY_END_GAME_OVER) ||
		(t2replay_header.end_reason > T2REPLAY_END_CLEAR) ||
		(t2replay_header.input_offset != T2REPLAY_HEADER_SIZE) ||
		(t2replay_header.input_size > T2REPLAY_INPUT_SIZE_MAX) ||
		(t2replay_header.packet_count >
		 (T2REPLAY_INPUT_SIZE_MAX / T2REPLAY_PACKET_SIZE)) ||
		(t2replay_header.input_size !=
		 (t2replay_header.packet_count * T2REPLAY_PACKET_SIZE)) ||
		(stored_checksum != computed_checksum) ||
		!t2replay_start_valid(&t2replay_header.start) ||
		!t2replay_stage_scores_valid() ||
		!t2replay_bytes_zero(t2replay_header.reserved, sizeof(t2replay_header.reserved))
	) {
		t2replay_dos_close(fd);
		return false;
	}
	if(!t2replay_payload_validate(fd, file_size)) {
		t2replay_dos_close(fd);
		return false;
	}
	t2replay_dos_close(fd);
	t2replay_buffer_len = 0;
	t2replay_buffer_pos = 0;
	return true;
}

static bool t2replay_packet_read(t2replay_packet_t far *packet)
{
	uint32_t remaining;
	unsigned want;
	unsigned len;
	int fd;

	if(t2replay_packet_cursor >= t2replay_header.packet_count) {
		return false;
	}
	if(t2replay_buffer_pos >= t2replay_buffer_len) {
		remaining = (t2replay_header.packet_count - t2replay_packet_cursor);
		want = static_cast<unsigned>(
			(remaining > T2REPLAY_BUFFER_PACKET_COUNT)
				? T2REPLAY_BUFFER_PACKET_COUNT : remaining
		);
		len = (want * T2REPLAY_PACKET_SIZE);
		fd = t2replay_dos_open(t2replay_slot_fn, T2REPLAY_DOS_ACCESS_READ);
		if(fd < 0) {
			return false;
		}
		if(!t2replay_dos_seek(fd, t2replay_header.input_offset +
			(t2replay_packet_cursor * T2REPLAY_PACKET_SIZE)) ||
			(t2replay_dos_read(fd, t2replay_buffer, len) != len)) {
			t2replay_dos_close(fd);
			return false;
		}
		t2replay_dos_close(fd);
		t2replay_buffer_len = want;
		t2replay_buffer_pos = 0;
	}
	*packet = t2replay_buffer[t2replay_buffer_pos++];
	t2replay_packet_cursor++;
	return true;
}

static bool t2replay_playback_sample(uint8_t phase)
{
	input_t input;

	if(t2replay_decode_run == 0) {
		if(!t2replay_packet_read(&t2replay_pending) ||
			((t2replay_pending.tag >> T2REPLAY_PACKET_PHASE_SHIFT) != phase) ||
			(t2replay_pending.arg != 0)) {
			return false;
		}
		t2replay_decode_run = static_cast<uint8_t>(
			(t2replay_pending.tag & T2REPLAY_PACKET_RUN_MASK) + 1
		);
	}
	input = static_cast<input_t>(
		t2replay_pending.input_low |
		(static_cast<uint16_t>(t2replay_pending.input_high) << 8)
	);
	if(input & ~T2REPLAY_INPUT_KNOWN) {
		return false;
	}
	key_det = input;
	t2replay_decode_run--;
	t2replay_sample_cursor++;
	return true;
}

static bool t2replay_playback_control(uint8_t opcode, uint16_t value, uint8_t arg)
{
	t2replay_packet_t packet;

	if((t2replay_decode_run != 0) || !t2replay_packet_read(&packet)) {
		return false;
	}
	return (
		(packet.tag == static_cast<uint8_t>(
			(T2REPLAY_PHASE_CONTROL << T2REPLAY_PACKET_PHASE_SHIFT) | opcode
		)) &&
		(packet.input_low == static_cast<uint8_t>(value & 0xFF)) &&
		(packet.input_high == static_cast<uint8_t>(value >> 8)) &&
		(packet.arg == arg)
	);
}

static bool t2replay_stage_score_matches(uint8_t stage)
{
	return (
		(stage < T2REPLAY_STAGE_COUNT) &&
		(static_cast<uint32_t>(score) == t2replay_header.stage_scores[stage])
	);
}

static bool t2replay_terminal_state_matches(void)
{
	return (
		t2replay_stage_score_matches(t2replay_header.terminal_stage) &&
		(score == t2replay_header.score_final) &&
		(lives == t2replay_header.lives_final) &&
		(bombs == t2replay_header.bombs_final) &&
		(power == t2replay_header.power_final) &&
		(stage_id == static_cast<char>(t2replay_header.terminal_stage))
	);
}

static void t2replay_fail(void)
{
	t2replay_failed = true;
	key_det = INPUT_NONE;
	quit = true;
}

static void t2replay_header_capture(void)
{
	t2replay_memclear(&t2replay_header, sizeof(t2replay_header));
	t2replay_header.magic[0] = 'T';
	t2replay_header.magic[1] = '2';
	t2replay_header.magic[2] = 'R';
	t2replay_header.magic[3] = 'P';
	t2replay_header.magic[4] = 'Y';
	t2replay_header.magic[5] = '1';
	t2replay_header.version = T2REPLAY_VERSION;
	t2replay_header.header_size = T2REPLAY_HEADER_SIZE;
	t2replay_header.packet_size = T2REPLAY_PACKET_SIZE;
	t2replay_header.flags = T2REPLAY_KNOWN_FLAGS;
	t2replay_header.status = T2REPLAY_STATUS_RECORDING;
	t2replay_header.game_id = 2;
	t2replay_header.ruleset = T2REPLAY_RULESET_STOCK;
	t2replay_header.input_semantics = T2REPLAY_INPUT_SEMANTICS_KEY_DET;
	t2replay_header.stage_count = T2REPLAY_STAGE_COUNT;
	t2replay_header.input_offset = T2REPLAY_HEADER_SIZE;
	t2replay_header.start.resident_frame = static_cast<uint32_t>(resident->frame);
	// main_entry() assigns this exact resident value to [random_seed] immediately
	// before stage_init(). stage_init() then consumes it to fill [randring].
	t2replay_header.start.random_seed = t2replay_header.start.resident_frame;
	t2replay_header.start.score = resident->score;
	t2replay_header.start.score_highest = resident->score_highest;
	t2replay_header.start.continues_used = resident->continues_used;
	t2replay_header.start.skill = resident->skill;
	t2replay_header.start.stage = resident->stage;
	t2replay_header.start.rank = resident->rank;
	t2replay_header.start.rem_lives = resident->rem_lives;
	t2replay_header.start.rem_bombs = resident->rem_bombs;
	t2replay_header.start.start_lives = resident->start_lives;
	t2replay_header.start.start_bombs = resident->start_bombs;
	t2replay_header.start.start_power = resident->start_power;
	t2replay_header.start.shottype = resident->shottype;
	t2replay_header.start.bgm_mode = resident->bgm_mode;
	t2replay_header.start.reduce_effects = (resident->reduce_effects ? 1 : 0);
}

// cfg_load() has already copied these fields into MAIN globals when replay_entry()
// consumes a command. Apply the same portable start to both owners before
// gameplay_init() and stage_init() derive their native state.
static void t2replay_start_apply(const t2replay_start_t far *start)
{
	resident->frame = static_cast<long>(start->resident_frame);
	resident->score = start->score;
	resident->score_highest = start->score_highest;
	resident->continues_used = start->continues_used;
	resident->skill = start->skill;
	resident->stage = static_cast<unsigned char>(start->stage);
	resident->rank = start->rank;
	resident->rem_lives = start->rem_lives;
	resident->rem_bombs = start->rem_bombs;
	resident->start_lives = start->start_lives;
	resident->start_bombs = start->start_bombs;
	resident->start_power = start->start_power;
	resident->shottype = start->shottype;
	resident->bgm_mode = start->bgm_mode;
	resident->reduce_effects = (start->reduce_effects != 0);
	resident->debug = false;
	resident->demo_num = 0;
	stage_id = start->stage;
	lives = start->start_lives;
	bombs = start->start_bombs;
	rank = start->rank;
	power = start->start_power;
	if(power == 0) {
		power++;
	}
	score = start->score;
}

static void t2replay_header_apply(void)
{
	t2replay_start_apply(&t2replay_header.start);
}

static bool t2replay_command_valid(const t2replay_command_t far *command)
{
	unsigned i;

	if(!t2replay_command_magic_matches(command->magic) ||
		((command->mode != T2REPLAY_COMMAND_RECORD) &&
		 (command->mode != T2REPLAY_COMMAND_PLAYBACK)) ||
		(command->slot >= T2REPLAY_SLOT_COUNT) ||
		((command->flags & ~T2REPLAY_COMMAND_KNOWN_FLAGS) != 0) ||
		(command->reserved_0 != 0)) {
		return false;
	}
	for(i = 0; i < sizeof(command->reserved); i++) {
		if(command->reserved[i] != 0) {
			return false;
		}
	}
	if(command->mode == T2REPLAY_COMMAND_PLAYBACK) {
		return (
			(command->flags == 0) &&
			t2replay_bytes_zero(
				reinterpret_cast<const uint8_t far *>(&command->start),
				sizeof(command->start)
			)
		);
	}
	if(command->flags == 0) {
		return t2replay_bytes_zero(
			reinterpret_cast<const uint8_t far *>(&command->start),
			sizeof(command->start)
		);
	}
	return t2replay_practice_start_valid(&command->start);
}

static t2replay_mode_t t2replay_command_load(
	uint8_t far *slot, uint8_t far *flags, t2replay_start_t far *start
)
{
	t2replay_command_t command;
	uint32_t size;
	int fd;

	fd = t2replay_dos_open(t2replay_command_fn, T2REPLAY_DOS_ACCESS_READ);
	if(fd < 0) {
		return T2RM_DISABLED;
	}
	if((t2replay_dos_read(fd, &command, sizeof(command)) != sizeof(command)) ||
		!t2replay_dos_size(fd, &size)) {
		t2replay_dos_close(fd);
		t2replay_dos_delete(t2replay_command_fn);
		return T2RM_DISABLED;
	}
	t2replay_dos_close(fd);
	t2replay_dos_delete(t2replay_command_fn);
	if((size != sizeof(command)) || !t2replay_command_valid(&command)) {
		return T2RM_DISABLED;
	}
	*slot = command.slot;
	*flags = command.flags;
	*start = command.start;
	return static_cast<t2replay_mode_t>(command.mode);
}

static void t2replay_final_score_capture(void)
{
	if(t2replay_stage_seen && (t2replay_last_stage < T2REPLAY_STAGE_COUNT)) {
		t2replay_header.stage_scores[t2replay_last_stage] =
			static_cast<uint32_t>(score);
	}
	t2replay_header.score_final = score;
	t2replay_header.lives_final = lives;
	t2replay_header.bombs_final = bombs;
	t2replay_header.power_final = power;
	t2replay_header.terminal_stage = static_cast<uint8_t>(stage_id);
}

static void t2replay_finalize(uint8_t end_reason)
{
	if(t2replay_finished || (t2replay_mode == T2RM_DISABLED)) {
		return;
	}
	t2replay_finished = true;
	if(t2replay_mode == T2RM_RECORD) {
		t2replay_final_score_capture();
		t2replay_header.end_reason = end_reason;
		if(!t2replay_failed &&
			(!t2replay_record_control(
				T2REPLAY_CONTROL_TERMINAL,
				end_reason,
				t2replay_header.terminal_stage
			) || !t2replay_buffer_flush())) {
			t2replay_failed = true;
		}
		t2replay_header.status = (
			t2replay_failed ? T2REPLAY_STATUS_ERROR : T2REPLAY_STATUS_FINALIZED
		);
		if(!t2replay_header_write(false)) {
			t2replay_failed = true;
		}
		t2replay_mode = T2RM_DISABLED;
	} else {
		if(
			t2replay_failed ||
			!t2replay_playback_control(
				T2REPLAY_CONTROL_TERMINAL,
				end_reason,
				static_cast<uint8_t>(stage_id)
			) ||
			!t2replay_terminal_state_matches() ||
			(t2replay_decode_run != 0) ||
			(t2replay_packet_cursor != t2replay_header.packet_count) ||
			(t2replay_sample_cursor != t2replay_header.sample_count)
		) {
			t2replay_failed = true;
		}
		t2replay_playback_exit = true;
	}
}

void replay_entry(void)
{
	uint8_t slot;
	uint8_t command_flags;
	t2replay_mode_t command_mode;
	t2replay_start_t command_start;

	if(t2replay_mode != T2RM_DISABLED) {
		return;
	}
	t2replay_paths_init();
	command_mode = t2replay_command_load(&slot, &command_flags, &command_start);
	if(command_mode == T2RM_DISABLED) {
		return;
	}
	t2replay_slot_set(slot);
	t2replay_payload_checksum = T2REPLAY_FNV1A_BASIS;
	t2replay_buffer_len = 0;
	t2replay_buffer_pos = 0;
	t2replay_payload_written = 0;
	t2replay_packet_cursor = 0;
	t2replay_sample_cursor = 0;
	t2replay_pending_run = 0;
	t2replay_decode_run = 0;
	t2replay_pending_valid = false;
	t2replay_failed = false;
	t2replay_finished = false;
	t2replay_playback_exit = false;
	t2replay_stage_seen = false;
	if(command_mode == T2RM_RECORD) {
		t2replay_mode = T2RM_RECORD;
		t2replay_header_capture();
		if(command_flags & T2REPLAY_COMMAND_FLAG_PRACTICE) {
			t2replay_header.start = command_start;
			t2replay_header_apply();
		}
		if(!t2replay_start_valid(&t2replay_header.start) ||
			!t2replay_header_write(true)) {
			t2replay_mode = T2RM_DISABLED;
		}
	} else if(t2replay_header_read()) {
		t2replay_mode = T2RM_PLAYBACK;
		t2replay_payload_checksum = T2REPLAY_FNV1A_BASIS;
		t2replay_header_apply();
	}
}

void replay_stage_start(void)
{
	if(t2replay_mode == T2RM_DISABLED) {
		return;
	}
	if(t2replay_mode == T2RM_RECORD) {
		if(t2replay_stage_seen && (t2replay_last_stage < T2REPLAY_STAGE_COUNT)) {
			t2replay_header.stage_scores[t2replay_last_stage] =
				static_cast<uint32_t>(score);
		}
		t2replay_header.stage_reached = static_cast<uint8_t>(stage_id);
		if(!t2replay_record_control(
			T2REPLAY_CONTROL_STAGE_START, stage_id, 0
		)) {
			t2replay_failed = true;
		}
	} else if(
		!t2replay_playback_control(T2REPLAY_CONTROL_STAGE_START, stage_id, 0) ||
		(t2replay_stage_seen && !t2replay_stage_score_matches(t2replay_last_stage))
	) {
		t2replay_fail();
	}
	t2replay_last_stage = static_cast<uint8_t>(stage_id);
	t2replay_stage_seen = true;
}

void replay_input_sample(uint8_t phase)
{
	input_t host_input;

	if(t2replay_mode == T2RM_DISABLED) {
		return;
	}
	host_input = key_det;
	if(t2replay_mode == T2RM_RECORD) {
		if(!t2replay_failed && !t2replay_record_sample(phase)) {
			t2replay_failed = true;
		}
	} else {
		if(!t2replay_playback_sample(phase)) {
			t2replay_fail();
			return;
		}
		if(host_input & INPUT_CANCEL) {
			t2replay_fail();
		}
	}
}

bool replay_gameover(void)
{
	if(t2replay_mode == T2RM_DISABLED) {
		return false;
	}
	t2replay_finalize(T2REPLAY_END_GAME_OVER);
	return t2replay_playback_exit;
}

bool replay_process_end(const char *binary_fn)
{
	if(!t2replay_finished && (t2replay_mode != T2RM_DISABLED)) {
		t2replay_finalize(
			(binary_fn[0] == 'm') ? T2REPLAY_END_CLEAR : T2REPLAY_END_GAME_OVER
		);
	}
	return t2replay_playback_exit;
}

bool replay_playback_active(void)
{
	return (t2replay_mode == T2RM_PLAYBACK);
}

#pragma codeseg
