#pragma option -zCT1STAT_VIEW_TEXT -zPT1STAT_VIEW_TEXT

#include "th01/statview.hpp"
#include "th01/hardware/grppsafx.h"
#include "th04/scorestat.hpp"

#if (BINARY == 'E')
extern uint8_t rank;
#else
extern int8_t rank;
#endif

void far __cdecl scorestat_row_put(
	screen_x_t left, vram_y_t top, int16_t col_and_fx, const shiftjis_t *str
)
{
	graph_putsa_fx(left, top, col_and_fx, str);
	// The last row's dash uses the existing external far call in both EXEs.
	if(top == (48 + (SCOREDAT_PLACES * GLYPH_H))) {
		scorestat_view_put(static_cast<uint8_t>(rank));
	}
}

FILE* far scorestat_fopen(const char *fn, const char *mode)
{
	scorestat_stock_save_prepare(static_cast<uint8_t>(rank));
	return fopen(fn, mode);
}

int far scorestat_fclose(FILE *fp)
{
	int result = fclose(fp);
	scorestat_stock_save_finish();
	return result;
}

void far scorestat_view_put(uint8_t rank)
{
	char line[48];
	scorestat_view_format(rank, line);
	graph_putsa_fx(176, 224, (7 | FX_WEIGHT_BOLD),
		reinterpret_cast<const shiftjis_t *>(line));
}
