/* ReC98
 * -----
 * Generic shot control functions
 */

#pragma option -zCmain_01_TEXT -zPmain_01

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


/// hitshot_from(), in the head half of main_01_TEXT
/// -----------------------------------------------
/// HITSHOT_TEXT is th05_main.asm's own new name for the head of that block --
/// sub_1240B() and shots_render(), both still ASM -- split off by a
/// kb/codegen/0080 carve so that this object can append to it, which is what
/// puts this function back at 0x12647. The two procs BELOW it in the original
/// block, @shots_hittest$qv and sub_12842, are both blocked on the same
/// eight-byte clamp (state/notes/sub_12842.md) and the carve does not touch
/// them: they keep the original segment name, together with the nine C++
/// contributions that follow them.
///
/// `#pragma codeseg` rather than a new translation unit, so this costs no
/// Tupfile.lua line (kb/codegen/0155). The group has to be named: both call
/// sites are NEAR, in the tail half of the same group.
///
/// This is the LAST thing in this file on purpose -- everything above it is
/// already matched, and an `#include` or a pragma ahead of matched code can
/// move it.
#pragma codeseg HITSHOT_TEXT main_01

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
