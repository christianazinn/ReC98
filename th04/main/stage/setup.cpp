/// TH04 stage initialization
/// -------------------------
/// (#included from th04/main_035.cpp. ZUN's object for main_035_TEXT held all
/// seven stage setup functions, stage1_setup() through stagex_setup(), in that
/// address order, and all seven are now here — the file grew upwards, one
/// tail at a time, and th04_main.asm's contribution to that segment is down to
/// boss_reset() and one include.
/// th05/main/stage/setup.cpp is the same file for TH05, and holds all seven.)
///
/// TH04's are `far` where TH05's are `near` — the same function under the same
/// name compiled into a different memory model, which is kb/codegen/0115, not
/// a rename. The dump's `call` is the tell: `retf` here, `retn` there.

#include "libs/master.lib/pc98_gfx.hpp"
#include "th04/playchar.h"
#include "th04/sprites/main_cdg.h"
#include "th04/sprites/main_pat.h"
#include "th04/formats/bb.h"
#include "th04/formats/cdg.h"
#include "th04/main/null.hpp"
#include "th04/main/rank.hpp"
#include "th04/main/boss/boss.hpp"
#include "th04/main/boss/bosses.hpp"
#include "th04/main/boss/backdrop.hpp"
#include "th04/main/midboss/midboss.hpp"
#include "th04/main/stage/stage.hpp"
#include "th04/main/stage/stages.hpp"

// Both defined in th04/main/boss/boss.cpp, which notes that it holds them for
// alignment reasons rather than because they belong there.
extern char st06bk_cdg[];
extern char st06_bb[];

// Unlike st06_bb above, this one has NOT been lifted into C++: the string
// still sits in th04_main.asm's _DATA as `aSt05_bb`, reached through the
// zero-byte `_st05_bb` alias this parcel added in front of it
// (kb/codegen/0123). Defining it here instead would move bytes between
// objects, which is exactly what the alias exists to avoid.
extern char st05_bb[];

// Same story as st05_bb below: all three are private `_DATA` labels in
// th04_main.asm, reached through the zero-byte aliases this parcel added in
// front of them (kb/codegen/0123).
extern char st04bk_cdg[];
extern char st04_bb[];
extern char st04_cdg[];

// Four more private `_DATA` labels in th04_main.asm, aliased in place
// (kb/codegen/0123).
extern char st03_bmt[];
extern char st03bk_cdg[];
extern char st03bk2_cdg[];
extern char st03_bb[];

// Three more of the same, aliased in place (kb/codegen/0123).
extern char st02_bmt[];
extern char st02bk_cdg[];
extern char st02_bb[];

// And three more (kb/codegen/0123).
extern char st01_bmt[];
extern char st01bk_cdg[];
extern char st01_bb[];

// The last three (kb/codegen/0123).
extern char st00_bmt[];
extern char st00bk_cdg[];
extern char st00_bb[];

/// Stage 1
/// -------

// Orange enters on the first cel of the stage sprite range, the way every
// stage-1-style boss does. (th04/main/boss/render.cpp)
static const int PAT_ORANGE_STILL = PAT_STAGE;

void pascal far stage1_setup(void)
{
	midboss_update_func = midboss1_update;
	midboss_render_func = midboss1_render;
	midboss.frames_until = 3100;

	// Below the bottom of the playfield — Stage 1's midboss is the only one
	// that enters from underneath.
	//
	// [measured] The seeded velocity is DOWNWARD and never reaches an
	// integration step. +1 is down: stage2_setup() and stage3_setup() seed
	// (192, -32), above the top, with the same sign, and stagex_setup() spells
	// "up and to the right" as -4. And midboss1_update()'s phase 0 writes -1
	// into the Y velocity as its first statement, two instructions before its
	// first update_seg3() call, so the upward drift the midboss actually has
	// is that function's, not this seed's.
	//
	// A seeded value its own updater overwrites before first use would be a
	// taxonomy candidate, but it is deliberately left UNLABELLED: nothing
	// established whether anything reads this field between stage start and
	// midboss activation, so step 1 of kb/conventions/rec98-taxonomy.md's
	// table cannot be discharged, and an unclear classification is a stop
	// condition.
	midboss.pos.     cur.set(192, 368);
	midboss.pos.    prev.set(192, 368);
	midboss.pos.velocity.set(0, 1);
	midboss.hp = 800;

	// [inferred] Alone among the five setups that have a midboss at all, this
	// one does not clear [midboss.sprite]. The only other writer for this
	// midboss is midboss1_update() in th04/main/midboss/m1_updt.cpp, which seeds it during the
	// battle; nothing in the binary says whether the omission is deliberate.

	boss_reset();
	boss.pos.init(192, 40);
	boss_bg_render_func = orange_bg_render;
	boss_update_func = orange_update;
	boss_fg_render_func = orange_fg_render;
	boss.sprite = PAT_ORANGE_STILL;

	// [measured] The one boss in TH04 with a hitbox wider than it is tall.
	// The population is the seven boss_hitbox_radius.set() calls in this file
	// plus th04/main/boss/boss.cpp's Gengetsu override, which is
	// (GENGETSU_W / 4, GENGETSU_H / 2) = (24, 48); every other one is square
	// or taller than wide.
	boss_hitbox_radius.set(24, 16);

	boss_backdrop_colorfill = orange_backdrop_colorfill;

	super_entry_bfnt(st00_bmt);
	cdg_load_single_noalpha(CDG_BG_BOSS, st00bk_cdg, 0);
	bb_boss_load(st00_bb);

	// R and G of color 0, in master.lib's palette copy only: nothing here
	// calls palette_show(), so the change reaches the hardware at whatever
	// the next show is. B is left at whatever the background load put there.
	// The only stage setup that touches the palette at all.
	Palettes[0].c.r = 255;
	Palettes[0].c.g = 255;

	stage_render = nullfunc_near;
	stage_invalidate = nullfunc_near;
}
/// -------

/// Stage 2
/// -------

void pascal far stage2_setup(void)
{
	midboss_update_func = midboss2_update;
	midboss_render_func = midboss2_render;
	midboss.frames_until = 2600;
	midboss.pos.     cur.set(192, -32);
	midboss.pos.    prev.set(192, -32);
	midboss.pos.velocity.set(0, 1);
	midboss.hp = 750;
	midboss.sprite = 0;

	boss_reset();
	boss.pos.init(192, 81);
	boss_bg_render_func = kurumi_bg_render;
	boss_update_func = kurumi_update;
	boss_fg_render_func = kurumi_fg_render;

	// [measured] No PAT_* constant, and that is Kurumi's quirk rather than a
	// missing name: hers is the one TH04 boss whose [boss.sprite] is a cel
	// index into her own range instead of an absolute patnum, so
	// kurumi_fg_render() adds PAT_KURUMI on every blit.
	// (th04/sprites/main_pat.h says the same from the other side.)
	boss.sprite = 0;

	boss_hitbox_radius.set(24, 24);
	boss_backdrop_colorfill = kurumi_backdrop_colorfill;

	super_entry_bfnt(st01_bmt);
	cdg_load_single_noalpha(CDG_BG_BOSS, st01bk_cdg, 0);
	bb_boss_load(st01_bb);

	// The only per-rank parameter Kurumi takes, and it is a frame interval:
	// kurumi_1905A() in th04/main/boss/b2_updt.cpp fires a spread of aimed
	// pellets on every frame where
	// [boss.phase_frame] divides evenly by it. 255 on Easy against 8 on
	// Lunatic.
	#define spread_interval	boss_statebyte[0]
	spread_interval = select_for_rank(255, 128, 32, 8);
	#undef spread_interval

	stage_render = nullfunc_near;
	stage_invalidate = nullfunc_near;
}
/// -------

/// Stage 3
/// -------

// Elly's resting pose. elly_fg_render() blits [boss.sprite] as a single cel,
// and elly_update() in th04/main/boss/b3_upd.cpp animates forward from this
// patnum through the seven cels above it during her cast, then stores this
// value back when the animation ends. So it is both the still cel and the base
// of that range; only the former is what stage3_setup() means by it.
static const int PAT_ELLY_STILL = (PAT_STAGE + 6);

void pascal far stage3_setup(void)
{
	midboss_update_func = midboss3_update;
	midboss_render_func = midboss3_render;
	midboss.frames_until = 1600;
	midboss.pos.     cur.set(192, -32);
	midboss.pos.    prev.set(192, -32);
	midboss.pos.velocity.set(0, 4);
	midboss.hp = 850;
	midboss.sprite = 0;

	boss_reset();
	boss.pos.init(192, 64);
	boss_bg_render_func = elly_bg_render;
	boss_update_func = elly_update;
	boss_fg_render_func = elly_fg_render;
	boss.sprite = PAT_ELLY_STILL;
	boss_hitbox_radius.set(24, 24);
	boss_backdrop_colorfill = elly_backdrop_colorfill;

	super_entry_bfnt(st02_bmt);
	cdg_load_single_noalpha(CDG_BG_BOSS, st02bk_cdg, 0);
	bb_boss_load(st02_bb);

	// Elly's stage has no scrolling layer of its own; her background is drawn
	// by boss_bg_render_func() above.
	stage_render = nullfunc_near;
	stage_invalidate = nullfunc_near;
}
/// -------

/// Stage 4
/// -------

// Whichever of the two playable characters you are NOT playing as.
static const int PAT_REIMU_MARISA_STILL = PAT_STAGE;

void pascal far stage4_setup(void)
{
	midboss_update_func = midboss4_update;
	midboss_render_func = midboss4_render;
	midboss.frames_until = 2800;
	midboss.pos.     cur.set(144, -32);
	midboss.pos.    prev.set(144, -32);
	midboss.pos.velocity.set(4, 2);
	midboss.hp = 1200;
	midboss.sprite = 0;

	boss_reset();
	boss.pos.init(192, 64);
	boss_bg_render_func = reimu_marisa_bg_render;

	// Stage 4's boss is the character you did not pick.
	if(playchar == PLAYCHAR_MARISA) {
		boss_update_func = reimu_update;
		boss_fg_render_func = reimu_fg_render;

		// Reimu seeds seven per-rank pattern parameters; Marisa seeds none
		// and takes a flat HP pool instead.
		//
		// [4] and [5] are BOTH BSB_spread_delta_angle in boss_statebyte_t —
		// the union names the field, not the slot — and they are read back
		// from two different patterns in th04/main/boss/b4r_upd.cpp. Nothing in the binary
		// distinguishes them, so they stay bare indices rather than taking
		// an invented discriminator, the same call
		// th04/main/midboss/mx_update.cpp made for its own slots.
		#define orb_count       	boss_statebyte[0]
		#define orb_interval    	boss_statebyte[1]
		#define spread_turns_max	boss_statebyte[2]
		#define spread          	boss_statebyte[3]
		#define stack           	boss_statebyte[6]

		orb_count        = select_for_rank( 4,  6,  8, 12);
		orb_interval     = select_for_rank(16, 12,  8,  6);
		spread_turns_max = select_for_rank( 1,  2,  3,  4);
		spread           = select_for_rank(23, 23, 24, 24);
		boss_statebyte[4] = select_for_rank( 8,  9,  9, 10);
		boss_statebyte[5] = select_for_rank(18, 16, 14, 10);
		stack            = select_for_rank( 6,  8,  9, 10);

		#undef orb_count
		#undef orb_interval
		#undef spread_turns_max
		#undef spread
		#undef stack
	} else {
		boss_update_func = marisa_update;
		boss_fg_render_func = marisa_fg_render;
		boss.hp = 6000;
	}

	boss.sprite = PAT_REIMU_MARISA_STILL;
	boss_hitbox_radius.set(24, 24);
	boss_backdrop_colorfill = reimu_marisa_backdrop_colorfill;

	super_entry_bfnt(st03_bmt);

	// Written out in both arms rather than as one call with a ternary
	// filename: the original re-pushes the slot and DS inside each arm and
	// shares only the trailing 0 and the call itself, which is what -O's tail
	// merger produces from two full calls (kb/codegen/0097 — the `jmp` into
	// the middle of the second arm is the tell). A ternary instead hoists the
	// two common pushes above the test and routes the filename through AX,
	// which is 2 bytes shorter and one instruction longer.
	if(playchar != PLAYCHAR_REIMU) {
		cdg_load_single_noalpha(CDG_BG_BOSS, st03bk_cdg, 0);
	} else {
		cdg_load_single_noalpha(CDG_BG_BOSS, st03bk2_cdg, 0);
	}
	bb_boss_load(st03_bb);

	stage_render = nullfunc_near;
	stage_invalidate = nullfunc_near;
}
/// -------

/// Stage 5
/// -------

// Yuuka's single entrance/idle cel, at the start of the stage sprite range.
// th04/main/boss/b5r.cpp gives the same value the same name for
// yuuka5_fg_render()'s own use; it has internal linkage there, so the
// definition is repeated rather than shared.
static const int PAT_YUUKA5_STILL = PAT_STAGE;

void pascal far stage5_setup(void)
{
	// Like Stage 6, Stage 5 has no midboss.
	midboss_update_func = nullfunc_far;
	midboss_render_func = nullfunc_near;
	midboss.frames_until = 60000;

	boss_reset();
	boss.pos.init(192, 64);
	boss_bg_render_func = yuuka5_bg_render;
	boss_update_func = yuuka5_update;
	boss_fg_render_func = yuuka5_fg_render;
	boss.sprite = PAT_YUUKA5_STILL;
	boss_hitbox_radius.set(26, 26);
	boss_backdrop_colorfill = yuuka5_backdrop_colorfill;

	cdg_load_single_noalpha(CDG_BG_BOSS, st04bk_cdg, 0);
	bb_boss_load(st04_bb);
	cdg_load_single_noalpha(CDG_BG_2, st04_cdg, 0);

	// [inferred] Seeded off-screen and staggered, so the three stars do not
	// cross the playfield in step. The three values are ZUN's; the reason is
	// ours. stage5_render() scrolls them from here.
	stage5_star_center_y[0].v = to_sp(320);
	stage5_star_center_y[1].v = to_sp(40);
	stage5_star_center_y[2].v = to_sp(190);

	stage_render = stage5_render;
	stage_invalidate = stage5_invalidate;

	#define thicklaser_radius	boss_statebyte[0]
	thicklaser_radius = select_for_rank(144, 160, 168, 180);
	#undef thicklaser_radius
}
/// -------

/// Stage 6
/// -------

// Yuuka enters on the first cel of her own Stage 6 sprite range, the way every
// stage-1-style boss does — NOT "every boss": stage2_setup()'s own comment
// above records Kurumi as the one whose [boss.sprite] is a cel index into her
// own range rather than an absolute patnum.
//
// Unlike every sibling block in this file, nothing is declared under this
// comment. The value is PAT_YUUKA6_PARASOL_BACK_OPEN, which
// th04/sprites/main_pat.h already names, so there is no local constant for the
// comment to introduce.

void pascal far stage6_setup(void)
{
	// Stage 6 has no midboss at all — like Stage 5, and unlike the other five
	// setups, these three are only ever cleared, never pointed at anything.
	// The frame count is still seeded, because midboss_update() counts down
	// from it unconditionally.
	midboss_update_func = nullfunc_far;
	midboss_render_func = nullfunc_near;
	midboss.frames_until = 60000;

	boss_reset();
	boss.pos.init(192, 80);
	boss_bg_render_func = yuuka6_bg_render;
	boss_update_func = yuuka6_update;
	boss_fg_render_func = yuuka6_fg_render;
	boss.sprite = PAT_YUUKA6_PARASOL_BACK_OPEN;
	boss_hitbox_radius.set(24, 48);
	bb_boss_load(st05_bb);

	// Yuuka draws her own background; the stage layer stays empty.
	stage_render = nullfunc_near;
	stage_invalidate = nullfunc_near;

	// Both slots are named by th04/main/boss/boss[bss].asm's boss_statebyte_t
	// union, and both are read back from th04/main/boss/b4m.cpp. Spelled here the
	// way gengetsu_started is below: a function-local #define off the union
	// member name, since the array itself is a bare unsigned char[16].
	#define thicklaser_radius	boss_statebyte[0]
	#define spin_ring        	boss_statebyte[1]

	thicklaser_radius = select_for_rank(48, 64, 80, 96);
	spin_ring = select_for_rank(1, 1, 2, 4);

	#undef thicklaser_radius
	#undef spin_ring
}
/// -------

/// Extra Stage
/// -----------

// Mugetsu enters on the same cel the Extra Stage sprite range starts at, the
// way every stage-1-style boss does. (th04/main/boss/render.cpp)
static const int PAT_MUGETSU_STILL = PAT_STAGE;

// Off the left edge of the playfield and below its bottom, drifting up and to
// the right — the midboss flies in diagonally rather than fading in.
static const int MIDBOSSX_ENTRANCE_X = -16;
static const int MIDBOSSX_ENTRANCE_Y = 256;

void pascal far stagex_setup(void)
{
	midboss_update_func = midbossx_update;
	midboss_render_func = midbossx_render;
	midboss.frames_until = 5400;
	midboss.pos.     cur.set(MIDBOSSX_ENTRANCE_X, MIDBOSSX_ENTRANCE_Y);
	midboss.pos.    prev.set(MIDBOSSX_ENTRANCE_X, MIDBOSSX_ENTRANCE_Y);
	midboss.pos.velocity.set(4, -4);
	midboss.hp = 4096;
	midboss.sprite = 0;
	midboss.angle = 0x60;

	boss_reset();
	boss.pos.init(192, 80);
	boss_bg_render_func = mugetsu_gengetsu_bg_render;
	boss_update_func = mugetsu_update;
	boss_fg_render_func = mugetsu_fg_render;
	boss.sprite = PAT_MUGETSU_STILL;
	boss_hitbox_radius.set(24, 48);
	boss_backdrop_colorfill = mugetsu_gengetsu_backdrop_colorfill;

	// Cleared here rather than in boss_reset() because Gengetsu shares this
	// stage's single boss slot with Mugetsu: th04/main/boss/boss.cpp reads the
	// same byte under the same name after the Extra Stage dialog, and starts
	// her fight only if it is still clear.
	#define gengetsu_started static_cast<bool>(boss_statebyte[0])
	gengetsu_started = false;
	#undef gengetsu_started

	cdg_load_single_noalpha(CDG_BG_BOSS, st06bk_cdg, 0);
	bb_boss_load(st06_bb);

	stage_render = nullfunc_near;
	stage_invalidate = nullfunc_near;
}
/// -----------
