/// TH04 stage initialization
/// -------------------------
/// (#included from th04/main_035.cpp. ZUN's object for main_035_TEXT held all
/// seven stage setup functions, stage1_setup() through stagex_setup(), in that
/// address order; the other six are still in th04_main.asm, so this file is
/// appended to that contribution and grows upwards, one tail at a time.
/// th05/main/stage/setup.cpp is the same file for TH05, and holds all seven.)
///
/// TH04's are `far` where TH05's are `near` — the same function under the same
/// name compiled into a different memory model, which is kb/codegen/0115, not
/// a rename. The dump's `call` is the tell: `retf` here, `retn` there.

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

/// Stage 6
/// -------

// Yuuka enters on the first cel of her own Stage 6 sprite range, the way every
// boss does. (th04/sprites/main_pat.h)

void pascal far stage6_setup(void)
{
	// Stage 6 has no midboss at all — unlike every other stage, these three are
	// only ever cleared, never pointed at anything. The frame count is still
	// seeded, because midboss_update() counts down from it unconditionally.
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
	// union, and both are read back from still-ASM Yuuka code. Spelled here the
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
