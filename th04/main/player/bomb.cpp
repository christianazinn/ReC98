/// Dropping a bomb
/// ---------------
/// TH04 only, for now: TH05's player_bomb() is a separate body — it reads its
/// bomb count out of a global rather than [resident], picks the invincibility
/// and bullet-clear durations per playchar through select_for_playchar(), and
/// ends with playperf_lower() — and its dump was held by another lane when
/// this parcel ran. Add it here under `#if (GAME == 5)`, with the matching
/// mai_TEXT carve in th05_main.asm and a th05/player_b.cpp wrapper.
/// (state/notes/player_bomb_th0405.md)

#pragma option -zCPLAYER_B_TEXT -zPmain_01

#include "platform.h"
#include "pc98.h"
#include "th04/resident.hpp"
#include "th04/main/player/player.hpp"
#include "th04/main/player/bomb.hpp"
#include "th04/main/bg.hpp"
#include "th04/main/bullet/clearzap.hpp"
#include "th04/main/hud/hud.hpp"
#include "th02/snd/snd.h"

// Declared here rather than through th04/main/item/item.hpp, which pulls in
// th04/main/bullet/bullet.hpp a second time; that header has no include guard.
// Same reason as th04/main/execl.cpp and th04/main/stage/loop.cpp.
extern bool items_pull_to_player;

// Still unnamed. Set to 72 together with [miss_time] when the player is hit,
// counted down once per frame while nonzero, and while it *is* nonzero the
// player keeps drifting on the momentum they had rather than responding to
// input. Cleared both at stage start and here, so a deathbomb also returns
// control immediately. Not named because "72 frames of something" is not
// enough evidence to pick between "knockback" and "controls locked".
extern "C" unsigned char byte_259A3;

// Drops a bomb, if possible. Also cancels a death if called during the
// deathbomb window — which is what the [miss_time] branch is: [miss_time]
// counts down from (MISS_ANIM_FRAMES + DEATHBOMB_WINDOW), so a value above
// MISS_ANIM_FRAMES means the miss animation has not started yet and the death
// can still be taken back.
extern "C" void pascal near player_bomb(void)
{
	if(bombing || (resident->rem_bombs == 0) || bombing_disabled) {
		return;
	}
	if(miss_time != 0) {
		if(miss_time <= MISS_ANIM_FRAMES) {
			return;
		}
		miss_time = 0;
		player_is_hit = false;
		byte_259A3 = 0;
	}
	resident->rem_bombs--;

	// Same code group, so this is a `nopcall`. (kb/codegen/0083)
	asm { nop; push cs; call near ptr hud_bombs_put; }

	bombing = true;
	bomb_frame = 0;
	player_invincibility_time = BOMB_INVINCIBILITY_FRAMES;
	bg_render_bombing = bg_render_bombing_func;
	// Left as a literal, like bullets_clear()'s own 20: the dump names
	// BOMB_INVINCIBILITY_FRAMES two lines up but never named this one.
	bullet_clear_time = 192;
	snd_se_play(13);
	items_pull_to_player = true;
	resident->bombs_used++;
}
