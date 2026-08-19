/// TH05's staff roll — the "space" scene's animation
/// -------------------------------------------------
/// The head of th05_maine.asm's MAINE_01__TEXT block. That segment has no C++
/// contribution at all — its only two are th05_maine_master.asm's zero-byte
/// anchor and the dump itself — so a lift here needs a new object, and the
/// Tupfile.lua line for it is POSITION-CRITICAL: TLINK concatenates a
/// segment's contributions in link order, so this entry has to sit
/// **immediately before** "th05_maine.asm" to land at the head of its block
/// rather than after it. (kb/codegen/0105 + 0112 + 0114; 0112's "you still
/// need a new TU and a Tupfile.lua line" is a price, not a blocker.)
///
/// Both pragmas have to precede every emitted byte (kb/codegen/0138):
///
/// • -zC, because kb/codegen/0105's basename default would name this object's
///   code segment SPACE_TEXT. MAINE_01__TEXT is one of only two segments in
///   the project whose name minus _TEXT is not 8.3-legal, so the explicit
///   spelling is unavoidable. TASM and TLINK compare segment names
///   case-insensitively, so this and the dump's `maine_01__TEXT` are one
///   segment.
/// • -zP, because th05_maine.asm declares
///   `group_01 group CUTSCENE_TEXT, maine_01_TEXT, SCORE_TEXT, maine_01__TEXT`
///   and space_window_set() lives in SCORE_TEXT. The call stays near only
///   because the group spans both segments (kb/codegen/0104).
#pragma option -zCMAINE_01__TEXT -zPgroup_01

#include "th05/staff.hpp"
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/v_colors.hpp"
#include "th02/hardware/frmdelay.h"
#include "th04/math/vector.hpp"
#include "th04/gaiji/gaiji.h"
#include "th04/formats/cdg.h"
#include "th05/snd/snd.h"

// Storage that stays in th05_maine.asm because still-ASM procedures further
// down the segment share it, reached through kb/codegen/0123 zero-byte
// aliases. The dump's own references keep its original spellings.
// ----------------------------------------------------------------------
extern "C" {

// Scratch point that vector2_at() writes before the result is widened into a
// particle's 32-bit [center]. Both of its users — orb_burst() and
// orb_particle_emit() — are in this file; it is a global rather than a local
// only because the two coordinate types differ in width.
extern SPPoint particle_spawn_center;

// The VRAM page this scene shows. Set with the same idiom TH02 and TH03 spell
// out in C++ — `graph_accesspage(page_shown); graph_showpage(page_shown =
// (1 - page_shown));` (th02/op_04.cpp:189, th03/op/m_select.cpp:615) — so it
// carries upstream's own name for the variable, scoped because this binary
// already links a different [page_shown]. Four of its five readers use its
// value as a 0/1 frame parity rather than as a page.
extern page_t staffroll_page_shown;

// Frames since the space scene started. Zeroed by staffroll_animate(),
// incremented once per rendered frame, and divided by 22 to stand in for
// [measure] whenever snd_bgm_measure() has nothing to report.
extern int staffroll_frame;

// Round-robin cursor into [particles], upstream's own name for it in the dump
// minus the address. orb_particle_emit() is its only user.
extern int particle_i;

// Frames since orb_phase_update() first ran, i.e. since the orb finished
// gathering. Its only user is orb_phase_update(), which scripts the whole orb
// phase off it — nothing ever resets it.
extern int orb_phase_frame;

// Which half of the camera's vertical speed oscillation the orb phase is in:
// 0 while [space_camera_velocity].y is still being accelerated towards
// to_sp(11.375f), 1 while it is being braked back towards to_sp(11.125f).
// orb_phase_update() is its only user, and flips it at each end.
extern bool orb_phase_camera_slowing;

// Cursor into [particles] for the rain phase. Unlike [particle_i] this one
// does not wrap: rain_particle_spawn() stops recycling particles once it has
// walked the whole array once. Its only user is rain_particle_spawn().
extern int rain_particle_i;

// Frames since rain_phase_update() first ran, i.e. since the orb burst. Its
// only user is rain_phase_update(), which scripts the rain phase off it and
// clamps it just below 30000 so it cannot overflow during a long staff roll.
extern int rain_phase_frame;

// Set by everything that leaves gaiji or text behind on the text layer, and
// acted on once per frame by staffroll_frame_and_flip(), which calls
// text_clear() and clears this again.
extern bool text_clear_pending;

// Steps the orb's own cel, once per rendered frame in which the orb is a
// finished circle. Its only user is space_put(); nothing ever resets it, so it
// simply wraps around its own 8-bit range.
extern unsigned char orb_cel_frame;

// The measure snd_bgm_measure() last reported for the staff roll BGM, which is
// what both credit lines time their fade-out on. staffroll_frame_and_flip()
// writes it every frame, falling back to ([staffroll_frame] / 22) whenever
// snd_bgm_measure() has nothing to report. Scoped because this binary already
// links a different [measure_cur], in th05/end/allcast.cpp.
extern int staffroll_measure_cur;

// The two credit lines' state machines. Each counts 0 -> 5 and back to 0 over
// one line: 0 and 1 blit the image (once per VRAM page), 2 and 3 fade it in,
// 4 fades it back out, and 5 erases it. Every value is used on two consecutive
// frames because the scene is double-buffered.
extern int credit_phase;
extern int credit_2_phase;

// Each line's fade curtain: the cel of the column the curtain currently leads
// with, and the frame counter that steps it. Both are reset at the start of
// every line, and both are only ever touched by the line's own function.
extern int credit_fade_cel;
extern int credit_fade_frame;
extern int credit_2_fade_cel;
extern int credit_2_fade_frame;

// Two HALF-width text cells' worth of spaces -- the dump stores two 0x20 bytes
// and a terminator, and a half-width cell is GLYPH_HALF_W by GLYPH_H, so the
// two of them cover one 16x16 cell's worth of area between them, not two.
// Drawn in reverse video so that they come out solid black. credit_animate()
// tiles the rectangle its image is about to cover with these, to hide whatever
// the previous line left on the text layer. ZUN emitted one copy per line
// rather than sharing a single string.
extern const char CREDIT_BLACK[];
extern const char CREDIT_2_BLACK[];

}
// ----------------------------------------------------------------------

// The space window's edges, in the entities' own coordinate system, whose
// origin is the window's center.
#define space_left()	(-space_window_w * (SUBPIXEL_FACTOR / 2))
#define space_right()	(space_window_w * (SUBPIXEL_FACTOR / 2))
#define space_top()	(-space_window_h * (SUBPIXEL_FACTOR / 2))
#define space_bottom()	(space_window_h * (SUBPIXEL_FACTOR / 2))

// A random position inside the space window.
#define space_random_x() \
	(TO_SP(irand() % space_window_w) - space_right())
#define space_random_y() \
	(TO_SP(irand() % space_window_h) - space_bottom())

void near space_reset(void)
{
	orb_particle_t near *p = particles;
	SPPoint near *trail_center = orb_trails_center;
	SPPoint near *star_center = stars_center;
	int i;

	space_window_set((RES_X / 2), (RES_Y / 2), 384, 320);
	particle_decel = 0;

	for(i = 0; i < ORB_PARTICLE_COUNT; i++, p++) {
		p->center.x.v = space_random_x();
		p->center.y.v = space_random_y();
		p->angle = irand();
		p->speed.set(0.625f);
		p->patnum_tiny = PAT_ORB_PARTICLE;
		p->al.rain_sway_x_direction = X_RIGHT;
		vector2_at(p->velocity, 0, 0, p->speed, p->angle);
	}
	// p == &orb
	p->center.x.v = Subpixel::None();
	p->al.radius = 0;
	p->velocity.x.v = 0;
	p->velocity.y.v = 0;
	space_camera_velocity.x.v = 0;
	space_camera_velocity.y.v = 0;

	for(i = 0; i < ORB_TRAIL_COUNT; i++, trail_center++) {
		trail_center->x.v = Subpixel::None();
	}
	for(i = 0; i < STAR_COUNT; i++, star_center++) {
		star_center->x.v = space_random_x();
		star_center->y.v = space_random_y();
	}
}

// The three functions below have neither parameters nor stack locals, so the
// build's global -k- would drop their BP frame — which the originals have.
// kb/codegen/0042, restored to -k- immediately afterwards. space_reset() and
// space_update() need no such wrapper: both have stack locals, so -k- emits
// an `enter` for them on its own.
#pragma option -k.

// Turns every particle towards the center of space at a speed proportional to
// its distance from it, and activates the orb they are gathering into.
void near orb_gather_start(void)
{
	orb_particle_t near *p = particles;
	int i;

	for(i = 0; i < ORB_PARTICLE_COUNT; i++, p++) {
		p->angle = iatan2(
			-static_cast<subpixel_t>(p->center.y.v),
			-static_cast<subpixel_t>(p->center.x.v)
		);
		p->speed.v = (ihypot(
			static_cast<subpixel_t>(p->center.x.v),
			static_cast<subpixel_t>(p->center.y.v)
		) / 32);
		p->gather_frame = 0;
		vector2_at(p->velocity, 0, 0, p->speed, p->angle);
	}
	// p == &orb
	p->center.x.v = 0;
	p->center.y.v = 0;
	p->al.radius = 1;
}

// Ends the gather phase: every particle has arrived, so they all go away and
// the orb is complete. From here on, the particles orb_particle_emit() throws
// off the orb decay instead of wrapping around the window.
void near orb_gather_end(void)
{
	orb_particle_t near *p = particles;
	int i;

	for(i = 0; i < ORB_PARTICLE_COUNT; i++, p++) {
		p->center.x.v = Subpixel::None();
	}
	// p == &orb
	p->al.radius = ORB_RADIUS_FULL;
	particle_decel = 1;
}

// Blows the orb apart into the particle field again.
void near orb_burst(void)
{
	orb_particle_t near *p = particles;
	int i;

	for(i = 0; i < ORB_PARTICLE_COUNT; i++, p++) {
		// ZUN bloat: These two are copied from orb_gather_start() and are
		// immediately overwritten by the random ones below.
		p->angle = iatan2(
			-static_cast<subpixel_t>(p->center.y.v),
			-static_cast<subpixel_t>(p->center.x.v)
		);
		p->speed.v = (ihypot(
			static_cast<subpixel_t>(p->center.x.v),
			static_cast<subpixel_t>(p->center.y.v)
		) / 32);

		p->patnum_tiny = PAT_ORB_PARTICLE;
		p->angle = (irand() & 0x7F);
		p->speed.v = ((irand() % to_sp(4.0f)) + to_sp(5.75f));
		vector2_at(
			particle_spawn_center,
			static_cast<subpixel_t>(orb.center.x.v),
			static_cast<subpixel_t>(orb.center.y.v),
			to_sp(12.0f),
			p->angle
		);
		vector2_at(p->velocity, 0, 0, p->speed, p->angle);
		p->center.x.v = particle_spawn_center.x.v;
		p->center.y.v = particle_spawn_center.y.v;
	}
	// p == &orb
	p->center.x.v = Subpixel::None();
	particle_decel = 1;
}

#pragma option -k-

// Moves every entity by its own velocity and the camera's, and either wraps it
// around the space window or decays it, depending on [particle_decel].
void near space_update(void)
{
	orb_particle_t near *p = particles;
	int i;
	SPPoint near *trail_center = orb_trails_center;
	SPPoint near *star_center = stars_center;

	for(i = 0; i < ORB_PARTICLE_COUNT; i++, p++) {
		if(p->center.x.v == Subpixel::None()) {
			continue;
		}
		p->center.x.v += (p->velocity.x.v - space_camera_velocity.x.v);
		p->center.y.v += (p->velocity.y.v - space_camera_velocity.y.v);
		if(particle_decel == 0) {
			if((space_left() + to_sp(-4.0f)) >= p->center.x.v) {
				p->center.x.v += (TO_SP(space_window_w) + to_sp(8.0f));
			} else if((space_right() + to_sp(4.0f)) <= p->center.x.v) {
				p->center.x.v -= (TO_SP(space_window_w) + to_sp(8.0f));
			}
			if((space_top() + to_sp(-4.0f)) >= p->center.y.v) {
				p->center.y.v += (TO_SP(space_window_h) + to_sp(8.0f));
			} else if((space_bottom() + to_sp(4.0f)) <= p->center.y.v) {
				p->center.y.v -= (TO_SP(space_window_h) + to_sp(8.0f));
			}
		}
		if(particle_decel == 0) {
			continue;
		}
		p->speed.v -= particle_decel;
		if(p->speed.v == 0) {
			p->center.x.v = Subpixel::None();
			continue;
		}
		if(p->speed.v > to_sp(2.1875f)) {
			p->patnum_tiny = PAT_ORB_PARTICLE;
		} else if((p->speed.v % 7) == 0) {
			p->patnum_tiny++;
		}
		vector2_at(p->velocity, 0, 0, p->speed, p->angle);
	}

	for(i = 0; i < ORB_TRAIL_COUNT; i++, trail_center++) {
		if(trail_center->x.v == Subpixel::None()) {
			continue;
		}
		trail_center->x.v -= space_camera_velocity.x.v;
		trail_center->y.v -= space_camera_velocity.y.v;
	}

	for(i = 0; i < STAR_COUNT; i++, star_center++) {
		if((i % 2) == 0) {
			star_center->x.v -= space_camera_velocity.x.v;
			star_center->y.v -= space_camera_velocity.y.v;
		} else {
			star_center->x.v -= (space_camera_velocity.x.v / 2);
			star_center->y.v -= (space_camera_velocity.y.v / 2);
		}
		if(space_camera_velocity.x.v == 0) {
			if(
				(space_left() >= star_center->x.v) ||
				(space_right() <= star_center->x.v)
			) {
				star_center->x.v = space_random_x();
				star_center->y.v = space_bottom();
			}
		} else if(space_left() >= star_center->x.v) {
			star_center->x.v = space_right();
		} else if(space_right() <= star_center->x.v) {
			star_center->x.v = space_left();
		}
		if(space_top() >= star_center->y.v) {
			star_center->y.v = space_bottom();
		} else if(space_bottom() <= star_center->y.v) {
			star_center->y.v = space_top();
		}
	}

	p = &orb;
	if(p->center.x.v != Subpixel::None()) {
		p->center.x.v += (p->velocity.x.v - space_camera_velocity.x.v);
		p->center.y.v += (p->velocity.y.v - space_camera_velocity.y.v);
		if((p->al.radius >= ORB_RADIUS_FULL) && (staffroll_page_shown != 0)) {
			orb_particle_emit();
			if(p->velocity.y.v < to_sp(11.25f)) {
				p->velocity.y.v++;
			}
		}
	}
	if((staffroll_frame % 3) == 0) {
		orb_trails_advance();
	}
}

// None of the three below has parameters or stack locals (kb/codegen/0149).
#pragma option -k.

// Recycles the next particle in [particles] as a spark thrown off the orb.
void near orb_particle_emit(void)
{
	orb_particle_t near *p = &particles[particle_i++];

	p->patnum_tiny = PAT_ORB_PARTICLE;
	p->angle = (irand() & 0x7F);
	p->speed.set(3.0f);
	vector2_at(
		particle_spawn_center,
		static_cast<subpixel_t>(orb.center.x.v),
		static_cast<subpixel_t>(orb.center.y.v),
		to_sp(12.0f),
		p->angle
	);
	p->center.x.v = particle_spawn_center.x.v;
	p->center.y.v = particle_spawn_center.y.v;
	if(particle_i >= ORB_PARTICLE_COUNT) {
		particle_i = 0;
	}
}

// Shifts the orb's position history down by one and records where it is now.
void near orb_trails_advance(void)
{
	int i;

	for(i = (ORB_TRAIL_COUNT - 2); i >= 0; i--) {
		orb_trails_center[i + 1].x.v = orb_trails_center[i].x.v;
		orb_trails_center[i + 1].y.v = orb_trails_center[i].y.v;
	}
	orb_trails_center[0].x.v = static_cast<subpixel_t>(orb.center.x.v);
	orb_trails_center[0].y.v = static_cast<subpixel_t>(orb.center.y.v);
}

// Advances the gather animation by one frame: every particle cycles through
// the stf01.bft cels as it flies in, and the orb it is gathering into grows
// until it is a full circle. staffroll_animate() runs this exactly 32 times
// between orb_gather_start() and orb_gather_end().
void near orb_gather_animate(void)
{
	orb_particle_t near *p = particles;
	int i;

	for(i = 0; i < ORB_PARTICLE_COUNT; i++, p++) {
		p->gather_frame++;
		if(
			((p->gather_frame % 8) == 0) &&
			(p->patnum_tiny < PAT_ORB_PARTICLE_last)
		) {
			p->patnum_tiny++;
		}
	}
	// p == &orb
	if(p->al.radius < ORB_RADIUS_FULL) {
		p->al.radius += staffroll_page_shown;
	}
}

#pragma option -k-

// One frame of the orb phase — the stretch between orb_gather_end() and
// orb_burst(), during which the credits are shown next to the finished orb.
// Accelerates the camera downwards to its cruising speed, then holds that
// speed by oscillating around it; from frame 234 on, slides the space window
// off to the left and stretches it vertically to clear the right half of the
// screen for the credit images. Returns whether the scripted slide is over,
// which is what staffroll_animate() waits for before showing the first line.
bool16 near orb_phase_update(void)
{
	int frames;
	int center_shift;

	orb_phase_frame++;
	if(orb_phase_frame < 80) {
		return false;
	}
	if(orb_phase_frame < 176) {
		space_camera_velocity.y.v++;
	} else if(orb_phase_frame < 344) {
		// Every other frame, since [staffroll_page_shown] alternates.
		space_camera_velocity.y.v += staffroll_page_shown;
	} else if(!orb_phase_camera_slowing) {
		space_camera_velocity.y.v += ((staffroll_frame % 16) == 0);
		if(space_camera_velocity.y.v > to_sp(11.375f)) {
			orb_phase_camera_slowing++;
		}
	} else {
		space_camera_velocity.y.v -= ((staffroll_frame % 16) == 0);
		if(space_camera_velocity.y.v < to_sp(11.125f)) {
			orb_phase_camera_slowing--;
		}
	}
	if(orb_phase_frame < 234) {
		return false;
	}
	if(orb_phase_frame < 512) {
		frames = (orb_phase_frame - 234);
		center_shift = ((frames > 88) ? (frames - 88) : 0);
		space_window_set(
			((RES_X / 2) - center_shift),
			(RES_Y / 2),
			(384 - (frames / 2)),
			((frames / 8) + 320)
		);
	} else {
		if((orb_phase_frame >= 992) && (orb_phase_frame <= 1024)) {
			orb.velocity.x.v--;
		}
		if((orb_phase_frame >= 1008) && (orb_phase_frame <= 1040)) {
			space_camera_velocity.x.v--;
		}
		return true;
	}
	return false;
}

// Neither of the two below has parameters or stack locals (kb/codegen/0149).
#pragma option -k.

// Recycles the next particle in [particles] as a raindrop entering the space
// window from above at a random speed, angle and cel. Unlike
// orb_particle_emit() this one does not wrap around: once [rain_particle_i]
// has walked the whole array, every further call is a no-op and the rain stops
// thickening.
void near rain_particle_spawn(void)
{
	orb_particle_t near *p = &particles[rain_particle_i];

	// Unlike orb_particle_emit()'s post-increment in the subscript, this one
	// is a pre-increment inside the condition: the cursor is bumped after the
	// pointer is taken, and it is the NEW value that is range-checked, so the
	// last element of [particles] is skipped rather than the orb overwritten.
	if(++rain_particle_i < ORB_PARTICLE_COUNT) {
		p->speed.v = ((irand() % to_sp(1.5f)) + to_sp(0.5f));
		p->angle = ((irand() % 0x40) + 0x20);
		p->patnum_tiny = (irand() % ORB_PARTICLE_CELS);
		p->center.x.v = space_random_x();
		p->center.y.v = (space_top() - to_sp(4.0f));
		p->al.rain_sway_x_direction = ((p->angle < 0x40) ? X_RIGHT : X_LEFT);
		vector2_at(p->velocity, 0, 0, p->speed, p->angle);
	}
}

// One frame of the rain phase — everything after orb_burst(). Undoes the orb
// phase's camera script, slides the space window back to the right until it
// covers the screen, and turns the burst particles into rain: the field goes
// back to its drifting mode, a new raindrop is added every 8 frames, and every
// particle's angle sways between 0x20 and 0x60 so the rain looks blown about.
// Returns whether the window has finished sliding.
bool16 near rain_phase_update(void)
{
	orb_particle_t near *p = particles;
	int i;

	if(space_camera_velocity.x.v < 0) {
		space_camera_velocity.x.v += staffroll_page_shown;
	}
	rain_phase_frame++;
	if(rain_phase_frame < 32) {
		return false;
	}
	if(rain_phase_frame < 128) {
		space_camera_velocity.y.v--;
	} else if(rain_phase_frame < 308) {
		space_camera_velocity.y.v -= staffroll_page_shown;
	} else {
		if(
			((space_window_w / 2) + space_window_center.x) < (RES_X - 10)
		) {
			space_window_set(
				(space_window_center.x + 4),
				(RES_Y / 2),
				space_window_w,
				space_window_h
			);
		}
		particle_decel = 0;
		if((rain_phase_frame % 8) == 0) {
			rain_particle_spawn();
			for(i = 0; i < ORB_PARTICLE_COUNT; i++, p++) {
				if(p->al.rain_sway_x_direction == X_RIGHT) {
					p->angle++;
					if(p->angle >= 0x60) {
						p->al.rain_sway_x_direction = X_LEFT;
					}
				} else {
					p->angle--;
					if(p->angle <= 0x20) {
						p->al.rain_sway_x_direction = X_RIGHT;
					}
				}
				vector2_at(p->velocity, 0, 0, p->speed, p->angle);
			}
		}
		// Keeps this counter from overflowing during a long staff roll — and
		// 29992 is 8 below the limit, so both cycles above stay in phase
		// across the wrap. [inferred] 30000 frames is about 9 minutes, so
		// whether a real staff roll ever gets here was not measured.
		if(rain_phase_frame >= 30000) {
			rain_phase_frame = 29992;
		}
		if(((space_window_w / 2) + space_window_center.x) >= (RES_X - 10)) {
			return true;
		}
	}
	if(((rain_phase_frame % 4) == 0) && (Palettes[0].c.b < 96)) {
		Palettes[0].c.b++;
		palette_show();
	}
	return false;
}

#pragma option -k-

/// The credit lines
/// ----------------

/// master.lib's GRCG_OFF_CLOBBERING macro, which spills the port number to DX
/// rather than using the 8-bit immediate form. Spelled the same way
/// th04/end/staff_dissolve.cpp and th04/main/stage/loop.cpp spell it.
#define grcg_off_clobbering_dx() outportb(0x7C, GC_OFF)

// One curtain cel per [CREDIT_FADE_INTERVAL] frames.
static const int CREDIT_FADE_INTERVAL = 4;

// Anything past the far end of a [w]-wide image's curtain travel, i.e. past
// ((w / GAIJI_W) + (STAFF_FADE_CELS - 1)). Passed as the [cel] of the final,
// unconditional erase, which therefore always takes credit_fade_put()'s
// CREDIT_FADE_ERASED branch.
static const int CREDIT_CEL_PAST_END = 128;

// Which end of its travel the curtain hit on the frame credit_fade_put() just
// drew. (-b- makes this an `unsigned char`, which is why the originals set
// only AL.)
enum credit_fade_ret_t {
	// Neither end: the curtain is somewhere across the image, and was drawn.
	CREDIT_FADE_ACTIVE = 0,

	// Lifted. The leading column has passed the near end, so every column
	// would have been g_EMPTY and nothing was drawn at all — but the gaiji of
	// the previous frame are still on the text layer, hence the
	// [text_clear_pending].
	CREDIT_FADE_LIFTED = 1,

	// Closed. The leading column has passed the far end, so every column would
	// have been g_STAFF_FADE. Rather than drawing that, this function takes the
	// image out of the current VRAM page for good.
	CREDIT_FADE_ERASED = 2,
};

// Draws one frame of the curtain that fades the [w]x[h] credit image at
// ([left], [top]) in or out, as a rectangle of gaiji on the text layer. [cel]
// is the curtain cel of the column the curtain leads with — the leftmost one
// for a [cel_delta] of 1, the rightmost one for -1 — and every further column
// trails one cel further behind it, which is what turns a straight fade into a
// sideways wipe. Advancing [cel] by 1 per call therefore moves the whole wipe
// by one column.
static credit_fade_ret_t pascal near credit_fade_put(
	screen_x_t left, vram_y_t top, pixel_t w, pixel_t h, int cel, int cel_delta
)
{
	int i;
	int col;

	// The frame is 0x2C bytes with [col] nearest BP, so this is 42 rather than
	// the 41 the loop below can actually reach (a 640-pixel-wide image is 40
	// gaiji plus the terminator). [inferred]: ZUN's slack, not a live element.
	char cels[42];

	if(cel < 0) {
		text_clear_pending = true;
		return CREDIT_FADE_LIFTED;
	}
	if(((w / GAIJI_W) + (STAFF_FADE_CELS - 1)) < cel) {
		grcg_setcolor(GC_RMW, 1);
		grcg_byteboxfill_x(
			(left / BYTE_DOTS),
			top,
			((left + w - 1) / BYTE_DOTS),
			(top + h - 1)
		);
		grcg_off_clobbering_dx();
		return CREDIT_FADE_ERASED;
	}

	// From pixels to the text layer's own units. gaiji_putsa() takes an X in
	// 8-pixel cells but advances by 16 pixels per character.
	left /= BYTE_DOTS;
	top /= GLYPH_H;
	w /= GAIJI_W;
	h /= GLYPH_H;

	if(cel_delta > 0) {
		col = 0;
	} else {
		col = (w - 1);
	}
	for(i = 0; i < w; i++, col += cel_delta, cel--) {
		if(cel >= (STAFF_FADE_CELS - 1)) {
			cels[col] = g_STAFF_FADE;
		} else if(cel < 0) {
			cels[col] = g_EMPTY;
		} else {
			cels[col] = (g_STAFF_FADE_last - cel);
		}
		// The image can extend past the right edge of the text layer, and
		// gaiji_putsa() would wrap around to the next row rather than clip.
		if(((i * GAIJI_TRAM_W) + left) >= text_width()) {
			break;
		}
	}
	cels[i] = '\0';

	for(i = 0; i <= h; i++, top++) {
		gaiji_putsa(left, top, cels, TX_BLACK);
	}
	return CREDIT_FADE_ACTIVE;
}

bool16 pascal near credit_animate(
	screen_x_t x_center, vram_y_t y_center, int slot, int measure
)
{
	CDG near *cdg = &cdg_slots[slot];
	int y_off;
	int x_off;

	x_center -= (cdg->pixel_w / 2);
	y_center -= (cdg->pixel_h / 2);
	if(credit_phase <= 1) {
		credit_fade_cel = ((cdg->pixel_w / GAIJI_W) + (STAFF_FADE_CELS - 1));
		credit_fade_frame = 0;
		credit_phase++;
		cdg_put_noalpha_8(x_center, y_center, slot);

		// Blacks out the text layer behind the image, one gaiji-sized cell at
		// a time, so that the previous line's leftover curtain cannot show
		// through this one.
		for(
			y_off = 0;
			cdg->pixel_h >= y_off;
			y_off += GLYPH_H, y_center += GLYPH_H
		) {
			for(x_off = 0; cdg->pixel_w >= x_off; x_off += GAIJI_W) {
				if(
					((x_center / BYTE_DOTS) + (x_off / BYTE_DOTS)) <
					text_width()
				) {
					text_putsa(
						((x_center / BYTE_DOTS) + (x_off / BYTE_DOTS)),
						(y_center / GLYPH_H),
						CREDIT_BLACK,
						(TX_BLACK | TX_REVERSE)
					);
				}
			}
		}
	} else if(credit_phase <= 3) {
		if((++credit_fade_frame % CREDIT_FADE_INTERVAL) == 0) {
			credit_fade_cel--;
		}
		if(credit_fade_put(
			x_center, y_center, cdg->pixel_w, cdg->pixel_h, credit_fade_cel, 1
		) == CREDIT_FADE_LIFTED) {
			credit_phase++;
			credit_fade_cel = 0;
			credit_fade_frame = 0;
		}
	} else if(
		(staffroll_measure_cur >= measure) && (measure != CREDIT_MEASURE_HOLD)
	) {
		if(credit_phase == 4) {
			if((++credit_fade_frame % CREDIT_FADE_INTERVAL) == 0) {
				credit_fade_cel++;
			}
			if(credit_fade_put(
				x_center, y_center, cdg->pixel_w, cdg->pixel_h,
				credit_fade_cel, -1
			) == CREDIT_FADE_ERASED) {
				credit_phase++;
			}
		} else {
			// Both pages have been erased by now, so this call only exists to
			// take the image out of the one that was just flipped to.
			credit_fade_put(
				x_center, y_center, cdg->pixel_w, cdg->pixel_h,
				CREDIT_CEL_PAST_END, -1
			);
			credit_phase = 0;
			text_clear_pending = true;
			return true;
		}
	}
	return false;
}

bool16 pascal near credit_2_animate(
	screen_x_t x_center, vram_y_t y_center, int slot, int measure
)
{
	CDG near *cdg = &cdg_slots[slot];
	int y_off;
	int x_off;

	x_center -= (cdg->pixel_w / 2);
	y_center -= (cdg->pixel_h / 2);
	if(credit_2_phase <= 1) {
		credit_2_fade_cel = ((cdg->pixel_w / GAIJI_W) + (STAFF_FADE_CELS - 1));
		credit_2_fade_frame = 0;
		credit_2_phase++;
		cdg_put_noalpha_8(x_center, y_center, slot);
		for(
			y_off = 0;
			cdg->pixel_h >= y_off;
			y_off += GLYPH_H, y_center += GLYPH_H
		) {
			for(x_off = 0; cdg->pixel_w >= x_off; x_off += GAIJI_W) {
				if(
					((x_center / BYTE_DOTS) + (x_off / BYTE_DOTS)) <
					text_width()
				) {
					text_putsa(
						((x_center / BYTE_DOTS) + (x_off / BYTE_DOTS)),
						(y_center / GLYPH_H),
						CREDIT_2_BLACK,
						(TX_BLACK | TX_REVERSE)
					);
				}
			}
		}
	} else if(credit_2_phase <= 3) {
		if((++credit_2_fade_frame % CREDIT_FADE_INTERVAL) == 0) {
			credit_2_fade_cel--;
		}
		if(credit_fade_put(
			x_center, y_center, cdg->pixel_w, cdg->pixel_h, credit_2_fade_cel, 1
		) == CREDIT_FADE_LIFTED) {
			credit_2_phase++;
			credit_2_fade_cel = 0;
			credit_2_fade_frame = 0;
		}
	} else if(
		(staffroll_measure_cur >= measure) && (measure != CREDIT_MEASURE_HOLD)
	) {
		if(credit_2_phase == 4) {
			if((++credit_2_fade_frame % CREDIT_FADE_INTERVAL) == 0) {
				credit_2_fade_cel++;
			}
			if(credit_fade_put(
				x_center, y_center, cdg->pixel_w, cdg->pixel_h,
				credit_2_fade_cel, -1
			) == CREDIT_FADE_ERASED) {
				credit_2_phase++;
			}
		} else {
			credit_fade_put(
				x_center, y_center, cdg->pixel_w, cdg->pixel_h,
				CREDIT_CEL_PAST_END, -1
			);
			credit_2_phase = 0;
			text_clear_pending = true;
			return true;
		}
	}
	return false;
}
/// ----------------

/// The renderer
/// ------------

// The color the innermost orb trail is drawn in. Each further one out is drawn
// in the next hardware color, which is why the trail fades from red to white.
static const vc2 COL_TRAIL_INNERMOST = 6;

// The border that space_put() paints over on all four sides of the space
// window, and that space_window_set() therefore includes in its clipping
// rectangle. Anything the window covered before it moved has to end up inside
// it, or it would be left on screen forever.
static const pixel_t SPACE_BORDER = 8;

// One orb cel per [ORB_FRAMES_PER_CEL] rendered frames, cycling through the
// [ORB_CELS] of them that th05/staff.hpp declares beside [PAT_ORB].
//
// Spelled the way th01/main/player/orb.hpp already spells this exact quantity
// -- a different value there, but the same role in the same expression shape,
// a pattern-number base plus a frame counter divided by it. The
// _FRAMES_PER_CEL suffix has 25 other identifiers tree-wide; nothing else ends
// in _CEL_FRAMES at all.
static const int ORB_FRAMES_PER_CEL = 4;

// Distance the stars and particles are blitted up and to the left of their own
// center, i.e. half of a small .BFT cel.
static const pixel_t TINY_SMALL_RADIUS = 4;

void near space_put(void)
{
	orb_particle_t near *p = &orb;
	int i;
	screen_x_t x;
	screen_y_t y;
	vc2 trail_col;
	SPPoint near *trail_center = &orb_trails_center[ORB_TRAIL_COUNT - 1];
	SPPoint near *star_center = stars_center;

	// ZUN bloat: TDW mode would have been faster, exactly as it would have
	// been for the equivalent clears in th05/end/allcast.cpp.
	grcg_setcolor(GC_RMW, 0);
	grcg_boxfill(
		(space_window_center.x - (space_window_w / 2)),
		(space_window_center.y - (space_window_h / 2)),
		((space_window_center.x + (space_window_w / 2)) - 1),
		((space_window_center.y + (space_window_h / 2)) - 1)
	);
	grcg_off_clobbering_dx();

	for(i = 0; i < STAR_COUNT; i++, star_center++) {
		x = (
			(star_center->x.to_pixel_slow() + space_window_center.x) -
			TINY_SMALL_RADIUS
		);
		y = (
			(star_center->y.to_pixel_slow() + space_window_center.y) -
			TINY_SMALL_RADIUS
		);
		super_put_tiny_small(x, y, ((i % 2) + PAT_STAR_BIG));
	}

	// Walked from the oldest trail position to the newest, so that the newest
	// one ends up on top and in the brightest color.
	for(
		i = 0, trail_col = COL_TRAIL_INNERMOST;
		i < ORB_TRAIL_COUNT;
		i++, trail_center--
	) {
		if(trail_center->x.v == Subpixel::None()) {
			continue;
		}
		x = (trail_center->x.to_pixel_slow() + space_window_center.x);
		y = (trail_center->y.to_pixel_slow() + space_window_center.y);
		grcg_setcolor(GC_RMW, trail_col++);
		grcg_circlefill(x, y, ORB_RADIUS_FULL);
		grcg_off_clobbering_dx();
	}

	if(p->center.x.v != Subpixel::None()) {
		x = (
			(p->center.x.to_pixel_slow() + space_window_center.x) - (ORB_W / 2)
		);
		y = (
			(p->center.y.to_pixel_slow() + space_window_center.y) - (ORB_H / 2)
		);
		if(p->al.radius >= ORB_RADIUS_FULL) {
			i = (((orb_cel_frame / ORB_FRAMES_PER_CEL) % ORB_CELS) + PAT_ORB);
			super_put_rect(x, y, i);
			orb_cel_frame++;
		} else {
			// Still gathering, so there is no cel for this size yet.
			grcg_setcolor(GC_RMW, V_WHITE);
			grcg_circlefill(
				(x + (ORB_W / 2)), (y + (ORB_H / 2)), p->al.radius
			);
			grcg_off_clobbering_dx();
		}
	}

	p--; // p == &particles[ORB_PARTICLE_COUNT - 1]

	// Backwards, so that the most recently emitted spark is drawn last.
	for(i = 0; i < ORB_PARTICLE_COUNT; i++, p--) {
		if(p->center.x.v == Subpixel::None()) {
			continue;
		}

		// super_put_tiny_small() does not clip, and grc_setclip() only applies
		// to master.lib's own drawing functions, so every particle outside the
		// window has to be rejected here.
		if((space_left() + to_sp(-4.0f)) >= p->center.x.v) {
			continue;
		}
		if((space_right() + to_sp(4.0f)) <= p->center.x.v) {
			continue;
		}
		if((space_top() + to_sp(-4.0f)) >= p->center.y.v) {
			continue;
		}
		if((space_bottom() + to_sp(4.0f)) <= p->center.y.v) {
			continue;
		}
		x = (
			(p->center.x.to_pixel_slow() + space_window_center.x) -
			TINY_SMALL_RADIUS
		);
		y = (
			(p->center.y.to_pixel_slow() + space_window_center.y) -
			TINY_SMALL_RADIUS
		);
		super_put_tiny_small(x, y, p->patnum_tiny);
	}

	// The border: left, right, top, bottom.
	grcg_setcolor(GC_RMW, 1);
	grcg_boxfill(
		((space_window_center.x - (space_window_w / 2)) - SPACE_BORDER),
		(space_window_center.y - (space_window_h / 2)),
		((space_window_center.x - (space_window_w / 2)) - 1),
		((space_window_center.y + (space_window_h / 2)) - 1)
	);
	grcg_boxfill(
		(space_window_center.x + (space_window_w / 2)),
		(space_window_center.y - (space_window_h / 2)),
		(
			(space_window_center.x + (space_window_w / 2)) +
			(SPACE_BORDER - 1)
		),
		((space_window_center.y + (space_window_h / 2)) - 1)
	);
	grcg_boxfill(
		((space_window_center.x - (space_window_w / 2)) - SPACE_BORDER),
		((space_window_center.y - (space_window_h / 2)) - SPACE_BORDER),
		(
			(space_window_center.x + (space_window_w / 2)) +
			(SPACE_BORDER - 1)
		),
		((space_window_center.y - (space_window_h / 2)) - 1)
	);
	grcg_boxfill(
		((space_window_center.x - (space_window_w / 2)) - SPACE_BORDER),
		(space_window_center.y + (space_window_h / 2)),
		(
			(space_window_center.x + (space_window_w / 2)) +
			(SPACE_BORDER - 1)
		),
		(
			(space_window_center.y + (space_window_h / 2)) +
			(SPACE_BORDER - 1)
		)
	);
	grcg_off_clobbering_dx();
}

// kb/codegen/0042: neither parameters nor stack locals, so the BP frame the
// original has needs -k restored around the body.
#pragma option -k.

// One frame of the scene, rendered onto the page that is not currently being
// shown and then flipped in. Spelled the same way th05/regist.cpp spells
// regist_frame_and_flip().
void near staffroll_frame_and_flip(void)
{
	space_update();
	space_put();
	frame_delay(1);
	graph_accesspage(staffroll_page_shown);
	graph_showpage(staffroll_page_shown = (1 - staffroll_page_shown));
	if(text_clear_pending) {
		text_clear();
		text_clear_pending = false;
	}
	staffroll_frame++;
	staffroll_measure_cur = snd_bgm_measure();
	if(staffroll_measure_cur < 0) {
		// The staff roll's own track is 「Mystic Dream」, not the 「Days」 an
		// earlier revision of this comment named: that is TH04's staff-roll
		// theme (th04/shiftjis/bgm.hpp's TH04_19, its only definition in the
		// tree). th05/shiftjis/music.hpp puts "staff" at the MUSIC_FILES index
		// whose MUSIC_CHOICES title is TH05_21, one entry before the "exed" /
		// TH05_22 「Peaceful Romancer」 pair that upstream's own comment in
		// th05/end/allcast.cpp names for that file's copy of this fallback.
		// [measured]
		//
		// This copy is NOT allcast's. Its
		// wait_flip_and_check_measure_target() calls frame_delay(2) and
		// divides the half-frame counter, so its fallback advances once per 22
		// DOUBLE frames; staffroll_frame_and_flip() calls frame_delay(1) and
		// divides [staffroll_frame], so this one advances once per 22 SINGLE
		// frames. [measured]
		//
		// That single-frame divisor is the figure allcast.cpp argues ZUN
		// actually intended — about 153.9 BPM rather than the 76.9 its own
		// doubled copy produces — so this fallback is the corrected shape of
		// the one that file labels a ZUN bug, and importing that label here
		// would invert it. Whether 153.9 BPM is right for 「Mystic Dream」 is
		// an OPEN taxonomy question: that song's tempo is not measurable from
		// this tree, so the label is unsupported rather than refuted, and no
		// marker is carried for it. [not measured]
		staffroll_measure_cur = (staffroll_frame / 22);
	}
}

#pragma option -k-
/// ------------
