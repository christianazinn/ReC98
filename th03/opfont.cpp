#pragma option -zCOPFONT_TEXT -zPOPFONT_TEXT

#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/v_colors.hpp"
#include "th03/menu_font.hpp"
#include "th03/op/m_main.hpp"
#include "th03/opfont.hpp"

extern char replay_menu_line[81];

static tram_y_t choice_tram_y(unsigned line)
{
	return ((BOX_TOP / GLYPH_H) + 1 + line);
}

void pascal far title_menu_graphics_unput(void)
{
	int line;

	if(!menu_font) {
		return;
	}
	for(line = 0; line < 7; line++) {
		menu_font_restore_rect(
			(BOX_LEFT + 16), (choice_tram_y(line) * GLYPH_H),
			(SUBMENU_W - 32), GLYPH_H
		);
	}
}

void pascal far title_choice_graphics_unput(unsigned line)
{
	if(menu_font) {
		menu_font_restore_rect(
			(BOX_LEFT + 16), (choice_tram_y(line) * GLYPH_H),
			(SUBMENU_W - 32), GLYPH_H
		);
	}
}

void pascal far title_credit_graphics_unput(void)
{
	if(menu_font) {
		menu_font_restore_rect(320, 0, 320, (GLYPH_H * 2));
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

// Together with MNUFONT_TEXT, retain OP's 0.2.13 runtime paragraph phase.
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
