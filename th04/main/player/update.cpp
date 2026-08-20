/// Player update
/// -------------
/// (#included from th04/main_0.cpp, ahead of th04/main/player/render.cpp,
/// which is the address order the two bodies had in main_0_TEXT. This proc was
/// the tail of th04_main.asm's contribution to that segment, and the object
/// that hosts player_render() is the segment's only other non-empty
/// contribution, so it grows backwards into the hole and every byte above it
/// keeps its address -- no carve, no new segment name, no group-list edit and
/// no Tupfile.lua line (kb/codegen 0099 + 0112 + 0114).
///
/// The only header this file and th04/main/player/render.cpp both reach is
/// th04/main/player/player.hpp, which MATCH-TH05-MAIN-TAILS-1 guarded for the
/// same collision in th05/shot_inv.cpp, so neither has to decline it
/// (kb/codegen/0129).
///
/// The function keeps IDA's spelling. th04/main/stage/loop.cpp already calls
/// it as `sub_10ABF()`, TH05's counterpart in the same slot of the same loop
/// is the equally unnamed sub_1214A(), and the failed name search is recorded
/// in state/notes/th04-main-carve-tails-1.md.
///
/// One frame of the player: the invincibility countdown, the hit that starts a
/// miss, movement (with one retry that masks out the previous frame's
/// direction), the shot cycle, the options trailing one frame behind, the bomb
/// key, and the miss animation.

#include "th02/snd/snd.h"
#include "th04/main/player/shot.hpp"
#include "th04/main/player/move.hpp"
#include "th04/main/player/bomb.hpp"

// Zero-byte alias in th04_main.asm's _BSS, exactly like the one
// th04/main/player/bomb.cpp and th04/main/stage/init.cpp already use for it.
extern "C" unsigned char byte_259A3;
#define miss_move_lock_time byte_259A3

// The movement input this function last managed to apply. If the current
// frame's input is rejected, it is retried once with these directions masked
// out. Still unnamed; see the note.
extern "C" unsigned int word_2598C;

// Fires one round of the current shottype's shot, like TH02's identically
// named pointer. Set from the shottype together with the movement speeds.
extern "C" void (near *playchar_shot_func)(void);

// The miss animation. A C++ definition since MATCH-TH04-MAIN-CARVE-TAILS-2,
// in th04/main/player/miss.cpp, which th04/main_0.cpp includes ahead of this
// file -- so this declaration is now a re-declaration of something already
// defined in the same object, kept because it is also what documents the
// call below.
extern "C" void near sub_10988(void);

extern "C" void near sub_10ABF(void)
{
	// Declaration order is the frame layout: [bp-1], [bp-2].
	// (kb/codegen/0010)
	unsigned char first_attempt;
	move_ret_t move_ret;

	register input_t input;

	if(player_invincibility_time != 0) {
		player_invincibility_time--;
	}
	if(player_is_hit) {
		if(player_invincibility_time != 0) {
			player_is_hit = false;
		} else {
			// Cancels a laser that is still being fired.
			if(shot_laser_time > (SHOT_LASER_COOLDOWN_FRAMES + 1)) {
				shot_laser_time = (SHOT_LASER_COOLDOWN_FRAMES + 1);
			}
			miss_time = (MISS_ANIM_FRAMES + DEATHBOMB_WINDOW);
			player_is_hit = false;
			player_invincibility_time = MISS_INVINCIBILITY_FRAMES;
			miss_move_lock_time = 0x48;
			player_pos.velocity.x.v = 0;
			player_pos.velocity.y.v = 0;
		}
	}
	if(miss_move_lock_time == 0) {
		player_pos.velocity.x.v = 0;
		player_pos.velocity.y.v = 0;
		input = (key_det & INPUT_MOVEMENT);
		first_attempt = 1;
		while(1) {
			move_ret = player_move(input);
			if(move_ret != MOVE_INVALID) {
				break;
			}
			if(first_attempt == 0) {
				break;
			}
			if(word_2598C == input) {
				break;
			}
			input &= ~word_2598C;
			first_attempt = 0;
		}
		if(shiftkey) {
			player_pos.velocity.x.v /= 2;
			player_pos.velocity.y.v /= 2;
		}
		player_pos_update_and_clamp();
		if(first_attempt != 0) {
			word_2598C = input;
		}
		if((key_det & INPUT_SHOT) && (shot_time <= 1)) {
			shot_time = SHOT_CYCLE_FRAMES;
			goto shoot;
		}
		if(shot_time != 0) {
			shot_time--;
			if(shot_time == ((SHOT_CYCLE_FRAMES / 3) * 1)) {
				goto shoot;
			}
			if(shot_time == ((SHOT_CYCLE_FRAMES / 3) * 2)) {
shoot:
				playchar_shot_func();
				snd_se_play(1);
			}
		}
	} else {
		player_pos.update_seg1();
		miss_move_lock_time--;
	}
	player_option_pos_prev = player_option_pos_cur;
	player_option_pos_cur = player_pos.cur;
	player_option_pos_cur.x.v -= player_pos.velocity.x.v;
	player_option_pos_cur.y.v -= player_pos.velocity.y.v;
	if(key_det & INPUT_BOMB) {
		player_bomb_func();
	}
	if(miss_time != 0) {
		sub_10988();
	}
}
