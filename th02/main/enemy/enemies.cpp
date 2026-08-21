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
#include "th02/hardware/pages.hpp"

// How far enemies_update_and_render() walks into [enemies]. NOT a count:
// enemies_invalidate() recomputes it as (highest non-F_FREE slot) + 1, so free
// slots below it are still iterated. Declared here rather than in
// th02/main/enemy/enemy.hpp, which belongs to the loader's parcel.
extern "C" uint8_t enemies_loop_bound;

// Declared here rather than by including th02/main/tile/tile.hpp. That header
// IS already in this translation unit -- th02/main/boss/b5m.cpp includes it,
// further down th02/boss_5.cpp -- but it is included there BELOW planar.h and
// eleven other headers, and hoisting a guarded header to the top of a TU
// reorders every macro and type the includes under it then see. A repeated
// declaration is the route th04/main/enemy/inv.cpp already takes for the same
// function's TH04 counterpart. (kb/codegen/0138)
void pascal tiles_invalidate_rect(
	screen_x_t left, vram_y_t top, pixel_t w, pixel_t h
);

// The fixed box the pass below unblits, regardless of the per-type [w] and [h]
// that enemy_stagedata_load() derives into each [enemy_template_t]. Same
// spelling and same values as TH04's and TH05's, which
// th04/main/enemy/size.hpp gives their own enemies_invalidate().
static const pixel_t ENEMY_W = 32;
static const pixel_t ENEMY_H = 32;


// Unblits every enemy that still occupies a slot, retires the ones
// enemies_remove_all() below flagged on an earlier frame, and recomputes
// [enemies_loop_bound] from the slots that survive. Installed into
// [enemies_invalidate_func] by stage_init(), and called once per frame from
// the unblitting half of stage_loop().
//
// The position copy at the end is what TH02 has instead of TH04's and TH05's
// temporal `prev`: this game unblits an enemy from where it was drawn on the
// page it is about to rebuild, so the position it was last blitted at has to
// be carried from the other page's slot before the update pass overwrites it.
extern "C" void far enemies_invalidate(void)
{
	int i;
	screen_x_t left;
	vram_y_t top;

	enemies_loop_bound = 0;
	for(i = 0; i < ENEMY_COUNT; i++) {
		if(enemies[i].flag == F_FREE) {
			continue;
		}
		left = enemies[i].pos_on_page[page_back].x;
		top = enemies[i].pos_on_page[page_back].y;
		tiles_invalidate_rect(left, top, ENEMY_W, ENEMY_H);
		if(enemies[i].flag == F_REMOVE) {
			enemies[i].flag = F_FREE;
			continue;
		}
		enemies_loop_bound = (i + 1);
		enemies[i].pos_on_page[page_back].x =
			enemies[i].pos_on_page[page_front].x;
		enemies[i].pos_on_page[page_back].y =
			enemies[i].pos_on_page[page_front].y;
	}
}



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
