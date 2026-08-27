// Private, measurement-only SinGyoku presentation probe. T1PIX.BIN is not a
// user replay sidecar and is compiled out of every public profile.

#pragma option -zCT1PIXEL_TEXT -G-

#include <stdio.h>
#include "platform.h"
#include "pc98.h"
#include "x86real.h"
#include "th01/hardware/graph.h"
#include "th01/hardware/palette.h"
#include "th01/main/boss/boss.hpp"
#include "th01/rpypixel.hpp"

#if T1REPLAY_PIXEL_TRACE

#define T1PIXEL_HEADER_SIZE 24
#define T1PIXEL_ROW_SIZE 260
#define T1PIXEL_ROW_COUNT 3
#define T1PIXEL_TRAM_ROWS (RES_Y / GLYPH_H)
#define T1PIXEL_TRAM_ROW_BYTES ((RES_X / GLYPH_HALF_W) * 2)

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

extern page_t page_accessed;
extern page_t page_shown;

static t1pixel_state_t t1pixel_state;
static uint32_t t1pixel_anchor_sample;
static uint32_t t1pixel_last_sample;

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
	page_t accessed_before = page_accessed;
	uint8_t page;
	uint8_t plane;
	uint8_t tram_row;
	FILE *file;
	bool ok;

	t1pixel_memclear(&row, sizeof(row));
	row.point = point;
	row.process_seq = process_seq;
	row.visible_page = page_shown;
	row.accessed_page = accessed_before;
	row.sample_cursor = sample_cursor;
	row.packet_cursor = packet_cursor;
	row.input_cursor = input_cursor;
	row.semantic_digest = semantic_digest;
	row.palette_digest = t1pixel_hash(
		T1REPLAY_FNV1A_BASIS, &z_Palettes, sizeof(z_Palettes)
	);
	for(page = 0; page < PAGE_COUNT; page++) {
		graph_accesspage_func(page);
		for(plane = 0; plane < PLANE_COUNT; plane++) {
			row.vram_digest[page][plane] = t1pixel_hash(
				T1REPLAY_FNV1A_BASIS,
				MK_FP(t1pixel_plane_segment(plane), 0), PLANE_SIZE
			);
		}
	}
	graph_accesspage_func(accessed_before);
	for(tram_row = 0; tram_row < T1PIXEL_TRAM_ROWS; tram_row++) {
		row.tram_jis_digest[tram_row] = t1pixel_hash(
			T1REPLAY_FNV1A_BASIS,
			MK_FP(SEG_TRAM_JIS, (tram_row * T1PIXEL_TRAM_ROW_BYTES)),
			T1PIXEL_TRAM_ROW_BYTES
		);
		row.tram_attr_digest[tram_row] = t1pixel_hash(
			T1REPLAY_FNV1A_BASIS,
			MK_FP(SEG_TRAM_ATRB, (tram_row * T1PIXEL_TRAM_ROW_BYTES)),
			T1PIXEL_TRAM_ROW_BYTES
		);
	}
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

void t1replay_pixel_probe_reset(void)
{
	t1pixel_state = T1PXS_OFF;
	t1pixel_anchor_sample = 0;
	t1pixel_last_sample = 0;
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
