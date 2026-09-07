// Patch-owned staff-roll acceleration for MAINE.EXE.

#pragma option -zCT2STAFF_FF_TEXT -G-

#include "platform/x86real/pc98/keyboard.hpp"
#include "th01/rank.h"
#include "th02/end/staff_fast_forward.hpp"
#include "th02/hardware/frmdelay.h"
#include "th02/resident.hpp"
#include "th02/snd/measure.hpp"
#include "x86real.h"

static bool t2staff_fast_forward_active;
static uint8_t t2staff_fast_forward_phase;

static bool near t2staff_fast_forward_held(void)
{
	return (
		t2staff_fast_forward_active && (resident->unused_3 != 0) &&
		((peekb(0, KEYGROUP_5) & K5_Z) != 0)
	);
}

void far t2staff_fast_forward_begin(void)
{
	t2staff_fast_forward_active = true;
	t2staff_fast_forward_phase = 0;
}

void far t2staff_fast_forward_end(void)
{
	t2staff_fast_forward_active = false;
	t2staff_fast_forward_phase = 0;
}

void pascal far t2staff_fast_forward_frame_delay(int frames)
{
	if(
		!t2staff_fast_forward_active || (resident->unused_3 == 0) ||
		!t2staff_fast_forward_held()
	) {
		t2staff_fast_forward_phase = 0;
		frame_delay(frames);
		return;
	}
	if(frames == 0) {
		frame_delay(0);
		return;
	}
	while(frames > 0) {
		if(t2staff_fast_forward_held()) {
			t2staff_fast_forward_phase++;
			if(t2staff_fast_forward_phase >= T2_STAFF_FAST_FORWARD_RATE) {
				t2staff_fast_forward_phase = 0;
				frame_delay(1);
			}
		} else {
			t2staff_fast_forward_phase = 0;
			frame_delay(1);
		}
		frames--;
	}
}

void far t2staff_fast_forward_delay_until_measure(int measure)
{
	unsigned int logical_frames = 0;

	if(!t2staff_fast_forward_active || (resident->unused_3 == 0)) {
		snd_delay_until_measure(measure);
		return;
	}
	while(snd_active && (snd_get_song_measure() < measure)) {
		if(t2staff_fast_forward_held()) {
			t2staff_fast_forward_frame_delay(1);
			logical_frames++;
			if(logical_frames >= SND_FALLBACK_DELAY_FRAMES) {
				return;
			}
		} else {
			frame_delay(1);
			logical_frames++;
		}
	}
	if(!snd_active) {
		t2staff_fast_forward_frame_delay(SND_FALLBACK_DELAY_FRAMES);
	}
}
