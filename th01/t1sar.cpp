// Private Sariel first-combat natural/direct acceptance witness. This tail is
// empty in release builds and never provides a public Practice target.

#pragma option -zCT1SAR_TEXT -G-
#pragma codeseg T1SAR_TEXT

#include "th01/t1sar.hpp"

#if T1SAR_TRACE

#include <stdio.h>
#include "pc98.h"
#include "x86real.h"
#include "th01/hardware/palette.h"
#include "th01/main/boss/b20m.hpp"
#include "th01/main/boss/entity_a.hpp"
#include "th01/replay.hpp"

enum {
	T1SAR_HEADER_SIZE = 32,
	T1SAR_ROW_SIZE = 344,
	T1SAR_TARGET = 6,
	T1SAR_ROW_COUNT = 2,
	T1SAR_POINT_FIRST_PRE_INPUT = 1,
	T1SAR_POINT_SECOND_PRE_INPUT = 2,
	T1SAR_TRAM_ROWS = (RES_Y / GLYPH_H),
	T1SAR_TRAM_ROW_BYTES = ((RES_X / GLYPH_HALF_W) * 2),
	T1SAR_RUN_NATURAL = 1,
	T1SAR_RUN_DIRECT = 2,
};

#if T1SAR_DIRECT_TRACE
#define T1SAR_RUN_KIND T1SAR_RUN_DIRECT
#else
#define T1SAR_RUN_KIND T1SAR_RUN_NATURAL
#endif

enum t1sar_state_t {
	T1SARS_OFF,
	T1SARS_WAIT_FIRST,
	T1SARS_WAIT_SECOND,
	T1SARS_DONE,
	T1SARS_FAILED,
};

struct t1sar_entity_t {
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

struct t1sar_anim_t {
	int16_t left;
	int16_t top;
	uint8_t image;
	uint8_t reserved;
};

struct t1sar_owner_t {
	int16_t boss_phase;
	int16_t boss_phase_frame;
	int16_t boss_hp;
	int16_t hud_hp_first_white;
	int16_t hud_hp_first_redwhite;
	int16_t pattern_state;
	int16_t invincibility_frame;
	int16_t invincible;
	int16_t phase_pattern;
	int16_t patterns_done;
	int16_t patterns_until_next;
	int16_t initial_hp_rendered;
	t1sar_entity_t shield;
	t1sar_anim_t dress;
	t1sar_anim_t wand;
};

struct t1sar_header_t {
	char magic[8];
	uint16_t version;
	uint16_t header_size;
	uint16_t row_size;
	uint8_t expected_rows;
	uint8_t run_kind;
	uint8_t target;
	uint8_t pages;
	uint8_t planes;
	uint8_t tram_rows;
	uint8_t visible_page;
	uint8_t accessed_page;
	uint16_t owner_size;
	uint32_t expected_owner_digest;
	uint32_t checksum;
};

struct t1sar_row_t {
	uint8_t point;
	uint8_t process_seq;
	uint8_t visible_page;
	uint8_t accessed_page;
	t1sar_owner_t owner;
	uint32_t owner_digest;
	t1replay_checkpoint_input_t input;
	uint32_t input_digest;
	uint32_t frame_rand;
	uint32_t random_seed;
	uint32_t palette_digest;
	uint32_t vram_digest[PAGE_COUNT][PLANE_COUNT];
	uint32_t tram_jis_digest[T1SAR_TRAM_ROWS];
	uint32_t tram_attr_digest[T1SAR_TRAM_ROWS];
	uint32_t checksum;
};

typedef char t1sar_entity_size_check[
	(sizeof(t1sar_entity_t) == 16) ? 1 : -1
];
typedef char t1sar_anim_size_check[
	(sizeof(t1sar_anim_t) == 6) ? 1 : -1
];
typedef char t1sar_owner_size_check[
	(sizeof(t1sar_owner_t) == 52) ? 1 : -1
];
typedef char t1sar_header_size_check[
	(sizeof(t1sar_header_t) == T1SAR_HEADER_SIZE) ? 1 : -1
];
typedef char t1sar_row_size_check[
	(sizeof(t1sar_row_t) == T1SAR_ROW_SIZE) ? 1 : -1
];

static t1sar_state_t far t1sar_state;
static t1sar_owner_t far t1sar_owner;
static uint32_t far t1sar_expected_owner_digest;
static uint8_t far t1sar_visible_page;
static uint8_t far t1sar_accessed_page;
#if T1SAR_NATURAL_TRACE
static bool16 far t1sar_natural_armed;
#endif
#if T1SAR_DIRECT_TRACE
static bool16 far t1sar_direct_armed;
#endif

extern unsigned long frame_rand;
extern long random_seed;

static void t1sar_memclear(void far *buf, unsigned size)
{
	uint8_t far *p = reinterpret_cast<uint8_t far *>(buf);

	while(size != 0) {
		*p++ = 0;
		size--;
	}
}

static uint32_t t1sar_hash(uint32_t hash, const void far *buf, unsigned size)
{
	const uint8_t far *p = reinterpret_cast<const uint8_t far *>(buf);

	while(size != 0) {
		hash ^= *p++;
		hash *= 0x01000193UL;
		size--;
	}
	return hash;
}

static uint16_t t1sar_plane_segment(uint8_t plane)
{
	if(plane == 0) {
		return SEG_PLANE_B;
	} else if(plane == 1) {
		return SEG_PLANE_R;
	} else if(plane == 2) {
		return SEG_PLANE_G;
	}
	return SEG_PLANE_E;
}

static void t1sar_entity_capture(t1sar_entity_t *out, const CBossEntity& in)
{
	out->left = in.cur_left;
	out->top = in.cur_top;
	out->prev_left = in.prev_left;
	out->prev_top = in.prev_top;
	out->prev_delta_x = in.prev_delta_x;
	out->prev_delta_y = in.prev_delta_y;
	out->image = static_cast<uint8_t>(in.image());
	out->hitbox_inactive = static_cast<uint8_t>(in.hitbox_orb_inactive);
	out->lock_frame = in.lock_frame;
}

static void t1sar_anim_capture(t1sar_anim_t *out, const CBossAnim& in)
{
	out->left = in.left;
	out->top = in.top;
	out->image = in.bos_image;
	out->reserved = 0;
}

static bool16 t1sar_first_owner_valid(const t1sar_owner_t *owner)
{
	return (
		(owner->boss_phase == 1) && (owner->boss_phase_frame == 0) &&
		(owner->boss_hp == 18) && (owner->hud_hp_first_white == 8) &&
		(owner->hud_hp_first_redwhite == 2) &&
		(owner->pattern_state == 0) &&
		(owner->invincibility_frame == 0) && !owner->invincible &&
		(owner->phase_pattern == 0) && (owner->patterns_done == 0) &&
		(owner->patterns_until_next >= 1) &&
		(owner->patterns_until_next <= 6) && !owner->initial_hp_rendered
	);
}

static bool16 t1sar_header_write(void)
{
	t1sar_header_t header;
	FILE *file;
	bool ok;
	char filename[10];
	char mode[3];

	t1sar_memclear(&header, sizeof(header));
	header.magic[0] = 'T'; header.magic[1] = '1';
	header.magic[2] = 'S'; header.magic[3] = 'A';
	header.magic[4] = 'R'; header.magic[5] = '1';
	header.version = 1;
	header.header_size = T1SAR_HEADER_SIZE;
	header.row_size = T1SAR_ROW_SIZE;
	header.expected_rows = T1SAR_ROW_COUNT;
	header.run_kind = T1SAR_RUN_KIND;
	header.target = T1SAR_TARGET;
	header.pages = PAGE_COUNT;
	header.planes = PLANE_COUNT;
	header.tram_rows = T1SAR_TRAM_ROWS;
	header.visible_page = t1sar_visible_page;
	header.accessed_page = t1sar_accessed_page;
	header.owner_size = sizeof(t1sar_owner_t);
	header.expected_owner_digest = t1sar_expected_owner_digest;
	header.checksum = t1sar_hash(
		T1REPLAY_FNV1A_BASIS, &header,
		(sizeof(header) - sizeof(header.checksum))
	);
	filename[0] = 'T'; filename[1] = '1'; filename[2] = 'S';
	filename[3] = 'A'; filename[4] = 'R'; filename[5] = '.';
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

static bool16 t1sar_surface_capture(t1sar_row_t *row)
{
	page_t accessed_before;
	uint8_t page;
	uint8_t plane;
	uint8_t tram_row;

	if(
		!row || (t1sar_visible_page >= PAGE_COUNT) ||
		(t1sar_accessed_page >= PAGE_COUNT)
	) {
		return false;
	}
	row->visible_page = t1sar_visible_page;
	accessed_before = t1sar_accessed_page;
	row->accessed_page = accessed_before;
	row->palette_digest = t1sar_hash(
		T1REPLAY_FNV1A_BASIS, &z_Palettes, sizeof(z_Palettes)
	);
	for(page = 0; page < PAGE_COUNT; page++) {
		graph_accesspage_func(page);
		for(plane = 0; plane < PLANE_COUNT; plane++) {
			row->vram_digest[page][plane] = t1sar_hash(
				T1REPLAY_FNV1A_BASIS,
				MK_FP(t1sar_plane_segment(plane), 0), PLANE_SIZE
			);
		}
	}
	graph_accesspage_func(accessed_before);
	for(tram_row = 0; tram_row < T1SAR_TRAM_ROWS; tram_row++) {
		row->tram_jis_digest[tram_row] = t1sar_hash(
			T1REPLAY_FNV1A_BASIS,
			MK_FP(SEG_TRAM_JIS, (tram_row * T1SAR_TRAM_ROW_BYTES)),
			T1SAR_TRAM_ROW_BYTES
		);
		row->tram_attr_digest[tram_row] = t1sar_hash(
			T1REPLAY_FNV1A_BASIS,
			MK_FP(SEG_TRAM_ATRB, (tram_row * T1SAR_TRAM_ROW_BYTES)),
			T1SAR_TRAM_ROW_BYTES
		);
	}
	return true;
}

static bool16 t1sar_row_write(uint8_t point, uint8_t process_seq)
{
	t1sar_row_t row;
	FILE *file;
	bool ok;
	char filename[10];
	char mode[3];

	t1sar_memclear(&row, sizeof(row));
	row.point = point;
	row.process_seq = process_seq;
	row.owner = t1sar_owner;
	row.owner_digest = t1sar_hash(
		T1REPLAY_FNV1A_BASIS, &row.owner, sizeof(row.owner)
	);
	if((point == T1SAR_POINT_FIRST_PRE_INPUT) &&
		(row.owner_digest != t1sar_expected_owner_digest)) {
		return false;
	}
	t1replay_input_checkpoint_export(&row.input);
	row.input_digest = t1sar_hash(
		T1REPLAY_FNV1A_BASIS, &row.input, sizeof(row.input)
	);
	row.frame_rand = frame_rand;
	row.random_seed = random_seed;
	if(!t1sar_surface_capture(&row)) {
		return false;
	}
	row.checksum = t1sar_hash(
		T1REPLAY_FNV1A_BASIS, &row,
		(sizeof(row) - sizeof(row.checksum))
	);
	filename[0] = 'T'; filename[1] = '1'; filename[2] = 'S';
	filename[3] = 'A'; filename[4] = 'R'; filename[5] = '.';
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

void t1sar_trace_reset(void)
{
	t1sar_state = T1SARS_OFF;
	t1sar_memclear(&t1sar_owner, sizeof(t1sar_owner));
	t1sar_expected_owner_digest = 0;
	t1sar_visible_page = 0xFF;
	t1sar_accessed_page = 0xFF;
#if T1SAR_NATURAL_TRACE
	t1sar_natural_armed = false;
#endif
#if T1SAR_DIRECT_TRACE
	t1sar_direct_armed = false;
#endif
}

void t1sar_natural_prepare(void)
{
#if T1SAR_NATURAL_TRACE
	if(t1sar_state == T1SARS_OFF) {
		t1sar_natural_armed = true;
	}
#endif
}

bool16 t1sar_direct_prepare(void)
{
#if T1SAR_DIRECT_TRACE
	if((t1sar_state == T1SARS_OFF) && !t1sar_direct_armed) {
		t1sar_direct_armed = true;
		return true;
	}
#endif
	return false;
}

bool16 t1sar_direct_ready(void)
{
	return (
		(t1sar_state == T1SARS_WAIT_FIRST) &&
		(t1sar_expected_owner_digest != 0)
	);
}

void t1sar_visible_page_set(page_t page)
{
	t1sar_visible_page = (page < PAGE_COUNT) ? page : 0xFF;
}

void t1sar_accessed_page_set(page_t page)
{
	t1sar_accessed_page = (page < PAGE_COUNT) ? page : 0xFF;
}

void t1sar_owner_set(
	int boss_phase,
	int boss_phase_frame,
	int boss_hp,
	int hud_hp_first_white,
	int hud_hp_first_redwhite,
	int pattern_state,
	int invincibility_frame,
	bool16 invincible,
	int phase_pattern,
	int patterns_done,
	int patterns_until_next,
	bool16 initial_hp_rendered,
	const CBossEntity& shield,
	const CBossAnim& dress,
	const CBossAnim& wand
)
{
	t1sar_owner_t owner;
	bool16 armed = false;

	t1sar_memclear(&owner, sizeof(owner));
	owner.boss_phase = boss_phase;
	owner.boss_phase_frame = boss_phase_frame;
	owner.boss_hp = boss_hp;
	owner.hud_hp_first_white = hud_hp_first_white;
	owner.hud_hp_first_redwhite = hud_hp_first_redwhite;
	owner.pattern_state = pattern_state;
	owner.invincibility_frame = invincibility_frame;
	owner.invincible = invincible;
	owner.phase_pattern = phase_pattern;
	owner.patterns_done = patterns_done;
	owner.patterns_until_next = patterns_until_next;
	owner.initial_hp_rendered = initial_hp_rendered;
	t1sar_entity_capture(&owner.shield, shield);
	t1sar_anim_capture(&owner.dress, dress);
	t1sar_anim_capture(&owner.wand, wand);

#if T1SAR_NATURAL_TRACE
	armed = t1sar_natural_armed;
#endif
#if T1SAR_DIRECT_TRACE
	armed = t1sar_direct_armed;
#endif
	if(t1sar_state == T1SARS_OFF) {
		if(!armed) {
			return;
		}
		if(
			!t1sar_first_owner_valid(&owner) ||
			(t1sar_visible_page >= PAGE_COUNT) ||
			(t1sar_accessed_page >= PAGE_COUNT)
		) {
			t1sar_state = T1SARS_FAILED;
			return;
		}
		t1sar_owner = owner;
		t1sar_expected_owner_digest = t1sar_hash(
			T1REPLAY_FNV1A_BASIS, &owner, sizeof(owner)
		);
		if(!t1sar_header_write()) {
			t1sar_state = T1SARS_FAILED;
			return;
		}
		t1sar_state = T1SARS_WAIT_FIRST;
		return;
	}
	if((t1sar_state == T1SARS_WAIT_FIRST) ||
		(t1sar_state == T1SARS_WAIT_SECOND)) {
		t1sar_owner = owner;
	}
}

void t1sar_pre_input(uint8_t process_seq)
{
	uint8_t point;

	if(t1sar_state == T1SARS_WAIT_FIRST) {
		point = T1SAR_POINT_FIRST_PRE_INPUT;
		t1sar_state = T1SARS_WAIT_SECOND;
	} else if(t1sar_state == T1SARS_WAIT_SECOND) {
		point = T1SAR_POINT_SECOND_PRE_INPUT;
		t1sar_state = T1SARS_DONE;
	} else {
		return;
	}
	if(!t1sar_row_write(point, process_seq)) {
		t1sar_state = T1SARS_FAILED;
	}
}

#endif
