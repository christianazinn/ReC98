/// Stage 4 Bosses - Mai & Yuki, SOLO foreground rendering
/// ------------------------------------------------------
/// NOT the Stage 4 renderer: that is mai_yuki_fg_render(), which draws both
/// of them and is what th05/main/stage/setup.cpp installs into
/// [boss_fg_render_func]. This one draws ONE of them, and th05_main.asm
/// installs it into the *other* global, the runtime [boss_fg_render], when
/// the fight moves to its second phase — the block that loads _DM08.TX2 or
/// _DM09.TX2, gives the survivor 7900 fresh HP, and either leaves Mai in
/// [boss] or copies [yuki.pos.cur] into it and switches [boss_update] to
/// yuki_update(). Whichever of the pair is left therefore arrives in [boss],
/// which is why one body serves both. th05/main/boss/b4_both.cpp owns the
/// game-logic half of the same pairing and spells the arrangement
/// `#define mai boss` / `#define yuki boss2`.
///
/// They keep the three-way [boss_fg_render] contract that
/// th04/main/boss/fg.cpp documents for TH04's bosses, with two differences:
///
/// • One cel of each character's sprite block animates, over four cels
///   selected by [stage_frame_mod8]. Every other cel is blitted as-is.
/// • They are the TH05 bosses that do NOT reset [boss.damage_this_frame]
///   after the white flash — yumeko_fg_render() and exalice_fg_render() both
///   do. Something else has to clear it, which is Orange's and Kurumi's
///   arrangement in TH04 rather than Reimu's.
///
/// (#included from th05/b6cbull.cpp, ahead of
/// th05/main/bullet/swords_render.cpp. This function was the last `proc` of
/// th05_main.asm's MIDBOSSX_TEXT block once that module was lifted out from
/// under it, and this object is the segment's next contribution, so the lift
/// lands exactly where the root's block ended. kb/codegen 0112 + 0114.)

#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/v_colors.hpp"
#include "th04/main/frames.h"
#include "th04/main/boss/boss.hpp"
#include "th05/main/boss/bosses.hpp"

// [inferred] Kept file-local rather than added to th05/sprites/main_pat.h,
// because what these cels depict has not been decided — only where they are.
// Mai's block starts at PAT_MAI and Yuki's at PAT_YUKI, B4_CELS apart, and
// the animated one sits at the same offset inside both. th05/sprites/main_pat.h
// has no include guard and is already reached through th04/main/boss/boss.hpp,
// so the two patnums are spelled out here rather than derived from it.
static const int PAT_MAI_ANIMATED = 192;
static const int PAT_YUKI_ANIMATED = 208;

static const int B4_ANIM_FRAMES_PER_CEL = 2;

// `extern "C"` + `pascal`, because th05_main.asm installs this function by
// ADDRESS, in the block that assigns boss.pos.cur from yuki_pos.cur, and the
// original published no symbol for it at all — IDA named it after its address
// alone. Plain C++ linkage would emit a mangled name for that dump line to
// spell, for no gain; the undecorated upper-case spelling is what every other
// dump-referenced lift in this game uses (kb/codegen 0081 + 0102).
extern "C" void pascal near b4_solo_fg_render(void)
{
	// Same frame as exalice_fg_render(): `ENTER 2, 0`, three 16-bit locals,
	// two enregistered and one on the stack, running DI, SI, `[bp-2]`. So
	// [left] has to be declared FIRST to reach DI and [top] LAST to reach the
	// stack slot. (kb/codegen 0010 + 0146)
	screen_x_t left;
	int patnum;
	screen_y_t top;

	left = boss.pos.cur.to_screen_left(BOSS_W);
	top = boss.pos.cur.to_screen_top(BOSS_H);

	if(boss.phase == PHASE_BOSS_EXPLODE_BIG) {
		super_large_put(left, top, boss.sprite);
	} else {
		patnum = boss.sprite;
		if(
			(boss.sprite == PAT_YUKI_ANIMATED) ||
			(boss.sprite == PAT_MAI_ANIMATED)
		) {
			patnum += (stage_frame_mod8 / B4_ANIM_FRAMES_PER_CEL);
		}
		if(boss.damage_this_frame == 0) {
			super_put(left, top, patnum);
		} else {
			super_put_1plane(left, top, patnum, 0, super_plane(V_WHITE));
		}
	}
	explosions_small_update_and_render();
	explosions_big_update_and_render();
}
