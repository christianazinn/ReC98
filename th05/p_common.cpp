/* ReC98
 * -----
 * Generic shot control functions
 */

#pragma option -zCmain_01_TEXT -zPmain_01

// kb/codegen/0155: Turbo C++ 4.02 binds a function to a code segment at its
// FIRST DECLARATION, not at its definition. shots_render() is declared by
// th04/main/player/shot.hpp, which the #include below reaches, so the
// `#pragma codeseg HITSHOT_TEXT` block at the end of this file could not place
// the body -- it went to the end of the main_01_TEXT contribution instead, and
// cost this parcel its one build cycle. Neither of that entry's two published
// fixes applies: the definition needs the header's types, and shot.hpp is read
// by every other lane's parcels. So this is its third, measured here -- a local
// forward declaration inside the pragma pair, ahead of every #include.
//
// The pair that entry warns about is harmless in exactly this position, and
// that is measured rather than assumed: a REdeclaration does not move a
// binding, so the header's later one is inert. A declaration also emits
// nothing, and every other function defined in this object is first declared
// after the restore below -- which is why the three matched bodies here did
// not move a byte.
#pragma codeseg HITSHOT_TEXT main_01
void pascal near shots_render(void);
#pragma codeseg main_01_TEXT main_01

#include "th05/main/player/shot.hpp"
#include "th02/snd/snd.h"

#pragma option -a2

char near shot_cycle_init(void)
{
	char cycle = 0;
	switch(shot_time) {
	case 18:
		cycle = SC_6X | SC_3X | SC_2X | SC_1X;
		snd_se_play(1);
		break;
	case 15:
		cycle = SC_6X;
		break;
	case 12:
		cycle = SC_6X | SC_3X;
		snd_se_play(1);
		break;
	case  9:
		cycle = SC_6X | SC_2X;
		break;
	case  6:
		cycle = SC_6X | SC_3X;
		snd_se_play(1);
		break;
	case  3:
		cycle = SC_6X;
		break;
	default:
		return 0;
	}
	shot_ptr = shots;
	shot_last_id = 0;
	return cycle;
}
void pascal near shot_l0(void)
{
	if( (shot_cycle_init() & SC_3X) == 0) {
		return;
	}
	Shot near *shot;
	if(( shot = shots_add() ) != nullptr) {
		shot->damage = 10;
	}
}

void pascal near shot_l1(void)
{
	if( (shot_cycle_init() & SC_3X) == 0) {
		return;
	}
	Shot near *shot;
	if(( shot = shots_add() ) != nullptr) {
		shot_velocity_set(
			&shot->pos.velocity, randring1_next8_ge_lt(-0x44, -0x3C)
		);
		shot->damage = 10;
	}
}
/// shots_render() and hitshot_from(), in the head half of main_01_TEXT
/// -------------------------------------------------------------------
/// HITSHOT_TEXT is th05_main.asm's own new name for the head of that block,
/// split off by a kb/codegen/0080 carve so that this object can append to it.
/// The following main_01_TEXT root is empty. Its former two hittest procedures
/// are compiled from th05/main/player/shot_hit.cpp as a standalone object
/// immediately before this one, preserving this object's switch-table parity.
/// What the root block still holds above this segment is sub_1240B() -- TH05's
/// counterpart of shots_update() -- and the four-entry jump table its
/// per-shottype switch compiles to.
///
/// `#pragma codeseg` rather than a new translation unit, so this costs no
/// Tupfile.lua line (kb/codegen/0155). The group has to be named: every call
/// site is NEAR and in the same group.
///
/// This is the LAST thing in this file on purpose -- everything above it is
/// already matched, and an `#include` or a pragma ahead of matched code can
/// move it. The three headers below are here for the same reason, after every
/// matched body in the file rather than at the top of it.
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th04/formats/super.h"
#include "th04/hardware/grcg.hpp"
#include "th04/main/homing.hpp"

#pragma codeseg HITSHOT_TEXT main_01

// master.lib's GRCG_OFF_CLOBBERING macro, which spills the port number to DX
// instead of using the immediate-port form that _outportb_() would emit. Same
// spelling as th04/main/player/shots_render.cpp and th04/main/stage/loop.cpp.
#define grcg_off_clobbering_dx() outportb(0x7C, GC_OFF)

// TH05's counterpart of TH04's shots_update(), in the same slot of the same
// frame loop -- th04/main/stage/loop.cpp calls it there, under IDA's spelling,
// and that spelling stays: naming round 16 left the identity of this proc and
// its three siblings in that loop explicitly open
// (state/re/DETERMINISTIC_STATE_TH05.md), and TH04's counterpart in the
// adjacent slot, sub_10ABF(), reached the same verdict independently.
//
// One frame of every shot: retires the ones whose removal was requested last
// frame, runs the per-shottype homing and missile steering, moves the rest,
// drops the ones that left the playfield, and caches the survivors into
// [shots_alive] for shots_hittest() and shots_render(). Then one frame of every
// hitshot: its own clipping, its age, and the cel its age selects.
//
// `#pragma option -a1`, and it is not cosmetic. The four-entry jump table this
// function's `switch` compiles to sits immediately after the body at 1259Bh
// with ZERO pad bytes, and `-a2` -- which this file turns on for the three
// matched bodies above -- would pad it to an even offset. kb/codegen/0168: read
// the pad count before believing an odd table offset means anything else. The
// pragma is restored below rather than left off, because everything after it
// here was matched with `-a2` in force.
#pragma option -a1

extern "C" void near sub_1240B(void)
{
	// [shot] and [hitshot] take SI and DI. The third pointer, [sa], is
	// dereferenced three times against those two's dozen-odd and loses, so it
	// joins [i] and the homing delta on the frame -- [i] at [bp-2], [sa] at
	// [bp-4] and [delta] as a BYTE at [bp-5], in declaration order
	// (kb/codegen/0010, including its byte/word variant).
	Shot near *shot;
	HitShot near *hitshot;
	int i;
	shot_alive_t near *sa;
	unsigned char delta;

	// The new position is read back out of the registers the call returns it
	// in, exactly as th04/main/player/shots_update.cpp does for TH04's body.
	// The casts are load-bearing: the pseudo-registers are `unsigned`, and all
	// eight comparisons below are signed in the original.
	#define cur_x	static_cast<subpixel_t>(_AX)
	#define cur_y	static_cast<subpixel_t>(_DX)

	shots_alive_count = 0;
	shot = shots;
	sa = shots_alive;
	for(i = 0; i < SHOT_COUNT; (i++, shot++)) {
		// A removal requested last frame frees its slot here, one frame later
		// than it visually ended.
		if(shot->flag == F_REMOVE) {
			shot->flag = F_FREE;
			continue;
		}
		if(shot->flag == F_FREE) {
			continue;
		}
		if(shot->flag == F_ALIVE) {
			// THE CAST IS THE ZERO EXTENSION: [type] is a signed `char`, so a
			// bare `switch` on it promotes through `cbw` where the original
			// has `mov ah, 0`. Same device as hitshot_from()'s sprite compare
			// one block below, and kb/codegen/0142's on an add.
			switch(static_cast<unsigned char>(shot->type)) {
			case ST_HOMING:
				// Homing expires with age rather than on arrival, and the
				// shot keeps flying as an ordinary one afterwards.
				if(static_cast<unsigned char>(shot->age) >= 0x10) {
					shot->type = ST_NORMAL;
				}
				if(homing_target.x.v == Subpixel::None()) {
					break;
				}

				// How far the current heading is from the target's, as a
				// signed 8-bit turn: >= 0x80 means the target is to the left.
				delta = (shot->angle - iatan2(
					(homing_target.y - shot->pos.cur.y),
					(homing_target.x - shot->pos.cur.x)
				));
				// BOTH arms spell the iatan2() snap out, and `-O`'s
				// cross-jumping merges the two copies -- which is what turns the
				// first arm's `else` into a `jmp` into the second arm's copy of
				// it, and leaves the conditional above it un-inverted
				// (kb/codegen/0097). A `goto` is the WRONG reconstruction of that
				// `jmp`: it compiles to a single folded conditional branch and
				// comes out three bytes short. 0144's bound does not bite -- the
				// shared tail is one call and no conditional branch.
				if(delta >= 0x80) {
					delta = ((256 - delta) / 4);
					if(delta < 3) {
						shot->angle = iatan2(
							(homing_target.y - shot->pos.cur.y),
							(homing_target.x - shot->pos.cur.x)
						);
					} else {
						// THE CAST IS THE INSTRUCTION ORDER, not a readability
						// choice, and it is hitshot_from()'s three-byte lesson on
						// an ADD this time (kb/codegen/0142). Without it Turbo C++
						// loads the DESTINATION into AL first and adds the local to
						// it, three instructions and three bytes over; narrowing the
						// right-hand side to `char` instead loads the LOCAL and adds
						// AL straight into the field, which is what the original does.
						shot->angle += static_cast<char>(delta);
					}
				} else {
					delta = (delta / 4);
					if(delta < 3) {
						shot->angle = iatan2(
							(homing_target.y - shot->pos.cur.y),
							(homing_target.x - shot->pos.cur.x)
						);
					} else {
						shot->angle -= static_cast<char>(delta);
					}
				}
				shot_velocity_set(
					reinterpret_cast<SPPoint near *>(&shot->pos.velocity),
					shot->angle
				);
				break;

			// The missiles accelerate upwards; the left and right ones also
			// drift sideways by one subpixel per frame. The upward
			// acceleration is spelled out in all three arms and `-O` merges
			// the three copies, which is where the original's `jmp` out of
			// the left arm and the right arm's fallthrough both come from
			// (kb/codegen/0097 again).
			case ST_MISSILE_LEFT:
				shot->pos.velocity.x.v++;
				shot->pos.velocity.y.v -= 4;
				break;

			case ST_MISSILE_RIGHT:
				shot->pos.velocity.x.v--;
				shot->pos.velocity.y.v -= 4;
				break;

			case ST_MISSILE_STRAIGHT:
				shot->pos.velocity.y.v -= 4;
				break;
			}
		}

		/* _DX:_AX = */ shot->pos.update_seg1();
		if(!(
			(cur_x > TO_SP(-(SHOT_W / 2))) &&
			(cur_x < TO_SP(PLAYFIELD_W + (SHOT_W / 2))) &&
			(cur_y > TO_SP(-(SHOT_H / 2))) &&
			(cur_y < TO_SP(PLAYFIELD_H + (SHOT_H / 2)))
		)) {
			// Not freed immediately: F_REMOVE is retired at the top of the
			// next frame, by the branch above.
			shot->flag = F_REMOVE;
			continue;
		}
		sa->pos.x.v = cur_x;
		sa->pos.y.v = cur_y;
		sa->shot = shot;
		sa++;
		shots_alive_count++;
		shot->age++;
	}

	hitshot = hitshots;
	for(i = 0; i < HITSHOT_COUNT; (i++, hitshot++)) {
		// The out-of-range value the clipping branch below writes is retired
		// here, one frame later, exactly as F_REMOVE is for a shot.
		if(hitshot->age >= (HITSHOT_FRAMES + 1)) {
			hitshot->age = 0;
		}
		if(hitshot->age == 0) {
			continue;
		}

		/* _DX:_AX = */ hitshot->pos.update_seg1();
		if(!(
			(cur_x > TO_SP(-(HITSHOT_W / 2))) &&
			(cur_x < TO_SP(PLAYFIELD_W + (HITSHOT_W / 2))) &&
			(cur_y > TO_SP(-(HITSHOT_H / 2))) &&
			(cur_y < TO_SP(PLAYFIELD_H + (HITSHOT_H / 2)))
		)) {
			// ZUN bloat: A relic from TH04, where SF_DONE was equal to this
			// value: hitshots were still integrated into the regular `Shot`
			// structure, their flags started at 2, and HITSHOT_FRAMES_PER_CEL
			// was 3 rather than 4 in that game. (The dump's own comment.)
			hitshot->age = (HITSHOT_FRAMES + HITSHOT_CELS + 2);
			continue;
		}
		hitshot->age++;
		// A real modulo, unlike TH04's `& (HITSHOT_FRAMES_PER_CEL - 1)`:
		// TH05's cel length is 3, not a power of two.
		if((hitshot->age % HITSHOT_FRAMES_PER_CEL) == 1) {
			hitshot->patnum++;
		}
	}

	#undef cur_y
	#undef cur_x
}

#pragma option -a2

// Blits every shot in the [shots_alive] cache that sub_1240B() built earlier
// this frame, then every hitshot still inside its decay animation.
//
// TH04's body is a different function and stays in its own file: that game
// walks [shots] backwards rather than the alive cache, blits the option laser
// ahead of everything, and renders no hitshots at all, because a TH04 hitshot
// is a Shot in its SF_HIT decay range.
void pascal near shots_render(void)
{
	// Four candidates, two registers. The three near pointers all outrank
	// [i] (kb/codegen/0146's cross-type rule: a pointer beats an `int`, and
	// mentions do not trade across that boundary), and the two winners are
	// the two most-dereferenced ones -- [hitshot] in SI, [sa] in DI. [shot]
	// loses and joins [i] on the frame, which is what makes this frame two
	// words deep rather than one. Declaration order then picks the two slots:
	// [i] at [bp-2], [shot] at [bp-4] (kb/codegen/0010).
	int i;
	Shot near *shot;
	HitShot near *hitshot;
	shot_alive_t near *sa;

	_ES = SEG_PLANE_B;
	grcg_setmode_rmw();

	sa = shots_alive;
	for(i = 0; i < shots_alive_count; (i++, sa++)) {
		// An invalidated X marks a slot whose shot turned into a hitshot
		// during this frame's update; the loop below renders it instead.
		// (th04/main/player/shot.hpp says so at [shot_alive_t] itself.)
		if(sa->pos.x.v == Subpixel::None()) {
			continue;
		}
		shot = sa->shot;

		// CX rather than a local, and it has to stay live across the
		// scroll_subpixel_y_to_vram_seg1() call below, which no compiler
		// temporary could be. Only the pseudo-registers express that.
		// Unlike TH04, the two-frame sprite alternation is unconditional
		// here -- a hitshot has left [shots_alive] by now, so there is no
		// decaying shot in this loop to exclude from it.
		_CH = 0;
		_CL = shot->patnum_base;
		_AL = shot->age;
		_AL &= 1;
		_AL += _CL;
		_CL = _AL;
		z_super_roll_put_tiny_16x16(
			(sa->pos.x.to_pixel() + (PLAYFIELD_LEFT - (SHOT_W / 2))),
			scroll_subpixel_y_to_vram_seg1(
				sa->pos.y.v + TO_SP(PLAYFIELD_TOP - (SHOT_H / 2))
			),
			_CX
		);
	}

	hitshot = hitshots;
	for(i = 0; i < HITSHOT_COUNT; (i++, hitshot++)) {
		// Both halves are unsigned compares -- `jnb` then `jbe` -- which is
		// what HitShot::age being an `unsigned char` gives. The upper bound
		// is also what keeps a clipped hitshot off the screen: the ZUN bloat
		// value hitshot_from()'s clipping branch writes into [age] is
		// (HITSHOT_FRAMES + HITSHOT_CELS + 2) and fails this test.
		if((hitshot->age < (HITSHOT_FRAMES + 1)) && (hitshot->age > 0)) {
			// Already advanced by the update, so no alternation here.
			_CH = 0;
			_CL = hitshot->patnum;
			z_super_roll_put_tiny_16x16(
				(hitshot->pos.cur.x.to_pixel() +
					(PLAYFIELD_LEFT - (HITSHOT_W / 2))),
				scroll_subpixel_y_to_vram_seg1(
					hitshot->pos.cur.y.v +
						TO_SP(PLAYFIELD_TOP - (HITSHOT_H / 2))
				),
				_CX
			);
		}
	}
	grcg_off_clobbering_dx();
}

// Next free array element in [hitshots], in the dump's own words
// (th05/main/player/hitshot_from[bss].asm, which publishes it). Declared here
// rather than in th04/main/player/shot.hpp beside [hitshots], for the reason
// th04/main/stage/reset.cpp gives for its own copy of this line: that header is
// read by every other lane's parcels. `unsigned`, and that is measured -- the
// wrap test below compiles to `jnb`, not `jge`.
extern unsigned int hitshot_next_free_id;

// Creates a hit animation at the position of [shot], invalidating [shot] in the
// process. (The comment above the deleted module, kept verbatim.)
//
// The three sprite indices are bare literals, and they OWE A NAMING ROUND
// rather than three coinages: th05/sprites/main_pat.h names PAT_SHOT_SUB = 22
// and PAT_OPTION = 26 but nothing at 20, 28 or 32, and inventing all three
// inside a lift whose whole naming cost is otherwise zero is what
// state/notes/bb_playchar_load.md's census warns against.
extern "C" void pascal near hitshot_from(Shot near *shot)
{
	HitShot near *hitshot = &hitshots[hitshot_next_free_id];
	if(hitshot_next_free_id < (HITSHOT_COUNT - 1)) {
		hitshot_next_free_id++;
	} else {
		hitshot_next_free_id = 0;
	}
	shot->flag = F_REMOVE;

	// `// Wat?` in the dump, and it is a real quirk: a slot whose animation is
	// still running is silently skipped, so the shot is invalidated without
	// any hit animation being played at all.
	if(hitshot->age != 0) {
		return;
	}
	hitshot->age = 1;
	// THE CAST IS THREE BYTES, not a readability choice: [patnum_base] is a
	// signed `char`, so a bare `== 20` promotes it and Turbo C++ emits
	// `mov al, [di+0eh]` / `cbw` / `cmp ax, 20`. Narrowing it to `unsigned
	// char` first gives the original's `cmp byte ptr [di+0eh], 20`. Same
	// device as kb/codegen/0142, which measured it on an ADD rather than a
	// compare; six spellings probed, and this is the only one that reaches
	// 0x6C bytes.
	if(static_cast<unsigned char>(shot->patnum_base) == 20) {
		hitshot->patnum = 28;
	} else {
		hitshot->patnum = 32;
	}
	hitshot->pos.cur = shot->pos.cur;
	hitshot->pos.prev = shot->pos.prev;

	// One `mov bx, 6` for both divisions: `-Z` suppresses the redundant load,
	// which is why this is two statements and not a helper.
	hitshot->pos.velocity.x.v = shot->pos.velocity.x.v / 6;
	hitshot->pos.velocity.y.v = shot->pos.velocity.y.v / 6;
}

#pragma codeseg
