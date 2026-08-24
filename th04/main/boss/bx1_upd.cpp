/// Stage 5 Boss - Mugetsu: the fight's own update function
/// -------------------------------------------------------
/// (#included from th04/bx1_upd.cpp, which names MUGETSU_TEXT --
/// th04_main.asm's kb/codegen/0080 carve off main_033_TEXT's head. That
/// segment's root contribution is enemies_update() and nothing else now, and
/// this fight's four objects append behind it in address order, which is the
/// order these functions already had. See th04/bx1_gath.cpp.)
///
/// mugetsu_fg_render() is elsewhere, in th04/main/boss/bx1_fg.cpp, and so is
/// mugetsu_gengetsu_bg_render(), which phase 0 installs.
///
/// **A naming round is owed** for every address-suffixed function name below.
/// Every one of them is reached from mugetsu_update()'s two `switch`es, or
/// from a pose driver, and from nowhere else, which is why the ones that stay
/// inside one object are `static` -- exactly like Gengetsu's half of the Extra
/// Stage in th04/main/boss/bx2_upd.cpp -- and why the zero-byte `label`
/// aliases th04_main.asm would otherwise have needed (kb/codegen/0123) do not
/// exist.
///
/// THE SPLIT, and why there are four objects rather than one
/// ----------------------------------------------------------
/// `[measured 2026-08-24]` A single object for all fourteen procs is RED at
/// 400 bytes: in any translation unit that reaches th04/main/bullet/bullet.hpp
/// -- which th04/main/gather.hpp pulls in, and which pulls in
/// th04/main/playfld.hpp TOGETHER WITH th04/main/rank.hpp, either alone being
/// harmless -- Turbo C++'s OBJ writer stages the two pose drivers' dense
/// `switch` selector through AX (`mov ax,mem` / `sub ax,10h` / `mov bx,ax`)
/// instead of loading BX directly, one byte longer. That extra byte flips the
/// function's `-a2` table parity and the pad in front of the table disappears
/// with it, so the change is LENGTH-NEUTRAL and neither an object-length probe
/// nor a SEGDEF or PUBDEF check can see it -- and `tcc -S` prints the BX form
/// for the very
/// same source, so kb/codegen/0152's listing screen reports it clean. Six
/// `#pragma option` settings and seven source spellings do not move it; only
/// the header set does.
///
/// So the pose pair gets an object with no bullet.hpp in its closure. The
/// other three boundaries are then forced by the `-a2` parity arithmetic of
/// kb/codegen/0096 (as corrected by 0154: the pad appears when the natural
/// table offset is EVEN *in the compiling object*), measured lengths:
///
///   bx1_gath  mugetsu_1802F 0x15, mugetsu_18044 0x67 + 0x10 sparse pair
///                                                              = 0x8C
///   bx1_pose  mugetsu_180BB 0x6F, mugetsu_1812A 0xB1 + pad + 0x42,
///             mugetsu_1821E 0xB3 + pad + 0x42                   = 0x259
///   bx1_ptn   the four patterns and the fight's helpers          = 0x3D7
///   bx1_upd   mugetsu_update 0x2CE + pad + 0x24 + 0x10           = 0x303
///
/// mugetsu_180BB() is in the POSE object and not the gather one because
/// mugetsu_1812A()'s table needs an ODD prefix to land on an even offset and
/// take its pad; 0x6F supplies it, and 1821E's follows. That leaves 0x3D7 --
/// odd -- of helpers ahead of mugetsu_update(), which would put ITS table at
/// an odd offset and lose its pad, so mugetsu_update() takes the fourth
/// object and its table sits at 0x2CE from a zero prefix. Sum 0x9BF, the same
/// 0x9BF the one-object version had: only the boundaries moved.
///
/// The eleven functions called across an object boundary therefore lost their
/// `static`; they are declared in th04/main/boss/bx1.cpp. Every call is still
/// near, within MUGETSU_TEXT and within `main_03`, so no call site changed
/// length.

#include "platform.h"
#include "pc98.h"
// iatan2(), which the cross-ring pattern aims with.
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th03/hardware/palette.hpp"
#include "th02/main/player/player.hpp"
#include "th02/main/player/bomb.hpp"
#include "th02/v_colors.hpp"
#include "th04/snd/snd.h"
#include "th04/formats/std.hpp"
#include "th04/math/randring.hpp"
#include "th04/sprites/main_pat.h"
#include "th04/main/frames.h"
#include "th04/main/bg.hpp"
#include "th04/main/homing.hpp"
#include "th04/main/null.hpp"
#include "th04/main/gather.hpp"
#include "th04/main/circle.hpp"
#include "th04/main/hud/hud.hpp"
#include "th04/main/bullet/bullet.hpp"
#include "th04/main/bullet/clearzap.hpp"
#include "th04/main/player/player.hpp"
#include "th04/main/tile/bb.hpp"
#include "th04/main/midboss/midboss.hpp"
#include "th04/main/boss/boss.hpp"

// Declared FAR here, and only here, which is why
// th04/main/boss/bosses.hpp -- the header that declares it `near`, which
// is what it is -- is deliberately not included. A near reference under
// this object's `-zPmain_03` frames its offset on main_03, and
// mugetsu_gengetsu_bg_render() lives in main_01, which is a
// `Fixup overflow at MUGETSU_TEXT` at link time. kb/codegen/0162, and
// th04/main/boss/bx2_upd.cpp does the same thing for the same store.
// Nothing else in that header is reached from this object.
void pascal far mugetsu_gengetsu_bg_render(void);
// [mugetsu_phase2_mode] and this file's own mugetsu_phase2_next()
// declaration, which predates the lift.
#include "th04/main/boss/bx1.cpp"

/// The fight's own state
/// ---------------------
/// All three are th04_main.asm slots with no `public` of ZUN's, and this
/// object's functions are their only readers or writers in any of the five
/// binaries, so this parcel coined all three names. `[inferred]`.
extern "C" {
	// Added to [boss.phase_frame] before mugetsu_18044()'s gather `switch`,
	// which is how the three pose drivers put the same animation on three
	// different timelines. `_DATA` rather than `.data?`: it is initialised to
	// 0x10, which is mugetsu_180BB's value.
	extern int mugetsu_gather_frame_offset;

	// The current pose driver: mugetsu_180BB(), mugetsu_1812A() or
	// mugetsu_1821E().
	extern unsigned char (near *mugetsu_pose_func)(void);

	// Where the next gather animation is centred, and where the teleport in
	// the middle of a pose sequence puts the boss.
	extern SPPoint mugetsu_gather_center;
}

// The byte th04_main.asm already aliases as `_extra_boss_bomb_immunity`, with
// the same meaning and the same 32 frames Gengetsu's half of the Extra Stage
// gives it: while it is nonzero the fight takes damage through a wider fixed
// box and throws the result away. gengetsu_update() and gengetsu_20202() in
// th04/main/boss/bx2_upd.cpp are this file's twins on both counts, so the name
// and the constant are theirs rather than newly coined -- **the naming round
// that file's own comment says is owed covers this reader too.**
extern "C" unsigned char extra_boss_bomb_immunity;
static const int BOMB_IMMUNITY_FRAMES = 32;
/// ---------------------

#pragma option -a2
void pascal far mugetsu_update(void)
{
	unsigned char pattern;

	if(bombing) {
		extra_boss_bomb_immunity = BOMB_IMMUNITY_FRAMES;
	}
	if(extra_boss_bomb_immunity != 0) {
		extra_boss_bomb_immunity--;
	}
	bullet_template.origin.x.v = boss.pos.cur.x.v;
	bullet_template.origin.y.v = (boss.pos.cur.y.v - TO_SP(10));
	bullet_template.spawn_type = BST_PELLET;

	switch(boss.phase) {
	case 0:
		// The entrance: the fight owns the stage script from here on.
		if(boss.phase_frame == 0) {
			stage_vm = nullfunc_far;
			midboss.frames_until = 0;
			mugetsu_pose_func = mugetsu_180BB;
			extra_boss_bomb_immunity = 0;
			boss.hp = 9400;
			boss.phase_end_hp = 3700;
			mugetsu_gather_center.x.v = boss.pos.cur.x.v;
			mugetsu_gather_center.y.v = boss.pos.cur.y.v;
		}
		mugetsu_186B9();
		if(boss.phase_frame > 128) {
			boss.phase++;
			boss.phase_frame = 0;
			snd_se_play(13);
			tiles_bb_col = V_WHITE;
			// One line of inline ASM because the `far` declaration above
			// makes the plain assignment ill-typed. It mentions no
			// register, so kb/codegen/0009's hidden SI push and
			// kb/codegen/0143's register demotion do not apply -- and
			// mugetsu_update() has no register variable to demote anyway.
			_asm mov word ptr bg_render_bombing_func, offset mugetsu_gengetsu_bg_render
		}
		break;

	case 1:
		mugetsu_186B9();
		if(boss.phase_frame >= 64) {
			boss.phase++;
			boss.mode = 0;
			boss.phase_state.patterns_seen = 2;
			boss.phase_frame = 0;
			boss.pos.velocity.x.v = 0;
			mugetsu_phase2_mode = 0;
		}
		break;

	case 2:
		// The fight proper: eight patterns cycled by [mugetsu_phase2_mode],
		// with a reposition-and-reroll interlude between them.
		switch(boss.mode) {
		// The fight's first pattern, spelled as its own `case` rather than
		// folded into `case 6` below even though the two bodies are identical.
		// That is measured, not stylistic: `-O` cross-jumping turns this body
		// into a `jmp short` to case 6's, and then retargets this value's
		// jump-table slot at case 6's body too -- which leaves the `jmp` in
		// the object with nothing pointing at it. Those two dead bytes exist
		// in the original and a single `case 0: case 6:` cannot produce them.
		case 0:
			mugetsu_18314();
			break;

		case 3:
			mugetsu_184AC();
			break;

		case 1: case 4: case 5:
			mugetsu_1845E();
			break;

		case 6:
			mugetsu_18314();
			break;

		case 2: case 7:
			mugetsu_1838A();
			break;

		case 255:
			if(boss.phase_frame > 16) {
				if(randring2_next16_and(3) == 0) {
					mugetsu_pose_func = mugetsu_180BB;
				} else {
					mugetsu_pose_func = mugetsu_1812A;
					do {
						pattern = randring2_next16_mod(5);
					} while(boss.phase_state.patterns_seen == pattern);
					boss.phase_state.patterns_seen = pattern;
					_AX = pattern;
					_AX <<= 6;
					_AX <<= 4;
					mugetsu_gather_center.x.v = (_AX + TO_SP(64));
					mugetsu_gather_center.y.v = boss.pos.cur.y.v;
				}
				boss.phase_frame = 0;
				mugetsu_phase2_mode++;
				_AL = mugetsu_phase2_mode;
				_AL &= 7;
				boss.mode = _AL;
			}
			break;
		}

		if(
			(mugetsu_phase2_mode >= 32) &&
			(boss.mode != 255) &&
			(boss.phase_frame > 24)
		) {
			mugetsu_18655();
		}
		if(mugetsu_phase2_mode < 36) {
			if(!mugetsu_186B9()) {
				break;
			}
			boss_score_bonus(100);
		}
		if(bullet_clear_time < 20) {
			bullet_clear_time = 20;
		}
		mugetsu_phase2_next(static_cast<explosion_type_t>(0), 0);
		mugetsu_gather_center.x.v = TO_SP(192);
		break;

	case 3:
		mugetsu_186B9();
		if(boss.phase_frame >= 64) {
			boss.phase++;
			mugetsu_pose_func = mugetsu_1821E;
			boss.phase_frame = 0;
		}
		break;

	case 4:
		mugetsu_186B9();
		mugetsu_18556();
		if(boss.phase_frame == 0) {
			boss_explode_small(ET_HORIZONTAL);
			boss.phase++;
			boss.phase_frame = 0;
			boss.sprite = 129;
		}
		break;

	case 5:
		mugetsu_186B9();
		if(boss.phase_frame >= 128) {
			boss.phase++;
			boss.phase_frame = 0;
		}
		break;

	case 6:
		// The last phase: no pose driver, both volleys on the phase clock,
		// and a 4000-frame timeout that ends the fight without the bonus.
		mugetsu_185E4();
		if(boss.phase_frame >= 3000) {
			mugetsu_18655();
		}
		if(!mugetsu_186B9()) {
			if(boss.phase_frame < 4000) {
				break;
			}
		}
		boss_explode_small(ET_NW_SE);
		boss.phase++;
		if(boss.phase_frame < 4000) {
			boss.phase_state.defeat_bonus = true;
		} else {
			boss.phase_state.defeat_bonus = false;
		}
		boss.phase_frame = 0;
		boss.mode = 0;
		palette_settone_deferred(100);
		break;

	case 7:
		boss.phase_frame++;
		if(boss.phase_frame == 16) {
			boss_explode_small(ET_VERTICAL);
		}
		if(boss.phase_frame == 32) {
			boss_defeat_explode_big(ET_SW_NE, 200);
			snd_se_play(12);
			palette_changed = true;
			player_invincibility_time = BOSS_DEFEAT_INVINCIBILITY_FRAMES;
		}
		break;

	default:
		boss_defeat_update();
		return;
	}

	homing_target.x.v = boss.pos.cur.x.v;
	homing_target.y.v = boss.pos.cur.y.v;
	hud_hp_update_and_render(boss.hp, 9400);
}
#pragma option -a1
