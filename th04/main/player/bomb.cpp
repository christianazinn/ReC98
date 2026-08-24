/// Dropping a bomb
/// ---------------
/// TWO bodies. The condition that guards the bomb — including the deathbomb
/// window — is identical, but everything after it differs: TH04 keeps its bomb
/// count in [resident] and hardcodes both durations, while TH05 keeps it in a
/// plain global, picks both durations per playchar, and lowers [playperf].

// Both wrappers carry the segment/group pragma; see
// state/notes/bb_playchar_load.md.

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
#include "th04/main/null.hpp"
#include "th04/main/bullet/clearzap.hpp"
#include "th04/main/hud/hud.hpp"
#include "th02/snd/snd.h"

// Declared here rather than through th04/main/item/item.hpp, which pulls in
// th04/main/bullet/bullet.hpp a second time; that header has no include guard.
// Same reason as th04/main/execl.cpp and th04/main/stage/loop.cpp.
extern bool items_pull_to_player;

// Set to 72 together with [miss_time] when the player is hit, counted down once
// per frame while nonzero, and while it *is* nonzero the player keeps drifting
// on the momentum they had rather than responding to input. Cleared both at
// stage start and here, so a deathbomb also returns control immediately.
//
// The name follows TH03's `move_lock_time`, its own word for exactly
// this: th03/main/player/stuff.hpp:36 declares `unsigned char move_lock_time;`
// — same width, same role, counted down the same way
// (th03/main/player/bomb.cpp:209-210, :290). TH03 sets it alongside a
// *separate* [knockback_time] (bomb.cpp:450-452), and its knockback is a
// directional shove that also stores a [knockback_angle] (:572-581). This byte
// stores no angle and applies no velocity, so it is TH03's move lock and not
// TH03's knockback. An earlier knockback-timer interpretation named the one
// construct this evidence rules out, so it is not retained.
extern "C" unsigned char miss_move_lock_time;

#if (GAME == 5)
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
#endif

// Cancels any bomb still in progress and unhooks its background renderer.
// stage_init() calls this once per stage, four lines away from the shot-state
// reset, in both games.
//
// ONE body: kb/codegen/0115 over the two originals — TH04's copy at
// `0AAF:54B4` against TH05's at `0AE1:1663`, `0x10` bytes each — gives 6
// differing bytes, at offsets 5, 6, 10, 11, 12, 13. Those are exactly the two
// memory operands and the one `offset` immediate, so EVERY non-operand byte is
// identical and this is one source function in both games. (The dumps used to
// call those two procs sub_FFA4 and sub_C473; both names are written here
// without backticks on purpose, because they are retired placeholders rather
// than symbols this tree defines.)
//
// This body used to be `#if (GAME == 5)`-guarded, purely because TH04's copy
// was still ASM at the identical tail position of th04_main.asm's own
// PLAYER_B_TEXT root contribution: an unguarded body would have grown TH04's
// C++ contribution to that segment while the dump still held its own copy,
// which is a guaranteed RED. That guard was a lifting-order artifact, not a
// divergence, and both dumps now spell the symbol
// `@bomb_reset$qv procdesc near` instead.
//
// Named for TH02's bomb_reset(), which th02/main/player/bomb.hpp declares with
// the same role and spells out the convention: singular like every other
// scalar-state reset in the tree (scroll_reset(), score_reset(),
// player_reset()) rather than plural like the array ones.
void near bomb_reset(void)
{
	bombing = false;
	bg_render_bombing = nullfunc_near;
}

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
		miss_move_lock_time = 0;
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

		// Left as a literal, like bullets_clear()'s own 20
		// (th04/main/bullet/clearzap.hpp:23-25): the ASM side names
		// BOMB_INVINCIBILITY_FRAMES two lines up, at th04/th04.inc:49, but
		// never named this one.
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
