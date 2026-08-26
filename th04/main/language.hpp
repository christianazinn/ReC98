#ifndef TH04_MAIN_LANGUAGE_HPP
#define TH04_MAIN_LANGUAGE_HPP

#include "platform.h"

bool16 language_main_english_selected(void);
void language_main_titles_apply(void);
const char *language_main_pause_label(uint8_t option);
const char *language_main_pause_title(void);

// These have the same far-pascal ABI as master.lib's file_ropen() and
// file_close().  The fixed-span dialog loaders can therefore redirect their
// two existing calls without changing their stock instruction widths.
int far pascal language_main_dialog_ropen(const char *fn);
void far pascal language_main_dialog_close(void);
#if (GAME == 4)
	int far pascal language_main_gaiji_entry_bfnt(const char *fn);
#endif

#endif /* TH04_MAIN_LANGUAGE_HPP */
