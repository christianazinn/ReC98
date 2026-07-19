#pragma option -zCOPFONT_TEXT -zPOPFONT_TEXT

#include "libs/master.lib/pc98_gfx.hpp"
#include "th01/hardware/egc_impl.hpp"
#include "th02/v_colors.hpp"
#include "th03/menu_font.hpp"
#include "th03/op/m_main.hpp"
#include "th03/opfont.hpp"
#include "planar.h"
#include "x86real.h"

extern char replay_menu_line[81];
extern "C" unsigned graph_VramSeg;

static tram_y_t choice_tram_y(unsigned line)
{
	return ((BOX_TOP / GLYPH_H) + 1 + line);
}

void pascal far title_box_interior_restore(
	unsigned top, unsigned h, screen_x_t box_right
)
{
	vram_offset_t vo_row = vram_offset_shift(BOX_LEFT, top);
	unsigned word_count = (
		((box_right - BOX_LEFT) + 15) / 16
	);
	unsigned last_dots = ((box_right - 2) & 15);
	unsigned row;
	unsigned column;
	dots16_t mask;
	dots16_t last_mask;
	dots16_t tmp;

	if(last_dots == 0) {
		last_mask = 0xFFFF;
	} else if(last_dots <= 8) {
		last_mask = (0xFF << (8 - last_dots));
	} else {
		last_mask = (
			0x00FF | ((0xFF << (16 - last_dots)) << 8)
		);
	}

	// Page 1 owns the bare title. Restore it through the transparent half of
	// OPWIN's checker without touching the two-pixel frame on either side.
	egc_on();
	egc_setup_copy();
	for(row = 0; row < h; row++) {
		for(column = 0; column < word_count; column++) {
			mask = ((column == 0) ? 0xFF3F : 0xFFFF);
			if(column == (word_count - 1)) {
				mask &= last_mask;
			}
			outport(EGC_MASKREG, mask);
			graph_accesspage(1);
			tmp = *reinterpret_cast<dots16_t far *>(
				MK_FP(graph_VramSeg, (vo_row + (column * 2)))
			);
			graph_accesspage(0);
			*reinterpret_cast<dots16_t far *>(
				MK_FP(graph_VramSeg, (vo_row + (column * 2)))
			) = tmp;
		}
		vo_row += ROW_SIZE;
	}
	egc_off();

	// Color 0 is transparent in OPWIN.BFT; only color 3 is opaque.
	grcg_setcolor(GC_RMW, 3);
	_asm {
		push	es;
		push	di;
		push	bx;
		mov 	ax, top;
		mov 	di, ax;
		shl 	di, 4;
		mov 	dx, di;
		shl 	di, 2;
		add 	di, dx;
		add 	di, ((BOX_LEFT + 2) / BYTE_DOTS);
		mov 	es, graph_VramSeg;
		mov 	bx, h;
		mov 	cx, box_right;
		shr 	cx, 3;
		sub 	cx, ((BOX_LEFT + 2) / BYTE_DOTS);
		mov 	al, 0AAh;
	row_loop:
		push	di;
		mov 	ah, al;
		and 	ah, 03Fh;
		mov 	es:[di], ah;
		inc 	di;
		mov 	dx, cx;
		sub 	dx, 2;
	byte_loop:
		mov 	es:[di], al;
		inc 	di;
		dec 	dx;
		jnz 	byte_loop;
	last_byte:
		mov 	ah, al;
		and 	ah, 0FCh;
		mov 	es:[di], ah;
		pop 	di;
		add 	di, ROW_SIZE;
		xor 	al, 0FFh;
		dec 	bx;
		jnz 	row_loop;
		pop 	bx;
		pop 	di;
		pop 	es;
	}
	grcg_off();
}

void pascal far title_credit_graphics_unput(void)
{
	if(menu_font) {
		menu_font_restore_rect(0, 0, RES_X, (GLYPH_H * 2));
	}
}

void pascal far title_credit_line_put(
	const char far *str, unsigned len, unsigned y
)
{
	if(menu_font) {
		menu_font_put_right(638, (y * GLYPH_H), str, 0);
	} else {
		text_putsa((80 - len), y, str, TX_BLACK);
	}
}

void pascal far choice_put_centered(
	screen_x_t center_x,
	unsigned line,
	int shift_x,
	const char far *str,
	unsigned atrb
)
{
	unsigned len = 0;

	(void)shift_x;
	if(menu_font) {
		menu_font_put_centered(
			center_x, (choice_tram_y(line) * GLYPH_H), str,
			((atrb == TX_BLACK) ? 0 : V_WHITE)
		);
		return;
	}
	while(str[len]) {
		len++;
	}
	text_putsa(
		((center_x / GLYPH_HALF_W) - (len / 2)),
		choice_tram_y(line), str, atrb
	);
}

void pascal far replay_menu_span_clear(
	unsigned x, unsigned y, unsigned w
)
{
	char *p = replay_menu_line;

	if(menu_font) {
		menu_font_restore_rect(
			(x * GLYPH_HALF_W), (y * GLYPH_H),
			(w * GLYPH_HALF_W), GLYPH_H
		);
		return;
	}
	while(w != 0) {
		*p++ = ' ';
		w--;
	}
	*p = '\0';
	text_putsa(x, y, replay_menu_line, TX_BLACK);
}

void pascal far replay_menu_line_put(unsigned x, unsigned y, unsigned atrb)
{
	int color;

	if(!menu_font) {
		text_putsa(x, y, replay_menu_line, atrb);
		return;
	}
	if(atrb == TX_BLACK) {
		color = 0;
	} else if(atrb == TX_CYAN) {
		color = 13;
	} else if(atrb == TX_YELLOW) {
		color = 12;
	} else {
		color = V_WHITE;
	}
	menu_font_put_cells(
		(x * GLYPH_HALF_W), (y * GLYPH_H), replay_menu_line, color
	);
}

// Keep the following compiler runtime contributions at their 0.3.2-rc2 phase.
#pragma codestring "\x90\x90\x90\x90"
