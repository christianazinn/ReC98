/// TH05-specific stage callbacks
/// -----------------------------
/// TH05 has exactly one: th05/main/stage/setup.cpp installs nullfunc_near into
/// [stage_render] and [stage_invalidate] for every stage except Stage 2. So
/// this file is the TH05 counterpart of th04/main/stage/stages.cpp, and not a
/// `#if (GAME == 5)` arm of it — the two share no code, and TH04's set is
/// already complete.
///
/// Despite the name ZUN's `public` line fixes, stage2_update() is installed as
/// [stage_render], not as an update function. It runs the Stage 2 entrance:
/// a white flash that opens two columns of the tile ring, a one-frame
/// hand-off to hardware scrolling, and then a field of star particles over a
/// background colour that pulses for the rest of the stage.
///
/// The particle respawner it calls, s2particle_respawn(), is the other half of
/// the same effect and lives here too, ahead of it because that is where it
/// sits in the segment. It is where the choreography actually is: the emitter
/// walks a 4096-frame cycle between three positions, and every particle that
/// leaves the playfield comes back out of wherever the emitter has got to.

#include "platform.h"
#include "pc98.h"
#include "x86real.h"
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
// After pc98_gfx.hpp, and required: it `#undef`s master.lib's grcg_off()
// prototype and replaces it with the inline `outportb(0x7C, 0)` macro, which
// is the 6-byte `mov dx, 7Ch` / `mov al, 0` / `out dx, al` the original emits.
// Without it the call links as a 5-byte far call into master.lib and the body
// comes out exactly one byte short.
#include "th01/hardware/grcg.hpp"
// Wraps th02/math/vector.hpp in `extern "C"`, which is the linkage the dump's
// bare `call vector2` needs; including that header directly does not link.
#include "th04/math/vector.hpp"
#include "th03/hardware/palette.hpp"
#include "th04/main/custom.hpp"
#include "th04/main/frames.h"
#include "th04/main/scroll.hpp"
#include "th04/main/boss/boss.hpp"
#include "th04/main/tile/tile.hpp"
#include "th04/math/randring.hpp"
#include "th05/formats/super.h"
#include "th05/sprites/main_pat.h"
#include "th05/main/stage/s2part.hpp"
#include "th05/main/stage/stages.hpp"

// State
// -----

// How many of the S2PARTICLE_COUNT particles have been spawned so far. Also
// the index of the next one: they are spawned one every other frame and never
// freed, so this only ever counts up, and stops at S2PARTICLE_COUNT.
extern int s2particles_spawned;

// The colour level of the background pulse, ping-ponging between
// S2_BG_PULSE_MIN and S2_BG_PULSE_MAX for the rest of the stage.
extern unsigned char stage2_bg_pulse;

// Added to the neutral tone of 100 to give [PaletteTone] during the entrance:
// raised by 2 on every frame of the flash, then decayed back to 0 by 4 per
// frame once the stage proper starts. Also the flag that says which of those
// two the code is doing.
extern unsigned char stage2_flash_tone;

// Direction of the [stage2_bg_pulse] ping-pong; 0 brightens, nonzero dims.
// Same shape and same name as [reimu_bg_pulse_direction]
// (th04/main/boss/b4r.cpp:17), which drives the identical effect for TH04's
// Stage 4 boss — except that ZUN wrote this one with `++`/`--` rather than
// assignments.
extern int8_t stage2_bg_pulse_direction;
// -----

// Constants
// ---------

// The entrance flash covers this range of [stage_frame]; the hand-off to
// hardware scrolling happens on the two frames after it.
static const unsigned int S2_FLASH_FRAME_FIRST = 256;
static const unsigned int S2_FLASH_FRAME_LAST = 304;
static const unsigned int S2_SCROLL_HANDOFF_FRAME_LAST = 306;

// The two tile-ring columns the flash opens outwards from the centre, and the
// VRAM offset it writes into them.
static const int S2_GAP_LEFT_COLUMN = 11;
static const int S2_GAP_RIGHT_COLUMN = 12;
static const vram_offset_t S2_GAP_TILE_VO = (
	TILE_AREA_VRAM_LEFT + (256 * ROW_SIZE)
);

static const unsigned char S2_BG_PULSE_MIN = 0x20;
static const unsigned char S2_BG_PULSE_MAX = 0x3F;

// A particle leaves the emitter at a random angle within
// S2PARTICLE_ANGLE_RANGE of a base, and at a point that is
// S2PARTICLE_PIXELS_PER_ANGLE pixels further right for every angle unit its
// own angle falls short of the top of that range. So the whole spray is a fan
// anchored at one X, and moving the emitter means moving the base and the X
// together.
static const unsigned char S2PARTICLE_ANGLE_RANGE = 0x20;
static const pixel_t S2PARTICLE_PIXELS_PER_ANGLE = 8;
static const subpixel_t S2PARTICLE_SPEED = TO_SP(8);

// The three emitter positions the stage uses, and the angle base that goes
// with each. The entrance spawn and the S2PARTICLE_CYCLE_FRAMES cycle below
// both start from the middle one.
static const unsigned char S2PARTICLE_ANGLE_LEFT = 0x20;
static const unsigned char S2PARTICLE_ANGLE_MID = 0x30;
static const unsigned char S2PARTICLE_ANGLE_RIGHT = 0x40;
static const pixel_t S2PARTICLE_ORIGIN_LEFT = 0;
static const pixel_t S2PARTICLE_ORIGIN_MID = 64;
static const pixel_t S2PARTICLE_ORIGIN_RIGHT = 128;

// The cycle s2particle_respawn() reads its emitter out of: mostly parked at
// one of the three positions, with four S2PARTICLE_SWEEP_FRAMES-long moves
// between them at one angle unit per S2PARTICLE_FRAMES_PER_ANGLE frames and
// S2PARTICLE_PIXELS_PER_ANGLE subpixels of X per frame — which is the same
// ratio the fan itself uses, and why the spray does not visibly bend while
// the emitter travels.
static const int S2PARTICLE_CYCLE_FRAMES = 4096;
static const int S2PARTICLE_SWEEP_FRAMES = 128;
static const int S2PARTICLE_FRAMES_PER_ANGLE = 8;
static const int S2PARTICLE_SWEEP_MID_TO_RIGHT = 1000;
static const int S2PARTICLE_SWEEP_RIGHT_TO_MID = 2000;
static const int S2PARTICLE_SWEEP_MID_TO_LEFT = 3000;
static const int S2PARTICLE_SWEEP_LEFT_TO_MID = 3968;

// Despawn margin around the playfield, one half-sprite on each edge.
static const pixel_t S2PARTICLE_MARGIN = (PARTICLE_W / 2);

// Frames each of the PARTICLE_CELS zoom cels is held for, given that [zoom]
// only advances on 3 of every 4 particles.
static const unsigned int S2PARTICLE_ZOOM_PER_CEL = 16;
static const unsigned int S2PARTICLE_ZOOM_INITIAL = 8;

// Jitter added to the respawn X, on top of the emitter's own.
static const pixel_t S2PARTICLE_JITTER_W = 8;
// ---------

// The two halves of "put this particle at the emitter": a random angle within
// S2PARTICLE_ANGLE_RANGE of [angle_base], and the X that goes with the angle
// it drew. Separate rather than one macro because the sweeping arms of
// s2particle_respawn() reuse [cycle] between the two.
#define s2particle_angle_from(angle_base) { \
	particle->angle = ( \
		randring1_next16_mod(S2PARTICLE_ANGLE_RANGE) + (angle_base) \
	); \
}

#define s2particle_x_from(angle_base) \
	TO_SP( \
		(((angle_base) + S2PARTICLE_ANGLE_RANGE) - particle->angle) * \
		S2PARTICLE_PIXELS_PER_ANGLE \
	)

// Everything a respawn does once the emitter has decided where the particle
// goes. Written out in all eight arms of s2particle_respawn() rather than
// factored into a tail, because that is what the original does: `-O`
// cross-jumps the eight identical copies into one and jumps to it from each
// arm, and the merge point is exactly where the arms stop agreeing
// (kb/codegen/0097). The tail is branch-free, so 0144's bound on that rule
// does not apply here.
// (Its parameter cannot be called `x`: this macro writes through
// `particle->pos.cur.x`, and the substitution would eat the member name.)
#define s2particle_emit(left) { \
	particle->pos.cur.x.v = (left); \
	particle->pos.cur.x.v += TO_SP(irand() % S2PARTICLE_JITTER_W); \
	particle->zoom = S2PARTICLE_ZOOM_INITIAL; \
	particle->pos.cur.y.v = 0; \
	vector2( \
		particle->pos.velocity.x.v, particle->pos.velocity.y.v, \
		particle->angle, S2PARTICLE_SPEED \
	); \
}

// Sends a particle that has left the playfield back out of the emitter, whose
// position is a function of where the stage currently is in its
// S2PARTICLE_CYCLE_FRAMES cycle. Four parked stretches and four sweeps, and
// the two kinds are only told apart by whether the angle base and the X are
// constants or track [cycle].
extern "C" void pascal near s2particle_respawn(s2particle_t near *particle)
{
	// Where in the cycle the stage is; then, inside a sweep, how far into it,
	// and finally that same distance scaled to the X the emitter has reached.
	int cycle;

	// The angle base the emitter has swept to. Read as a byte by the
	// s2particle_angle_from() below, which is what keeps it off a register
	// (kb/codegen/0131).
	int angle_base;

	cycle = (stage_frame % S2PARTICLE_CYCLE_FRAMES);
	if(cycle < S2PARTICLE_SWEEP_MID_TO_RIGHT) {
		s2particle_angle_from(S2PARTICLE_ANGLE_MID);
		s2particle_emit(
			s2particle_x_from(S2PARTICLE_ANGLE_MID) +
			TO_SP(S2PARTICLE_ORIGIN_MID)
		);
	} else if(
		cycle < (S2PARTICLE_SWEEP_MID_TO_RIGHT + S2PARTICLE_SWEEP_FRAMES)
	) {
		cycle -= S2PARTICLE_SWEEP_MID_TO_RIGHT;
		angle_base = (
			(cycle / S2PARTICLE_FRAMES_PER_ANGLE) + S2PARTICLE_ANGLE_MID
		);
		s2particle_angle_from(angle_base);
		cycle = (cycle * S2PARTICLE_PIXELS_PER_ANGLE);
		s2particle_emit(
			s2particle_x_from(angle_base) + cycle +
			TO_SP(S2PARTICLE_ORIGIN_MID)
		);
	} else if(cycle < S2PARTICLE_SWEEP_RIGHT_TO_MID) {
		s2particle_angle_from(S2PARTICLE_ANGLE_RIGHT);
		s2particle_emit(
			s2particle_x_from(S2PARTICLE_ANGLE_RIGHT) +
			TO_SP(S2PARTICLE_ORIGIN_RIGHT)
		);
	} else if(
		cycle < (S2PARTICLE_SWEEP_RIGHT_TO_MID + S2PARTICLE_SWEEP_FRAMES)
	) {
		cycle -= S2PARTICLE_SWEEP_RIGHT_TO_MID;
		angle_base = (
			S2PARTICLE_ANGLE_RIGHT - (cycle / S2PARTICLE_FRAMES_PER_ANGLE)
		);
		s2particle_angle_from(angle_base);
		cycle = (cycle * S2PARTICLE_PIXELS_PER_ANGLE);
		s2particle_emit(
			s2particle_x_from(angle_base) +
			(TO_SP(S2PARTICLE_ORIGIN_RIGHT) - cycle)
		);
	} else if(cycle < S2PARTICLE_SWEEP_MID_TO_LEFT) {
		s2particle_angle_from(S2PARTICLE_ANGLE_MID);
		s2particle_emit(
			s2particle_x_from(S2PARTICLE_ANGLE_MID) +
			TO_SP(S2PARTICLE_ORIGIN_MID)
		);
	} else if(
		cycle < (S2PARTICLE_SWEEP_MID_TO_LEFT + S2PARTICLE_SWEEP_FRAMES)
	) {
		cycle -= S2PARTICLE_SWEEP_MID_TO_LEFT;
		angle_base = (
			S2PARTICLE_ANGLE_MID - (cycle / S2PARTICLE_FRAMES_PER_ANGLE)
		);
		s2particle_angle_from(angle_base);
		cycle = (cycle * S2PARTICLE_PIXELS_PER_ANGLE);
		s2particle_emit(
			s2particle_x_from(angle_base) +
			(TO_SP(S2PARTICLE_ORIGIN_MID) - cycle)
		);
	} else if(cycle < S2PARTICLE_SWEEP_LEFT_TO_MID) {
		s2particle_angle_from(S2PARTICLE_ANGLE_LEFT);
		s2particle_emit(s2particle_x_from(S2PARTICLE_ANGLE_LEFT));
	} else {
		cycle -= S2PARTICLE_SWEEP_LEFT_TO_MID;
		angle_base = (
			(cycle / S2PARTICLE_FRAMES_PER_ANGLE) + S2PARTICLE_ANGLE_LEFT
		);
		s2particle_angle_from(angle_base);
		cycle = (cycle * S2PARTICLE_PIXELS_PER_ANGLE);
		s2particle_emit(s2particle_x_from(angle_base) + cycle);
	}
}

void pascal near stage2_update(void)
{
	s2particle_t near *particle;
	int i;
	int j;
	int k;

	if(
		(boss.phase > PHASE_BOSS_ENTRANCE_BB) &&
		(boss.phase < PHASE_BOSS_EXPLODE_BIG)
	) {
		return;
	}
	if((boss.phase == PHASE_BOSS_ENTRANCE_BB) && (boss.phase_frame >= 32)) {
		return;
	}

	// The entrance flash. [scroll_active] stays off for its whole length:
	// this function draws the background itself until the hand-off below.
	if(stage_frame < S2_FLASH_FRAME_LAST) {
		scroll_active = false;
		if(stage_frame >= S2_FLASH_FRAME_FIRST) {
			j = ((S2_FLASH_FRAME_LAST - 1) - stage_frame);
			Palettes[0].c.r = j;
			Palettes[0].c.g = j;
			Palettes[0].c.b = (j * 5);
			PaletteTone = (stage2_flash_tone + 100);
			palette_changed = true;
			stage2_flash_tone += 2;

			// Opens a gap in the tile ring, one column outwards per cel.
			if(stage_frame_mod4 == 0) {
				j = ((stage_frame - S2_FLASH_FRAME_FIRST) / 4);
				i = (S2_GAP_LEFT_COLUMN - j);
				k = (S2_GAP_RIGHT_COLUMN + j);
				j = 0;
				while(j < TILES_Y) {
					tile_ring[j][i] = S2_GAP_TILE_VO;
					tile_ring[j][k] = S2_GAP_TILE_VO;
					j++;
				}
			}
		}
		tiles_invalidate_all();
		return;
	}

	// Hand-off: blank the whole tile ring, hardware-scroll once, and hand the
	// background over to the regular scrolling code from the next frame on.
	if(stage_frame < S2_SCROLL_HANDOFF_FRAME_LAST) {
		graph_scrollup(scroll_line);
		i = 0;
		while(i < TILES_X) {
			j = 0;
			while(j < TILES_Y) {
				tile_ring[j][i] = TILE_AREA_VRAM_LEFT;
				j++;
			}
			i++;
		}
		tiles_invalidate_all();
		scroll_active = true;
		s2particles_spawned = 0;
		stage2_bg_pulse = 0;
		stage2_bg_pulse_direction = 0;
		return;
	}

	// Decaying the flash and running the particles are exclusive: the stage
	// only starts spawning once [PaletteTone] is back at 100.
	if(stage2_flash_tone != 0) {
		if(stage2_flash_tone > 4) {
			stage2_flash_tone -= 4;
		} else {
			stage2_flash_tone = 0;
		}
		PaletteTone = (stage2_flash_tone + 100);
	} else {
		if(
			(s2particles_spawned < S2PARTICLE_COUNT) &&
			(stage_frame_mod2 == 0)
		) {
			particle = &s2particles[s2particles_spawned];
			particle->flag = 1;
			// The same emitter s2particle_respawn() parks at for most of
			// the cycle, minus its jitter and its [zoom] reset.
			s2particle_angle_from(S2PARTICLE_ANGLE_MID);
			particle->pos.cur.x.v = (
				s2particle_x_from(S2PARTICLE_ANGLE_MID) +
				TO_SP(S2PARTICLE_ORIGIN_MID)
			);
			particle->pos.cur.y.v = 0;
			vector2(
				particle->pos.velocity.x.v, particle->pos.velocity.y.v,
				particle->angle, S2PARTICLE_SPEED
			);
			s2particles_spawned++;
		}

		grcg_setcolor(GC_RMW, 0);
		particle = s2particles;
		j = 0;
		while(j < S2PARTICLE_COUNT) {
			if(particle->flag != 0) {
				// The casts are load-bearing: the pseudo-registers are
				// `unsigned`, and all four of these comparisons are signed in
				// the original. Same reason shinki_bg_particles_render()
				// wraps its own _CX and _AX reads.
				#define cur_x	static_cast<subpixel_t>(_AX)
				#define cur_y	static_cast<subpixel_t>(_DX)

				/* _DX:_AX = */ particle->pos.update_seg1();
				if(!(
					(cur_x > TO_SP(-S2PARTICLE_MARGIN)) &&
					(cur_x < TO_SP(PLAYFIELD_W + S2PARTICLE_MARGIN)) &&
					(cur_y > TO_SP(-S2PARTICLE_MARGIN)) &&
					(cur_y < TO_SP(PLAYFIELD_H + S2PARTICLE_MARGIN))
				)) {
					s2particle_respawn(particle);
				} else {
					_ES = SEG_PLANE_B;

					// Every 4th particle never grows. [inferred]: the index is
					// not a property of the particle, so this looks like a
					// cheap way of keeping a few of them small rather than
					// anything the effect needs.
					if(j & 3) {
						particle->zoom++;
					}
					i = (particle->zoom / S2PARTICLE_ZOOM_PER_CEL);
					if(i >= (PARTICLE_CELS - 1)) {
						i = (PARTICLE_CELS - 1);
					}
					i += PAT_PARTICLE;

					#define left	static_cast<pixel_t>(_CX)
					#define top 	static_cast<pixel_t>(_AX)

					// Same two-step assignment as
					// shinki_bg_particles_render(); the second call would
					// otherwise overwrite _CX.
					k = particle->pos.cur.to_screen_left(PARTICLE_W);
					top = particle->pos.cur.to_vram_top_scrolled_seg1(
						PARTICLE_H
					);
					left = k;
					z_super_roll_put_16x16_mono_raw(i);

					#undef top
					#undef left
				}

				#undef cur_y
				#undef cur_x
			}
			j++;
			particle++;
		}
		grcg_off();

		if(stage_frame_mod4 != 0) {
			return;
		}
		if(stage2_bg_pulse_direction == 0) {
			stage2_bg_pulse++;
			if(stage2_bg_pulse >= S2_BG_PULSE_MAX) {
				stage2_bg_pulse_direction++;
			}
		} else {
			stage2_bg_pulse--;
			if(stage2_bg_pulse <= S2_BG_PULSE_MIN) {
				stage2_bg_pulse_direction--;
			}
		}
		Palettes[0].c.r = (stage2_bg_pulse * 2);
		Palettes[0].c.g = (stage2_bg_pulse * 2);
		Palettes[0].c.b = (stage2_bg_pulse * 4);
	}
	palette_changed = true;
}
