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
#include "th04/main/boss/boss.hpp"
#include "th04/main/boss/bosses.hpp"
#include "th04/main/boss/backdrop.hpp"
#include "th04/main/midboss/midboss.hpp"
#include "th04/main/stage/stage.hpp"

// Both defined in th04/main/boss/boss.cpp, which notes that it holds them for
// alignment reasons rather than because they belong there.
extern char st06bk_cdg[];
extern char st06_bb[];

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
