/// Stage 4 Boss - Marisa
/// ---------------------
/// The drift helper, the per-frame update, the background renderer and the
/// stage-end callback.
/// They are the last bodies of the nameless code segment that also holds
/// Marisa's other callback and the Stage 5 boss, so it needs no split and no
/// new segment - this translation unit just contributes to that same segment
/// after th02_main.asm's block.
/// (kb/codegen/0099)

// -G, because the original's prologs are `push bp; mov bp, sp` with no locals
// rather than an `ENTER`. (kb/codegen/0011)
#pragma option -zCmain_03__TEXT -zPmain_03 -G -a2

#include "platform.h"
#include "pc98.h"
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/hardware/pages.hpp"
#include "th02/main/playfld.hpp"
#include "th02/main/frames.hpp"
#include "th02/math/randring.hpp"
#include "th02/main/bg_particle.hpp"
#include "th02/main/boss/boss.hpp"
#include "th02/main/boss/b4.hpp"
#include "th02/main/player/player.hpp"
#include "th02/main/stage/stage.hpp"
#include "th02/main/stage/bonus.hpp"
#include "th02/main/dialog/dialog.hpp"
#include "th02/main/hud/overlay.hpp"

// th02/main/dialog/dialog.cpp. dialog.hpp declares every dialog_script_*
// function but not this one.
void near dialog_pre(void);

// th02/resident.hpp, declared the same way th02/main/bg_particle.cpp does
// rather than by pulling the whole resident structure in for one flag.
extern "C" bool reduce_effects;

// The point shottype B's homing shots aim at, set by whichever boss is on
// screen. Declared here the way th02/main/player/shot.cpp and
// th02/main/player/reset.cpp already declare them; no header owns them.
extern "C" int boss_pos_x;
extern "C" int boss_pos_y;

// th02/main/bullet/bullet.cpp, which owns the `[inferred]` licence for this
// name. 0 for as long as Marisa is fighting; marisa_update() sets it to 1 on
// the frame she is defeated, which switches the tail below to the defeat
// animation.
extern "C" uint8_t boss_phase;

/// Marisa's still-ASM helpers
/// --------------------------
/// All of them are `proc near` in th02_main.asm's main_03__TEXT block, all sit
/// above marisa_update() in the same segment, and marisa_update() is the only
/// caller of every one of them. marisa_1BE72() was the sixteenth and is now
/// defined below. The spellings are the dump's own; they are
/// address-suffixed rather than IDA placeholders, so naming them is a separate
/// decision that this parcel does not make (`marisa_1AA60` is not matched by
/// tools/re/naming_precheck.py's placeholder pattern, which is keyed on IDA's
/// own kind prefixes).

extern "C" void near marisa_1AA60(void);
extern "C" void near marisa_1AB35(void);

// Returns nonzero once the defeat animation has finished.
extern "C" bool16 near marisa_1AC7B(void);

extern "C" void near marisa_1AD80(int orb_i);
extern "C" void near marisa_1AE98(void);
extern "C" void near marisa_1B025(void);
extern "C" void near marisa_1B24A(void);
extern "C" void near marisa_1B2E9(void);
extern "C" void near marisa_1B35F(void);
extern "C" void near marisa_1B3DE(void);
extern "C" void near marisa_1B477(void);
extern "C" void near marisa_1B555(void);
extern "C" void near marisa_1B6DA(void);
extern "C" void near marisa_1B7D3(void);
extern "C" void near marisa_1B996(void);
extern "C" void near marisa_1BAFF(void);
extern "C" void near marisa_1BC43(void);
/// --------------------------


// marisa_1BC43()'s final `retn`, handed to this translation unit so that the
// object's code contribution starts one byte before marisa_1BE72() and
// therefore an ODD number of bytes before marisa_update(). That is the parity
// under which -a2 pads marisa_update()'s generated jump table
// (kb/codegen/0154); marisa_1BE72()'s own `retn` used to buy it, and now that
// this object emits marisa_1BE72() the byte has to come from the proc above.
// kb/codegen/0096's named escape hatch.
#pragma codestring "\xC3"

// Marisa's drift. Run after nine of the eleven patterns below, and by
// MP_UNSTARTED's own branch as well. Picks a direction on frame 2 of the
// pattern and then walks one pixel per axis per frame for the next 48.
extern "C" void near marisa_1BE72(void)
{
	if(boss_phase_frame == 1) {
		return;
	}
	if(boss_phase_frame == 2) {
		marisa_velocity_x = (
			((marisa_topleft.x + 32) < player_topleft.x) ? 1 : -1
		);
		// ZUN quirk: the vertical pick is not the mirror of the horizontal
		// one. Inside the band it is random rather than player-seeking, and
		// (1 - (n % 3)) can come out 0, so Marisa can spend a whole pattern
		// drifting purely sideways.
		marisa_velocity_y = (
			(marisa_topleft.y <  72) ?  1 :
			(marisa_topleft.y > 108) ? -1 :
			(1 - (randring2_next8() % 3))
		);
	}
	if(boss_phase_frame < 50) {
		*boss_left_on_back_page += marisa_velocity_x;
		*boss_top_on_back_page += marisa_velocity_y;
		marisa_topleft.x = *boss_left_on_back_page;
		marisa_topleft.y = *boss_top_on_back_page;
	}
}


/// Why this function needs an ODD number of bytes ahead of it
/// ---------------------------------------------------------
/// The original has a single padding byte between this function's epilogue and
/// its generated jump table. Reproducing it is kb/codegen/0070's shape, and the
/// three measurements below are what decided the route. All of them were taken
/// from tcc -S listings, so none of them cost a build cycle.
///
/// `[measured]` With this translation unit's code contribution starting at an
/// EVEN offset ahead of this function - which is what a plain lift gives, since
/// it would be the first thing the object emits - Turbo C++ emits no padding at
/// all, and the listings for -a2, -a and no alignment option are byte-identical
/// apart from their debug timestamp record. The natural table offset there is
/// odd (0x26D).
///
/// `[measured]` The prefix is the whole of what this object emits ahead of
/// marisa_update(), not just the codestring: marisa_1BE72() is 0x80 bytes, so
/// the codestring above is what keeps the total odd. A lift of marisa_1BE72()
/// with no codestring would be byte-identical in both function bodies and
/// still lose the pad, which is kb/codegen/0119's failure mode exactly - diff
/// the whole segment, never the two functions.
///
/// `[measured]` With an ODD prefix and -a2, the same compiler emits the pad.
/// Probed across prefixes of 0, 1 and 2 bytes: only the 1-byte prefix produces
/// it. So the alignment is object-relative as kb/codegen/0096 says, but the
/// parity is the other way round from that entry's wording - what Turbo C++
/// aligns is the byte AFTER the table, not the table itself. kb/codegen/0139
/// already recorded one case of this from the other direction.
///
/// `[measured]` The alternative routes are both closed, and closed the way
/// kb/codegen/0070 recorded for TH03: a post-function codestring lands after
/// the table entries, and one placed before or inside the function body lands
/// at the top of the object's code instead.

// Marisa's [boss_update] callback, installed by stage_init(). Returns
// SP_CLEAR once her defeat animation has run out, SP_BOSS on every other
// frame.
//
// `int` rather than [stage_progression_t]: that enum is byte-sized -
// th02_main.asm spells its one variable `_stage_progression db ?` - but the
// original returns its value in the whole of AX. The callback slot in
// th02/main/stage/callback.hpp models the return as the enum anyway, which is
// harmless because stage_loop()'s `stage_progression = boss_update();` only
// ever stores the low byte.

extern "C" int far marisa_update(void)
{
	register int i;
	register screen_x_t particle_left;

	i = 0;
	marisa_orb_flag_sum = 0;
	while(i < MARISA_ORB_COUNT) {
		marisa_orb_flag_sum += marisa_orb_flag[i];
		i++;
	}
	if(marisa_orb_flag_sum == (MARISA_ORB_COUNT * MOF_REMOVED)) {
		boss_pos_x = (marisa_topleft.x + MARISA_CENTER_OFFSET);
		boss_pos_y = (marisa_topleft.y + MARISA_CENTER_OFFSET);
	} else {
		boss_pos_x = -1;
		boss_pos_y = -1;
	}
	boss_phase_frame++;

	// ZUN quirk: the [reduce_effects] arm can never take its branch. It is
	// only reached on an odd [stage_frame], and an odd number can never have
	// its low two bits clear, so the (& 3) test always falls through to the
	// spawn. The effect is the same one particle every 2 frames either way.
	if(((stage_frame & 1) != 0) && (!reduce_effects || ((stage_frame & 3) != 0))) {
		particle_left = ((randring2_next16() % PLAYFIELD_W) + PLAYFIELD_LEFT);
		bg_particles_add(
			particle_left,
			PLAYFIELD_TOP,
			((((PLAYFIELD_LEFT + (PLAYFIELD_W / 2)) - particle_left) / 3) + 0x40)
		);
	}
	if((stage_frame & (MARISA_BG_PARTICLE_COL_FRAMES - 1)) == 0) {
		marisa_bg_particle_col_i++;
		if(marisa_bg_particle_col_i >= MARISA_BG_PARTICLE_COLS_COUNT) {
			marisa_bg_particle_col_i = 0;
		}
		bg_particle_col = MARISA_BG_PARTICLE_COLS[marisa_bg_particle_col_i];
	}
	bg_particles_update_and_render();

	if(boss_phase == 0) {
		if(marisa_intro_step == 0) {
			marisa_1B24A();
			if(boss_phase_frame == 0) {
				marisa_intro_step++;
			}
		} else if(marisa_intro_step == 1) {
			marisa_1B477();
			if(boss_phase_frame == 0) {
				marisa_intro_step++;
				marisa_pattern = 1;
				marisa_patterns_seen = 0;
			}
		} else if(marisa_intro_step == 2) {
			switch(marisa_pattern) {
			case -3:
				marisa_1B35F();
				marisa_1BE72();
				break;
			case -2:
				marisa_1B2E9();
				marisa_1BE72();
				break;
			case -1:
				marisa_1B24A();
				marisa_1BE72();
				break;
			case 0:
				marisa_1B477();
				break;
			case 1:
				marisa_1B555();
				break;
			case 2:
				marisa_1B6DA();
				marisa_1BE72();
				break;
			case 3:
				marisa_1BAFF();
				marisa_1BE72();
				break;
			case 4:
				marisa_1BC43();
				marisa_1BE72();
				break;
			case 5:
				marisa_1B7D3();
				marisa_1BE72();
				break;
			case 6:
				marisa_1B996();
				marisa_1BE72();
				break;
			case MP_UNSTARTED:
				marisa_1BE72();
				if(boss_phase_frame > MARISA_PATTERN_GAP_FRAMES) {
					if(marisa_orb_flag_sum < (MARISA_ORB_COUNT * MOF_REMOVED)) {
						if(marisa_patterns_seen >= MARISA_PATTERNS_PER_ROUND) {
							marisa_pattern = 6;
						} else {
							marisa_pattern = ((randring2_next8() % 5) + 1);
							marisa_patterns_seen++;
						}
					} else if(marisa_orbless_patterns_seen >= 2) {
						marisa_patterns_seen = 0;
						marisa_pattern = 0;
						marisa_orbless_patterns_seen = 0;
						marisa_rounds_done++;
						if(marisa_rounds_done >= 2) {
							marisa_damage_multiplier = 1;
						}
						if(marisa_rounds_done >= MARISA_ROUNDS) {
							boss_phase = 1;
						}
					} else {
						marisa_orbless_patterns_seen++;
						marisa_pattern = (255 - (randring2_next8() % 3));
					}
					boss_phase_frame = 1; // Skip the initial movement
				}
				break;
			}
			if(boss_phase_frame == 0) {
				marisa_pattern = MP_UNSTARTED;
			}
		}
	}

	marisa_1AA60();
	marisa_1AB35();
	if(boss_phase != 0) {
		if(marisa_1AC7B()) {
			return SP_CLEAR;
		}
	} else {
		marisa_1AE98();
	}
	for(i = 0; i < MARISA_ORB_COUNT; i++) {
		if(marisa_orb_flag[i] == MOF_KILL_ANIM) {
			marisa_1AD80(i);
		}
	}
	marisa_1B3DE();
	marisa_1B025();
	return SP_BOSS;
}


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
