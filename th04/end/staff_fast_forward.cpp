// Patch-owned staff-roll acceleration shared by TH04 and TH05's MAINE.EXE.

#pragma option -zCREPLAY_END_TEXT -zPgroup_01 -G-

#include "libs/master.lib/master.hpp"
#include "platform/x86real/pc98/keyboard.hpp"
#include "th01/rank.h"
#include "th02/hardware/frmdelay.h"
#include "th02/snd/measure.hpp"
#include "th04/end/staff_fast_forward.hpp"
#include "th04/formats/scoredat/scoredat.hpp"
#include "th04/snd/snd.h"
#if (GAME == 5)
#include "th05/playchar.h"
#include "th05/resident.hpp"
#else
#include "th04/playchar.h"
#include "th04/resident.hpp"
#endif
#include "x86real.h"

#if (GAME == 5)
typedef int staff_playchar_t;
#else
typedef playchar_t staff_playchar_t;
#endif

bool pascal near hiscore_scoredat_load_for(staff_playchar_t playchar);

extern unsigned char rank;
#if (GAME == 5)
extern int staffroll_frame;
extern int frame_half;
#endif

static bool staff_fast_forward_checked;
static bool staff_fast_forward_unlocked;
static uint8_t staff_fast_forward_phase;

static int near staff_fast_forward_bgm_measure(void)
{
	if(!snd_bgm_active()) {
		return -1;
	}
	return snd_get_song_measure();
}

static bool near staff_fast_forward_unlocked_load(void)
{
	unsigned char rank_saved = rank;
	bool unlocked = false;

	for(int playchar = 0; playchar < PLAYCHAR_COUNT; playchar++) {
		for(rank = RANK_EASY; rank < RANK_COUNT; rank++) {
			if(hiscore_scoredat_load_for(
				static_cast<staff_playchar_t>(playchar)
			)) {
				continue;
			}
#if (GAME == 5)
			if(hi.score.cleared == SCOREDAT_CLEARED) {
#else
			if((hi.score.cleared & SCOREDAT_CLEARED_BOTH) != 0) {
#endif
				unlocked = true;
				break;
			}
		}
		if(unlocked) {
			break;
		}
	}
	rank = rank_saved;
	return unlocked;
}

static bool near staff_fast_forward_held(void)
{
	if(!staff_fast_forward_checked) {
		staff_fast_forward_unlocked = staff_fast_forward_unlocked_load();
		staff_fast_forward_checked = true;
	}
	return (
		staff_fast_forward_unlocked &&
		((peekb(0, KEYGROUP_5) & K5_Z) != 0)
	);
}

void pascal far staff_fast_forward_frame_delay(int frames)
{
	if(!staff_fast_forward_held()) {
		staff_fast_forward_phase = 0;
		frame_delay(frames);
		return;
	}
	if(frames == 0) {
		frame_delay(0);
		return;
	}
	while(frames > 0) {
		if(staff_fast_forward_held()) {
			staff_fast_forward_phase++;
			if(staff_fast_forward_phase >= TH04_STAFF_FAST_FORWARD_RATE) {
				staff_fast_forward_phase = 0;
				frame_delay(1);
			}
		} else {
			staff_fast_forward_phase = 0;
			frame_delay(1);
		}
		frames--;
	}
}

void pascal far staff_fast_forward_vsync_wait(int frames)
{
	if(staff_fast_forward_held()) {
		staff_fast_forward_phase++;
		if(staff_fast_forward_phase < TH04_STAFF_FAST_FORWARD_RATE) {
			return;
		}
		staff_fast_forward_phase = 0;
	} else {
		staff_fast_forward_phase = 0;
	}
	while(vsync_Count1 < frames) {
	}
	vsync_Count1 = 0;
}

void pascal far staff_fast_forward_delay_until_measure(
	int measure, unsigned int frames_if_no_bgm
)
{
	unsigned int logical_frames = 0;
	int measure_cur;

	do {
		measure_cur = staff_fast_forward_bgm_measure();
		if(measure_cur < 0) {
			staff_fast_forward_frame_delay(frames_if_no_bgm);
			return;
		}
		if(measure_cur >= measure) {
			return;
		}
		if(staff_fast_forward_held()) {
			staff_fast_forward_frame_delay(1);
			logical_frames++;
			if(logical_frames >= frames_if_no_bgm) {
				return;
			}
		} else {
			frame_delay(1);
			logical_frames++;
		}
	} while(1);
}

#if (GAME == 5)
int far staff_fast_forward_main_measure(void)
{
	if(staff_fast_forward_held()) {
		return (staffroll_frame / 22);
	}
	return staff_fast_forward_bgm_measure();
}

int far staff_fast_forward_allcast_measure(void)
{
	if(staff_fast_forward_held()) {
		return (frame_half / 22);
	}
	return staff_fast_forward_bgm_measure();
}
#endif
