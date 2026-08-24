/// Enemy updating
/// --------------
/// (#included from th04/enemy_u.cpp, which names MUGETSU_TEXT and is linked
/// immediately ahead of th04/bx1_gath.cpp. This function was the entire
/// remaining root contribution to that segment. Keeping it in a separate
/// object also keeps this header set away from the four switch-bearing
/// Mugetsu objects; see kb/codegen/0171.)
///
/// Runs one frame of every occupied slot in [enemies]: the .STD script, the
/// enemy's collision with the player, shot damage, autofire, and the kill
/// animation. It also picks the lowest homing target that is not below the
/// player.

#include "platform.h"
#include "pc98.h"
#include "th04/main/enemy/enemy.hpp"
#include "th04/main/enemy/size.hpp"
#include "th04/main/homing.hpp"
#include "th02/main/player/player.hpp"
#include "th04/main/player/player.hpp"
#include "th04/main/player/shot.hpp"
#include "th04/main/spark.hpp"

// These unguarded headers are not needed for their full definitions here.
extern unsigned long score_delta; // th04/main/score.hpp
extern "C" void pascal snd_se_play(int new_se); // th02/snd/snd.h

// Neither variable has a header in either game.
extern unsigned int enemies_gone;
extern unsigned int enemies_killed;

extern bool player_is_hit;

// The enemy .STD script interpreter. Its C++ definition is linked in the
// MAIN_03 group, so the original ordinary near call remains possible.
bool near enemy_run(void);

/// Box extents
/// -----------

// An enemy with [kills_player_on_collision] hits the player inside this box.
#define ENEMY_COLLIDE_W 24
#define ENEMY_COLLIDE_H 24

// An enemy does not autofire while the player is inside this box.
#define ENEMY_AUTOFIRE_HOLD_W 96
#define ENEMY_AUTOFIRE_HOLD_H 96
/// -----------

// The original subtracts first and tests strict less-than on the unsigned
// result. Neither shared overlap helper has both properties.
#define overlap_1d_lt_fast(p1, p2, extent) ( \
	static_cast<unsigned int>(((p1) - (p2)) + ((extent) / 2)) < (extent) \
)

#define overlap_wh_lt_fast(p1, p2, w, h) ( \
	overlap_1d_lt_fast((p1).x, (p2).x, (w)) && \
	overlap_1d_lt_fast((p1).y, (p2).y, (h)) \
)

extern "C" void pascal enemies_update(void)
{
	// The original allocates one byte at [bp-1] in a two-byte frame and reuses
	// it for the kill animation's patnum.
	unsigned char damage;

	// SI and DI, in that order.
	register enemy_t near *enemy;
	register int i;

	homing_target.x.v = Subpixel::None();
	homing_target.y.v = Subpixel::None();
	shot_hitbox_radius.x.v = TO_SP(16);
	shot_hitbox_radius.y.v = TO_SP(12);

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
			enemy_run();

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
				if(
					(enemy->pos.cur.y > homing_target.y) &&
					(enemy->pos.cur.y <= player_pos.cur.y)
				) {
					homing_target.x = enemy->pos.cur.x;
					homing_target.y = enemy->pos.cur.y;
				}

				shot_hitbox_center = enemy->pos.cur;
				damage = shots_hittest();
				if(damage) {
					if(enemy->hp != -2) {
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
							items_add(
								enemy->pos.cur.x,
								enemy->pos.cur.y,
								enemy->item
							);
							snd_se_play(3);
							score_delta += static_cast<unsigned int>(enemy->score);

							// TASM lowers this same-group far call to
							// `nop; push cs; call near`; Turbo C++ does not.
							_asm {
								db  	0xFF, 0x74, 0x02;
								db  	0xFF, 0x74, 0x04;
								db  	0x66, 0x68, 8, 0x00, (4 * 16), 0x00;
								nop;
								push	cs;
								call	near ptr sparks_add_random;
							}
							enemies_gone++;
							enemies_killed++;
							continue;
						}

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
					enemy_bullet_template_push(*enemy);
					bullet_template.origin.x.v += enemy->pos.cur.x;
					bullet_template.origin.y.v += enemy->pos.cur.y;
					bullet_template_tune();
					bullets_add_regular();
				}
			}
			enemy->age++;
		} else {
			enemy->pos.update_seg3();
			damage = ++enemy->flag;
			damage = ((damage - EF_KILL_ANIM) / 4) + PAT_ENEMY_KILL;
			enemy->patnum_base = damage;
			if(damage >= (PAT_ENEMY_KILL + ENEMY_KILL_CELS)) {
				enemy->flag = EF_KILLED;
			}
		}
	}
}
