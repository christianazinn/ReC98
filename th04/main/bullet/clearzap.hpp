#ifndef TH04_MAIN_BULLET_CLEARZAP_HPP
#define TH04_MAIN_BULLET_CLEARZAP_HPP

// Guarded for the reason th04/main/player/shot.hpp states in full: two body
// files that both include this one now meet in a single translation unit,
// and Turbo C++ 4.02 rejects the second expansion of every `static const`
// and every declaration below. Ordering the includes so only one of them
// wins would work today and break at the next host; a guard is the
// invariant. Byte-inert: this file only declares.

#include "th04/sprites/cels.h"
#include "platform.h"

// Set to `true` to clear all on-screen bullets, giving out a semi-exponential
// bonus for all bullets that were alive on the first frame of activity.
// Lasts for BULLET_ZAP_FRAMES and resets to `false` afterwards.
extern union {
	bool active;
	uint8_t frame; // doubles as the animation timer
} bullet_zap;

static const int BULLET_ZAP_FRAMES_PER_CEL = 4;

// ZUN quirk: Effectively 1 in TH05, see bullets_update() for the cause.
static const int BULLET_ZAP_FRAMES = (
	BULLET_ZAP_CELS * BULLET_ZAP_FRAMES_PER_CEL
);

// # of frames left during which all on-screen bullets should decay.
// Gives a constant point bonus for every bullet decayed during that time.
extern unsigned char bullet_clear_time;

#define bullets_clear() { \
	if(bullet_clear_time < 20) { \
		bullet_clear_time = 20; \
	} \
}
#endif /* TH04_MAIN_BULLET_CLEARZAP_HPP */
