/* ReC98
 * -----
 * Shot collision against the current hitbox
 */

#pragma option -zCmain_01_TEXT -zPmain_01

#include "th05/main/player/shot.hpp"
#include "th04/main/spark.hpp"
#include "th04/main/bullet/bullet.hpp"
#include "th04/main/score.hpp"
#include "th04/main/frames.h"
#include "th04/main/player/bomb.hpp"
#include "th04/main/stage/stage.hpp"
#include "th05/playchar.h"

extern "C" void pascal near hitshot_from(Shot near *shot);

// Shared between both hit tests. Every second colliding shot gets a spark.
extern "C" unsigned char shot_hit_spark_parity;

#define shot_damage_accumulate_clamped(sa, hit_count, accumulator) { \
	_AX = (static_cast<unsigned char>((sa)->shot->damage) / (hit_count)); \
	if(_AX == 0) { \
		_AX = 1; \
	} \
	accumulator += _AX; \
}

#define shot_alive_at(reg) reinterpret_cast<shot_alive_t near *>(reg)

// -3 -Z packs each adjacent pair of Pascal arguments into one dword PUSH.
// Spell those bytes explicitly because the call itself is also a nopcall.
#define shot_bomb_damage_for_playchar() { \
	_asm { \
		db 0x66, 0x68, 4, 0, 3, 0; \
		db 0x66, 0x68, 3, 0, 15, 0; \
		nop; push cs; call near ptr select_for_playchar; \
	} \
}

int shots_hittest(void)
{
	unsigned int i;
	int left = (shot_hitbox_center.x.v - shot_hitbox_radius.x.v);
	int top = (shot_hitbox_center.y.v - shot_hitbox_radius.y.v);
	int w = (shot_hitbox_radius.x.v * 2);
	int h = (shot_hitbox_radius.y.v * 2);
	unsigned char hit_count;
	_SI = 0;
	hit_count = 0;
	_DI = reinterpret_cast<unsigned int>(shots_alive);

	for(i = 0; i < shots_alive_count; i++, _DI += sizeof(shot_alive_t)) {
		if(static_cast<unsigned int>(shot_alive_at(_DI)->pos.x.v - left) > w) {
			continue;
		}
		if(static_cast<unsigned int>(shot_alive_at(_DI)->pos.y.v - top) > h) {
			continue;
		}

		hit_count++;
		shot_damage_accumulate_clamped(shot_alive_at(_DI), hit_count, _SI);

		shot_hit_spark_parity++;
		if(shot_hit_spark_parity & 1) {
			sparks_add_random(
				shot_alive_at(_DI)->pos.x, shot_alive_at(_DI)->pos.y,
				TO_SP(8), 1
			);
		}
		hitshot_from(shot_alive_at(_DI)->shot);
		shot_alive_at(_DI)->pos.x.v = Subpixel::None();
	}

	if(bombing) {
		if(shots_hittest_against_boss) {
			_SI /= 4;
		}
		if(stage_frame_mod4 == 0) {
			shot_bomb_damage_for_playchar();
			_SI += _AX;
		}
		if(
			(playchar == PLAYCHAR_MARISA) &&
			(bomb_frame >= 16) &&
			(bomb_frame < 80) &&
			((bomb_frame % 4) == 0) &&
			(static_cast<unsigned int>(player_pos.cur.x.v - left) <= w) &&
			(static_cast<unsigned int>(player_pos.cur.y.v) >=
			 static_cast<unsigned int>(top))
		) {
			_SI += 22;
		}
	}

	if(shots_hittest_against_boss) {
		if((playchar <= PLAYCHAR_REIMU) || (playchar >= PLAYCHAR_YUUKA)) {
			if(stage_id == 6) {
				_SI *= 5;
				_SI /= 4;
			} else if(playchar == PLAYCHAR_REIMU) {
				_SI *= 8;
				_SI /= 7;
			}
		} else {
			_SI *= 4;
			_SI /= 5;
		}
	} else {
		if((stage_id == 6) && (playchar == PLAYCHAR_REIMU)) {
			_SI *= 4;
			_SI /= 5;
		}
		if(playchar == PLAYCHAR_YUUKA) {
			_SI *= 4;
			_SI /= 5;
		}
	}

	score_delta += _SI;
	return _SI;
}

// Alice's barrier uses this version to fire a revenge bullet from every shot
// that lands. Its caller initializes both the hitbox and bullet template.
extern "C" int far shots_hittest_revenge(void)
{
	unsigned int i;
	int left = (shot_hitbox_center.x.v - shot_hitbox_radius.x.v);
	int top = (shot_hitbox_center.y.v - shot_hitbox_radius.y.v);
	int w = (shot_hitbox_radius.x.v * 2);
	int h = (shot_hitbox_radius.y.v * 2);
	unsigned char hit_count;
	shot_alive_t near *sa;
	_DI = 0;
	hit_count = 0;
	sa = shots_alive;

	for(i = 0; i < shots_alive_count; i++, sa++) {
		if(static_cast<unsigned int>(sa->pos.x.v - left) > w) {
			continue;
		}
		if(static_cast<unsigned int>(sa->pos.y.v - top) > h) {
			continue;
		}

		hit_count++;
		shot_damage_accumulate_clamped(sa, hit_count, _DI);

		shot_hit_spark_parity++;
		if(shot_hit_spark_parity & 1) {
			sparks_add_random(sa->pos.x, sa->pos.y, TO_SP(8), 1);
		}
		if((rank != RANK_EASY) || (stage_frame_mod4 == 0)) {
			static_cast<SPPoint &>(bullet_template.origin) = sa->pos;
			bullet_template.angle = randring1_next16();
			bullets_add_regular_far();
		}
		hitshot_from(sa->shot);
		sa->pos.x.v = Subpixel::None();
	}
	score_delta += _DI;
	return _DI;
}

#undef shot_bomb_damage_for_playchar
#undef shot_alive_at
#undef shot_damage_accumulate_clamped
