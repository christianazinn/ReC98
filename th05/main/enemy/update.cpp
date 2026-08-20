/// Enemy updating
/// --------------
/// (#included from th05/enemy_u.cpp, which names main_032_TEXT and is linked
/// ahead of th05/gather.cpp. This function was the entire remaining root
/// contribution to th05_main.asm's main_032_TEXT block, so that object's
/// first byte lands exactly where the dump's contribution began and every
/// byte below keeps its address (kb/codegen 0112 + 0114). The dump's block is
/// declaration-only now, and its map record is a zero-length one at the
/// segment start -- the same shape SHOT_INV_TEXT took in
/// MATCH-TH05-MAIN-TAILS-3. th05/enemy_u.cpp records why this is a new object
/// rather than an #include at the front of th05/gather.cpp.)
///
/// TH04's copy is still ASM, as enemies_update in th04_main.asm's
/// main_033_TEXT. It is this loop without the homing target selection, the
/// on-screen enemy count and the [dream] tick, so it is deliberately NOT
/// shared from here: the two differ structurally rather than by a few `#if`s,
/// and nothing has graded TH04's half.
///
/// Runs one frame of every occupied slot in [enemies]: the .STD script, the
/// enemy's own kill box against the player, the shot hittest and the damage
/// it deals, the autofire timer, and -- for an enemy that is already dying --
/// the kill animation. Along the way it picks the homing target for this
/// frame (the lowest damageable enemy on screen) and counts how many
/// damageable enemies are on screen, which decides how fast [dream] fills.

#include "platform.h"
#include "pc98.h"
// TH05's enemy_t has to be declared before th04/main/enemy/enemy.hpp's
// `extern enemies[]`, so in that game that header may only ever be reached
// through this one. Same conditional every other shared enemy TU carries
// (th04/main/enemy/add.cpp, inv.cpp, pos.cpp, render.cpp).
#include "th05/main/enemy/enemy.hpp"
#include "th04/main/enemy/size.hpp"
#include "th04/main/homing.hpp"
#include "th02/main/player/player.hpp"
#include "th04/main/player/player.hpp"
#include "th04/main/player/shot.hpp"
#include "th04/main/spark.hpp"
#include "th04/main/frames.h"

// Respelled rather than #included, so that this file stays #includable from a
// crowded object as well as from its own. Each of the headers named below is
// unguarded, and th05/gather.cpp -- the object this body was very nearly a
// part of -- reaches every one of them through th05/main/stage/bonus.cpp or
// th04/main/boss/explode_small.cpp. Re-DECLARING is harmless where
// re-EXPANDING would redefine structs and `static const`s.
// (kb/codegen/0129)
extern unsigned long score_delta; // th04/main/score.hpp
extern unsigned char stage_id; // th04/main/stage/stage.hpp
extern "C" void pascal hud_dream_put(void); // th04/main/hud/hud.hpp
extern "C" void pascal snd_se_play(int new_se); // th02/snd/snd.h

// Same value as BAR_MAX in th04/main/hud/hud.hpp, respelled for the same
// reason as the four declarations above.
static const unsigned char DREAM_MAX = 128;

// Neither of these two has a header in either game; every other file that
// needs them respells them exactly like this (th04/main/enemy/pos.cpp,
// th04/main/execl.cpp).
extern unsigned int enemies_gone;
extern unsigned int enemies_killed;

// The enemy .STD script interpreter, still ASM in th05_main.asm's
// main_031_TEXT and reached with an ordinary near call from here because both
// segments are in the MAIN_03 group. The dump gave it no `public` of its own,
// so this lift added the zero-byte `label near` alias kb/codegen/0123
// prescribes; the dump's own call sites keep the bare spelling. Nothing in
// either dump, any header, ReC98's history or upstream/master ever named it,
// so the placeholder stays (kb/conventions/naming-precedents.md section 3).
extern "C" void near sub_1535A(void);

/// Box extents
/// -----------
/// [inferred] Both boxes are square, both are centered on the enemy, and
/// neither is declared anywhere in either game -- the dump carries the four
/// bounds as bare `shl 4` literals.

// An enemy with [kills_player_on_collision] hits the player inside this box.
#define ENEMY_COLLIDE_W 24
#define ENEMY_COLLIDE_H 24

// An enemy does NOT autofire while the player is inside this box. Getting
// close to a shooting enemy therefore silences it.
#define ENEMY_AUTOFIRE_HOLD_W 96
#define ENEMY_AUTOFIRE_HOLD_H 96
/// -----------

// The unsigned-wraparound distance test this function runs twice on two axes
// each, with the association and the boundary the original used. Neither macro
// in th01/math/overlap.hpp fits, and both are exactly one token away.
// overlap_1d_fast() has the strict less-than, but spells its operands in the
// order (p1 + (extent / 2) - p2), which adds before it subtracts;
// overlap_points_wh_fast() subtracts first and then tests
// less-than-or-equal. The original subtracts first AND tests strict
// less-than, and both of those choices are visible in the bytes: the operand
// order as a SUB ahead of an ADD, and the boundary as a JNB rather than a JA.
#define overlap_1d_lt_fast(p1, p2, extent) ( \
	static_cast<unsigned int>(((p1) - (p2)) + ((extent) / 2)) < (extent) \
)

#define overlap_wh_lt_fast(p1, p2, w, h) ( \
	overlap_1d_lt_fast((p1).x, (p2).x, (w)) && \
	overlap_1d_lt_fast((p1).y, (p2).y, (h)) \
)

extern "C" void pascal enemies_update(void)
{
	// Declaration order is the frame layout, and `enter 2, 0` is the proof
	// that these two share the whole frame: [bp-1], then [bp-2].
	// (kb/codegen/0010)
	//
	// ZUN reuses [damage] for the kill animation's patnum further down. That
	// is not a transcription artifact -- one byte of frame for two unrelated
	// values is what the original allocates, and splitting it into two
	// block-scoped variables moves [enemies_onscreen] to [bp-1].
	unsigned char damage;

	// Damageable enemies currently on screen. Reused, once the loop is done,
	// as the [dream] tick interval in frames.
	unsigned char enemies_onscreen;

	// The two register variables, SI and DI, in that order: [enemy] is
	// mentioned far more often than [i]. (kb/codegen/0146)
	register enemy_t near *enemy;
	register int i;

	homing_target.x.v = Subpixel::None();
	homing_target.y.v = Subpixel::None();
	shot_hitbox_radius.x.v = TO_SP(16);
	shot_hitbox_radius.y.v = TO_SP(12);
	enemies_onscreen = 0;

	enemy = enemies;
	for(i = 0; i < ENEMY_COUNT; (i++, enemy++)) {
		if(enemy->flag == EF_FREE) {
			continue;
		}
		if(enemy->flag == EF_KILLED) {
			enemy->flag = EF_FREE;
			continue;
		}
		enemy_cur = enemy;

		if(enemy->flag < EF_KILL_ANIM) {
			sub_1535A();

			if(
				(enemy->kills_player_on_collision) &&
				(overlap_wh_lt_fast(
					enemy->pos.cur, player_pos.cur,
					TO_SP(ENEMY_COLLIDE_W), TO_SP(ENEMY_COLLIDE_H)
				))
			) {
				player_is_hit = true;
				goto kill;
			}

			// An enemy is only shootable, and only eligible as the homing
			// target, while it is inside the playfield. Note the asymmetry
			// in the two bounds: the X one works out to exactly
			// (PLAYFIELD_W + ENEMY_W), but PLAYFIELD_BOTTOM is 16 pixels
			// short of (PLAYFIELD_H + ENEMY_H) and is a screen coordinate
			// rather than a playfield-relative one. The dump spells both with
			// these two constants, and the values are what decide the bytes.
			if(
				(enemy->can_be_damaged) &&
				(enemy->hp != -1) &&
				(static_cast<unsigned int>(
					enemy->pos.cur.x.v + TO_SP(ENEMY_W / 2)
				) < TO_SP(PLAYFIELD_RIGHT)) &&
				(static_cast<unsigned int>(
					enemy->pos.cur.y.v + TO_SP(ENEMY_H / 2)
				) < TO_SP(PLAYFIELD_BOTTOM))
			) {
				enemies_onscreen++;

				// Lowest one on screen wins.
				if(enemy->pos.cur.y > homing_target.y) {
					homing_target.x = enemy->pos.cur.x;
					homing_target.y = enemy->pos.cur.y;
				}

				// Not shots_hittest()'s inline overload: the radius is set
				// once before the loop rather than per enemy, so only the
				// center is written here. In TH05 that is a single 32-bit
				// move.
				shot_hitbox_center = enemy->pos.cur;
				damage = shots_hittest();
				if(damage) {
					// Both of the inversions below are visible in the bytes,
					// and neither is the shape an ordinary
					// `if / else if / else` chain emits.
					//
					// An enemy with exactly -2 HP is invulnerable and only
					// plays a sound, and ZUN spells that as the ELSE of the
					// ordinary damage path rather than as the first arm: the
					// dump jumps AWAY (`jz`) to a block sunk below everything
					// else in this `if`, where the first-arm spelling would
					// have run it in a straight line.
					if(enemy->hp != -2) {
						// ... and the surviving case is the `if`, with the
						// kill as its `else`, for the same reason: `jge` to a
						// sunk block.
						if(damage < enemy->hp) {
							enemy->hp -= damage;
						} else {
kill:
							enemy->flag = EF_KILL_ANIM;
							enemy->anim_cels = 1;
							enemy->can_be_damaged = false;
							enemy->kills_player_on_collision = false;
							enemy->pos.velocity.x.v = 0;
							enemy->pos.velocity.y.v = 0;
							if(enemy->item != IT_NONE) {
								items_add(
									enemy->pos.cur.x,
									enemy->pos.cur.y,
									enemy->item
								);
							}
							snd_se_play(3);
							score_delta += static_cast<unsigned int>(enemy->score);

							// sparks_add_random() is `pascal`, and therefore far
							// under -ml, but it sits in SPARK_A_TEXT which is in
							// this function's own MAIN_03 group. TASM turns a far
							// call to a same-group proc into `push cs` + a near
							// call; Turbo C++ does not, and emits a 9A far call
							// instead. So the whole call, arguments included, is
							// spelled out. (kb/codegen 0014 + 0083, and the same
							// island th04/main/bullet/update.cpp already carries
							// -- only the packed dword differs, radius 4.0f and
							// count 7 here against 2.0f and 2 there.)
							//
							// [si+2] and [si+4] are enemy->pos.cur.x and .y:
							// enemy_t's prefix is flag, age, pos, exactly like
							// bullet_t's, so that island's operands carry over
							// unchanged.
							_asm {
								db  	0xFF, 0x74, 0x02;
								db  	0xFF, 0x74, 0x04;
								db  	0x66, 0x68, 7, 0x00, (4 * 16), 0x00;
								nop;
								push	cs;
								call	near ptr sparks_add_random;
							}
							enemies_gone++;
							enemies_killed++;
							continue;
						}

						// Outside the inner `if`, not inside its surviving
						// arm: the kill above `continue`s, so this is only
						// ever reached from the branch that subtracted. The
						// dump puts it BELOW the whole kill block and jumps
						// down into it, which no in-arm spelling produces.
						enemy->damaged_this_frame = true;
					} else {
						snd_se_play(10);
					}
				}
			}

			if(enemy->autofire) {
				enemy->autofire_cur_frame++;
				if(
					(enemy->autofire_cur_frame >=
						enemy->autofire_interval) &&
					(enemy->pos.cur.y < TO_SP(304)) &&
					(!overlap_wh_lt_fast(
						enemy->pos.cur, player_pos.cur,
						TO_SP(ENEMY_AUTOFIRE_HOLD_W),
						TO_SP(ENEMY_AUTOFIRE_HOLD_H)
					))
				) {
					enemy->autofire_cur_frame = 0;
					enemy_bullet_template_push(enemy->bullet_template);
					// `.v` on both sides, deliberately: SubpixelBase declares
					// no `operator +=(const SelfType &)`, only one taking a
					// float, so the natural-looking `+=` would resolve to
					// that one and silently multiply the operand by 16.
					bullet_template.origin.x.v += enemy->pos.cur.x;
					bullet_template.origin.y.v += enemy->pos.cur.y;
					bullet_template_tune();
					bullets_add_regular();
				}
			}
			enemy->age++;
		} else {
			enemy->pos.update_seg3();

			// [damage] again, as the kill animation's patnum. [flag] doubles
			// as that animation's frame counter, counting up from
			// EF_KILL_ANIM, and 4 is its frames per cel.
			damage = ++enemy->flag;
			damage = ((damage - EF_KILL_ANIM) / 4) + PAT_ENEMY_KILL;
			enemy->patnum_base = damage;
			if(damage >= (PAT_ENEMY_KILL + ENEMY_KILL_CELS)) {
				enemy->flag = EF_KILLED;
			}
		}
	}

	if(homing_target.x.v != Subpixel::None()) {
		// The more damageable enemies are on screen, the faster [dream]
		// fills; a later stage fills it faster still.
		if(enemies_onscreen >= 8) {
			enemies_onscreen = 80;
		} else {
			enemies_onscreen = (144 - (enemies_onscreen * 8));
		}
		enemies_onscreen -= (stage_id * 16);
		if((enemies_onscreen > 144) || (enemies_onscreen < 4)) {
			enemies_onscreen = 4;
		}
		if(
			((stage_frame % enemies_onscreen) == 0) &&
			(dream < DREAM_MAX)
		) {
			dream++;
			hud_dream_put();
		}
	}
}
