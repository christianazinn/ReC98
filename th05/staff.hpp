/// TH05's staff roll — the "space" scene
/// -------------------------------------
/// A field of drifting particles and stars around a central orb, seen through
/// a window that space_window_set() clips to and slides across the screen.
///
/// The entities used to be declared inside th05/staff.cpp. They moved here
/// because the scene is split across two objects, and ZUN's own segmentation
/// is why: staff.cpp contributes to SCORE_TEXT, while the scene's animation
/// code sits in MAINE_01__TEXT and therefore has to live in a separate
/// translation unit (th05/space.cpp). One declaration, two users.

#ifndef TH05_STAFF_HPP
#define TH05_STAFF_HPP

#include "th01/math/dir.hpp"
#include "th01/math/subpixel.hpp"

/// Pattern numbers for the super_*() functions
/// -------------------------------------------
static const int ORB_PARTICLE_CELS = 6;

typedef enum {
	// stf01.bft
	// ---------
	// Ordered from big to small
	PAT_ORB_PARTICLE = 0,
	PAT_ORB_PARTICLE_last = (PAT_ORB_PARTICLE + ORB_PARTICLE_CELS - 1),
	PAT_STAR_BIG,
	PAT_STAR_SMALL,
	// ---------

	// stf00.bft
	// ---------
	// The orb's four 32x32 cels, cycled through by space_put().
	PAT_ORB,
	// ---------
} staff_patnum_t;
/// -------------------------------------------

// Particle structure
// ------------------

// Below this [phase_value.radius], the orb is rendered as a monochrome filled
// circle.
static const pixel_t ORB_RADIUS_FULL = 16;
static const pixel_t ORB_W = 32;
static const pixel_t ORB_H = 32;

struct orb_particle_t {
	SPPointBase< SubpixelBase< long, pixel_t > > center;
	SPPoint velocity;
	int patnum_tiny;
	Subpixel speed;
	int gather_frame;	// Only used during the orb gather phase.
	unsigned char angle;
	union {
		unsigned char radius;	// In pixels. Only used with the [orb] instance
		x_direction_t rain_sway_x_direction;
	} al;
};
// ------------------

// State
// -----
static const int ORB_PARTICLE_COUNT = 64;
static const int ORB_TRAIL_COUNT = 8;
static const int STAR_COUNT = 48;
static const int ORB_INDEX = ORB_PARTICLE_COUNT;

extern pixel_t space_window_w;
extern pixel_t space_window_h;
extern screen_point_t space_window_center;

// Effectively just moves all the entities into the opposite direction.
extern SPPoint space_camera_velocity;

// Subtracted from every [particles] element's [speed] on every frame, in
// space_update(). A particle whose [speed] reaches exactly 0 is removed.
//
// 0 means more than "no deceleration": while this is 0, space_update() instead
// wraps every particle that leaves the [space_window] around to the opposite
// edge, which is what makes the field look endless. So the two values ZUN
// writes are the scene's two particle modes — 0 for the drifting field
// (space_reset(), and the rain phase), 1 once the particles are supposed to
// die out (orb_gather_end(), orb_burst()).
extern int particle_decel;

// All coordinates of these are relative to the center of space.
extern orb_particle_t particles[ORB_PARTICLE_COUNT + 1];
#define orb particles[ORB_INDEX]

extern SPPoint orb_trails_center[ORB_TRAIL_COUNT];

// Elements at even indices are rendered as PAT_STAR_BIG, elements at odd
// indices are rendered as PAT_STAR_SMALL.
extern SPPoint stars_center[STAR_COUNT];
// -----

void pascal near space_window_set(
	screen_x_t center_x, screen_y_t center_y, pixel_t w, pixel_t h
);

// The scene's own animation, in MAINE_01__TEXT (th05/space.cpp).
// -------------------------------------------------------------

// Scatters all [particles] and [stars_center] randomly across a fresh
// [space_window], resets the orb, its trails and the camera, and puts the
// field into its drifting mode.
void near space_reset(void);

// Turns every particle towards the center of space at a speed proportional to
// its distance from it, and activates the orb they are gathering into.
void near orb_gather_start(void);

// Ends the gather phase: every particle has arrived, so they all go away and
// the orb is complete.
void near orb_gather_end(void);

// Blows the orb apart into the particle field again.
void near orb_burst(void);

// Moves every entity by its own velocity and the camera's, and either wraps it
// around the space window or decays it, depending on [particle_decel].
void near space_update(void);

// Recycles the next particle in [particles] as a spark thrown off the orb.
void near orb_particle_emit(void);

// Shifts the orb's position history down by one and records where it is now.
void near orb_trails_advance(void);

// Advances the gather animation by one frame: every particle cycles through
// its cels as it flies in, and the orb they gather into grows.
void near orb_gather_animate(void);

// One frame of the orb phase, between orb_gather_end() and orb_burst().
// Scripts the camera and slides the space window off to the left; returns
// whether that slide is over.
bool16 near orb_phase_update(void);

// Recycles the next particle in [particles] as a raindrop entering the space
// window from above. Stops once the whole array has been walked once.
void near rain_particle_spawn(void);

// One frame of the rain phase, after orb_burst(). Undoes the orb phase's
// camera script, slides the space window back over the screen and rains the
// burst particles down; returns whether that slide is over.
bool16 near rain_phase_update(void);

// One frame of the first credit line: the .CDG image in [slot], centered at
// ([x_center], [y_center]), fading in behind a gaiji curtain, held until
// snd_bgm_measure() reports [measure], then fading back out and erasing
// itself. Returns whether that whole cycle is over, which is what
// staffroll_animate() waits for before moving on to the next line.
//
// Both this function and its state machine have to be called twice per frame,
// once for each VRAM page: the first two calls blit the image, the next two
// fade it in, and so on.
bool16 pascal near credit_animate(
	screen_x_t x_center, vram_y_t y_center, int slot, int measure
);

// credit_animate() for the second credit line — the one shown below the first
// one and at the same time as it, on its own copy of the same state. ZUN
// duplicated the entire function rather than parameterizing it, exactly as
// TH04's staff roll duplicates dissolve_out_animate() into
// dissolve_out_2_animate().
bool16 pascal near credit_2_animate(
	screen_x_t x_center, vram_y_t y_center, int slot, int measure
);

// Redraws the entire scene onto the current VRAM page: the space window is
// cleared, every star, orb trail, orb and particle inside it is blitted, and
// the 8-pixel border around it is painted over so that nothing survives the
// window sliding across the screen.
void near space_put(void);

// One frame of the scene: space_update(), space_put(), a frame of delay, and
// the page flip, followed by the pending text layer clear and the BGM measure
// that both credit lines time their fade-out on.
void near staffroll_frame_and_flip(void);

#endif /* TH05_STAFF_HPP */
