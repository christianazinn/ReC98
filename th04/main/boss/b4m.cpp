/// Stage 4 Boss - Marisa
/// ---------------------
/// (#included from th04/boss_4m.cpp, BELOW th04/main/boss/explode_big.cpp,
/// which is the address order both had in the original. That wrapper now
/// carries this object's `-zC`/`-zP` pragma, because `-zC` has to be seen
/// before any code is generated and this file is no longer the first in the
/// object — kb/codegen/0105.)

// iatan2(), for the one Yuuka pattern that aims at the player.
#include "libs/master.lib/master.hpp"
#include "decomp.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/v_colors.hpp"
#include "th03/hardware/palette.hpp"
#include "th04/sprites/main_pat.h"
#include "th04/snd/snd.h"
#include "th04/formats/std.hpp"
#include "th04/math/randring.hpp"
#include "th04/main/frames.h"
#include "th04/main/rank.hpp"
#include "th04/main/bg.hpp"
#include "th04/main/circle.hpp"
#include "th04/main/custom.hpp"
#include "th04/main/gather.hpp"
#include "th02/main/player/player.hpp"
#include "th04/main/player/player.hpp"
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

// The thick laser's two colored layers are this wide at most, and the
// hittest in thicklasers_update_and_hittest() is against the white core
// inside them rather than against the full radius.
// th04/main/bullet/laser_render.cpp's own, repeated because that file is a
// translation unit rather than a header.
static const pixel_t THICKLASER_LAYER_W_MAX = 16;
// ---------

// Structures
// ----------
// Marisa's bits, her charge enum and everything around them moved to
// th04/main/boss/b4m.hpp once th04/main/boss/b4m_upd.cpp -- the other half of
// her fight, and a different segment and object -- grew to need them too.
#include "th04/main/boss/b4m.hpp"
// ----------

// State
// -----

#define flystep_pointreflected_tick boss_statebyte[13]
// -----

// Game logic
// ----------

/// The thick-laser API
/// --------------------
/// The four procs that opened ZUN's contribution to B4M_UPDATE_TEXT, ahead of
/// Yuuka's fight. Their spawner and their per-frame updater are reached from
/// three different fights and from th04/main/stage/init.cpp, so none of them
/// is `static`; th04/main/bullet/laser_t.hpp declares the two the rest of the
/// tree calls.
///
/// `#pragma option -G` is scoped over thicklasers_update_and_hittest() alone
/// below, and that scoping is EVIDENCE, not decoration: its original frame is
/// the manual three-instruction one, where yuuka5_161D7() and yuuka5_16389()
/// in this same object both open with the single-instruction form. `-G` is a
/// per-translation-unit flag and two settings cannot coexist in one of ZUN's
/// objects, so ZUN split his source at 0x15ECE, exactly where
/// kb/codegen/0164 reads a segment's object boundaries off its table pads.
/// The scoped pragma reproduces the bytes from inside this file either way;
/// a lane that would rather have the split can move these four into their own
/// `-zCB4M_UPDATE_TEXT` object linked immediately ahead of this one.

// Frees both slots and re-seeds the four [thicklaser_template] fields a
// spawner does not set, so that every laser spawned during the stage begins
// as a 1-pixel telegraph line that grows by 1 pixel per frame until a
// spawner overrides [radius_speed]. Called once per stage, from
// stage_init(). `[inferred]` name pending: `thicklasers_reset()`, on the
// model of TH02's lasers_reset() — see the naming note at the head of this
// file.
extern "C" void far thicklasers_reset(void)
{
	thicklaser_t near *tl = thicklasers;
	int i;

	for(i = 0; i < THICKLASER_COUNT; i++, tl++) {
		tl->flag = TF_FREE;
	}
	thicklaser_template.cur_flag_frame = 0;
	thicklaser_template.flag = TF_LINE;
	thicklaser_template.radius_cur = 1;
	thicklaser_template.radius_speed = 1;
}

// Copies [thicklaser_template] over [thicklaser].
//
// Spelled out with pseudo-registers rather than as `thicklaser =
// thicklaser_template;`, which cannot reproduce it in either direction:
// thicklaser_t is not trivially copyable to Turbo C++ (SPPoint has a base
// class), so a plain copy assignment becomes a far call to the runtime's
// structure-copy helper --
// and even under `-G`, where it does inline, the six instructions come out
// in the order SI, DI, ES, CX. The original's order is CX, ES, SI, DI,
// which is kb/codegen/0109's second row. `decomp.hpp`'s
// copy_near_struct_member() is exactly this sequence, but only in its
// `GAME == 5` arm, so the four statements are written out here.
void near pascal thicklaser_template_pull(thicklaser_t near& thicklaser)
{
	_CX = (sizeof(thicklaser_t) / sizeof(uint16_t));
	asm { push ds; pop es; }
	prepare_si_di(
		FP_OFF(&thicklaser), 0, FP_OFF(&thicklaser_template), 0
	);
	asm { rep movsw; }
}

// Copies [thicklaser_template] into the first free slot and plays the spawn
// sound effect. Does nothing at all if both slots are busy. `[inferred]`
// name, on the model of bullets_add_regular() one template over; **a naming
// round is owed**, together with the one for
// thicklasers_update_and_hittest().
extern "C" void near thicklaser_add(void)
{
	thicklaser_t near *tl = thicklasers;
	int i;

	for(i = 0; i < THICKLASER_COUNT; i++, tl++) {
		if(tl->flag == TF_FREE) {
			thicklaser_template_pull(*tl);
			snd_se_play(5);
			return;
		}
	}
}

// Advances every non-TF_FREE thick laser through one step of the flag state
// machine, then hittests the player against the ones that have a body.
//
// The hitbox is the laser's WHITE CORE, not its full radius: the same
// [radius_cur] / 4 layer width that th04/main/bullet/laser_render.cpp insets
// the core by, capped at the same THICKLASER_LAYER_W_MAX, is subtracted from
// the half-width here — in subpixels throughout, which is why the three
// shifts are by 2, 4 and 3 rather than a TO_SP() of a pixel quantity. The
// vertical half is one-sided and generous: only the top edge of the circle
// is tested, at half the radius below the origin, and everything below it
// down the beam counts as a hit.
//
// `#pragma option -G` for the manual three-instruction frame this one opens
// with; kb/codegen/0011, and the note at the head of this file.
#pragma option -G
extern "C" void near thicklasers_update_and_hittest(void)
{
	int i;
	thicklaser_t near *tl = thicklasers;

	// DI. One variable for three quantities, exactly as the original does
	// it: first the vertical offset of the circle's top edge, then the
	// colored layers' width, then the half-width of the white core.
	subpixel_t hitbox_half_w;

	for(i = 0; i < THICKLASER_COUNT; i++, tl++) {
		if(tl->flag == TF_FREE) {
			continue;
		}
		if(tl->flag == TF_LINE) {
			if(tl->cur_flag_frame >= tl->line_frames) {
				tl->flag++; // -> TF_GROW
				tl->cur_flag_frame = 0;
				snd_se_play(6);
			}
		} else if(tl->flag == TF_GROW) {
			tl->radius_cur += tl->radius_speed;
			if(tl->radius_cur >= tl->radius_max) {
				tl->flag++; // -> TF_STATIC
				tl->cur_flag_frame = 0;
				tl->radius_cur = tl->radius_max;
			}
		} else if(tl->flag == TF_STATIC) {
			if(tl->cur_flag_frame >= tl->static_frames) {
				tl->flag++; // -> TF_SHRINK
				tl->cur_flag_frame = 0;
			}
		} else if(tl->flag == TF_SHRINK) {
			tl->radius_cur -= tl->radius_speed;
			if(tl->radius_cur <= 1) {
				tl->flag = TF_FREE;
			}
		}
		tl->cur_flag_frame++;

		// A TF_LINE laser is a 1-pixel telegraph line and does not hit.
		// UNSIGNED, which is what _thicklaser_flag_t_FORCE_UINT8 buys.
		if(tl->flag <= TF_LINE) {
			continue;
		}

		hitbox_half_w = (tl->radius_cur * 8);
		if((tl->origin.y.v + hitbox_half_w) > player_pos.cur.y.v) {
			continue;
		}
		hitbox_half_w = (tl->radius_cur * 4);
		if(hitbox_half_w >= TO_SP(THICKLASER_LAYER_W_MAX)) {
			hitbox_half_w = TO_SP(THICKLASER_LAYER_W_MAX);
		}
		hitbox_half_w = (TO_SP(tl->radius_cur) - hitbox_half_w);
		if((tl->origin.x.v - hitbox_half_w) > player_pos.cur.x.v) {
			continue;
		}
		if((tl->origin.x.v + hitbox_half_w) < player_pos.cur.x.v) {
			continue;
		}
		player_is_hit = true;
	}
}
#pragma option -G-
/// ------------


/// Stage 5 Boss - Yuuka
/// --------------------
/// ZUN's object for this code segment opened with Yuuka's fight and only then
/// moved on to Marisa's (kb/codegen/0112, and the segment is named after the
/// second half). These two functions are the bottom of that first half.

// th04/main/midboss/vars[bss].asm publishes it, but no TH04 header declares
// it: the only two C++ functions that need it are this one and
// yuuka6_update() in th04/main/boss/b6_upd.cpp.
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

/// Yuuka's patterns
/// ----------------
/// All eight of these sat directly above yuuka5_update() in ZUN's object, and
/// every one of them is reached from its `switch(boss.mode)` dispatch and from
/// nowhere else, so all eight are `static` here and the eight zero-byte
/// `label` aliases th04_main.asm carried for them are gone with the bodies.
/// They keep the dump's address-suffixed names; **a naming round is owed** for
/// all eight and for the four state variables below.

// Yuuka's four pattern-local state variables, all of them th04_main.asm
// `.data?` with no `public` of ZUN's, and all of them read and written only by
// the functions below (`[measured]`, over all five binaries). They keep
// address-suffixed names on the same terms as the functions; the evidence for
// naming them is:
//
// • [yuuka5_25662] is the x origin the 11-bullet fan in yuuka5_15F97() is
//   fired from. It starts one half-screen left of Yuuka and then ping-pongs
//   64 pixels either way on every volley.
// • [yuuka5_25664] and [yuuka5_25665] are a rate and its accumulator:
//   yuuka5_160A5() adds the first to the second every frame, fires a ring
//   whenever the second reaches 0x10, and then subtracts 0x10 from it and
//   BUMPS THE RATE — so the rings come faster and faster.
// • [yuuka5_25666] is the palette tone yuuka5_16389() ramps up to and then
//   back down from, written straight into master.lib's [PaletteTone].
extern "C" {
	extern subpixel_t yuuka5_25662;
	extern unsigned char yuuka5_25664;
	extern unsigned char yuuka5_25665;
	extern unsigned char yuuka5_25666;
}

// The thick laser radius yuuka5_16389()'s last case reads, parked in the
// state byte the rest of the fight does not use.
#define yuuka5_thicklaser_radius boss_statebyte[0]

// Runs Yuuka's four-state warp animation, and returns `true` on the one frame
// it completes on. The parameter picks the destination: `false` is a random
// point in the upper playfield, `true` is the fixed point the fight's later
// phases warp back to. The declaration is `pascal`, which is why the alias
// this replaces was spelled in upper case.
//
// The parameter is `bool16` and not `bool`: the original tests it with
// `cmp word ptr [bp+4], 0`, and a 1-byte `bool` gives `cmp byte ptr` at the
// same instruction length. The call site is unaffected — `pascal` pushes a
// word for either — so this is invisible to every length check and shows up
// only in a funcdiff. The `bool` RETURN is right as it stands; the original
// homes it in AL.
static bool pascal near yuuka5_15ECE(bool16 to_fixed_point)
{
	subpixel_t target_x;
	subpixel_t target_y;

	if(yuuka5_warp_phase == 0) {
		yuuka5_warp_phase = 1;
		boss.damage_this_frame = 0;
		boss.pos.cur.y.v += TO_SP(16);
	}
	if(yuuka5_warp_phase == 1) {
		if(boss.phase_frame < 32) {
			return false;
		}
		boss.phase_frame = 0;
		yuuka5_warp_phase = 2;
		if(!to_fixed_point) {
			target_x = (randring2_next16_mod(TO_SP(256)) + TO_SP(64));
			target_y = (randring2_next16_mod(TO_SP(64)) + TO_SP(64));
		} else {
			target_x = TO_SP(PLAYFIELD_W / 2);
			target_y = TO_SP(80);
		}
		boss.pos.velocity.x.v = ((target_x - boss.pos.cur.x.v) / 64);
		boss.pos.velocity.y.v = ((target_y - boss.pos.cur.y.v) / 64);
	} else if(yuuka5_warp_phase == 2) {
		boss.pos.update_seg3();
		if(boss.phase_frame < 64) {
			return false;
		}
		boss.phase_frame = 0;
		yuuka5_warp_phase = 3;
	} else if(yuuka5_warp_phase == 3) {
		if(boss.phase_frame < 8) {
			return false;
		}
		boss.phase_frame = 0;
		yuuka5_warp_phase = 0;
		boss.mode = 254;
		boss.pos.cur.y.v -= TO_SP(16);
		return true;
	}
	return false;
}

// A 11-bullet spread every 16th frame, from an origin that walks half a screen
// left and right of Yuuka while the spread's own angle steps through eight
// hardcoded directions.
static void near yuuka5_15F97(void)
{
	if(boss.phase_frame == 1) {
		yuuka5_25662 = (boss.pos.cur.x.v + TO_SP(-32));
		bullet_template.spawn_type = BST_BULLET16;
		bullet_template.patnum = PAT_BULLET16_N_CROSS_YELLOW;
		bullet_template.group = BG_SPREAD;
		bullet_template.count = 11;
		bullet_template.delta.spread_angle = 5;
		bullet_template.speed.v = (TO_SP(2) + 8);
		bullet_template_tune();
	} else if(boss.phase_frame == 15) {
		bullet_template.angle = 0x00;
	} else if(boss.phase_frame == 31) {
		yuuka5_25662 += TO_SP(64);
		bullet_template.angle = 0x80;
	} else if(boss.phase_frame == 47) {
		yuuka5_25662 -= TO_SP(64);
		bullet_template.angle = 0x10;
	} else if(boss.phase_frame == 63) {
		yuuka5_25662 += TO_SP(64);
		bullet_template.angle = 0x70;
	} else if(boss.phase_frame == 79) {
		yuuka5_25662 -= TO_SP(64);
		bullet_template.angle = 0x20;
	} else if(boss.phase_frame == 95) {
		yuuka5_25662 += TO_SP(64);
		bullet_template.angle = 0x60;
	} else if(boss.phase_frame == 111) {
		yuuka5_25662 -= TO_SP(64);
		bullet_template.angle = 0x30;
	} else if(boss.phase_frame == 127) {
		yuuka5_25662 += TO_SP(64);
		bullet_template.angle = 0x50;
	} else if(boss.phase_frame == 140) {
		boss.mode = 255;
		boss.phase_frame = 0;
	}
	if((boss.phase_frame % 16) == 15) {
		bullet_template.origin.x.v = yuuka5_25662;
		bullet_template.origin.y.v = boss.pos.cur.y.v;
		bullets_add_regular();
		snd_se_play(3);
	}
}

// An accelerating stream of forward-cloud rings, sweeping clockwise for the
// first 128 frames and anticlockwise for the next 128, with a 32-bullet
// speed-up ring fired at each turn.
static void near yuuka5_160A5(void)
{
	if(boss.phase_frame == 1) {
		bullet_template.angle = 0;
		bullet_template.spawn_type = BST_BULLET16_CLOUD_FORWARDS;
		bullet_template.patnum = PAT_BULLET16_D_BLUE;
		bullet_template.group = BG_RING;
		_AL = rank;
		_AL++;
		bullet_template.count = _AL;
		bullet_template.speed.v = TO_SP(4);
		yuuka5_25664 = 1;
		yuuka5_25665 = 0;
	} else if(boss.phase_frame < 128) {
		if(yuuka5_25665 >= 0x10) {
			_AL = bullet_template.angle;
			_AL += 7;
			bullet_template.angle = _AL;
			bullets_add_regular();
			snd_se_play(3);
			yuuka5_25664++;
			_AL = yuuka5_25665;
			_AL += -0x10;
			yuuka5_25665 = _AL;
		}
		yuuka5_25665 += yuuka5_25664;
	} else if(boss.phase_frame == 128) {
		bullet_template.spawn_type = BST_BULLET16;
		bullet_template.patnum = PAT_BULLET16_N_CROSS_YELLOW;
		bullet_template.angle = 0x80;
		bullet_template.speed.v = TO_SP(1);
		bullet_template.special_motion = BSM_SPEEDUP;
		bullet_special.speed_delta.v = 1;
		bullet_template.count = 32;
		bullet_template_tune();
		bullets_add_special_fixedspeed();
		bullet_template.spawn_type = BST_BULLET16_CLOUD_FORWARDS;
		bullet_template.speed.v = TO_SP(4);
		_AL = rank;
		_AL++;
		bullet_template.count = _AL;
		yuuka5_25664 = 1;
		yuuka5_25665 = 0;
		snd_se_play(9);
		bullet_template.patnum = PAT_BULLET16_D_BLUE;
	} else if(boss.phase_frame < 256) {
		if(yuuka5_25665 >= 0x10) {
			_AL = bullet_template.angle;
			_AL += -7;
			bullet_template.angle = _AL;
			bullets_add_regular();
			snd_se_play(3);
			yuuka5_25664++;
			_AL = yuuka5_25665;
			_AL += -0x10;
			yuuka5_25665 = _AL;
		}
		yuuka5_25665 += yuuka5_25664;
	} else if(boss.phase_frame == 256) {
		bullet_template.spawn_type = BST_BULLET16;
		bullet_template.patnum = PAT_BULLET16_N_CROSS_YELLOW;
		bullet_template.angle = 0;
		bullet_template.speed.v = TO_SP(1);
		bullet_template.special_motion = BSM_SPEEDUP;
		bullet_special.speed_delta.v = 1;
		bullet_template.count = 32;
		bullet_template_tune();
		bullets_add_special_fixedspeed();
		snd_se_play(9);
	} else if(boss.phase_frame == 288) {
		boss.mode = 255;
		boss.phase_frame = 0;
	}
}

// A three-circle gather that opens into a 7-way spread of bouncing bullets,
// re-fired at a random angle every 16th frame. Its `switch` is sparse, which
// is what the value/jump table pair and the one padding byte behind this
// function are — see the `#pragma option -a2` note above yuuka5_update().
#pragma option -a2
static void near yuuka5_161D7(void)
{
	switch(boss.phase_frame) {
	case 1:
		gather_template.center.x.v = bullet_template.origin.x.v;
		gather_template.center.y.v = bullet_template.origin.y.v;
		gather_template.ring_points = 32;
		gather_template.col = 11;
		gather_template.radius.v = TO_SP(256);
		gather_template.angle_delta = 3;
		gather_add_only();
		break;
	case 3:
		gather_template.col = 10;
		gather_add_only();
		break;
	case 5:
		gather_add_only();
		break;
	case 0x11:
		circles_add_shrinking(
			bullet_template.origin.x.v, bullet_template.origin.y.v
		);
		circles_color = V_WHITE;
		bullet_template.spawn_type = BST_BULLET16;
		bullet_template.speed.v = TO_SP(1);
		bullet_template.special_motion = BSM_BOUNCE_LEFT_RIGHT_TOP;
		bullet_template.group = BG_SPREAD;
		bullet_template.count = 7;
		bullet_template.delta.spread_angle = 8;
		bullet_template.patnum = PAT_BULLET16_N_CROSS_YELLOW;
		bullet_template.speed.v = TO_SP(2);
		bullet_template_tune();
		bullet_special.turns_max = 1;
		break;
	}
	if(boss.phase_frame >= 32) {
		if((boss.phase_frame % 16) == 0) {
			bullet_template.angle = randring2_next16();
			bullets_add_special();
			snd_se_play(9);
		}
	}
}
#pragma option -a1

// One aimed speed-up ring every 16th frame, three bullets wider each time.
static void near yuuka5_162A3(void)
{
	if(boss.phase_frame == 1) {
		bullet_template.spawn_type = BST_BULLET16;
		bullet_template.patnum = PAT_BULLET16_N_CROSS_YELLOW;
		bullet_template.group = BG_RING_AIMED;
		bullet_template.delta.spread_angle = 6;
		bullet_template.speed.v = TO_SP(1);
		bullet_template.special_motion = BSM_SPEEDUP;
		bullet_template.count = 8;
		bullet_special.speed_delta.v = 1;
	} else if(boss.phase_frame == 170) {
		boss.mode = 255;
		boss.phase_frame = 0;
	}
	if((boss.phase_frame % 16) == 15) {
		bullets_add_special();
		_AL = bullet_template.count;
		_AL += 3;
		bullet_template.count = _AL;
		snd_se_play(3);
	}
}

// One aimed 5-way spread every 8th frame, narrowing by 4 units per volley
// from a 0x42 fan that starts wider than a right angle.
static void near yuuka5_1630D(void)
{
	if(boss.phase_frame == 1) {
		bullet_template.angle = iatan2(
			(player_pos.cur.y.v - bullet_template.origin.y.v),
			(player_pos.cur.x.v - bullet_template.origin.x.v)
		);
		bullet_template.spawn_type = BST_BULLET16_CLOUD_FORWARDS;
		bullet_template.patnum = PAT_BULLET16_N_BALL_BLUE;
		bullet_template.group = BG_SPREAD;
		bullet_template.count = 5;
		bullet_template.delta.spread_angle = 0x42;
		bullet_template.speed.v = TO_SP(5);
		bullet_template_tune();
	} else if(boss.phase_frame == 128) {
		boss.mode = 255;
		boss.phase_frame = 0;
	}
	if((boss.phase_frame % 8) == 7) {
		_AL = bullet_template.delta.spread_angle;
		_AL += -4;
		bullet_template.delta.spread_angle = _AL;
		bullets_add_regular();
		snd_se_play(3);
	}
}

// Yuuka's last pattern: six alternating gather circles that whiten the whole
// palette, a thick laser at frame 0x60, then rings every 32nd frame and a pair
// of random-angle bullets on every other frame past frame 192.
//
// Its `switch` is the sparsest in the fight — 20 cases over seven blocks — and
// the six repetitions differ only in which of the three blocks they enter. The
// `goto` below is one of those entries written out: the arm that only recolors
// the gather jumps BACKWARDS into the arm that adds it, which is emitted
// ahead of it, so it cannot be a fall-through.
static void near yuuka5_16389(void)
{
	switch(boss.phase_frame) {
	case 0x10:
		gather_template.angle_delta = 3;
		break;

	case 0x30:
		bullet_template.angle = 0;
		snd_se_play(8);
		yuuka5_25666 = 100;
		// fall through
	case 0x38: case 0x40: case 0x48: case 0x50:
		circles_color = V_WHITE;
		circles_add_shrinking(
			bullet_template.origin.x.v, bullet_template.origin.y.v
		);
		// fall through
	case 0x28:
		_AL = gather_template.angle_delta;
		_AL = -_AL;
		gather_template.angle_delta = _AL;
		gather_template.center.x.v = bullet_template.origin.x.v;
		gather_template.center.y.v = bullet_template.origin.y.v;
		gather_template.ring_points = 8;
		gather_template.col = 9;
		gather_template.radius.v = TO_SP(256);
		// fall through
	case 0x2C: case 0x34: case 0x3C: case 0x44: case 0x4C: case 0x54:
gather:
		gather_add_only();
		break;

	case 0x2A: case 0x32: case 0x3A: case 0x42: case 0x4A: case 0x52:
		gather_template.col = 8;
		goto gather;

	case 0x60:
		thicklaser_template.origin.x.v = bullet_template.origin.x.v;
		thicklaser_template.origin.y.v = bullet_template.origin.y.v;
		thicklaser_template.radius_max = yuuka5_thicklaser_radius;
		thicklaser_template.radius_speed = 6;
		thicklaser_template.line_frames = 32;
		thicklaser_template.static_frames = 144;
		thicklaser_template.col_outline = 8;
		thicklaser_add();
		break;
	}

	// An early `return`, where the identical test 20 lines down is an `if`:
	// the original's first `jl` is a NEAR jump to the function's one exit and
	// the second is a near jump to the same place, while an
	// `if(…>= 128) { … } if(… >= 128) { … }` pair makes the first one a SHORT
	// jump to the second block and the function two bytes shorter.
	if(boss.phase_frame < 128) {
		return;
	}
	if(boss.phase_frame <= 160) {
		_AL = yuuka5_25666;
		_AL += 2;
		yuuka5_25666 = _AL;
		goto tone;
	} else if(yuuka5_25666 > 100) {
		// The ramp back down is half-speed, because it only subtracts on the
		// odd frames.
		_AL = boss.phase_frame;
		_AL &= 1;
		_DL = yuuka5_25666;
		_DL -= _AL;
		yuuka5_25666 = _DL;
tone:
		PaletteTone = yuuka5_25666;
		palette_changed = true;
	}
	if(boss.phase_frame >= 128) {
		if((boss.phase_frame % 32) == 0) {
			bullet_template.spawn_type = BST_BULLET16;
			bullet_template.group = BG_RING;
			bullet_template.count = 32;
			bullet_template.patnum = PAT_BULLET16_D_BLUE;
			bullet_template.speed.v = (TO_SP(4) + 8);
			bullet_template_tune();
			bullets_add_regular();
			_AL = bullet_template.angle;
			_AL += 2;
			bullet_template.angle = _AL;
			snd_se_play(9);
		}
		if(boss.phase_frame >= 192) {
			if(stage_frame_mod2) {
				bullet_template.spawn_type = BST_BULLET16;
				bullet_template.group = BG_RANDOM_ANGLE;
				bullet_template.patnum = PAT_BULLET16_N_OUTLINED_BALL_BLUE;
				bullet_template.speed.v = TO_SP(2);
				bullet_template.count = 2;
				bullet_template_tune();
				bullets_add_regular();
			}
		}
	}
}

// Two mirrored pairs every 8th frame from 32 pixels either side of Yuuka: a
// 5-way cloud spread walking 16 units anticlockwise, and a 3-way pellet spread
// walking 9 units the other way.
static void near yuuka5_1653D(void)
{
	if(boss.phase_frame == 48) {
		circles_add_shrinking(
			bullet_template.origin.x.v, bullet_template.origin.y.v
		);
		circles_color = V_WHITE;
		boss.angle = 16;
		yuuka5_spread_angle = 0x10;
		bullet_template.special_motion = BSM_NONE;
		return;
	}
	if(boss.phase_frame >= 64) {
		if((boss.phase_frame % 8) == 0) {
			bullet_template.spawn_type = BST_BULLET16_CLOUD_FORWARDS;
			bullet_template.count = 5;
			bullet_template.delta.spread_angle = 1;
			bullet_template.group = BG_SPREAD;
			bullet_template.patnum = PAT_BULLET16_N_BALL_BLUE;
			bullet_template.speed.v = (TO_SP(2) + 8);
			bullet_template_tune();

			bullet_template.origin.x.v += TO_SP(32);
			bullet_template.angle = boss.angle;
			bullets_add_special();
			bullet_template.origin.x.v -= TO_SP(64);
			_AL = 0x80;
			_AL -= boss.angle;
			bullet_template.angle = _AL;
			bullets_add_special();
			_AL = boss.angle;
			_AL += -16;
			boss.angle = _AL;

			bullet_template.spawn_type = BST_PELLET;
			bullet_template.count = 3;
			bullet_template.speed.v = (TO_SP(1) + 8);
			bullet_template_tune();
			bullet_template.angle = yuuka5_spread_angle;
			bullets_add_special();
			bullet_template.origin.x.v += TO_SP(64);
			_AL = 0x80;
			_AL -= yuuka5_spread_angle;
			bullet_template.angle = _AL;
			bullets_add_special();
			_AL = yuuka5_spread_angle;
			_AL += 9;
			yuuka5_spread_angle = _AL;
			snd_se_play(3);
		}
	}
}
/// ----------------

// Yuuka's Stage 5 fight, all 19 phases of it. Three of them are the entrance
// and the defeat; the other 16 are five repetitions of the same
// pattern/warp/pattern triple, differing only in which two pattern functions
// the [boss.mode] dispatch below reaches and in how much HP the phase costs.
//
// [boss.mode] is that dispatch's own state, and its two negative values are
// not patterns: 254 advances to the next pattern of the pair, 255 runs the
// warp. Both of the dispatches over it are sparse, which is why this tail
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
		// Mod: Prevent the division by zero by not moving Marisa at all in
		// that case.
		int frames_to_point = ((duration / 2) - (BRAKE_DURATION / 2));
		if(frames_to_point == 0) {
			return true;
		}
		boss.pos.velocity.x.v = ((POINT_X - boss.pos.cur.x) / frames_to_point);
		boss.pos.velocity.y.v = ((POINT_Y - boss.pos.cur.y) / frames_to_point);
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
