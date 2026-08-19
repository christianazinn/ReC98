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

// Neither of these two has parameters or stack locals either (kb/codegen/0149).
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

#pragma option -k-
