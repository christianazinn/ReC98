/// Stage 4 midboss - update function
/// ---------------------------------
/// (#included from th04/enm_pos.cpp, ahead of enemy_pos_update() and
/// enemy_velocity_set(), which is this function's original address order at
/// the head of ENM_POS_TEXT's C++ object. That ZUN's object for this segment
/// held the four midboss update functions and the two enemy position helpers
/// is kb/codegen/0112.)
///
/// midboss4_render() is th04/main/midboss/m4.cpp, in a different segment and
/// therefore a different object.

#include "platform.h"
#include "pc98.h"
// iatan2(), for the two patterns below that re-aim at the player.
// th04/main/enemy/velocity.cpp, the third file in this object, includes this
// same header; `[measured]` it survives the second expansion, unlike
// th04/main/enemy/enemy.hpp two lines further down that file.
#include "libs/master.lib/master.hpp"
#include "th04/snd/snd.h"
#include "th04/math/randring.hpp"
#include "th04/sprites/main_pat.h"
#include "th04/main/player/player.hpp"
#include "th04/main/homing.hpp"
#include "th04/main/scroll.hpp"
#include "th04/main/spark.hpp"
#include "th04/main/hud/hud.hpp"
#include "th04/main/item/item.hpp"
#include "th04/main/bullet/bullet.hpp"
#include "th04/main/bullet/clearzap.hpp"
// MIDBOSS_DEC(4) in th04/main/midboss/midboss.hpp declares midboss4_render()
// `near`, which is what it is. This object needs a FAR view of it, because a
// near reference under `-zPmain_03` frames its offset on main_03 and that
// function is main_01 (kb/codegen/0162) -- and Turbo C++ 4.02 rejects both a
// second file-scope declaration and a block-scope one. So the header's
// declaration is renamed out of the way as the header is expanded. Nothing in
// this translation unit calls midboss4_render() at all, so the renamed
// declaration is never referenced and emits no EXTDEF.
#define midboss4_render midboss4_render_near_unused
#include "th04/main/midboss/midboss.hpp"
#undef midboss4_render
#include "th04/main/phase.hpp"

void pascal far midboss4_render(void);

// The midboss's own state, all of them th04_main.asm `.data?` bytes with no
// `public` of ZUN's. `[inferred]`, and **a naming round is owed**: every read
// and write of all three is inside this function and the four patterns below,
// and the four are what set [midboss4_pattern] back to 255.
extern "C" {
	// Which of the four patterns is running, or 255 while the midboss is
	// traversing the playfield between two of them.
	extern unsigned char midboss4_pattern;

	// Traversals completed. The eighth one is the last: after it the midboss
	// leaves through the side instead of turning around, and the score bonus
	// for killing it is `30 - this`.
	extern unsigned char midboss4_passes;

	// Written 0 exactly once, here, and read by nothing in any of the five
	// binaries. ZUN bloat, and the reason it keeps an address-suffixed name.
	extern unsigned char midboss4_255C8;
}

/// ---------

/// The Stage 4 midboss's four bullet patterns
/// ------------------------------------------
/// All four sat directly above midboss4_update() in ZUN's object, and every
/// one of them is reached from its `switch(midboss4_pattern)` and from nowhere
/// else. They keep the address-suffixed names the previous parcel gave them
/// (`state/notes/midboss4_update.md`); **a naming round is owed for all four**,
/// together with [midboss4_pattern], [midboss4_passes] and [midboss4_22B9E].
///
/// All four share one three-arm skeleton, written out longhand in each because
/// that is how the original emits it:
///
/// 1) frames 0…24, the wind-up: the sprite ramps up through cels 0…3, and the
///    24th frame runs the pattern's own one-time setup;
/// 2) the firing window, whose length and interval are the pattern's own;
/// 3) a wind-DOWN ramp back through the same cels, and then
///    [midboss4_pattern] = 255, which sends the midboss back to traversing.

// A coin that is flipped once per activation of midboss4_15027(), and read
// only by it: its low bit picks between re-aiming that pattern's stacks at the
// player and walking them 8 units anticlockwise. th04_main.asm `.data`,
// initialized to 1, and private to ZUN's object, so it needed a zero-byte
// `label` alias to become linkable (kb/codegen/0123). `[measured]`: those two
// sites are the only reads or writes of it in any of the five binaries.
extern "C" unsigned char midboss4_22B9E;

// Random pellet-and-ball spray: every 4th frame, one aimed 2-spread of pellets
// and one 16×16 ball, both at a random speed and a random angle offset of
// ±0x30 around the player.
static void near midboss4_14F78(void)
{
	if(midboss.phase_frame <= 24) {
		midboss.sprite = (midboss.phase_frame / 8);
		if(midboss.phase_frame == 24) {
			snd_se_play(6);
		}
	} else if(midboss.phase_frame < 128) {
		if((midboss.phase_frame & 3) == 0) {
			bullet_template.group = BG_SPREAD_AIMED;
			bullet_template.count = 2;
			bullet_template.delta.spread_angle = 6;

			// kb/codegen/0032: the original adds the minimum to the returned
			// byte in AL and then stores it. The `u` on the half-range is
			// kb/codegen/0022 and is load-bearing: a signed `0x30` here
			// emits `add al, 0xD0` where the original has `sub al, 0x30`.
			// Same length, different bytes, so only a funcdiff sees it.
			_AL = randring2_next16_mod(TO_SP(1) + 8);
			_AL += TO_SP(2);
			bullet_template.speed.v = _AL;
			_AL = randring2_next16_mod(0x60);
			_AL -= 0x30u;
			bullet_template.angle = _AL;

			bullet_template_tune();
			bullets_add_regular();
			bullet_template.spawn_type = BST_BULLET16;
			bullet_template.patnum = PAT_BULLET16_N_SMALL_BALL_YELLOW;
			_AL = randring2_next16_mod(TO_SP(1) + 8);
			_AL += TO_SP(2);
			bullet_template.speed.v = _AL;
			_AL = randring2_next16_mod(0x60);
			_AL -= 0x30u;
			bullet_template.angle = _AL;
			bullets_add_regular();
			snd_se_play(3);
		}
	} else if(midboss.phase_frame < 152) {
		midboss.sprite = ((159 - midboss.phase_frame) / 8);
	} else {
		midboss.phase_frame = 0;
		midboss4_pattern = 255;
	}
}

// Stacks of 12, fired every 16 frames and mirrored around the vertical center
// line, with [midboss4_22B9E] deciding whether the volley is re-aimed at the
// player or walked one step anticlockwise from the last one.
static void near midboss4_15027(void)
{
	unsigned char angle_before_mirror;

	if(midboss.phase_frame <= 24) {
		midboss.sprite = (midboss.phase_frame / 8);
		if(midboss.phase_frame == 24) {
			snd_se_play(6);

			// Away from the closer side wall. An `if`/`else` with two
			// direct stores, not a ternary: a ternary picks the byte in AL
			// and stores it once, which is 10 bytes where the original's two
			// `mov [angle], imm8` are 12. The half with the LOWER angle is
			// the one that has to be written first, or the branch inverts.
			if(midboss.pos.cur.x.v < TO_SP(192)) {
				bullet_template.angle = 0x36;
			} else {
				bullet_template.angle = 0x56;
			}
			midboss4_22B9E++;
		}
	} else if(midboss.phase_frame <= 136) {
		if(((midboss.phase_frame - 25) & 0x0F) == 0) {
			bullet_template.group = BG_STACK;
			if(midboss4_22B9E & 1) {
				_AL = iatan2(
					(player_pos.cur.y.v - midboss.pos.cur.y.v),
					(player_pos.cur.x.v - midboss.pos.cur.x.v)
				);
			} else {
				_AL = bullet_template.angle;
				_AL += -8;
			}
			bullet_template.angle = _AL;
			angle_before_mirror = bullet_template.angle;
			snd_se_play(3);
			bullet_template.spawn_type = BST_BULLET16;
			bullet_template.patnum = PAT_BULLET16_N_SMALL_BALL_YELLOW;
			bullet_template_tune();
			bullet_template.speed.v = TO_SP(1);
			bullet_template.count = 12;
			bullet_template.delta.stack_speed.v = 7;
			bullets_add_regular_fixedspeed();

			// The mirror: reflect the volley's angle around whichever
			// horizontal axis the midboss is NOT on the side of.
			_AL = ((midboss.pos.cur.x.v > TO_SP(192)) ? -0x60 : 0x60);
			_AL -= bullet_template.angle;
			bullet_template.angle = _AL;
			bullets_add_regular_fixedspeed();

			bullet_template.angle = angle_before_mirror;
		}
	} else if(midboss.phase_frame < 160) {
		midboss.sprite = ((165 - midboss.phase_frame) / 8);
	} else {
		midboss.phase_frame = 0;
		midboss4_pattern = 255;
	}
}

// Two interleaved streams: a random-width spread of pellets every 8th frame,
// and a 32-frame volley of eight accelerating yellow bullets along one angle
// that is re-aimed at the player at the start of each volley.
static void near midboss4_1511D(void)
{
	int frame_in_volley;

	if(midboss.phase_frame <= 24) {
		midboss.sprite = (midboss.phase_frame / 8);
		if(midboss.phase_frame == 24) {
			snd_se_play(6);
		}
	} else if(midboss.phase_frame <= 120) {
		if((midboss.phase_frame & 7) == 0) {
			bullet_template.group = BG_SPREAD_AIMED;

			// 1, 3, 5 or 7 bullets, 0x0A…0x11 apart.
			_AL = randring2_next16_and(3);
			_AL += _AL;
			_AL++;
			bullet_template.count = _AL;
			_AL = randring2_next16_and(7);
			_AL += 0x0A;
			bullet_template.delta.spread_angle = _AL;

			bullet_template.speed.v = (TO_SP(3) + 2);
			bullet_template.angle = 0;
			bullet_template_tune();
			bullets_add_regular_fixedspeed();
		}
		frame_in_volley = ((midboss.phase_frame - 25) & 0x1F);
		if(frame_in_volley == 0) {
			midboss.angle = iatan2(
				(player_pos.cur.y.v - midboss.pos.cur.y.v),
				(player_pos.cur.x.v - midboss.pos.cur.x.v)
			);
		}
		if((frame_in_volley & 3) == 0) {
			bullet_template.spawn_type = BST_BULLET16;
			bullet_template.group = BG_SINGLE;
			bullet_template.patnum = PAT_BULLET16_D_YELLOW;

			// Each of the volley's eight bullets is faster than the last, so
			// the whole volley arrives as a line rather than a stream.
			_AL = (frame_in_volley * 3);
			_AL += (TO_SP(2) + 8);
			bullet_template.speed.v = _AL;

			bullet_template.angle = midboss.angle;
			bullet_template_tune();
			bullets_add_regular_fixedspeed();
			snd_se_play(3);
		}
	} else if(midboss.phase_frame < 144) {
		midboss.sprite = ((151 - midboss.phase_frame) / 8);
	} else {
		midboss.phase_frame = 0;
		midboss4_pattern = 255;
	}
}

// A 3-spread every other frame, walking 6 units clockwise per volley and
// reflected around the vertical center line while the midboss is on the left
// half of the playfield — twice, so the stored angle keeps walking in the same
// direction either way.
static void near midboss4_15202(void)
{
	if(midboss.phase_frame <= 24) {
		midboss.sprite = (midboss.phase_frame / 8);
		if(midboss.phase_frame == 24) {
			snd_se_play(6);
			bullet_template.angle = -0x20;
		}
	} else if(midboss.phase_frame < 128) {
		if((midboss.phase_frame & 2) == 0) {
			bullet_template.group = BG_SPREAD;
			bullet_template.count = 3;
			bullet_template.delta.spread_angle = 6;
			bullet_template.speed.v = (TO_SP(3) + 12);
			if(midboss.pos.cur.x.v < TO_SP(192)) {
				_AL = 0x80;
				_AL -= bullet_template.angle;
				bullet_template.angle = _AL;
			}
			bullet_template_tune();
			bullets_add_regular();
			if(midboss.pos.cur.x.v < TO_SP(192)) {
				_AL = 0x80;
				_AL -= bullet_template.angle;
				bullet_template.angle = _AL;
			}
			snd_se_play(3);
			_AL = bullet_template.angle;
			_AL += 6;
			bullet_template.angle = _AL;
		}
	} else if(midboss.phase_frame < 152) {
		midboss.sprite = ((159 - midboss.phase_frame) / 8);
	} else {
		midboss.phase_frame = 0;
		midboss4_pattern = 255;
	}
}
/// ------------------------------------------

// Both halves of the fight test the same box.
#define midboss4_hittest(se_on_hit) \
	midboss_hittest_shots_damage(TO_SP(24), TO_SP(24), se_on_hit)

// `#pragma option -a2` is the one padding byte between this function's
// epilogue and its generated value/jump table pair. kb/codegen/0160 for the
// instrument: read the OBJ's PUBDEF offsets, not the `tcc -S` listing.
// `[measured]` at a zero prefix -- which is what this function being the first
// thing the object emits gives it -- the object is 0x29B bytes to
// enemy_pos_update() and carries the pad; without `-a2` it is 0x29A and does
// not. That is the same direction as BOSS_BG_TEXT's and the opposite of
// B4M_UPDATE_TEXT's, which is why 0160 says to probe rather than compute.
#pragma option -a2
void pascal far midboss4_update(void)
{
	int damage;

	homing_target.x.v = midboss.pos.cur.x.v;
	homing_target.y.v = midboss.pos.cur.y.v;

	if(midboss.phase == 0) {
		// The entrance: it drifts in from the top right and is invincible
		// until it stops, but the hittest still runs.
		midboss.pos.update_seg3();
		midboss.phase_frame++;
		damage = midboss4_hittest(10); // ZUN bloat: never read
		if(midboss.phase_frame >= 48) {
			midboss.phase++;
			midboss.phase_frame = 0;
			midboss.pos.velocity.x.v = 0;
			midboss.pos.velocity.y.v = 0;
			midboss4_255C8 = 0;
			midboss4_pattern = 0;
			midboss4_passes = 0;
		}
	} else if(midboss.phase == 1) {
		midboss.pos.update_seg3();
		midboss.phase_frame++;
		bullet_template.spawn_type = BST_PELLET;
		bullet_template.origin.x.v = midboss.pos.cur.x.v;
		bullet_template.origin.y.v = (midboss.pos.cur.y.v - TO_SP(16));

		switch(midboss4_pattern) {
		case 0:
			midboss4_14F78();
			break;
		case 1:
			midboss4_1511D();
			break;
		case 2:
			midboss4_15027();
			break;
		case 3:
			midboss4_15202();
			break;
		case 255:
			if(midboss.phase_frame == 1) {
				// Away from whichever half of the playfield it stopped in.
				midboss.pos.velocity.x.v = ((
					midboss.pos.cur.x.v >= TO_SP(180)
				) ? -TO_SP(4) : TO_SP(4));
			} else if(midboss4_passes < 8) {
				if(
					(midboss.pos.cur.x.v <= TO_SP(48)) ||
					(midboss.pos.cur.x.v >= TO_SP(336))
				) {
					midboss.pos.velocity.x.v = 0;
					midboss.phase_frame = 0;
					midboss4_passes++;
					midboss4_pattern = (midboss4_passes & 3);
				}
			} else if(
				// A whole sprite past either edge, since this traversal is
				// the one it never comes back from.
				(midboss.pos.cur.x.v <= TO_SP(-32)) ||
				(midboss.pos.cur.x.v >= TO_SP(416))
			) {
				midboss.phase = 3;
			}
			break;
		}

		if(
			(midboss.pos.cur.y.v >= TO_SP(368)) ||
			(midboss.pos.cur.x.v <= 0) ||
			(midboss.pos.cur.x.v >= TO_SP(384))
		) {
			midboss.phase = PHASE_NONE;
		}
		damage = midboss4_hittest(4);
		if(damage) {
			midboss.hp -= damage;
			if(midboss.hp > 0) {
				midboss.damage_this_frame = 1;
			} else {
				midboss.damage_this_frame = 1;

				// ZUN bloat: the return value is never read, and neither is
				// the assignment below it.
				damage = scroll_subpixel_y_to_vram_always(
					midboss.pos.cur.y.v - TO_SP(16)
				);

				bullet_zap.active = true;
				midboss_score_bonus(30 - midboss4_passes);
				playfield_shake_anim_time = 12;
				midboss.phase = PHASE_EXPLODE_BIG;
				midboss.sprite = 4;
				midboss.phase_frame = 0;
				midboss.pos.velocity.x.v = 0;
				sparks_add_circle(
					midboss.pos.cur.x, midboss.pos.cur.y, TO_SP(6), 48
				);
				snd_se_play(12);

				// The first of the stage's two midbosses drops the bomb.
				if(midboss.frames_until == 2800) {
					items_add(
						midboss.pos.cur.x.v, midboss.pos.cur.y.v, IT_BOMB
					);
				} else {
					items_add(
						midboss.pos.cur.x.v, midboss.pos.cur.y.v, IT_1UP
					);
				}
			}
		}
	} else if(midboss.phase == PHASE_EXPLODE_BIG) {
		midboss.pos.velocity.x.v = 0;
		midboss.pos.velocity.y.v = 0;
		midboss.pos.update_seg3();
		midboss.phase_frame++;
		if((midboss.phase_frame % 16) == 0) {
			midboss.sprite++;
			if(midboss.sprite >= 12) {
				midboss.phase++;
				midboss.hp = 0;
			}
		}
	} else {
		// kb/codegen/0014: a far call to a function in the same GROUP is a
		// near call with CS pushed by hand, not the inter-segment call Turbo
		// C++ emits for a far target on its own.
		_asm { nop; push cs; call near ptr midboss_reset }

		// …and the second one re-arms itself as the same midboss, which is
		// the only reason this function is ever installed twice.
		if(midboss.frames_until == 2800) {
			midboss.frames_until = 5600;
			midboss_update_func = midboss4_update;
			_asm mov word ptr midboss_render_func, offset midboss4_render
			midboss.pos.cur.x.v = TO_SP(240);
			midboss.pos.cur.y.v = TO_SP(-32);
			midboss.pos.prev.x.v = TO_SP(240);
			midboss.pos.prev.y.v = TO_SP(-32);
			midboss.pos.velocity.x.v = -TO_SP(4);
			midboss.pos.velocity.y.v = TO_SP(2);
			midboss.hp = 1200;
			midboss.sprite = 0;
			midboss.phase_frame = 0;
			return;
		}
	}
	hud_hp_update_and_render(midboss.hp, 1200);
}
#pragma option -a1
/// ---------------------------------
