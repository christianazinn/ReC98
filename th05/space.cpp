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
#include "th04/math/vector.hpp"

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
