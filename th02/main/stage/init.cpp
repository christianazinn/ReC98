/* ReC98
 * -----
 * TH02's per-stage initialization. main() calls this once per stage, right
 * after seeding [random_seed] and right before the gameplay loop in
 * th02/main/stage/loop.cpp.
 */

// -G- (optimize for size) is what turns the prolog into a single ENTER;
// stage_loop() next door needs -G and its `push bp; mov bp, sp; sub sp, N`.
#pragma option -zCSTAGE_INIT_TEXT -zPmain_01 -G-

#include <mem.h>
#include "platform.h"
#include "pc98.h"
#include "shiftjis.hpp"
#include "decomp.hpp"
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "platform/x86real/pc98/page.hpp"
#include "th02/resident.hpp"
#include "th02/formats/map.hpp"
#include "th02/formats/mpn.hpp"
#include "th02/formats/pi.h"
#include "th01/rank.h"
#include "th02/core/globals.hpp"
#include "th02/gaiji/gaiji.h"
#include "th02/gaiji/loadfree.h"
#include "th02/hardware/pages.hpp"
#include "th02/math/randring.hpp"
#include "th02/snd/snd.h"
#include "th02/main/frames.hpp"
#include "th02/main/main.hpp"
#include "th02/main/null.hpp"
#include "th02/main/playfld.hpp"
#include "th02/main/playperf.hpp"
#include "th02/main/score.hpp"
#include "th02/main/scroll.hpp"
#include "th02/main/slowdown.hpp"
#include "th02/main/boss/bosses.hpp"
#include "th02/main/bullet/bullet.hpp"
#include "th02/main/dialog/dialog.hpp"
#include "th02/main/enemy/enemy.hpp"
#include "th02/main/hud/hud.hpp"
#include "th02/main/hud/overlay.hpp"
#include "th02/main/item/item.hpp"
#include "th02/main/midboss/midboss.hpp"
#include "th02/main/player/bomb.hpp"
#include "th02/main/player/player.hpp"
#include "th02/main/stage/stage.hpp"
#include "th02/main/stage/callback.hpp" // needs stage_progression_t, above
#include "th02/main/tile/tile.hpp"
#include "th02/main/bg_particle.hpp"
#include "th02/main/hiscore.hpp"
#include "th02/main/laser.hpp"

// Defined at the end of th02/main/stage/loop.cpp, which owns STAGE_TEXT.
void pascal near text_wipe(void);

// Frees every [enemies] slot and rewinds the spawn grid.
// th02/main/enemy/update.cpp, its own object in BOSS_5_TEXT.
extern "C" void far enemies_reset(void);

// th02/main/boss/b1.cpp
extern "C" void far boss_bgm_load(char *fn);

// Resets the scrolling state. Called through a `nopcall` alias.
// th02/main/scroll.cpp
void far scroll_reset(void);

// Callback defaults.
// th02/main/enemy/enemies.cpp, under th02/boss_5.cpp.
extern "C" void far enemies_invalidate(void);
// th02/main/enemy/update.cpp, its own object in the same segment.
extern "C" void far enemies_update_and_render(void);

// Now C++, in th02/main/bgm_show.cpp, which owns MAIN_01___TEXT's tail.
extern "C" void far stage_title_unput(void);
extern "C" void far boss_activate_if_scroll_done(void);

// Per-stage effect functions.
extern "C" void far stage_scenery_invalidate(void); // Stages 1 and 2
extern "C" void far stage1_update_and_render(void);
extern "C" void far stage2_update_and_render(void);
// th02/main/stage/stages.cpp owns DIALOG_TEXT's head.
extern "C" void far stage3_invalidate(void);
extern "C" void far stage3_update_and_render(void);
// th02/main/boss/b5.cpp. Named for the slot it is installed into below.
extern "C" void far stage4_update_and_render(void);

#define BOSS_DEC(name) \
	extern "C" void far name##_init(void); \
	extern "C" void far name##_end(void); \
	extern "C" void far name##_bg_render(void); \
	extern "C" stage_progression_t far name##_update(void);

BOSS_DEC(rika);
BOSS_DEC(meira);
BOSS_DEC(stones);
BOSS_DEC(marisa);
BOSS_DEC(mima);
BOSS_DEC(sigma);

// The stage's own copy of the .MPN palette, taken right after the file is
// loaded and applied once the previous stage has faded out.
//
// Shares nothing but the name with TH01's [stage_palette]
// (th01/main/shape.cpp): that one is a Palette4 holding the *live* hardware
// palette, which bosses mutate component-wise in place and re-show
// (th01/main/boss/b10m.cpp). This one is a Palette8 snapshot of a file that is
// only ever read back out.
extern "C" Palette8 stage_palette;

extern "C" uint8_t bgm_show_timer;
extern "C" uint8_t bgm_title_id;

// The BGM title shown once the boss starts; boss_activate_if_scroll_done()
// copies it into [bgm_title_id]. Qualifier first, because that is how a
// sibling game already spells the same concept in both languages:
// th04/main/hud/overlay.hpp's [boss_bgm_title] and
// th04/main/hud/overlay[bss].asm's [_boss_bgm_title_len].
extern "C" uint8_t boss_bgm_title_id;

extern "C" uint8_t stage1_gaiji_halflen;
extern "C" uint8_t gStage1[];

extern "C" shiftjis_t near *STAGE_TITLES[];
extern "C" shiftjis_t near *stage_title;
extern "C" const uint8_t STAGE_TITLE_HALFLENGTHS[];
extern "C" uint8_t stage_title_halflen;

extern "C" bool reduce_effects;

extern "C" const char aBmt[];
extern "C" const char aBbt[];
extern "C" const char aMap[];
extern "C" const char aMpn[];
extern "C" const char aM[];
extern "C" const char aMiko_k_mpn[];
extern "C" const char aHuuma_efc[];
extern "C" const char aEye_pi[];
extern "C" const char aMiko_bft[];
extern "C" const char aMiko32_bft[];
extern "C" const char aMiko16_bft[];

// The [farfp_1F4A4] slot: main() calls the per-stage gameplay loop through
// it, and gameplay_init() is the only thing that ever writes it.
// [HELD FOR NAMING REVIEW]
extern "C" bool16 (far *stage_loop_func)(void);

// th02/main/stage/loop.cpp
bool16 stage_loop(void);
// ---------------------------

// Turbo C++ compiled ZUN's far calls to same-code-group functions as
// `nop; push cs; call near ptr`. Now that this function sits in its own
// segment, only inline ASM still emits those bytes. (kb/codegen/0014)
#define nopcall_same_group(func) _asm { \
	nop; \
	push	cs; \
	call	near ptr func; \
}

// gameplay_init() and the two filename helpers all start with a plain
// `push bp; mov bp, sp` and have no locals, which is -G; stage_init() below
// starts with `ENTER 0Ch, 0`, which is -G-.
// (kb/codegen/0011, scoped exactly as th03/main/player/defeat.cpp does it.)
#pragma option -G

// Run-wide gameplay setup. main() calls this once, after the optional demo
// load and before the first stage_init(), and never again — everything here
// outlives a stage transition.
void near gameplay_init(void)
{
	snd_load(aHuuma_efc, SND_LOAD_SE);
	hiscore_get();
	pi_load(0, aEye_pi);
	super_entry_bfnt(aMiko_bft);
	super_entry_bfnt(aMiko32_bft);
	super_entry_bfnt(aMiko16_bft);
	for(int i = 48; i < 128; i++) {
		super_convert_tiny(i);
	}
	gaiji_load();
	bomb_load();
	reduce_effects = resident->reduce_effects;
	stage_loop_func = stage_loop;
	if(resident->continues_used) {
		score = (
			(resident->continues_used >= 9) ? 9 : resident->continues_used
		);
		item_bigpower_override = ((stage_id % 5) + 2);
	}
	if(rank == RANK_EASY) {
		playperf_max = 4;
	} else {
		playperf_max = 16;
	}
}

// The filename layout both of these assume: "stage<N>.<ext>".
static const int STAGE_FN_DIGIT = 5;
static const int STAGE_FN_EXT = 7;

// Replaces the 3-character extension of a filename previously built by
// stage_fn(). Defined before it because ZUN put it at the lower address.
void pascal near stage_fn_ext_set(char *fn, const char *ext)
{
	fn[STAGE_FN_EXT + 0] = ext[0];
	fn[STAGE_FN_EXT + 1] = ext[1];
	fn[STAGE_FN_EXT + 2] = ext[2];
}

// Writes the current stage's .BFT filename into [fn], which must have room for
// 11 characters and a terminator. Named after TH01's scoredat_fn().
void pascal near stage_fn(char *fn)
{
	fn[0] = 's';
	fn[1] = 't';
	fn[2] = 'a';
	fn[3] = 'g';
	fn[4] = 'e';
	fn[STAGE_FN_DIGIT] = (stage_id + '0');
	fn[6] = '.';
	fn[STAGE_FN_EXT + 0] = 'b';
	fn[STAGE_FN_EXT + 1] = 'f';
	fn[STAGE_FN_EXT + 2] = 't';
	fn[10] = '\0';
}

#pragma option -G-

void near stage_init(void)
{
	char fn[12];
	register int i;

	stage_fn(fn);
	vsync_Count1 = 0;
	text_wipe();
	graph_scrollup(0);
	graph_accesspage(1);
	graph_clear();
	graph_accesspage(0);
	graph_clear();
	graph_showpage(0);
	hud_put();
	overlay_wipe();
	// ZUN bloat: Applied twice, for no effect.
	pi_palette_apply(0);
	pi_palette_apply(0);
	pi_put_8(96, 144, 0);
	bullets_and_sparks_init();
	enemies_reset();
	bg_particles_reset();
	lasers_reset();
	bomb_reset();
	snd_se_reset();
	bosses_reset();
	nopcall_same_group(scroll_reset);
	randring_fill();
	palette_100();

	player_left_on_page[1] = player_left_on_page[0] = PLAYER_LEFT_START;
	player_top_on_page[1] = player_top_on_page[0] = PLAYER_TOP_START;
	stage_frame = 0;
	midboss_active = false;
	stage_progression = SP_STAGE;
	slowdown_factor = 1;
	if(!resident->demo_num) {
		bgm_show_timer = 1;
		bgm_title_id = (stage_id * 2);
		boss_bgm_title_id = ((stage_id * 2) + 1);
	} else {
		bgm_show_timer = 0;
	}

	for(i = 192; i >= 128; i--) {
		if(super_patsize[i]) {
			super_cancel_pat(i);
		}
	}

	stage1_gaiji_halflen = 6;
	gStage1[5] = ((stage_id % 5) + gb_1);
	stage_title = STAGE_TITLES[stage_id];
	stage_title_halflen = STAGE_TITLE_HALFLENGTHS[stage_id];

	super_entry_bfnt(fn);
	stage_fn_ext_set(fn, aBmt);
	super_entry_bfnt(fn);
	stage_fn_ext_set(fn, aBbt);
	super_entry_bfnt(fn);
	stage_fn_ext_set(fn, aMap);
	map_load(fn);
	tiles_stuff_reset();
	stage_fn_ext_set(fn, aMpn);
	mpn_load(fn);
	memcpy(
		reinterpret_cast<void *>(&stage_palette),
		reinterpret_cast<void *>(&mpn_palette),
		sizeof(Palette8)
	);
	enemy_stagedata_free();
	enemy_stagedata_load();
	dialog_load_and_init();

	boss_bg_render = nullfunc_void;
	boss_update = reinterpret_cast<stage_progression_t (far *)(void)>(
		nullfunc_false
	);
	stage_update_and_render = nullfunc_void;
	stage_invalidate = nullfunc_void;
	lasers_invalidate_func = nullfunc_void;
	lasers_update_and_render_func = nullfunc_void;
	enemies_invalidate_func = enemies_invalidate;
	enemies_update_and_render_func = enemies_update_and_render;
	boss_activate_if_scroll_done_func = boss_activate_if_scroll_done;
	stage_title_unput_func = stage_title_unput;
	stage_should_end_func = nullfunc_false;
	player_reset();
	scroll_speed = 1;
	scroll_interval = 4;

	switch(stage_id) {
	case 0:
		midboss_scroll_step = 116;
		midboss_invalidate = midboss1_invalidate;
		midboss_update_and_render = midboss1_update_and_render;
		boss_init = rika_init;
		boss_end = rika_end;
		boss_bg_render_func = rika_bg_render;
		boss_update_func = rika_update;
		stage_update_and_render = stage1_update_and_render;
		stage_invalidate = stage_scenery_invalidate;
		break;

	case 1:
		midboss_scroll_step = 80;
		midboss_invalidate = midboss2_invalidate;
		midboss_update_and_render = midboss2_update_and_render;
		boss_init = meira_init;
		boss_end = meira_end;
		boss_bg_render_func = meira_bg_render;
		boss_update_func = meira_update;
		stage_update_and_render = stage2_update_and_render;
		stage_invalidate = stage_scenery_invalidate;
		break;

	case 2:
		midboss_scroll_step = 103;
		midboss_invalidate = midboss3_invalidate;
		midboss_update_and_render = midboss3_update_and_render;
		boss_init = stones_init;
		boss_end = stones_end;
		boss_bg_render_func = stones_bg_render;
		boss_update_func = stones_update;
		stage_update_and_render = stage3_update_and_render;
		stage_invalidate = stage3_invalidate;
		break;

	case 3:
		midboss_scroll_step = 944;
		midboss_invalidate = midboss4_invalidate;
		midboss_update_and_render = midboss4_update_and_render;
		boss_init = marisa_init;
		boss_end = marisa_end;
		boss_bg_render_func = marisa_bg_render;
		boss_update_func = marisa_update;
		if(!reduce_effects) {
			stage_update_and_render = stage4_update_and_render;
		}
		lasers_callbacks_set();
		scroll_speed = 2;
		scroll_interval = 1;
		break;

	case 4:
		// Stage 5 has no midboss, and disables it with an unreachable
		// [scroll_step].
		midboss_scroll_step = -1;
		midboss_invalidate = nullfunc_false;
		midboss_update_and_render = nullfunc_void;
		boss_init = mima_init;
		boss_end = mima_end;
		boss_bg_render_func = mima_bg_render;
		boss_update_func = mima_update;
		scroll_interval = 1;
		break;

	case 5:
		midboss_scroll_step = 200;
		midboss_invalidate = midbossx_invalidate;
		midboss_update_and_render = midbossx_update_and_render;
		boss_init = sigma_init;
		boss_end = sigma_end;
		boss_bg_render_func = sigma_bg_render;
		boss_update_func = sigma_update;
		lasers_callbacks_set();
		scroll_interval = 2;
		break;
	}

	tile_area_init_and_put_both();
	if(!resident->demo_num) {
		snd_delay_until_volume(255);
		stage_fn_ext_set(fn, aM);
	}
	items_init_and_reset();
	score_extend_init();
	while(vsync_Count1 < 100) {
	}
	text_boxfilla(4, 1, 51, 23, (TX_BLACK | TX_REVERSE));
	palette_set_all(stage_palette);
	palette_show();
	graph_accesspage(1);
	tiles_fill_and_put_initial();
	graph_accesspage(0);
	tiles_render_all();
	mpn_free();
	mpn_load(aMiko_k_mpn);
	if(!resident->demo_num) {
		boss_bgm_load(fn);
	}
	grc_setclip(0, 0, (RES_X - 1), (RES_Y - 1));
	grcg_setcolor(GC_RMW, 11);
	graph_accesspage(0);
	grcg_boxfill(PLAYFIELD_RIGHT, 0, 575, (RES_Y - 1));
	graph_accesspage(1);
	grcg_boxfill(PLAYFIELD_RIGHT, 0, 575, (RES_Y - 1));
	grcg_off();
	page_front = 1;
	page_show(page_front);
	// ZUN's compiler materialized the 0 once, into AL, and reused it for both
	// the port write and the [page_back] store. `page_access(0); page_back =
	// 0;` cannot reproduce that: the __emit__ that carries the immediate-form
	// OUT is an opaque barrier to -Z's register tracking, so the store falls
	// back to a 5-byte `mov byte ptr [page_back], 0`. The `db` also pins the
	// direction bit of the `xor` (kb/codegen/0088).
	_asm {
		db  	030h, 0C0h; // xor al, al
		out 	0A6h, al;
	}
	page_back = _AL;
}

#pragma option -G

// Restarts the current stage after the player used a continue. main() calls
// this immediately after the continue menu returns 1, and then re-enters the
// gameplay loop without going through stage_init() again.
void near continue_resume(void)
{
	hud_put();
	nopcall_same_group(overlay_stage_leave_animate);
	// Assigned in this order, not the other way round: the store to [1] comes
	// first in the original, and -O emits a chained assignment's stores in the
	// reverse of their source order (kb/codegen/0092).
	player_left_on_page[0] = player_left_on_page[1] = PLAYER_LEFT_START;
	player_top_on_page[0] = player_top_on_page[1] = PLAYER_TOP_START;
	player_reset();
	player_invincibility_time = CONTINUE_INVINCIBILITY_FRAMES;
	graph_accesspage(page_front);
	tiles_render_all();
	graph_accesspage(page_back);
	tiles_render_all();
	bullets_clear();
	palette_100();
	nopcall_same_group(overlay_stage_enter_animate);
}
