#ifndef TH04_SCORESTAT_HPP
#define TH04_SCORESTAT_HPP

#include "platform.h"

struct scorestat_totals_t {
	uint32_t play_frames;
	uint32_t one_ccs;
	uint32_t continues_used;
};

extern scorestat_totals_t scorestat_view;

// [owner] is ignored in TH01 and TH02. TH04 and TH05 pass their playchar ID.
bool16 far scorestat_run_begin(uint8_t rank, uint8_t owner);
// Eligible means native Story/Extra, excluding Practice, demos and playback.
// Call after VSync startup and mode resolution, including at EXE handoffs.
void far scorestat_process_enter(
	uint8_t rank, uint8_t owner, bool16 story_eligible
);
void far scorestat_process_sync(void);
// Discard a native clock reset, retaining accumulated time and eligibility.
void far scorestat_process_rebase(void);
// Checkpoint retains the active run. Terminal clear/end consumes it.
void far scorestat_process_checkpoint(void);
void far scorestat_continue_accept(void);
void far scorestat_run_complete(void);
void far scorestat_run_end(void);

bool16 far scorestat_view_load(uint8_t rank);
// At least 37 bytes. TH01 renderer lives in registration's own code segment.
void far scorestat_view_format(uint8_t rank, char *line);
// Other games' renderers are linked for OP only.
void far scorestat_view_put(uint8_t rank);

// ABI-compatible redirects for existing master.lib calls in score screens.
#if (GAME == 2)
int far pascal scorestat_clip(int left, int top, int right, int bottom);
#elif (GAME >= 4)
void far pascal scorestat_rank_put(int left, int top, int patnum);
#endif

// TH01's native score writer truncates its file. Bracket that writer with
// these two calls to preserve the appended statistics generations.
void far scorestat_stock_save_prepare(uint8_t rank);
void far scorestat_stock_save_finish(void);

#endif /* TH04_SCORESTAT_HPP */
