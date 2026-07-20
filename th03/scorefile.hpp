#ifndef TH03_SCOREFILE_HPP
#define TH03_SCOREFILE_HPP

#include "platform.h"
#include "th01/rank.h"
#include "th03/playchar.hpp"
#include "th03/keyconfig.hpp"
#include "th03/practice.hpp"

extern const char far t3_scorefile_fn[];
extern const char far t3_scorefile_temp_fn[];
extern const char far t3_scorefile_old_fn[];
extern const char far t3_scorefile_backup_fn[];
extern const char far t3_scorefile_backup_1_fn[];
extern const char far t3_scoretime_fn[];

#define T3_SCOREFILE_FN t3_scorefile_fn
#define T3_SCOREFILE_TEMP_FN t3_scorefile_temp_fn
#define T3_SCOREFILE_OLD_FN t3_scorefile_old_fn
#define T3_SCOREFILE_BACKUP_FN t3_scorefile_backup_fn
#define T3_SCOREFILE_BACKUP_1_FN t3_scorefile_backup_1_fn
#define T3_SCORETIME_FN t3_scoretime_fn

#define T3_SCOREFILE_PLACES 10
#define T3_SCOREFILE_NAME_LEN 8
#define T3_SCOREFILE_SCORE_DIGITS 8
#define T3_SCOREFILE_VIEW_ALL 0xFF

#define T3_SCORESTAT_RES_START_INDEX 98
#define T3_SCORESTAT_RES_MAGIC_0_INDEX 98
#define T3_SCORESTAT_RES_MAGIC_1_INDEX 99
#define T3_SCORESTAT_RES_VERSION_INDEX 100
#define T3_SCORESTAT_RES_ACTIVE_INDEX 101
#define T3_SCORESTAT_RES_RANK_INDEX 102
#define T3_SCORESTAT_RES_PLAYCHAR_INDEX 103
#define T3_SCORESTAT_RES_FRAMES_INDEX 104
#define T3_SCORESTAT_RES_RUN_ID_INDEX 108
#define T3_SCORESTAT_RES_END_INDEX 112

#if (T3_SCORESTAT_RES_START_INDEX < T3_KEYCONFIG_RES_END_INDEX)
#error Score-stat resident block overlaps KeyConfig
#endif
#if (T3_SCORESTAT_RES_END_INDEX > T3_PRACTICE_RES_START_INDEX)
#error Score-stat resident block overlaps Practice
#endif

struct scorefile_stats_t {
	uint32_t play_frames;
	uint32_t one_ccs;
	uint32_t continues_used;
};

extern scorefile_stats_t scorefile_view_stats;
extern uint8_t scorefile_view_page;

bool16 far scorefile_compat_load(rank_t rank);
void far scorefile_compat_save(rank_t rank);
bool16 far scorefile_view_load(rank_t rank, uint8_t page);
bool16 far scorefile_row_total(uint8_t place);
void far scorefile_row_insert(uint8_t place);
bool16 far scorefile_unlocked(void);
void far scorefile_view_overlay_put(void);
void far scorefile_close(void);

void far scorestat_run_begin(void);
void far scorestat_frame_tick(void);
void far scorestat_exit_checkpoint(void);
void far scorestat_continue_accept(void);
bool16 far scorestat_active(void);

#endif /* TH03_SCOREFILE_HPP */
