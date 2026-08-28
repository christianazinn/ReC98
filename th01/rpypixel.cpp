// Private, measurement-only TH01 pixel witnesses. T1PIX.BIN and T1KPX.BIN
// are not user replay sidecars and are compiled out of every public profile.

#pragma option -zCT1PIXEL_TEXT -G-

#include <stdio.h>
#include "platform.h"
#include "pc98.h"
#include "x86real.h"
#include "th01/hardware/graph.h"
#include "th01/hardware/palette.h"
#include "th01/main/boss/entity_a.hpp"
#include "th01/t1ymx.hpp"
#include "th01/replay.hpp"
#include "th01/rpypixel.hpp"

#if T1REPLAY_PRIVATE_PIXEL_TRACE

#define T1PIXEL_HEADER_SIZE 24
#define T1PIXEL_TRAM_ROWS (RES_Y / GLYPH_H)
#define T1PIXEL_TRAM_ROW_BYTES ((RES_X / GLYPH_HALF_W) * 2)

extern page_t page_accessed;
#if !T1REPLAY_YUUGENMAGAN_FIRST_COMBAT_TRACE
extern page_t page_shown;
#endif

static void t1pixel_memclear(void far *buf, unsigned size)
{
	uint8_t far *p = reinterpret_cast<uint8_t far *>(buf);

	while(size != 0) {
		*p++ = 0;
		size--;
	}
}

static uint32_t t1pixel_hash(
	uint32_t hash, const void far *buf, unsigned size
)
{
	const uint8_t far *p = reinterpret_cast<const uint8_t far *>(buf);

	while(size != 0) {
		hash ^= *p++;
		hash *= 0x01000193UL;
		size--;
	}
	return hash;
}

static uint16_t t1pixel_plane_segment(uint8_t plane)
{
	switch(plane) {
	case 0: return SEG_PLANE_B;
	case 1: return SEG_PLANE_R;
	case 2: return SEG_PLANE_G;
	default: return SEG_PLANE_E;
	}
}

static void t1pixel_surface_capture(
	uint8_t &visible_page, uint8_t &accessed_page,
	uint32_t &palette_digest,
	uint32_t vram_digest[PAGE_COUNT][PLANE_COUNT],
	uint32_t tram_jis_digest[T1PIXEL_TRAM_ROWS],
	uint32_t tram_attr_digest[T1PIXEL_TRAM_ROWS]
)
{
	page_t accessed_before = page_accessed;
	uint8_t page;
	uint8_t plane;
	uint8_t tram_row;

	// T1RP7 preserves the native access page, but its source has no stable
	// visible-page register to observe without injecting graph state.
#if T1REPLAY_YUUGENMAGAN_FIRST_COMBAT_TRACE
	visible_page = 0xFF;
#else
	visible_page = page_shown;
#endif
	accessed_page = accessed_before;
	palette_digest = t1pixel_hash(
		T1REPLAY_FNV1A_BASIS, &z_Palettes, sizeof(z_Palettes)
	);
	for(page = 0; page < PAGE_COUNT; page++) {
		graph_accesspage_func(page);
		for(plane = 0; plane < PLANE_COUNT; plane++) {
			vram_digest[page][plane] = t1pixel_hash(
				T1REPLAY_FNV1A_BASIS,
				MK_FP(t1pixel_plane_segment(plane), 0), PLANE_SIZE
			);
		}
	}
	graph_accesspage_func(accessed_before);
	for(tram_row = 0; tram_row < T1PIXEL_TRAM_ROWS; tram_row++) {
		tram_jis_digest[tram_row] = t1pixel_hash(
			T1REPLAY_FNV1A_BASIS,
			MK_FP(SEG_TRAM_JIS, (tram_row * T1PIXEL_TRAM_ROW_BYTES)),
			T1PIXEL_TRAM_ROW_BYTES
		);
		tram_attr_digest[tram_row] = t1pixel_hash(
			T1REPLAY_FNV1A_BASIS,
			MK_FP(SEG_TRAM_ATRB, (tram_row * T1PIXEL_TRAM_ROW_BYTES)),
			T1PIXEL_TRAM_ROW_BYTES
		);
	}
}

#endif

#if T1REPLAY_YUUGENMAGAN_FIRST_COMBAT_TRACE

#define T1YMX_HEADER_SIZE 24
#define T1YMX_ROW_SIZE 248
#define T1YMX_ROW_COUNT 2

enum t1ymx_point_t {
	T1YMX_FIRST_PRE_INPUT = 1,
	T1YMX_SECOND_PRE_INPUT,
};

// boss.hpp's BID_YUUGENMAGAN value. entity_a.hpp already transitively brings
// in its unguarded declarations, so keep this private profile self-contained.
enum { T1YMX_BID_YUUGENMAGAN = 2 };

enum t1ymx_state_t {
	T1YMXS_OFF,
	T1YMXS_WAIT_FIRST,
	T1YMXS_WAIT_SECOND,
	T1YMXS_DONE,
	T1YMXS_FAILED,
};

struct t1ymx_header_t {
	char magic[8];
	uint16_t version;
	uint16_t header_size;
	uint16_t row_size;
	uint8_t expected_rows;
	uint8_t target;
	uint8_t pages;
	uint8_t planes;
	uint8_t tram_rows;
	uint8_t visible_page;
	uint8_t owner_size;
	uint8_t reserved[3];
};

struct t1ymx_row_t {
	uint8_t point;
	uint8_t visible_page;
	uint8_t accessed_page;
	uint8_t reserved;
	uint32_t owner_digest;
	uint32_t palette_digest;
	uint32_t vram_digest[PAGE_COUNT][PLANE_COUNT];
	uint32_t tram_jis_digest[T1PIXEL_TRAM_ROWS];
	uint32_t tram_attr_digest[T1PIXEL_TRAM_ROWS];
	uint32_t checksum;
};

typedef char t1ymx_header_size_check[
	(sizeof(t1ymx_header_t) == T1YMX_HEADER_SIZE) ? 1 : -1
];
typedef char t1ymx_row_size_check[
	(sizeof(t1ymx_row_t) == T1YMX_ROW_SIZE) ? 1 : -1
];

// Keep the private recorder's state out of DGROUP. This profile must preserve
// every stock near-BSS offset, including the C runtime tail.
static t1ymx_state_t far t1ymx_state;
static uint32_t far t1ymx_expected_owner_digest;

extern int8_t boss_id;

static bool t1ymx_records_equal(
	const t1boss_yuugenmagan_checkpoint_t& expected,
	const t1boss_yuugenmagan_checkpoint_t& actual
)
{
	int i;

	if(
		(expected.owner != actual.owner) ||
		(expected.schema != actual.schema) ||
		(expected.phase != actual.phase) ||
		(expected.reserved_0 != actual.reserved_0) ||
		(expected.phase_frame != actual.phase_frame) ||
		(expected.hp != actual.hp) ||
		(expected.invincibility_frame != actual.invincibility_frame) ||
		(expected.pattern_interval != actual.pattern_interval) ||
		(expected.u1 != actual.u1) || (expected.u2 != actual.u2) ||
		(expected.target_left != actual.target_left) ||
		(expected.unused_distance != actual.unused_distance) ||
		(expected.after_hit_frame != actual.after_hit_frame) ||
		(expected.u3 != actual.u3) ||
		(expected.initial_hp_rendered != actual.initial_hp_rendered) ||
		(expected.angle != actual.angle) ||
		(expected.angle_missile_southeast != actual.angle_missile_southeast) ||
		(expected.hit_invincible != actual.hit_invincible) ||
		(expected.pentagram_phase != actual.pentagram_phase) ||
		(expected.pentagram_angle != actual.pentagram_angle) ||
		(expected.line_radius != actual.line_radius) ||
		(expected.line_center_x != actual.line_center_x) ||
		(expected.line_center_y != actual.line_center_y) ||
		(expected.line_velocity_x != actual.line_velocity_x) ||
		(expected.line_velocity_y != actual.line_velocity_y) ||
		(expected.reserved[0] != actual.reserved[0])
	) {
		return false;
	}
	for(i = 0; i < 5; i++) {
		if(
			(expected.line_x[i] != actual.line_x[i]) ||
			(expected.line_y[i] != actual.line_y[i]) ||
			(expected.eye_image[i] != actual.eye_image[i]) ||
			(expected.eye_hitbox_inactive[i] != actual.eye_hitbox_inactive[i]) ||
			(expected.eye_lock_frame[i] != actual.eye_lock_frame[i])
		) {
			return false;
		}
	}
	return true;
}

static bool t1ymx_header_write(void)
{
	t1ymx_header_t header;
	FILE *file;
	bool ok;
	char filename[10];
	char mode[3];

	t1pixel_memclear(&header, sizeof(header));
	header.magic[0] = 'T'; header.magic[1] = '1';
	header.magic[2] = 'Y'; header.magic[3] = 'M';
	header.magic[4] = 'X'; header.magic[5] = '1';
	header.version = 1;
	header.header_size = T1YMX_HEADER_SIZE;
	header.row_size = T1YMX_ROW_SIZE;
	header.expected_rows = T1YMX_ROW_COUNT;
	header.target = 1;
	header.pages = PAGE_COUNT;
	header.planes = PLANE_COUNT;
	header.tram_rows = T1PIXEL_TRAM_ROWS;
	header.visible_page = 0xFF;
	header.owner_size = T1BOSS_YUUGENMAGAN_CHECKPOINT_SIZE;
	filename[0] = 'T'; filename[1] = '1'; filename[2] = 'Y';
	filename[3] = 'M'; filename[4] = 'X'; filename[5] = '.';
	filename[6] = 'B'; filename[7] = 'I'; filename[8] = 'N';
	filename[9] = '\0';
	mode[0] = 'w'; mode[1] = 'b'; mode[2] = '\0';
	file = fopen(filename, mode);
	if(!file) {
		return false;
	}
	ok = (fwrite(&header, 1, sizeof(header), file) == sizeof(header));
	if(fclose(file) != 0) {
		ok = false;
	}
	return ok;
}

static bool t1ymx_row_write(uint8_t point)
{
	t1ymx_row_t row;
	t1boss_yuugenmagan_checkpoint_t actual;
	FILE *file;
	bool ok;
	char filename[10];
	char mode[3];

	if(!t1boss_yuugenmagan_checkpoint_capture(&actual)) {
		return false;
	}
	t1pixel_memclear(&row, sizeof(row));
	row.point = point;
	row.owner_digest = t1pixel_hash(
		T1REPLAY_FNV1A_BASIS, &actual, sizeof(actual)
	);
	if(
		(point == T1YMX_FIRST_PRE_INPUT) &&
		(row.owner_digest != t1ymx_expected_owner_digest)
	) {
		return false;
	}
	t1pixel_surface_capture(
		row.visible_page, row.accessed_page, row.palette_digest,
		row.vram_digest, row.tram_jis_digest, row.tram_attr_digest
	);
	row.checksum = t1pixel_hash(
		T1REPLAY_FNV1A_BASIS, &row, (sizeof(row) - sizeof(row.checksum))
	);
	filename[0] = 'T'; filename[1] = '1'; filename[2] = 'Y';
	filename[3] = 'M'; filename[4] = 'X'; filename[5] = '.';
	filename[6] = 'B'; filename[7] = 'I'; filename[8] = 'N';
	filename[9] = '\0';
	mode[0] = 'a'; mode[1] = 'b'; mode[2] = '\0';
	file = fopen(filename, mode);
	if(!file) {
		return false;
	}
	ok = (fwrite(&row, 1, sizeof(row), file) == sizeof(row));
	if(fclose(file) != 0) {
		ok = false;
	}
	return ok;
}

static bool t1ymx_arm_if_first_combat(void)
{
	t1boss_yuugenmagan_checkpoint_t expected;
	t1boss_yuugenmagan_checkpoint_t actual;

	if(
		(boss_id != T1YMX_BID_YUUGENMAGAN) ||
		!t1boss_yuugenmagan_first_combat_construct(&expected) ||
		!t1boss_yuugenmagan_checkpoint_capture(&actual) ||
		(actual.phase != 1) ||
		(actual.phase_frame != 0)
	) {
		return false;
	}
	t1ymx_state = T1YMXS_FAILED;
	if(!t1ymx_records_equal(expected, actual) || !t1ymx_header_write()) {
		return false;
	}
	t1ymx_expected_owner_digest = t1pixel_hash(
		T1REPLAY_FNV1A_BASIS, &expected, sizeof(expected)
	);
	t1ymx_state = T1YMXS_WAIT_FIRST;
	return true;
}

void t1ymx_pre_input(void)
{
	uint8_t point;

	if(t1ymx_state == T1YMXS_OFF) {
		if(!t1ymx_arm_if_first_combat()) {
			return;
		}
	}
	if(t1ymx_state == T1YMXS_WAIT_FIRST) {
		point = T1YMX_FIRST_PRE_INPUT;
		t1ymx_state = T1YMXS_WAIT_SECOND;
	} else if(t1ymx_state == T1YMXS_WAIT_SECOND) {
		point = T1YMX_SECOND_PRE_INPUT;
		t1ymx_state = T1YMXS_DONE;
	} else {
		return;
	}
	if(!t1ymx_row_write(point)) {
		t1ymx_state = T1YMXS_FAILED;
	}
}

#endif

#if T1REPLAY_KONNGARA_PHASE1_TRACE

#define T1KPX_HEADER_SIZE 24
#define T1KPX_ROW_SIZE 444
#define T1KPX_ROW_COUNT 3
#define T1KPX_OWNER_VALUE_COUNT 81
#define T1KPX_WORLD_COUNT 9

// b20j.cpp keeps these owner-local enums private. The probe receives their
// values at the arm site and checks the documented canonical seam values.
enum {
	T1KPX_FACE_DIRECTION_CENTER = 2,
	T1KPX_FACE_EXPRESSION_NEUTRAL = 0,
};

enum t1kpx_point_t {
	T1KXP_NATURAL_SEAM = 1,
	T1KXP_FIRST_PRE_INPUT,
	T1KXP_SECOND_PRE_INPUT,
};

enum t1kpx_state_t {
	T1KXS_OFF,
	T1KXS_WAIT_NATURAL_SEAM,
	T1KXS_WAIT_FIRST,
	T1KXS_WAIT_SECOND,
	T1KXS_DONE,
	T1KXS_FAILED,
};

struct t1kpx_header_t {
	char magic[8];
	uint16_t version;
	uint16_t header_size;
	uint16_t row_size;
	uint8_t expected_rows;
	uint8_t target;
	uint8_t pages;
	uint8_t planes;
	uint8_t tram_rows;
	uint8_t visible_page;
	uint8_t owner_value_count;
	uint8_t world_count;
	uint16_t reserved;
};

struct t1kpx_row_t {
	uint8_t point;
	uint8_t process_seq;
	uint8_t visible_page;
	uint8_t accessed_page;
	uint32_t sample_cursor;
	uint32_t packet_cursor;
	uint32_t input_cursor;
	uint32_t semantic_digest;
	uint32_t owner_digest;
	uint16_t owner_values[T1KPX_OWNER_VALUE_COUNT];
	uint16_t world_counts[T1KPX_WORLD_COUNT];
	uint32_t palette_digest;
	uint32_t vram_digest[PAGE_COUNT][PLANE_COUNT];
	uint32_t tram_jis_digest[T1PIXEL_TRAM_ROWS];
	uint32_t tram_attr_digest[T1PIXEL_TRAM_ROWS];
	uint32_t checksum;
};

typedef char t1kpx_header_size_check[
	(sizeof(t1kpx_header_t) == T1KPX_HEADER_SIZE) ? 1 : -1
];
typedef char t1kpx_row_size_check[
	(sizeof(t1kpx_row_t) == T1KPX_ROW_SIZE) ? 1 : -1
];

static t1kpx_state_t t1kpx_state;
static uint16_t t1kpx_owner_values[T1KPX_OWNER_VALUE_COUNT];
static uint32_t t1kpx_owner_digest;

static void t1kpx_owner_value_set(unsigned &index, int value)
{
	if(index < T1KPX_OWNER_VALUE_COUNT) {
		t1kpx_owner_values[index] = static_cast<uint16_t>(value);
	}
	index++;
}

static void t1kpx_owner_entity_capture(
	unsigned &index, const CBossEntity& entity
)
{
	t1kpx_owner_value_set(index, entity.cur_left);
	t1kpx_owner_value_set(index, entity.cur_top);
	t1kpx_owner_value_set(index, entity.prev_left);
	t1kpx_owner_value_set(index, entity.prev_top);
	t1kpx_owner_value_set(index, entity.vram_w);
	t1kpx_owner_value_set(index, entity.h);
	t1kpx_owner_value_set(index, entity.move_clamp.left);
	t1kpx_owner_value_set(index, entity.move_clamp.right);
	t1kpx_owner_value_set(index, entity.move_clamp.top);
	t1kpx_owner_value_set(index, entity.move_clamp.bottom);
	t1kpx_owner_value_set(index, entity.hitbox_orb.left);
	t1kpx_owner_value_set(index, entity.hitbox_orb.right);
	t1kpx_owner_value_set(index, entity.hitbox_orb.top);
	t1kpx_owner_value_set(index, entity.hitbox_orb.bottom);
	t1kpx_owner_value_set(index, entity.prev_delta_x);
	t1kpx_owner_value_set(index, entity.prev_delta_y);
	t1kpx_owner_value_set(index, entity.bos_image_count);
	t1kpx_owner_value_set(index, entity.image());
	t1kpx_owner_value_set(index, entity.hitbox_orb_inactive);
	t1kpx_owner_value_set(index, entity.loading);
	t1kpx_owner_value_set(index, entity.lock_frame);
	t1kpx_owner_value_set(index, entity.bos_slot);
}

static bool t1kpx_header_write(void)
{
	t1kpx_header_t header;
	FILE *file;
	bool ok;

	t1pixel_memclear(&header, sizeof(header));
	header.magic[0] = 'T'; header.magic[1] = '1';
	header.magic[2] = 'K'; header.magic[3] = 'P';
	header.magic[4] = 'X'; header.magic[5] = '1';
	header.version = 1;
	header.header_size = T1KPX_HEADER_SIZE;
	header.row_size = T1KPX_ROW_SIZE;
	header.expected_rows = T1KPX_ROW_COUNT;
	header.target = 1;
	header.pages = PAGE_COUNT;
	header.planes = PLANE_COUNT;
	header.tram_rows = T1PIXEL_TRAM_ROWS;
	header.visible_page = page_shown;
	header.owner_value_count = T1KPX_OWNER_VALUE_COUNT;
	header.world_count = T1KPX_WORLD_COUNT;
	file = fopen("T1KPX.BIN", "wb");
	if(!file) {
		return false;
	}
	ok = (fwrite(&header, 1, sizeof(header), file) == sizeof(header));
	if(fclose(file) != 0) {
		ok = false;
	}
	return ok;
}

static bool t1kpx_row_write(
	uint8_t point, uint8_t process_seq, uint32_t sample_cursor,
	uint32_t packet_cursor, uint32_t input_cursor,
	const t1replay_pixel_world_t& world
)
{
	t1kpx_row_t row;
	FILE *file;
	bool ok;
	unsigned i;

	t1pixel_memclear(&row, sizeof(row));
	row.point = point;
	row.process_seq = process_seq;
	row.sample_cursor = sample_cursor;
	row.packet_cursor = packet_cursor;
	row.input_cursor = input_cursor;
	row.semantic_digest = world.semantic_digest;
	row.owner_digest = t1kpx_owner_digest;
	for(i = 0; i < T1KPX_OWNER_VALUE_COUNT; i++) {
		row.owner_values[i] = t1kpx_owner_values[i];
	}
	row.world_counts[0] = world.cards;
	row.world_counts[1] = world.obstacles;
	row.world_counts[2] = world.bomb_items;
	row.world_counts[3] = world.point_items;
	row.world_counts[4] = world.pellets;
	row.world_counts[5] = world.shots;
	row.world_counts[6] = world.missiles;
	row.world_counts[7] = world.lasers;
	row.world_counts[8] = world.particles;
	t1pixel_surface_capture(
		row.visible_page, row.accessed_page, row.palette_digest,
		row.vram_digest, row.tram_jis_digest, row.tram_attr_digest
	);
	row.checksum = t1pixel_hash(
		T1REPLAY_FNV1A_BASIS, &row,
		(sizeof(row) - sizeof(row.checksum))
	);
	file = fopen("T1KPX.BIN", "ab");
	if(!file) {
		return false;
	}
	ok = (fwrite(&row, 1, sizeof(row), file) == sizeof(row));
	if(fclose(file) != 0) {
		ok = false;
	}
	return ok;
}

void t1replay_pixel_probe_konngara_phase1_arm(
	int8_t phase, int phase_frame, int hp, int hp_first_white,
	int hp_first_redwhite, int pattern_state, int face_direction,
	int face_expression, bool16 face_direction_can_change,
	bool16 hit_invincible, int hit_invincibility_frame, int pattern_prev,
	int pattern_cur, int patterns_done, bool16 initial_hp_rendered,
	const CBossEntity& head, const CBossEntity& face_closed_or_glare,
	const CBossEntity& face_aim
)
{
	unsigned index = 0;

	t1kpx_state = T1KXS_FAILED;
	if(
		(phase != 1) || (phase_frame != 0) || (hp != 18) ||
		(hp_first_white != 16) || (hp_first_redwhite != 10) ||
		(pattern_state != 0) ||
		(face_direction != T1KPX_FACE_DIRECTION_CENTER) ||
		(face_expression != T1KPX_FACE_EXPRESSION_NEUTRAL) ||
		!face_direction_can_change ||
		hit_invincible || (hit_invincibility_frame != 0) ||
		(pattern_prev != 99) || (pattern_cur != 99) ||
		(patterns_done != 0) || initial_hp_rendered
	) {
		return;
	}
	t1pixel_memclear(t1kpx_owner_values, sizeof(t1kpx_owner_values));
	t1kpx_owner_value_set(index, phase);
	t1kpx_owner_value_set(index, phase_frame);
	t1kpx_owner_value_set(index, hp);
	t1kpx_owner_value_set(index, hp_first_white);
	t1kpx_owner_value_set(index, hp_first_redwhite);
	t1kpx_owner_value_set(index, pattern_state);
	t1kpx_owner_value_set(index, face_direction);
	t1kpx_owner_value_set(index, face_expression);
	t1kpx_owner_value_set(index, face_direction_can_change);
	t1kpx_owner_value_set(index, hit_invincible);
	t1kpx_owner_value_set(index, hit_invincibility_frame);
	t1kpx_owner_value_set(index, pattern_prev);
	t1kpx_owner_value_set(index, pattern_cur);
	t1kpx_owner_value_set(index, patterns_done);
	t1kpx_owner_value_set(index, initial_hp_rendered);
	t1kpx_owner_entity_capture(index, head);
	t1kpx_owner_entity_capture(index, face_closed_or_glare);
	t1kpx_owner_entity_capture(index, face_aim);
	if(index != T1KPX_OWNER_VALUE_COUNT) {
		return;
	}
	t1kpx_owner_digest = t1pixel_hash(
		T1REPLAY_FNV1A_BASIS, t1kpx_owner_values,
		sizeof(t1kpx_owner_values)
	);
	t1kpx_state = T1KXS_WAIT_NATURAL_SEAM;
}

void t1replay_pixel_probe_konngara_phase1_pre_input(
	uint8_t process_seq, uint32_t sample_cursor, uint32_t packet_cursor,
	uint32_t input_cursor, int pellet_speed_raise_cycle
)
{
	t1replay_pixel_world_t world;
	uint8_t point;

	if(t1kpx_state == T1KXS_WAIT_NATURAL_SEAM) {
		if(!t1kpx_header_write()) {
			t1kpx_state = T1KXS_FAILED;
			return;
		}
		point = T1KXP_NATURAL_SEAM;
		t1kpx_state = T1KXS_WAIT_FIRST;
	} else if(t1kpx_state == T1KXS_WAIT_FIRST) {
		point = T1KXP_FIRST_PRE_INPUT;
		t1kpx_state = T1KXS_WAIT_SECOND;
	} else if(t1kpx_state == T1KXS_WAIT_SECOND) {
		point = T1KXP_SECOND_PRE_INPUT;
		t1kpx_state = T1KXS_DONE;
	} else {
		return;
	}
	if(
		!t1replay_pixel_probe_world_capture(&world, pellet_speed_raise_cycle) ||
		!t1kpx_row_write(
			point, process_seq, sample_cursor, packet_cursor, input_cursor, world
		)
	) {
		t1kpx_state = T1KXS_FAILED;
	}
}

#endif

#if T1REPLAY_PIXEL_TRACE

#define T1PIXEL_ROW_SIZE 260
#define T1PIXEL_ROW_COUNT 3

enum t1pixel_point_t {
	T1PXP_SEAM = 1,
	T1PXP_POST_RESTORE,
	T1PXP_FIRST_RESUMED,
	T1PXP_SECOND_RESUMED,
};

enum t1pixel_state_t {
	T1PXS_OFF,
	T1PXS_WAIT_SEAM,
	T1PXS_WAIT_POST_RESTORE,
	T1PXS_WAIT_FIRST,
	T1PXS_WAIT_SECOND,
	T1PXS_DONE,
	T1PXS_FAILED,
};

struct t1pixel_header_t {
	char magic[8];
	uint16_t version;
	uint16_t header_size;
	uint16_t row_size;
	uint8_t expected_rows;
	uint8_t arm;
	uint8_t pages;
	uint8_t planes;
	uint8_t tram_rows;
	uint8_t visible_page;
	uint32_t reserved;
};

struct t1pixel_row_t {
	uint8_t point;
	uint8_t process_seq;
	uint8_t visible_page;
	uint8_t accessed_page;
	uint32_t sample_cursor;
	uint32_t packet_cursor;
	uint32_t input_cursor;
	uint32_t semantic_digest;
	uint32_t palette_digest;
	uint32_t vram_digest[PAGE_COUNT][PLANE_COUNT];
	uint32_t tram_jis_digest[T1PIXEL_TRAM_ROWS];
	uint32_t tram_attr_digest[T1PIXEL_TRAM_ROWS];
	uint32_t checksum;
};

typedef char t1pixel_header_size_check[
	(sizeof(t1pixel_header_t) == T1PIXEL_HEADER_SIZE) ? 1 : -1
];
typedef char t1pixel_row_size_check[
	(sizeof(t1pixel_row_t) == T1PIXEL_ROW_SIZE) ? 1 : -1
];

static t1pixel_state_t t1pixel_state;
static uint32_t t1pixel_anchor_sample;
static uint32_t t1pixel_last_sample;

static bool t1pixel_header_write(void)
{
	t1pixel_header_t header;
	FILE *file;
	bool ok;

	t1pixel_memclear(&header, sizeof(header));
	header.magic[0] = 'T'; header.magic[1] = '1';
	header.magic[2] = 'P'; header.magic[3] = 'X';
	header.magic[4] = 'R'; header.magic[5] = '1';
	header.version = 1;
	header.header_size = T1PIXEL_HEADER_SIZE;
	header.row_size = T1PIXEL_ROW_SIZE;
	header.expected_rows = T1PIXEL_ROW_COUNT;
	header.arm = (T1RP == 4) ? 1 : 2;
	header.pages = PAGE_COUNT;
	header.planes = PLANE_COUNT;
	header.tram_rows = T1PIXEL_TRAM_ROWS;
	header.visible_page = page_shown;
	file = fopen("T1PIX.BIN", "wb");
	if(!file) {
		return false;
	}
	ok = (fwrite(&header, 1, sizeof(header), file) == sizeof(header));
	if(fclose(file) != 0) {
		ok = false;
	}
	return ok;
}

static bool t1pixel_row_write(
	uint8_t point, uint8_t process_seq, uint32_t sample_cursor,
	uint32_t packet_cursor, uint32_t input_cursor, uint32_t semantic_digest
)
{
	t1pixel_row_t row;
	FILE *file;
	bool ok;

	t1pixel_memclear(&row, sizeof(row));
	row.point = point;
	row.process_seq = process_seq;
	row.sample_cursor = sample_cursor;
	row.packet_cursor = packet_cursor;
	row.input_cursor = input_cursor;
	row.semantic_digest = semantic_digest;
	t1pixel_surface_capture(
		row.visible_page, row.accessed_page, row.palette_digest,
		row.vram_digest, row.tram_jis_digest, row.tram_attr_digest
	);
	row.checksum = t1pixel_hash(
		T1REPLAY_FNV1A_BASIS, &row,
		(sizeof(row) - sizeof(row.checksum))
	);
	file = fopen("T1PIX.BIN", "ab");
	if(!file) {
		return false;
	}
	ok = (fwrite(&row, 1, sizeof(row), file) == sizeof(row));
	if(fclose(file) != 0) {
		ok = false;
	}
	return ok;
}

bool16 t1replay_pixel_probe_arm(
	const t1replay_checkpoint_t far *checkpoint
)
{
	if(
		!checkpoint ||
		(checkpoint->boss.boss_id != BID_SINGYOKU) ||
		!t1pixel_header_write()
	) {
		t1pixel_state = T1PXS_FAILED;
		return false;
	}
	t1pixel_anchor_sample = checkpoint->pacing.replay_sample_anchor;
	t1pixel_last_sample = t1pixel_anchor_sample;
	t1pixel_state = (T1RP == 4) ? T1PXS_WAIT_SEAM : T1PXS_WAIT_POST_RESTORE;
	return true;
}

void t1replay_pixel_probe_restored(
	uint8_t process_seq, uint32_t sample_cursor, uint32_t packet_cursor,
	uint32_t input_cursor, uint32_t semantic_digest
)
{
	if(t1pixel_state != T1PXS_WAIT_POST_RESTORE) {
		return;
	}
	if(!t1pixel_row_write(
		T1PXP_POST_RESTORE, process_seq, sample_cursor, packet_cursor,
		input_cursor, semantic_digest
	)) {
		t1pixel_state = T1PXS_FAILED;
		return;
	}
	t1pixel_last_sample = sample_cursor;
	t1pixel_state = T1PXS_WAIT_FIRST;
}

void t1replay_pixel_probe_pre_input(
	uint8_t process_seq, uint32_t sample_cursor, uint32_t packet_cursor,
	uint32_t input_cursor, uint32_t semantic_digest
)
{
	uint8_t point;

	if(t1pixel_state == T1PXS_WAIT_SEAM) {
		if(sample_cursor != t1pixel_anchor_sample) {
			return;
		}
		point = T1PXP_SEAM;
		t1pixel_state = T1PXS_WAIT_FIRST;
	} else if(t1pixel_state == T1PXS_WAIT_FIRST) {
		if(sample_cursor <= t1pixel_last_sample) {
			return;
		}
		point = T1PXP_FIRST_RESUMED;
		t1pixel_state = T1PXS_WAIT_SECOND;
	} else if(t1pixel_state == T1PXS_WAIT_SECOND) {
		if(sample_cursor <= t1pixel_last_sample) {
			return;
		}
		point = T1PXP_SECOND_RESUMED;
		t1pixel_state = T1PXS_DONE;
	} else {
		return;
	}
	if(!t1pixel_row_write(
		point, process_seq, sample_cursor, packet_cursor, input_cursor,
		semantic_digest
	)) {
		t1pixel_state = T1PXS_FAILED;
		return;
	}
	t1pixel_last_sample = sample_cursor;
}

#endif

#if T1REPLAY_PRIVATE_PIXEL_TRACE
void t1replay_pixel_probe_reset(void)
{
#if T1REPLAY_PIXEL_TRACE
	t1pixel_state = T1PXS_OFF;
	t1pixel_anchor_sample = 0;
	t1pixel_last_sample = 0;
#endif
#if T1REPLAY_KONNGARA_PHASE1_TRACE
	t1kpx_state = T1KXS_OFF;
	t1pixel_memclear(t1kpx_owner_values, sizeof(t1kpx_owner_values));
	t1kpx_owner_digest = 0;
#endif
#if T1REPLAY_YUUGENMAGAN_FIRST_COMBAT_TRACE
	t1ymx_state = T1YMXS_OFF;
	t1ymx_expected_owner_digest = 0;
#endif
}
#endif
