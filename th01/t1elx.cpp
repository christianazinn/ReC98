// Private Elis post-entrance paired-acceptance witness. This module is a
// profile-only trailing tail and is absent from every release build.

#pragma option -zCT1ELX_TEXT -G-

#include <stdio.h>
#include "platform.h"
#include "pc98.h"
#include "x86real.h"
#include "libs/master.lib/master.hpp"
#include "th01/hardware/graph.h"
#include "th01/hardware/palette.h"
#include "th01/main/boss/entity_a.hpp"
#include "th01/replay_format.hpp"
#include "th01/resident.hpp"
#include "th01/t1elx.hpp"

#if T1ELX_TRACE

#define T1ELX_HEADER_SIZE 24
#define T1ELX_ROW_SIZE 256
#define T1ELX_ROW_COUNT 2
#define T1ELX_TRAM_ROWS (RES_Y / GLYPH_H)
#define T1ELX_TRAM_ROW_BYTES ((RES_X / GLYPH_HALF_W) * 2)

enum t1elx_run_kind_t {
	T1ELX_RUN_NATURAL = 1,
	T1ELX_RUN_DIRECT = 2,
};

// boss.hpp has no include guard and entity_a.hpp already includes its owner
// declarations. Keep the measured Elis ID local, as the T1YMX probe does.
enum { T1ELX_BID_ELIS = 5 };

enum t1elx_point_t {
	T1ELX_FIRST_PRE_INPUT = 1,
	T1ELX_SECOND_PRE_INPUT,
};

enum t1elx_state_t {
	T1ELXS_OFF,
	T1ELXS_WAIT_FIRST,
	T1ELXS_WAIT_SECOND,
	T1ELXS_DONE,
	T1ELXS_FAILED,
};

struct t1elx_entity_t {
	int16_t left;
	int16_t top;
	int16_t prev_left;
	int16_t prev_top;
	int16_t prev_delta_x;
	int16_t prev_delta_y;
	uint8_t image;
	uint8_t hitbox_inactive;
	int16_t lock_frame;
};

struct t1elx_owner_t {
	int16_t boss_id;
	int16_t boss_phase;
	int16_t boss_phase_frame;
	int16_t boss_hp;
	int16_t pattern_state;
	int16_t form;
	int16_t hit_invincibility_frame;
	int16_t hit_invincible;
	int16_t phase_pattern;
	int16_t teleport_done;
	int16_t bat_velocity_x;
	int16_t bat_velocity_y;
	int16_t initial_hp_rendered;
	t1elx_entity_t entity[3];
};

struct t1elx_header_t {
	char magic[8];
	uint16_t version;
	uint16_t header_size;
	uint16_t row_size;
	uint8_t expected_rows;
	uint8_t run_kind;
	uint8_t target;
	uint8_t stage_id;
	uint8_t route;
	uint8_t pages;
	uint8_t planes;
	uint8_t tram_rows;
	uint8_t visible_page;
	uint8_t owner_size;
};

struct t1elx_row_t {
	uint8_t point;
	uint8_t visible_page;
	uint8_t accessed_page;
	uint8_t reserved;
	uint32_t random_seed;
	uint32_t frame_rand;
	uint32_t owner_digest;
	uint32_t palette_digest;
	uint32_t vram_digest[PAGE_COUNT][PLANE_COUNT];
	uint32_t tram_jis_digest[T1ELX_TRAM_ROWS];
	uint32_t tram_attr_digest[T1ELX_TRAM_ROWS];
	uint32_t checksum;
};

typedef char t1elx_entity_size_check[
	(sizeof(t1elx_entity_t) == 16) ? 1 : -1
];
typedef char t1elx_owner_size_check[
	(sizeof(t1elx_owner_t) == 74) ? 1 : -1
];
typedef char t1elx_header_size_check[
	(sizeof(t1elx_header_t) == T1ELX_HEADER_SIZE) ? 1 : -1
];
typedef char t1elx_row_size_check[
	(sizeof(t1elx_row_t) == T1ELX_ROW_SIZE) ? 1 : -1
];

// Every state byte belongs to the trailing profile tail. No static initializer
// is used so this module contributes no initialized DGROUP data.
static t1elx_state_t far t1elx_state;
static bool16 far t1elx_natural_armed;
static bool16 far t1elx_direct_armed;
static uint8_t far t1elx_visible_page;
static uint8_t far t1elx_accessed_page;

static void t1elx_memclear(void far *buf, unsigned size)
{
	uint8_t far *p = reinterpret_cast<uint8_t far *>(buf);

	while(size != 0) {
		*p++ = 0;
		size--;
	}
}

static uint32_t t1elx_hash(uint32_t hash, const void far *buf, unsigned size)
{
	const uint8_t far *p = reinterpret_cast<const uint8_t far *>(buf);

	while(size != 0) {
		hash ^= *p++;
		hash *= 0x01000193UL;
		size--;
	}
	return hash;
}

static uint16_t t1elx_plane_segment(uint8_t plane)
{
	switch(plane) {
	case 0: return SEG_PLANE_B;
	case 1: return SEG_PLANE_R;
	case 2: return SEG_PLANE_G;
	default: return SEG_PLANE_E;
	}
}

static void t1elx_entity_capture(
	t1elx_entity_t& out, const CBossEntity& entity
)
{
	out.left = entity.cur_left;
	out.top = entity.cur_top;
	out.prev_left = entity.prev_left;
	out.prev_top = entity.prev_top;
	out.prev_delta_x = entity.prev_delta_x;
	out.prev_delta_y = entity.prev_delta_y;
	out.image = static_cast<uint8_t>(entity.image());
	out.hitbox_inactive = static_cast<uint8_t>(entity.hitbox_orb_inactive);
	out.lock_frame = entity.lock_frame;
}

static void t1elx_owner_capture(
	t1elx_owner_t& out,
	int boss_id_in, int boss_phase_in, int boss_phase_frame_in,
	int boss_hp_in, int pattern_state_in, int form_in,
	int hit_invincibility_frame_in, bool16 hit_invincible_in,
	int phase_pattern_in, bool16 teleport_done_in,
	int bat_velocity_x_in, int bat_velocity_y_in,
	bool16 initial_hp_rendered_in, const CBossEntity& still_or_wave,
	const CBossEntity& attack, const CBossEntity& bat
)
{
	out.boss_id = boss_id_in;
	out.boss_phase = boss_phase_in;
	out.boss_phase_frame = boss_phase_frame_in;
	out.boss_hp = boss_hp_in;
	out.pattern_state = pattern_state_in;
	out.form = form_in;
	out.hit_invincibility_frame = hit_invincibility_frame_in;
	out.hit_invincible = hit_invincible_in;
	out.phase_pattern = phase_pattern_in;
	out.teleport_done = teleport_done_in;
	out.bat_velocity_x = bat_velocity_x_in;
	out.bat_velocity_y = bat_velocity_y_in;
	out.initial_hp_rendered = initial_hp_rendered_in;
	t1elx_entity_capture(out.entity[0], still_or_wave);
	t1elx_entity_capture(out.entity[1], attack);
	t1elx_entity_capture(out.entity[2], bat);
}

static bool t1elx_first_owner_valid(const t1elx_owner_t& owner)
{
	return (
		(owner.boss_id == T1ELX_BID_ELIS) &&
		(owner.boss_phase == 1) && (owner.boss_phase_frame == 0) &&
		(owner.boss_hp == 14) && (owner.pattern_state == 0) &&
		(owner.form == 0) && (owner.hit_invincibility_frame == 0) &&
		!owner.hit_invincible && (owner.phase_pattern == 1) &&
		!owner.teleport_done && (owner.bat_velocity_x == 0) &&
		(owner.bat_velocity_y == 0) && !owner.initial_hp_rendered
	);
}

static void t1elx_filename_init(char *filename)
{
	filename[0] = 'T'; filename[1] = '1'; filename[2] = 'E';
	filename[3] = 'L'; filename[4] = 'X'; filename[5] = '.';
	filename[6] = 'B'; filename[7] = 'I'; filename[8] = 'N';
	filename[9] = '\0';
}

static FILE *t1elx_open(bool append)
{
	char filename[10];
	char mode[3];

	t1elx_filename_init(filename);
	mode[0] = append ? 'a' : 'w';
	mode[1] = 'b';
	mode[2] = '\0';
	return fopen(filename, mode);
}

static bool t1elx_header_write(void)
{
	t1elx_header_t header;
	FILE *file;
	bool ok;

	t1elx_memclear(&header, sizeof(header));
	header.magic[0] = 'T'; header.magic[1] = '1';
	header.magic[2] = 'E'; header.magic[3] = 'L';
	header.magic[4] = 'X'; header.magic[5] = '1';
	header.version = 1;
	header.header_size = T1ELX_HEADER_SIZE;
	header.row_size = T1ELX_ROW_SIZE;
	header.expected_rows = T1ELX_ROW_COUNT;
	header.run_kind = T1ELX_NATURAL_TRACE
		? T1ELX_RUN_NATURAL : T1ELX_RUN_DIRECT;
	header.target = T1ELX_BID_ELIS;
	header.stage_id = ((2 * STAGES_PER_SCENE) + BOSS_STAGE);
	header.route = ROUTE_MAKAI;
	header.pages = PAGE_COUNT;
	header.planes = PLANE_COUNT;
	header.tram_rows = T1ELX_TRAM_ROWS;
	header.visible_page = t1elx_visible_page;
	header.owner_size = sizeof(t1elx_owner_t);
	file = t1elx_open(false);
	if(!file) {
		return false;
	}
	ok = (fwrite(&header, 1, sizeof(header), file) == sizeof(header));
	if(fclose(file) != 0) {
		ok = false;
	}
	return ok;
}

static void t1elx_surface_capture(t1elx_row_t& row)
{
	uint8_t page;
	uint8_t plane;
	uint8_t tram_row;
	const uint8_t accessed_before = t1elx_accessed_page;

	row.visible_page = t1elx_visible_page;
	row.accessed_page = accessed_before;
	row.palette_digest = t1elx_hash(
		T1REPLAY_FNV1A_BASIS, &z_Palettes, sizeof(z_Palettes)
	);
	for(page = 0; page < PAGE_COUNT; page++) {
		graph_accesspage_func(page);
		for(plane = 0; plane < PLANE_COUNT; plane++) {
			row.vram_digest[page][plane] = t1elx_hash(
				T1REPLAY_FNV1A_BASIS,
				MK_FP(t1elx_plane_segment(plane), 0), PLANE_SIZE
			);
		}
	}
	graph_accesspage_func(accessed_before);
	for(tram_row = 0; tram_row < T1ELX_TRAM_ROWS; tram_row++) {
		row.tram_jis_digest[tram_row] = t1elx_hash(
			T1REPLAY_FNV1A_BASIS,
			MK_FP(SEG_TRAM_JIS, (tram_row * T1ELX_TRAM_ROW_BYTES)),
			T1ELX_TRAM_ROW_BYTES
		);
		row.tram_attr_digest[tram_row] = t1elx_hash(
			T1REPLAY_FNV1A_BASIS,
			MK_FP(SEG_TRAM_ATRB, (tram_row * T1ELX_TRAM_ROW_BYTES)),
			T1ELX_TRAM_ROW_BYTES
		);
	}
}

static bool t1elx_row_write(uint8_t point, const t1elx_owner_t& owner)
{
	t1elx_row_t row;
	FILE *file;
	bool ok;

	t1elx_memclear(&row, sizeof(row));
	row.point = point;
	row.random_seed = static_cast<uint32_t>(random_seed);
	row.frame_rand = frame_rand;
	row.owner_digest = t1elx_hash(
		T1REPLAY_FNV1A_BASIS, &owner, sizeof(owner)
	);
	t1elx_surface_capture(row);
	row.checksum = t1elx_hash(
		T1REPLAY_FNV1A_BASIS, &row, (sizeof(row) - sizeof(row.checksum))
	);
	file = t1elx_open(true);
	if(!file) {
		return false;
	}
	ok = (fwrite(&row, 1, sizeof(row), file) == sizeof(row));
	if(fclose(file) != 0) {
		ok = false;
	}
	return ok;
}

void t1elx_trace_reset(void)
{
	t1elx_state = T1ELXS_OFF;
	t1elx_natural_armed = false;
	t1elx_direct_armed = false;
	t1elx_visible_page = 0xFF;
	t1elx_accessed_page = 0xFF;
}

void t1elx_natural_prepare(void)
{
#if T1ELX_NATURAL_TRACE
	if(t1elx_state == T1ELXS_OFF) {
		t1elx_natural_armed = true;
	}
#endif
}

bool16 t1elx_direct_prepare(void)
{
#if T1ELX_DIRECT_TRACE
	if((t1elx_state == T1ELXS_OFF) && !t1elx_direct_armed) {
		t1elx_direct_armed = true;
		return true;
	}
#endif
	return false;
}

void t1elx_pre_input(
	int boss_id_in, int boss_phase_in, int boss_phase_frame_in,
	int boss_hp_in, int pattern_state_in, int form_in,
	int hit_invincibility_frame_in, bool16 hit_invincible_in,
	int phase_pattern_in, bool16 teleport_done_in, int bat_velocity_x_in,
	int bat_velocity_y_in, bool16 initial_hp_rendered_in,
	const CBossEntity& still_or_wave, const CBossEntity& attack,
	const CBossEntity& bat
)
{
	t1elx_owner_t owner;
	uint8_t point;

	t1elx_owner_capture(
		owner, boss_id_in, boss_phase_in, boss_phase_frame_in, boss_hp_in,
		pattern_state_in, form_in, hit_invincibility_frame_in,
		hit_invincible_in, phase_pattern_in, teleport_done_in,
		bat_velocity_x_in, bat_velocity_y_in, initial_hp_rendered_in,
		still_or_wave, attack, bat
	);
	if(t1elx_state == T1ELXS_OFF) {
		if(
			(!t1elx_natural_armed && !t1elx_direct_armed) ||
			!t1elx_first_owner_valid(owner) ||
			(t1elx_visible_page >= PAGE_COUNT) ||
			(t1elx_accessed_page >= PAGE_COUNT) ||
			!t1elx_header_write()
		) {
			if(t1elx_natural_armed || t1elx_direct_armed) {
				t1elx_state = T1ELXS_FAILED;
			}
			return;
		}
		t1elx_state = T1ELXS_WAIT_FIRST;
	}
	if(t1elx_state == T1ELXS_WAIT_FIRST) {
		point = T1ELX_FIRST_PRE_INPUT;
		t1elx_state = T1ELXS_WAIT_SECOND;
	} else if(t1elx_state == T1ELXS_WAIT_SECOND) {
		point = T1ELX_SECOND_PRE_INPUT;
		t1elx_state = T1ELXS_DONE;
	} else {
		return;
	}
	if(!t1elx_row_write(point, owner)) {
		t1elx_state = T1ELXS_FAILED;
	}
}

void t1elx_visible_page_set(page_t page)
{
	t1elx_visible_page = (page < PAGE_COUNT) ? page : 0xFF;
}

void t1elx_accessed_page_set(page_t page)
{
	t1elx_accessed_page = (page < PAGE_COUNT) ? page : 0xFF;
}

#endif
