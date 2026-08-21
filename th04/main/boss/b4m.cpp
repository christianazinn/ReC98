/// Stage 4 Boss - Marisa
/// ---------------------

#pragma option -zCB4M_UPDATE_TEXT -zPmain_03

#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/v_colors.hpp"
#include "th03/hardware/palette.hpp"
#include "th04/sprites/main_pat.h"
#include "th04/snd/snd.h"
#include "th04/formats/std.hpp"
#include "th04/math/randring.hpp"
#include "th04/main/bg.hpp"
#include "th04/main/circle.hpp"
#include "th04/main/custom.hpp"
#include "th04/main/gather.hpp"
#include "th02/main/player/player.hpp"
#include "th04/main/homing.hpp"
#include "th04/main/null.hpp"
#include "th04/main/hud/hud.hpp"
#include "th04/main/bullet/bullet.hpp"
#include "th04/main/bullet/clearzap.hpp"
#include "th04/main/bullet/laser_t.hpp"
#include "th04/main/tile/bb.hpp"
#include "th04/main/boss/boss.hpp"

// Constants
// ---------

static const pixel_t BIT_W = 32;
static const pixel_t BIT_H = 32;
static const int BIT_KILL_FRAMES_PER_CEL = 4;
// ---------

// Structures
// ----------

#define BIT_COUNT 4

enum bit_flag_t {
	BF_FREE = 0,
	BF_MOVEOUT_SPIN = 1,
	BF_SPIN = 2,
	BF_KILL_ANIM = 0x80,
	BF_KILL_ANIM_last = (
		BF_KILL_ANIM + (ENEMY_KILL_CELS * BIT_KILL_FRAMES_PER_CEL) - 1
	),

	_bit_flag_t_FORCE_UINT8 = 0xFF
};

struct bit_t {
	bit_flag_t flag;
	unsigned char angle;	// input for [center]
	PlayfieldPoint center;
	main_patnum_t patnum;
	/* ------------------------- */ int8_t unused_1[8];
	Subpixel distance;	// input for [center]
	Subpixel moveout_speed;
	int hp;
	int damage_this_frame;
	/* ------------------------- */ int8_t unused_2;
	char angle_speed;	// ACTUAL TYPE: unsigned char
};

#define bits (reinterpret_cast<bit_t *>(custom_entities))

// What marisa_charge_animate() tells the pattern it opens. Deliberately a
// byte-sized enum: the original returns these in AL alone, and every caller
// homes the result in a byte local before testing it.
enum marisa_charge_t {
	// Still charging. The pattern must not fire yet.
	MC_CHARGING = 0,

	// The charge is over and the pattern has already fired; keep running it.
	MC_RUNNING = 1,

	// This one frame is the frame the pattern fires on.
	MC_FIRE = 2,

	_marisa_charge_t_FORCE_UINT8 = 0xFF
};
// ----------

// State
// -----

#define flystep_pointreflected_tick boss_statebyte[13]

extern uint8_t bits_alive;

extern void (near pascal *near bit_fire)(bit_t near& bit);
extern screen_x_t bit_center_x[BIT_COUNT];
extern screen_x_t bit_center_y[BIT_COUNT];
// -----

// Game logic
// ----------

/// Stage 5 Boss - Yuuka
/// --------------------
/// ZUN's object for this code segment opened with Yuuka's fight and only then
/// moved on to Marisa's (kb/codegen/0112, and the segment is named after the
/// second half). These two functions are the bottom of that first half.

// Still ASM, all of them in this same segment, and all of them private to
// ZUN's object -- so each needed a zero-byte `label` alias in th04_main.asm to
// become linkable at all (kb/codegen/0123). The address-suffixed names are the
// dump's own; naming them is a separate decision from making them linkable,
// and belongs to whoever lifts them.
extern "C" {
	// Runs Yuuka's four-state warp animation, and returns `true` on the one
	// frame it completes on. The parameter picks the destination: `false` is a
	// random point in the upper playfield, `true` is the fixed point the fight
	// phases warp back to. `pascal`, hence the UPPER-case alias.
	bool pascal near yuuka5_15ECE(bool to_fixed_point);

	void near yuuka5_15F97(void);
	void near yuuka5_160A5(void);
	void near yuuka5_161D7(void);
	void near yuuka5_162A3(void);
	void near yuuka5_1630D(void);
	void near yuuka5_16389(void);
	void near yuuka5_1653D(void);
}

// th04/main/midboss/vars[bss].asm publishes it, but no TH04 header declares
// it: the only two C++ functions that have ever needed it are this one and
// yuuka6_update(), and the latter is still ASM.
extern "C" int midboss_frames_until;

// Yuuka's warp animation state, documented at length in
// th04/main/boss/b5r.cpp, which is the only thing that reads it.
extern "C" unsigned char yuuka5_warp_phase;

// Declared FAR here, and only here: th04/main/boss/bosses.hpp declares the same
// function `near`, which is what it is. Borland does not encode near/far in a
// mangled name, so both declarations reach the same `@YUUKA5_BG_RENDER$QV` --
// but a NEAR reference under this object's `-zPmain_03` frames its offset on
// main_03, and this function lives in main_01. That is `Fixup overflow at
// B4M_UPDATE_TEXT:006D`; declaring it far makes Turbo C++ frame the offset on
// the target's own group instead, which is the 0x7874 the original stores.
void pascal far yuuka5_bg_render(void);

// Both of Yuuka's phase-2 patterns use this to carry the spread angle of the
// pellet fan across frames, incremented by 9 on every volley.
#define yuuka5_spread_angle boss_statebyte[15]

// The HP Yuuka starts every one of her fights with, and the denominator the
// HUD bar is drawn against.
static const int YUUKA5_HP = 9000;

// Yuuka's Stage 5 fight, all 19 phases of it. Three of them are the entrance
// and the defeat; the other 16 are five repetitions of the same
// pattern/warp/pattern triple, differing only in which two pattern functions
// the [boss.mode] dispatch below reaches and in how much HP the phase costs.
//
// [boss.mode] is that dispatch's own state, and its two negative values are
// not patterns: 254 advances to the next pattern of the pair, 255 runs the
// warp. Both `switch`es over it are sparse, which is why this function's tail
// carries a value/jump table PAIR for each of them on top of the dense one for
// [boss.phase] -- three tables and one padding byte, all of which lifting the
// function moves out of the dump with it.
//
// `#pragma option -a2` is that padding byte, and it needs this function to be
// the FIRST thing the object emits. kb/codegen/0160 for the instrument -- read
// the OBJ's PUBDEF offsets, never the `tcc -S` listing, which prints
// `db 1 dup (?)` for the parity that emits nothing. `[measured]` here, both
// ways: at a zero prefix the object is 0x40A bytes to the next function and
// carries the pad; with one more byte ahead of it, 0x40A again but one of
// those bytes is the prefix and the pad is gone. That is the OPPOSITE sign
// from BOSS_BG_TEXT's yuuka6_bg_update_and_render(), which is why 0160 says to
// probe both parities rather than to compute one.
#pragma option -a2
void pascal far yuuka5_update(void)
{
	bullet_template.origin.x.v = boss.pos.cur.x.v;
	bullet_template.origin.y.v = (boss.pos.cur.y.v + TO_SP(16));

	switch(boss.phase) {
	case 0:
		if(boss.phase_frame == 0) {
			// Yuuka's fight ends the stage script, and pushes the midboss
			// that will never come out of reach for good.
			stage_vm = nullfunc_far;
			midboss_frames_until = 0;
		}
		boss_hittest_shots_invincible();
		if(boss.phase_frame > 128) {
			boss.phase++;
			boss.phase_frame = 0;
			snd_se_play(13);
			yuuka5_warp_phase = 0;
			tiles_bb_col = V_WHITE;
			_asm { mov word ptr bg_render_bombing_func, offset yuuka5_bg_render }
		}
		break;

	case 1:
		boss_hittest_shots_invincible();
		if(boss.phase_frame == 32) {
			Palettes[0].c.r = 64;
			Palettes[0].c.g = 64;
			Palettes[0].c.b = 64;
			palette_changed = true;
		}
		if(boss.phase_frame >= 64) {
			boss.phase++;
			boss.pos.velocity.x.v = 0;
			boss.phase_state.patterns_seen = 0;
			boss.mode = 0;
			boss.hp = YUUKA5_HP;
			boss.phase_end_hp = 7900;
			boss.phase_frame = 0;
			boss.pos.cur.y.v -= TO_SP(16);
		}
		break;

	case 2: case 5: case 8:
		switch(boss.mode) {
		case 0:
			yuuka5_15F97();
			break;
		case 1:
			yuuka5_160A5();
			break;
		case 254:
			boss.phase_frame = 0;
			boss.phase_state.patterns_seen++;
			boss.mode = (boss.phase_state.patterns_seen % 2);
			break;
		case 255:
			yuuka5_15ECE(false);
			break;
		}
		if(yuuka5_warp_phase == 0) {
			if(boss.phase_state.patterns_seen < 4) {
				if(!boss_hittest_shots()) {
					break;
				}
				boss_score_bonus(15);
				boss_items_drop();
			}
			bullets_clear();
			boss_explode_small(ET_CIRCLE);
			boss.phase++;
			boss.hp = boss.phase_end_hp;
			boss.phase_end_hp -= 800;
		} else {
			boss.phase_frame++;
		}
		break;

	case 3: case 6: case 9:
		boss.phase_frame++;
		if(yuuka5_15ECE(true)) {
			boss.phase++;
			boss.phase_frame = 0;
			boss.phase_state.patterns_seen = 0;
			boss.mode = 0;
		}
		break;

	case 4: case 7: case 10:
		yuuka5_161D7();
		if(boss.phase_frame < 500) {
			if(!boss_hittest_shots()) {
				break;
			}
			boss_score_bonus(15);
			boss_items_drop();
		}
		bullets_clear();
		boss_explode_small(ET_NW_SE);
		boss.phase++;
		boss.phase_frame = 0;
		boss.phase_state.patterns_seen = 0;
		boss.mode = 0;
		boss.hp = boss.phase_end_hp;
		if(boss.phase < 10) {
			boss.phase_end_hp -= 1100;
		} else {
			boss.phase_end_hp -= 1200;
		}
		break;

	case 11: case 14:
		switch(boss.mode) {
		case 0:
			yuuka5_162A3();
			break;
		case 1:
			yuuka5_1630D();
			break;
		case 254:
			boss.phase_frame = 0;
			boss.phase_state.patterns_seen++;
			boss.mode = (boss.phase_state.patterns_seen % 2);
			break;
		case 255:
			yuuka5_15ECE(false);
			break;
		}
		if(yuuka5_warp_phase == 0) {
			if(boss.phase_state.patterns_seen < 4) {
				if(!boss_hittest_shots()) {
					break;
				}
				boss_score_bonus(15);
				boss_items_drop();
			}
			bullets_clear();
			boss_explode_small(ET_CIRCLE);
			boss.phase++;
			boss.hp = boss.phase_end_hp;
		} else {
			boss.phase_frame++;
		}
		break;

	case 12: case 15:
		boss.phase_frame++;
		if(yuuka5_15ECE(true)) {
			boss.phase++;
			boss.phase_frame = 0;
			boss.phase_state.patterns_seen = 0;
			boss.mode = 0;
		}
		break;

	case 13: case 16:
		yuuka5_16389();
		boss_hittest_shots_invincible();
		if(boss.phase_frame >= 288) {
			boss_explode_small(ET_VERTICAL);
			bullets_clear();
			boss.phase++;
			boss.hp = boss.phase_end_hp;
			if(boss.phase == 17) {
				// The last phase is the one that has to end at 0 HP, and it
				// is also the one that reddens the background.
				boss.phase_end_hp = 0;
				Palettes[0].c.r = 128;
				Palettes[0].c.g = 64;
				Palettes[0].c.b = 64;
				palette_changed = true;
			} else {
				boss.phase_end_hp -= 1200;
			}
			boss.phase_frame = 0;
			boss.phase_state.patterns_seen = 0;
			boss.mode = 0;
			PaletteTone = 100;
			palette_changed = true;
		}
		break;

	case 17:
		yuuka5_1653D();
		if(boss_hittest_shots() || (boss.phase_frame >= 1000)) {
			boss_explode_small(ET_NW_SE);
			boss.phase++;

			// The defeat bonus is the one thing that distinguishes killing
			// Yuuka from surviving her: the timeout takes the same branch.
			if(boss.phase_frame < 1000) {
				boss.phase_state.defeat_bonus = true;
			} else {
				boss.phase_state.defeat_bonus = false;
			}
			boss.phase_frame = 0;
			boss.mode = 0;
			PaletteTone = 100;
			palette_changed = true;
		}
		break;

	case 18:
		boss.phase_frame++;
		if(boss.phase_frame == 16) {
			boss_explode_small(ET_VERTICAL);
		}
		if(boss.phase_frame == 32) {
			boss_defeat_explode_big(ET_SW_NE, 60);
			snd_se_play(12);
			Palettes[0].c.r = 0;
			Palettes[0].c.g = 0;
			Palettes[0].c.b = 0;
			palette_changed = true;
			player_invincibility_time = BOSS_DEFEAT_INVINCIBILITY_FRAMES;
		}
		break;

	default:
		boss_defeat_update();
		return;
	}

	homing_target.x.v = boss.pos.cur.x.v;
	homing_target.y.v = boss.pos.cur.y.v;
	thicklasers_update_and_hittest();
	hud_hp_update_and_render(boss.hp, YUUKA5_HP);
}
#pragma option -a1
/// --------------------


// The fixed 64-frame charge-up that every one of Marisa's patterns opens
// with, and the only thing that tells the pattern when to fire. Advances
// nothing itself: the caller owns [boss.phase_frame], and this function just
// reads it and stages the animation, the two sound effects and the two
// effect spawns that go with each of its landmark frames.
//
// The gather ring is a three-circle stack, added on the same 0/+2/+4 frames
// and in the same shape as gather_add_only_3stack() -- which this function
// does NOT call, because its first circle also has to re-seed the whole
// [gather_template] and that helper only sets the color.
//
// The ring is the one part of the animation gated on [boss.mode], which
// leaves Marisa's two defeat phases without it.
marisa_charge_t near marisa_charge_animate(void)
{
	enum {
		// The landmark frames. FRAME_FIRE also ends the charge, so the whole
		// lead-in is exactly that many frames long.
		FRAME_CAST = 16,
		FRAME_CIRCLE = 30,
		FRAME_GATHER = 32,	// ... and +2 and +4
		FRAME_CELS = 44,
		FRAME_FIRE = 64,

		FRAMES_PER_CEL = 4,

		// Both the gather ring and the shrinking circle are centered above
		// and to the left of Marisa's origin, not on it.
		CENTER_OFFSET_X = TO_SP(20),
		CENTER_OFFSET_Y = TO_SP(8),

		GATHER_RADIUS = TO_SP(256),
		GATHER_RING_POINTS = 16,
		GATHER_ANGLE_DELTA = -2,
		COL_GATHER_1 = 3,
		COL_GATHER_2 = 2,

		// Marisa's cels are absolute patnums -- marisa_fg_render() blits
		// [boss.sprite] as it stands, with no per-boss base added.
		PAT_MARISA_CAST = (PAT_STAGE + 2),

		// The [boss.mode] range that gets the gather ring.
		MODE_GATHER_FIRST = 1,
		MODE_GATHER_LAST = 6,
	};
	if((boss.mode >= MODE_GATHER_FIRST) && (boss.mode <= MODE_GATHER_LAST)) {
		switch(boss.phase_frame) {
		case FRAME_GATHER:
			gather_template.center.x.v = (boss.pos.cur.x - CENTER_OFFSET_X);
			gather_template.center.y.v = (boss.pos.cur.y - CENTER_OFFSET_Y);
			gather_template.ring_points = GATHER_RING_POINTS;
			gather_template.angle_delta = GATHER_ANGLE_DELTA;
			gather_template.col = COL_GATHER_1;
			gather_template.radius.v = GATHER_RADIUS;
			gather_add_only();
			break;
		case (FRAME_GATHER + 2):
			gather_template.col = COL_GATHER_2;
			gather_add_only();
			break;
		case (FRAME_GATHER + 4):
			gather_add_only();
		}
	}
	if(boss.phase_frame == FRAME_CAST) {
		boss.sprite = PAT_MARISA_CAST;
		snd_se_play(8);
	} else if(boss.phase_frame == FRAME_CIRCLE) {
		circles_add_shrinking(
			(boss.pos.cur.x - CENTER_OFFSET_X),
			(boss.pos.cur.y - CENTER_OFFSET_Y)
		);
	} else if(
		(boss.phase_frame >= FRAME_CELS) && (boss.phase_frame < FRAME_FIRE)
	) {
		boss.sprite = (
			PAT_STAGE + ((boss.phase_frame - FRAME_GATHER) / FRAMES_PER_CEL)
		);
	} else if(boss.phase_frame == FRAME_FIRE) {
		boss.sprite = PAT_MARISA_CAST;
		snd_se_play(15);
		return MC_FIRE;
	}
	if(boss.phase_frame < FRAME_FIRE) {
		return MC_CHARGING;
	}
	return MC_RUNNING;
}

// Marisa's other flight step: a bounded random wander. Every 32nd frame — on
// the frame *after* each multiple of 32, since the test is against 1 rather
// than 0 — this picks a new velocity for each axis independently. An axis
// whose coordinate has left its box is turned back towards the middle at a
// fixed speed; otherwise the new direction is random. Every call, including
// the 31 that pick nothing, then applies the current velocity to [boss].
//
// The two axes are not symmetric, and the asymmetry is ZUN's: X gets a
// straight coin flip between two speeds, while Y draws from four, adding the
// two 1.5-pixel variants that have no X counterpart.
void near marisa_flystep_random(void)
{
	enum {
		INTERVAL_MASK = 0x1F,

		LEFT   = TO_SP(112),
		RIGHT  = TO_SP(272),
		TOP    = TO_SP(80),
		BOTTOM = TO_SP(144),

		SPEED_TURN_X = TO_SP(2),
		SPEED_TURN_Y = TO_SP(1),
		SPEED_RANDOM = TO_SP(1),

		// 1.5 pixels. Not expressible as TO_SP() of an integer, which is why
		// the original stores it as the raw subpixel literal 24.
		SPEED_RANDOM_FAST = (TO_SP(3) / 2),
	};
	if((boss.phase_frame & INTERVAL_MASK) == 1) {
		if(boss.pos.cur.x <= LEFT) {
			boss.pos.velocity.x.v = SPEED_TURN_X;
		} else if(boss.pos.cur.x >= RIGHT) {
			boss.pos.velocity.x.v = -SPEED_TURN_X;
		} else {
			boss.pos.velocity.x.v = (
				randring2_next16_and(1) ? SPEED_RANDOM : -SPEED_RANDOM
			);
		}

		if(boss.pos.cur.y <= TOP) {
			boss.pos.velocity.y.v = SPEED_TURN_Y;
		} else if(boss.pos.cur.y >= BOTTOM) {
			boss.pos.velocity.y.v = -SPEED_TURN_Y;
		} else {
			unsigned char direction = randring2_next16_and(3);
			boss.pos.velocity.y.v = (
				(direction == 0) ?  SPEED_RANDOM :
				(direction == 1) ? -SPEED_RANDOM :
				(direction == 2) ?  SPEED_RANDOM_FAST :
				                   -SPEED_RANDOM_FAST
			);
		}
	}
	boss.pos.update_seg3();
}

// On [flystep_pointreflected_tick] 0, this function sets up [boss] movement
// towards the point reflection of Marisa's position across a fixed position
// near the top of the sealed moon in the background. The velocity is
// calculated to reach this exact point at [duration - 12], with Marisa braking
// on the last 12 frames by halving that velocity each frame. Every call to
// this function, including the one on frame 0, then applies this velocity to
// [boss].
// [duration] values <12 will move Marisa into the opposite direction instead.
// Returns `true` if the function was called for [duration] frames.
//
// ZUN bug: Not defined for [duration] values of 12 or 13, which will crash the
// game with a division by zero ("Divide Error"). The two patterns that pass a
// variable [duration] to this function also only happen to call this function
// every 4 frames rather than every frame, introducing additional jerkiness.
bool pascal near marisa_flystep_pointreflected(int duration)
{
	enum {
		POINT_X = TO_SP(PLAYFIELD_W / 2),
		POINT_Y = TO_SP((PLAYFIELD_H * 7) / 23),
		BRAKE_DURATION = 12,
	};
	if(flystep_pointreflected_tick == 0) {
		boss.pos.velocity.x.v = (
			(POINT_X - boss.pos.cur.x) / ((duration / 2) - (BRAKE_DURATION / 2))
		);
		boss.pos.velocity.y.v = (
			(POINT_Y - boss.pos.cur.y) / ((duration / 2) - (BRAKE_DURATION / 2))
		);
	}
	flystep_pointreflected_tick++;
	if(flystep_pointreflected_tick >= (duration - BRAKE_DURATION)) {
		boss.pos.velocity.x.v /= 2;
		boss.pos.velocity.y.v /= 2;
	}
	if(flystep_pointreflected_tick >= duration) {
		return true;
	}
	boss.pos.update_seg3();
	return false;
}
// ----------
