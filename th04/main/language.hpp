#ifndef TH04_MAIN_LANGUAGE_HPP
#define TH04_MAIN_LANGUAGE_HPP

#include "platform.h"

bool16 language_main_english_selected(void);
void language_main_titles_apply(void);
#if (GAME == 5)
	void language_main_hud_gaiji_apply(void);
#endif
const char far *language_main_pause_label(uint8_t option);
const char far *language_main_pause_title(void);

// Same signature and ABI as text_putsa(). Redirecting the original calls to
// this seam therefore preserves their instruction widths and segment layout.
void far pascal language_main_clear_bonus_putsa(
	unsigned x, unsigned y, const char far *stock, unsigned atrb
);

// These have the same far-pascal ABI as master.lib's file_ropen() and
// file_close().  The fixed-span dialog loaders can therefore redirect their
// two existing calls without changing their stock instruction widths.
int far pascal language_main_dialog_ropen(const char *fn);
void far pascal language_main_dialog_close(void);
#if (GAME == 5)
	// Same-width TH05 dialogue call seams. The stock JP bytecode continues
	// to own resource effects and canonical replay input.
	void far language_main_dialog_reset_sense(void);
	void far pascal language_main_dialog_wait_for_change(int frames);
	void far pascal language_main_dialog_putsa(
		unsigned x, unsigned y, const char far *text, unsigned atrb
	);
	void far pascal language_main_dialog_gaiji_putca(
		unsigned x, unsigned y, unsigned c, unsigned atrb
	);
#endif
#if (GAME == 4)
	int far pascal language_main_gaiji_entry_bfnt(const char *fn);
#endif

#endif /* TH04_MAIN_LANGUAGE_HPP */
