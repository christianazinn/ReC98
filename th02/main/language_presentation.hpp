#ifndef TH02_MAIN_LANGUAGE_PRESENTATION_HPP
#define TH02_MAIN_LANGUAGE_PRESENTATION_HPP

#include "platform.h"
#include "shiftjis.hpp"

// Selects the MAIN text tables that match the font BFT loaded immediately
// before this call. The argument is false for Japanese and overlay fallback.
void far t2_language_main_presentation_apply(bool english_bft_loaded);

// These accessors fail closed to stock Japanese unless the matching English
// BFT transaction completed in this MAIN process.
const char far *t2_language_main_pause_label(uint8_t option);
const shiftjis_t far *t2_language_main_bonus_label(
	const shiftjis_t far *stock_label
);
void far pascal t2_language_main_bonus_row_put_and_add(
	tram_y_t y, const shiftjis_t far *stock_label, int far &sum, int val_x10
);

#endif /* TH02_MAIN_LANGUAGE_PRESENTATION_HPP */
