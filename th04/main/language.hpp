#ifndef TH04_MAIN_LANGUAGE_HPP
#define TH04_MAIN_LANGUAGE_HPP

#include "platform.h"

// These have the same far-pascal ABI as master.lib's file_ropen() and
// file_close().  The fixed-span dialog loaders can therefore redirect their
// two existing calls without changing their stock instruction widths.
int far pascal language_main_dialog_ropen(const char *fn);
void far pascal language_main_dialog_close(void);
#if (GAME == 4)
	int far pascal language_main_gaiji_entry_bfnt(const char *fn);
#endif

#endif /* TH04_MAIN_LANGUAGE_HPP */
