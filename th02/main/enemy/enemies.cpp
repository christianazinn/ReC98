/// TH02's regular stage enemies, at run time
/// -----------------------------------------
/// th02/main/enemy/enemy.cpp next door is the `STAGE?.DT1` LOADER, and ZUN
/// compiled it into its own object and its own code segment. These are the
/// enemies' per-frame procs, and they are in BOSS_5_TEXT instead - the same
/// segment Mima's fight is in, below her code. So this file is not its own
/// object: th02/boss_5.cpp includes it AHEAD of th02/main/boss/b5m.cpp,
/// because BOSS_5_TEXT's only C++ host is that wrapper's object and this
/// block is at the lowest addresses in it. (kb/codegen/0099)
///
/// Everything here is prepended in dump order, so a later lift out of the
/// same block goes at the TOP of this file, not the bottom.

#include "platform.h"
#include "pc98.h"
#include "th02/main/entity.hpp"
#include "th02/main/enemy/enemy.hpp"

// How far enemies_update_and_render() walks into [enemies]. NOT a count:
// enemies_invalidate() recomputes it as (highest non-F_FREE slot) + 1, so free
// slots below it are still iterated. Declared here rather than in
// th02/main/enemy/enemy.hpp, which belongs to the loader's parcel.
extern "C" uint8_t enemies_loop_bound;


// Flags every live enemy for removal, and stops the update loop from walking
// into any of them again. `far`, and th02/main/boss/b3.cpp already declares it
// that way for the Stage 3 boss.
extern "C" void far enemies_remove_all(void)
{
	int i;

	for(i = 0; i < ENEMY_COUNT; i++) {
		if(enemies[i].flag != F_FREE) {
			enemies[i].flag = F_REMOVE;
		}
	}
	enemies_loop_bound = 0;
}
