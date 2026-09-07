#ifndef TH01_STATVIEW_HPP
#define TH01_STATVIEW_HPP

#include "platform.h"
#include <stdio.h>
#include "th01/formats/scoredat.hpp"
#include "th01/score.h"
#include "shiftjis.hpp"
#include "pc98.h"

// Null-terminated scoredat name, shared with the native table renderer.
typedef ShiftJISKanjiBuffer<SCOREDAT_NAME_KANJI + 1> scoredat_name_z_t;

void far __cdecl scorestat_row_put(
	screen_x_t left, vram_y_t top, int16_t col_and_fx, const shiftjis_t *str
);
FILE* far scorestat_fopen(const char *fn, const char *mode);
int far scorestat_fclose(FILE *fp);

#endif
