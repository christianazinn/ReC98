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
#include "libs/master.lib/pc98_gfx.hpp"
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
#include "th02/main/s1_actor.hpp"
#include "th02/main/s2_actor.hpp"
#include "th02/main/s3_actor.hpp"
#include "th02/main/s4_actor.hpp"
#include "th02/main/s5_actor.hpp"
#include "th02/main/s6_actor.hpp"
#include "th02/main/playperf.hpp"
#include "th02/main/practice.hpp"
#include "th02/main/score.hpp"
#include "th02/main/scroll.hpp"
#include "th02/main/slowdown.hpp"
#include "th02/main/stage/stage.hpp"
#include "th02/main/player/player.hpp"
#include "th02/main/player/bomb.hpp"
#include "th02/main/player/shot.hpp"
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
static uint8_t t2replay_practice_target;

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
	const uint8_t *envelope, uint32_t envelope_size,
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
	uint8_t practice_target = start->reserved[T2REPLAY_PRACTICE_TARGET_OFFSET];
	bool practice_target_valid = false;

	switch(practice_target) {
	case T2RPT_STAGE_START:
		practice_target_valid = true;
		break;
	case T2RPT_STAGE1_CHAPTER2:
		practice_target_valid = (start->stage == 0);
		break;
	case T2RPT_STAGE2_CHAPTER2:
		practice_target_valid = (start->stage == 1);
		break;
	case T2RPT_STAGE3_CHAPTER2:
		practice_target_valid = (start->stage == 2);
		break;
	case T2RPT_STAGE4_CHAPTER2:
	case T2RPT_STAGE4_CHAPTER3:
		practice_target_valid = (start->stage == 3);
		break;
	case T2RPT_EXTRA_CHAPTER2:
		practice_target_valid = (start->stage == 5);
		break;
	case T2RPT_STAGE1_MIDBOSS:
	case T2RPT_STAGE1_BOSS_PHASE1:
	case T2RPT_STAGE1_BOSS_PHASE2:
	case T2RPT_STAGE1_BOSS_PHASE3:
		practice_target_valid = (start->stage == 0);
		break;
	case T2RPT_STAGE2_MIDBOSS:
	case T2RPT_STAGE2_BOSS_PHASE1:
	case T2RPT_STAGE2_BOSS_PHASE2:
	case T2RPT_STAGE2_BOSS_PHASE3:
		practice_target_valid = (start->stage == 1);
		break;
	case T2RPT_STAGE3_MIDBOSS:
	case T2RPT_STAGE3_BOSS_START:
		practice_target_valid = (start->stage == 2);
		break;
	case T2RPT_STAGE4_MIDBOSS_FIRST:
	case T2RPT_STAGE4_MIDBOSS_SECOND:
	case T2RPT_STAGE4_BOSS_START:
		practice_target_valid = (start->stage == 3);
		break;
	case T2RPT_STAGE5_BOSS_START:
		practice_target_valid = (start->stage == 4);
		break;
	case T2RPT_EXTRA_MIDBOSS:
	case T2RPT_EXTRA_BOSS_START:
		practice_target_valid = (start->stage == 5);
		break;
	default:
		break;
	}
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
		!practice_target_valid ||
		!t2replay_bytes_zero(
			&start->reserved[T2REPLAY_PRACTICE_RESERVED_OFFSET],
			T2REPLAY_PRACTICE_RESERVED_SIZE
		)
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
		 (command->mode != T2REPLAY_COMMAND_PLAYBACK) &&
		 (command->mode != T2REPLAY_COMMAND_PRACTICE)) ||
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
	if(command->mode == T2REPLAY_COMMAND_PRACTICE) {
		return (
			(command->flags == T2REPLAY_COMMAND_FLAG_PRACTICE) &&
			t2replay_practice_start_valid(&command->start)
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

static uint8_t t2replay_command_load(
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
	return command.mode;
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
	uint8_t command_mode;
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
	t2replay_practice_target = T2RPT_STAGE_START;
	if(command_mode == T2REPLAY_COMMAND_PRACTICE) {
		t2replay_start_apply(&command_start);
		t2replay_practice_target = command_start.reserved[
			T2REPLAY_PRACTICE_TARGET_OFFSET
		];
		return;
	}
	if(command_mode == T2RM_RECORD) {
		t2replay_mode = T2RM_RECORD;
		t2replay_header_capture();
		if(command_flags & T2REPLAY_COMMAND_FLAG_PRACTICE) {
			t2replay_header.start = command_start;
			t2replay_header_apply();
			t2replay_practice_target = command_start.reserved[
				T2REPLAY_PRACTICE_TARGET_OFFSET
			];
		}
		if(!t2replay_start_valid(&t2replay_header.start) ||
			!t2replay_header_write(true)) {
			t2replay_mode = T2RM_DISABLED;
		}
	} else if(t2replay_header_read()) {
		t2replay_mode = T2RM_PLAYBACK;
		t2replay_payload_checksum = T2REPLAY_FNV1A_BASIS;
		t2replay_header_apply();
		t2replay_practice_target = t2replay_header.start.reserved[
			T2REPLAY_PRACTICE_TARGET_OFFSET
		];
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
	if(!th02_s1_rika_clean_init(target)) {
		return false;
	}
	t2replay_boss_promote_clean(rika_bgm_fn);
	return true;
}

static bool16 near t2replay_stage2_meira_activate_clean(
	th02_s2_meira_clean_target_t target
)
{
	if(!th02_s2_meira_clean_init(target)) {
		return false;
	}
	t2replay_boss_promote_clean(aBoss4_m);
	return true;
}

static bool16 near t2replay_stage4_midboss_activate_clean(
	th02_s4_midboss_clean_target_t target, int target_scroll_step
)
{
	if(
		!practice_chapter_field_build(target_scroll_step) ||
		!th02_s4_midboss_clean_init(target)
	) {
		return false;
	}
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
		return false;
	}
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

bool16 replay_practice_target_apply(void)
{
	uint8_t target = t2replay_practice_target;
	int target_scroll_step;
	th02_s1_rika_clean_target_t rika_target;
	th02_s2_meira_clean_target_t meira_target;

	if(target == T2RPT_STAGE_START) {
		return true;
	}
	switch(target) {
	case T2RPT_STAGE1_CHAPTER2:
		if(stage_id != 0) {
			return false;
		}
		target_scroll_step = 186;
		break;
	case T2RPT_STAGE2_CHAPTER2:
		if(stage_id != 1) {
			return false;
		}
		target_scroll_step = 135;
		break;
	case T2RPT_STAGE3_CHAPTER2:
		if(stage_id != 2) {
			return false;
		}
		th02_s3_field_clean_init();
		target_scroll_step = 151;
		break;
	case T2RPT_STAGE4_CHAPTER2:
		if(stage_id != 3) {
			return false;
		}
		target_scroll_step = 1327;
		break;
	case T2RPT_STAGE4_CHAPTER3:
		if(stage_id != 3) {
			return false;
		}
		target_scroll_step = 2008;
		break;
	case T2RPT_EXTRA_CHAPTER2:
		if(stage_id != 5) {
			return false;
		}
		target_scroll_step = 239;
		break;
	case T2RPT_STAGE1_MIDBOSS:
		if(stage_id != 0) {
			return false;
		}
		target_scroll_step = 116;
		break;
	case T2RPT_STAGE1_BOSS_PHASE1:
	case T2RPT_STAGE1_BOSS_PHASE2:
	case T2RPT_STAGE1_BOSS_PHASE3:
		if(stage_id != 0) {
			return false;
		}
		rika_target = static_cast<th02_s1_rika_clean_target_t>(
			target - T2RPT_STAGE1_BOSS_PHASE1
		);
		if(
			!practice_terminal_field_build() ||
			!t2replay_stage1_rika_activate_clean(rika_target)
		) {
			return false;
		}
		t2replay_practice_target = T2RPT_STAGE_START;
		return true;
	case T2RPT_STAGE2_MIDBOSS:
		if(stage_id != 1) {
			return false;
		}
		target_scroll_step = 80;
		break;
	case T2RPT_STAGE2_BOSS_PHASE1:
	case T2RPT_STAGE2_BOSS_PHASE2:
	case T2RPT_STAGE2_BOSS_PHASE3:
		if(stage_id != 1) {
			return false;
		}
		meira_target = static_cast<th02_s2_meira_clean_target_t>(
			target - T2RPT_STAGE2_BOSS_PHASE1
		);
		if(
			!practice_terminal_field_build() ||
			!t2replay_stage2_meira_activate_clean(meira_target)
		) {
			return false;
		}
		t2replay_practice_target = T2RPT_STAGE_START;
		return true;
	case T2RPT_STAGE3_MIDBOSS:
		if(stage_id != 2) {
			return false;
		}
		th02_s3_field_clean_init();
		target_scroll_step = 103;
		break;
	case T2RPT_STAGE3_BOSS_START:
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
		th02_s3_stones_clean_init();
		t2replay_boss_promote_clean(aBoss2_m);
		t2replay_practice_target = T2RPT_STAGE_START;
		return true;
	case T2RPT_STAGE4_MIDBOSS_FIRST:
		if(
			(stage_id != 3) ||
			!t2replay_stage4_midboss_activate_clean(
				T2S4_MIDBOSS_FIRST, 944
			)
		) {
			return false;
		}
		t2replay_practice_target = T2RPT_STAGE_START;
		return true;
	case T2RPT_STAGE4_MIDBOSS_SECOND:
		if(
			(stage_id != 3) ||
			!t2replay_stage4_midboss_activate_clean(
				T2S4_MIDBOSS_SECOND, 1632
			)
		) {
			return false;
		}
		t2replay_practice_target = T2RPT_STAGE_START;
		return true;
	case T2RPT_STAGE4_BOSS_START:
		if((stage_id != 3) || !t2replay_stage4_marisa_activate_clean()) {
			return false;
		}
		t2replay_practice_target = T2RPT_STAGE_START;
		return true;
	case T2RPT_STAGE5_BOSS_START:
		if((stage_id != 4) || !t2replay_stage5_mima_activate_clean()) {
			return false;
		}
		t2replay_practice_target = T2RPT_STAGE_START;
		return true;
	case T2RPT_EXTRA_MIDBOSS:
		if(stage_id != 5) {
			return false;
		}
		target_scroll_step = 200;
		break;
	case T2RPT_EXTRA_BOSS_START:
		if((stage_id != 5) || !t2replay_extra_sigma_activate_clean()) {
			return false;
		}
		t2replay_practice_target = T2RPT_STAGE_START;
		return true;
	default:
		return false;
	}
	if(!practice_chapter_field_build(target_scroll_step)) {
		return false;
	}
	if((target == T2RPT_STAGE4_CHAPTER2) ||
	   (target == T2RPT_STAGE4_CHAPTER3)) {
		// The first Stage 4 midboss appearance owns this re-arm in native play.
		midboss_scroll_step = 1632;
	}
	t2replay_practice_target = T2RPT_STAGE_START;
	return true;
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

#pragma codeseg T2RCKVAL_TEXT
// Read-only bridge for the later common-apply parcel. Keeping it in its own
// tail means the existing T2REPLAY_TEXT contribution remains size-stable.
bool16 far replay_checkpoint_schema4_valid(
	const uint8_t far *container, uint32_t container_size
)
{
	return t2replay_checkpoint_valid(container, container_size);
}

#pragma codeseg
