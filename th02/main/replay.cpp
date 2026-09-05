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
#include "platform/x86real/pc98/keyboard.hpp"
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "planar.h"
#include "th01/rank.h"
#include "th02/common.h"
#include "th02/formats/map.hpp"
#include "th02/replay_format.hpp"
#include "th02/resident.hpp"
#include "th02/core/globals.hpp"
#include "th02/gaiji/gaiji.h"
#include "th02/hardware/frmdelay.h"
#include "th02/hardware/input.hpp"
#include "th02/hardware/pages.hpp"
#include "th02/math/randring.hpp"
#include "th02/snd/snd.h"
#include "th02/main/frames.hpp"
#include "th02/main/main.hpp"
#include "th02/main/memory_budget.hpp"
#include "th02/main/midboss/midboss.hpp"
#include "th02/main/s1_actor.hpp"
#include "th02/main/s2_actor.hpp"
#include "th02/main/s3_actor.hpp"
#include "th02/main/s3_north.hpp"
#include "th02/main/s3_pract.hpp"
#include "th02/main/s4_actor.hpp"
#include "th02/main/s5_actor.hpp"
#include "th02/main/s5_cbred.hpp"
#include "th02/main/s5_fx.hpp"
#include "th02/main/s5_palette.hpp"
#include "th02/main/s5_tile.hpp"
#include "th02/main/s6_actor.hpp"
#include "th02/main/later_boss_practice.hpp"
#include "th02/main/actor_core.hpp"
#include "th02/main/checkpoint_apply.hpp"
#include "th02/main/hud/hud.hpp"
#include "th02/main/playperf.hpp"
#include "th02/main/practice.hpp"
#include "th02/practice_diag.hpp"
#include "th02/savestate_acceptance.hpp"
#include "th02/main/score.hpp"
#include "th02/main/scroll.hpp"
#include "th02/main/slowdown.hpp"
#include "th02/main/stage/stage.hpp"
#include "th02/main/player/player.hpp"
#include "th02/main/player/bomb.hpp"
#include "th02/main/player/shot.hpp"
#include "th02/main/bg_particle.hpp"
#include "th02/main/bullet/bullet.hpp"
#include "th02/main/bullet/state.hpp"
#include "th02/main/replay.hpp"
#include "th02/main/laser.hpp"
#include "th02/main/enemy/enemy.hpp"
#include "th02/main/item/item.hpp"
#include "th02/main/item/shared.hpp"
#include "th02/main/spark.hpp"
#include "th02/main/pointnum/pointnum.hpp"
#include "th02/sprites/pointnum.h"
#include "th02/sprites/bombpart.h"
#include "th02/v_colors.hpp"
#include "th02/main/tile/tile.hpp"
#include "th02/main/stage/callback.hpp"
#include "th02/main/null.hpp"
#include "th04/main/rp_guard.hpp"

#define T2REPLAY_BUFFER_PACKET_COUNT 256
#define T2REPLAY_BUFFER_SIZE (T2REPLAY_BUFFER_PACKET_COUNT * T2REPLAY_PACKET_SIZE)
#define T2REPLAY_FAST_FORWARD_RATE 4
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
static char t2replay_stage_seek_fn[11];
static bool t2replay_paths_ready;
static t2replay_mode_t t2replay_mode;
static t2replay_header_t t2replay_header;
static t2replay_packet_t t2replay_buffer[T2REPLAY_BUFFER_PACKET_COUNT];
static uint16_t t2replay_buffer_len;
static uint16_t t2replay_buffer_pos;
static uint32_t t2replay_payload_written;
static uint32_t t2replay_packet_cursor;
static uint32_t t2replay_sample_cursor;
static uint32_t t2replay_protect_sample_count;
static uint32_t t2replay_payload_checksum;
static t2replay_packet_t t2replay_pending;
static uint8_t t2replay_pending_run;
static uint8_t t2replay_decode_run;
static bool t2replay_pending_valid;
static bool t2replay_failed;
static bool t2replay_finished;
static bool t2replay_playback_exit;
static bool t2replay_save_prompted;
static bool t2replay_stage_seen;
static uint8_t t2replay_last_stage;
static uint8_t t2replay_practice_target;
static uint8_t t2replay_fast_forward_phase;
static uint8_t t2replay_fast_forward_slowdown;
static bool t2replay_fast_forward_slowdown_active;
static uint16_t t2replay_timing_baseline;
static bool t2replay_timing_armed;
static bool t2replay_timing_pause_opened;
static uint8_t t2replay_timing_target;
static bool t2replay_rank_lock_active;
static int16_t t2replay_rank_lock_value;
static bool t2replay_autofire_active;
static bool t2replay_autofire_release_frame;
static t2replay_public_seek_entry_t
	t2replay_stage_seek_entries[T2REPLAY_STAGE_COUNT];
static t2replay_start_t t2replay_stage_seek_starts[T2REPLAY_STAGE_COUNT];
static uint8_t t2replay_stage_seek_count;

#ifdef T2SGA
static int t2replay_debug_midboss_step;
static int t2replay_debug_previous_midboss_step;
#endif

#if T2REPLAY_EXACT_APPLY
enum t2replay_exact_load_result_t {
	T2XLR_ABSENT = 0,
	T2XLR_READY = 1,
	T2XLR_REJECTED = 2,
};

enum t2replay_exact_diag_state_t {
	T2XDS_NONE = 0,
	T2XDS_REJECTED = 1,
	T2XDS_PREPARED = 2,
	T2XDS_COMMITTED = 3,
	T2XDS_REVEALED = 4,
};

static char t2replay_exact_request_fn[11];
static char t2replay_exact_diag_fn[12];
static char t2replay_public_seek_request_fn[11];
static char t2replay_public_seek_sidecar_fn[11];
static char t2xobs_req_fn[11];
static char t2xobs_out_fn[11];
static uint8_t far *t2replay_exact_envelope;
static t2replay_exact_apply_request_t t2replay_exact_request;
static t2xobs_req_t t2xobs_req;
static t2xobs_out_t t2xobs_out;
static bool t2replay_exact_pending;
static bool t2replay_exact_active;
static bool t2replay_exact_anchor_sample_pending;
static bool t2replay_exact_first_sample_pending;
static bool t2xobs_active;
static bool t2xobs_direct;

struct t2replay_exact_diag_t {
	char magic[8];
	uint8_t version;
	uint8_t state;
	uint8_t reject;
	uint8_t page_front;
	uint8_t page_back;
	uint8_t phase;
	uint8_t anchor_consumed;
	uint8_t reserved;
	uint32_t sample_cursor;
	uint32_t packet_cursor;
	uint32_t sample_anchor;
	uint32_t packet_anchor;
};

static t2replay_exact_diag_t t2replay_exact_diag;
static bool16 near t2replay_exact_apply_at_loop_top(void);
static void near t2replay_exact_envelope_free(void);
static void near t2replay_exact_diag_flush(void);
static void near t2xobs_reveal(void);
static void near t2xobs_terminal(void);
static bool near t2xobs_req_load(
	uint8_t slot, bool replay_header_valid
);
static t2replay_exact_load_result_t near t2replay_public_seek_request_load(
	uint8_t slot, bool replay_header_valid
);
#endif
static void t2replay_fail(void);

static void t2replay_indicator_put(void)
{
	const tram_x_t left = (HUD_LEFT + 2);

	gaiji_putca((left + 0), 1, gb_R, TX_YELLOW);
	gaiji_putca((left + 2), 1, gb_E, TX_YELLOW);
	gaiji_putca((left + 4), 1, gb_P, TX_YELLOW);
	gaiji_putca((left + 6), 1, gb_L, TX_YELLOW);
	gaiji_putca((left + 8), 1, gb_A, TX_YELLOW);
	gaiji_putca((left + 10), 1, gb_Y, TX_YELLOW);
}

static void t2replay_fast_forward_restore(void)
{
	if(t2replay_fast_forward_slowdown_active) {
		// zero is private to the preceding replay pacing override. Native
		// gameplay never uses it as a valid delay factor.
		if(slowdown_factor == 0) {
			slowdown_factor = t2replay_fast_forward_slowdown;
		}
		t2replay_fast_forward_slowdown_active = false;
	}
}

static void t2replay_fast_forward_boundary_reset(void)
{
	t2replay_fast_forward_restore();
	// A stage/process terminal can occur after the last replay sample of a
	// playback frame. Do not let that frame's private wait bypass escape into
	// the native transition before another input seam can clear it.
	t2replay_fast_forward_phase = 0;
	t2replay_timing_armed = false;
	t2replay_timing_pause_opened = false;
	t2replay_timing_target = 0;
}

static void t2replay_fast_forward_wait_skip(bool held)
{
	uint8_t phase;

	if(!held || (t2replay_mode != T2RM_PLAYBACK)) {
		t2replay_fast_forward_phase = 0;
		return;
	}
	phase = static_cast<uint8_t>(t2replay_fast_forward_phase + 1);
	if(phase >= T2REPLAY_FAST_FORWARD_RATE) {
		t2replay_fast_forward_phase = 0;
		return;
	}
	t2replay_fast_forward_phase = phase;
	t2replay_fast_forward_slowdown = slowdown_factor;
	slowdown_factor = 0;
	t2replay_fast_forward_slowdown_active = true;
}

static void t2replay_timing_gameplay_sample(input_t input)
{
	uint16_t elapsed;

	if(t2replay_mode != T2RM_RECORD) {
		return;
	}
	if(t2replay_timing_armed) {
		elapsed = static_cast<uint16_t>(vsync_Count2 - t2replay_timing_baseline);
		if(!t2replay_timing_pause_opened && (t2replay_timing_target != 0)) {
			if(t2replay_header.timed_frames == 0xFFFFFFFFUL) {
				t2replay_failed = true;
			} else {
				t2replay_header.timed_frames++;
				if(elapsed > t2replay_timing_target) {
					if(t2replay_header.slow_frames == 0xFFFFFFFFUL) {
						t2replay_failed = true;
					} else {
						t2replay_header.slow_frames++;
					}
				}
			}
		}
	}
	t2replay_timing_baseline = vsync_Count2;
	t2replay_timing_target = slowdown_factor;
	t2replay_timing_pause_opened = ((input & INPUT_CANCEL) != 0);
	t2replay_timing_armed = true;
}

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
extern "C" char rika_bgm_fn[];
extern "C" char aBoss4_m[];
extern "C" char aBoss2_m[];
extern "C" char aBoss3_m[];
extern "C" const char aStage3_b_bft[];
extern "C" const char aStage3_b_btt_0[];
extern "C" char aMima_m[];
extern "C" const char mima1_bft[];
extern "C" const char aStage3_b_btt[];
extern "C" const char stage5b1_bft[];
extern "C" const char stage5b2_bft[];
extern "C" char aBoss5_m[];
extern screen_point_t sigma_topleft;
extern Palette8 __cdecl Palettes;
extern void far pascal palette_show(void);

#ifdef T2SGA
static void t2replay_debug_uint_put(
	tram_x_t left, uint32_t value, uint32_t divisor
)
{
	char digits[11];
	int i = 0;

	while(divisor != 0) {
		digits[i++] = static_cast<char>('0' + (value / divisor));
		value %= divisor;
		divisor /= 10;
	}
	digits[i] = '\0';
	text_putsa(left, 0, digits, TX_WHITE);
}

static void t2replay_debug_step_put(tram_x_t left, int value)
{
	if(value < 0) {
		text_putsa(left, 0, "----", TX_WHITE);
		return;
	}
	t2replay_debug_uint_put(left, static_cast<uint16_t>(value), 1000UL);
}

static void t2debug_coords_reset(void)
{
	t2replay_debug_midboss_step = midboss_scroll_step;
	t2replay_debug_previous_midboss_step = -1;
}

static void t2debug_coords_put(void)
{
	int previous_step = -1;
	int next_step = -1;
	int event_step;
	int i;

	if(t2replay_debug_midboss_step != midboss_scroll_step) {
		if(
			(t2replay_debug_midboss_step >= 0) &&
			(t2replay_debug_midboss_step <= scroll_step)
		) {
			t2replay_debug_previous_midboss_step =
				t2replay_debug_midboss_step;
		}
		t2replay_debug_midboss_step = midboss_scroll_step;
	}
	if((spawn_rows > 0) && (spawn_grid[0] != NULL)) {
		for(i = 0; i < spawn_rows; i++) {
			event_step = spawn_grid[0][i];
			if((event_step <= scroll_step) && (event_step > previous_step)) {
				previous_step = event_step;
			} else if(
				(event_step > scroll_step) &&
				((next_step < 0) || (event_step < next_step))
			) {
				next_step = event_step;
			}
		}
	}
	if(t2replay_debug_previous_midboss_step > previous_step) {
		previous_step = t2replay_debug_previous_midboss_step;
	}
	if(midboss_scroll_step >= 0) {
		if(
			(midboss_scroll_step <= scroll_step) &&
			(midboss_scroll_step > previous_step)
		) {
			previous_step = midboss_scroll_step;
		} else if(
			(midboss_scroll_step > scroll_step) &&
			((next_step < 0) || (midboss_scroll_step < next_step))
		) {
			next_step = midboss_scroll_step;
		}
	}

	text_putca(2, 0, 'F', TX_WHITE);
	t2replay_debug_uint_put(3, stage_frame, 10000UL);
	text_putca(9, 0, 'L', TX_WHITE);
	t2replay_debug_step_put(10, previous_step);
	text_putca(15, 0, 'N', TX_WHITE);
	t2replay_debug_step_put(16, next_step);
	text_putca(21, 0, 'M', TX_WHITE);
	t2replay_debug_step_put(22, scroll_step);
	text_putca(27, 0, 'R', TX_WHITE);
	t2replay_debug_uint_put(
		28, static_cast<uint16_t>(resident->skill), 100UL
	);
	text_putca(32, 0, 'G', TX_WHITE);
	t2replay_debug_uint_put(
		33, static_cast<uint32_t>(random_seed), 1000000000UL
	);
}
#endif

extern "C" void far boss_bgm_load(char *fn);
extern "C" void far enemies_remove_all(void);
extern "C" void far enemies_callbacks_null(void);
extern "C" void far stage_title_unput(void);
extern "C" bool16 far stage_should_end(void);

// Stable callback vocabulary for the first common-world codec. These IDs are
// intentionally captured and validated even though schema 4 has no apply
// path. A future restorer must reject an unrecognized ID before touching live
// callback slots; it must never deserialize a far code pointer.
enum t2replay_checkpoint_callback_id_t {
	T2RCCB_DISABLED = 0,
	T2RCCB_LIVE = 1,
};

extern "C" void far enemies_invalidate(void);
extern "C" void far enemies_update_and_render(void);
extern "C" void far nullfunc_void_2(void);

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
	case T2RCGI_PLAYER:
		return T2REPLAY_CHECKPOINT_PLAYER_SIZE;
	case T2RCGI_BOMB:
		return T2REPLAY_CHECKPOINT_BOMB_SIZE;
	case T2RCGI_BULLET:
		return T2REPLAY_CHECKPOINT_BULLET_SIZE;
	case T2RCGI_LASER:
		return T2REPLAY_CHECKPOINT_LASER_SIZE;
	case T2RCGI_ENEMY:
		return T2REPLAY_CHECKPOINT_ENEMY_SIZE;
	case T2RCGI_EFFECT:
		return T2REPLAY_CHECKPOINT_EFFECT_SIZE;
	default:
		return 0;
	}
}

static unsigned t2replay_checkpoint_payload_offset(uint8_t id)
{
	unsigned offset = (
		T2REPLAY_CHECKPOINT_HEADER_SIZE +
		(T2REPLAY_CHECKPOINT_GROUP_COUNT * T2REPLAY_CHECKPOINT_GROUP_SIZE)
	);
	uint8_t group_id;

	for(group_id = 0; group_id < id; group_id++) {
		offset += t2replay_checkpoint_group_size(group_id);
	}
	return offset;
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

static void t2replay_checkpoint_put_point(
	uint8_t far *data, unsigned& offset, const screen_point_t& point
)
{
	t2replay_checkpoint_put_u16(data, offset, static_cast<uint16_t>(point.x));
	offset += 2;
	t2replay_checkpoint_put_u16(data, offset, static_cast<uint16_t>(point.y));
	offset += 2;
}

static void t2replay_checkpoint_put_spoint(
	uint8_t far *data, unsigned& offset, const SPPoint& point
)
{
	t2replay_checkpoint_put_u16(data, offset, static_cast<uint16_t>(point.x.v));
	offset += 2;
	t2replay_checkpoint_put_u16(data, offset, static_cast<uint16_t>(point.y.v));
	offset += 2;
}

static bool t2replay_checkpoint_entity_flag_valid(uint8_t value)
{
	return (value <= F_REMOVE);
}

static bool t2replay_checkpoint_bullet_motion_valid(uint8_t value)
{
	switch(value) {
	case BG_NONE:
	case BG_1:
	case BG_1_AIMED:
	case BG_2_SPREAD_NARROW:
	case BG_2_SPREAD_MEDIUM:
	case BG_2_SPREAD_WIDE:
	case BG_2_SPREAD_NARROW_AIMED:
	case BG_2_SPREAD_MEDIUM_AIMED:
	case BG_2_SPREAD_WIDE_AIMED:
	case BG_2_SPREAD_ULTRAWIDE_AIMED:
	case BG_3_SPREAD_NARROW:
	case BG_3_SPREAD_MEDIUM:
	case BG_3_SPREAD_WIDE:
	case BG_3_SPREAD_NARROW_AIMED:
	case BG_3_SPREAD_MEDIUM_AIMED:
	case BG_3_SPREAD_WIDE_AIMED:
	case BG_4_SPREAD_NARROW:
	case BG_4_SPREAD_MEDIUM:
	case BG_4_SPREAD_WIDE:
	case BG_4_SPREAD_NARROW_AIMED:
	case BG_4_SPREAD_MEDIUM_AIMED:
	case BG_4_SPREAD_WIDE_AIMED:
	case BG_5_SPREAD_NARROW:
	case BG_5_SPREAD_MEDIUM:
	case BG_5_SPREAD_WIDE:
	case BG_5_SPREAD_NARROW_AIMED:
	case BG_5_SPREAD_MEDIUM_AIMED:
	case BG_5_SPREAD_WIDE_AIMED:
	case BG_2_RING:
	case BG_4_RING:
	case BG_8_RING:
	case BG_16_RING:
	case BG_32_RING:
	case BG_1_RANDOM_ANGLE:
	case BG_RANDOM_ANGLE:
	case BG_RANDOM_ANGLE_AND_SPEED:
	case BG_2_SPREAD_HORIZONTALLY_SYMMETRIC:
	case BSM_CHASE:
	case BSM_HOMING:
	case BSM_DECELERATE_THEN_TURN_AIMED:
	case BSM_GRAVITY:
	case BSM_DRIFT_ANGLE_AND_SPEED:
	case BSM_DRIFT_ANGLE_CHASE:
	case BSM_BOUNCE_LEFT_RIGHT_TOP_BOTTOM:
	case BSM_BOUNCE_TOP_BOTTOM:
	case BSM_1:
		return true;
	default:
		return false;
	}
}

static bool t2replay_checkpoint_laser_callbacks_capture(uint8_t far *data)
{
	if(
		(lasers_invalidate_func == nullfunc_void) &&
		(lasers_update_and_render_func == nullfunc_void)
	) {
		data[0] = T2RCCB_DISABLED;
		data[1] = T2RCCB_DISABLED;
		return true;
	}
	if(
		(lasers_invalidate_func == lasers_invalidate) &&
		(lasers_update_and_render_func == lasers_update_and_render)
	) {
		data[0] = T2RCCB_LIVE;
		data[1] = T2RCCB_LIVE;
		return true;
	}
	return false;
}

static bool t2replay_checkpoint_enemy_callbacks_capture(uint8_t far *data)
{
	if(
		(enemies_invalidate_func == nullfunc_void_2) &&
		(enemies_update_and_render_func == nullfunc_void_2)
	) {
		data[0] = T2RCCB_DISABLED;
		data[1] = T2RCCB_DISABLED;
		return true;
	}
	if(
		(enemies_invalidate_func == enemies_invalidate) &&
		(enemies_update_and_render_func == enemies_update_and_render)
	) {
		data[0] = T2RCCB_LIVE;
		data[1] = T2RCCB_LIVE;
		return true;
	}
	return false;
}

static bool t2replay_checkpoint_player_capture(uint8_t far *data)
{
	unsigned offset = 0;
	unsigned i;
	const shot_t near *shot;

	for(i = 0; i < PAGE_COUNT; i++) {
		t2replay_checkpoint_put_u16(
			data, offset, static_cast<uint16_t>(player_left_on_page[i])
		);
		offset += 2;
		t2replay_checkpoint_put_u16(
			data, offset, static_cast<uint16_t>(player_top_on_page[i])
		);
		offset += 2;
	}
	for(i = 0; i < PAGE_COUNT; i++) {
		t2replay_checkpoint_put_point(
			data, offset, player_option_left_topleft[i]
		);
	}
	data[offset++] = player_option_patnum;
	data[offset++] = static_cast<uint8_t>(playchar_speed_aligned_x);
	data[offset++] = static_cast<uint8_t>(playchar_speed_aligned_y);
	data[offset++] = static_cast<uint8_t>(playchar_speed_diagonal_x);
	data[offset++] = static_cast<uint8_t>(playchar_speed_diagonal_y);
	data[offset++] = static_cast<uint8_t>(player_is_hit);
	data[offset++] = player_invincibility_time;
	data[offset++] = (player_invincible_via_bomb ? 1 : 0);
	data[offset++] = miss_frame;
	data[offset++] = (miss_active ? 1 : 0);
	data[offset++] = power;
	t2replay_checkpoint_put_u16(
		data, offset, static_cast<uint16_t>(power_overflow)
	);
	offset += 2;
	data[offset++] = shot_level;
	t2replay_checkpoint_put_u16(
		data, offset, static_cast<uint16_t>(player_patnum)
	);
	offset += 2;
	data[offset++] = shot_stream_a_phase;
	data[offset++] = shot_stream_b_phase;
	data[offset++] = shot_stream_a_cooldown_time;
	data[offset++] = shot_stream_b_cooldown_time;
	data[offset++] = shot_patnum;
	data[offset++] = shot_option_patnum;
	data[offset++] = shot_patnum_powered;
	data[offset++] = shot_option_patnum_powered;
	data[offset++] = static_cast<uint8_t>(shot_a_spread_angle_delta);
	data[offset++] = option_shots_alive;
	t2replay_checkpoint_put_u16(data, offset, static_cast<uint16_t>(boss_pos_x));
	offset += 2;
	t2replay_checkpoint_put_u16(data, offset, static_cast<uint16_t>(boss_pos_y));
	offset += 2;
	t2replay_checkpoint_put_u16(
		data, offset, static_cast<uint16_t>(boss_pos_x_unused)
	);
	offset += 2;
	data[offset++] = shot_c_cycle;
	for(i = 0; i < SHOT_COUNT; i++) {
		data[offset++] = (shots[i].flag == F_FREE) ? 0 : shot_anim_frame[i];
	}
	data[offset++] = (resident->shottype == 0) ? 0 :
		static_cast<uint8_t>(shot_option_decay_interval);
	t2replay_checkpoint_put_u16(data, offset, static_cast<uint16_t>(shot_slot_i));
	offset += 2;
	t2replay_checkpoint_put_u16(data, offset, static_cast<uint16_t>(shot_spawn_top));
	offset += 2;
	for(i = 0; i < SHOT_COUNT; i++) {
		shot = &shots[i];
		if(shot->flag == F_FREE) {
			t2replay_memclear(data + offset, 16);
			offset += 16;
			continue;
		}
		data[offset++] = static_cast<uint8_t>(shot->flag);
		data[offset++] = shot->decay_cel;
		t2replay_checkpoint_put_spoint(data, offset, shot->pos_on_page[0]);
		t2replay_checkpoint_put_spoint(data, offset, shot->pos_on_page[1]);
		t2replay_checkpoint_put_spoint(data, offset, shot->velocity);
		data[offset++] = shot->patnum;
		data[offset++] = (shot->from_option ? 1 : 0);
	}
	return (offset == T2REPLAY_CHECKPOINT_PLAYER_SIZE);
}

static bool t2replay_checkpoint_bomb_capture(uint8_t far *data)
{
	unsigned offset = 0;
	unsigned i;

	data[offset++] = (bombing ? 1 : 0);
	if(bombing) {
		t2replay_checkpoint_put_u16(data, offset, static_cast<uint16_t>(bomb_frame));
		offset += 2;
		t2replay_checkpoint_put_u16(
			data, offset, static_cast<uint16_t>(bomb_circle_center.x)
		);
		offset += 2;
		t2replay_checkpoint_put_u16(
			data, offset, static_cast<uint16_t>(bomb_circle_center.y)
		);
		offset += 2;
		t2replay_checkpoint_put_u16(
			data, offset, static_cast<uint16_t>(bomb_circle_frame)
		);
		offset += 2;
		t2replay_checkpoint_put_u16(
			data, offset, static_cast<uint16_t>(bomb_circle_done)
		);
		offset += 2;
		for(i = 0; i < BOMB_PARTICLE_COUNT; i++) {
			t2replay_checkpoint_put_u16(
				data, offset, static_cast<uint16_t>(bomb_particle_pos[i].x)
			);
			offset += 2;
			t2replay_checkpoint_put_u16(
				data, offset, static_cast<uint16_t>(bomb_particle_pos[i].y)
			);
			offset += 2;
		}
		for(i = 0; i < BOMB_PARTICLE_COUNT; i++) {
			data[offset++] = bomb_particle_cel[i];
		}
		for(i = 0; i < BOMB_SMEAR_COLUMNS; i++) {
			t2replay_checkpoint_put_u16(
				data, offset, static_cast<uint16_t>(bomb_smears[i].bottom)
			);
			offset += 2;
		}
		data[offset++] = static_cast<uint8_t>(tile_mode_before_bomb_a);
		data[offset++] = static_cast<uint8_t>(tile_mode_before_bomb_b);
		data[offset++] = static_cast<uint8_t>(tile_mode_before_bomb_c);
		for(i = 0; i < COMPONENT_COUNT; i++) {
			data[offset++] = col0_before_bomb_a.v[i];
		}
		for(i = 0; i < COMPONENT_COUNT; i++) {
			data[offset++] = col3_before_bomb_a.v[i];
		}
		data[offset++] = bomb_b_cel;
	} else {
		offset = (T2REPLAY_CHECKPOINT_BOMB_SIZE - 4);
	}
	data[offset++] = stage_miss_count;
	data[offset++] = stage_bombs_used;
	data[offset++] = total_miss_count;
	data[offset++] = total_bombs_used;
	return (offset == T2REPLAY_CHECKPOINT_BOMB_SIZE);
}

static bool t2replay_checkpoint_bullet_capture(uint8_t far *data)
{
	unsigned offset = 0;
	unsigned i;
	const bullet_t near *bullet;

	t2replay_checkpoint_put_u16(
		data, offset, static_cast<uint16_t>(bullet_special.u1.chase_speed.v)
	);
	offset += 2;
	t2replay_checkpoint_put_u16(
		data, offset, static_cast<uint16_t>(bullet_special.u2.homing_frames)
	);
	offset += 2;
	t2replay_checkpoint_put_u16(
		data, offset, static_cast<uint16_t>(bullet_special.u3.turns_max)
	);
	offset += 2;
	data[offset++] = static_cast<uint8_t>(rank_base_speed.v);
	data[offset++] = rank_base_stack;
	data[offset++] = bullet_stack;
	data[offset++] = static_cast<uint8_t>(easy_slow_skip_cycle);
	for(i = 0; i < BULLET_COUNT; i++) {
		bullet = &bullets[i];
		if(bullet->flag == F_FREE) {
			t2replay_memclear(data + offset, 19);
			offset += 19;
			continue;
		}
		data[offset++] = static_cast<uint8_t>(bullet->flag);
		data[offset++] = static_cast<uint8_t>(bullet->size_type);
		t2replay_checkpoint_put_spoint(data, offset, bullet->screen_topleft[0]);
		t2replay_checkpoint_put_spoint(data, offset, bullet->screen_topleft[1]);
		t2replay_checkpoint_put_spoint(data, offset, bullet->velocity);
		data[offset++] = bullet->patnum;
		data[offset++] = static_cast<uint8_t>(bullet->group_or_special_motion);
		data[offset++] = bullet->angle;
		data[offset++] = bullet->speed.v;
		data[offset++] = bullet->u1.v;
	}
	return (offset == T2REPLAY_CHECKPOINT_BULLET_SIZE);
}

static bool t2replay_checkpoint_laser_capture(uint8_t far *data)
{
	unsigned offset = 0;
	unsigned i;
	const laser_t near *laser;

	if(!t2replay_checkpoint_laser_callbacks_capture(data + offset)) {
		return false;
	}
	offset += 2;
	data[offset++] = laser_wait_frames;
	for(i = 0; i < LASER_COUNT; i++) {
		laser = &lasers[i];
		if(laser->flag == F_FREE) {
			t2replay_memclear(data + offset, 12);
			offset += 12;
			continue;
		}
		data[offset++] = static_cast<uint8_t>(laser->flag);
		data[offset++] = laser->phase;
		t2replay_checkpoint_put_point(data, offset, laser->origin);
		t2replay_checkpoint_put_u16(
			data, offset, static_cast<uint16_t>(laser->wait_frames)
		);
		offset += 2;
		t2replay_checkpoint_put_u16(
			data, offset, static_cast<uint16_t>(laser->active_frames)
		);
		offset += 2;
		data[offset++] = laser->charge_cel;
		data[offset++] = laser->patnum_base;
	}
	return (offset == T2REPLAY_CHECKPOINT_LASER_SIZE);
}

static bool t2replay_checkpoint_enemy_capture(uint8_t far *data)
{
	unsigned offset = 0;
	unsigned i;
	const enemy_t near *enemy;

	data[offset++] = enemy_scripts_used;
	data[offset++] = enemies_loop_bound;
	if(!t2replay_checkpoint_enemy_callbacks_capture(data + offset)) {
		return false;
	}
	offset += 2;
	for(i = 0; i < ENEMY_COUNT; i++) {
		enemy = &enemies[i];
		if(enemy->flag == F_FREE) {
			t2replay_memclear(data + offset, 36);
			offset += 36;
			continue;
		}
		t2replay_checkpoint_put_point(data, offset, enemy->pos_on_page[0]);
		t2replay_checkpoint_put_point(data, offset, enemy->pos_on_page[1]);
		t2replay_checkpoint_put_u16(data, offset, static_cast<uint16_t>(enemy->script_ip));
		offset += 2;
		t2replay_checkpoint_put_u16(data, offset, static_cast<uint16_t>(enemy->age));
		offset += 2;
		t2replay_checkpoint_put_u16(data, offset, static_cast<uint16_t>(enemy->template_id));
		offset += 2;
		data[offset++] = static_cast<uint8_t>(enemy->flag);
		data[offset++] = enemy->anim_frame;
		data[offset++] = (enemy->in_kill_anim ? 1 : 0);
		t2replay_checkpoint_put_u16(data, offset, static_cast<uint16_t>(enemy->patnum_delta));
		offset += 2;
		t2replay_checkpoint_put_u16(data, offset, static_cast<uint16_t>(enemy->render_as));
		offset += 2;
		t2replay_checkpoint_put_u16(data, offset, static_cast<uint16_t>(enemy->angle));
		offset += 2;
		t2replay_checkpoint_put_u16(
			data, offset, static_cast<uint16_t>(enemy->spawned_in_left_half)
		);
		offset += 2;
		t2replay_checkpoint_put_u16(data, offset, static_cast<uint16_t>(enemy->loop_i));
		offset += 2;
		data[offset++] = static_cast<uint8_t>(enemy->velocity_x);
		data[offset++] = static_cast<uint8_t>(enemy->velocity_y);
		data[offset++] = (enemy->despawn_when_offscreen_vertically ? 1 : 0);
		t2replay_checkpoint_put_u16(data, offset, static_cast<uint16_t>(enemy->damage));
		offset += 2;
		data[offset++] = (enemy->not_shootable ? 1 : 0);
		data[offset++] = (enemy->no_player_collision ? 1 : 0);
		data[offset++] = enemy->pellet_group;
		data[offset++] = enemy->pellet_speed;
	}
	return (offset == T2REPLAY_CHECKPOINT_ENEMY_SIZE);
}

static bool t2replay_checkpoint_effect_capture(uint8_t far *data)
{
	unsigned offset = 0;
	unsigned i;
	const item_t near *item;
	const spark_t near *spark;

	t2replay_checkpoint_put_u16(data, offset, item_bigpower_override);
	offset += 2;
	data[offset++] = (items_miss_add_gameover ? 1 : 0);
	data[offset++] = item_semirandom_ring_p;
	data[offset++] = item_semirandom_cycle;
	data[offset++] = item_drop_cycle;
	data[offset++] = item_collect_skill;
	t2replay_checkpoint_put_u32(
		data, offset, static_cast<uint32_t>(item_score_this_frame)
	);
	offset += 4;
	t2replay_checkpoint_put_u16(
		data, offset, static_cast<uint16_t>(item_skill)
	);
	offset += 2;
	t2replay_checkpoint_put_u16(
		data, offset, static_cast<uint16_t>(point_items_collected)
	);
	offset += 2;
	for(i = 0; i < ITEM_COUNT; i++) {
		item = &items[i];
		if(item->flag == F_FREE) {
			t2replay_memclear(data + offset, 16);
			offset += 16;
			continue;
		}
		data[offset++] = static_cast<uint8_t>(item->flag);
		data[offset++] = static_cast<uint8_t>(item->type);
		for(unsigned page = 0; page < PAGE_COUNT; page++) {
			t2replay_checkpoint_put_u16(
				data, offset, static_cast<uint16_t>(item->pos[page].screen_left)
			);
			offset += 2;
			t2replay_checkpoint_put_u16(
				data, offset, static_cast<uint16_t>(item->pos[page].screen_top.v)
			);
			offset += 2;
		}
		t2replay_checkpoint_put_u16(
			data, offset, static_cast<uint16_t>(item->velocity_y.v)
		);
		offset += 2;
		t2replay_checkpoint_put_u16(
			data, offset, static_cast<uint16_t>(item->velocity_x_during_bounce)
		);
		offset += 2;
		t2replay_checkpoint_put_u16(data, offset, static_cast<uint16_t>(item->age));
		offset += 2;
	}
	for(i = 0; i < SPARK_COUNT; i++) {
		spark = &sparks[i];
		if(spark->flag == F_FREE) {
			t2replay_memclear(data + offset, 18);
			offset += 18;
			continue;
		}
		data[offset++] = static_cast<uint8_t>(spark->flag);
		data[offset++] = spark->age;
		t2replay_checkpoint_put_spoint(data, offset, spark->screen_topleft[0]);
		t2replay_checkpoint_put_spoint(data, offset, spark->screen_topleft[1]);
		t2replay_checkpoint_put_spoint(data, offset, spark->velocity);
		data[offset++] = static_cast<uint8_t>(spark->render_as);
		data[offset++] = spark->angle;
		data[offset++] = spark->speed_base.v;
		data[offset++] = static_cast<uint8_t>(spark->default_render_as);
	}
	t2replay_checkpoint_put_u16(data, offset, spark_ring_i);
	offset += 2;
	data[offset++] = spark_sprite_interval;
	data[offset++] = spark_age_max;
	t2replay_checkpoint_put_u16(
		data, offset, static_cast<uint16_t>(spark_accel_x.v)
	);
	offset += 2;
	data[offset++] = pointnums.col;
	for(i = 0; i < POINTNUM_COUNT; i++) {
		if(pointnums.flag[i] == F_FREE) {
			t2replay_memclear(data + offset, 10);
			offset += 10;
			continue;
		}
		t2replay_checkpoint_put_u16(
			data, offset, static_cast<uint16_t>(pointnums.left[i])
		);
		offset += 2;
		t2replay_checkpoint_put_u16(
			data, offset, static_cast<uint16_t>(pointnums.top[i][0])
		);
		offset += 2;
		t2replay_checkpoint_put_u16(
			data, offset, static_cast<uint16_t>(pointnums.top[i][1])
		);
		offset += 2;
		t2replay_checkpoint_put_u16(data, offset, pointnums.points[i]);
		offset += 2;
		data[offset++] = static_cast<uint8_t>(pointnums.flag[i]);
		data[offset++] = pointnums.age[i];
	}
	data[offset++] = static_cast<uint8_t>(pointnums.op);
	data[offset++] = static_cast<uint8_t>(pointnums.operand);
	return (offset == T2REPLAY_CHECKPOINT_EFFECT_SIZE);
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
		data[33] = hiscore_continues;
		t2replay_checkpoint_put_u32(data, 34, static_cast<uint32_t>(score_delta));
		t2replay_checkpoint_put_u16(
			data, 38, score_delta_transferred_prev
		);
		t2replay_checkpoint_put_u16(data, 40, extends_gained);
		data[42] = static_cast<uint8_t>(lives);
		data[43] = static_cast<uint8_t>(bombs);
		data[44] = power;
		data[45] = (quit ? 1 : 0);
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

	case T2RCGI_PLAYER:
		return t2replay_checkpoint_player_capture(data);

	case T2RCGI_BOMB:
		return t2replay_checkpoint_bomb_capture(data);

	case T2RCGI_BULLET:
		return t2replay_checkpoint_bullet_capture(data);

	case T2RCGI_LASER:
		return t2replay_checkpoint_laser_capture(data);

	case T2RCGI_ENEMY:
		return t2replay_checkpoint_enemy_capture(data);

	case T2RCGI_EFFECT:
		return t2replay_checkpoint_effect_capture(data);

	default:
		return false;
	}
}

static bool t2replay_checkpoint_player_payload_valid(
	const uint8_t far *data
)
{
	unsigned offset = 92;
	unsigned i;
	int16_t value;

	if(
		(data[21] != PLAYER_NOT_HIT) &&
		(data[21] != PLAYER_HIT) &&
		(data[21] != PLAYER_HIT_GAMEOVER)
	) {
		return false;
	}
	value = static_cast<int16_t>(t2replay_checkpoint_get_u16(data, 27));
	if(
		(data[23] > 1) || (data[25] > 1) || (data[26] > POWER_MAX) ||
		(value < 0) || (value > POWER_OVERFLOW_MAX) ||
		(data[29] > SHOT_LEVEL_MAX) || (data[41] > 1) ||
		(data[87] > 16) ||
		(static_cast<int16_t>(t2replay_checkpoint_get_u16(data, 88)) < 0) ||
		(static_cast<int16_t>(t2replay_checkpoint_get_u16(data, 88)) > SHOT_COUNT)
	) {
		return false;
	}
	for(i = 0; i < SHOT_COUNT; i++, offset += 16) {
		if(data[offset] == F_FREE) {
			if(!t2replay_bytes_zero(data + offset, 16)) {
				return false;
			}
			continue;
		}
		if(
			!t2replay_checkpoint_entity_flag_valid(data[offset]) ||
			(data[offset + 15] > 1)
		) {
			return false;
		}
	}
	return true;
}

static bool t2replay_checkpoint_bomb_payload_valid(
	const uint8_t far *data
)
{
	unsigned offset;
	unsigned i;

	if(data[0] > 1) {
		return false;
	}
	if(data[0] == 0) {
		return t2replay_bytes_zero(data + 1,
			(T2REPLAY_CHECKPOINT_BOMB_SIZE - 5));
	}
	if(
		(static_cast<int16_t>(t2replay_checkpoint_get_u16(data, 1)) < 0) ||
		(static_cast<int16_t>(t2replay_checkpoint_get_u16(data, 1)) > 184) ||
		(t2replay_checkpoint_get_u16(data, 9) > 1)
	) {
		return false;
	}
	offset = 11 + (BOMB_PARTICLE_COUNT * 4);
	for(i = 0; i < BOMB_PARTICLE_COUNT; i++) {
		if(data[offset + i] > BOMB_PARTICLE_CELS) {
			return false;
		}
	}
	offset += BOMB_PARTICLE_COUNT + (BOMB_SMEAR_COLUMNS * 2);
	return (
		(data[offset + 0] <= TM_NONE) &&
		(data[offset + 1] <= TM_NONE) &&
		(data[offset + 2] <= TM_NONE) &&
		(data[offset + 9] < 18)
	);
}

static bool t2replay_checkpoint_bullet_payload_valid(
	const uint8_t far *data
)
{
	unsigned offset = 10;
	unsigned i;

	if(data[7] > 1) {
		return false;
	}
	for(i = 0; i < BULLET_COUNT; i++, offset += 19) {
		if(data[offset] == F_FREE) {
			if(!t2replay_bytes_zero(data + offset, 19)) {
				return false;
			}
			continue;
		}
		if(
			!t2replay_checkpoint_entity_flag_valid(data[offset]) ||
			((data[offset + 1] != BST_PELLET) &&
				(data[offset + 1] != BST_BULLET16)) ||
			!t2replay_checkpoint_bullet_motion_valid(data[offset + 15])
		) {
			return false;
		}
	}
	return true;
}

static bool t2replay_checkpoint_laser_payload_valid(
	const uint8_t far *data
)
{
	unsigned offset = 3;
	unsigned i;

	if(
		(data[0] > T2RCCB_LIVE) || (data[1] > T2RCCB_LIVE) ||
		(data[0] != data[1])
	) {
		return false;
	}
	for(i = 0; i < LASER_COUNT; i++, offset += 12) {
		if(data[offset] == F_FREE) {
			if(!t2replay_bytes_zero(data + offset, 12)) {
				return false;
			}
			continue;
		}
		if(
			!t2replay_checkpoint_entity_flag_valid(data[offset]) ||
			(data[offset + 1] < LASER_PHASE_WAIT) ||
			(data[offset + 1] > LASER_PHASE_DONE)
		) {
			return false;
		}
	}
	return true;
}

static bool t2replay_checkpoint_enemy_payload_valid(
	const uint8_t far *data
)
{
	unsigned offset = 4;
	unsigned i;
	int16_t value;

	if(
		(data[0] > ENEMY_SCRIPT_COUNT) || (data[1] > ENEMY_COUNT) ||
		(data[2] > T2RCCB_LIVE) || (data[3] > T2RCCB_LIVE) ||
		(data[2] != data[3])
	) {
		return false;
	}
	for(i = 0; i < ENEMY_COUNT; i++, offset += 36) {
		if(data[offset + 14] == F_FREE) {
			if(!t2replay_bytes_zero(data + offset, 36)) {
				return false;
			}
			continue;
		}
		value = static_cast<int16_t>(
			t2replay_checkpoint_get_u16(data, offset + 8)
		);
		if((value < 0) || (value >= ENEMY_SCRIPT_SIZE)) {
			return false;
		}
		value = static_cast<int16_t>(
			t2replay_checkpoint_get_u16(data, offset + 12)
		);
		if(
			(value < 0) || (value >= ENEMY_TEMPLATE_COUNT) ||
			!t2replay_checkpoint_entity_flag_valid(data[offset + 14]) ||
			(data[offset + 16] > 1) ||
			(t2replay_checkpoint_get_u16(data, offset + 19) > 2) ||
			(t2replay_checkpoint_get_u16(data, offset + 23) > 1) ||
			(data[offset + 29] > 1) || (data[offset + 32] > 1) ||
			(data[offset + 33] > 1) ||
			!t2replay_checkpoint_bullet_motion_valid(data[offset + 34])
		) {
			return false;
		}
	}
	return true;
}

static bool t2replay_checkpoint_effect_payload_valid(
	const uint8_t far *data
)
{
	// item_skill is a native signed 16-bit accumulator at bytes 11..12.
	// Every bit pattern is a representable native value, so it has no narrower
	// semantic range than its ABI representation. Item records follow it.
	unsigned offset = 15;
	unsigned i;

	if((data[2] > 1) || (data[3] >= 10)) {
		return false;
	}
	for(i = 0; i < ITEM_COUNT; i++, offset += 16) {
		if(data[offset] == F_FREE) {
			if(!t2replay_bytes_zero(data + offset, 16)) {
				return false;
			}
			continue;
		}
		if(
			!t2replay_checkpoint_entity_flag_valid(data[offset]) ||
			(data[offset + 1] >= IT_COUNT)
		) {
			return false;
		}
	}
	for(i = 0; i < SPARK_COUNT; i++, offset += 18) {
		if(data[offset] == F_FREE) {
			if(!t2replay_bytes_zero(data + offset, 18)) {
				return false;
			}
			continue;
		}
		if(
			!t2replay_checkpoint_entity_flag_valid(data[offset]) ||
			((data[offset + 14] != SRA_DOT) &&
				(data[offset + 14] != SRA_SPRITE)) ||
			((data[offset + 17] != SRA_DOT) &&
				(data[offset + 17] != SRA_SPRITE))
		) {
			return false;
		}
	}
	if(
		(t2replay_checkpoint_get_u16(data, offset) >= SPARK_COUNT) ||
		(data[offset + 2] == 0) || (data[offset + 3] == 0) ||
		(data[offset + 6] > V_WHITE)
	) {
		return false;
	}
	offset += 7;
	for(i = 0; i < POINTNUM_COUNT; i++, offset += 10) {
		if(data[offset + 8] == F_FREE) {
			if(!t2replay_bytes_zero(data + offset, 10)) {
				return false;
			}
			continue;
		}
		if(!t2replay_checkpoint_entity_flag_valid(data[offset + 8])) {
			return false;
		}
	}
	return (
		(data[offset + 0] < POINTNUM_CELS) &&
		(data[offset + 1] < POINTNUM_CELS)
	);
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
			(data[33] <= 9) &&
			(static_cast<int8_t>(data[42]) >= -1) &&
			(static_cast<int8_t>(data[42]) <= 5) &&
			(static_cast<int8_t>(data[43]) >= -1) &&
			(static_cast<int8_t>(data[43]) <= 5) &&
			(data[44] <= POWER_MAX) && (data[45] <= 1)
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

	case T2RCGI_PLAYER:
		return t2replay_checkpoint_player_payload_valid(data);

	case T2RCGI_BOMB:
		return t2replay_checkpoint_bomb_payload_valid(data);

	case T2RCGI_BULLET:
		return t2replay_checkpoint_bullet_payload_valid(data);

	case T2RCGI_LASER:
		return t2replay_checkpoint_laser_payload_valid(data);

	case T2RCGI_ENEMY:
		return t2replay_checkpoint_enemy_payload_valid(data);

	case T2RCGI_EFFECT:
		return t2replay_checkpoint_effect_payload_valid(data);

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
	if(
		(container[t2replay_checkpoint_payload_offset(T2RCGI_LASER)] ==
			T2RCCB_LIVE) &&
		(container[t2replay_checkpoint_payload_offset(T2RCGI_IDENTITY)] != 3) &&
		(container[t2replay_checkpoint_payload_offset(T2RCGI_IDENTITY)] != 5)
	) {
		return false;
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

#if T2REPLAY_EXACT_APPLY
	if(t2replay_exact_pending) {
		if(!t2replay_exact_apply_at_loop_top()) {
			t2replay_fail();
		}
		return;
	}
#endif
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
	t2replay_stage_seek_fn[0] = 'T';
	t2replay_stage_seek_fn[1] = 'H';
	t2replay_stage_seek_fn[2] = '2';
	t2replay_stage_seek_fn[3] = 'R';
	t2replay_stage_seek_fn[4] = '0';
	t2replay_stage_seek_fn[5] = '0';
	t2replay_stage_seek_fn[6] = '.';
	t2replay_stage_seek_fn[7] = 'R';
	t2replay_stage_seek_fn[8] = 'S';
	t2replay_stage_seek_fn[9] = 'K';
	t2replay_stage_seek_fn[10] = '\0';
#if T2REPLAY_EXACT_APPLY
	t2replay_exact_request_fn[0] = 'T';
	t2replay_exact_request_fn[1] = '2';
	t2replay_exact_request_fn[2] = 'X';
	t2replay_exact_request_fn[3] = 'A';
	t2replay_exact_request_fn[4] = 'P';
	t2replay_exact_request_fn[5] = '1';
	t2replay_exact_request_fn[6] = '.';
	t2replay_exact_request_fn[7] = 'B';
	t2replay_exact_request_fn[8] = 'I';
	t2replay_exact_request_fn[9] = 'N';
	t2replay_exact_request_fn[10] = '\0';
	t2replay_exact_diag_fn[0] = 'T';
	t2replay_exact_diag_fn[1] = '2';
	t2replay_exact_diag_fn[2] = 'X';
	t2replay_exact_diag_fn[3] = 'D';
	t2replay_exact_diag_fn[4] = 'I';
	t2replay_exact_diag_fn[5] = 'A';
	t2replay_exact_diag_fn[6] = 'G';
	t2replay_exact_diag_fn[7] = '.';
	t2replay_exact_diag_fn[8] = 'B';
	t2replay_exact_diag_fn[9] = 'I';
	t2replay_exact_diag_fn[10] = 'N';
	t2replay_exact_diag_fn[11] = '\0';
	t2replay_public_seek_request_fn[0] = 'T';
	t2replay_public_seek_request_fn[1] = '2';
	t2replay_public_seek_request_fn[2] = 'R';
	t2replay_public_seek_request_fn[3] = 'S';
	t2replay_public_seek_request_fn[4] = 'Q';
	t2replay_public_seek_request_fn[5] = '1';
	t2replay_public_seek_request_fn[6] = '.';
	t2replay_public_seek_request_fn[7] = 'B';
	t2replay_public_seek_request_fn[8] = 'I';
	t2replay_public_seek_request_fn[9] = 'N';
	t2replay_public_seek_request_fn[10] = '\0';
	t2replay_public_seek_sidecar_fn[0] = 'T';
	t2replay_public_seek_sidecar_fn[1] = 'H';
	t2replay_public_seek_sidecar_fn[2] = '2';
	t2replay_public_seek_sidecar_fn[3] = 'R';
	t2replay_public_seek_sidecar_fn[4] = '0';
	t2replay_public_seek_sidecar_fn[5] = '0';
	t2replay_public_seek_sidecar_fn[6] = '.';
	t2replay_public_seek_sidecar_fn[7] = 'R';
	t2replay_public_seek_sidecar_fn[8] = 'S';
	t2replay_public_seek_sidecar_fn[9] = 'K';
	t2replay_public_seek_sidecar_fn[10] = '\0';
	t2xobs_req_fn[0] = 'T';
	t2xobs_req_fn[1] = '2';
	t2xobs_req_fn[2] = 'X';
	t2xobs_req_fn[3] = 'O';
	t2xobs_req_fn[4] = 'B';
	t2xobs_req_fn[5] = 'Q';
	t2xobs_req_fn[6] = '.';
	t2xobs_req_fn[7] = 'B';
	t2xobs_req_fn[8] = 'I';
	t2xobs_req_fn[9] = 'N';
	t2xobs_req_fn[10] = '\0';
	t2xobs_out_fn[0] = 'T';
	t2xobs_out_fn[1] = '2';
	t2xobs_out_fn[2] = 'X';
	t2xobs_out_fn[3] = 'O';
	t2xobs_out_fn[4] = 'B';
	t2xobs_out_fn[5] = 'S';
	t2xobs_out_fn[6] = '.';
	t2xobs_out_fn[7] = 'B';
	t2xobs_out_fn[8] = 'I';
	t2xobs_out_fn[9] = 'N';
	t2xobs_out_fn[10] = '\0';
#endif
	t2replay_paths_ready = true;
}

static void t2replay_save_request_fn_set(char *fn)
{
	fn[0] = 'T';
	fn[1] = '2';
	fn[2] = 'R';
	fn[3] = 'S';
	fn[4] = 'A';
	fn[5] = 'V';
	fn[6] = '.';
	fn[7] = 'C';
	fn[8] = 'F';
	fn[9] = 'G';
	fn[10] = '\0';
}

static void t2replay_handoff_fn_set(char *fn)
{
	fn[0] = 'T';
	fn[1] = '2';
	fn[2] = 'R';
	fn[3] = 'H';
	fn[4] = 'A';
	fn[5] = 'N';
	fn[6] = 'D';
	fn[7] = '.';
	fn[8] = 'B';
	fn[9] = 'I';
	fn[10] = 'N';
	fn[11] = '\0';
}

static void t2replay_slot_set(uint8_t slot)
{
	t2replay_slot_fn[0] = 'T';
	t2replay_slot_fn[1] = 'H';
	t2replay_slot_fn[2] = '2';
	t2replay_slot_fn[3] = 'R';
	t2replay_slot_fn[4] = static_cast<char>('0' + (slot / 10));
	t2replay_slot_fn[5] = static_cast<char>('0' + (slot % 10));
	t2replay_slot_fn[6] = '.';
	t2replay_slot_fn[7] = 'R';
	t2replay_slot_fn[8] = 'P';
	t2replay_slot_fn[9] = 'Y';
	t2replay_slot_fn[10] = '\0';
	t2replay_stage_seek_fn[0] = 'T';
	t2replay_stage_seek_fn[1] = 'H';
	t2replay_stage_seek_fn[2] = '2';
	t2replay_stage_seek_fn[3] = 'R';
	t2replay_stage_seek_fn[4] = t2replay_slot_fn[4];
	t2replay_stage_seek_fn[5] = t2replay_slot_fn[5];
	t2replay_stage_seek_fn[6] = '.';
	t2replay_stage_seek_fn[7] = 'R';
	t2replay_stage_seek_fn[8] = 'S';
	t2replay_stage_seek_fn[9] = 'K';
	t2replay_stage_seek_fn[10] = '\0';
#if T2REPLAY_EXACT_APPLY
	t2replay_public_seek_sidecar_fn[4] = t2replay_slot_fn[4];
	t2replay_public_seek_sidecar_fn[5] = t2replay_slot_fn[5];
#endif
}

static void t2replay_temp_set(void)
{
	t2replay_slot_fn[0] = 'T';
	t2replay_slot_fn[1] = '2';
	t2replay_slot_fn[2] = 'R';
	t2replay_slot_fn[3] = 'P';
	t2replay_slot_fn[4] = 'Y';
	t2replay_slot_fn[5] = '.';
	t2replay_slot_fn[6] = 'T';
	t2replay_slot_fn[7] = 'M';
	t2replay_slot_fn[8] = 'P';
	t2replay_slot_fn[9] = '\0';
	t2replay_stage_seek_fn[0] = 'T';
	t2replay_stage_seek_fn[1] = '2';
	t2replay_stage_seek_fn[2] = 'R';
	t2replay_stage_seek_fn[3] = 'P';
	t2replay_stage_seek_fn[4] = 'Y';
	t2replay_stage_seek_fn[5] = '.';
	t2replay_stage_seek_fn[6] = 'R';
	t2replay_stage_seek_fn[7] = 'S';
	t2replay_stage_seek_fn[8] = 'K';
	t2replay_stage_seek_fn[9] = '\0';
	t2replay_stage_seek_fn[10] = '\0';
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

static void t2replay_dos_flush(void)
{
	_asm {
		mov	ah, 0Dh
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

#if T2REPLAY_SAVESTATE_ACCEPTANCE
static const char T2SAVESTATE_ACCEPTANCE_FN[] = "T2SGA.BIN";

static void t2savestate_acceptance_clear(
	t2savestate_acceptance_record_t *record
)
{
	uint8_t *p = reinterpret_cast<uint8_t *>(record);
	unsigned i;

	for(i = 0; i < sizeof(*record); i++) {
		p[i] = 0;
	}
}

static uint16_t t2savestate_acceptance_checksum(
	const t2savestate_acceptance_record_t *record
)
{
	const uint8_t *p = reinterpret_cast<const uint8_t *>(record);
	uint16_t sum = 0;
	unsigned i;

	for(i = 0; i < sizeof(*record); i++) {
		sum = static_cast<uint16_t>(sum + p[i]);
	}
	return sum;
}

// The trace is evidence for a cache-rollback detector, so AH=0Dh alone is not
// strong enough. Match the guard's PSP-wide commit without making the trace a
// production dependency.
static void t2savestate_acceptance_commit(void)
{
	uint16_t dpl[11];
	uint16_t psp;
	unsigned i;

	for(i = 0; i < 11; i++) {
		dpl[i] = 0;
	}
	_asm {
		mov ah, 51h
		int 21h
		mov psp, bx
	}
	dpl[10] = psp;
	_asm {
		push bp
		push si
		push di
		push es
		push ds
		push ss
		pop  ds
		lea  dx, dpl
		mov  ax, 5D01h
		int  21h
		pop  ds
		pop  es
		pop  di
		pop  si
		pop  bp
	}
}

static void t2savestate_acceptance_emit(
	uint8_t event, uint8_t checkpoint_result
)
{
	t2savestate_acceptance_record_t record;
	int fd;

	t2savestate_acceptance_clear(&record);
	record.magic[0] = 'T'; record.magic[1] = '2';
	record.magic[2] = 'S'; record.magic[3] = 'G';
	record.magic[4] = 'A'; record.magic[5] = '0';
	record.magic[6] = '0'; record.magic[7] = '1';
	record.schema = T2SAVESTATE_ACCEPTANCE_SCHEMA;
	record.event = event;
	record.checkpoint_result = checkpoint_result;
	record.guard_flags = replay_protect_blocked() ?
		REPLAY_PROTECT_FLAG_INVALID : 0;
	record.checksum = 0;
	record.checksum = t2savestate_acceptance_checksum(&record);
	fd = t2replay_dos_create(T2SAVESTATE_ACCEPTANCE_FN);
	if(fd < 0) {
		return;
	}
	if(t2replay_dos_write(fd, &record, sizeof(record)) != sizeof(record)) {
		t2replay_dos_close(fd);
		return;
	}
	t2replay_dos_close(fd);
	t2replay_dos_flush();
	t2savestate_acceptance_commit();
}

static bool t2savestate_acceptance_begin(void)
{
	bool ok = replay_protect_begin();

	t2savestate_acceptance_emit(T2SAE_BEGIN, ok);
	return ok;
}

static bool t2savestate_acceptance_checkpoint(uint8_t event)
{
	bool ok = replay_protect_checkpoint(T2REPLAY_GUARD_EVENT(event));

	t2savestate_acceptance_emit(event, ok);
	return ok;
}

static void t2savestate_acceptance_observe(uint8_t event)
{
	t2savestate_acceptance_emit(event, 0xFF);
}

static void t2savestate_acceptance_end(void)
{
	t2savestate_acceptance_emit(T2SAE_END, 0xFF);
	replay_protect_end();
}
#endif

static bool t2replay_handoff_witness_write(
	const void far *payload, unsigned size
)
{
	char fn[12];
	int fd;
	bool ok;

	// The target DOS does not reliably expose a newly closed directory entry
	// across execl() after AH=0Dh alone. This second create is therefore part of
	// the handoff itself, independent of optional diagnostics.
	t2replay_dos_flush();
	t2replay_handoff_fn_set(fn);
	fd = t2replay_dos_create(fn);
	if(fd < 0) {
		return false;
	}
	ok = (t2replay_dos_write(fd, payload, size) == size);
	t2replay_dos_close(fd);
	if(!ok) {
		t2replay_dos_delete(fn);
	}
	return ok;
}

static void t2replay_pending_files_delete(void)
{
	char request_fn[11];

	t2replay_save_request_fn_set(request_fn);
	t2replay_dos_delete(request_fn);
	t2replay_dos_delete(t2replay_slot_fn);
	t2replay_dos_delete(t2replay_stage_seek_fn);
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

static bool t2replay_save_request_read(t2replay_save_request_t far *request)
{
	char request_fn[11];
	uint32_t file_size;
	uint32_t stored;
	uint32_t computed;
	int fd;
	unsigned read;

	t2replay_save_request_fn_set(request_fn);
	fd = t2replay_dos_open(request_fn, T2REPLAY_DOS_ACCESS_READ);
	if(fd < 0) {
		return false;
	}
	t2replay_memclear(request, sizeof(*request));
	read = t2replay_dos_read(fd, request, sizeof(*request));
	if(!t2replay_dos_size(fd, &file_size)) {
		file_size = 0;
	}
	t2replay_dos_close(fd);
	stored = request->checksum;
	request->checksum = 0;
	computed = t2replay_fnv1a(T2REPLAY_FNV1A_BASIS, request, sizeof(*request));
	request->checksum = stored;
	return (
		(read == sizeof(*request)) &&
		(file_size == sizeof(*request)) &&
		(request->magic[0] == 'T') &&
		(request->magic[1] == '2') &&
		(request->magic[2] == 'R') &&
		(request->magic[3] == 'S') &&
		(request->magic[4] == 'A') &&
		(request->magic[5] == 'V') &&
		(request->magic[6] == '1') &&
		(request->magic[7] == '\0') &&
		(request->schema == T2REPLAY_SAVE_REQUEST_SCHEMA) &&
		(request->source >= T2REPLAY_SAVE_REQUEST_GAME_OVER) &&
		(request->source <= T2REPLAY_SAVE_REQUEST_MENU_RETURN) &&
		(request->reserved == 0) &&
		(request->replay_header_checksum != 0) &&
		(stored == computed)
	);
}

static bool t2replay_save_request_write(uint8_t source)
{
	char request_fn[11];
	t2replay_save_request_t request;
	int fd;
	bool ok;

	t2replay_save_request_fn_set(request_fn);
	t2replay_memclear(&request, sizeof(request));
	request.magic[0] = 'T';
	request.magic[1] = '2';
	request.magic[2] = 'R';
	request.magic[3] = 'S';
	request.magic[4] = 'A';
	request.magic[5] = 'V';
	request.magic[6] = '1';
	request.magic[7] = '\0';
	request.schema = T2REPLAY_SAVE_REQUEST_SCHEMA;
	request.source = source;
	request.replay_header_checksum = t2replay_header.header_checksum;
	request.checksum = t2replay_fnv1a(
		T2REPLAY_FNV1A_BASIS, &request, sizeof(request)
	);
	t2replay_dos_delete(request_fn);
	fd = t2replay_dos_create(request_fn);
	if(fd < 0) {
		return false;
	}
	ok = (
		t2replay_dos_write(fd, &request, sizeof(request)) == sizeof(request)
	);
	t2replay_dos_close(fd);
	if(ok) {
		ok = t2replay_handoff_witness_write(&request, sizeof(request));
	}
	if(!ok) {
		t2replay_dos_delete(request_fn);
	}
	return ok;
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

static uint16_t t2replay_reserved_u16_get(unsigned offset)
{
	return static_cast<uint16_t>(
		t2replay_header.reserved[offset] |
		(static_cast<uint16_t>(t2replay_header.reserved[offset + 1]) << 8)
	);
}

static void t2replay_reserved_u16_set(unsigned offset, uint16_t value)
{
	t2replay_header.reserved[offset] = static_cast<uint8_t>(value);
	t2replay_header.reserved[offset + 1] = static_cast<uint8_t>(value >> 8);
}

static bool t2replay_dos_datetime_valid(void)
{
	uint16_t date = t2replay_reserved_u16_get(
		T2REPLAY_RESERVED_DOS_DATE_OFFSET
	);
	uint16_t time = t2replay_reserved_u16_get(
		T2REPLAY_RESERVED_DOS_TIME_OFFSET
	);

	// Zero is the compatible representation used by T2RPY1/2 and early T2RPY3
	// captures. New captures store an ordinary packed DOS date and time.
	if((date == 0) && (time == 0)) {
		return true;
	}
	return (
		(((date >> 5) & 0x0F) >= 1) &&
		(((date >> 5) & 0x0F) <= 12) &&
		((date & 0x1F) >= 1) &&
		((date & 0x1F) <= 31) &&
		((time >> 11) <= 23) &&
		(((time >> 5) & 0x3F) <= 59) &&
		((time & 0x1F) <= 29)
	);
}

// The name is written only by OP after terminal capture. An all-zero field is
// the pre-name pending state and also keeps earlier T2RPY1 files playable.
// Stored glyphs mirror the native TH02 high-score alphabet, excluding its
// navigation-only left/right/end cells.
static bool t2replay_name_glyph_valid(uint8_t glyph)
{
	return (
		((glyph >= 0xA0) && (glyph <= 0xC3)) ||
		(glyph == 0x02) ||
		(glyph == 0x03) ||
		((glyph >= 0xDA) && (glyph <= 0xDE)) ||
		((glyph >= 0xE0) && (glyph <= 0xE4))
	);
}

static bool t2replay_name_valid(const uint8_t far *name)
{
	unsigned i;
	bool all_zero = true;

	for(i = 0; i < T2REPLAY_NAME_LEN; i++) {
		if(name[i] != 0) {
			all_zero = false;
		}
	}
	if(all_zero) {
		return true;
	}
	for(i = 0; i < T2REPLAY_NAME_LEN; i++) {
		if(!t2replay_name_glyph_valid(name[i])) {
			return false;
		}
	}
	return true;
}

// Exact-restore schema 1 is a deliberately non-applying admission gate. Its
// directory contains every future group, but no actor payload is emitted or
// consumed before the owning codecs and redraw hooks exist.
#define T2REC_HEADER_TOTAL_SIZE 0x10
#define T2REC_HEADER_SOURCE_FINGERPRINT 0x14
#define T2REC_HEADER_GROUP_MASK 0x18
#define T2REC_HEADER_BOUNDARY_GENERATION 0x1C
#define T2REC_HEADER_RULESET 0x1D
#define T2REC_HEADER_STAGE_ID 0x1E
#define T2REC_HEADER_SHOTTYPE 0x1F
#define T2REC_HEADER_RANK 0x20
#define T2REC_HEADER_REDUCE_EFFECTS 0x21
#define T2REC_HEADER_ACTOR_TAG 0x22
#define T2REC_HEADER_ACTOR_MODE 0x23
#define T2REC_HEADER_STAGE_FX_TAG 0x24
#define T2REC_HEADER_CALLBACK_PROFILE 0x25
#define T2REC_HEADER_RESOURCE_ID 0x26
#define T2REC_HEADER_INPUT_SEMANTICS 0x27
#define T2REC_HEADER_DECODED_SIZE 0x28
#define T2REC_HEADER_CONTAINER_CHECKSUM 0x2C

static bool t2replay_exact_checkpoint_magic_matches(const uint8_t far *magic)
{
	return (
		(magic[0] == 'T') &&
		(magic[1] == '2') &&
		(magic[2] == 'X') &&
		(magic[3] == 'C') &&
		(magic[4] == 'K') &&
		(magic[5] == '1') &&
		(magic[6] == '\0') &&
		(magic[7] == '\0')
	);
}

static uint32_t t2replay_exact_checkpoint_checksum(
	const uint8_t far *data, uint32_t size
)
{
	uint32_t hash = T2REPLAY_FNV1A_BASIS;
	uint32_t offset;
	uint8_t byte;

	for(offset = 0; offset < size; offset++) {
		byte = (
			(offset >= T2REC_HEADER_CONTAINER_CHECKSUM) &&
			(offset < (T2REC_HEADER_CONTAINER_CHECKSUM + 4))
		) ? 0 : data[offset];
		hash = t2replay_fnv1a(hash, &byte, sizeof(byte));
	}
	return hash;
}

static bool t2replay_exact_actor_stage_valid(uint8_t stage, uint8_t actor)
{
	switch(actor) {
	case T2REAT_NONE:
		return true;
	case T2REAT_S1_MIDBOSS:
	case T2REAT_S1_RIKA:
		return (stage == 0);
	case T2REAT_S2_MIDBOSS:
	case T2REAT_S2_MEIRA:
		return (stage == 1);
	case T2REAT_S3_MIDBOSS:
	case T2REAT_S3_STONES:
		return (stage == 2);
	case T2REAT_S4_MIDBOSS:
	case T2REAT_S4_MARISA:
		return (stage == 3);
	case T2REAT_S5_MIMA:
		return (stage == 4);
	case T2REAT_EX_MIDBOSS:
	case T2REAT_EX_SIGMA:
		return (stage == 5);
	default:
		return false;
	}
}

static bool t2replay_exact_stage_fx_valid(uint8_t stage, uint8_t stage_fx)
{
	switch(stage_fx) {
	case T2RESFT_NONE:
		return true;
	case T2RESFT_S1_SCENERY:
		return (stage == 0);
	case T2RESFT_S2_SCENERY:
		return (stage == 1);
	case T2RESFT_S3_RING:
		return (stage == 2);
	case T2RESFT_S4_MARISA_FIELD:
		return (stage == 3);
	case T2RESFT_S5_MIMA_FIELD:
		return (stage == 4);
	case T2RESFT_EX_SIGMA_FIELD:
		return (stage == 5);
	default:
		return false;
	}
}

static uint8_t t2replay_exact_callback_profile_expected(
	uint8_t stage, uint8_t actor
)
{
	switch(actor) {
	case T2REAT_NONE:
		switch(stage) {
		case 0: return T2RECP_S1_SCROLL;
		case 1: return T2RECP_S2_SCROLL;
		case 2: return T2RECP_S3_SCROLL;
		case 3: return T2RECP_S4_SCROLL;
		case 4: return T2RECP_S5_SCROLL;
		case 5: return T2RECP_EX_SCROLL;
		}
		break;
	case T2REAT_S1_MIDBOSS: return T2RECP_S1_MIDBOSS;
	case T2REAT_S1_RIKA: return T2RECP_S1_RIKA;
	case T2REAT_S2_MIDBOSS: return T2RECP_S2_MIDBOSS;
	case T2REAT_S2_MEIRA: return T2RECP_S2_MEIRA;
	case T2REAT_S3_MIDBOSS: return T2RECP_S3_MIDBOSS;
	case T2REAT_S3_STONES: return T2RECP_S3_STONES;
	case T2REAT_S4_MIDBOSS: return T2RECP_S4_MIDBOSS;
	case T2REAT_S4_MARISA: return T2RECP_S4_MARISA;
	case T2REAT_S5_MIMA: return T2RECP_S5_MIMA;
	case T2REAT_EX_MIDBOSS: return T2RECP_EX_MIDBOSS;
	case T2REAT_EX_SIGMA: return T2RECP_EX_SIGMA;
	}
	return 0xFF;
}

static bool t2replay_exact_checkpoint_tags_valid(const uint8_t far *data)
{
	uint8_t stage = data[T2REC_HEADER_STAGE_ID];
	uint8_t actor = data[T2REC_HEADER_ACTOR_TAG];
	uint8_t actor_mode = data[T2REC_HEADER_ACTOR_MODE];

	if(
		(stage >= T2REPLAY_STAGE_COUNT) ||
		(data[T2REC_HEADER_SHOTTYPE] >= SHOTTYPE_COUNT) ||
		(data[T2REC_HEADER_RANK] > RANK_EXTRA) ||
		((stage == (T2REPLAY_STAGE_COUNT - 1)) !=
		 (data[T2REC_HEADER_RANK] == RANK_EXTRA)) ||
		(data[T2REC_HEADER_REDUCE_EFFECTS] > 1) ||
		(data[T2REC_HEADER_RULESET] != T2REPLAY_RULESET_STOCK) ||
		(data[T2REC_HEADER_INPUT_SEMANTICS] !=
		 T2REPLAY_INPUT_SEMANTICS_KEY_DET) ||
		(data[T2REC_HEADER_RESOURCE_ID] != stage) ||
		!t2replay_exact_actor_stage_valid(stage, actor) ||
		!t2replay_exact_stage_fx_valid(
			stage, data[T2REC_HEADER_STAGE_FX_TAG]
		) ||
		(data[T2REC_HEADER_CALLBACK_PROFILE] !=
		 t2replay_exact_callback_profile_expected(stage, actor))
	) {
		return false;
	}
	if(actor == T2REAT_NONE) {
		return (actor_mode == T2REAM_SCROLL);
	}
	return (
		(actor_mode == T2REAM_ACTIVE) ||
		(actor_mode == T2REAM_DEFEAT)
	);
}

typedef char t2rec_actor_core_wire_size_check[
	(TH02_ACTOR_CORE_WIRE_SIZE == T2REPLAY_EXACT_ACTOR_CORE_SIZE) ? 1 : -1
];
typedef char t2rec_s5_mima_wire_size_check[
	(TH02_S5_MIMA_WIRE_SIZE == T2REPLAY_EXACT_S5_MIMA_SIZE) ? 1 : -1
];
typedef char t2rec_s5_tile_wire_size_check[
	(TH02_S5_TILE_LOGIC_WIRE_SIZE ==
	 T2REPLAY_EXACT_S5_TILE_LOGIC_SIZE) ? 1 : -1
];
typedef char t2rec_s5_stage_fx_wire_size_check[
	(TH02_S5_MIMA_STAGE_FX_WIRE_SIZE ==
	 T2REPLAY_EXACT_S5MFX_SIZE) ? 1 : -1
];
typedef char t2rec_s5_palette_wire_size_check[
	(TH02_S5_MIMA_PALETTE_WIRE_SIZE ==
	 T2REPLAY_EXACT_S5_PALETTE_SIZE) ? 1 : -1
];
typedef char t2rec_s5_callback_wire_size_check[
	(TH02_S5_MIMA_CALLBACK_WIRE_SIZE ==
	 T2REPLAY_EXACT_S5_CALLBACK_SIZE) ? 1 : -1
];
typedef char t2rec_s5_redraw_wire_size_check[
	(TH02_S5_MIMA_REDRAW_WIRE_SIZE ==
	 T2REPLAY_EXACT_S5_REDRAW_SIZE) ? 1 : -1
];

static uint32_t t2replay_exact_s5_mima_capture_size(uint16_t schema)
{
	if(schema == T2REPLAY_EXACT_S5_MIMA_SCHEMA) {
		return T2REPLAY_EXACT_S5_MIMA_CAPTURE_SIZE;
	}
	if(schema == T2REPLAY_EXACT_S5_MIMA_TILE_SCHEMA) {
		return T2REPLAY_EXACT_S5_MIMA_TILE_CAPTURE_SIZE;
	}
	if(schema == T2REPLAY_EXACT_S5MFX_SCHEMA) {
		return T2REPLAY_EXACT_S5MFX_CAPTURE_SIZE;
	}
	if(schema == T2REPLAY_EXACT_S5PAL_SCHEMA) {
		return T2REPLAY_EXACT_S5PAL_CAPTURE_SIZE;
	}
	if(schema == T2REPLAY_EXACT_S5CBRD_SCHEMA) {
		return T2REPLAY_EXACT_S5CBRD_CAPTURE_SIZE;
	}
	return 0;
}

static uint32_t t2replay_exact_s5_mima_source_fingerprint(uint16_t schema)
{
	if(schema == T2REPLAY_EXACT_S5_MIMA_SCHEMA) {
		return T2REPLAY_EXACT_S5_MIMA_SOURCE_FINGERPRINT;
	}
	if(schema == T2REPLAY_EXACT_S5_MIMA_TILE_SCHEMA) {
		return T2REPLAY_EXACT_S5_MIMA_TILE_SOURCE_FINGERPRINT;
	}
	if(schema == T2REPLAY_EXACT_S5MFX_SCHEMA) {
		return T2REPLAY_EXACT_S5MFX_SOURCE_FINGERPRINT;
	}
	if(schema == T2REPLAY_EXACT_S5PAL_SCHEMA) {
		return T2REPLAY_EXACT_S5PAL_SOURCE_FINGERPRINT;
	}
	if(schema == T2REPLAY_EXACT_S5CBRD_SCHEMA) {
		return T2REPLAY_EXACT_S5CBRD_SOURCE_FINGERPRINT;
	}
	return 0;
}

static bool t2replay_exact_s5_mima_tile_present(uint16_t schema)
{
	return (
		(schema == T2REPLAY_EXACT_S5_MIMA_TILE_SCHEMA) ||
		(schema == T2REPLAY_EXACT_S5MFX_SCHEMA) ||
		(schema == T2REPLAY_EXACT_S5PAL_SCHEMA) ||
		(schema == T2REPLAY_EXACT_S5CBRD_SCHEMA)
	);
}

static bool t2replay_exact_s5_mima_stage_fx_present(uint16_t schema)
{
	return (
		(schema == T2REPLAY_EXACT_S5MFX_SCHEMA) ||
		(schema == T2REPLAY_EXACT_S5PAL_SCHEMA) ||
		(schema == T2REPLAY_EXACT_S5CBRD_SCHEMA)
	);
}

static bool t2replay_exact_s5_mima_palette_present(uint16_t schema)
{
	return (
		(schema == T2REPLAY_EXACT_S5PAL_SCHEMA) ||
		(schema == T2REPLAY_EXACT_S5CBRD_SCHEMA)
	);
}

static bool t2replay_exact_s5_mima_callback_redraw_present(uint16_t schema)
{
	return (schema == T2REPLAY_EXACT_S5CBRD_SCHEMA);
}

static void t2replay_exact_group_set(
	uint8_t far *envelope, uint8_t id, uint8_t flags,
	uint32_t payload_offset, uint32_t payload_size
)
{
	uint8_t far *group = envelope + T2REPLAY_EXACT_HEADER_SIZE +
		(static_cast<uint32_t>(id) * T2REPLAY_EXACT_GROUP_SIZE);

	group[T2RCK_GROUP_ID] = id;
	group[T2RCK_GROUP_SCHEMA] = T2REPLAY_EXACT_GROUP_SCHEMA;
	group[T2RCK_GROUP_CODEC] = T2RCC_RAW;
	group[T2RCK_GROUP_FLAGS] = flags;
	t2replay_checkpoint_put_u32(group, T2RCK_GROUP_OFFSET, payload_offset);
	t2replay_checkpoint_put_u32(group, T2RCK_GROUP_STORED_SIZE, payload_size);
	t2replay_checkpoint_put_u32(group, T2RCK_GROUP_DECODED_SIZE, payload_size);
	t2replay_checkpoint_put_u32(
		group, T2RCK_GROUP_CHECKSUM,
		t2replay_fnv1a(
			T2REPLAY_FNV1A_BASIS, envelope + payload_offset,
			static_cast<unsigned>(payload_size)
		)
	);
}

static bool t2replay_exact_s5_mima_group_valid(
	const uint8_t far *envelope, uint16_t schema, uint8_t id,
	uint32_t& payload_offset
)
{
	const uint8_t far *group = envelope + T2REPLAY_EXACT_HEADER_SIZE +
		(static_cast<uint32_t>(id) * T2REPLAY_EXACT_GROUP_SIZE);
	uint32_t payload_size;
	uint8_t flags = 0;

	if(id < T2REPLAY_CHECKPOINT_GROUP_COUNT) {
		payload_size = t2replay_checkpoint_group_size(id);
	} else if(id == T2RXGI_ACTOR_CORE) {
		payload_size = T2REPLAY_EXACT_ACTOR_CORE_SIZE;
	} else if(id == T2RXGI_ACTOR_STAGE) {
		payload_size = T2REPLAY_EXACT_S5_MIMA_SIZE;
	} else if(
		(id == T2RXGI_STAGE_FX) &&
		t2replay_exact_s5_mima_stage_fx_present(schema)
	) {
		payload_size = T2REPLAY_EXACT_S5MFX_SIZE;
	} else if(
		(id == T2RXGI_TILE_LOGIC) &&
		t2replay_exact_s5_mima_tile_present(schema)
	) {
		payload_size = T2REPLAY_EXACT_S5_TILE_LOGIC_SIZE;
	} else if(
		(id == T2RXGI_PALETTE) &&
		t2replay_exact_s5_mima_palette_present(schema)
	) {
		payload_size = T2REPLAY_EXACT_S5_PALETTE_SIZE;
	} else if(
		(id == T2RXGI_CALLBACKS) &&
		t2replay_exact_s5_mima_callback_redraw_present(schema)
	) {
		payload_size = T2REPLAY_EXACT_S5_CALLBACK_SIZE;
	} else if(
		(id == T2RXGI_REDRAW) &&
		t2replay_exact_s5_mima_callback_redraw_present(schema)
	) {
		payload_size = T2REPLAY_EXACT_S5_REDRAW_SIZE;
	} else {
		payload_size = 0;
		flags = T2REPLAY_EXACT_GROUP_FLAG_DEFERRED;
	}
	if(
		(group[T2RCK_GROUP_ID] != id) ||
		(group[T2RCK_GROUP_SCHEMA] != T2REPLAY_EXACT_GROUP_SCHEMA) ||
		(group[T2RCK_GROUP_CODEC] != T2RCC_RAW) ||
		(group[T2RCK_GROUP_FLAGS] != flags) ||
		(t2replay_checkpoint_get_u32(group, T2RCK_GROUP_OFFSET) !=
		 payload_offset) ||
		(t2replay_checkpoint_get_u32(group, T2RCK_GROUP_STORED_SIZE) !=
		 payload_size) ||
		(t2replay_checkpoint_get_u32(group, T2RCK_GROUP_DECODED_SIZE) !=
		 payload_size) ||
		(t2replay_checkpoint_get_u32(group, T2RCK_GROUP_CHECKSUM) !=
		 t2replay_fnv1a(
			T2REPLAY_FNV1A_BASIS, envelope + payload_offset,
			static_cast<unsigned>(payload_size)
		))
	) {
		return false;
	}
	if(id < T2REPLAY_CHECKPOINT_GROUP_COUNT) {
		if(!t2replay_checkpoint_group_payload_valid(
			id, envelope + payload_offset
		)) {
			return false;
		}
	} else if(id == T2RXGI_ACTOR_CORE) {
		if(!th02_actor_core_state_wire_valid(
			envelope + payload_offset, static_cast<uint16_t>(payload_size)
		)) {
			return false;
		}
	} else if(id == T2RXGI_ACTOR_STAGE) {
		if(!th02_s5_mima_state_wire_valid(
			envelope + payload_offset, static_cast<uint16_t>(payload_size)
		)) {
			return false;
		}
	} else if(
		(id == T2RXGI_STAGE_FX) &&
		t2replay_exact_s5_mima_stage_fx_present(schema)
	) {
		if(!th02_s5_mima_stage_fx_wire_valid(
			envelope + payload_offset, static_cast<uint16_t>(payload_size)
		)) {
			return false;
		}
	} else if(
		(id == T2RXGI_TILE_LOGIC) &&
		t2replay_exact_s5_mima_tile_present(schema)
	) {
		if(!th02_s5_tile_logic_wire_valid(
			envelope + payload_offset, static_cast<uint16_t>(payload_size)
		)) {
			return false;
		}
	} else if(
		(id == T2RXGI_PALETTE) &&
		t2replay_exact_s5_mima_palette_present(schema)
	) {
		if(!th02_s5_mima_palette_wire_valid(
			envelope + payload_offset, static_cast<uint16_t>(payload_size)
		)) {
			return false;
		}
	} else if(
		(id == T2RXGI_CALLBACKS) &&
		t2replay_exact_s5_mima_callback_redraw_present(schema)
	) {
		if(!th02_s5_mima_callback_wire_valid(
			envelope + payload_offset, static_cast<uint16_t>(payload_size)
		)) {
			return false;
		}
	} else if(
		(id == T2RXGI_REDRAW) &&
		t2replay_exact_s5_mima_callback_redraw_present(schema)
	) {
		if(!th02_s5_mima_redraw_wire_valid(
			envelope + payload_offset, static_cast<uint16_t>(payload_size)
		)) {
			return false;
		}
	}
	payload_offset += payload_size;
	return true;
}

static bool t2replay_exact_s5_mima_common_tags_valid(
	const uint8_t far *envelope
)
{
	const uint8_t far *identity = envelope + T2REPLAY_EXACT_CHECKPOINT_SIZE;
	const uint8_t far *stage_vm = identity;
	uint8_t group_id;

	for(group_id = 0; group_id < T2RCGI_STAGE_VM; group_id++) {
		stage_vm += t2replay_checkpoint_group_size(group_id);
	}
	// These common-world bytes are independently range-checked above. Bind the
	// fields that select a live Stage 5 Mima boundary to the outer declaration
	// so a valid foreign common snapshot cannot be relabeled as this actor.
	return (
		(identity[0] == envelope[T2REC_HEADER_STAGE_ID]) &&
		(identity[1] == envelope[T2REC_HEADER_SHOTTYPE]) &&
		(identity[2] == envelope[T2REC_HEADER_RANK]) &&
		(identity[3] == envelope[T2REC_HEADER_REDUCE_EFFECTS]) &&
		(stage_vm[0] == identity[0]) &&
		(stage_vm[1] == SP_BOSS)
	);
}

static bool t2replay_exact_s5_mima_tile_field_agree(
	const uint8_t far *envelope, uint16_t schema
)
{
	const uint8_t far *field = envelope + T2REPLAY_EXACT_CHECKPOINT_SIZE;
	const uint8_t far *tile;
	uint8_t group_id;

	if(!t2replay_exact_s5_mima_tile_present(schema)) {
		return true;
	}
	for(group_id = 0; group_id < T2RCGI_FIELD; group_id++) {
		field += t2replay_checkpoint_group_size(group_id);
	}
	tile = envelope + t2replay_checkpoint_get_u32(
		envelope + T2REPLAY_EXACT_HEADER_SIZE +
			(T2RXGI_TILE_LOGIC * T2REPLAY_EXACT_GROUP_SIZE),
		T2RCK_GROUP_OFFSET
	);
	// FIELD owns these two generic renderer switches. TILE_LOGIC repeats them
	// for a self-describing restore payload, so accept only one coherent value.
	return (
		(field[3] == tile[0]) &&
		(field[18] == tile[1])
	);
}

static const uint8_t far *t2replay_exact_s5_mima_group_payload(
	const uint8_t far *envelope, uint8_t group_id
)
{
	return envelope + t2replay_checkpoint_get_u32(
		envelope + T2REPLAY_EXACT_HEADER_SIZE +
			(static_cast<uint32_t>(group_id) * T2REPLAY_EXACT_GROUP_SIZE),
		T2RCK_GROUP_OFFSET
	);
}

static bool t2replay_exact_s5_mima_actor_groups_agree(
	const uint8_t far *envelope
)
{
	const uint8_t far *field = t2replay_exact_s5_mima_group_payload(
		envelope, T2RXGI_FIELD
	);
	return th02_s5_mima_actor_wire_agree(
		t2replay_exact_s5_mima_group_payload(envelope, T2RXGI_ACTOR_CORE),
		t2replay_exact_s5_mima_group_payload(envelope, T2RXGI_ACTOR_STAGE),
		field[0]
	);
}

static bool t2replay_exact_s5_mima_palette_actor_agree(
	const uint8_t far *envelope, uint16_t schema
)
{
	if(!t2replay_exact_s5_mima_palette_present(schema)) {
		return true;
	}
	return th02_s5_mima_palette_wire_agree(
		t2replay_exact_s5_mima_group_payload(envelope, T2RXGI_BOMB),
		t2replay_exact_s5_mima_group_payload(envelope, T2RXGI_ACTOR_CORE),
		t2replay_exact_s5_mima_group_payload(envelope, T2RXGI_ACTOR_STAGE),
		t2replay_exact_s5_mima_group_payload(envelope, T2RXGI_PALETTE)
	);
}

static bool t2replay_exact_s5_mima_callback_groups_agree(
	const uint8_t far *envelope, uint16_t schema
)
{
	if(!t2replay_exact_s5_mima_callback_redraw_present(schema)) {
		return true;
	}
	return th02_s5_mima_callback_wire_agree(
		envelope[T2REC_HEADER_CALLBACK_PROFILE],
		t2replay_exact_s5_mima_group_payload(envelope, T2RXGI_CALLBACKS),
		t2replay_exact_s5_mima_group_payload(envelope, T2RXGI_LASER),
		t2replay_exact_s5_mima_group_payload(envelope, T2RXGI_ENEMY)
	);
}

static enum t2rec_reject_t t2replay_exact_s5_mima_validate(
	const uint8_t far *envelope, uint32_t envelope_size, uint16_t schema
)
{
	uint32_t capture_size = t2replay_exact_s5_mima_capture_size(schema);
	uint32_t source_fingerprint =
		t2replay_exact_s5_mima_source_fingerprint(schema);
	uint32_t payload_offset;
	uint8_t group_id;

	if(
		(capture_size == 0) ||
		(envelope_size != capture_size) ||
		!t2replay_exact_checkpoint_magic_matches(envelope) ||
		(t2replay_checkpoint_get_u16(envelope, 8) != schema) ||
		(t2replay_checkpoint_get_u16(envelope, 10) !=
		 T2REPLAY_EXACT_HEADER_SIZE) ||
		(envelope[12] != 2) ||
		(envelope[13] != T2REPLAY_EXACT_GROUP_COUNT) ||
		(envelope[14] != T2REPLAY_EXACT_GROUP_SCHEMA) ||
		(envelope[15] != 0) ||
		(t2replay_checkpoint_get_u32(envelope, T2REC_HEADER_TOTAL_SIZE) !=
		 capture_size) ||
		(t2replay_checkpoint_get_u32(
			envelope, T2REC_HEADER_SOURCE_FINGERPRINT
		) != source_fingerprint) ||
		(t2replay_checkpoint_get_u32(envelope, T2REC_HEADER_GROUP_MASK) !=
		 T2REPLAY_EXACT_CHECKPOINT_GROUP_MASK) ||
		(envelope[T2REC_HEADER_BOUNDARY_GENERATION] !=
		 T2REPLAY_EXACT_BOUNDARY_GENERATION) ||
		(t2replay_checkpoint_get_u32(envelope, T2REC_HEADER_DECODED_SIZE) !=
		 (capture_size - T2REPLAY_EXACT_CHECKPOINT_SIZE))
	) {
		return T2REC_HEADER;
	}
	if(
		!t2replay_exact_checkpoint_tags_valid(envelope) ||
		(envelope[T2REC_HEADER_STAGE_ID] != 4) ||
		(envelope[T2REC_HEADER_ACTOR_TAG] != T2REAT_S5_MIMA) ||
		(envelope[T2REC_HEADER_ACTOR_MODE] != T2REAM_ACTIVE) ||
		(envelope[T2REC_HEADER_STAGE_FX_TAG] != T2RESFT_S5_MIMA_FIELD) ||
		(envelope[T2REC_HEADER_CALLBACK_PROFILE] != T2RECP_S5_MIMA) ||
		(envelope[T2REC_HEADER_RESOURCE_ID] != T2RERI_STAGE_5) ||
		!t2replay_exact_s5_mima_common_tags_valid(envelope)
	) {
		return T2REC_TAG;
	}
	payload_offset = T2REPLAY_EXACT_CHECKPOINT_SIZE;
	for(group_id = 0; group_id < T2REPLAY_EXACT_GROUP_COUNT;
		group_id++) {
		if(!t2replay_exact_s5_mima_group_valid(
			envelope, schema, group_id, payload_offset
		)) {
			return T2REC_DIRECTORY;
		}
	}
	if(!t2replay_exact_s5_mima_actor_groups_agree(envelope)) {
		return T2REC_DIRECTORY;
	}
	if(!t2replay_exact_s5_mima_tile_field_agree(envelope, schema)) {
		return T2REC_DIRECTORY;
	}
	if(!t2replay_exact_s5_mima_palette_actor_agree(envelope, schema)) {
		return T2REC_DIRECTORY;
	}
	if(!t2replay_exact_s5_mima_callback_groups_agree(envelope, schema)) {
		return T2REC_DIRECTORY;
	}
	if(
		(payload_offset != capture_size) ||
		(t2replay_checkpoint_get_u32(
			envelope, T2REC_HEADER_CONTAINER_CHECKSUM
		) != t2replay_exact_checkpoint_checksum(envelope, envelope_size))
	) {
		return T2REC_CHECKSUM;
	}
	return T2REC_DEFERRED_CODECS;
}

bool replay_exact_checkpoint_boundary_available(
	const struct t2rec_boundary_t *boundary,
	enum t2rec_reject_t *reason
)
{
	enum t2rec_reject_t result = T2REC_DEFERRED_CODECS;

	if(boundary == 0) {
		result = T2REC_BOUNDARY_NOT_LOOP_TOP;
	} else if(!boundary->at_ordinary_stage_loop_top) {
		result = T2REC_BOUNDARY_NOT_LOOP_TOP;
	} else if(!boundary->stage_init_complete) {
		result = T2REC_BOUNDARY_STAGE_INIT;
	} else if(boundary->input_sampled) {
		result = T2REC_BOUNDARY_INPUT_SAMPLED;
	} else if(boundary->pause_or_debounce_active) {
		result = T2REC_BOUNDARY_PAUSE;
	} else if(boundary->blocking_presentation_active) {
		result = T2REC_BOUNDARY_PRESENTATION;
	} else if(boundary->restore_or_redraw_active) {
		result = T2REC_BOUNDARY_RESTORE_OR_REDRAW;
	} else if(
		(boundary->stage_progression != SP_STAGE) &&
		(boundary->stage_progression != SP_BOSS)
	) {
		result = T2REC_BOUNDARY_STAGE_PROGRESSION;
	}
	if(reason != 0) {
		*reason = result;
	}
	return (result == T2REC_DEFERRED_CODECS);
}

enum t2rec_reject_t replay_exact_checkpoint_validate(
	const uint8_t far *envelope, uint32_t envelope_size,
	const struct t2rec_boundary_t *boundary
)
{
	enum t2rec_reject_t result;
	unsigned payload_offset;
	unsigned group_offset;
	uint8_t group_id;
	const uint8_t far *group;

	if(envelope == 0) {
		return T2REC_NULL_ENVELOPE;
	}
	if(!replay_exact_checkpoint_boundary_available(boundary, &result)) {
		return result;
	}
	if(
		(envelope_size >= T2REPLAY_EXACT_HEADER_SIZE) &&
		((t2replay_checkpoint_get_u16(envelope, 8) ==
		  T2REPLAY_EXACT_S5_MIMA_SCHEMA) ||
		 (t2replay_checkpoint_get_u16(envelope, 8) ==
		  T2REPLAY_EXACT_S5_MIMA_TILE_SCHEMA) ||
		 (t2replay_checkpoint_get_u16(envelope, 8) ==
		  T2REPLAY_EXACT_S5MFX_SCHEMA) ||
		 (t2replay_checkpoint_get_u16(envelope, 8) ==
		  T2REPLAY_EXACT_S5PAL_SCHEMA) ||
		 (t2replay_checkpoint_get_u16(envelope, 8) ==
		  T2REPLAY_EXACT_S5CBRD_SCHEMA))
	) {
		return t2replay_exact_s5_mima_validate(
			envelope, envelope_size, t2replay_checkpoint_get_u16(envelope, 8)
		);
	}
	if(
		(envelope_size != T2REPLAY_EXACT_CHECKPOINT_SIZE) ||
		!t2replay_exact_checkpoint_magic_matches(envelope) ||
		(t2replay_checkpoint_get_u16(envelope, 8) !=
		 T2REPLAY_EXACT_CHECKPOINT_SCHEMA) ||
		(t2replay_checkpoint_get_u16(envelope, 10) !=
		 T2REPLAY_EXACT_HEADER_SIZE) ||
		(envelope[12] != 2) ||
		(envelope[13] != T2REPLAY_EXACT_GROUP_COUNT) ||
		(envelope[14] != T2REPLAY_EXACT_GROUP_SCHEMA) ||
		(envelope[15] != 0) ||
		(t2replay_checkpoint_get_u32(envelope, T2REC_HEADER_TOTAL_SIZE) !=
		 T2REPLAY_EXACT_CHECKPOINT_SIZE) ||
		(t2replay_checkpoint_get_u32(
			envelope, T2REC_HEADER_SOURCE_FINGERPRINT
		) != T2REPLAY_EXACT_CHECKPOINT_SOURCE_FINGERPRINT) ||
		(t2replay_checkpoint_get_u32(envelope, T2REC_HEADER_GROUP_MASK) !=
		 T2REPLAY_EXACT_CHECKPOINT_GROUP_MASK) ||
		(envelope[T2REC_HEADER_BOUNDARY_GENERATION] !=
		 T2REPLAY_EXACT_BOUNDARY_GENERATION) ||
		(t2replay_checkpoint_get_u32(envelope, T2REC_HEADER_DECODED_SIZE) != 0)
	) {
		return T2REC_HEADER;
	}
	if(!t2replay_exact_checkpoint_tags_valid(envelope)) {
		return T2REC_TAG;
	}
	payload_offset = (
		T2REPLAY_EXACT_HEADER_SIZE +
		(T2REPLAY_EXACT_GROUP_COUNT * T2REPLAY_EXACT_GROUP_SIZE)
	);
	for(group_id = 0; group_id < T2REPLAY_EXACT_GROUP_COUNT;
		group_id++) {
		group_offset = (
			T2REPLAY_EXACT_HEADER_SIZE +
			(static_cast<unsigned>(group_id) *
			 T2REPLAY_EXACT_GROUP_SIZE)
		);
		group = envelope + group_offset;
		if(
			(group[T2RCK_GROUP_ID] != group_id) ||
			(group[T2RCK_GROUP_SCHEMA] !=
			 T2REPLAY_EXACT_GROUP_SCHEMA) ||
			(group[T2RCK_GROUP_CODEC] != T2RCC_RAW) ||
			(group[T2RCK_GROUP_FLAGS] != 0) ||
			(t2replay_checkpoint_get_u32(group, T2RCK_GROUP_OFFSET) !=
			 payload_offset) ||
			(t2replay_checkpoint_get_u32(group, T2RCK_GROUP_STORED_SIZE) != 0) ||
			(t2replay_checkpoint_get_u32(group, T2RCK_GROUP_DECODED_SIZE) != 0) ||
			(t2replay_checkpoint_get_u32(group, T2RCK_GROUP_CHECKSUM) !=
			 T2REPLAY_FNV1A_BASIS)
		) {
			return T2REC_DIRECTORY;
		}
	}
	if(t2replay_checkpoint_get_u32(
		envelope, T2REC_HEADER_CONTAINER_CHECKSUM
	) != t2replay_exact_checkpoint_checksum(envelope, envelope_size)) {
		return T2REC_CHECKSUM;
	}

	// No state has been written. A typed-codec/apply parcel may replace only
	// this final rejection after it has validated every payload and dependency.
	return T2REC_DEFERRED_CODECS;
}

static bool16 t2replay_exact_stage5_mima_capture_schema(
	uint8_t far *envelope, uint32_t envelope_size,
	const struct t2rec_boundary_t *boundary, uint16_t schema
)
{
	uint32_t capture_size = t2replay_exact_s5_mima_capture_size(schema);
	uint32_t source_fingerprint =
		t2replay_exact_s5_mima_source_fingerprint(schema);
	uint32_t payload_offset;
	uint32_t payload_size;
	uint8_t group_id;
	enum t2rec_reject_t reason;

	if(
		(envelope == 0) ||
		(capture_size == 0) ||
		(envelope_size != capture_size) ||
		!replay_exact_checkpoint_boundary_available(boundary, &reason) ||
		(stage_id != 4) || (stage_progression != SP_BOSS)
	) {
		return false;
	}
	t2replay_memclear(envelope, static_cast<unsigned>(envelope_size));
	envelope[0] = 'T'; envelope[1] = '2'; envelope[2] = 'X';
	envelope[3] = 'C'; envelope[4] = 'K'; envelope[5] = '1';
	t2replay_checkpoint_put_u16(
		envelope, 8, schema
	);
	t2replay_checkpoint_put_u16(
		envelope, 10, T2REPLAY_EXACT_HEADER_SIZE
	);
	envelope[12] = 2;
	envelope[13] = T2REPLAY_EXACT_GROUP_COUNT;
	envelope[14] = T2REPLAY_EXACT_GROUP_SCHEMA;
	t2replay_checkpoint_put_u32(
		envelope, T2REC_HEADER_TOTAL_SIZE,
		capture_size
	);
	t2replay_checkpoint_put_u32(
		envelope, T2REC_HEADER_SOURCE_FINGERPRINT,
		source_fingerprint
	);
	t2replay_checkpoint_put_u32(
		envelope, T2REC_HEADER_GROUP_MASK,
		T2REPLAY_EXACT_CHECKPOINT_GROUP_MASK
	);
	envelope[T2REC_HEADER_BOUNDARY_GENERATION] =
		T2REPLAY_EXACT_BOUNDARY_GENERATION;
	envelope[T2REC_HEADER_RULESET] = T2REPLAY_RULESET_STOCK;
	envelope[T2REC_HEADER_STAGE_ID] = 4;
	envelope[T2REC_HEADER_SHOTTYPE] = resident->shottype;
	envelope[T2REC_HEADER_RANK] = static_cast<uint8_t>(rank);
	envelope[T2REC_HEADER_REDUCE_EFFECTS] = (reduce_effects ? 1 : 0);
	envelope[T2REC_HEADER_ACTOR_TAG] = T2REAT_S5_MIMA;
	envelope[T2REC_HEADER_ACTOR_MODE] = T2REAM_ACTIVE;
	envelope[T2REC_HEADER_STAGE_FX_TAG] = T2RESFT_S5_MIMA_FIELD;
	envelope[T2REC_HEADER_CALLBACK_PROFILE] = T2RECP_S5_MIMA;
	envelope[T2REC_HEADER_RESOURCE_ID] = T2RERI_STAGE_5;
	envelope[T2REC_HEADER_INPUT_SEMANTICS] =
		T2REPLAY_INPUT_SEMANTICS_KEY_DET;
	t2replay_checkpoint_put_u32(
		envelope, T2REC_HEADER_DECODED_SIZE,
		(capture_size -
		 T2REPLAY_EXACT_CHECKPOINT_SIZE)
	);
	payload_offset = T2REPLAY_EXACT_CHECKPOINT_SIZE;
	for(group_id = 0; group_id < T2REPLAY_CHECKPOINT_GROUP_COUNT;
		group_id++) {
		payload_size = t2replay_checkpoint_group_size(group_id);
		if(
			(payload_size == 0) ||
			!t2replay_checkpoint_group_capture(
				group_id, envelope + payload_offset
			)
		) {
			return false;
		}
		t2replay_exact_group_set(
			envelope, group_id, 0, payload_offset, payload_size
		);
		payload_offset += payload_size;
	}
	if(!th02_actor_core_state_wire_capture(
		envelope + payload_offset, T2REPLAY_EXACT_ACTOR_CORE_SIZE
	)) {
		return false;
	}
	t2replay_exact_group_set(
		envelope, T2RXGI_ACTOR_CORE, 0, payload_offset,
		T2REPLAY_EXACT_ACTOR_CORE_SIZE
	);
	payload_offset += T2REPLAY_EXACT_ACTOR_CORE_SIZE;
	if(!th02_s5_mima_state_wire_capture(
		envelope + payload_offset, T2REPLAY_EXACT_S5_MIMA_SIZE
	)) {
		return false;
	}
	t2replay_exact_group_set(
		envelope, T2RXGI_ACTOR_STAGE, 0, payload_offset,
		T2REPLAY_EXACT_S5_MIMA_SIZE
	);
	payload_offset += T2REPLAY_EXACT_S5_MIMA_SIZE;
	if(t2replay_exact_s5_mima_stage_fx_present(schema)) {
		if(!th02_s5_mima_stage_fx_wire_capture(
			envelope + payload_offset,
			T2REPLAY_EXACT_S5MFX_SIZE
		)) {
			return false;
		}
		t2replay_exact_group_set(
			envelope, T2RXGI_STAGE_FX, 0, payload_offset,
			T2REPLAY_EXACT_S5MFX_SIZE
		);
		payload_offset += T2REPLAY_EXACT_S5MFX_SIZE;
	}
	if(t2replay_exact_s5_mima_tile_present(schema)) {
		if(!th02_s5_tile_logic_wire_capture(
			envelope + payload_offset, T2REPLAY_EXACT_S5_TILE_LOGIC_SIZE
		)) {
			return false;
		}
		t2replay_exact_group_set(
			envelope, T2RXGI_TILE_LOGIC, 0, payload_offset,
			T2REPLAY_EXACT_S5_TILE_LOGIC_SIZE
		);
		payload_offset += T2REPLAY_EXACT_S5_TILE_LOGIC_SIZE;
	}
	if(t2replay_exact_s5_mima_palette_present(schema)) {
		if(!th02_s5_mima_palette_wire_capture(
			envelope + payload_offset, T2REPLAY_EXACT_S5_PALETTE_SIZE
		)) {
			return false;
		}
		t2replay_exact_group_set(
			envelope, T2RXGI_PALETTE, 0, payload_offset,
			T2REPLAY_EXACT_S5_PALETTE_SIZE
		);
		payload_offset += T2REPLAY_EXACT_S5_PALETTE_SIZE;
	}
	if(t2replay_exact_s5_mima_callback_redraw_present(schema)) {
		if(!th02_s5_mima_callback_wire_capture(
			envelope + payload_offset, T2REPLAY_EXACT_S5_CALLBACK_SIZE,
			envelope[T2REC_HEADER_CALLBACK_PROFILE],
			envelope[T2REC_HEADER_SHOTTYPE],
			t2replay_exact_s5_mima_group_payload(envelope, T2RXGI_LASER),
			t2replay_exact_s5_mima_group_payload(envelope, T2RXGI_ENEMY)
		)) {
			return false;
		}
		t2replay_exact_group_set(
			envelope, T2RXGI_CALLBACKS, 0, payload_offset,
			T2REPLAY_EXACT_S5_CALLBACK_SIZE
		);
		payload_offset += T2REPLAY_EXACT_S5_CALLBACK_SIZE;
		if(!th02_s5_mima_redraw_wire_capture(
			envelope + payload_offset, T2REPLAY_EXACT_S5_REDRAW_SIZE
		)) {
			return false;
		}
		t2replay_exact_group_set(
			envelope, T2RXGI_REDRAW, 0, payload_offset,
			T2REPLAY_EXACT_S5_REDRAW_SIZE
		);
		payload_offset += T2REPLAY_EXACT_S5_REDRAW_SIZE;
	}
	for(group_id = T2RXGI_STAGE_FX;
		group_id < T2REPLAY_EXACT_GROUP_COUNT; group_id++) {
		if(
			(group_id == T2RXGI_STAGE_FX) &&
			t2replay_exact_s5_mima_stage_fx_present(schema)
		) {
			continue;
		}
		if(
			(group_id == T2RXGI_TILE_LOGIC) &&
			t2replay_exact_s5_mima_tile_present(schema)
		) {
			continue;
		}
		if(
			(group_id == T2RXGI_PALETTE) &&
			t2replay_exact_s5_mima_palette_present(schema)
		) {
			continue;
		}
		if(
			((group_id == T2RXGI_CALLBACKS) ||
			 (group_id == T2RXGI_REDRAW)) &&
			t2replay_exact_s5_mima_callback_redraw_present(schema)
		) {
			continue;
		}
		t2replay_exact_group_set(
			envelope, group_id, T2REPLAY_EXACT_GROUP_FLAG_DEFERRED,
			payload_offset, 0
		);
	}
	if(payload_offset != capture_size) {
		return false;
	}
	t2replay_checkpoint_put_u32(
		envelope, T2REC_HEADER_CONTAINER_CHECKSUM,
		t2replay_exact_checkpoint_checksum(envelope, envelope_size)
	);
	return (
		replay_exact_checkpoint_validate(envelope, envelope_size, boundary) ==
		T2REC_DEFERRED_CODECS
	);
}

bool16 replay_exact_stage5_mima_capture(
	uint8_t far *envelope, uint32_t envelope_size,
	const struct t2rec_boundary_t *boundary
)
{
	return t2replay_exact_stage5_mima_capture_schema(
		envelope, envelope_size, boundary, T2REPLAY_EXACT_S5_MIMA_SCHEMA
	);
}

bool16 replay_exact_stage5_mima_tile_capture(
	uint8_t far *envelope, uint32_t envelope_size,
	const struct t2rec_boundary_t *boundary
)
{
	return t2replay_exact_stage5_mima_capture_schema(
		envelope, envelope_size, boundary,
		T2REPLAY_EXACT_S5_MIMA_TILE_SCHEMA
	);
}

bool16 replay_exact_stage5_mima_stage_fx_capture(
	uint8_t far *envelope, uint32_t envelope_size,
	const struct t2rec_boundary_t *boundary
)
{
	return t2replay_exact_stage5_mima_capture_schema(
		envelope, envelope_size, boundary,
		T2REPLAY_EXACT_S5MFX_SCHEMA
	);
}

bool16 replay_exact_stage5_mima_palette_capture(
	uint8_t far *envelope, uint32_t envelope_size,
	const struct t2rec_boundary_t *boundary
)
{
	return t2replay_exact_stage5_mima_capture_schema(
		envelope, envelope_size, boundary, T2REPLAY_EXACT_S5PAL_SCHEMA
	);
}

bool16 replay_exact_stage5_mima_callback_redraw_capture(
	uint8_t far *envelope, uint32_t envelope_size,
	const struct t2rec_boundary_t *boundary
)
{
	return t2replay_exact_stage5_mima_capture_schema(
		envelope, envelope_size, boundary, T2REPLAY_EXACT_S5CBRD_SCHEMA
	);
}

#if T2REPLAY_EXACT_APPLY
static void t2replay_exact_graph_hide(void)
{
	_AH = 0x41;
	__int__(0x18);
}

static void t2replay_exact_graph_show(void)
{
	_AH = 0x40;
	__int__(0x18);
}

static void t2replay_exact_hud_score_put(utram_y_t y, score_t value)
{
	int32_t divisor = 1000000;
	tram_x_t x;

	for(x = HUD_LEFT; x < (HUD_LEFT + 14); x += GAIJI_TRAM_W) {
		int numeral = static_cast<int>(value / divisor);
		value -= (numeral * divisor);
		gaiji_putca(x, y, (gb_0 + numeral), TX_WHITE);
		divisor /= 10;
	}
}

static void t2replay_exact_hud_continues_put(
	utram_y_t y, int continues_used
)
{
	if(continues_used >= 10) {
		continues_used = 9;
	}
	gaiji_putca((HUD_LEFT + 14), y, (gb_0 + continues_used), TX_WHITE);
}

static void t2replay_exact_hud_tally_put(
	utram_y_t y, int value, int gaiji
)
{
	int i;

	for(i = 0; i < 5; i++) {
		gaiji_putca(
			(HUD_LABELED_LEFT + (i * GAIJI_TRAM_W)), y,
			((i < value) ? gaiji : gb_SP), TX_WHITE
		);
	}
}

static void t2replay_exact_hud_power_put(void)
{
	char bar[6];
	int i;
	int value_rem;

	for(i = 0; i < 5; i++) {
		bar[i] = gb_SP;
	}
	bar[5] = '\0';
	if(shot_level == SHOT_LEVEL_MAX) {
		for(i = 0; i < 5; i++) {
			bar[i] = (g_BAR_MAX_0 + i);
		}
	} else {
		value_rem = (power - BAR_GAIJI_MAX);
		i = 0;
		while(value_rem > 0) {
			bar[i++] = g_BAR_16W;
			value_rem -= BAR_GAIJI_MAX;
		}
		bar[i] = (g_BAR_01W + ((power - 1) & (BAR_GAIJI_MAX - 1)));
	}
	gaiji_putsa(
		HUD_LABELED_LEFT, HUD_POWER_Y, bar,
		((shot_level <= 2) ? TX_RED :
		 (shot_level <= 4) ? TX_MAGENTA :
		 (shot_level == 5) ? TX_BLUE :
		 (shot_level <= 7) ? TX_GREEN :
		 (shot_level == 8) ? TX_CYAN : TX_YELLOW)
	);
}

static void t2replay_exact_hud_values_put(void)
{
	t2replay_exact_hud_score_put(HUD_SCORE_Y, score);
	t2replay_exact_hud_continues_put(
		HUD_SCORE_Y, resident->continues_used
	);
	t2replay_exact_hud_score_put(HUD_HISCORE_Y, hiscore);
	t2replay_exact_hud_continues_put(HUD_HISCORE_Y, hiscore_continues);
	t2replay_exact_hud_tally_put(HUD_LIVES_Y, lives, gs_YINYANG);
	t2replay_exact_hud_tally_put(HUD_BOMBS_Y, bombs, gs_BOMB);
	t2replay_exact_hud_power_put();
}

static void far t2replay_exact_reveal(void)
{
	boss_activate_if_scroll_done_func = nullfunc_void;
	t2replay_exact_graph_show();
	t2replay_exact_active = false;
	t2replay_exact_diag.state = T2XDS_REVEALED;
	t2replay_exact_diag.page_front = page_front;
	t2replay_exact_diag.page_back = page_back;
	t2replay_exact_diag.sample_cursor = t2replay_sample_cursor;
	t2replay_exact_diag.packet_cursor = t2replay_packet_cursor;
	t2xobs_reveal();
}

static bool16 near t2replay_exact_apply_at_loop_top(void)
{
	struct t2rec_boundary_t exact_boundary;
	struct t2checkpoint_common_boundary_t common_boundary;
	struct t2checkpoint_common_exact_plan_t common_plan;
	th02_actor_core_state_t actor_core_plan;
	th02_s5_mima_state_t actor_stage_plan;
	th02_s5_fx_apply_plan_t stage_fx_plan;
	th02_s5_tile_apply_plan_t tile_plan;
	th02_s5_palette_apply_plan_t palette_plan;
	th02_s5_callback_redraw_plan_t callback_redraw_plan;
	const uint8_t far *group[T2REPLAY_EXACT_GROUP_COUNT];
	enum t2checkpoint_common_reject_t common_reject = T2CCAR_OK;
	uint8_t group_id;

	t2replay_memclear(&exact_boundary, sizeof(exact_boundary));
	exact_boundary.at_ordinary_stage_loop_top = 1;
	exact_boundary.stage_init_complete = 1;
	exact_boundary.stage_progression = stage_progression;
	t2replay_memclear(&common_boundary, sizeof(common_boundary));
	common_boundary.at_ordinary_stage_loop_top = 1;
	common_boundary.stage_init_complete = 1;
	if(
		(t2replay_mode != T2RM_PLAYBACK) ||
		(t2replay_exact_envelope == 0) || t2replay_exact_active ||
		!t2replay_exact_anchor_sample_pending
	) {
		t2replay_exact_diag.state = T2XDS_REJECTED;
		t2replay_exact_pending = false;
		t2replay_exact_anchor_sample_pending = false;
		t2replay_exact_first_sample_pending = false;
		t2replay_exact_envelope_free();
		return false;
	}
	for(group_id = 0; group_id < T2REPLAY_EXACT_GROUP_COUNT; group_id++) {
		group[group_id] = t2replay_exact_s5_mima_group_payload(
			t2replay_exact_envelope, group_id
		);
	}
	if(
		(t2replay_sample_cursor != t2replay_exact_request.sample_anchor) ||
		(t2replay_packet_cursor !=
		 (t2replay_exact_request.packet_anchor + 1)) ||
		(t2replay_decode_run == 0) ||
		(stage_id != 4) ||
		(replay_exact_checkpoint_validate(
			t2replay_exact_envelope,
			T2REPLAY_EXACT_S5CBRD_CAPTURE_SIZE, &exact_boundary
		) != T2REC_DEFERRED_CODECS) ||
		!t2checkpoint_common_exact_plan_prepare(
			&common_plan, group, &common_boundary, &common_reject
		) ||
		!th02_actor_core_state_wire_prepare(
			&actor_core_plan, group[T2RXGI_ACTOR_CORE],
			T2REPLAY_EXACT_ACTOR_CORE_SIZE
		) ||
		!th02_s5_mima_state_wire_prepare(
			&actor_stage_plan, group[T2RXGI_ACTOR_STAGE],
			T2REPLAY_EXACT_S5_MIMA_SIZE
		) ||
		!th02_s5_mima_stage_fx_wire_prepare(
			&stage_fx_plan, group[T2RXGI_STAGE_FX],
			T2REPLAY_EXACT_S5MFX_SIZE
		) ||
		!th02_s5_tile_logic_wire_prepare(
			&tile_plan, group[T2RXGI_TILE_LOGIC],
			T2REPLAY_EXACT_S5_TILE_LOGIC_SIZE
		) ||
		!th02_s5_mima_palette_wire_prepare(
			&palette_plan, group[T2RXGI_PALETTE],
			T2REPLAY_EXACT_S5_PALETTE_SIZE
		) ||
		!th02_s5_mima_callback_redraw_prepare(
			&callback_redraw_plan,
			group[T2RXGI_CALLBACKS], group[T2RXGI_REDRAW],
			group[T2RXGI_ACTOR_STAGE], group[T2RXGI_LASER],
			group[T2RXGI_ENEMY],
			t2replay_exact_envelope[T2REC_HEADER_CALLBACK_PROFILE],
			t2replay_exact_envelope[T2REC_HEADER_SHOTTYPE]
		) ||
		!th02_s5_mima_resources_prepare(group[T2RXGI_ACTOR_STAGE])
	) {
		t2replay_exact_diag.state = T2XDS_REJECTED;
		t2replay_exact_diag.reject = static_cast<uint8_t>(common_reject);
		t2replay_exact_pending = false;
		t2replay_exact_anchor_sample_pending = false;
		t2replay_exact_first_sample_pending = false;
		t2replay_exact_envelope_free();
		return false;
	}

	// No operation below this point can reject, allocate, load a resource, or
	// access DOS. The first common-world write begins the transaction.
	t2replay_exact_pending = false;
	t2replay_exact_active = true;
	t2replay_exact_anchor_sample_pending = false;
	t2replay_exact_first_sample_pending = true;
	t2checkpoint_common_exact_commit_prepared(&common_plan);
	th02_actor_core_state_commit_prepared(&actor_core_plan);
	th02_s5_mima_state_commit_prepared(&actor_stage_plan);
	th02_s5_mima_stage_fx_commit_prepared(&stage_fx_plan);
	th02_s5_tile_logic_commit_prepared(&tile_plan);
	th02_s5_mima_palette_commit_prepared(&palette_plan);
	palette_show();
	th02_s5_mima_callback_commit_prepared(&callback_redraw_plan);
	t2replay_exact_hud_values_put();
	th02_s5_mima_redraw_commit_prepared(
		&callback_redraw_plan, common_plan.page_back
	);
	boss_activate_if_scroll_done_func = t2replay_exact_reveal;
	t2replay_exact_diag.state = T2XDS_COMMITTED;
	t2replay_exact_diag.page_front = common_plan.page_front;
	t2replay_exact_diag.page_back = common_plan.page_back;
	t2replay_exact_diag.sample_cursor = t2replay_sample_cursor;
	t2replay_exact_diag.packet_cursor = t2replay_packet_cursor;
	return true;
}
#endif

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

static uint16_t t2replay_header_wire_size(void)
{
	if(
		t2replay_magic_matches(t2replay_header.magic, '1') &&
		(t2replay_header.version == T2REPLAY_VERSION_LEGACY) &&
		(t2replay_header.header_size == T2REPLAY_HEADER_SIZE_LEGACY)
	) {
		return T2REPLAY_HEADER_SIZE_LEGACY;
	}
	if(
		t2replay_magic_matches(t2replay_header.magic, '2') &&
		(t2replay_header.version == T2REPLAY_VERSION_PREVIOUS) &&
		(t2replay_header.header_size == T2REPLAY_HEADER_SIZE)
	) {
		return T2REPLAY_HEADER_SIZE;
	}
	if(
		t2replay_magic_matches(t2replay_header.magic, '3') &&
		(t2replay_header.version == T2REPLAY_VERSION_TELEMETRY) &&
		(t2replay_header.header_size == T2REPLAY_HEADER_SIZE)
	) {
		return T2REPLAY_HEADER_SIZE;
	}
	if(
		t2replay_magic_matches(t2replay_header.magic, '4') &&
		(t2replay_header.version == T2REPLAY_VERSION_EMBEDDED_ACCELERATOR) &&
		(t2replay_header.header_size == T2REPLAY_HEADER_SIZE)
	) {
		return T2REPLAY_HEADER_SIZE;
	}
	if(
		t2replay_magic_matches(t2replay_header.magic, '5') &&
		(t2replay_header.version == T2REPLAY_VERSION) &&
		(t2replay_header.header_size == T2REPLAY_HEADER_WIRE_SIZE)
	) {
		return T2REPLAY_HEADER_WIRE_SIZE;
	}
	return 0;
}

static uint16_t t2replay_header_checksum_size(void)
{
	return (
		(t2replay_header.version == T2REPLAY_VERSION)
			? T2REPLAY_HEADER_SIZE : t2replay_header_wire_size()
	);
}

static void t2replay_header_checksum_set(void)
{
	uint16_t checksum_size = t2replay_header_checksum_size();

	t2replay_header.header_checksum = 0;
	t2replay_header.header_checksum = t2replay_fnv1a(
		T2REPLAY_FNV1A_BASIS, &t2replay_header, checksum_size
	);
}

static bool t2replay_header_opaque_tail_create(int fd)
{
	uint8_t zero[32];
	unsigned left = T2REPLAY_HEADER_OPAQUE_SIZE;
	unsigned count;

	t2replay_memclear(zero, sizeof(zero));
	while(left != 0) {
		count = ((left > sizeof(zero)) ? sizeof(zero) : left);
		if(t2replay_dos_write(fd, zero, count) != count) {
			return false;
		}
		left -= count;
	}
	return true;
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
		(t2replay_dos_write(
			fd, &t2replay_header, sizeof(t2replay_header)
		) != sizeof(t2replay_header)) ||
		(create && !t2replay_header_opaque_tail_create(fd))) {
		t2replay_dos_close(fd);
		return false;
	}
	t2replay_dos_close(fd);
	return true;
}

static bool t2replay_stage_seek_write(void)
{
	t2replay_public_seek_header_t header;
	uint32_t checkpoint_offset;
	uint32_t hash;
	uint8_t i;
	int fd;
	bool ok;

	if((t2replay_stage_seek_count == 0) ||
		(t2replay_stage_seek_count > T2REPLAY_STAGE_COUNT)) {
		return false;
	}
	t2replay_memclear(&header, sizeof(header));
	header.magic[0] = 'T'; header.magic[1] = '2';
	header.magic[2] = 'R'; header.magic[3] = 'S';
	header.magic[4] = 'K'; header.magic[5] = '2';
	header.version = T2REPLAY_STAGE_SEEK_VERSION;
	header.header_size = T2REPLAY_PUBLIC_SEEK_HEADER_SIZE;
	header.entry_size = T2REPLAY_PUBLIC_SEEK_ENTRY_SIZE;
	header.entry_count = t2replay_stage_seek_count;
	header.replay_header_checksum = t2replay_header.header_checksum;
	header.replay_payload_checksum = t2replay_header.payload_checksum;
	header.format_fingerprint = T2REPLAY_STAGE_SEEK_FORMAT_FINGERPRINT;
	header.replay_sample_count = t2replay_header.sample_count;
	header.replay_packet_count = t2replay_header.packet_count;
	header.reserved[0] = T2REPLAY_TEMP_SLOT;
	checkpoint_offset = (
		T2REPLAY_PUBLIC_SEEK_HEADER_SIZE +
		(static_cast<uint32_t>(t2replay_stage_seek_count) *
		 T2REPLAY_PUBLIC_SEEK_ENTRY_SIZE)
	);
	for(i = 0; i < t2replay_stage_seek_count; i++) {
		t2replay_stage_seek_entries[i].checkpoint_offset = checkpoint_offset;
		checkpoint_offset += T2REPLAY_START_SIZE;
	}
	header.total_size = checkpoint_offset;
	header.directory_checksum = t2replay_fnv1a(
		T2REPLAY_FNV1A_BASIS, t2replay_stage_seek_entries,
		static_cast<unsigned>(
			t2replay_stage_seek_count * T2REPLAY_PUBLIC_SEEK_ENTRY_SIZE
		)
	);
	hash = t2replay_fnv1a(T2REPLAY_FNV1A_BASIS, &header, sizeof(header));
	hash = t2replay_fnv1a(
		hash, t2replay_stage_seek_entries,
		static_cast<unsigned>(
			t2replay_stage_seek_count * T2REPLAY_PUBLIC_SEEK_ENTRY_SIZE
		)
	);
	hash = t2replay_fnv1a(
		hash, t2replay_stage_seek_starts,
		static_cast<unsigned>(
			t2replay_stage_seek_count * T2REPLAY_START_SIZE
		)
	);
	header.sidecar_checksum = hash;
	t2replay_dos_delete(t2replay_stage_seek_fn);
	fd = t2replay_dos_create(t2replay_stage_seek_fn);
	if(fd < 0) {
		return false;
	}
	ok = (
		(t2replay_dos_write(fd, &header, sizeof(header)) == sizeof(header)) &&
		(t2replay_dos_write(
			fd, t2replay_stage_seek_entries,
			static_cast<unsigned>(
				t2replay_stage_seek_count * T2REPLAY_PUBLIC_SEEK_ENTRY_SIZE
			)
		) == static_cast<unsigned>(
			t2replay_stage_seek_count * T2REPLAY_PUBLIC_SEEK_ENTRY_SIZE
		)) &&
		(t2replay_dos_write(
			fd, t2replay_stage_seek_starts,
			static_cast<unsigned>(
				t2replay_stage_seek_count * T2REPLAY_START_SIZE
			)
		) == static_cast<unsigned>(
			t2replay_stage_seek_count * T2REPLAY_START_SIZE
		))
	);
	t2replay_dos_close(fd);
	if(ok) {
		t2replay_dos_flush();
	} else {
		t2replay_dos_delete(t2replay_stage_seek_fn);
	}
	return ok;
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
	} else {
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
	}
	t2replay_protect_sample_count++;
	if(
		((t2replay_protect_sample_count &
		  (REPLAY_PROTECT_INTERVAL_SAMPLES - 1)) == 0) &&
		!t2replay_guard_blocked()
	) {
		(void)t2replay_guard_checkpoint(T2SAE_PERIODIC);
	}
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

static void t2replay_stage_seek_start_capture(t2replay_start_t far *start)
{
	t2replay_memclear(start, sizeof(*start));
	start->resident_frame = static_cast<uint32_t>(resident->frame);
	start->random_seed = start->resident_frame;
	start->score = score;
	start->score_highest = (
		resident->score_highest < static_cast<uint32_t>(score)
			? static_cast<uint32_t>(score) : resident->score_highest
	);
	start->continues_used = resident->continues_used;
	start->skill = resident->skill;
	start->stage = stage_id;
	start->rank = rank;
	start->rem_lives = lives;
	start->rem_bombs = bombs;
	// Resume uses these fields to initialize live MAIN globals. They therefore
	// carry current stock, not the run's original starting stock.
	start->start_lives = static_cast<uint8_t>(lives);
	start->start_bombs = static_cast<uint8_t>(bombs);
	start->start_power = static_cast<int8_t>(power);
	start->shottype = resident->shottype;
	start->bgm_mode = resident->bgm_mode;
	start->reduce_effects = (resident->reduce_effects ? 1 : 0);
	start->reserved[T2REPLAY_PRACTICE_PLAYPERF_OFFSET] =
		t2replay_practice_playperf_encode(playperf);
	start->reserved[T2REPLAY_PRACTICE_RANK_LOCK_OFFSET] =
		(t2replay_rank_lock_active ? 1 : 0);
	start->reserved[T2REPLAY_AUTOFIRE_OFFSET] =
		(t2replay_autofire_active ? 1 : 0);
}

static bool t2replay_stage_seek_capture(void)
{
	t2replay_public_seek_entry_t far *entry;
	t2replay_start_t far *start;
	uint8_t index = t2replay_stage_seek_count;

	if((index >= T2REPLAY_STAGE_COUNT) ||
		((index != 0) &&
		 (stage_id != (t2replay_stage_seek_entries[index - 1].stage_id + 1))) ||
		!t2replay_pending_commit()) {
		return false;
	}
	entry = &t2replay_stage_seek_entries[index];
	start = &t2replay_stage_seek_starts[index];
	t2replay_memclear(entry, sizeof(*entry));
	t2replay_stage_seek_start_capture(start);
	entry->stage_id = static_cast<uint8_t>(stage_id);
	entry->target_kind = T2REPLAY_PUBLIC_SEEK_TARGET_STAGE;
	entry->capture_generation = T2REPLAY_STAGE_SEEK_CAPTURE_GENERATION;
	entry->checkpoint_schema = T2REPLAY_STAGE_SEEK_SCHEMA;
	entry->group_count = 1;
	entry->sample_anchor = t2replay_header.sample_count;
	entry->packet_anchor = t2replay_header.packet_count;
	entry->prefix_checksum = t2replay_payload_checksum;
	entry->semantic_digest = (
		t2replay_header.input_offset +
		(entry->packet_anchor * T2REPLAY_PACKET_SIZE)
	);
	entry->checkpoint_size = sizeof(*start);
	entry->checkpoint_checksum = t2replay_fnv1a(
		T2REPLAY_FNV1A_BASIS, start, sizeof(*start)
	);
	entry->source_fingerprint = T2REPLAY_STAGE_SEEK_FORMAT_FINGERPRINT;
	t2replay_stage_seek_count++;
	return true;
}

static bool t2replay_start_valid(const t2replay_start_t far *start)
{
	uint8_t practice_target = start->reserved[T2REPLAY_PRACTICE_TARGET_OFFSET];
	bool practice_target_valid = false;
	t2practice_diag_lifecycle(T2PDLM_MAIN_START_CORE_BEGIN, 0, 0, 0);

	if(practice_target == T2RPT_STAGE_START) {
		practice_target_valid = true;
	} else if(
		(practice_target == T2RPT_STAGE1_CHAPTER2) ||
		((practice_target >= T2RPT_STAGE1_MIDBOSS) &&
		 (practice_target <= T2RPT_STAGE1_BOSS_PHASE3))
	) {
		practice_target_valid = (start->stage == 0);
	} else if(
		(practice_target == T2RPT_STAGE2_CHAPTER2) ||
		((practice_target >= T2RPT_STAGE2_MIDBOSS) &&
		 (practice_target <= T2RPT_STAGE2_BOSS_PHASE3))
	) {
		practice_target_valid = (start->stage == 1);
	} else if(
		(practice_target == T2RPT_STAGE3_CHAPTER2) ||
		(practice_target == T2RPT_STAGE3_MIDBOSS) ||
		(practice_target == T2RPT_STAGE3_BOSS_START) ||
		(practice_target == T2RPT_STAGE3_INNER_PAIR) ||
		(practice_target == T2RPT_STAGE3_OUTER_PAIR) ||
		(practice_target == T2RPT_STAGE3_NORTH_PHASE4)
	) {
		practice_target_valid = (start->stage == 2);
	} else if(
		(practice_target == T2RPT_STAGE4_CHAPTER2) ||
		(practice_target == T2RPT_STAGE4_CHAPTER3) ||
		((practice_target >= T2RPT_STAGE4_MIDBOSS_FIRST) &&
		 (practice_target <= T2RPT_STAGE4_BOSS_START)) ||
		(practice_target == T2RPT_STAGE4_BOSS_PHASE1) ||
		(practice_target == T2RPT_STAGE4_BOSS_ROUND2) ||
		(practice_target == T2RPT_STAGE4_BOSS_ROUND3) ||
		((practice_target >= T2RPT_STAGE4_BOSS_ROUND4) &&
		 (practice_target <= T2RPT_STAGE4_BOSS_ROUND7))
	) {
		practice_target_valid = (start->stage == 3);
	} else if(
		(practice_target == T2RPT_STAGE5_BOSS_START) ||
		(practice_target == T2RPT_STAGE5_BOSS_PHASE1) ||
		(practice_target == T2RPT_STAGE5_BOSS_PHASE3) ||
		(practice_target == T2RPT_STAGE5_BOSS_PHASE5) ||
		(practice_target == T2RPT_STAGE5_BOSS_PHASE7) ||
		(practice_target == T2RPT_STAGE5_BOSS_PHASE9)
	) {
		practice_target_valid = (
			(start->stage == 4) &&
			((practice_target != T2RPT_STAGE5_BOSS_PHASE9) ||
			 (start->continues_used == 0))
		);
	} else if(
		(practice_target == T2RPT_EXTRA_CHAPTER2) ||
		(practice_target == T2RPT_EXTRA_MIDBOSS) ||
		(practice_target == T2RPT_EXTRA_BOSS_START) ||
		(practice_target == T2RPT_EXTRA_BOSS_PHASE1) ||
		(practice_target == T2RPT_EXTRA_BOSS_PHASE3) ||
		(practice_target == T2RPT_EXTRA_BOSS_PHASE5) ||
		(practice_target == T2RPT_EXTRA_BOSS_PHASE7) ||
		(practice_target == T2RPT_EXTRA_BOSS_PHASE9)
	) {
		practice_target_valid = (start->stage == 5);
	}
	t2practice_diag_lifecycle(
		T2PDLM_MAIN_START_TARGET_RESOLVED,
		(practice_target_valid ? 1 : 0), 0, 0
	);
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
		(t2replay_practice_playperf_decode(
			start->reserved[T2REPLAY_PRACTICE_PLAYPERF_OFFSET]
		) < playperf_min) ||
		(t2replay_practice_playperf_decode(
			start->reserved[T2REPLAY_PRACTICE_PLAYPERF_OFFSET]
		) > ((start->rank == RANK_EASY) ? 4 : 16)) ||
		(start->reserved[T2REPLAY_PRACTICE_RANK_LOCK_OFFSET] > 1) ||
		(start->reserved[T2REPLAY_AUTOFIRE_OFFSET] > 1) ||
		!practice_target_valid
	) {
		return false;
	}
	t2practice_diag_lifecycle(T2PDLM_MAIN_START_SCALARS_VALID, 0, 0, 0);
	if(!t2replay_bytes_zero(
		&start->reserved[T2REPLAY_PRACTICE_RESERVED_OFFSET],
		T2REPLAY_PRACTICE_RESERVED_SIZE
	)) {
		return false;
	}
	t2practice_diag_lifecycle(T2PDLM_MAIN_START_RESERVED_VALID, 0, 0, 0);
	return true;
}

static bool t2replay_practice_start_valid(const t2replay_start_t far *start)
{
	// A stored zero is native: cfg_load() maps it to the first live power unit.
	if(!t2replay_start_valid(start)) {
		return false;
	}
	t2practice_diag_lifecycle(T2PDLM_MAIN_PRACTICE_CORE_VALID, 0, 0, 0);
	return (
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
			 (packet->input_low != T2REPLAY_END_CLEAR) &&
			 (packet->input_low != T2REPLAY_END_MENU_RETURN)) ||
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

static bool t2replay_embedded_stage_seek_load(
	uint8_t selected_stage, t2replay_start_t far *start,
	uint32_t far *sample_anchor, uint32_t far *packet_anchor,
	uint32_t far *prefix_checksum
);

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

	uint32_t input_end = (
		t2replay_header.input_offset + t2replay_header.input_size
	);
	if(
		(((t2replay_header.version == T2REPLAY_VERSION_EMBEDDED_ACCELERATOR) ||
		  (t2replay_header.version == T2REPLAY_VERSION)) &&
		 (file_size < (input_end + T2REPLAY_ACCELERATOR_HEADER_SIZE))) ||
		((t2replay_header.version != T2REPLAY_VERSION_EMBEDDED_ACCELERATOR) &&
		 (t2replay_header.version != T2REPLAY_VERSION) &&
		 (file_size != input_end))
	) {
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
	uint16_t wire_size;
	t2replay_start_t accelerator_start;
	uint32_t accelerator_sample;
	uint32_t accelerator_packet;
	uint32_t accelerator_prefix;
	int fd;

	fd = t2replay_dos_open(t2replay_slot_fn, T2REPLAY_DOS_ACCESS_READ);
	if(fd < 0) {
		return false;
	}
	t2replay_memclear(&t2replay_header, sizeof(t2replay_header));
	if(
		(t2replay_dos_read(
			fd, &t2replay_header, T2REPLAY_HEADER_SIZE_LEGACY
		) != T2REPLAY_HEADER_SIZE_LEGACY) ||
		(((t2replay_header.magic[5] == '2') ||
		  (t2replay_header.magic[5] == '3') ||
		  (t2replay_header.magic[5] == '4') ||
		  (t2replay_header.magic[5] == '5')) &&
		 (t2replay_dos_read(
			fd,
			reinterpret_cast<uint8_t far *>(&t2replay_header) +
				T2REPLAY_HEADER_SIZE_LEGACY,
			T2REPLAY_HEADER_SIZE - T2REPLAY_HEADER_SIZE_LEGACY
		 ) != (T2REPLAY_HEADER_SIZE - T2REPLAY_HEADER_SIZE_LEGACY))) ||
		!t2replay_dos_size(fd, &file_size)
	) {
		t2replay_dos_close(fd);
		return false;
	}
	wire_size = t2replay_header_wire_size();
	stored_checksum = t2replay_header.header_checksum;
	t2replay_header.header_checksum = 0;
	computed_checksum = (wire_size == 0) ? 0 : t2replay_fnv1a(
		T2REPLAY_FNV1A_BASIS, &t2replay_header,
		((t2replay_header.version == T2REPLAY_VERSION)
			? T2REPLAY_HEADER_SIZE : wire_size)
	);
	t2replay_header.header_checksum = stored_checksum;
	if(
		(wire_size == 0) ||
		(t2replay_header.packet_size != T2REPLAY_PACKET_SIZE) ||
		((t2replay_header.flags & T2REPLAY_REQUIRED_FLAGS) !=
		 T2REPLAY_REQUIRED_FLAGS) ||
		((t2replay_header.flags & ~T2REPLAY_KNOWN_FLAGS) != 0) ||
		(t2replay_header.status != T2REPLAY_STATUS_FINALIZED) ||
		(t2replay_header.game_id != 2) ||
		(t2replay_header.ruleset != T2REPLAY_RULESET_STOCK) ||
		(t2replay_header.input_semantics != T2REPLAY_INPUT_SEMANTICS_KEY_DET) ||
		(t2replay_header.stage_count != T2REPLAY_STAGE_COUNT) ||
		(t2replay_header.stage_reached >= T2REPLAY_STAGE_COUNT) ||
		(t2replay_header.terminal_stage >= T2REPLAY_STAGE_COUNT) ||
		(t2replay_header.end_reason < T2REPLAY_END_GAME_OVER) ||
		(t2replay_header.end_reason > T2REPLAY_END_MENU_RETURN) ||
		(t2replay_header.input_offset != wire_size) ||
		(t2replay_header.input_size > T2REPLAY_INPUT_SIZE_MAX) ||
		(t2replay_header.packet_count >
		 (T2REPLAY_INPUT_SIZE_MAX / T2REPLAY_PACKET_SIZE)) ||
		(t2replay_header.input_size !=
		 (t2replay_header.packet_count * T2REPLAY_PACKET_SIZE)) ||
		(t2replay_header.continues_final > 9) ||
		(stored_checksum != computed_checksum) ||
		(t2replay_header.slow_frames > t2replay_header.timed_frames) ||
		!t2replay_start_valid(&t2replay_header.start) ||
		!t2replay_stage_scores_valid() ||
		!t2replay_name_valid(
			reinterpret_cast<const uint8_t far *>(
				t2replay_header.reserved + T2REPLAY_RESERVED_NAME_OFFSET
			)
		) ||
		!t2replay_dos_datetime_valid()
	) {
		t2replay_dos_close(fd);
		return false;
	}
	if(!t2replay_payload_validate(fd, file_size)) {
		t2replay_dos_close(fd);
		return false;
	}
	t2replay_dos_close(fd);
	if(
		((t2replay_header.version == T2REPLAY_VERSION_EMBEDDED_ACCELERATOR) ||
		 (t2replay_header.version == T2REPLAY_VERSION)) &&
		!t2replay_embedded_stage_seek_load(
			static_cast<uint8_t>(t2replay_header.start.stage),
			&accelerator_start, &accelerator_sample, &accelerator_packet,
			&accelerator_prefix
		)
	) {
		return false;
	}
	t2replay_buffer_len = 0;
	t2replay_buffer_pos = 0;
	return true;
}

static bool t2replay_stage_seek_magic_matches(const char far *magic)
{
	return (
		(magic[0] == 'T') && (magic[1] == '2') && (magic[2] == 'R') &&
		(magic[3] == 'S') && (magic[4] == 'K') && (magic[5] == '2') &&
		(magic[6] == '\0') && (magic[7] == '\0')
	);
}

static bool t2replay_stage_seek_file_checksum(
	int fd, uint32_t size, uint32_t far *checksum
)
{
	uint8_t bytes[64];
	uint32_t offset = 0;
	uint32_t hash = T2REPLAY_FNV1A_BASIS;
	unsigned want;
	unsigned i;

	if(!t2replay_dos_seek(fd, 0)) {
		return false;
	}
	while(offset < size) {
		want = static_cast<unsigned>(
			((size - offset) > sizeof(bytes)) ? sizeof(bytes) : (size - offset)
		);
		if(t2replay_dos_read(fd, bytes, want) != want) {
			return false;
		}
		for(i = 0; i < want; i++) {
			if(((offset + i) >= 0x2C) && ((offset + i) < 0x30)) {
				bytes[i] = 0;
			}
		}
		hash = t2replay_fnv1a(hash, bytes, want);
		offset += want;
	}
	*checksum = hash;
	return true;
}

static bool t2replay_stage_seek_start_valid(
	const t2replay_start_t far *start
)
{
	int playperf_value = t2replay_practice_playperf_decode(
		start->reserved[T2REPLAY_PRACTICE_PLAYPERF_OFFSET]
	);

	return (
		(start->stage >= 0) && (start->stage < T2REPLAY_STAGE_COUNT) &&
		(start->rank <= RANK_EXTRA) &&
		((start->stage == (T2REPLAY_STAGE_COUNT - 1)) ==
		 (start->rank == RANK_EXTRA)) &&
		(start->score >= 0) &&
		(start->score_highest >= static_cast<uint32_t>(start->score)) &&
		(start->continues_used <= 9) &&
		(start->rem_lives >= 0) && (start->rem_lives <= 5) &&
		(start->rem_bombs >= 0) && (start->rem_bombs <= 5) &&
		(start->start_lives == static_cast<uint8_t>(start->rem_lives)) &&
		(start->start_bombs == static_cast<uint8_t>(start->rem_bombs)) &&
		(start->start_power >= 1) && (start->start_power <= 80) &&
		(start->random_seed == start->resident_frame) &&
		(start->shottype < SHOTTYPE_COUNT) &&
		(start->bgm_mode <= SND_BGM_MIDI) &&
		(start->reduce_effects <= 1) && (start->debug == 0) &&
		(playperf_value >= playperf_min) &&
		(playperf_value <= ((start->rank == RANK_EASY) ? 4 : 16)) &&
		(start->reserved[T2REPLAY_PRACTICE_TARGET_OFFSET] ==
		 T2RPT_STAGE_START) &&
		(start->reserved[T2REPLAY_PRACTICE_RANK_LOCK_OFFSET] <= 1) &&
		(start->reserved[T2REPLAY_AUTOFIRE_OFFSET] <= 1) &&
		t2replay_bytes_zero(
			&start->reserved[T2REPLAY_PRACTICE_RESERVED_OFFSET],
			T2REPLAY_PRACTICE_RESERVED_SIZE
		)
	);
}

static bool t2replay_stage_seek_prefix_valid(
	const t2replay_public_seek_entry_t far *entry
)
{
	uint32_t packet_index = 0;
	uint32_t samples = 0;
	uint32_t hash = T2REPLAY_FNV1A_BASIS;
	uint32_t remaining;
	unsigned want;
	unsigned len;
	unsigned i;
	uint8_t expected_stage = static_cast<uint8_t>(t2replay_header.start.stage);
	uint8_t phase;
	uint8_t opcode;
	input_t input;
	int fd;
	bool valid = false;

	if(entry->packet_anchor >= t2replay_header.packet_count) {
		return false;
	}
	fd = t2replay_dos_open(t2replay_slot_fn, T2REPLAY_DOS_ACCESS_READ);
	if((fd < 0) || !t2replay_dos_seek(fd, t2replay_header.input_offset)) {
		if(fd >= 0) {
			t2replay_dos_close(fd);
		}
		return false;
	}
	while(packet_index <= entry->packet_anchor) {
		remaining = (entry->packet_anchor - packet_index + 1);
		want = static_cast<unsigned>(
			(remaining > T2REPLAY_BUFFER_PACKET_COUNT)
				? T2REPLAY_BUFFER_PACKET_COUNT : remaining
		);
		len = (want * T2REPLAY_PACKET_SIZE);
		if(t2replay_dos_read(fd, t2replay_buffer, len) != len) {
			break;
		}
		for(i = 0; i < want; i++, packet_index++) {
			t2replay_packet_t far *packet = &t2replay_buffer[i];
			if(packet_index == entry->packet_anchor) {
				valid = (
					(packet->tag == static_cast<uint8_t>(
						(T2REPLAY_PHASE_CONTROL <<
						 T2REPLAY_PACKET_PHASE_SHIFT) |
						T2REPLAY_CONTROL_STAGE_START
					)) &&
					(packet->input_low == entry->stage_id) &&
					(packet->input_high == 0) && (packet->arg == 0) &&
					(expected_stage == entry->stage_id) &&
					(samples == entry->sample_anchor) &&
					(hash == entry->prefix_checksum)
				);
				packet_index++;
				break;
			}
			hash = t2replay_fnv1a(hash, packet, T2REPLAY_PACKET_SIZE);
			phase = static_cast<uint8_t>(
				packet->tag >> T2REPLAY_PACKET_PHASE_SHIFT
			);
			if(phase < T2REPLAY_PHASE_CONTROL) {
				input = static_cast<input_t>(
					packet->input_low |
					(static_cast<uint16_t>(packet->input_high) << 8)
				);
				if((packet->arg != 0) || (input & ~T2REPLAY_INPUT_KNOWN)) {
					packet_index = entry->packet_anchor + 1;
					break;
				}
				remaining = static_cast<uint32_t>(
					(packet->tag & T2REPLAY_PACKET_RUN_MASK) + 1
				);
				if((samples + remaining) < samples) {
					packet_index = entry->packet_anchor + 1;
					break;
				}
				samples += remaining;
				continue;
			}
			opcode = static_cast<uint8_t>(
				packet->tag & T2REPLAY_PACKET_RUN_MASK
			);
			if((phase != T2REPLAY_PHASE_CONTROL) ||
				(opcode != T2REPLAY_CONTROL_STAGE_START) ||
				(packet->input_low != expected_stage) ||
				(packet->input_high != 0) || (packet->arg != 0)) {
				packet_index = entry->packet_anchor + 1;
				break;
			}
			expected_stage++;
		}
	}
	t2replay_dos_close(fd);
	return valid;
}

static bool t2replay_external_stage_seek_load(
	uint8_t slot, uint8_t selected_stage, t2replay_start_t far *start,
	uint32_t far *sample_anchor, uint32_t far *packet_anchor,
	uint32_t far *prefix_checksum
)
{
	t2replay_public_seek_header_t header;
	t2replay_public_seek_entry_t entry;
	t2replay_public_seek_entry_t selected;
	uint32_t file_size;
	uint32_t checksum;
	uint32_t directory_checksum = T2REPLAY_FNV1A_BASIS;
	uint32_t checkpoint_base;
	uint8_t i;
	int fd;
	bool found = false;

	fd = t2replay_dos_open(t2replay_stage_seek_fn, T2REPLAY_DOS_ACCESS_READ);
	if(fd < 0) {
		return false;
	}
	t2replay_memclear(&header, sizeof(header));
	t2replay_memclear(&selected, sizeof(selected));
	if((t2replay_dos_read(fd, &header, sizeof(header)) != sizeof(header)) ||
		!t2replay_dos_size(fd, &file_size) ||
		!t2replay_stage_seek_file_checksum(fd, file_size, &checksum) ||
		!t2replay_stage_seek_magic_matches(header.magic) ||
		(header.version != T2REPLAY_STAGE_SEEK_VERSION) ||
		(header.header_size != T2REPLAY_PUBLIC_SEEK_HEADER_SIZE) ||
		(header.entry_size != T2REPLAY_PUBLIC_SEEK_ENTRY_SIZE) ||
		(header.entry_count == 0) ||
		(header.entry_count > T2REPLAY_STAGE_COUNT) ||
		(header.entry_count != static_cast<uint16_t>(
			t2replay_header.stage_reached - t2replay_header.start.stage + 1
		)) ||
		(header.total_size != file_size) ||
		(header.replay_header_checksum != t2replay_header.header_checksum) ||
		(header.replay_payload_checksum != t2replay_header.payload_checksum) ||
		(header.format_fingerprint != T2REPLAY_STAGE_SEEK_FORMAT_FINGERPRINT) ||
		(header.replay_sample_count != t2replay_header.sample_count) ||
		(header.replay_packet_count != t2replay_header.packet_count) ||
		(header.sidecar_checksum != checksum) ||
		(header.reserved[0] != slot) ||
		!t2replay_bytes_zero(&header.reserved[1], sizeof(header.reserved) - 1)) {
		t2replay_dos_close(fd);
		return false;
	}
	checkpoint_base = (
		header.header_size +
		(static_cast<uint32_t>(header.entry_count) * header.entry_size)
	);
	if((checkpoint_base +
		(static_cast<uint32_t>(header.entry_count) * T2REPLAY_START_SIZE)) !=
		header.total_size || !t2replay_dos_seek(fd, header.header_size)) {
		t2replay_dos_close(fd);
		return false;
	}
	for(i = 0; i < header.entry_count; i++) {
		if(t2replay_dos_read(fd, &entry, sizeof(entry)) != sizeof(entry)) {
			t2replay_dos_close(fd);
			return false;
		}
		directory_checksum = t2replay_fnv1a(
			directory_checksum, &entry, sizeof(entry)
		);
		if((entry.stage_id != static_cast<uint8_t>(
			t2replay_header.start.stage + i
		)) ||
			(entry.target_kind != T2REPLAY_PUBLIC_SEEK_TARGET_STAGE) ||
			(entry.actor_tag != 0) || (entry.actor_mode != 0) ||
			(entry.stage_fx_tag != 0) || (entry.callback_profile != 0) ||
			(entry.redraw_recipe != 0) ||
			(entry.capture_generation !=
			 T2REPLAY_STAGE_SEEK_CAPTURE_GENERATION) ||
			(entry.checkpoint_schema != T2REPLAY_STAGE_SEEK_SCHEMA) ||
			(entry.group_count != 1) || (entry.reserved_0 != 0) ||
			(entry.checkpoint_offset !=
			 (checkpoint_base +
			  (static_cast<uint32_t>(i) * T2REPLAY_START_SIZE))) ||
			(entry.checkpoint_size != T2REPLAY_START_SIZE) ||
			(entry.semantic_digest !=
			 (t2replay_header.input_offset +
			  (entry.packet_anchor * T2REPLAY_PACKET_SIZE))) ||
			(entry.source_fingerprint !=
			 T2REPLAY_STAGE_SEEK_FORMAT_FINGERPRINT) ||
			(entry.reserved_1 != 0) ||
			(entry.packet_anchor >= t2replay_header.packet_count) ||
			(entry.sample_anchor > t2replay_header.sample_count)) {
			t2replay_dos_close(fd);
			return false;
		}
		if(entry.stage_id == selected_stage) {
			selected = entry;
			found = true;
		}
	}
	if((directory_checksum != header.directory_checksum) || !found ||
		!t2replay_dos_seek(fd, selected.checkpoint_offset) ||
		(t2replay_dos_read(fd, start, sizeof(*start)) != sizeof(*start))) {
		t2replay_dos_close(fd);
		return false;
	}
	t2replay_dos_close(fd);
	if((selected.checkpoint_checksum != t2replay_fnv1a(
		T2REPLAY_FNV1A_BASIS, start, sizeof(*start)
	)) || !t2replay_stage_seek_start_valid(start) ||
		(start->stage != static_cast<int8_t>(selected_stage)) ||
		(start->rank != t2replay_header.start.rank) ||
		(start->shottype != t2replay_header.start.shottype) ||
		(start->bgm_mode != t2replay_header.start.bgm_mode) ||
		(start->reduce_effects != t2replay_header.start.reduce_effects) ||
		(start->reserved[T2REPLAY_PRACTICE_RANK_LOCK_OFFSET] !=
		 t2replay_header.start.reserved[T2REPLAY_PRACTICE_RANK_LOCK_OFFSET]) ||
		(start->reserved[T2REPLAY_AUTOFIRE_OFFSET] !=
		 t2replay_header.start.reserved[T2REPLAY_AUTOFIRE_OFFSET]) ||
		!t2replay_stage_seek_prefix_valid(&selected)) {
		return false;
	}
	*sample_anchor = selected.sample_anchor;
	*packet_anchor = selected.packet_anchor;
	*prefix_checksum = selected.prefix_checksum;
	return true;
}

static bool t2replay_accelerator_checksum(
	int fd, uint32_t tail_offset, uint32_t size, uint32_t far *checksum
)
{
	uint8_t bytes[64];
	uint32_t offset = 0;
	uint32_t hash = T2REPLAY_FNV1A_BASIS;
	unsigned want;
	unsigned i;

	if(!t2replay_dos_seek(fd, tail_offset)) {
		return false;
	}
	while(offset < size) {
		want = static_cast<unsigned>(
			((size - offset) > sizeof(bytes)) ? sizeof(bytes) : (size - offset)
		);
		if(t2replay_dos_read(fd, bytes, want) != want) {
			return false;
		}
		for(i = 0; i < want; i++) {
			if(((offset + i) >= 28) && ((offset + i) < 32)) {
				bytes[i] = 0;
			}
		}
		hash = t2replay_fnv1a(hash, bytes, want);
		offset += want;
	}
	*checksum = hash;
	return true;
}

static bool t2replay_embedded_stage_seek_load(
	uint8_t selected_stage, t2replay_start_t far *start,
	uint32_t far *sample_anchor, uint32_t far *packet_anchor,
	uint32_t far *prefix_checksum
)
{
	t2replay_accelerator_header_t header;
	t2replay_accelerator_entry_t entry;
	t2replay_accelerator_entry_t selected;
	t2replay_public_seek_entry_t prefix_entry;
	uint32_t file_size;
	uint32_t checksum;
	uint32_t tail_offset = (
		t2replay_header.input_offset + t2replay_header.input_size
	);
	uint32_t directory_checksum = T2REPLAY_FNV1A_BASIS;
	uint32_t expected_start_offset;
	uint8_t i;
	int fd = t2replay_dos_open(t2replay_slot_fn, T2REPLAY_DOS_ACCESS_READ);
	bool found = false;

	if(fd < 0) {
		return false;
	}
	t2replay_memclear(&header, sizeof(header));
	t2replay_memclear(&selected, sizeof(selected));
	if(!t2replay_dos_size(fd, &file_size) || !t2replay_dos_seek(fd, tail_offset) ||
		(t2replay_dos_read(fd, &header, sizeof(header)) != sizeof(header)) ||
		(header.magic[0] != 'T') || (header.magic[1] != '2') ||
		(header.magic[2] != 'A') || (header.magic[3] != 'C') ||
		(header.magic[4] != 'C') || (header.magic[5] != '1') ||
		(header.magic[6] != '\0') || (header.magic[7] != '\0') ||
		(header.version != T2REPLAY_ACCELERATOR_VERSION) ||
		(header.header_size != T2REPLAY_ACCELERATOR_HEADER_SIZE) ||
		(header.entry_size != T2REPLAY_ACCELERATOR_ENTRY_SIZE) ||
		(header.entry_count != static_cast<uint16_t>(
			t2replay_header.stage_reached - t2replay_header.start.stage + 1
		)) ||
		(header.total_size != (T2REPLAY_ACCELERATOR_HEADER_SIZE +
			(static_cast<uint32_t>(header.entry_count) *
			 (T2REPLAY_ACCELERATOR_ENTRY_SIZE + T2REPLAY_START_SIZE)))) ||
		(file_size != (tail_offset + header.total_size)) ||
		(header.replay_header_checksum != t2replay_header.header_checksum) ||
		!t2replay_accelerator_checksum(
			fd, tail_offset, header.total_size, &checksum
		) || (checksum != header.accelerator_checksum) ||
		!t2replay_dos_seek(fd, tail_offset + header.header_size)) {
		t2replay_dos_close(fd);
		return false;
	}
	expected_start_offset = T2REPLAY_ACCELERATOR_HEADER_SIZE +
		(static_cast<uint32_t>(header.entry_count) *
		 T2REPLAY_ACCELERATOR_ENTRY_SIZE);
	for(i = 0; i < header.entry_count; i++) {
		if((t2replay_dos_read(fd, &entry, sizeof(entry)) != sizeof(entry)) ||
			(entry.stage_id != static_cast<uint8_t>(
				t2replay_header.start.stage + i
			)) || (entry.schema != T2REPLAY_STAGE_SEEK_SCHEMA) ||
			(entry.reserved != 0) ||
			(entry.start_offset != expected_start_offset) ||
			(entry.packet_anchor >= t2replay_header.packet_count) ||
			(entry.sample_anchor > t2replay_header.sample_count)) {
			t2replay_dos_close(fd);
			return false;
		}
		directory_checksum = t2replay_fnv1a(
			directory_checksum, &entry, sizeof(entry)
		);
		if(entry.stage_id == selected_stage) {
			selected = entry;
			found = true;
		}
		expected_start_offset += T2REPLAY_START_SIZE;
	}
	if((directory_checksum != header.directory_checksum) || !found ||
		!t2replay_dos_seek(fd, tail_offset + selected.start_offset) ||
		(t2replay_dos_read(fd, start, sizeof(*start)) != sizeof(*start))) {
		t2replay_dos_close(fd);
		return false;
	}
	t2replay_dos_close(fd);
	if((selected.start_checksum != t2replay_fnv1a(
		T2REPLAY_FNV1A_BASIS, start, sizeof(*start)
	)) || !t2replay_stage_seek_start_valid(start) ||
		(start->stage != static_cast<int8_t>(selected_stage))) {
		return false;
	}
	t2replay_memclear(&prefix_entry, sizeof(prefix_entry));
	prefix_entry.stage_id = selected.stage_id;
	prefix_entry.sample_anchor = selected.sample_anchor;
	prefix_entry.packet_anchor = selected.packet_anchor;
	prefix_entry.prefix_checksum = selected.prefix_checksum;
	if(!t2replay_stage_seek_prefix_valid(&prefix_entry)) {
		return false;
	}
	*sample_anchor = selected.sample_anchor;
	*packet_anchor = selected.packet_anchor;
	*prefix_checksum = selected.prefix_checksum;
	return true;
}

static bool t2replay_stage_seek_load(
	uint8_t slot, uint8_t selected_stage, t2replay_start_t far *start,
	uint32_t far *sample_anchor, uint32_t far *packet_anchor,
	uint32_t far *prefix_checksum
)
{
	if((t2replay_header.version == T2REPLAY_VERSION_EMBEDDED_ACCELERATOR) ||
	   (t2replay_header.version == T2REPLAY_VERSION)) {
		return t2replay_embedded_stage_seek_load(
			selected_stage, start, sample_anchor, packet_anchor, prefix_checksum
		);
	}
	return t2replay_external_stage_seek_load(
		slot, selected_stage, start, sample_anchor, packet_anchor, prefix_checksum
	);
}

#if T2REPLAY_EXACT_APPLY
static void near t2replay_exact_envelope_free(void)
{
	if(t2replay_exact_envelope != 0) {
		hmem_free(reinterpret_cast<void __seg *>(t2replay_exact_envelope));
		t2replay_exact_envelope = 0;
	}
}

static void near t2replay_exact_diag_init(void)
{
	t2replay_memclear(&t2replay_exact_diag, sizeof(t2replay_exact_diag));
	t2replay_exact_diag.magic[0] = 'T';
	t2replay_exact_diag.magic[1] = '2';
	t2replay_exact_diag.magic[2] = 'X';
	t2replay_exact_diag.magic[3] = 'D';
	t2replay_exact_diag.magic[4] = 'I';
	t2replay_exact_diag.magic[5] = 'A';
	t2replay_exact_diag.magic[6] = 'G';
	t2replay_exact_diag.magic[7] = '1';
	t2replay_exact_diag.version = 1;
}

static void near t2replay_exact_diag_flush(void)
{
#if T2REPLAY_EXACT_TRACE
	int fd;

	t2replay_dos_delete(t2replay_exact_diag_fn);
	fd = t2replay_dos_create(t2replay_exact_diag_fn);
	if(fd >= 0) {
		t2replay_dos_write(fd, &t2replay_exact_diag, sizeof(t2replay_exact_diag));
		t2replay_dos_close(fd);
	}
#endif
}

static bool near t2replay_exact_request_magic_matches(
	const char far *magic
)
{
	return (
		(magic[0] == 'T') && (magic[1] == '2') && (magic[2] == 'X') &&
		(magic[3] == 'A') && (magic[4] == 'P') && (magic[5] == '1') &&
		(magic[6] == '\0') && (magic[7] == '\0')
	);
}

static bool near t2replay_exact_request_delete(void)
{
	unsigned fn_seg = T2REPLAY_FP_SEG(t2replay_exact_request_fn);
	unsigned fn_off = T2REPLAY_FP_OFF(t2replay_exact_request_fn);
	unsigned deleted;

	_asm {
		push	ds
		mov	dx, fn_off
		mov	ds, fn_seg
		mov	ah, 41h
		int	21h
		pop	ds
		sbb	ax, ax
		not	ax
		mov	deleted, ax
	}
	if(deleted != 0) {
		t2replay_dos_flush();
		return true;
	}
	return false;
}

static bool near t2replay_exact_anchor_prepare(bool derive_run_offset)
{
	t2replay_packet_t far *packet;
	uint32_t packet_index = 0;
	uint32_t samples = 0;
	uint32_t anchor_offset;
	uint32_t prefix_checksum = T2REPLAY_FNV1A_BASIS;
	uint32_t remaining;
	unsigned want;
	unsigned len;
	unsigned i;
	int fd;
	uint8_t phase;
	uint8_t last_stage = 0xFF;
	uint8_t expected_stage = static_cast<uint8_t>(
		t2replay_header.start.stage
	);
	uint8_t run_offset = t2replay_exact_request.run_offset;
	bool stage_seen = false;

	if(t2replay_exact_request.packet_anchor >= t2replay_header.packet_count) {
		return false;
	}
	fd = t2replay_dos_open(t2replay_slot_fn, T2REPLAY_DOS_ACCESS_READ);
	if(fd < 0) {
		return false;
	}
	if(!t2replay_dos_seek(fd, t2replay_header.input_offset)) {
		t2replay_dos_close(fd);
		return false;
	}
	while(packet_index <= t2replay_exact_request.packet_anchor) {
		remaining = (
			t2replay_exact_request.packet_anchor - packet_index + 1
		);
		want = static_cast<unsigned>(
			(remaining > T2REPLAY_BUFFER_PACKET_COUNT)
				? T2REPLAY_BUFFER_PACKET_COUNT : remaining
		);
		len = (want * T2REPLAY_PACKET_SIZE);
		if(t2replay_dos_read(fd, t2replay_buffer, len) != len) {
			t2replay_dos_close(fd);
			return false;
		}
		for(i = 0; i < want; i++, packet_index++) {
			packet = &t2replay_buffer[i];
			if(packet_index == t2replay_exact_request.packet_anchor) {
				t2replay_pending = *packet;
				packet_index++;
				break;
			}
			prefix_checksum = t2replay_fnv1a(
				prefix_checksum, packet, T2REPLAY_PACKET_SIZE
			);
			phase = static_cast<uint8_t>(
				packet->tag >> T2REPLAY_PACKET_PHASE_SHIFT
			);
			if(phase < T2REPLAY_PHASE_CONTROL) {
				samples += static_cast<uint32_t>(
					(packet->tag & T2REPLAY_PACKET_RUN_MASK) + 1
				);
			} else if(
				(packet->tag & T2REPLAY_PACKET_RUN_MASK) ==
				T2REPLAY_CONTROL_STAGE_START
			) {
				if(packet->input_low != expected_stage) {
					t2replay_dos_close(fd);
					return false;
				}
				last_stage = packet->input_low;
				expected_stage++;
				stage_seen = true;
			} else if(
				(packet->tag & T2REPLAY_PACKET_RUN_MASK) ==
				T2REPLAY_CONTROL_TERMINAL
			) {
				t2replay_dos_close(fd);
				return false;
			}
		}
	}
	t2replay_dos_close(fd);
	phase = static_cast<uint8_t>(
		t2replay_pending.tag >> T2REPLAY_PACKET_PHASE_SHIFT
	);
	if(derive_run_offset) {
		if(t2replay_exact_request.sample_anchor < samples) {
			return false;
		}
		anchor_offset = t2replay_exact_request.sample_anchor - samples;
		if(anchor_offset > T2REPLAY_PACKET_RUN_MASK) {
			return false;
		}
		run_offset = static_cast<uint8_t>(anchor_offset);
		t2replay_exact_request.run_offset = run_offset;
	}
	if(
		!stage_seen || (last_stage != t2replay_exact_request.stage_id) ||
		(phase != T2REPLAY_PHASE_GAMEPLAY) ||
		(t2replay_pending.arg != 0) ||
		(run_offset >
		 (t2replay_pending.tag & T2REPLAY_PACKET_RUN_MASK)) ||
		((samples + run_offset) !=
		 t2replay_exact_request.sample_anchor) ||
		(t2replay_exact_request.sample_anchor >=
		 t2replay_header.sample_count) ||
		(prefix_checksum != t2replay_exact_request.prefix_checksum)
	) {
		return false;
	}
	t2replay_buffer_len = 0;
	t2replay_buffer_pos = 0;
	t2replay_packet_cursor = t2replay_exact_request.packet_anchor + 1;
	t2replay_sample_cursor = t2replay_exact_request.sample_anchor;
	t2replay_decode_run = static_cast<uint8_t>(
		(t2replay_pending.tag & T2REPLAY_PACKET_RUN_MASK) + 1 -
		run_offset
	);
	t2replay_stage_seen = true;
	t2replay_last_stage = last_stage;
	return true;
}

static t2replay_exact_load_result_t near t2replay_exact_request_load(
	uint8_t slot, bool replay_header_valid
)
{
	struct t2rec_boundary_t boundary;
	uint32_t file_size;
	uint32_t stored_checksum;
	uint32_t computed_checksum;
	int fd;
	unsigned read;
	bool extent_valid;

	fd = t2replay_dos_open(
		t2replay_exact_request_fn, T2REPLAY_DOS_ACCESS_READ
	);
	if(fd < 0) {
		return T2XLR_ABSENT;
	}
	t2replay_exact_graph_hide();
	t2replay_exact_diag_init();
	t2replay_memclear(
		&t2replay_exact_request, sizeof(t2replay_exact_request)
	);
	read = t2replay_dos_read(
		fd, &t2replay_exact_request, sizeof(t2replay_exact_request)
	);
	if(!t2replay_dos_size(fd, &file_size) ||
		!t2replay_dos_seek(fd, sizeof(t2replay_exact_request))) {
		file_size = 0;
	}
	extent_valid = (
		(read != sizeof(t2replay_exact_request)) ||
		(file_size != T2REPLAY_EXACT_APPLY_FILE_SIZE)
	) ? false : true;
	if(extent_valid) {
		t2replay_exact_envelope = reinterpret_cast<uint8_t far *>(
			hmem_allocbyte(T2REPLAY_EXACT_S5CBRD_CAPTURE_SIZE)
		);
		if(t2replay_exact_envelope != 0) {
			read = t2replay_dos_read(
				fd, t2replay_exact_envelope,
				T2REPLAY_EXACT_S5CBRD_CAPTURE_SIZE
			);
		}
	}
	t2replay_dos_close(fd);
	// T2XAP1.BIN is a one-shot private command, including malformed requests.
	// A failed deletion leaves graphics hidden and returns through OP without
	// admitting this or any later launch of the stale request.
	if(!t2replay_exact_request_delete()) {
		t2replay_exact_envelope_free();
		return T2XLR_REJECTED;
	}
	if(
		!replay_header_valid || !extent_valid ||
		(t2replay_exact_envelope == 0) ||
		(read != T2REPLAY_EXACT_S5CBRD_CAPTURE_SIZE)
	) {
		t2replay_exact_envelope_free();
		return T2XLR_REJECTED;
	}
	stored_checksum = t2replay_exact_request.request_checksum;
	t2replay_exact_request.request_checksum = 0;
	computed_checksum = t2replay_fnv1a(
		T2REPLAY_FNV1A_BASIS, &t2replay_exact_request,
		sizeof(t2replay_exact_request)
	);
	computed_checksum = t2replay_fnv1a(
		computed_checksum, t2replay_exact_envelope,
		T2REPLAY_EXACT_S5CBRD_CAPTURE_SIZE
	);
	t2replay_exact_request.request_checksum = stored_checksum;
	t2replay_memclear(&boundary, sizeof(boundary));
	boundary.at_ordinary_stage_loop_top = 1;
	boundary.stage_init_complete = 1;
	boundary.stage_progression = SP_BOSS;
	if(
		!t2replay_exact_request_magic_matches(
			t2replay_exact_request.magic
		) ||
		(t2replay_exact_request.version !=
		 T2REPLAY_EXACT_APPLY_REQUEST_VERSION) ||
		(t2replay_exact_request.header_size !=
		 T2REPLAY_EXACT_APPLY_REQUEST_SIZE) ||
		(t2replay_exact_request.total_size !=
		 T2REPLAY_EXACT_APPLY_FILE_SIZE) ||
		(t2replay_exact_request.envelope_size !=
		 T2REPLAY_EXACT_S5CBRD_CAPTURE_SIZE) ||
		(t2replay_exact_request.replay_header_checksum !=
		 t2replay_header.header_checksum) ||
		(t2replay_exact_request.slot != slot) ||
		(t2replay_exact_request.stage_id != 4) ||
		(t2replay_exact_request.phase != T2REPLAY_PHASE_GAMEPLAY) ||
		(t2replay_exact_request.reserved != 0) ||
		(stored_checksum != computed_checksum) ||
		(t2replay_checkpoint_get_u16(t2replay_exact_envelope, 8) !=
		 T2REPLAY_EXACT_S5CBRD_SCHEMA) ||
		(t2replay_exact_envelope[T2REC_HEADER_SHOTTYPE] !=
		 t2replay_header.start.shottype) ||
		(t2replay_exact_envelope[T2REC_HEADER_RANK] !=
		 t2replay_header.start.rank) ||
		(t2replay_exact_envelope[T2REC_HEADER_REDUCE_EFFECTS] !=
		 t2replay_header.start.reduce_effects) ||
		(replay_exact_checkpoint_validate(
			t2replay_exact_envelope,
			T2REPLAY_EXACT_S5CBRD_CAPTURE_SIZE, &boundary
		) != T2REC_DEFERRED_CODECS) ||
		!t2replay_exact_anchor_prepare(false)
	) {
		t2replay_exact_envelope_free();
		return T2XLR_REJECTED;
	}
	t2replay_exact_pending = true;
	t2replay_exact_anchor_sample_pending = true;
	t2replay_exact_diag.state = T2XDS_PREPARED;
	t2replay_exact_diag.phase = t2replay_exact_request.phase;
	t2replay_exact_diag.sample_anchor = t2replay_exact_request.sample_anchor;
	t2replay_exact_diag.packet_anchor = t2replay_exact_request.packet_anchor;
	return T2XLR_READY;
}

static bool near t2replay_public_seek_request_magic_matches(
	const char far *magic
)
{
	return (
		(magic[0] == 'T') && (magic[1] == '2') && (magic[2] == 'R') &&
		(magic[3] == 'S') && (magic[4] == 'Q') && (magic[5] == '1') &&
		(magic[6] == '\0') && (magic[7] == '\0')
	);
}

static bool near t2replay_public_seek_sidecar_magic_matches(
	const char far *magic
)
{
	return (
		(magic[0] == 'T') && (magic[1] == '2') && (magic[2] == 'R') &&
		(magic[3] == 'S') && (magic[4] == 'K') && (magic[5] == '1') &&
		(magic[6] == '\0') && (magic[7] == '\0')
	);
}

static bool near t2replay_public_seek_request_delete(void)
{
	unsigned fn_seg = T2REPLAY_FP_SEG(t2replay_public_seek_request_fn);
	unsigned fn_off = T2REPLAY_FP_OFF(t2replay_public_seek_request_fn);
	unsigned deleted;

	_asm {
		push	ds
		mov	dx, fn_off
		mov	ds, fn_seg
		mov	ah, 41h
		int	21h
		pop	ds
		sbb	ax, ax
		not	ax
		mov	deleted, ax
	}
	if(deleted != 0) {
		t2replay_dos_flush();
		return true;
	}
	return false;
}

static bool near t2replay_public_seek_sidecar_checksum(
	int fd, uint32_t size, uint32_t far *checksum
)
{
	uint8_t bytes[64];
	uint32_t offset = 0;
	uint32_t hash = T2REPLAY_FNV1A_BASIS;
	unsigned want;
	unsigned i;

	if(!t2replay_dos_seek(fd, 0)) {
		return false;
	}
	while(offset < size) {
		want = static_cast<unsigned>(
			((size - offset) > sizeof(bytes)) ? sizeof(bytes) : (size - offset)
		);
		if(t2replay_dos_read(fd, bytes, want) != want) {
			return false;
		}
		for(i = 0; i < want; i++) {
			if(
				((offset + i) >= 0x2C) &&
				((offset + i) < (0x2C + sizeof(uint32_t)))
			) {
				bytes[i] = 0;
			}
		}
		hash = t2replay_fnv1a(hash, bytes, want);
		offset += want;
	}
	*checksum = hash;
	return true;
}

static uint32_t t2replay_exact_s5_mima_semantic_digest(
	const uint8_t far *envelope
)
{
	uint32_t hash = T2REPLAY_FNV1A_BASIS;
	const uint8_t far *group;
	uint8_t group_id;
	uint32_t size;

	for(group_id = 0; group_id < T2REPLAY_EXACT_GROUP_COUNT; group_id++) {
		group = envelope + T2REPLAY_EXACT_HEADER_SIZE +
			(static_cast<uint32_t>(group_id) * T2REPLAY_EXACT_GROUP_SIZE);
		size = t2replay_checkpoint_get_u32(group, T2RCK_GROUP_STORED_SIZE);
		hash = t2replay_fnv1a(hash, &group_id, sizeof(group_id));
		hash = t2replay_fnv1a(hash, &size, sizeof(size));
		hash = t2replay_fnv1a(
			hash,
			envelope + t2replay_checkpoint_get_u32(
				group, T2RCK_GROUP_OFFSET
			),
			static_cast<unsigned>(size)
		);
	}
	return hash;
}

static bool near t2replay_public_seek_sidecar_load(
	uint8_t slot, const t2replay_public_seek_request_t far *request
)
{
	t2replay_public_seek_header_t header;
	t2replay_public_seek_entry_t entry;
	struct t2rec_boundary_t boundary;
	uint32_t file_size;
	uint32_t checksum;
	uint32_t expected_offset;
	int fd;
	unsigned read;

	fd = t2replay_dos_open(
		t2replay_public_seek_sidecar_fn, T2REPLAY_DOS_ACCESS_READ
	);
	if(fd < 0) {
		return false;
	}
	t2replay_memclear(&header, sizeof(header));
	t2replay_memclear(&entry, sizeof(entry));
	read = t2replay_dos_read(fd, &header, sizeof(header));
	if(
		(read != sizeof(header)) || !t2replay_dos_size(fd, &file_size) ||
		!t2replay_public_seek_sidecar_checksum(fd, file_size, &checksum) ||
		!t2replay_public_seek_sidecar_magic_matches(header.magic) ||
		(header.version != T2REPLAY_PUBLIC_SEEK_VERSION) ||
		(header.header_size != T2REPLAY_PUBLIC_SEEK_HEADER_SIZE) ||
		(header.entry_size != T2REPLAY_PUBLIC_SEEK_ENTRY_SIZE) ||
		(header.entry_count != 1) ||
		(header.total_size != file_size) ||
		(header.replay_header_checksum != t2replay_header.header_checksum) ||
		(header.replay_payload_checksum != t2replay_header.payload_checksum) ||
		(header.format_fingerprint != T2REPLAY_PUBLIC_SEEK_FORMAT_FINGERPRINT) ||
		(header.replay_sample_count != t2replay_header.sample_count) ||
		(header.replay_packet_count != t2replay_header.packet_count) ||
		(header.sidecar_checksum != checksum) ||
		!t2replay_bytes_zero(header.reserved, sizeof(header.reserved)) ||
		(request->slot != slot) ||
		(request->entry_index != 0) ||
		(request->replay_header_checksum != header.replay_header_checksum) ||
		(request->replay_payload_checksum != header.replay_payload_checksum) ||
		(request->sidecar_checksum != header.sidecar_checksum) ||
		!t2replay_dos_seek(fd, header.header_size) ||
		(t2replay_dos_read(fd, &entry, sizeof(entry)) != sizeof(entry))
	) {
		t2replay_dos_close(fd);
		return false;
	}
	checksum = t2replay_fnv1a(T2REPLAY_FNV1A_BASIS, &entry, sizeof(entry));
	expected_offset = (
		T2REPLAY_PUBLIC_SEEK_HEADER_SIZE + T2REPLAY_PUBLIC_SEEK_ENTRY_SIZE
	);
	if(
		(header.directory_checksum != checksum) ||
		(entry.stage_id != 4) ||
		(entry.target_kind != T2REPLAY_PUBLIC_SEEK_TARGET_BOSS) ||
		(entry.actor_tag != T2REAT_S5_MIMA) ||
		(entry.actor_mode != T2REAM_ACTIVE) ||
		(entry.stage_fx_tag != T2RESFT_S5_MIMA_FIELD) ||
		(entry.callback_profile != T2RECP_S5_MIMA) ||
		(entry.redraw_recipe != T2RERR_NATIVE_ONE_FRAME_REVEAL) ||
		(entry.capture_generation != T2REPLAY_EXACT_BOUNDARY_GENERATION) ||
		(entry.checkpoint_schema != T2REPLAY_EXACT_S5CBRD_SCHEMA) ||
		(entry.group_count != T2REPLAY_EXACT_GROUP_COUNT) ||
		(entry.reserved_0 != 0) || (entry.reserved_1 != 0) ||
		(entry.checkpoint_offset != expected_offset) ||
		(entry.checkpoint_size != T2REPLAY_EXACT_S5CBRD_CAPTURE_SIZE) ||
		(entry.source_fingerprint != T2REPLAY_EXACT_S5CBRD_SOURCE_FINGERPRINT) ||
		(entry.checkpoint_offset > header.total_size) ||
		(entry.checkpoint_size > (header.total_size - entry.checkpoint_offset)) ||
		((entry.checkpoint_offset + entry.checkpoint_size) != header.total_size)
	) {
		t2replay_dos_close(fd);
		return false;
	}
	t2replay_exact_envelope = reinterpret_cast<uint8_t far *>(
		hmem_allocbyte(T2REPLAY_EXACT_S5CBRD_CAPTURE_SIZE)
	);
	if(
		(t2replay_exact_envelope == 0) ||
		!t2replay_dos_seek(fd, entry.checkpoint_offset) ||
		(t2replay_dos_read(
			fd, t2replay_exact_envelope, T2REPLAY_EXACT_S5CBRD_CAPTURE_SIZE
		) != T2REPLAY_EXACT_S5CBRD_CAPTURE_SIZE)
	) {
		t2replay_dos_close(fd);
		t2replay_exact_envelope_free();
		return false;
	}
	t2replay_dos_close(fd);
	t2replay_memclear(&boundary, sizeof(boundary));
	boundary.at_ordinary_stage_loop_top = 1;
	boundary.stage_init_complete = 1;
	boundary.stage_progression = SP_BOSS;
	if(
		(t2replay_exact_envelope[T2REC_HEADER_SHOTTYPE] !=
		 t2replay_header.start.shottype) ||
		(t2replay_exact_envelope[T2REC_HEADER_RANK] !=
		 t2replay_header.start.rank) ||
		(t2replay_exact_envelope[T2REC_HEADER_REDUCE_EFFECTS] !=
		 t2replay_header.start.reduce_effects) ||
		(replay_exact_checkpoint_validate(
			t2replay_exact_envelope,
			T2REPLAY_EXACT_S5CBRD_CAPTURE_SIZE, &boundary
		) != T2REC_DEFERRED_CODECS) ||
		(entry.checkpoint_checksum != t2replay_exact_checkpoint_checksum(
			t2replay_exact_envelope, T2REPLAY_EXACT_S5CBRD_CAPTURE_SIZE
		)) ||
		(entry.semantic_digest != t2replay_exact_s5_mima_semantic_digest(
			t2replay_exact_envelope
		))
	) {
		t2replay_exact_envelope_free();
		return false;
	}
	t2replay_memclear(&t2replay_exact_request, sizeof(t2replay_exact_request));
	t2replay_exact_request.slot = slot;
	t2replay_exact_request.stage_id = entry.stage_id;
	t2replay_exact_request.phase = T2REPLAY_PHASE_GAMEPLAY;
	t2replay_exact_request.packet_anchor = entry.packet_anchor;
	t2replay_exact_request.sample_anchor = entry.sample_anchor;
	t2replay_exact_request.prefix_checksum = entry.prefix_checksum;
	t2replay_exact_request.replay_header_checksum = header.replay_header_checksum;
	if(
		!t2replay_exact_anchor_prepare(true)
	) {
		t2replay_exact_envelope_free();
		return false;
	}
	t2replay_exact_pending = true;
	t2replay_exact_anchor_sample_pending = true;
	t2replay_exact_diag.state = T2XDS_PREPARED;
	t2replay_exact_diag.phase = t2replay_exact_request.phase;
	t2replay_exact_diag.sample_anchor = t2replay_exact_request.sample_anchor;
	t2replay_exact_diag.packet_anchor = t2replay_exact_request.packet_anchor;
	return true;
}

static t2replay_exact_load_result_t near t2replay_public_seek_request_load(
	uint8_t slot, bool replay_header_valid
)
{
	t2replay_public_seek_request_t request;
	uint32_t file_size;
	uint32_t stored_checksum;
	uint32_t computed_checksum;
	int fd;
	unsigned read;

	fd = t2replay_dos_open(
		t2replay_public_seek_request_fn, T2REPLAY_DOS_ACCESS_READ
	);
	if(fd < 0) {
		return T2XLR_ABSENT;
	}
	t2replay_exact_graph_hide();
	t2replay_exact_diag_init();
	t2replay_memclear(&request, sizeof(request));
	read = t2replay_dos_read(fd, &request, sizeof(request));
	if(!t2replay_dos_size(fd, &file_size)) {
		file_size = 0;
	}
	t2replay_dos_close(fd);
	// T2RSQ1.BIN is one-shot even while this non-public gate is compiled.
	// A failed delete keeps MAIN hidden and refuses the launch.
	if(!t2replay_public_seek_request_delete()) {
		return T2XLR_REJECTED;
	}
	stored_checksum = request.request_checksum;
	request.request_checksum = 0;
	computed_checksum = t2replay_fnv1a(
		T2REPLAY_FNV1A_BASIS, &request, sizeof(request)
	);
	request.request_checksum = stored_checksum;
	if(
		(read != sizeof(request)) ||
		(file_size != sizeof(request)) ||
		!replay_header_valid ||
		!t2replay_public_seek_request_magic_matches(request.magic) ||
		(request.version != T2REPLAY_PUBLIC_SEEK_VERSION) ||
		(request.header_size != T2REPLAY_PUBLIC_SEEK_REQUEST_SIZE) ||
		(request.slot != slot) ||
		(request.reserved_0 != 0) ||
		!t2replay_bytes_zero(request.reserved, sizeof(request.reserved)) ||
		(stored_checksum != computed_checksum) ||
		!t2replay_public_seek_sidecar_load(slot, &request)
	) {
		t2replay_exact_envelope_free();
		return T2XLR_REJECTED;
	}
	return T2XLR_READY;
}

static bool near t2xobs_req_magic_matches(
	const char far *magic
)
{
	return (
		(magic[0] == 'T') && (magic[1] == '2') && (magic[2] == 'X') &&
		(magic[3] == 'O') && (magic[4] == 'B') && (magic[5] == 'Q') &&
		(magic[6] == '\0') && (magic[7] == '\0')
	);
}

static bool near t2xobs_req_delete(void)
{
	unsigned fn_seg = T2REPLAY_FP_SEG(t2xobs_req_fn);
	unsigned fn_off = T2REPLAY_FP_OFF(t2xobs_req_fn);
	unsigned deleted;

	_asm {
		push	ds
		mov	dx, fn_off
		mov	ds, fn_seg
		mov	ah, 41h
		int	21h
		pop	ds
		sbb	ax, ax
		not	ax
		mov	deleted, ax
	}
	if(deleted != 0) {
		t2replay_dos_flush();
		return true;
	}
	return false;
}

static bool near t2xobs_anchor_valid(
	uint32_t sample_anchor, uint32_t far *packet_anchor
)
{
	t2replay_packet_t far *packet;
	uint32_t packet_index = 0;
	uint32_t samples = 0;
	uint32_t remaining;
	unsigned want;
	unsigned len;
	unsigned i;
	int fd;
	uint8_t expected_stage = static_cast<uint8_t>(t2replay_header.start.stage);
	uint8_t last_stage = 0xFF;
	uint8_t phase;
	bool stage_seen = false;

	// This private scanner borrows the ordinary packet buffer. It never alters
	// the decoder cursors, and always leaves the next playback read cold.
	t2replay_buffer_len = 0;
	t2replay_buffer_pos = 0;
	if(sample_anchor >= t2replay_header.sample_count) {
		return false;
	}
	fd = t2replay_dos_open(t2replay_slot_fn, T2REPLAY_DOS_ACCESS_READ);
	if(fd < 0) {
		return false;
	}
	if(!t2replay_dos_seek(fd, t2replay_header.input_offset)) {
		t2replay_dos_close(fd);
		return false;
	}
	while(packet_index < t2replay_header.packet_count) {
		remaining = (t2replay_header.packet_count - packet_index);
		want = static_cast<unsigned>(
			(remaining > T2REPLAY_BUFFER_PACKET_COUNT)
				? T2REPLAY_BUFFER_PACKET_COUNT : remaining
		);
		len = (want * T2REPLAY_PACKET_SIZE);
		if(t2replay_dos_read(fd, t2replay_buffer, len) != len) {
			t2replay_dos_close(fd);
			return false;
		}
		for(i = 0; i < want; i++, packet_index++) {
			packet = &t2replay_buffer[i];
			phase = static_cast<uint8_t>(
				packet->tag >> T2REPLAY_PACKET_PHASE_SHIFT
			);
			if(phase < T2REPLAY_PHASE_CONTROL) {
				uint32_t run = static_cast<uint32_t>(
					(packet->tag & T2REPLAY_PACKET_RUN_MASK) + 1
				);

				if((samples <= sample_anchor) &&
					(sample_anchor < (samples + run))) {
					t2replay_dos_close(fd);
					if(
						(phase != T2REPLAY_PHASE_GAMEPLAY) ||
						(packet->arg != 0) || !stage_seen ||
						(last_stage != 4)
					) {
						return false;
					}
					*packet_anchor = packet_index;
					t2replay_buffer_len = 0;
					t2replay_buffer_pos = 0;
					return true;
				}
				samples += run;
			} else if(
				(packet->tag & T2REPLAY_PACKET_RUN_MASK) ==
				T2REPLAY_CONTROL_STAGE_START
			) {
				if(
					(packet->input_low != expected_stage) ||
					(packet->input_high != 0) || (packet->arg != 0)
				) {
					t2replay_dos_close(fd);
					return false;
				}
				last_stage = packet->input_low;
				expected_stage++;
				stage_seen = true;
			} else if(
				(packet->tag & T2REPLAY_PACKET_RUN_MASK) ==
				T2REPLAY_CONTROL_TERMINAL
			) {
				t2replay_dos_close(fd);
				return false;
			} else {
				t2replay_dos_close(fd);
				return false;
			}
		}
	}
	t2replay_dos_close(fd);
	return false;
}

static uint32_t near t2xobs_vram_digest(uint8_t page)
{
	uint32_t digest = T2REPLAY_FNV1A_BASIS;
	uint8_t plane;

	graph_accesspage(page);
	for(plane = 0; plane < PLANE_COUNT; plane++) {
		digest = t2replay_fnv1a(digest, VRAM_PLANE[plane], PLANE_SIZE);
	}
	return digest;
}

static uint32_t near t2xobs_tram_digest(void)
{
	const uint8_t far *jis = reinterpret_cast<const uint8_t far *>(
		MK_FP(SEG_TRAM_JIS, 0)
	);
	const uint8_t far *atrb = reinterpret_cast<const uint8_t far *>(
		MK_FP(SEG_TRAM_ATRB, 0)
	);
	uint32_t digest = T2REPLAY_FNV1A_BASIS;

	// 80 columns x 25 rows x one 16-bit value in each visible TRAM plane.
	digest = t2replay_fnv1a(digest, jis, (80 * 25 * sizeof(uint16_t)));
	return t2replay_fnv1a(digest, atrb, (80 * 25 * sizeof(uint16_t)));
}

static uint32_t near t2xobs_out_checksum(void)
{
	uint32_t stored = t2xobs_out.header.output_checksum;
	uint32_t checksum;

	t2xobs_out.header.output_checksum = 0;
	checksum = t2replay_fnv1a(
		T2REPLAY_FNV1A_BASIS, &t2xobs_out,
		sizeof(t2xobs_out)
	);
	t2xobs_out.header.output_checksum = stored;
	return checksum;
}

static void near t2xobs_flush(void)
{
	int fd;

	if(!t2xobs_active) {
		return;
	}
	t2xobs_out.header.output_checksum =
		t2xobs_out_checksum();
	t2replay_dos_delete(t2xobs_out_fn);
	fd = t2replay_dos_create(t2xobs_out_fn);
	if(fd >= 0) {
		if(t2replay_dos_write(
			fd, &t2xobs_out,
			sizeof(t2xobs_out)
		) == sizeof(t2xobs_out)) {
			t2replay_dos_flush();
		}
		t2replay_dos_close(fd);
	}
}

static t2xobs_rec_t near *
t2xobs_rec_begin(uint8_t kind, uint8_t phase)
{
	t2xobs_rec_t near *record;
	uint16_t index = t2xobs_out.header.record_count;

	if(index >= T2XOBS_REC_MAX) {
		return 0;
	}
	record = &t2xobs_out.record[index];
	t2replay_memclear(record, sizeof(*record));
	record->kind = kind;
	record->phase = phase;
	record->stage_id = static_cast<uint8_t>(stage_id);
	record->flags = (
		t2xobs_direct
			? T2XOBS_F_DIR : 0
	);
	record->page_front = page_front;
	record->page_back = page_back;
	record->stage_progression = stage_progression;
	record->callback_profile = T2RECP_S5_MIMA;
	record->sample_cursor = t2replay_sample_cursor;
	record->packet_cursor = t2replay_packet_cursor;
	record->resident_frame = resident->frame;
	record->stage_frame = stage_frame;
	record->scroll_line = static_cast<uint16_t>(scroll_line);
	record->scroll_line_page_0 = static_cast<uint16_t>(
		t2replay_scroll_pages.line[0]
	);
	record->scroll_line_page_1 = static_cast<uint16_t>(
		t2replay_scroll_pages.line[1]
	);
	record->palette_tone = static_cast<uint16_t>(PaletteTone);
	record->redraw_recipe = T2RERR_NATIVE_ONE_FRAME_REVEAL;
	return record;
}

static void near t2xobs_rec_finish(
	t2xobs_rec_t near *record
)
{
	if(record == 0) {
		return;
	}
	record->record_checksum = 0;
	record->record_checksum = t2replay_fnv1a(
		T2REPLAY_FNV1A_BASIS, record, sizeof(*record)
	);
	t2xobs_out.header.record_count++;
	t2xobs_flush();
}

static void near t2xobs_semantic_record(
	uint8_t kind, uint8_t phase
)
{
	struct t2rec_boundary_t boundary;
	t2xobs_rec_t near *record;
	uint8_t far *capture;

	if(!t2xobs_active) {
		return;
	}
	record = t2xobs_rec_begin(kind, phase);
	if(record == 0) {
		return;
	}
	if((stage_id == 4) && (stage_progression == SP_BOSS)) {
		t2replay_memclear(&boundary, sizeof(boundary));
		boundary.at_ordinary_stage_loop_top = 1;
		boundary.stage_init_complete = 1;
		boundary.stage_progression = stage_progression;
		capture = reinterpret_cast<uint8_t far *>(
			hmem_allocbyte(T2REPLAY_EXACT_S5CBRD_CAPTURE_SIZE)
		);
		if(
			(capture != 0) && replay_exact_stage5_mima_callback_redraw_capture(
				capture, T2REPLAY_EXACT_S5CBRD_CAPTURE_SIZE, &boundary
			)
		) {
			record->flags |= T2XOBS_F_SEM;
			record->semantic_digest =
				t2replay_exact_s5_mima_semantic_digest(capture);
			record->container_checksum = t2replay_exact_checkpoint_checksum(
				capture, T2REPLAY_EXACT_S5CBRD_CAPTURE_SIZE
			);
		}
		if(capture != 0) {
			hmem_free(reinterpret_cast<void __seg *>(capture));
		}
	}
	if(
		(kind == T2XOBS_REF_NEXT) ||
		(kind == T2XOBS_DIR_NEXT)
	) {
		record->flags |= T2XOBS_F_PRES;
		record->vram_front_digest =
			t2xobs_vram_digest(page_front);
		record->vram_back_digest =
			t2xobs_vram_digest(page_back);
		graph_accesspage(page_back);
		record->tram_digest = t2xobs_tram_digest();
	}
	t2xobs_rec_finish(record);
}

static void near t2xobs_reveal(void)
{
	t2xobs_rec_t near *record;

	if(!t2xobs_active || !t2xobs_direct) {
		return;
	}
	record = t2xobs_rec_begin(
		T2XOBS_DIR_REVEAL, T2REPLAY_PHASE_GAMEPLAY
	);
	if(record == 0) {
		return;
	}
	record->flags |= T2XOBS_F_PRES;
	record->vram_front_digest = t2xobs_vram_digest(page_front);
	record->vram_back_digest = t2xobs_vram_digest(page_back);
	graph_accesspage(page_back);
	record->tram_digest = t2xobs_tram_digest();
	t2xobs_rec_finish(record);
}

static void near t2xobs_terminal(void)
{
	t2xobs_rec_t near *record;

	if(!t2xobs_active) {
		return;
	}
	record = t2xobs_rec_begin(
		T2XOBS_TERMINAL, T2REPLAY_PHASE_CONTROL
	);
	t2xobs_rec_finish(record);
}

static bool near t2xobs_req_load(
	uint8_t slot, bool replay_header_valid
)
{
	uint32_t file_size;
	uint32_t stored_checksum;
	uint32_t packet_anchor;
	uint32_t computed_checksum;
	unsigned read;
	int fd;

	fd = t2replay_dos_open(
		t2xobs_req_fn, T2REPLAY_DOS_ACCESS_READ
	);
	if(fd < 0) {
		return false;
	}
	t2replay_memclear(
		&t2xobs_req,
		sizeof(t2xobs_req)
	);
	read = t2replay_dos_read(
		fd, &t2xobs_req,
		sizeof(t2xobs_req)
	);
	if(!t2replay_dos_size(fd, &file_size)) {
		file_size = 0;
	}
	t2replay_dos_close(fd);
	// The observer request is private and one-shot even when malformed.
	if(!t2xobs_req_delete()) {
		return false;
	}
	stored_checksum = t2xobs_req.request_checksum;
	t2xobs_req.request_checksum = 0;
	computed_checksum = t2replay_fnv1a(
		T2REPLAY_FNV1A_BASIS, &t2xobs_req,
		sizeof(t2xobs_req)
	);
	t2xobs_req.request_checksum = stored_checksum;
	if(
		(read != sizeof(t2xobs_req)) ||
		(file_size != sizeof(t2xobs_req)) ||
		!replay_header_valid ||
		!t2xobs_req_magic_matches(
			t2xobs_req.magic
		) ||
		(t2xobs_req.version !=
		 T2XOBS_VERSION) ||
		(t2xobs_req.header_size !=
		 T2XOBS_REQ_SIZE) ||
		(t2xobs_req.slot != slot) ||
		(t2xobs_req.stage_id != 4) ||
		(t2xobs_req.reserved_0 != 0) ||
		!t2replay_bytes_zero(
			t2xobs_req.reserved,
			sizeof(t2xobs_req.reserved)
		) ||
		(t2xobs_req.replay_header_checksum !=
		 t2replay_header.header_checksum) ||
		(t2xobs_req.replay_payload_checksum !=
		 t2replay_header.payload_checksum) ||
		(stored_checksum != computed_checksum) ||
		!t2xobs_anchor_valid(
			t2xobs_req.sample_anchor, &packet_anchor
		) ||
		(t2replay_exact_pending &&
		 ((t2xobs_req.sample_anchor !=
		   t2replay_exact_request.sample_anchor) ||
		  (packet_anchor != t2replay_exact_request.packet_anchor)))
	) {
		return false;
	}
	t2replay_memclear(
		&t2xobs_out,
		sizeof(t2xobs_out)
	);
	t2xobs_out.header.magic[0] = 'T';
	t2xobs_out.header.magic[1] = '2';
	t2xobs_out.header.magic[2] = 'X';
	t2xobs_out.header.magic[3] = 'O';
	t2xobs_out.header.magic[4] = 'B';
	t2xobs_out.header.magic[5] = 'S';
	t2xobs_out.header.magic[6] = '1';
	t2xobs_out.header.version =
		T2XOBS_VERSION;
	t2xobs_out.header.header_size =
		T2XOBS_HDR_SIZE;
	t2xobs_out.header.record_size =
		T2XOBS_REC_SIZE;
	t2xobs_out.header.replay_header_checksum =
		t2replay_header.header_checksum;
	t2xobs_out.header.replay_payload_checksum =
		t2replay_header.payload_checksum;
	t2xobs_out.header.request_checksum = stored_checksum;
	t2xobs_active = true;
	t2xobs_direct = t2replay_exact_pending;
	return true;
}
#endif

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
	uint16_t date_year;
	uint8_t date_month;
	uint8_t date_day;
	uint8_t time_hour;
	uint8_t time_minute;
	uint8_t time_second;

	t2replay_memclear(&t2replay_header, sizeof(t2replay_header));
	t2replay_header.magic[0] = 'T';
	t2replay_header.magic[1] = '2';
	t2replay_header.magic[2] = 'R';
	t2replay_header.magic[3] = 'P';
	t2replay_header.magic[4] = 'Y';
	t2replay_header.magic[5] = '5';
	t2replay_header.version = T2REPLAY_VERSION;
	t2replay_header.header_size = T2REPLAY_HEADER_WIRE_SIZE;
	t2replay_header.packet_size = T2REPLAY_PACKET_SIZE;
	t2replay_header.flags = T2REPLAY_DEFAULT_FLAGS;
	t2replay_header.status = T2REPLAY_STATUS_RECORDING;
	t2replay_header.game_id = 2;
	t2replay_header.ruleset = T2REPLAY_RULESET_STOCK;
	t2replay_header.input_semantics = T2REPLAY_INPUT_SEMANTICS_KEY_DET;
	t2replay_header.stage_count = T2REPLAY_STAGE_COUNT;
	t2replay_header.input_offset = T2REPLAY_HEADER_WIRE_SIZE;
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
	t2replay_header.start.reserved[T2REPLAY_PRACTICE_PLAYPERF_OFFSET] =
		t2replay_practice_playperf_encode(playperf);
	t2replay_header.start.reserved[T2REPLAY_AUTOFIRE_OFFSET] =
		(t2replay_autofire_active ? 1 : 0);
	t2replay_autofire_release_frame = false;
	_asm {
		mov ah, 2Ah
		int 21h
		mov date_year, cx
		mov date_month, dh
		mov date_day, dl
		mov ah, 2Ch
		int 21h
		mov time_hour, ch
		mov time_minute, cl
		mov time_second, dh
	}
	if(date_year >= 1980) {
		t2replay_reserved_u16_set(
			T2REPLAY_RESERVED_DOS_DATE_OFFSET,
			static_cast<uint16_t>(
				((date_year - 1980) << 9) |
				(static_cast<uint16_t>(date_month) << 5) | date_day
			)
		);
	}
	t2replay_reserved_u16_set(
		T2REPLAY_RESERVED_DOS_TIME_OFFSET,
		static_cast<uint16_t>(
			(static_cast<uint16_t>(time_hour) << 11) |
			(static_cast<uint16_t>(time_minute) << 5) |
			(time_second >> 1)
		)
	);
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
	playperf = t2replay_practice_playperf_decode(
		start->reserved[T2REPLAY_PRACTICE_PLAYPERF_OFFSET]
	);
	t2replay_rank_lock_active = (
		start->reserved[T2REPLAY_PRACTICE_RANK_LOCK_OFFSET] != 0
	);
	t2replay_rank_lock_value = playperf;
	t2replay_autofire_active = (
		start->reserved[T2REPLAY_AUTOFIRE_OFFSET] != 0
	);
	t2replay_autofire_release_frame = false;
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
		 (command->mode != T2REPLAY_COMMAND_PLAYBACK) &&
		 (command->mode != T2REPLAY_COMMAND_PRACTICE)) ||
		((command->mode == T2REPLAY_COMMAND_PLAYBACK) &&
		 (command->slot >= T2REPLAY_SLOT_COUNT)) ||
		((command->mode == T2REPLAY_COMMAND_RECORD) &&
		 (command->slot >= T2REPLAY_SLOT_COUNT) &&
		 (command->slot != T2REPLAY_TEMP_SLOT)) ||
		((command->mode == T2REPLAY_COMMAND_PRACTICE) &&
		 (command->slot != T2REPLAY_TEMP_SLOT)) ||
		((command->flags & ~T2REPLAY_COMMAND_KNOWN_FLAGS) != 0)) {
		return false;
	}
	for(i = 0; i < sizeof(command->reserved); i++) {
		if(command->reserved[i] != 0) {
			return false;
		}
	}
	if(command->mode == T2REPLAY_COMMAND_PLAYBACK) {
		return (
			(((command->flags == 0) && (command->reserved_0 == 0)) ||
			 ((command->flags == T2REPLAY_COMMAND_FLAG_STAGE_SEEK) &&
			  (command->reserved_0 >= 1) &&
			  (command->reserved_0 <= T2REPLAY_STAGE_COUNT))) &&
			t2replay_bytes_zero(
				reinterpret_cast<const uint8_t far *>(&command->start),
				sizeof(command->start)
			)
		);
	}
	if(command->flags & T2REPLAY_COMMAND_FLAG_PRACTICE) {
		return (
			(command->mode != T2REPLAY_COMMAND_PLAYBACK) &&
			!(command->flags & T2REPLAY_COMMAND_FLAG_STAGE_SEEK) &&
			(command->reserved_0 == 0) &&
			t2replay_practice_start_valid(&command->start) &&
			(((command->flags & T2REPLAY_COMMAND_FLAG_AUTOFIRE) != 0) ==
			 (command->start.reserved[T2REPLAY_AUTOFIRE_OFFSET] != 0))
		);
	}
	if(command->mode == T2REPLAY_COMMAND_RECORD) {
		return (!(command->flags & T2REPLAY_COMMAND_FLAG_STAGE_SEEK) &&
			(command->reserved_0 == 0) && t2replay_bytes_zero(
			reinterpret_cast<const uint8_t far *>(&command->start),
			sizeof(command->start)
		));
	}
	return false;
}

#if T2REPLAY_PRACTICE_DIAGNOSTICS
static uint16_t t2replay_command_validation_mask(
	const t2replay_command_t far *command
)
{
	uint16_t mask = 0;
	unsigned i;
	bool reserved_zero = true;

	if(t2replay_command_magic_matches(command->magic)) { mask |= 0x0001; }
	if((command->mode == T2REPLAY_COMMAND_RECORD) ||
	   (command->mode == T2REPLAY_COMMAND_PLAYBACK) ||
	   (command->mode == T2REPLAY_COMMAND_PRACTICE)) { mask |= 0x0002; }
	if((command->mode == T2REPLAY_COMMAND_RECORD)
		? ((command->slot < T2REPLAY_SLOT_COUNT) ||
		   (command->slot == T2REPLAY_TEMP_SLOT))
		: (command->slot < T2REPLAY_SLOT_COUNT)) { mask |= 0x0004; }
	if((command->flags & ~T2REPLAY_COMMAND_KNOWN_FLAGS) == 0) { mask |= 0x0008; }
	if(((command->flags & T2REPLAY_COMMAND_FLAG_STAGE_SEEK) != 0)
		? ((command->reserved_0 >= 1) &&
		   (command->reserved_0 <= T2REPLAY_STAGE_COUNT))
		: (command->reserved_0 == 0)) { mask |= 0x0010; }
	for(i = 0; i < sizeof(command->reserved); i++) {
		if(command->reserved[i] != 0) { reserved_zero = false; }
	}
	if(reserved_zero) { mask |= 0x0020; }
	return mask;
}
#endif

#if T2REPLAY_PRACTICE_DIAGNOSTICS
static uint8_t t2replay_command_load(
	uint8_t far *slot, uint8_t far *flags, uint8_t far *seek_stage,
	t2replay_start_t far *start
)
{
	t2replay_command_t command;
	char handoff_fn[12];
	uint32_t size;
	int fd;

	// The primary command is authoritative. The second file exists only to
	// force the preceding directory update across the process boundary.
	t2replay_handoff_fn_set(handoff_fn);
	t2replay_dos_delete(handoff_fn);
	fd = t2replay_dos_open(t2replay_command_fn, T2REPLAY_DOS_ACCESS_READ);
	if(fd < 0) {
		t2practice_diag_main_command(T2PDR_MAIN_COMMAND_MISSING, 0);
		return T2RM_DISABLED;
	}
	t2practice_diag_lifecycle(T2PDLM_MAIN_COMMAND_OPENED, 0, 0, 0);
#if T2REPLAY_PRACTICE_DIAGNOSTICS
	{
		int read = t2replay_dos_read(fd, &command, sizeof(command));
		bool size_read = t2replay_dos_size(fd, &size);

		if((read != sizeof(command)) || !size_read) {
			t2replay_dos_close(fd);
			t2replay_dos_delete(t2replay_command_fn);
			t2practice_diag_main_command(T2PDR_MAIN_COMMAND_READ, 0);
			return T2RM_DISABLED;
		}
	}
	t2practice_diag_lifecycle(T2PDLM_MAIN_COMMAND_READ, 0, 0, 0);
#else
	if((t2replay_dos_read(fd, &command, sizeof(command)) != sizeof(command)) ||
		!t2replay_dos_size(fd, &size)) {
		t2replay_dos_close(fd);
		t2replay_dos_delete(t2replay_command_fn);
		return T2RM_DISABLED;
	}
#endif
	t2replay_dos_close(fd);
#if !T2REPLAY_PRACTICE_DIAGNOSTICS
	t2replay_dos_delete(t2replay_command_fn);
#endif
	t2practice_diag_lifecycle(
		T2PDLM_MAIN_COMMAND_CLOSED, static_cast<uint16_t>(size), 0, 0
	);
#if T2REPLAY_PRACTICE_DIAGNOSTICS
	if(size != sizeof(command)) {
		t2practice_diag_main_command(T2PDR_MAIN_COMMAND_SIZE, &command);
		return T2RM_DISABLED;
	}
	t2practice_diag_lifecycle(T2PDLM_MAIN_COMMAND_SIZE_VALID, 0, 0, 0);
	t2practice_diag_lifecycle(
		T2PDLM_MAIN_COMMAND_VALIDATION_MASK,
		t2replay_command_validation_mask(&command), 0, 0
	);
	t2practice_diag_lifecycle(T2PDLM_MAIN_START_VALID_BEGIN, 0, 0, 0);
	t2practice_diag_lifecycle(
		T2PDLM_MAIN_START_VALID_END,
		(t2replay_practice_start_valid(&command.start) ? 1 : 0), 0, 0
	);
	if(!t2replay_command_valid(&command)) {
		t2practice_diag_main_command(T2PDR_MAIN_COMMAND_INVALID, &command);
		return T2RM_DISABLED;
	}
	t2practice_diag_lifecycle(T2PDLM_MAIN_COMMAND_STRUCT_VALID, 0, 0, 0);
	t2practice_diag_lifecycle(T2PDLM_MAIN_COMMAND_DIAG_BEGIN, 0, 0, 0);
	t2practice_diag_main_command(T2PDR_NONE, &command);
	t2practice_diag_lifecycle(T2PDLM_MAIN_COMMAND_DIAG_END, 0, 0, 0);
	t2replay_dos_delete(t2replay_command_fn);
#else
	if((size != sizeof(command)) || !t2replay_command_valid(&command)) {
		return T2RM_DISABLED;
	}
#endif
	*slot = command.slot;
	*flags = command.flags;
	*seek_stage = command.reserved_0;
	*start = command.start;
	return command.mode;
}
#else
static uint8_t t2replay_command_load(
	uint8_t far *slot, uint8_t far *flags, uint8_t far *seek_stage,
	t2replay_start_t far *start
)
{
	t2replay_command_t command;
	char handoff_fn[12];
	uint32_t size;
	int fd;

	// The primary command is authoritative. The second file exists only to
	// force the preceding directory update across the process boundary.
	t2replay_handoff_fn_set(handoff_fn);
	t2replay_dos_delete(handoff_fn);
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
	*seek_stage = command.reserved_0;
	*start = command.start;
	return command.mode;
}
#endif

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
	t2replay_header.continues_final = static_cast<uint8_t>(
		resident->continues_used
	);
	t2replay_header.terminal_stage = static_cast<uint8_t>(stage_id);
}

static void t2replay_finalize(uint8_t end_reason)
{
	bool protect_blocked = false;

	t2replay_fast_forward_boundary_reset();
	if(t2replay_finished || (t2replay_mode == T2RM_DISABLED)) {
		return;
	}
	t2replay_finished = true;
	if(t2replay_mode == T2RM_RECORD) {
		if(!t2replay_guard_blocked()) {
			(void)t2replay_guard_checkpoint(T2SAE_FINALIZE);
		} else {
			t2replay_guard_observe(T2SAE_FINALIZE);
		}
		protect_blocked = t2replay_guard_blocked();
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
		if(!t2replay_failed && !t2replay_header_write(false)) {
			t2replay_failed = true;
		}
		if(!t2replay_failed && !t2replay_stage_seek_write()) {
			t2replay_failed = true;
		}
		if(
			!t2replay_failed && !protect_blocked &&
			!t2replay_save_request_write(end_reason)
		) {
			t2replay_failed = true;
		}
		if(t2replay_failed || protect_blocked) {
			t2replay_pending_files_delete();
		}
		t2replay_guard_end();
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
	uint8_t command_mode;
	uint8_t command_seek_stage;
	uint8_t selected_stage;
	t2replay_start_t command_start;
	t2replay_start_t seek_start;
	uint32_t seek_sample_anchor;
	uint32_t seek_packet_anchor;
	uint32_t seek_prefix_checksum;
	bool replay_header_valid;
#if T2REPLAY_EXACT_APPLY
	t2replay_exact_load_result_t exact_load;
#endif

	if(t2replay_mode != T2RM_DISABLED) {
		return;
	}
	t2practice_diag_lifecycle(T2PDLM_MAIN_REPLAY_ENTRY_BEGIN, 0, 0, 0);
	t2replay_paths_init();
	t2practice_diag_lifecycle(T2PDLM_MAIN_REPLAY_PATHS_READY, 0, 0, 0);
	command_mode = t2replay_command_load(
		&slot, &command_flags, &command_seek_stage, &command_start
	);
	if(command_mode == T2RM_DISABLED) {
		return;
	}
	t2practice_diag_lifecycle(T2PDLM_MAIN_REPLAY_COMMAND_READY, 0, 0, 0);
	if(command_mode == T2REPLAY_COMMAND_RECORD) {
		t2replay_temp_set();
	} else {
		t2replay_slot_set(slot);
	}
	t2replay_payload_checksum = T2REPLAY_FNV1A_BASIS;
	t2replay_buffer_len = 0;
	t2replay_buffer_pos = 0;
	t2replay_payload_written = 0;
	t2replay_packet_cursor = 0;
	t2replay_sample_cursor = 0;
	t2replay_protect_sample_count = 0;
	t2replay_pending_run = 0;
	t2replay_decode_run = 0;
	t2replay_pending_valid = false;
	t2replay_failed = false;
	t2replay_finished = false;
	t2replay_playback_exit = false;
	t2replay_save_prompted = false;
	t2replay_stage_seen = false;
	t2replay_practice_target = T2RPT_STAGE_START;
	t2replay_fast_forward_phase = 0;
	t2replay_fast_forward_slowdown_active = false;
	t2replay_timing_armed = false;
	t2replay_timing_pause_opened = false;
	t2replay_timing_target = 0;
	t2replay_autofire_active = false;
	t2replay_autofire_release_frame = false;
	t2replay_stage_seek_count = 0;
	t2replay_memclear(
		t2replay_stage_seek_entries, sizeof(t2replay_stage_seek_entries)
	);
	t2replay_memclear(
		t2replay_stage_seek_starts, sizeof(t2replay_stage_seek_starts)
	);
#if T2REPLAY_EXACT_APPLY
	t2replay_exact_pending = false;
	t2replay_exact_active = false;
	t2replay_exact_anchor_sample_pending = false;
	t2replay_exact_first_sample_pending = false;
	t2replay_exact_envelope_free();
	t2replay_memclear(&t2replay_exact_request, sizeof(t2replay_exact_request));
	t2replay_memclear(&t2replay_exact_diag, sizeof(t2replay_exact_diag));
	t2replay_memclear(
		&t2xobs_req,
		sizeof(t2xobs_req)
	);
	t2replay_memclear(
		&t2xobs_out,
		sizeof(t2xobs_out)
	);
	t2xobs_active = false;
	t2xobs_direct = false;
#endif
	if(command_mode == T2REPLAY_COMMAND_PRACTICE) {
		t2replay_temp_set();
		t2replay_pending_files_delete();
		t2replay_start_apply(&command_start);
		t2replay_practice_target = command_start.reserved[
			T2REPLAY_PRACTICE_TARGET_OFFSET
		];
		t2practice_diag_lifecycle(
			T2PDLM_MAIN_PRACTICE_START_APPLIED, 0, 0, 0
		);
		return;
	}
	if(command_mode == T2RM_RECORD) {
		bool guard_admitted;

		t2replay_mode = T2RM_RECORD;
		t2replay_autofire_active = (
			(command_flags & T2REPLAY_COMMAND_FLAG_AUTOFIRE) != 0
		);
		t2replay_pending_files_delete();
		guard_admitted = t2replay_guard_begin();
		t2practice_diag_lifecycle(
			T2PDLM_REPLAY_GUARD_ADMITTED,
			(guard_admitted ? 1 : 0), 0,
			t2replay_game_heap_available_paras()
		);
		t2replay_header_capture();
		if(command_flags & T2REPLAY_COMMAND_FLAG_PRACTICE) {
			t2replay_header.flags |= T2REPLAY_FLAG_PRACTICE;
			t2replay_header.start = command_start;
			t2replay_header_apply();
			t2replay_practice_target = command_start.reserved[
				T2REPLAY_PRACTICE_TARGET_OFFSET
			];
		}
		if(!t2replay_start_valid(&t2replay_header.start) ||
			!t2replay_header_write(true)) {
			t2replay_pending_files_delete();
			t2replay_guard_end();
			t2replay_mode = T2RM_DISABLED;
		}
	} else {
		replay_header_valid = t2replay_header_read();
		if(!replay_header_valid) {
#if T2REPLAY_EXACT_APPLY
			exact_load = t2replay_public_seek_request_load(slot, false);
			if(exact_load == T2XLR_ABSENT) {
				exact_load = t2replay_exact_request_load(slot, false);
			}
			if(exact_load == T2XLR_ABSENT) {
				(void)t2xobs_req_load(slot, false);
			} else if(exact_load == T2XLR_REJECTED) {
				t2replay_exact_diag.state = T2XDS_REJECTED;
				t2replay_exact_diag.reject = 0xFF;
				t2replay_exact_diag_flush();
			}
#endif
			t2replay_mode = T2RM_PLAYBACK;
			t2replay_fail();
			return;
		}
		t2replay_mode = T2RM_PLAYBACK;
		if(command_flags & T2REPLAY_COMMAND_FLAG_STAGE_SEEK) {
			selected_stage = static_cast<uint8_t>(command_seek_stage - 1);
			if((selected_stage <= static_cast<uint8_t>(
				t2replay_header.start.stage
			)) || (selected_stage > t2replay_header.stage_reached) ||
				!t2replay_stage_seek_load(
					slot, selected_stage, &seek_start,
					&seek_sample_anchor, &seek_packet_anchor,
					&seek_prefix_checksum
				)) {
				t2replay_fail();
				return;
			}
			t2replay_start_apply(&seek_start);
			t2replay_practice_target = T2RPT_STAGE_START;
			t2replay_sample_cursor = seek_sample_anchor;
			t2replay_packet_cursor = seek_packet_anchor;
			t2replay_payload_checksum = seek_prefix_checksum;
			t2replay_buffer_len = 0;
			t2replay_buffer_pos = 0;
			t2replay_decode_run = 0;
		} else {
			t2replay_payload_checksum = T2REPLAY_FNV1A_BASIS;
			t2replay_header_apply();
			t2replay_practice_target = t2replay_header.start.reserved[
				T2REPLAY_PRACTICE_TARGET_OFFSET
			];
#if T2REPLAY_EXACT_APPLY
			exact_load = t2replay_public_seek_request_load(slot, true);
			if(exact_load == T2XLR_ABSENT) {
				exact_load = t2replay_exact_request_load(slot, true);
			}
			if(exact_load == T2XLR_READY) {
				resident->stage = 4;
				resident->rank = t2replay_exact_envelope[T2REC_HEADER_RANK];
				resident->shottype =
					t2replay_exact_envelope[T2REC_HEADER_SHOTTYPE];
				resident->reduce_effects =
					(t2replay_exact_envelope[T2REC_HEADER_REDUCE_EFFECTS] != 0);
				stage_id = 4;
				rank = resident->rank;
			} else if(exact_load == T2XLR_REJECTED) {
				// The private loader intentionally keeps graphics masked. The normal
				// MAIN-to-OP return and OP's process initialization own visibility.
				t2replay_exact_diag.state = T2XDS_REJECTED;
				t2replay_exact_diag.reject = 0xFF;
				t2replay_exact_diag_flush();
				t2replay_fail();
			} else {
				(void)t2xobs_req_load(slot, true);
			}
#endif
		}
	}
}

static void near t2replay_boss_promote_clean(char *bgm_fn)
{
	// Mirrors native post-scroll promotion after a clean actor initializer.
	stage_progression = SP_BOSS;
	midboss_active = false;
	enemies_remove_all();
	enemies_callbacks_null();
	boss_activate_if_scroll_done_func = nullfunc_void;
	boss_bg_render = boss_bg_render_func;
	boss_update = boss_update_func;
	stage_should_end_func = stage_should_end;
	scroll_cycle = -1;

	stage_frame = 160;
	stage_title_unput();
	stage_frame = 0;
	boss_bgm_load(bgm_fn);
	bgm_show_timer = 1;
	bgm_title_id = boss_bgm_title_id;
}

static bool16 near t2replay_stage1_rika_activate_clean(
	th02_s1_rika_clean_target_t target
)
{
#if T2REPLAY_PRACTICE_DIAGNOSTICS
	bool16 constructor_result = th02_s1_rika_clean_init(target);

	t2practice_diag_constructor_result(constructor_result);
	if(!constructor_result) {
		return false;
	}
#else
	if(!th02_s1_rika_clean_init(target)) {
		return false;
	}
#endif
	t2replay_boss_promote_clean(rika_bgm_fn);
	return true;
}

static bool16 near t2replay_stage2_meira_activate_clean(
	th02_s2_meira_clean_target_t target
)
{
#if T2REPLAY_PRACTICE_DIAGNOSTICS
	bool16 constructor_result = th02_s2_meira_clean_init(target);

	t2practice_diag_constructor_result(constructor_result);
	if(!constructor_result) {
		return false;
	}
#else
	if(!th02_s2_meira_clean_init(target)) {
		return false;
	}
#endif
	t2replay_boss_promote_clean(aBoss4_m);
	return true;
}

static bool16 near t2replay_stage3_stones_activate_clean(
	th02_s3_stones_clean_target_t target
)
{
	if(stage_id != 2) {
		return false;
	}
	switch(target) {
	case T2S3_STONES_BOSS_START:
	case T2S3_STONES_INNER_PAIR:
	case T2S3_STONES_OUTER_PAIR:
		break;
	default:
		return false;
	}
	if(!practice_terminal_field_build()) {
		return false;
	}
	th02_s3_field_clean_init();
	Palettes[0].v[0] = 0;
	Palettes[0].v[1] = 0;
	Palettes[0].v[2] = 0;
	palette_show();
	if(!th02_s3_stones_clean_init(target)) {
		t2practice_diag_constructor_result(false);
		return false;
	}
	t2practice_diag_constructor_result(true);
	t2replay_boss_promote_clean(aBoss2_m);
	return true;
}

static void near t2replay_stage3_stones_pools_clean(void)
{
	bullets_clear();
	lasers_reset();
	bg_particles_reset();
	shots_free_all();
}

static bool16 near t2replay_stage3_north_phase4_activate_clean(void)
{
	if(stage_id != 2) {
		return false;
	}
	if(!practice_terminal_field_build()) {
		return false;
	}
	th02_s3_field_clean_init();
	Palettes[0].v[0] = 0;
	Palettes[0].v[1] = 0;
	Palettes[0].v[2] = 0;
	palette_show();
	t2replay_stage3_stones_pools_clean();
	if(!th02_s3_stones_north_phase4_clean_init()) {
		t2practice_diag_constructor_result(false);
		return false;
	}
	t2practice_diag_constructor_result(true);
	t2replay_boss_promote_clean(aBoss2_m);
	return true;
}

static bool16 near t2replay_stage4_midboss_activate_clean(
	th02_s4_midboss_clean_target_t target, int target_scroll_step
)
{
#if T2REPLAY_PRACTICE_DIAGNOSTICS
	bool16 constructor_result;

	if(!practice_chapter_field_build(target_scroll_step)) {
		return false;
	}
	constructor_result = th02_s4_midboss_clean_init(target);
	t2practice_diag_constructor_result(constructor_result);
	if(!constructor_result) {
		return false;
	}
#else
	if(
		!practice_chapter_field_build(target_scroll_step) ||
		!th02_s4_midboss_clean_init(target)
	) {
		return false;
	}
#endif
	midboss_active = true;
	return true;
}

static void near t2replay_boss_scroll_reset_clean(void)
{
	scroll_line = 0;
	scroll_sad = 0;
	replay_scroll_page_line_set(0, 0);
	replay_scroll_page_line_set(1, 0);
	graph_scrollup(0);
}

static void near t2replay_later_boss_phase_pools_clean(void)
{
	bullets_clear();
	lasers_reset();
	bg_particles_reset();
	shots_free_all();
}

static bool16 near t2replay_stage4_marisa_activate_clean(void)
{
	if(!practice_terminal_field_build()) {
		return false;
	}
	t2replay_boss_scroll_reset_clean();
	super_clean(128, 511);
	super_patnum = 128;
	super_entry_bfnt(aStage3_b_bft);
	super_entry_bfnt(aStage3_b_btt_0);
	tile_mode = TM_NONE;
	shots_free_all();
	th02_s4_marisa_clean_init();
	t2practice_diag_constructor_result(true);
	t2replay_boss_promote_clean(aBoss3_m);
	return true;
}

static bool16 near t2replay_stage4_marisa_phase1_activate_clean(void)
{
	if(!practice_terminal_field_build()) {
		return false;
	}
	t2replay_boss_scroll_reset_clean();
	super_clean(128, 511);
	super_patnum = 128;
	super_entry_bfnt(aStage3_b_bft);
	super_entry_bfnt(aStage3_b_btt_0);
	tile_mode = TM_NONE;
	t2replay_later_boss_phase_pools_clean();
	palette_settone(100);
	if(!th02_later_boss_clean_init(T2LBPT_MARISA_PHASE1)) {
		t2practice_diag_constructor_result(false);
		return false;
	}
	t2practice_diag_constructor_result(true);
	t2replay_boss_promote_clean(aBoss3_m);
	return true;
}

static bool16 near t2replay_stage4_marisa_round_activate_clean(
	th02_later_boss_target_t target
)
{
	switch(target) {
	case T2LBPT_MARISA_ROUND2:
	case T2LBPT_MARISA_ROUND3:
	case T2LBPT_MARISA_ROUND4:
	case T2LBPT_MARISA_ROUND5:
	case T2LBPT_MARISA_ROUND6:
	case T2LBPT_MARISA_ROUND7:
		break;
	default:
		return false;
	}
	if(!practice_terminal_field_build()) {
		return false;
	}
	t2replay_boss_scroll_reset_clean();
	super_clean(128, 511);
	super_patnum = 128;
	super_entry_bfnt(aStage3_b_bft);
	super_entry_bfnt(aStage3_b_btt_0);
	tile_mode = TM_NONE;
	t2replay_later_boss_phase_pools_clean();
	palette_settone(100);
	if(!th02_later_boss_clean_init(target)) {
		t2practice_diag_constructor_result(false);
		return false;
	}
	t2practice_diag_constructor_result(true);
	t2replay_boss_promote_clean(aBoss3_m);
	return true;
}

static bool16 near t2replay_stage5_mima_activate_clean(void)
{
	if(!practice_terminal_field_build()) {
		return false;
	}
	t2replay_boss_scroll_reset_clean();
	super_clean(128, 192);
	super_patnum = 128;
	super_entry_bfnt(mima1_bft);
	super_entry_bfnt(aStage3_b_btt);
	tile_mode = TM_NONE;
	shots_free_all();
	if(!th02_s5_mima_clean_init(T2S5_MIMA_BOSS_START)) {
		t2practice_diag_constructor_result(false);
		return false;
	}
	t2practice_diag_constructor_result(true);
	graph_accesspage(page_front);
	graph_clear();
	graph_accesspage(page_back);
	graph_clear();
	grcg_setcolor(GC_RMW, 11);
	grc_setclip(PLAYFIELD_RIGHT, 0, (RES_X - 1), (RES_Y - 1));
	graph_accesspage(page_front);
	grcg_fill();
	graph_accesspage(page_back);
	grcg_fill();
	grcg_off();
	grc_setclip(PLAYFIELD_LEFT, 0, PLAYFIELD_RIGHT, (RES_Y - 1));
	palette_settone(100);
	t2replay_boss_promote_clean(aMima_m);
	return true;
}

static bool16 near t2replay_stage5_mima_phase1_activate_clean(void)
{
	if(!practice_terminal_field_build()) {
		return false;
	}
	t2replay_boss_scroll_reset_clean();
	super_clean(128, 192);
	super_patnum = 128;
	super_entry_bfnt(mima1_bft);
	super_entry_bfnt(aStage3_b_btt);
	tile_mode = TM_NONE;
	t2replay_later_boss_phase_pools_clean();
	palette_settone(100);
	if(!th02_later_boss_clean_init(T2LBPT_MIMA_PHASE1)) {
		t2practice_diag_constructor_result(false);
		return false;
	}
	t2practice_diag_constructor_result(true);
	t2replay_boss_promote_clean(aMima_m);
	return true;
}

static bool16 near t2replay_stage5_mima_phase3_activate_clean(void)
{
	if(!practice_terminal_field_build()) {
		return false;
	}
	t2replay_boss_scroll_reset_clean();
	super_clean(128, 192);
	super_patnum = 128;
	super_entry_bfnt(mima1_bft);
	super_entry_bfnt(aStage3_b_btt);
	tile_mode = TM_NONE;
	t2replay_later_boss_phase_pools_clean();
	palette_settone(100);
	if(!th02_later_boss_clean_init(T2LBPT_MIMA_PHASE3)) {
		t2practice_diag_constructor_result(false);
		return false;
	}
	t2practice_diag_constructor_result(true);
	t2replay_boss_promote_clean(aMima_m);
	return true;
}

static bool16 near t2replay_stage5_mima_phase5_activate_clean(void)
{
	if(!practice_terminal_field_build()) {
		return false;
	}
	t2replay_boss_scroll_reset_clean();
	super_clean(128, 192);
	super_patnum = 128;
	super_entry_bfnt(mima1_bft);
	super_entry_bfnt(aStage3_b_btt);
	tile_mode = TM_NONE;
	t2replay_later_boss_phase_pools_clean();
	palette_settone(100);
	if(!th02_later_boss_clean_init(T2LBPT_MIMA_PHASE5)) {
		t2practice_diag_constructor_result(false);
		return false;
	}
	t2practice_diag_constructor_result(true);
	t2replay_boss_promote_clean(aMima_m);
	return true;
}

static bool16 near t2replay_stage5_mima_phase7_activate_clean(void)
{
	if(!practice_terminal_field_build()) {
		return false;
	}
	t2replay_boss_scroll_reset_clean();
	super_clean(128, 192);
	super_patnum = 128;
	super_entry_bfnt(mima1_bft);
	super_entry_bfnt(aStage3_b_btt);
	tile_mode = TM_NONE;
	t2replay_later_boss_phase_pools_clean();
	palette_settone(100);
	if(!th02_later_boss_clean_init(T2LBPT_MIMA_PHASE7)) {
		t2practice_diag_constructor_result(false);
		return false;
	}
	t2practice_diag_constructor_result(true);
	t2replay_boss_promote_clean(aMima_m);
	return true;
}

static bool16 near t2replay_stage5_mima_phase9_activate_clean(void)
{
	// Mima's native form handoff skips Phase 9 after a continue. A direct
	// Practice entry must preserve that rule rather than manufacture the
	// winged form from an impossible continued-run state.
	if(resident->continues_used != 0) {
		t2practice_diag_constructor_result(false);
		return false;
	}
	if(!practice_terminal_field_build()) {
		return false;
	}
	t2replay_boss_scroll_reset_clean();
	tile_mode = TM_NONE;
	t2replay_later_boss_phase_pools_clean();
	palette_settone(100);
	if(!th02_later_boss_clean_init(T2LBPT_MIMA_PHASE9)) {
		t2practice_diag_constructor_result(false);
		return false;
	}
	t2practice_diag_constructor_result(true);
	t2replay_boss_promote_clean(aMima_m);
	return true;
}

static bool16 near t2replay_extra_sigma_activate_clean(void)
{
	int page;

	if(!practice_terminal_field_build()) {
		return false;
	}
	t2replay_boss_scroll_reset_clean();
	super_clean(128, 192);
	super_patnum = 128;
	super_entry_bfnt(stage5b1_bft);
	super_entry_bfnt(stage5b2_bft);
	tile_mode = TM_NONE;
	shots_free_all();
	th02_s6_sigma_clean_init();
	t2practice_diag_constructor_result(true);
	grc_setclip(PLAYFIELD_LEFT, 0, PLAYFIELD_RIGHT, (RES_Y - 1));
	for(page = 0; page < PAGE_COUNT; page++) {
		graph_accesspage(page);
		super_put_rect(sigma_topleft.x, sigma_topleft.y, 128);
		super_put_rect((sigma_topleft.x + 64), sigma_topleft.y, 129);
	}
	graph_accesspage(page_back);
	palette_settone(100);
	t2replay_boss_promote_clean(aBoss5_m);
	return true;
}

static bool16 near t2replay_extra_sigma_phase1_activate_clean(void)
{
	if(!practice_terminal_field_build()) {
		return false;
	}
	t2replay_boss_scroll_reset_clean();
	super_clean(128, 192);
	super_patnum = 128;
	super_entry_bfnt(stage5b1_bft);
	super_entry_bfnt(stage5b2_bft);
	tile_mode = TM_NONE;
	t2replay_later_boss_phase_pools_clean();
	palette_settone(100);
	if(!th02_later_boss_clean_init(T2LBPT_SIGMA_PHASE1)) {
		t2practice_diag_constructor_result(false);
		return false;
	}
	t2practice_diag_constructor_result(true);
	t2replay_boss_promote_clean(aBoss5_m);
	return true;
}

static bool16 near t2replay_extra_sigma_phase3_activate_clean(void)
{
	if(!practice_terminal_field_build()) {
		return false;
	}
	t2replay_boss_scroll_reset_clean();
	super_clean(128, 192);
	super_patnum = 128;
	super_entry_bfnt(stage5b1_bft);
	super_entry_bfnt(stage5b2_bft);
	tile_mode = TM_NONE;
	t2replay_later_boss_phase_pools_clean();
	palette_settone(100);
	if(!th02_later_boss_clean_init(T2LBPT_SIGMA_PHASE3)) {
		t2practice_diag_constructor_result(false);
		return false;
	}
	t2practice_diag_constructor_result(true);
	t2replay_boss_promote_clean(aBoss5_m);
	return true;
}

static bool16 near t2replay_extra_sigma_phase5_activate_clean(void)
{
	if(!practice_terminal_field_build()) {
		return false;
	}
	t2replay_boss_scroll_reset_clean();
	super_clean(128, 192);
	super_patnum = 128;
	super_entry_bfnt(stage5b1_bft);
	super_entry_bfnt(stage5b2_bft);
	tile_mode = TM_NONE;
	t2replay_later_boss_phase_pools_clean();
	palette_settone(100);
	if(!th02_later_boss_clean_init(T2LBPT_SIGMA_PHASE5)) {
		t2practice_diag_constructor_result(false);
		return false;
	}
	t2practice_diag_constructor_result(true);
	t2replay_boss_promote_clean(aBoss5_m);
	return true;
}

static bool16 near t2replay_extra_sigma_phase7_activate_clean(void)
{
	if(!practice_terminal_field_build()) {
		return false;
	}
	t2replay_boss_scroll_reset_clean();
	super_clean(128, 192);
	super_patnum = 128;
	super_entry_bfnt(stage5b1_bft);
	super_entry_bfnt(stage5b2_bft);
	tile_mode = TM_NONE;
	t2replay_later_boss_phase_pools_clean();
	palette_settone(100);
	if(!th02_later_boss_clean_init(T2LBPT_SIGMA_PHASE7)) {
		t2practice_diag_constructor_result(false);
		return false;
	}
	t2practice_diag_constructor_result(true);
	t2replay_boss_promote_clean(aBoss5_m);
	return true;
}

static bool16 near t2replay_extra_sigma_phase9_activate_clean(void)
{
	if(!practice_terminal_field_build()) {
		return false;
	}
	t2replay_boss_scroll_reset_clean();
	super_clean(128, 192);
	super_patnum = 128;
	super_entry_bfnt(stage5b1_bft);
	super_entry_bfnt(stage5b2_bft);
	tile_mode = TM_NONE;
	t2replay_later_boss_phase_pools_clean();
	palette_settone(100);
	if(!th02_later_boss_clean_init(T2LBPT_SIGMA_PHASE9)) {
		t2practice_diag_constructor_result(false);
		return false;
	}
	t2practice_diag_constructor_result(true);
	t2replay_boss_promote_clean(aBoss5_m);
	return true;
}

// Turbo C++'s dense switch table for these patch-tail IDs is not stable after
// this executable's native segments. Keep the public IDs explicit so every
// Practice target follows ordinary comparisons and a directly visible owner.
static bool16 near t2practice_target_apply_explicit(uint8_t target)
{
	int target_scroll_step = -1;
	th02_s1_rika_clean_target_t rika_target;
	th02_s2_meira_clean_target_t meira_target;
	th02_s3_stones_clean_target_t stones_target;

	if(target == T2RPT_STAGE_START) {
		return true;
	}
	if(target == T2RPT_STAGE1_CHAPTER2) {
		if(stage_id != 0) { t2practice_diag_failure(T2PDR_STAGE_MISMATCH); return false; }
		target_scroll_step = 186;
	} else if(target == T2RPT_STAGE1_MIDBOSS) {
		if(stage_id != 0) { t2practice_diag_failure(T2PDR_STAGE_MISMATCH); return false; }
		target_scroll_step = 116;
	} else if(
		(target >= T2RPT_STAGE1_BOSS_PHASE1) &&
		(target <= T2RPT_STAGE1_BOSS_PHASE3)
	) {
		if(stage_id != 0) { t2practice_diag_failure(T2PDR_STAGE_MISMATCH); return false; }
		rika_target = static_cast<th02_s1_rika_clean_target_t>(
			target - T2RPT_STAGE1_BOSS_PHASE1
		);
		if(!practice_terminal_field_build() ||
		   !t2replay_stage1_rika_activate_clean(rika_target)) { return false; }
	} else if(target == T2RPT_STAGE2_CHAPTER2) {
		if(stage_id != 1) { t2practice_diag_failure(T2PDR_STAGE_MISMATCH); return false; }
		target_scroll_step = 135;
	} else if(target == T2RPT_STAGE2_MIDBOSS) {
		if(stage_id != 1) { t2practice_diag_failure(T2PDR_STAGE_MISMATCH); return false; }
		target_scroll_step = 80;
	} else if(
		(target >= T2RPT_STAGE2_BOSS_PHASE1) &&
		(target <= T2RPT_STAGE2_BOSS_PHASE3)
	) {
		if(stage_id != 1) { t2practice_diag_failure(T2PDR_STAGE_MISMATCH); return false; }
		meira_target = static_cast<th02_s2_meira_clean_target_t>(
			target - T2RPT_STAGE2_BOSS_PHASE1
		);
		if(!practice_terminal_field_build() ||
		   !t2replay_stage2_meira_activate_clean(meira_target)) { return false; }
	} else if(target == T2RPT_STAGE3_CHAPTER2) {
		if(stage_id != 2) { t2practice_diag_failure(T2PDR_STAGE_MISMATCH); return false; }
		th02_s3_field_clean_init();
		target_scroll_step = 151;
	} else if(target == T2RPT_STAGE3_MIDBOSS) {
		if(stage_id != 2) { t2practice_diag_failure(T2PDR_STAGE_MISMATCH); return false; }
		th02_s3_field_clean_init();
		target_scroll_step = 103;
	} else if(
		(target == T2RPT_STAGE3_BOSS_START) ||
		(target == T2RPT_STAGE3_INNER_PAIR) ||
		(target == T2RPT_STAGE3_OUTER_PAIR)
	) {
		if(stage_id != 2) { t2practice_diag_failure(T2PDR_STAGE_MISMATCH); return false; }
		stones_target = static_cast<th02_s3_stones_clean_target_t>(
			(target == T2RPT_STAGE3_BOSS_START)
				? T2S3_STONES_BOSS_START
				: ((target == T2RPT_STAGE3_INNER_PAIR)
					? T2S3_STONES_INNER_PAIR : T2S3_STONES_OUTER_PAIR)
		);
		if(!t2replay_stage3_stones_activate_clean(stones_target)) { return false; }
	} else if(target == T2RPT_STAGE3_NORTH_PHASE4) {
		if((stage_id != 2) || !t2replay_stage3_north_phase4_activate_clean()) {
			if(stage_id != 2) { t2practice_diag_failure(T2PDR_STAGE_MISMATCH); }
			return false;
		}
	} else if(target == T2RPT_STAGE4_CHAPTER2) {
		if(stage_id != 3) { t2practice_diag_failure(T2PDR_STAGE_MISMATCH); return false; }
		target_scroll_step = 1327;
	} else if(target == T2RPT_STAGE4_CHAPTER3) {
		if(stage_id != 3) { t2practice_diag_failure(T2PDR_STAGE_MISMATCH); return false; }
		target_scroll_step = 2008;
	} else if(target == T2RPT_STAGE4_MIDBOSS_FIRST) {
		if((stage_id != 3) ||
		   !t2replay_stage4_midboss_activate_clean(T2S4_MIDBOSS_FIRST, 944)) {
			if(stage_id != 3) { t2practice_diag_failure(T2PDR_STAGE_MISMATCH); }
			return false;
		}
	} else if(target == T2RPT_STAGE4_MIDBOSS_SECOND) {
		if((stage_id != 3) ||
		   !t2replay_stage4_midboss_activate_clean(T2S4_MIDBOSS_SECOND, 1632)) {
			if(stage_id != 3) { t2practice_diag_failure(T2PDR_STAGE_MISMATCH); }
			return false;
		}
	} else if(target == T2RPT_STAGE4_BOSS_START) {
		if((stage_id != 3) || !t2replay_stage4_marisa_activate_clean()) {
			if(stage_id != 3) { t2practice_diag_failure(T2PDR_STAGE_MISMATCH); }
			return false;
		}
	} else if(target == T2RPT_STAGE4_BOSS_PHASE1) {
		if((stage_id != 3) || !t2replay_stage4_marisa_phase1_activate_clean()) {
			if(stage_id != 3) { t2practice_diag_failure(T2PDR_STAGE_MISMATCH); }
			return false;
		}
	} else if(target == T2RPT_STAGE4_BOSS_ROUND2) {
		if((stage_id != 3) ||
		   !t2replay_stage4_marisa_round_activate_clean(T2LBPT_MARISA_ROUND2)) {
			if(stage_id != 3) { t2practice_diag_failure(T2PDR_STAGE_MISMATCH); }
			return false;
		}
	} else if(target == T2RPT_STAGE4_BOSS_ROUND3) {
		if((stage_id != 3) ||
		   !t2replay_stage4_marisa_round_activate_clean(T2LBPT_MARISA_ROUND3)) {
			if(stage_id != 3) { t2practice_diag_failure(T2PDR_STAGE_MISMATCH); }
			return false;
		}
	} else if(
		(target >= T2RPT_STAGE4_BOSS_ROUND4) &&
		(target <= T2RPT_STAGE4_BOSS_ROUND7)
	) {
		if((stage_id != 3) || !t2replay_stage4_marisa_round_activate_clean(
			static_cast<th02_later_boss_target_t>(
				T2LBPT_MARISA_ROUND4 + (target - T2RPT_STAGE4_BOSS_ROUND4)
			)
		)) {
			if(stage_id != 3) { t2practice_diag_failure(T2PDR_STAGE_MISMATCH); }
			return false;
		}
	} else if(target == T2RPT_STAGE5_BOSS_START) {
		if((stage_id != 4) || !t2replay_stage5_mima_activate_clean()) { return false; }
	} else if(target == T2RPT_STAGE5_BOSS_PHASE1) {
		if((stage_id != 4) || !t2replay_stage5_mima_phase1_activate_clean()) { return false; }
	} else if(target == T2RPT_STAGE5_BOSS_PHASE3) {
		if((stage_id != 4) || !t2replay_stage5_mima_phase3_activate_clean()) { return false; }
	} else if(target == T2RPT_STAGE5_BOSS_PHASE5) {
		if((stage_id != 4) || !t2replay_stage5_mima_phase5_activate_clean()) { return false; }
	} else if(target == T2RPT_STAGE5_BOSS_PHASE7) {
		if((stage_id != 4) || !t2replay_stage5_mima_phase7_activate_clean()) { return false; }
	} else if(target == T2RPT_STAGE5_BOSS_PHASE9) {
		if((stage_id != 4) || !t2replay_stage5_mima_phase9_activate_clean()) { return false; }
	} else if(target == T2RPT_EXTRA_CHAPTER2) {
		if(stage_id != 5) { t2practice_diag_failure(T2PDR_STAGE_MISMATCH); return false; }
		target_scroll_step = 239;
	} else if(target == T2RPT_EXTRA_MIDBOSS) {
		if(stage_id != 5) { t2practice_diag_failure(T2PDR_STAGE_MISMATCH); return false; }
		target_scroll_step = 200;
	} else if(target == T2RPT_EXTRA_BOSS_START) {
		if((stage_id != 5) || !t2replay_extra_sigma_activate_clean()) { return false; }
	} else if(target == T2RPT_EXTRA_BOSS_PHASE1) {
		if((stage_id != 5) || !t2replay_extra_sigma_phase1_activate_clean()) { return false; }
	} else if(target == T2RPT_EXTRA_BOSS_PHASE3) {
		if((stage_id != 5) || !t2replay_extra_sigma_phase3_activate_clean()) { return false; }
	} else if(target == T2RPT_EXTRA_BOSS_PHASE5) {
		if((stage_id != 5) || !t2replay_extra_sigma_phase5_activate_clean()) { return false; }
	} else if(target == T2RPT_EXTRA_BOSS_PHASE7) {
		if((stage_id != 5) || !t2replay_extra_sigma_phase7_activate_clean()) { return false; }
	} else if(target == T2RPT_EXTRA_BOSS_PHASE9) {
		if((stage_id != 5) || !t2replay_extra_sigma_phase9_activate_clean()) { return false; }
	} else {
		t2practice_diag_failure(T2PDR_TARGET_UNKNOWN);
		return false;
	}

	if(target_scroll_step >= 0) {
		if(!practice_chapter_field_build(target_scroll_step)) { return false; }
		if((target == T2RPT_STAGE4_CHAPTER2) ||
		   (target == T2RPT_STAGE4_CHAPTER3)) {
			midboss_scroll_step = 1632;
		}
	}
	t2replay_practice_target = T2RPT_STAGE_START;
	return true;
}

#if T2REPLAY_PRACTICE_DIAGNOSTICS
static bool16 near t2practice_target_finish(uint8_t target, bool16 result)
{
	if(
		result && (target != T2RPT_STAGE_START) &&
		(stage_title_unput_func == stage_title_unput)
	) {
		stage_frame = 160;
		stage_title_unput();
		stage_frame = 0;
	}
	if(result) {
		t2practice_diag_lifecycle(
			T2PDLM_PRACTICE_TARGET_APPLIED, 0, 0,
			t2replay_game_heap_available_paras()
		);
	}
	t2practice_diag_apply_end(result);
	return result;
}

bool16 replay_practice_target_apply(void)
{
	uint8_t target = t2replay_practice_target;

	t2practice_diag_apply_begin(stage_id, target, map_length, spawn_rows);
	return t2practice_target_finish(
		target, t2practice_target_apply_explicit(target)
	);
}
#else
bool16 replay_practice_target_apply(void)
{
	uint8_t target = t2replay_practice_target;
	bool16 result = t2practice_target_apply_explicit(target);

	if(
		result && (target != T2RPT_STAGE_START) &&
		(stage_title_unput_func == stage_title_unput)
	) {
		stage_frame = 160;
		stage_title_unput();
		stage_frame = 0;
	}
	return result;
}
#endif

void replay_stage_start(void)
{
	t2replay_fast_forward_boundary_reset();
	replay_rank_lock_apply();
#ifdef T2SGA
	t2debug_coords_reset();
#endif
	if(t2replay_mode == T2RM_DISABLED) {
		return;
	}
#if T2REPLAY_EXACT_APPLY
	if(t2replay_exact_pending) {
		// The request's prefix validator already consumed and authenticated the
		// Stage 5 control packet. The anchored gameplay packet is next.
		return;
	}
#endif
	if(t2replay_mode == T2RM_RECORD) {
		if(t2replay_stage_seen && (t2replay_last_stage < T2REPLAY_STAGE_COUNT)) {
			t2replay_header.stage_scores[t2replay_last_stage] =
				static_cast<uint32_t>(score);
		}
		t2replay_header.stage_reached = static_cast<uint8_t>(stage_id);
		if(!t2replay_stage_seek_capture() || !t2replay_record_control(
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
	if(
		(t2replay_mode == T2RM_PLAYBACK) && !t2replay_failed &&
		!t2replay_finished && !quit
	) {
		t2replay_indicator_put();
	}
}

void replay_rank_lock_apply(void)
{
	if(t2replay_rank_lock_active) {
		playperf = t2replay_rank_lock_value;
	}
}

static void t2replay_autofire_apply(uint8_t phase)
{
	if(phase != T2REPLAY_PHASE_GAMEPLAY) {
		return;
	}
	if(!t2replay_autofire_active || !(key_det & INPUT_SHOT)) {
		t2replay_autofire_release_frame = false;
		return;
	}
	if(t2replay_autofire_release_frame) {
		key_det &= ~INPUT_SHOT;
	}
	t2replay_autofire_release_frame = !t2replay_autofire_release_frame;
}

void replay_input_sample(uint8_t phase)
{
	input_t host_input;
#if T2REPLAY_EXACT_APPLY
	uint32_t sample_before;
#endif
	replay_rank_lock_apply();

	t2replay_fast_forward_restore();
#ifdef T2SGA
	if(phase == T2REPLAY_PHASE_GAMEPLAY) {
		t2debug_coords_put();
	}
#endif
	if(t2replay_mode == T2RM_DISABLED) {
		return;
	}
	host_input = key_det;
#if T2REPLAY_EXACT_APPLY
	sample_before = t2replay_sample_cursor;
	if(
		t2xobs_active &&
		(phase == T2REPLAY_PHASE_GAMEPLAY) &&
		(sample_before == t2xobs_req.sample_anchor)
	) {
		t2xobs_semantic_record(
			t2xobs_direct
				? T2XOBS_DIR_PRE
				: T2XOBS_REF_PRE,
			phase
		);
	} else if(
		t2xobs_active &&
		(phase == T2REPLAY_PHASE_GAMEPLAY) &&
		(sample_before ==
		 (t2xobs_req.sample_anchor + 1))
	) {
		t2xobs_semantic_record(
			t2xobs_direct
				? T2XOBS_DIR_NEXT
				: T2XOBS_REF_NEXT,
			phase
		);
	}
#endif
	if(t2replay_mode == T2RM_RECORD) {
		if(!t2replay_failed && !t2replay_record_sample(phase)) {
			t2replay_failed = true;
		}
		t2replay_autofire_apply(phase);
		if(phase == T2REPLAY_PHASE_GAMEPLAY) {
			t2replay_timing_gameplay_sample(host_input);
		}
	} else {
		if(!t2replay_playback_sample(phase)) {
			t2replay_fail();
			return;
		}
		t2replay_autofire_apply(phase);
#if T2REPLAY_EXACT_APPLY
		if(t2replay_exact_first_sample_pending) {
			t2replay_exact_first_sample_pending = false;
			if(
				(phase != T2REPLAY_PHASE_GAMEPLAY) ||
				(sample_before != t2replay_exact_request.sample_anchor) ||
				(t2replay_sample_cursor != (sample_before + 1))
			) {
				t2replay_exact_diag.reject = 0xFE;
				t2replay_fail();
				return;
			}
			t2replay_exact_diag.anchor_consumed = 1;
			t2replay_exact_diag.sample_cursor = t2replay_sample_cursor;
			t2replay_exact_diag.packet_cursor = t2replay_packet_cursor;
		}
#endif
		if(host_input & INPUT_CANCEL) {
			t2replay_fail();
			return;
		}
		if(phase == T2REPLAY_PHASE_GAMEPLAY) {
			t2replay_fast_forward_wait_skip(
				(host_input & INPUT_SHOT) != 0
			);
		} else {
			t2replay_fast_forward_wait_skip(false);
		}
	}
}

static bool t2replay_input_wait_pair(bool far &seen)
{
	if(t2replay_mode == T2RM_PLAYBACK) {
		input_reset_sense();
		replay_input_sample(T2REPLAY_PHASE_DIALOG);
		if(replay_playback_exit_requested()) {
			return false;
		}
		seen = (key_det != INPUT_NONE);
		frame_delay(2);
	} else {
		seen = (key_delay_sense() != 0);
		key_det = seen ? INPUT_OK : INPUT_NONE;
		replay_input_sample(T2REPLAY_PHASE_DIALOG);
	}
	return true;
}

bool replay_input_wait_for_change(void)
{
	input_t key_before = key_det;
	bool seen;

	if(t2replay_mode == T2RM_DISABLED) {
		key_delay();
		return true;
	}
	// V1/V2 did not serialize these acknowledgement polls. Preserve their
	// historical live-input behavior; V3 is the first self-contained format.
	if(
		(t2replay_mode == T2RM_PLAYBACK) &&
		(t2replay_header.version < T2REPLAY_VERSION_TELEMETRY)
	) {
		key_delay();
		return true;
	}
	do {
		if(!t2replay_input_wait_pair(seen)) {
			key_det = key_before;
			return false;
		}
	} while(seen);
	do {
		if(!t2replay_input_wait_pair(seen)) {
			key_det = key_before;
			return false;
		}
	} while(!seen);
	// key_delay() senses its three groups directly and never updates key_det.
	// Keep that caller-visible contract even though the replay wire uses
	// temporary INPUT_NONE / INPUT_OK values for the logical acknowledgement.
	key_det = key_before;
	return true;
}

bool replay_gameover(void)
{
	if(t2replay_mode == T2RM_DISABLED) {
		return false;
	}
	t2replay_finalize(T2REPLAY_END_GAME_OVER);
	return t2replay_playback_exit;
}

bool replay_pause_save_available(void)
{
	return (
		(t2replay_mode == T2RM_RECORD) &&
		!t2replay_failed &&
		!t2replay_finished &&
		!t2replay_guard_blocked()
	);
}

bool replay_pause_save_refresh(void)
{
	if(replay_pause_save_available()) {
		(void)t2replay_guard_checkpoint(T2SAE_PAUSE);
	}
	return replay_pause_save_available();
}

bool replay_pause_restart_semantics(void)
{
	return (
		(t2replay_mode != T2RM_PLAYBACK) ||
		((t2replay_header.flags & T2REPLAY_FLAG_PAUSE_RESTART) != 0)
	);
}

bool replay_pause_restart_available(void)
{
	return (
		(t2replay_mode == T2RM_RECORD) &&
		!t2replay_finished
	);
}

bool replay_pause_restart(void)
{
	t2replay_command_t command;
	uint32_t seed;
	int fd;
	bool ok;

	if(!replay_pause_restart_available()) {
		return false;
	}
	t2replay_memclear(&command, sizeof(command));
	command.magic[0] = 'T'; command.magic[1] = '2';
	command.magic[2] = 'R'; command.magic[3] = 'C';
	command.magic[4] = 'F'; command.magic[5] = 'G';
	command.magic[6] = '2'; command.magic[7] = '\0';
	command.mode = T2REPLAY_COMMAND_RESTART;
	command.flags = (
		(t2replay_header.flags & T2REPLAY_FLAG_PRACTICE)
		? T2REPLAY_COMMAND_FLAG_PRACTICE : 0
	);
	if(t2replay_header.start.reserved[T2REPLAY_AUTOFIRE_OFFSET]) {
		command.flags |= T2REPLAY_COMMAND_FLAG_AUTOFIRE;
	}
	command.start = t2replay_header.start;
	seed = static_cast<uint32_t>(resident->frame);
	command.start.resident_frame = seed;
	command.start.random_seed = seed;

	t2replay_paths_init();
	t2replay_dos_delete(t2replay_command_fn);
	fd = t2replay_dos_create(t2replay_command_fn);
	if(fd < 0) {
		return false;
	}
	ok = (
		t2replay_dos_write(fd, &command, sizeof(command)) == sizeof(command)
	);
	t2replay_dos_close(fd);
	if(ok) {
		t2replay_fast_forward_boundary_reset();
		ok = t2replay_handoff_witness_write(&command, sizeof(command));
	}
	if(!ok) {
		t2replay_dos_delete(t2replay_command_fn);
		return false;
	}
	t2replay_guard_end();
	t2replay_temp_set();
	t2replay_pending_files_delete();
	t2replay_mode = T2RM_DISABLED;
	t2replay_finished = true;
	return true;
}

bool replay_pause_save_and_exit(void)
{
	if(t2replay_mode == T2RM_PLAYBACK) {
		// A finalized replay can only contain this Pause terminal action if the
		// recording saved on it. Consume and validate the terminal control before
		// GameExecl() returns to OP; disabling playback here would silently leave
		// that final packet unread.
		t2replay_finalize(T2REPLAY_END_MENU_RETURN);
		return (t2replay_playback_exit && !t2replay_failed);
	}
	bool saved = replay_pause_save_available();

	if(saved) {
		t2replay_finalize(T2REPLAY_END_MENU_RETURN);
		saved = (!t2replay_failed && !t2replay_guard_blocked());
	}
	if(!saved) {
		t2replay_paths_init();
		t2replay_temp_set();
		t2replay_pending_files_delete();
		t2replay_mode = T2RM_DISABLED;
		t2replay_finished = true;
	}
	return saved;
}

void replay_pause_exit_without_saving(void)
{
	t2replay_fast_forward_boundary_reset();
	if(t2replay_mode == T2RM_RECORD) {
		t2replay_guard_end();
	}
	t2replay_paths_init();
	t2replay_temp_set();
	t2replay_pending_files_delete();
	t2replay_mode = T2RM_DISABLED;
	t2replay_finished = true;
}

bool replay_process_end(const char *binary_fn)
{
	if(!t2replay_finished && (t2replay_mode != T2RM_DISABLED)) {
		t2replay_finalize(
			(binary_fn[0] == 'm') ? T2REPLAY_END_CLEAR : T2REPLAY_END_GAME_OVER
		);
	}
#if T2REPLAY_EXACT_APPLY
	t2xobs_terminal();
	t2replay_exact_diag.sample_cursor = t2replay_sample_cursor;
	t2replay_exact_diag.packet_cursor = t2replay_packet_cursor;
	t2replay_exact_diag.page_front = page_front;
	t2replay_exact_diag.page_back = page_back;
	t2replay_exact_diag_flush();
	t2replay_exact_envelope_free();
#endif
	return t2replay_playback_exit;
}

bool replay_save_request_prompt_needed(void)
{
	t2replay_save_request_t request;
	char request_fn[11];

	if(t2replay_save_prompted) {
		return false;
	}
	t2replay_save_prompted = true;
	if(
		(t2replay_header.status != T2REPLAY_STATUS_FINALIZED) ||
		(t2replay_header.end_reason != T2REPLAY_END_GAME_OVER)
	) {
		t2replay_save_request_fn_set(request_fn);
		t2replay_dos_delete(request_fn);
		return false;
	}
	if(
		!t2replay_save_request_read(&request) ||
		(request.source != T2REPLAY_SAVE_REQUEST_GAME_OVER) ||
		(request.source != t2replay_header.end_reason) ||
		(request.replay_header_checksum != t2replay_header.header_checksum)
	) {
		// Preserve a malformed T2RPY.TMP for diagnostics, but never its stale
		// handoff request.
		t2replay_save_request_fn_set(request_fn);
		t2replay_dos_delete(request_fn);
		return false;
	}
	return true;
}

void replay_save_request_discard(void)
{
	t2replay_paths_init();
	t2replay_temp_set();
	t2replay_pending_files_delete();
}

bool replay_playback_active(void)
{
	return (t2replay_mode == T2RM_PLAYBACK);
}

bool replay_playback_exit_requested(void)
{
	return (t2replay_mode == T2RM_PLAYBACK) && (t2replay_failed || quit);
}

// Keep this replay-owned segment's growth paragraph-aligned so every
// following stock and patch segment retains its audited phase.
#pragma codestring "\x90\x90\x90"

#pragma codeseg T2RCKVAL_TEXT
// Read-only bridge for the later common-apply parcel. Keeping it in its own
// tail means the existing T2REPLAY_TEXT contribution remains size-stable.
bool16 far replay_checkpoint_schema4_valid(
	const uint8_t far *container, uint32_t container_size
)
{
	return t2replay_checkpoint_valid(container, container_size);
}

#if T2REPLAY_EXACT_APPLY
bool16 far replay_checkpoint_common_groups_valid(
	const uint8_t far *group[]
)
{
	uint8_t id;

	if(group == 0) {
		return false;
	}
	for(id = 0; id < T2REPLAY_CHECKPOINT_GROUP_COUNT; id++) {
		if(
			(group[id] == 0) ||
			!t2replay_checkpoint_group_payload_valid(id, group[id])
		) {
			return false;
		}
	}
	return true;
}
#endif

#pragma codeseg
#if T2REPLAY_PRACTICE_DIAGNOSTICS
#define T2PRACT_DIAG_MAIN 1
#include "th02/t2pdiag.cpp"
#undef T2PRACT_DIAG_MAIN
#endif
