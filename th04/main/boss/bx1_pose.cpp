/// Stage 5 Boss - Mugetsu: the three pose drivers
/// -------------------------------------------------------
/// (#included from th04/bx1_pose.cpp, which names MUGETSU_TEXT --
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

// A DELIBERATELY MINIMAL include set, and the object's whole reason for
// existing: th04/main/gather.hpp -- and with it th04/main/bullet/bullet.hpp,
// th04/main/playfld.hpp and th04/main/rank.hpp -- must NOT be in this
// translation unit's closure, or both `switch` heads below stage their
// selector through AX. See the header comment. `boss.hpp` brings
// th04/main/playfld.hpp in on its own, which is measured harmless; it is
// playfld.hpp TOGETHER WITH rank.hpp that flips the allocation.
#include "platform.h"
#include "pc98.h"
#include "th04/snd/snd.h"
#include "th04/main/frames.h"
#include "th04/main/boss/boss.hpp"
// [mugetsu_phase2_mode], the mugetsu_phase2_next() declaration, and the
// eleven cross-object declarations this split needs -- mugetsu_18044() for
// this file, and the three drivers below for th04/main/boss/bx1_upd.cpp.
#include "th04/main/boss/bx1.cpp"

// This object's half of th04/main/boss/bx1_gath.cpp's state block. Duplicated
// rather than shared because that block sits inside the full include set the
// other three objects use, and this object exists precisely to not have it.
extern "C" {
	// Added to [boss.phase_frame] before mugetsu_18044()'s gather `switch`.
	extern int mugetsu_gather_frame_offset;

	// Where the next gather animation is centred, and where the teleport in
	// the middle of a pose sequence puts the boss.
	extern SPPoint mugetsu_gather_center;
}

/// The three pose drivers
/// ----------------------
/// One per pattern length. All three run the gather animation, walk
/// [boss.sprite], and return the pattern's phase; they differ only in the
/// three frame thresholds and in whether the walk goes through the sprite
/// sequence (the two below) or waits on it (this one).

// The short one: no sprite sequence at all, just the open-mouth flap, and it
// gates on [boss.sprite] having reached 24 rather than on a frame.
unsigned char near mugetsu_180BB(void)
{
	mugetsu_gather_frame_offset = 0x10;
	mugetsu_18044();
	if(boss.phase_frame >= 16) {
		if(boss.phase_frame == 16) {
			// Not `boss.sprite = 129;` but a jump into the store the
			// [stage_frame_mod2] test below already makes. The original
			// contains ONE such store, reached from here by a forward `jz`,
			// and Turbo C++'s `-O` tail merging cannot produce that from two
			// identical statements: it folds the LATER duplicate into the
			// EARLIER one, which gives a backward jump and puts the surviving
			// store in the wrong place. Measured both ways.
			goto sprite_flap_closed;
		}
		if(boss.sprite >= 24) {
			if(boss.phase_frame < 48) {
				if(boss.phase_frame == 24) {
					snd_se_play(8);
				}
				if(stage_frame_mod2 != 0) {
					boss.sprite = 130;
				} else {
sprite_flap_closed:
					boss.sprite = 129;
				}
			} else if(boss.phase_frame == 48) {
				boss.sprite = 128;
				return 1;
			} else if(boss.phase_frame < 128) {
				return 2;
			} else {
				return 3;
			}
		}
	}
	return 0;
}

// The medium one, and the first of the two that walk the 131…135 turn-around
// sequence and teleport at its midpoint.
#pragma option -a2
unsigned char near mugetsu_1812A(void)
{
	mugetsu_gather_frame_offset = 0;
	mugetsu_18044();
	switch(boss.phase_frame) {
	case 34:
		boss.sprite = 0;
		boss.pos.cur.x.v = mugetsu_gather_center.x.v;
		boss.pos.cur.y.v = mugetsu_gather_center.y.v;
		break;

	case 32: case 38:
		boss.sprite = 135;
		break;

	case 30: case 40:
		boss.sprite = 134;
		break;

	case 28: case 42:
		boss.sprite = 133;
		break;

	case 26: case 44:
		boss.sprite = 132;
		break;

	case 24: case 46:
		boss.sprite = 131;
		break;

	case 16: case 48:
		boss.sprite = 129;
		break;
	}
	if(boss.phase_frame < 48) {
		return 0;
	}
	if(boss.phase_frame < 64) {
		// ZUN bloat: unreachable. mugetsu_1821E() below is this same code with
		// this constant equal to its own window threshold, and therefore
		// reachable on exactly that frame; here the threshold moved to 64 and
		// the constant stayed at 32, so nothing inside the 48…63 window can
		// ever equal it.
		if(boss.phase_frame == 32) {
			snd_se_play(8);
		}
		if(stage_frame_mod2 != 0) {
			boss.sprite = 130;
		} else {
			boss.sprite = 129;
		}
	} else if(boss.phase_frame == 64) {
		boss.sprite = 128;
		return 1;
	} else if(boss.phase_frame < 144) {
		return 2;
	} else {
		return 3;
	}
	return 0;
}

// The long one: the same sequence again, on a 48/128/192 timeline, and the
// gather runs 0x50 frames early.
unsigned char near mugetsu_1821E(void)
{
	mugetsu_gather_frame_offset = -0x50;
	mugetsu_18044();
	switch(boss.phase_frame) {
	case 34:
		boss.sprite = 0;
		boss.pos.cur.x.v = mugetsu_gather_center.x.v;
		boss.pos.cur.y.v = mugetsu_gather_center.y.v;
		break;

	case 32: case 38:
		boss.sprite = 135;
		break;

	case 30: case 40:
		boss.sprite = 134;
		break;

	case 28: case 42:
		boss.sprite = 133;
		break;

	case 26: case 44:
		boss.sprite = 132;
		break;

	case 24: case 46:
		boss.sprite = 131;
		break;

	case 16: case 48:
		boss.sprite = 129;
		break;
	}
	if(boss.phase_frame < 48) {
		return 0;
	}
	if(boss.phase_frame < 128) {
		if(boss.phase_frame == 48) {
			snd_se_play(8);
		}
		if(stage_frame_mod2 != 0) {
			boss.sprite = 130;
		} else {
			boss.sprite = 129;
		}
	} else if(boss.phase_frame == 128) {
		boss.sprite = 128;
		return 1;
	} else if(boss.phase_frame < 192) {
		return 2;
	} else {
		return 3;
	}
	return 0;
}
#pragma option -a1
