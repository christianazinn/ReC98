/// TH02's regular stage enemies, per-frame update and render
/// --------------------------------------------------------
/// The BOSS_5_TEXT block below Mima's fight opens with this function, and it
/// is th02_main.asm's own carve-free tail there. th02/main/enemy/enemies.cpp
/// next door holds the two procs at HIGHER addresses, and they are in
/// th02/boss_5.cpp's object rather than this one.
///
/// THIS FILE IS ITS OWN OBJECT, NOT AN #include INTO th02/boss_5.cpp, and the
/// next lift out of this block must keep it that way. The body is 0x30B bytes
/// - ODD - and th02/boss_5.cpp sets -a2 for the one-byte alignment pad under
/// mima_update()'s generated jump table. Turbo C++ word-aligns that table
/// against the offset the object it emits starts at, which it necessarily
/// treats as 0, so folding an odd contribution in ahead of it drops the pad
/// and shifts every byte after it. kb/codegen/0119 measured exactly that on
/// this binary, in this group, and prescribes this remedy: a separate object,
/// named explicitly with -zC because the default segment name would otherwise
/// come from this file's own basename (kb/codegen/0105), and one Tupfile.lua
/// line ahead of th02/boss_5.cpp so TLINK concatenates the two in dump order.
///
/// sub_175E8() and sub_17562() directly above are 0x86 bytes each - EVEN - so
/// they prepend into THIS file, at the top, with no parity consequence.
///
/// -G for the `push bp; mov bp, sp; sub sp, 8` prolog (kb/codegen/0011).
/// -zPmain_03 for the near calls into the four enemy helpers that are still
/// ASM in the same segment. No -a2: nothing here emits a generated jump table.
#pragma option -zCBOSS_5_TEXT -zPmain_03 -G

#include "platform.h"
#include "pc98.h"
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th01/rank.h"
#include "th02/v_colors.hpp"
#include "th02/core/globals.hpp"
#include "th02/main/playfld.hpp"
#include "th02/main/scroll.hpp"
#include "th02/main/score.hpp"
#include "th02/main/item/item.hpp"
#include "th02/main/bullet/bullet.hpp"
#include "th02/main/entity.hpp"
#include "th02/main/enemy/enemy.hpp"
#include "th02/hardware/pages.hpp"
#include "th02/snd/snd.h"

// Declared here rather than in th02/main/enemy/enemy.hpp for the same reason
// th02/main/enemy/enemies.cpp declares it there: that header belongs to the
// loader's parcel. (Same for [enemy_cur] and its satellites below.)
extern "C" uint8_t enemies_loop_bound;

// th02/main/boss/b3.cpp, b4.cpp and b5m.cpp each declare these three the same
// way. -1 means "no enemy is a homing target this frame"; the loop below
// re-derives them every frame from the lowest still-shootable enemy.
extern "C" int boss_pos_x;
extern "C" int boss_pos_y;

/// The current-enemy cursor block
/// ------------------------------
/// TH02's enemy code has no `enemy` parameter anywhere. Instead the loop
/// below publishes the slot it is working on into these file-scope globals,
/// and every helper it calls - enemy_run(), enemy_hittest(),
/// enemy_kill_update_and_render(), sub_16AA7() and the movement step above
/// them, all still ASM - reads them implicitly.
///
/// [enemy_cur] is an ADOPTION: TH04 and TH05 publish a global of exactly this
/// name, type and role (th04/main/enemy/enemy.hpp, `_enemy_cur` in both root
/// dumps). Their loops keep the walk in a `register enemy_t near *enemy` and
/// merely copy it out to the global for the callees; TH02's is the earlier and
/// cruder shape, where the cursor itself IS the global and it carries four
/// derived-pointer satellites. [enemy_template_cur] mirrors that pinned stem.
///
/// THE OTHER SIX ARE COINAGES, not adoptions, and are recorded as such in
/// state/notes/th02-enemies-update-and-render.md: no sibling game has
/// derived-pointer globals at all, so there is no stem to inherit and the
/// search that would have found one is bounded there rather than asserted
/// here. [enemy_left] / [enemy_top] are the position snapshot taken AFTER
/// enemy_run() returns, which is what the playfield clip test and every
/// blit below consume; they are not a loop-top cache of the slot's position.
extern "C" enemy_t near *enemy_cur;
extern "C" enemy_template_t near *enemy_template_cur;
extern "C" screen_x_t near *enemy_left_p;
extern "C" screen_y_t near *enemy_top_p;
extern "C" screen_x_t enemy_left;
extern "C" screen_y_t enemy_top;
extern "C" pixel_delta_8_t near *enemy_velocity_x_p;
extern "C" pixel_delta_8_t near *enemy_velocity_y_p;

/// Still ASM in th02_main.asm, directly above this file's contribution, and
/// published for this object's sake (kb/codegen/0123).
/// -----------------------------------------------------------------------
// One step of this enemy's script VM. TH03 and TH04 both call theirs
// enemy_run() (th03/main/enemy/enemy.cpp, th04/main/enemy/script.cpp), and
// TH02's returns the same kind of status the loop below dispatches on:
// 2 = this enemy is done for the frame, 1 = run another opcode.
extern "C" int near enemy_run(void);

// One frame of this enemy's death animation: advances [age] as the cel timer,
// bursts sparks on the frame it starts, blits the cel, and returns `true` on
// the frame it retires the slot to F_REMOVE. 24 frames over 3 frames per cel
// is ENEMY_KILL_CELS (th02/sprites/cels.h), which is what fixes the stem;
// TH04 and TH05 inline the same animation into their enemy update instead of
// giving it a name.
extern "C" bool16 near enemy_kill_update_and_render(void);

// Collides this enemy with the player, then returns shots_hittest()'s damage
// for it. Named after TH03's enemy_hittest() (th03/main/enemy/enemy.cpp).
extern "C" int near enemy_hittest(void);

// Fires one pellet from this enemy's bullet origin at the given angle.
// PLACEHOLDER, DELIBERATELY: the bounded search is in
// state/notes/th02-enemies-update-and-render.md section 3 and it FAILED.
// nmlgc has never named it, no sibling game has a one-pellet-per-enemy helper
// to adopt from, and its four call sites (three script opcodes plus the
// autofire gate below) rule out every candidate stem that fits only one of
// them. th02_main.asm publishes it under this placeholder, exactly as it
// already does for _left_26C56 two data blocks below -- but in the TWO-line
// form, because `pascal` decorates UPPER CASE with no leading underscore
// (kb/codegen/0086), so the `public` is `SUB_16AA7` and TASM's
// case-insensitivity makes it the same symbol as the `proc` line.
extern "C" void pascal near sub_16AA7(int angle);


// Runs and draws every enemy up to [enemies_loop_bound] - the script VM, the
// playfield clip, the animation, the player and shot collisions, the death
// and the blit. Installed into [enemies_update_and_render] by stage_init().
//
// ZUN quirk: the [in_kill_anim] arm tests enemy_kill_update_and_render()'s
// return value and then does the same thing either way. The `if` is in the
// original's instructions, so it is in the original's source; only the two
// arms are indistinguishable.
extern "C" void far enemies_update_and_render(void)

{
	int script_ret;
	screen_y_t lowest_top;
	int damage;
	int interval;
	register int i;
	register screen_y_t top_scrolled;

	script_ret = 0;
	lowest_top = 0;
	boss_pos_x = -1;
	boss_pos_y = -1;
	enemy_cur = enemies;
	for(i = 0; enemies_loop_bound > i; i++, enemy_cur++) {
		if(enemy_cur->flag != F_ALIVE) {
			continue;
		}
		enemy_template_cur = &enemy_templates[enemy_cur->template_id];
		enemy_left_p = &enemy_cur->pos_on_page[page_back].x;
		enemy_top_p = &enemy_cur->pos_on_page[page_back].y;
		enemy_velocity_x_p = &enemy_cur->velocity_x;
		enemy_velocity_y_p = &enemy_cur->velocity_y;
		do {
			if(!enemy_cur->in_kill_anim) {
				script_ret = enemy_run();
			}
			if(script_ret == 2) {
				goto next;
			}
		} while(script_ret == 1);
		enemy_left = *enemy_left_p;
		enemy_top = *enemy_top_p;
		top_scrolled = enemy_top;
		if(
			(enemy_left < (PLAYFIELD_LEFT - 32)) ||
			(enemy_left > (PLAYFIELD_RIGHT + 2))
		) {
			enemy_cur->flag = F_REMOVE;
			goto next;
		}
		if(enemy_cur->despawn_when_offscreen_vertically) {
			if(
				(enemy_top >= PLAYFIELD_BOTTOM) ||
				(enemy_top <= (PLAYFIELD_TOP - 32))
			) {
				enemy_cur->flag = F_REMOVE;
				goto next;
			}
		}
		enemy_cur->anim_frame++;
		if(
			(enemy_template_cur->anim_cels != 0) &&
			(enemy_cur->render_as == 1) &&
			((enemy_cur->anim_frame % enemy_template_cur->anim_frames_per_cel)
				== 0)
		) {
			enemy_cur->patnum_delta++;
			if(enemy_cur->patnum_delta >= enemy_template_cur->anim_cels) {
				enemy_cur->patnum_delta = 0;
			}
		}
		if((enemy_template_cur->hp == -1) || enemy_cur->not_shootable) {
			goto render;
		}
		if(enemy_cur->in_kill_anim) {
			if(enemy_kill_update_and_render()) {
				goto next;
			}
			goto next;
		}
		if((top_scrolled < 360) && (top_scrolled > lowest_top)) {
			boss_pos_x = (enemy_left + 8);
			boss_pos_y = (top_scrolled + 8);
			lowest_top = top_scrolled;
		}
		if((damage = enemy_hittest()) != 0) {
			if(enemy_template_cur->hp == -2) {
				snd_se_play(4);
			} else {
				enemy_cur->damage += damage;
				if(enemy_cur->damage >= enemy_template_cur->hp) {
					enemy_cur->age = 0;
					if(enemy_template_cur->item > 1) {
						items_add(
							(enemy_left + 12),
							enemy_top,
							(enemy_template_cur->item - IT_BOMB)
						);
					} else {
						items_add_semirandom((enemy_left + 12), enemy_top);
					}
					snd_se_play(3);
					score_delta += enemy_template_cur->score;
					if(rank == RANK_LUNATIC) {
						bullets_add_pellet(
							(enemy_template_cur->bullet_origin_x + enemy_left),
							(enemy_template_cur->bullet_origin_y + enemy_top),
							0,
							BG_1_AIMED,
							((3 << 4) + 12)
						);
					}
					enemy_cur->in_kill_anim = true;
					goto next;
				}
				snd_se_play(4);
				top_scrolled += scroll_line;
				if(top_scrolled >= RES_Y) {
					top_scrolled -= RES_Y;
				}
				if(top_scrolled < 0) {
					top_scrolled += RES_Y;
				}
				super_roll_put_1plane(
					enemy_left,
					top_scrolled,
					(enemy_template_cur->patnum + enemy_cur->patnum_delta),
					0,
					super_plane(V_WHITE)
				);
				goto next;
			}
		}
render:
		top_scrolled = enemy_top;
		if(enemy_template_cur->autofire_interval > 1) {
			interval = enemy_template_cur->autofire_interval;
			if((enemy_cur->anim_frame % interval) == 0) {
				sub_16AA7(0);
			}
		}
		if(enemy_template_cur->patnum == 0) {
			goto next;
		}
		top_scrolled += scroll_line;
		if(top_scrolled >= RES_Y) {
			top_scrolled -= RES_Y;
		}
		if(top_scrolled < 0) {
			top_scrolled += RES_Y;
		}
		if(enemy_cur->render_as != 2) {
			super_roll_put(
				enemy_left,
				top_scrolled,
				(enemy_template_cur->patnum + enemy_cur->patnum_delta)
			);
		} else {
			super_roll_put(enemy_left, top_scrolled, enemy_cur->patnum_delta);
		}
next: ;
	}
}
