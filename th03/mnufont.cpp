#pragma option -zCMNUFONT_TEXT -zPMNUFONT_TEXT

#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th01/hardware/egc_impl.hpp"
#include "th03/menu_font.hpp"
#include "planar.h"
#include "x86real.h"

static const unsigned MENU_FONT_FILE_SIZE = 3168;
static const unsigned MENU_FONT_HEADER_SIZE = 32;
static const unsigned MENU_FONT_ADVANCES_OFFSET = 32;
static const unsigned MENU_FONT_BITMAP_OFFSET = 128;
static const unsigned MENU_FONT_GLYPH_COUNT = 95;
static const unsigned MENU_FONT_GLYPH_SIZE = 32;
static const unsigned char MENU_FONT_FIRST = 0x20;
static const unsigned char MENU_FONT_LAST = 0x7E;
static const unsigned char MENU_FONT_FALLBACK = 0x3F;
menu_font_t far menu_font;

extern "C" unsigned graph_VramSeg;
extern "C" unsigned graph_VramLines;

static uint16_t menu_font_u16(const uint8_t far *p)
{
	return (p[0] | (p[1] << 8));
}

static bool16 menu_font_validate(const uint8_t far *data)
{
	unsigned i;
	uint16_t checksum = 0;

	if(
		(data[0] != 'T') ||
		(data[1] != '3') ||
		(data[2] != 'P') ||
		(data[3] != 'F') ||
		(data[4] != 'N') ||
		(data[5] != 'T') ||
		(data[6] != '1') ||
		(data[7] != 0) ||
		(menu_font_u16(&data[8]) != MENU_FONT_FILE_SIZE) ||
		(menu_font_u16(&data[10]) != MENU_FONT_HEADER_SIZE) ||
		(menu_font_u16(&data[12]) != MENU_FONT_ADVANCES_OFFSET) ||
		(menu_font_u16(&data[14]) != MENU_FONT_BITMAP_OFFSET) ||
		(menu_font_u16(&data[16]) != MENU_FONT_GLYPH_COUNT) ||
		(data[18] != MENU_FONT_FIRST) ||
		(data[19] != MENU_FONT_LAST) ||
		(data[20] != 16) ||
		(data[21] != 16) ||
		(data[22] != MENU_FONT_FALLBACK) ||
		(data[23] != 0)
	) {
		return false;
	}
	for(i = 26; i < MENU_FONT_HEADER_SIZE; i++) {
		if(data[i] != 0) {
			return false;
		}
	}
	for(i = 0; i < MENU_FONT_GLYPH_COUNT; i++) {
		if((data[MENU_FONT_ADVANCES_OFFSET + i] == 0) ||
			(data[MENU_FONT_ADVANCES_OFFSET + i] > 16)) {
			return false;
		}
	}
	if(data[127] != 0) {
		return false;
	}
	for(i = MENU_FONT_HEADER_SIZE; i < MENU_FONT_FILE_SIZE; i++) {
		checksum += data[i];
	}
	return (checksum == menu_font_u16(&data[24]));
}

bool16 pascal menu_font_load(const unsigned char far *restore_pf_fn)
{
	menu_font_t loaded = 0;
	bool16 valid = false;
	uint8_t extra;
	char archive_fn[10];
	char font_fn[11];

	archive_fn[0] = 'A'; archive_fn[1] = 'Z'; archive_fn[2] = 'I';
	archive_fn[3] = 'N'; archive_fn[4] = 'N'; archive_fn[5] = '.';
	archive_fn[6] = 'D'; archive_fn[7] = 'A'; archive_fn[8] = 'T';
	archive_fn[9] = '\0';
	font_fn[0] = 'M'; font_fn[1] = 'N'; font_fn[2] = 'U';
	font_fn[3] = 'F'; font_fn[4] = 'O'; font_fn[5] = 'N';
	font_fn[6] = 'T'; font_fn[7] = '.'; font_fn[8] = 'P';
	font_fn[9] = 'F'; font_fn[10] = '\0';

	menu_font_free();
	pfend();
	pfstart(reinterpret_cast<const unsigned char far *>(archive_fn));
	if(file_ropen(font_fn)) {
		loaded = reinterpret_cast<menu_font_t>(
			hmem_allocbyte(MENU_FONT_FILE_SIZE)
		);
		if(loaded &&
			(file_read(loaded, MENU_FONT_FILE_SIZE) == MENU_FONT_FILE_SIZE) &&
			(file_read(&extra, 1) == 0)) {
			valid = menu_font_validate(
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
	menu_font = loaded;
	return true;
}

void pascal menu_font_free(void)
{
	if(menu_font) {
		hmem_free(menu_font);
		menu_font = 0;
	}
}

static unsigned menu_font_index(unsigned char c)
{
	if((c < MENU_FONT_FIRST) || (c > MENU_FONT_LAST)) {
		c = MENU_FONT_FALLBACK;
	}
	return (c - MENU_FONT_FIRST);
}

pixel_t pascal menu_font_width_n(const char far *str, unsigned count)
{
	const uint8_t far *data = reinterpret_cast<const uint8_t far *>(menu_font);
	pixel_t width = 0;

	if(!data) {
		return 0;
	}
	while(count && *str) {
		width += data[MENU_FONT_ADVANCES_OFFSET + menu_font_index(*str)];
		str++;
		count--;
	}
	return width;
}

pixel_t pascal menu_font_width(const char far *str)
{
	return menu_font_width_n(str, 0xFFFF);
}

static void near menu_font_glyph_put(
	uint8_t far *vram,
	const uint8_t far *glyph,
	uint8_t left_dots,
	unsigned byte_count
)
{
	_asm {
		push	ds
		push	es
		push	si
		push	di
		push	bx
		lds 	si, glyph
		les 	di, vram
		mov 	bx, 16

	row_loop:
		mov 	dx, [si]
		mov 	cl, left_dots
		or  	cl, cl
		jnz 	shifted
		cmp 	byte_count, 0
		je  	next_row
		mov 	es:[di], dl
		cmp 	byte_count, 1
		jbe 	next_row
		mov 	es:[di + 1], dh
		jmp 	short next_row

	shifted:
		cmp 	byte_count, 0
		je  	next_row
		mov 	al, dl
		shr 	al, cl
		mov 	es:[di], al
		cmp 	byte_count, 1
		jbe 	next_row
		mov 	al, dl
		mov 	cl, BYTE_DOTS
		sub 	cl, left_dots
		shl 	al, cl
		mov 	ah, al
		mov 	al, dh
		mov 	cl, left_dots
		shr 	al, cl
		or  	al, ah
		mov 	es:[di + 1], al
		cmp 	byte_count, 2
		jbe 	next_row
		mov 	al, dh
		mov 	cl, BYTE_DOTS
		sub 	cl, left_dots
		shl 	al, cl
		mov 	es:[di + 2], al

	next_row:
		add 	si, 2
		add 	di, ROW_SIZE
		dec 	bx
		jnz 	row_loop
		pop 	bx
		pop 	di
		pop 	si
		pop 	es
		pop 	ds
	}
}

void pascal menu_font_put_n(
	screen_x_t left,
	vram_y_t top,
	const char far *str,
	unsigned count,
	int color
)
{
	const uint8_t far *data = reinterpret_cast<const uint8_t far *>(menu_font);
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
		glyph_index = menu_font_index(*str);
		if(*str != ' ') {
			if((left >= 0) && (left < RES_X)) {
				left_dots = (left & (BYTE_DOTS - 1));
				byte_count = ((RES_X / BYTE_DOTS) - (left / BYTE_DOTS));
				vo = vram_offset_shift(left, top);
				vram = reinterpret_cast<uint8_t far *>(
					MK_FP(graph_VramSeg, vo)
				);
				glyph = &data[
					MENU_FONT_BITMAP_OFFSET +
					(glyph_index * MENU_FONT_GLYPH_SIZE)
				];
				menu_font_glyph_put(vram, glyph, left_dots, byte_count);
			}
		}
		left += data[MENU_FONT_ADVANCES_OFFSET + glyph_index];
		str++;
		count--;
	}
	grcg_off();
}

void pascal menu_font_put(
	screen_x_t left, vram_y_t top, const char far *str, int color
)
{
	menu_font_put_n(left, top, str, 0xFFFF, color);
}

void pascal menu_font_put_centered(
	screen_x_t center_x, vram_y_t top, const char far *str, int color
)
{
	menu_font_put((center_x - (menu_font_width(str) / 2)), top, str, color);
}

void pascal menu_font_put_right(
	screen_x_t right, vram_y_t top, const char far *str, int color
)
{
	menu_font_put((right - menu_font_width(str)), top, str, color);
}

void pascal menu_font_put_cells(
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
			menu_font_put_n((cell_left + (cells * 8)), top, run, count, color);
			cells += count;
		}
	}
}

void pascal menu_font_restore_rect(
	screen_x_t left, vram_y_t top, pixel_t w, pixel_t h
)
{
	screen_x_t right = (left + w);
	screen_x_t x;
	vram_offset_t vo_row;
	vram_offset_t vo;
	vram_y_t row;
	screen_x_t column;
	uint16_t tmp;

	if(left < 0) {
		left = 0;
	}
	if(right > RES_X) {
		right = RES_X;
	}
	if(
		(top < 0) || (top >= graph_VramLines) ||
		(right <= left) || (h <= 0)
	) {
		return;
	}
	if((top + h) > graph_VramLines) {
		h = (graph_VramLines - top);
	}
	x = (left & ~0xF);
	vo_row = vram_offset_shift(x, top);
	egc_on();
	egc_setup_copy();
	for(row = 0; row < h; row++) {
		for(column = x, vo = vo_row; column < right; column += 16, vo += 2) {
			graph_accesspage(1);
			tmp = *reinterpret_cast<uint16_t far *>(MK_FP(graph_VramSeg, vo));
			graph_accesspage(0);
			*reinterpret_cast<uint16_t far *>(MK_FP(graph_VramSeg, vo)) = tmp;
		}
		vo_row += ROW_SIZE;
	}
	egc_off();
	graph_accesspage(0);
}

// Keep the following compiler runtime contributions at their 0.2.13 phase.
#if BINARY == 'O'
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#else
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#endif
