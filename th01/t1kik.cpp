// Private Kikuri phase-2 natural/direct acceptance marker. This own tail
// object is nonempty only in its private profiles and adds no release data.

#pragma option -zCT1KIK_TEXT -G-
#pragma codeseg T1KIK_TEXT

#include "th01/t1kik.hpp"

#if T1KIK_TRACE

#include <stdio.h>
#include "pc98.h"
#include "x86real.h"
#include "th01/hardware/palette.h"
#include "th01/main/boss/boss.hpp"
#include "th01/main/boss/b15j.hpp"
#include "th01/replay.hpp"

enum {
	T1KIK_HEADER_SIZE = 32,
	T1KIK_ROW_SIZE = 556,
	T1KIK_ROW_COUNT = 3,
	T1KIK_TARGET = BID_KIKURI,
	T1KIK_POINT_PRE_INPUT_FIRST = 1,
	T1KIK_POINT_POST_INPUT_FIRST = 2,
	T1KIK_POINT_PRE_INPUT_SECOND = 3,
	T1KIK_RUN_NATURAL = 1,
	T1KIK_RUN_DIRECT = 2,
	T1KIK_TRAM_ROWS = (RES_Y / GLYPH_H),
	T1KIK_TRAM_ROW_BYTES = ((RES_X / GLYPH_HALF_W) * 2),
};

#if T1KIK_DIRECT_TRACE
#define T1KIK_RUN_KIND T1KIK_RUN_DIRECT
#else
#define T1KIK_RUN_KIND T1KIK_RUN_NATURAL
#endif

enum t1kik_state_t {
	T1KIKS_OFF,
	T1KIKS_WAIT_POST_INPUT_FIRST,
	T1KIKS_WAIT_PRE_INPUT_SECOND,
	T1KIKS_DONE,
	T1KIKS_FAILED,
};

struct t1kik_header_t {
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
	uint32_t provenance_digest;
};

struct t1kik_row_t {
	uint8_t point;
	uint8_t process_seq;
	uint8_t visible_page;
	uint8_t accessed_page;
	t1boss_kikuri_checkpoint_t owner;
	uint32_t owner_digest;
	t1replay_checkpoint_input_t input;
	uint32_t input_digest;
	uint32_t frame_rand;
	uint32_t random_seed;
	uint32_t palette_digest;
	uint32_t vram_digest[PAGE_COUNT][PLANE_COUNT];
	uint32_t tram_jis_digest[T1KIK_TRAM_ROWS];
	uint32_t tram_attr_digest[T1KIK_TRAM_ROWS];
	uint32_t checksum;
};

typedef char t1kik_header_size_check[
	(sizeof(t1kik_header_t) == T1KIK_HEADER_SIZE) ? 1 : -1
];
typedef char t1kik_row_size_check[
	(sizeof(t1kik_row_t) == T1KIK_ROW_SIZE) ? 1 : -1
];

static t1kik_state_t far t1kik_state;
static uint32_t far t1kik_expected_owner_digest;
static uint8_t far t1kik_visible_page;
static uint8_t far t1kik_accessed_page;
static bool16 far t1kik_visible_seen;
static bool16 far t1kik_accessed_seen;
#if T1KIK_NATURAL_TRACE
static bool16 far t1kik_natural_armed;
#endif
#if T1KIK_DIRECT_TRACE
static bool16 far t1kik_direct_armed;
#endif

extern unsigned long frame_rand;
extern long random_seed;
extern int8_t boss_id;

static void t1kik_memclear(void far *buf, unsigned size)
{
	uint8_t far *p = reinterpret_cast<uint8_t far *>(buf);

	while(size != 0) {
		*p++ = 0;
		size--;
	}
}

static uint32_t t1kik_hash(uint32_t hash, const void far *buf, unsigned size)
{
	const uint8_t far *p = reinterpret_cast<const uint8_t far *>(buf);

	while(size != 0) {
		hash ^= *p++;
		hash *= 0x01000193UL;
		size--;
	}
	return hash;
}

static uint16_t t1kik_plane_segment(uint8_t plane)
{
	switch(plane) {
	case 0: return SEG_PLANE_B;
	case 1: return SEG_PLANE_R;
	case 2: return SEG_PLANE_G;
	default: return SEG_PLANE_E;
	}
}

static bool16 t1kik_surface_capture(t1kik_row_t *row)
{
	page_t accessed_before;
	uint8_t page;
	uint8_t plane;
	uint8_t tram_row;

	if(!row ||
		(t1kik_visible_page_get() >= PAGE_COUNT) ||
		(t1kik_accessed_page_get() >= PAGE_COUNT)) {
		return false;
	}
	row->visible_page = t1kik_visible_page_get();
	accessed_before = t1kik_accessed_page_get();
	row->accessed_page = accessed_before;
	row->palette_digest = t1kik_hash(
		T1REPLAY_FNV1A_BASIS, &z_Palettes, sizeof(z_Palettes)
	);
	for(page = 0; page < PAGE_COUNT; page++) {
		graph_accesspage_func(page);
		for(plane = 0; plane < PLANE_COUNT; plane++) {
			row->vram_digest[page][plane] = t1kik_hash(
				T1REPLAY_FNV1A_BASIS,
				MK_FP(t1kik_plane_segment(plane), 0), PLANE_SIZE
			);
		}
	}
	graph_accesspage_func(accessed_before);
	for(tram_row = 0; tram_row < T1KIK_TRAM_ROWS; tram_row++) {
		row->tram_jis_digest[tram_row] = t1kik_hash(
			T1REPLAY_FNV1A_BASIS,
			MK_FP(SEG_TRAM_JIS, (tram_row * T1KIK_TRAM_ROW_BYTES)),
			T1KIK_TRAM_ROW_BYTES
		);
		row->tram_attr_digest[tram_row] = t1kik_hash(
			T1REPLAY_FNV1A_BASIS,
			MK_FP(SEG_TRAM_ATRB, (tram_row * T1KIK_TRAM_ROW_BYTES)),
			T1KIK_TRAM_ROW_BYTES
		);
	}
	return true;
}

static bool16 t1kik_header_write(void)
{
	t1kik_header_t header;
	FILE *file;
	bool ok;
	char filename[10];
	char mode[3];

	t1kik_memclear(&header, sizeof(header));
	header.magic[0] = 'T'; header.magic[1] = '1';
	header.magic[2] = 'K'; header.magic[3] = 'I';
	header.magic[4] = 'K'; header.magic[5] = '1';
	header.version = 2;
	header.header_size = T1KIK_HEADER_SIZE;
	header.row_size = T1KIK_ROW_SIZE;
	header.expected_rows = T1KIK_ROW_COUNT;
	header.run_kind = T1KIK_RUN_KIND;
	header.target = T1KIK_TARGET;
	header.pages = PAGE_COUNT;
	header.planes = PLANE_COUNT;
	header.tram_rows = T1KIK_TRAM_ROWS;
	header.visible_page = t1kik_visible_page_get();
	header.accessed_page = t1kik_accessed_page_get();
	header.owner_size = T1BOSS_KIKURI_CHECKPOINT_SIZE;
	header.expected_owner_digest = t1kik_expected_owner_digest;
	header.provenance_digest = t1kik_hash(
		T1REPLAY_FNV1A_BASIS, &header,
		(sizeof(header) - sizeof(header.provenance_digest))
	);
	filename[0] = 'T'; filename[1] = '1'; filename[2] = 'K';
	filename[3] = 'I'; filename[4] = 'K'; filename[5] = '.';
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

static bool16 t1kik_row_write(uint8_t point, uint8_t process_seq)
{
	t1kik_row_t row;
	FILE *file;
	bool ok;
	char filename[10];
	char mode[3];

	t1kik_memclear(&row, sizeof(row));
	if(!t1boss_kikuri_checkpoint_capture(&row.owner)) {
		return false;
	}
	row.point = point;
	row.process_seq = process_seq;
	row.owner_digest = t1kik_hash(
		T1REPLAY_FNV1A_BASIS, &row.owner, sizeof(row.owner)
	);
	// The post-input witness still describes the first ordinary frame. The
	// following pre-input witness deliberately observes one native update, so
	// its owner digest must be allowed to advance.
	if(
		(point != T1KIK_POINT_PRE_INPUT_SECOND) &&
		(row.owner_digest != t1kik_expected_owner_digest)
	) {
		return false;
	}
	t1replay_input_checkpoint_export(&row.input);
	row.input_digest = t1kik_hash(
		T1REPLAY_FNV1A_BASIS, &row.input, sizeof(row.input)
	);
	row.frame_rand = frame_rand;
	row.random_seed = random_seed;
	if(!t1kik_surface_capture(&row)) {
		return false;
	}
	row.checksum = t1kik_hash(
		T1REPLAY_FNV1A_BASIS, &row, (sizeof(row) - sizeof(row.checksum))
	);
	filename[0] = 'T'; filename[1] = '1'; filename[2] = 'K';
	filename[3] = 'I'; filename[4] = 'K'; filename[5] = '.';
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

static bool16 t1kik_arm_if_first_combat(uint8_t process_seq)
{
	t1boss_kikuri_checkpoint_t expected;
	t1boss_kikuri_checkpoint_t actual;

	if(
		(process_seq != 0) ||
		(boss_id != BID_KIKURI) ||
		!t1kik_visible_seen ||
		!t1kik_accessed_seen ||
		!t1boss_kikuri_first_combat_construct(&expected) ||
		!t1boss_kikuri_checkpoint_capture(&actual) ||
		(actual.phase != 2) || (actual.phase_frame != 0)
	) {
		return false;
	}
#if T1KIK_DIRECT_TRACE
	if(!t1kik_direct_armed) {
		return false;
	}
#elif T1KIK_NATURAL_TRACE
	if(!t1kik_natural_armed) {
		return false;
	}
#endif
	t1kik_state = T1KIKS_FAILED;
	t1kik_expected_owner_digest = t1kik_hash(
		T1REPLAY_FNV1A_BASIS, &expected, sizeof(expected)
	);
	if(
		(t1kik_expected_owner_digest != t1kik_hash(
			T1REPLAY_FNV1A_BASIS, &actual, sizeof(actual)
		)) ||
		!t1kik_header_write()
	) {
		return false;
	}
	return true;
}

void t1kik_trace_reset(void)
{
	t1kik_state = T1KIKS_OFF;
	t1kik_expected_owner_digest = 0;
	t1kik_visible_page = 0xFF;
	t1kik_accessed_page = 0xFF;
	t1kik_visible_seen = false;
	t1kik_accessed_seen = false;
#if T1KIK_NATURAL_TRACE
	t1kik_natural_armed = false;
#endif
#if T1KIK_DIRECT_TRACE
	t1kik_direct_armed = false;
#endif
}

void t1kik_pre_input(uint8_t process_seq)
{
	if(t1kik_state == T1KIKS_OFF) {
		if(!t1kik_arm_if_first_combat(process_seq) ||
			!t1kik_row_write(T1KIK_POINT_PRE_INPUT_FIRST, process_seq)) {
			t1kik_state = T1KIKS_FAILED;
			return;
		}
		t1kik_state = T1KIKS_WAIT_POST_INPUT_FIRST;
		return;
	}
	if(t1kik_state == T1KIKS_WAIT_PRE_INPUT_SECOND) {
		if(!t1kik_row_write(T1KIK_POINT_PRE_INPUT_SECOND, process_seq)) {
			t1kik_state = T1KIKS_FAILED;
			return;
		}
		t1kik_state = T1KIKS_DONE;
	}
}

void t1kik_post_input(uint8_t process_seq)
{
	if(t1kik_state != T1KIKS_WAIT_POST_INPUT_FIRST) {
		return;
	}
	if(!t1kik_row_write(T1KIK_POINT_POST_INPUT_FIRST, process_seq)) {
		t1kik_state = T1KIKS_FAILED;
		return;
	}
	t1kik_state = T1KIKS_WAIT_PRE_INPUT_SECOND;
}

#if T1KIK_NATURAL_TRACE
bool16 t1kik_natural_prepare(void)
{
	if((t1kik_state != T1KIKS_OFF) || t1kik_natural_armed) {
		return false;
	}
	t1kik_natural_armed = true;
	return true;
}
#endif

#if T1KIK_DIRECT_TRACE
bool16 t1kik_direct_prepare(void)
{
	if((t1kik_state != T1KIKS_OFF) || t1kik_direct_armed) {
		return false;
	}
	t1kik_direct_armed = true;
	return true;
}
#endif

void t1kik_visible_page_set(page_t page)
{
	t1kik_visible_page = ((page < PAGE_COUNT) ? page : 0xFF);
	t1kik_visible_seen = true;
}

uint8_t t1kik_visible_page_get(void)
{
	return t1kik_visible_page;
}

void t1kik_accessed_page_set(page_t page)
{
	t1kik_accessed_page = ((page < PAGE_COUNT) ? page : 0xFF);
	t1kik_accessed_seen = true;
}

uint8_t t1kik_accessed_page_get(void)
{
	return t1kik_accessed_page;
}

#endif

#pragma codeseg
