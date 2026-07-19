#ifndef TH03_OPFONT_HPP
#define TH03_OPFONT_HPP

#include "platform.h"
#include "pc98.h"

void pascal far title_box_interior_restore(
	unsigned top, unsigned h, screen_x_t box_right
);
void pascal far title_credit_graphics_unput(void);
void pascal far title_credit_line_put(
	const char far *str, unsigned len, unsigned y
);
void pascal far choice_put_centered(
	screen_x_t center_x,
	unsigned line,
	int shift_x,
	const char far *str,
	unsigned atrb
);
void pascal far replay_menu_span_clear(
	unsigned x, unsigned y, unsigned w
);
void pascal far replay_menu_line_put(unsigned x, unsigned y, unsigned atrb);

#endif
