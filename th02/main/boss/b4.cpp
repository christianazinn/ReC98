/// Stage 4 Boss - Marisa
/// ---------------------
/// The background renderer and the stage-end callback. They are the last two
/// bodies of the nameless code segment that also holds Marisa's other two
/// callbacks and the Stage 5 boss, so it needs no split and no new segment -
/// this translation unit just contributes to that same segment after
/// th02_main.asm's block.
/// (kb/codegen/0099)

// -G, because the original's prologs are `push bp; mov bp, sp` with no locals
// rather than an `ENTER`. (kb/codegen/0011)
#pragma option -zCmain_03__TEXT -zPmain_03 -G

#include "platform.h"
#include "pc98.h"
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/hardware/pages.hpp"
#include "th02/main/playfld.hpp"
#include "th02/main/bg_particle.hpp"
#include "th02/main/boss/boss.hpp"
#include "th02/main/boss/b4.hpp"
#include "th02/main/stage/stage.hpp"
#include "th02/main/stage/bonus.hpp"
#include "th02/main/dialog/dialog.hpp"
#include "th02/main/hud/overlay.hpp"

// th02/main/dialog/dialog.cpp. dialog.hpp declares every dialog_script_*
// function but not this one.
void near dialog_pre(void);

// Clears Marisa's background - the entire playfield - on the back page, and
// re-points the boss and orb position caches at that page's slots. Since the
// clear removes everything that would otherwise have to be unblitted, the
// positions the page recorded for the previous frame are dropped in the same
// pass, by carrying the front page's over. Installed into
// [boss_bg_render_func] by stage_init().
extern "C" void far marisa_bg_render(void)
{
	register int i;

	boss_left_on_back_page = &boss_left_on_page[page_back];
	boss_top_on_back_page = &boss_top_on_page[page_back];

	egc_off();
	grcg_setcolor(GC_RMW, 0);
	grcg_byteboxfill_x(
		PLAYFIELD_VRAM_LEFT,
		PLAYFIELD_TOP,
		(PLAYFIELD_VRAM_RIGHT - 1),
		(PLAYFIELD_BOTTOM - 1)
	);
	bg_particles_invalidate();

	*boss_left_on_back_page = boss_left_on_page[page_front];
	*boss_top_on_back_page = boss_top_on_page[page_front];

	for(i = 0; i < MARISA_ORB_COUNT; i++) {
		marisa_orb_left_on_back_page[i] = (
			&marisa_orb_left_on_page[page_back][i]
		);
		marisa_orb_top_on_back_page[i] = (
			&marisa_orb_top_on_page[page_back][i]
		);
		if(marisa_orb_flag[i] < MOF_REMOVED) {
			*marisa_orb_left_on_back_page[i] = (
				marisa_orb_left_on_page[page_front][i]
			);
			*marisa_orb_top_on_back_page[i] = (
				marisa_orb_top_on_page[page_front][i]
			);
		}
	}
	grcg_off();
}

// Runs Marisa's post-battle dialog and the stage clear bonus, then advances to
// the Extra-Stage-eligible Stage 5. Installed into [boss_end] by stage_init().
extern "C" void far marisa_end(void)
{
	dialog_pre();
	dialog_script_stage4_post_animate();
	stage_clear_bonus_animate();
	overlay_stage_leave_animate();
	stage_id++;
}
