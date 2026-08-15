/// Dropping a bomb
/// ---------------
/// TWO bodies. The condition that guards the bomb — including the deathbomb
/// window — is identical, but everything after it differs: TH04 keeps its bomb
/// count in [resident] and hardcodes both durations, while TH05 keeps it in a
/// plain global, picks both durations per playchar, and lowers [playperf].

#pragma option -zCPLAYER_B_TEXT -zPmain_01

#include "platform.h"
#include "pc98.h"
#if (GAME == 5)
	#include "th05/resident.hpp"
	#include "th05/playchar.h"
	#include "th04/main/playperf.hpp"
	#include "decomp.hpp"
#else
	#include "th04/resident.hpp"
#endif
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

// Still unnamed, in both games. Set to 72 together with [miss_time] when the
// player is hit, counted down once per frame while nonzero, and while it *is*
// nonzero the player keeps drifting on the momentum they had rather than
// responding to input. Cleared both at stage start and here, so a deathbomb
// also returns control immediately. Not named because "72 frames of something"
// is not enough evidence to pick between "knockback" and "controls locked".
#if (GAME == 5)
	extern "C" unsigned char byte_2CEBD;
	#define miss_knockback byte_2CEBD

	// TH05's bomb count is a plain global rather than [resident]'s field.
	extern unsigned char bombs;

	// Per-playchar bomb durations. These have to be `#define`s rather than
	// the house-style `static const`, because __emit__() only takes literals
	// (kb/codegen/0089).
	#define BOMB_INVINCIBILITY_REIMU  192
	#define BOMB_INVINCIBILITY_MARISA 160
	#define BOMB_INVINCIBILITY_MIMA   144
	#define BOMB_INVINCIBILITY_YUUKA  224

	#define BOMB_CLEAR_REIMU  192
	#define BOMB_CLEAR_MARISA 128
	#define BOMB_CLEAR_MIMA    96
	#define BOMB_CLEAR_YUUKA  192

	// `-3 -Z` folds each adjacent pair of 16-bit `pascal` arguments into one
	// 32-bit PUSH. They have to be spelled out here because the call itself
	// has to be hand-spelled as a `nopcall` (kb/codegen/0083), and hand-
	// spelling the call means hand-pushing its arguments too.
	#define push_playchar_pair(for_reimu, for_marisa, for_mima, for_yuuka) { \
		__emit__(0x66, 0x68, for_marisa, 0, for_reimu, 0); \
		__emit__(0x66, 0x68, for_yuuka, 0, for_mima, 0); \
		asm { nop; push cs; call near ptr select_for_playchar; } \
	}
#else
	extern "C" unsigned char byte_259A3;
	#define miss_knockback byte_259A3
#endif

// Drops a bomb, if possible. Also cancels a death if called during the
// deathbomb window — which is what the [miss_time] branch is: [miss_time]
// counts down from (MISS_ANIM_FRAMES + DEATHBOMB_WINDOW), so a value above
// MISS_ANIM_FRAMES means the miss animation has not started yet and the death
// can still be taken back.
extern "C" void pascal near player_bomb(void)
{
	if(
		bombing ||
		#if (GAME == 5)
			(bombs == 0) ||
		#else
			(resident->rem_bombs == 0) ||
		#endif
		bombing_disabled
	) {
		return;
	}
	if(miss_time != 0) {
		if(miss_time <= MISS_ANIM_FRAMES) {
			return;
		}
		miss_time = 0;
		player_is_hit = false;
		miss_knockback = 0;
	}
	#if (GAME == 5)
		bombs--;
	#else
		resident->rem_bombs--;
	#endif

	// Same code group, so this is a `nopcall`. (kb/codegen/0083)
	asm { nop; push cs; call near ptr hud_bombs_put; }

	bombing = true;
	bomb_frame = 0;
	#if (GAME == 5)
		push_playchar_pair(
			BOMB_INVINCIBILITY_REIMU, BOMB_INVINCIBILITY_MARISA,
			BOMB_INVINCIBILITY_MIMA, BOMB_INVINCIBILITY_YUUKA
		);
		player_invincibility_time = _AL;

		push_playchar_pair(
			BOMB_CLEAR_REIMU, BOMB_CLEAR_MARISA,
			BOMB_CLEAR_MIMA, BOMB_CLEAR_YUUKA
		);
		bullet_clear_time = _AL;

		bg_render_bombing = bg_render_bombing_func;
	#else
		player_invincibility_time = BOMB_INVINCIBILITY_FRAMES;
		bg_render_bombing = bg_render_bombing_func;

		// Left as a literal, like bullets_clear()'s own 20: the dump names
		// BOMB_INVINCIBILITY_FRAMES two lines up but never named this one.
		bullet_clear_time = 192;
	#endif
	snd_se_play(13);
	items_pull_to_player = true;
	resident->bombs_used++;
	#if (GAME == 5)
		__emit__(0x6A, 1);
		asm { nop; push cs; call near ptr playperf_lower; }
	#endif
}
