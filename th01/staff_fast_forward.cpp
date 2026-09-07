// Patch-owned staff-roll acceleration for FUUIN.EXE.

#pragma option -zCT1STAFF_FF_TEXT -G-

#include "platform/x86real/pc98/keyboard.hpp"
#include "th01/formats/scoredat.hpp"
#include "th01/hardware/frmdelay.h"
#include "th01/rank.h"
#include "th01/resident.hpp"
#include "th01/staff_fast_forward.hpp"
#include "x86real.h"

extern int8_t* scoredat_names;
extern int8_t* scoredat_routes;
extern int16_t* scoredat_stages;
extern score_t* scoredat_score;

static bool t1staff_fast_forward_active;
static bool t1staff_fast_forward_unlocked;
static uint8_t t1staff_fast_forward_phase;

static void near t1staff_scoredat_free(void)
{
	delete[] scoredat_names;
	delete[] scoredat_routes;
	delete[] scoredat_stages;
	delete[] scoredat_score;
}

static bool near t1staff_fast_forward_unlocked_load(void)
{
	int8_t rank_saved = rank;
	bool unlocked = false;

	for(rank = RANK_EASY; rank <= RANK_LUNATIC; rank++) {
		if(scoredat_load() != 0) {
			continue;
		}
		for(int place = 0; place < SCOREDAT_PLACES; place++) {
			if(scoredat_stages[place] >= SCOREDAT_CLEARED) {
				unlocked = true;
				break;
			}
		}
		t1staff_scoredat_free();
		if(unlocked) {
			break;
		}
	}
	rank = rank_saved;
	return unlocked;
}

static bool near t1staff_fast_forward_held(void)
{
	return (
		t1staff_fast_forward_active && t1staff_fast_forward_unlocked &&
		((peekb(0, KEYGROUP_5) & K5_Z) != 0)
	);
}

void far t1staff_fast_forward_begin(void)
{
	t1staff_fast_forward_unlocked = t1staff_fast_forward_unlocked_load();
	t1staff_fast_forward_phase = 0;
	t1staff_fast_forward_active = true;
}

void far t1staff_fast_forward_end(void)
{
	t1staff_fast_forward_active = false;
	t1staff_fast_forward_phase = 0;
}

void pascal far t1staff_fast_forward_frame_delay(unsigned int frames)
{
	if(
		!t1staff_fast_forward_active || !t1staff_fast_forward_unlocked ||
		!t1staff_fast_forward_held()
	) {
		t1staff_fast_forward_phase = 0;
		frame_delay(frames);
		return;
	}
	if(frames == 0) {
		frame_delay(0);
		return;
	}
	while(frames > 0) {
		if(t1staff_fast_forward_held()) {
			t1staff_fast_forward_phase++;
			if(t1staff_fast_forward_phase >= T1_STAFF_FAST_FORWARD_RATE) {
				t1staff_fast_forward_phase = 0;
				frame_delay(1);
			}
		} else {
			t1staff_fast_forward_phase = 0;
			frame_delay(1);
		}
		frames--;
	}
}
