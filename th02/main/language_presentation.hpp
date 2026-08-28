#ifndef TH02_MAIN_LANGUAGE_PRESENTATION_HPP
#define TH02_MAIN_LANGUAGE_PRESENTATION_HPP

#include "platform.h"

// Selects the MAIN text tables that match the font BFT loaded immediately
// before this call. The argument is false for Japanese and overlay fallback.
void far t2_language_main_presentation_apply(bool english_bft_loaded);

#endif /* TH02_MAIN_LANGUAGE_PRESENTATION_HPP */
