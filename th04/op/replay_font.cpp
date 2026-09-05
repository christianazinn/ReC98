#pragma codeseg REPLAY_OP_TEXT

#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "planar.h"
#include "x86real.h"
#include "th04/op/replay_font.hpp"

#define REPLAY_OP_FONT_FILE_SIZE 3168
#define REPLAY_OP_FONT_HEADER_SIZE 32
#define REPLAY_OP_FONT_ADVANCES_OFFSET 32
#define REPLAY_OP_FONT_BITMAP_OFFSET 128
#define REPLAY_OP_FONT_GLYPH_COUNT 95
#define REPLAY_OP_FONT_GLYPH_SIZE 32
#define REPLAY_OP_FONT_FIRST 0x20
#define REPLAY_OP_FONT_LAST 0x7E
#define REPLAY_OP_FONT_FALLBACK 0x3F

replay_op_font_t replay_op_font;

extern "C" unsigned graph_VramSeg;
extern "C" unsigned graph_VramLines;

static uint16_t replay_op_font_u16(const uint8_t far *p)
{
	return (p[0] | (p[1] << 8));
}

static bool16 replay_op_font_validate(const uint8_t far *data)
{
	unsigned i;
	uint16_t checksum = 0;

	if(
		(data[0] != 'T') || (data[1] != ('0' + GAME)) ||
		(data[2] != 'P') || (data[3] != 'F') ||
		(data[4] != 'N') || (data[5] != 'T') ||
		(data[6] != '1') || (data[7] != 0) ||
		(replay_op_font_u16(&data[8]) != REPLAY_OP_FONT_FILE_SIZE) ||
		(replay_op_font_u16(&data[10]) != REPLAY_OP_FONT_HEADER_SIZE) ||
		(replay_op_font_u16(&data[12]) != REPLAY_OP_FONT_ADVANCES_OFFSET) ||
		(replay_op_font_u16(&data[14]) != REPLAY_OP_FONT_BITMAP_OFFSET) ||
		(replay_op_font_u16(&data[16]) != REPLAY_OP_FONT_GLYPH_COUNT) ||
		(data[18] != REPLAY_OP_FONT_FIRST) ||
		(data[19] != REPLAY_OP_FONT_LAST) ||
		(data[20] != 16) || (data[21] != 16) ||
		(data[22] != REPLAY_OP_FONT_FALLBACK) || (data[23] != 0)
	) {
		return false;
	}
	for(i = 26; i < REPLAY_OP_FONT_HEADER_SIZE; i++) {
		if(data[i] != 0) {
			return false;
		}
	}
	for(i = 0; i < REPLAY_OP_FONT_GLYPH_COUNT; i++) {
		if(
			(data[REPLAY_OP_FONT_ADVANCES_OFFSET + i] == 0) ||
			(data[REPLAY_OP_FONT_ADVANCES_OFFSET + i] > 16)
		) {
			return false;
		}
	}
	if(data[127] != 0) {
		return false;
	}
	for(i = REPLAY_OP_FONT_HEADER_SIZE; i < REPLAY_OP_FONT_FILE_SIZE; i++) {
		checksum += data[i];
	}
	return (checksum == replay_op_font_u16(&data[24]));
}

bool16 pascal replay_op_font_load(const unsigned char far *restore_pf_fn)
{
	replay_op_font_t loaded = 0;
	bool16 valid = false;
	uint8_t extra;
	char archive_fn[12];
	char font_fn[11];

	archive_fn[0] = 'P'; archive_fn[1] = 'A'; archive_fn[2] = 'T';
	archive_fn[3] = 'C'; archive_fn[4] = 'H'; archive_fn[5] = '0';
	archive_fn[6] = ('0' + GAME); archive_fn[7] = '.';
	archive_fn[8] = 'D'; archive_fn[9] = 'A'; archive_fn[10] = 'T';
	archive_fn[11] = '\0';
	font_fn[0] = 'M'; font_fn[1] = 'N'; font_fn[2] = 'U';
	font_fn[3] = 'F'; font_fn[4] = 'O'; font_fn[5] = 'N';
	font_fn[6] = 'T'; font_fn[7] = '.'; font_fn[8] = 'P';
	font_fn[9] = 'F'; font_fn[10] = '\0';

	replay_op_font_free();
	pfend();
	pfstart(reinterpret_cast<const unsigned char *>(archive_fn));
	if(file_ropen(font_fn)) {
		loaded = reinterpret_cast<replay_op_font_t>(
			hmem_allocbyte(REPLAY_OP_FONT_FILE_SIZE)
		);
		if(
			loaded &&
			(file_read(loaded, REPLAY_OP_FONT_FILE_SIZE) ==
			 REPLAY_OP_FONT_FILE_SIZE) &&
			(file_read(&extra, 1) == 0)
		) {
			valid = replay_op_font_validate(
				reinterpret_cast<const uint8_t far *>(loaded)
			);
		}
		file_close();
	}
	pfend();
	pfstart(restore_pf_fn);
	if(!valid) {
		if(loaded) {
			hmem_free(loaded);
		}
		return false;
	}
	replay_op_font = loaded;
	return true;
}

void pascal replay_op_font_free(void)
{
	if(replay_op_font) {
		hmem_free(replay_op_font);
		replay_op_font = 0;
	}
}

static unsigned replay_op_font_index(unsigned char c)
{
	if((c < REPLAY_OP_FONT_FIRST) || (c > REPLAY_OP_FONT_LAST)) {
		c = REPLAY_OP_FONT_FALLBACK;
	}
	return (c - REPLAY_OP_FONT_FIRST);
}

pixel_t pascal replay_op_font_width(const char far *str)
{
	const uint8_t far *data = reinterpret_cast<const uint8_t far *>(
		replay_op_font
	);
	pixel_t width = 0;

	if(!data) {
		return 0;
	}
	while(*str) {
		width += data[
			REPLAY_OP_FONT_ADVANCES_OFFSET + replay_op_font_index(*str)
		];
		str++;
	}
	return width;
}

static void near replay_op_font_glyph_put(
	uint8_t far *vram,
	const uint8_t far *glyph,
	uint8_t left_dots,
	unsigned byte_count
)
{
	_asm {
		push ds
		push es
		push si
		push di
		push bx
		lds  si, glyph
		les  di, vram
		mov  bx, 16
	row_loop:
		mov  dx, [si]
		mov  cl, left_dots
		or   cl, cl
		jnz  shifted
		cmp  byte_count, 0
		je   next_row
		mov  es:[di], dl
		cmp  byte_count, 1
		jbe  next_row
		mov  es:[di + 1], dh
		jmp  short next_row
	shifted:
		cmp  byte_count, 0
		je   next_row
		mov  al, dl
		shr  al, cl
		mov  es:[di], al
		cmp  byte_count, 1
		jbe  next_row
		mov  al, dl
		mov  cl, BYTE_DOTS
		sub  cl, left_dots
		shl  al, cl
		mov  ah, al
		mov  al, dh
		mov  cl, left_dots
		shr  al, cl
		or   al, ah
		mov  es:[di + 1], al
		cmp  byte_count, 2
		jbe  next_row
		mov  al, dh
		mov  cl, BYTE_DOTS
		sub  cl, left_dots
		shl  al, cl
		mov  es:[di + 2], al
	next_row:
		add  si, 2
		add  di, ROW_SIZE
		dec  bx
		jnz  row_loop
		pop  bx
		pop  di
		pop  si
		pop  es
		pop  ds
	}
}

void pascal replay_op_font_put_n(
	screen_x_t left,
	vram_y_t top,
	const char far *str,
	unsigned count,
	int color
)
{
	const uint8_t far *data = reinterpret_cast<const uint8_t far *>(
		replay_op_font
	);
	uint8_t far *vram;
	const uint8_t far *glyph;
	unsigned glyph_index;
	unsigned byte_count;
	uint8_t left_dots;
	vram_offset_t vo;

	if(!data || (top < 0) || (top > (graph_VramLines - 16))) {
		return;
	}
	grcg_setcolor(GC_RMW, color);
	while(count && *str) {
		glyph_index = replay_op_font_index(*str);
		if((*str != ' ') && (left >= 0) && (left < RES_X)) {
			left_dots = (left & (BYTE_DOTS - 1));
			byte_count = ((RES_X / BYTE_DOTS) - (left / BYTE_DOTS));
			vo = vram_offset_shift(left, top);
			vram = reinterpret_cast<uint8_t far *>(MK_FP(graph_VramSeg, vo));
			glyph = &data[
				REPLAY_OP_FONT_BITMAP_OFFSET +
				(glyph_index * REPLAY_OP_FONT_GLYPH_SIZE)
			];
			replay_op_font_glyph_put(vram, glyph, left_dots, byte_count);
		}
		left += data[REPLAY_OP_FONT_ADVANCES_OFFSET + glyph_index];
		str++;
		count--;
	}
	grcg_off();
}

void pascal replay_op_font_put(
	screen_x_t left, vram_y_t top, const char far *str, int color
)
{
	replay_op_font_put_n(left, top, str, 0xFFFF, color);
}

void pascal replay_op_font_put_centered(
	screen_x_t center_x, vram_y_t top, const char far *str, int color
)
{
	replay_op_font_put(
		(center_x - (replay_op_font_width(str) / 2)), top, str, color
	);
}

void pascal replay_op_font_put_right(
	screen_x_t right, vram_y_t top, const char far *str, int color
)
{
	replay_op_font_put((right - replay_op_font_width(str)), top, str, color);
}

void pascal replay_op_font_put_cells(
	screen_x_t cell_left, vram_y_t top, const char far *str, int color
)
{
	const char far *run;
	unsigned cells = 0;
	unsigned count;

	while(*str) {
		while(*str == ' ') {
			str++;
			cells++;
		}
		run = str;
		count = 0;
		while(*str && (*str != ' ')) {
			str++;
			count++;
		}
		if(count) {
			replay_op_font_put_n(
				(cell_left + (cells * REPLAY_OP_FONT_NUMERIC_CELL_W)),
				top, run, count, color
			);
			cells += count;
		}
	}
}

void pascal replay_op_font_put_numeric_cells(
	screen_x_t cell_left,
	vram_y_t top,
	const char far *str,
	unsigned count,
	int color
)
{
	screen_x_t glyph_left;

	while(count && *str) {
		glyph_left = cell_left;
		if(*str == '1') {
			glyph_left += REPLAY_OP_FONT_ONE_INSET;
		}
		if(*str != ' ') {
			replay_op_font_put_n(glyph_left, top, str, 1, color);
		}
		cell_left += REPLAY_OP_FONT_NUMERIC_CELL_W;
		str++;
		count--;
	}
}

#undef REPLAY_OP_FONT_FALLBACK
#undef REPLAY_OP_FONT_LAST
#undef REPLAY_OP_FONT_FIRST
#undef REPLAY_OP_FONT_GLYPH_SIZE
#undef REPLAY_OP_FONT_GLYPH_COUNT
#undef REPLAY_OP_FONT_BITMAP_OFFSET
#undef REPLAY_OP_FONT_ADVANCES_OFFSET
#undef REPLAY_OP_FONT_HEADER_SIZE
#undef REPLAY_OP_FONT_FILE_SIZE

#pragma codeseg
