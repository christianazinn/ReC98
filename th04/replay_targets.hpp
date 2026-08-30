#ifndef TH04_REPLAY_TARGETS_HPP
#define TH04_REPLAY_TARGETS_HPP

#include "platform.h"
#include "th04/common.h"

// Stable, player-facing field boundaries. These are synthetic clean Practice
// starts at the first .STD record after a natural gap, not historical stage
// snapshots. Keep this switch-only so including the catalog in OP and MAIN
// adds no initialized data to either executable.
static uint8_t replay_practice_chapter_count(uint8_t stage)
{
	#if (GAME == 5)
		switch(stage) {
		case 0: return 8;
		case 1: return 8;
		case 2: return 8;
		case 3: return 10;
		case 4: return 6;
		case 5: return 2;
		case STAGE_EXTRA: return 6;
		}
	#else
		switch(stage) {
		case 0: return 7;
		case 1: return 6;
		case 2: return 11;
		case 3: return 11;
		case 4: return 4;
		case 5: return 4;
		case STAGE_EXTRA: return 10;
		}
	#endif
	return 0;
}

static uint8_t replay_practice_midboss_count(uint8_t stage)
{
	#if (GAME == 5)
		return ((stage == 5) ? 0 : ((stage <= STAGE_EXTRA) ? 1 : 0));
	#else
		if(stage == 3) {
			return 2;
		}
		return (((stage <= 2) || (stage == STAGE_EXTRA)) ? 1 : 0);
	#endif
}

// Chapter after which the given midboss appears in menu chronology.
static uint8_t replay_practice_midboss_after_chapter(
	uint8_t stage, uint8_t midboss_index
)
{
	#if (GAME == 5)
		if(midboss_index != 0) {
			return 0;
		}
		switch(stage) {
		case 0: return 3;
		case 1: return 4;
		case 2: return 5;
		case 3: return 4;
		case 4: return 4;
		case STAGE_EXTRA: return 4;
		}
	#else
		if((stage == 3) && (midboss_index == 1)) {
			return 6;
		}
		if(midboss_index != 0) {
			return 0;
		}
		switch(stage) {
		case 0: return 4;
		case 1: return 3;
		case 2: return 2;
		case 3: return 3;
		case STAGE_EXTRA: return 7;
		}
	#endif
	return 0;
}

static uint16_t replay_practice_chapter_frame(uint8_t stage, uint8_t chapter)
{
	if(chapter == 1) {
		return 0;
	}
	#if (GAME == 5)
		switch(stage) {
		case 0:
			switch(chapter) {
			case 2: return 1200; case 3: return 1900;
			case 4: return 2800; case 5: return 4200;
			case 6: return 5400; case 7: return 6600;
			case 8: return 7800;
			}
			break;
		case 1:
			switch(chapter) {
			case 2: return 700; case 3: return 1400; case 4: return 2200;
			case 5: return 3400; case 6: return 5100; case 7: return 5500;
			case 8: return 6200;
			}
			break;
		case 2:
			switch(chapter) {
			case 2: return 1600; case 3: return 2200; case 4: return 4000;
			case 5: return 5200; case 6: return 6200; case 7: return 8050;
			case 8: return 9000;
			}
			break;
		case 3:
			switch(chapter) {
			case 2: return 1700; case 3: return 2400; case 4: return 3100;
			case 5: return 5000; case 6: return 6800; case 7: return 7800;
			case 8: return 8900; case 9: return 9550; case 10: return 11000;
			}
			break;
		case 4:
			switch(chapter) {
			case 2: return 1300; case 3: return 2500; case 4: return 3700;
			case 5: return 5400; case 6: return 7800;
			}
			break;
		case 5:
			if(chapter == 2) return 1700;
			break;
		case STAGE_EXTRA:
			switch(chapter) {
			case 2: return 1000; case 3: return 1800; case 4: return 5600;
			case 5: return 7000; case 6: return 10500;
			}
			break;
		}
	#else
		switch(stage) {
		case 0:
			switch(chapter) {
			case 2: return 600; case 3: return 1320; case 4: return 2000;
			case 5: return 3400; case 6: return 4600; case 7: return 5800;
			}
			break;
		case 1:
			switch(chapter) {
			case 2: return 1080; case 3: return 1600; case 4: return 2800;
			case 5: return 5000; case 6: return 6500;
			}
			break;
		case 2:
			switch(chapter) {
			case 2: return 724; case 3: return 2100; case 4: return 3220;
			case 5: return 4200; case 6: return 4700; case 7: return 5188;
			case 8: return 5920; case 9: return 6300; case 10: return 6608;
			case 11: return 7200;
			}
			break;
		case 3:
			switch(chapter) {
			case 2: return 1400; case 3: return 2100; case 4: return 3400;
			case 5: return 4200; case 6: return 5200; case 7: return 6500;
			case 8: return 8200; case 9: return 9700; case 10: return 10600;
			case 11: return 12200;
			}
			break;
		case 4:
			if(chapter == 2) return 700;
			if(chapter == 3) return 3800;
			if(chapter == 4) return 4500;
			break;
		case 5:
			if(chapter == 2) return 1400;
			if(chapter == 3) return 3100;
			if(chapter == 4) return 3700;
			break;
		case STAGE_EXTRA:
			switch(chapter) {
			case 2: return 1100; case 3: return 2000; case 4: return 2700;
			case 5: return 3550; case 6: return 4200; case 7: return 4600;
			case 8: return 8000; case 9: return 9000; case 10: return 9900;
			}
			break;
		}
	#endif
	return 0;
}

static uint16_t replay_practice_midboss_frame(
	uint8_t stage, uint8_t midboss_index
)
{
	#if (GAME == 5)
		if(midboss_index != 0) {
			return 0;
		}
		switch(stage) {
		case 0: return 2500; case 1: return 2750; case 2: return 5750;
		case 3: return 3900; case 4: return 4800;
		case STAGE_EXTRA: return 5800;
		}
	#else
		if((stage == 3) && (midboss_index == 1)) {
			return 5600;
		}
		if(midboss_index != 0) {
			return 0;
		}
		switch(stage) {
		case 0: return 3100; case 1: return 2600; case 2: return 1600;
		case 3: return 2800; case STAGE_EXTRA: return 5400;
		}
	#endif
	return 0;
}

static bool replay_practice_chapter_valid(uint8_t stage, uint8_t chapter)
{
	return (
		(chapter >= 2) &&
		(chapter <= replay_practice_chapter_count(stage)) &&
		(replay_practice_chapter_frame(stage, chapter) != 0)
	);
}

static bool replay_practice_midboss_valid(uint8_t stage, uint8_t index)
{
	return (
		(index < replay_practice_midboss_count(stage)) &&
		(replay_practice_midboss_frame(stage, index) != 0)
	);
}

// Later midboss phases that have distinct community-facing patterns. Phase 0
// remains the native midboss start. Other midbosses retain one selectable
// start until their phase boundaries are reviewed.
static uint8_t replay_practice_midboss_phase_max(
	uint8_t stage, uint8_t index
)
{
	if(index != 0) {
		return 0;
	}
	#if (GAME == 5)
		if(stage == 1) {
			return 2;
		}
		if(stage == STAGE_EXTRA) {
			return 1;
		}
	#else
		if(stage == STAGE_EXTRA) {
			return 6;
		}
	#endif
	return 0;
}

enum replay_practice_target_label_t {
	RPTL_DEFAULT = 0,
	RPTL_PRE_BOSS,
	RPTL_RINGS,
};

static replay_practice_target_label_t replay_practice_chapter_label(
	uint8_t stage, uint8_t chapter
)
{
	if(chapter != replay_practice_chapter_count(stage)) {
		return RPTL_DEFAULT;
	}
	#if (GAME == 4)
		if(stage == 4) {
			return RPTL_RINGS;
		}
	#endif
	return RPTL_PRE_BOSS;
}

// Extension seam for community names such as color-based boss phases. Stable
// replay identities remain the numeric (stage, section, phase) tuple.
static replay_practice_target_label_t replay_practice_boss_phase_label(
	uint8_t stage, uint8_t section, uint8_t phase
)
{
	(void)stage;
	(void)section;
	(void)phase;
	return RPTL_DEFAULT;
}

#endif /* TH04_REPLAY_TARGETS_HPP */
