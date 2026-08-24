/// Updating the player's shots
/// ---------------------------
/// (#included from th04/main_.cpp, at the very front of it, ahead of
/// shots_render(), shots_hittest(), enemies_render() and player_invalidate().
/// This proc was the last thing th04_main.asm contributed to main__TEXT, so
/// the object grows backwards into the hole one final time and every byte
/// above it keeps its address (kb/codegen 0099 + 0112 + 0114). It is also the
/// last one: the root dump's contribution to main__TEXT is now EMPTY.)
///
/// TH05's counterpart, sub_1240B(), is a bigger body with per-shottype homing
/// and missile branches and is still in th05_main.asm, so this file is
/// TH04-only rather than shared.
///
/// One frame of every shot: retires the ones whose decay animation has run
/// out, moves the rest, drops the ones that left the playfield, advances the
/// decay animation of the ones that hit something, and caches the live ones
/// into [shots_alive] for shots_hittest() and nothing else. The option laser's
/// own position is tracked at the end, since the laser is not a Shot.

#include "platform.h"
#include "pc98.h"
#include "th04/main/player/shot.hpp"

void near shots_update(void)
{
	// SI and DI, in declaration order (kb/codegen/0146). [i] is the one thing
	// that does not fit and is the function's only stack local.
	register Shot near *shot;
	register shot_alive_t near *sa;
	int i;

	shots_alive_count = 0;
	shot = shots;
	sa = shots_alive;
	for(i = 0; i < SHOT_COUNT; (i++, shot++)) {
		// A hitshot that ran past the last cel of its decay animation frees
		// its slot here, one frame later than it visually ended.
		if(shot->flag >= SF_REMOVE) {
			shot->flag = SF_FREE;
		}
		if(shot->flag == SF_FREE) {
			continue;
		}

		// The new position is read back out of the registers the call returns
		// it in, exactly like th05/main/stage/stages.cpp's particle loop. The
		// casts are load-bearing: the pseudo-registers are `unsigned`, and all
		// four comparisons below are signed in the original.
		#define cur_x	static_cast<subpixel_t>(_AX)
		#define cur_y	static_cast<subpixel_t>(_DX)

		/* _DX:_AX = */ shot->pos.update_seg1();
		if(!(
			(cur_x > TO_SP(-(SHOT_W / 2))) &&
			(cur_x < TO_SP(PLAYFIELD_W + (SHOT_W / 2))) &&
			(cur_y > TO_SP(-(SHOT_H / 2))) &&
			(cur_y < TO_SP(PLAYFIELD_H + (SHOT_H / 2)))
		)) {
			// Not freed immediately: SF_REMOVE is retired at the top of the
			// next frame, by the branch above.
			shot->flag = SF_REMOVE;
			continue;
		}

		// A hitshot. Its [flag] doubles as the frame counter of the decay
		// animation, so advancing the sprite is a matter of counting cels off
		// it -- which is also why a hitshot is never cached into [shots_alive]
		// and can no longer hit anything.
		if(shot->flag > SF_ALIVE) {
			// `++` rather than `+= 1`: only the increment operator reaches
			// the `INC byte ptr` form the original uses (kb/codegen/0094).
			// The cast is th04/main/bullet/update.cpp's spelling for the same
			// problem -- C++ has no `++` for an enum.
			reinterpret_cast<unsigned char &>(shot->flag)++;
			if((shot->flag & (HITSHOT_FRAMES_PER_CEL - 1)) == SF_HIT) {
				shot->patnum_base++;
			}
			continue;
		}

		sa->pos.x.v = cur_x;
		sa->pos.y.v = cur_y;
		sa->shot = shot;
		sa++;
		shots_alive_count++;
		shot->age++;
	}

	// The option laser is not a Shot and has no slot in [shots], so its
	// position is tracked here rather than in the loop. [shot_laser_time]
	// counts all the way down through SHOT_LASER_COOLDOWN_FRAMES, so this
	// keeps running for the whole cooldown after the laser stopped rendering.
	if(shot_laser_time != 0) {
		shot_laser_bottomcenter.prev = shot_laser_bottomcenter.cur;
		shot_laser_bottomcenter.cur = player_option_pos_cur;
		shot_laser_time--;
	}
}
