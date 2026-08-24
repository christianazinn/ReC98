/// Stage 5 Boss - Mugetsu: the gather animation
/// -------------------------------------------------------
/// (#included from th04/bx1_gath.cpp, which names MUGETSU_TEXT --
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

/// The gather animation
/// --------------------

// One clockwise and one counter-clockwise ring, off the current
// [gather_template].
static void near mugetsu_1802F(void)
{
	gather_template.angle_delta = -2;
	gather_add_only();
	gather_template.angle_delta = 2;
	gather_add_only();
}

// Four frames of the animation, on whatever timeline
// [mugetsu_gather_frame_offset] puts [boss.phase_frame] on: set up and fire the
// first pair, recolour and fire the second, fire a third with no change, and
// then collapse the whole thing into a shrinking circle.
#pragma option -a1
void near mugetsu_18044(void)
{
	switch(boss.phase_frame + mugetsu_gather_frame_offset) {
	case 32:
		gather_template.radius.v = TO_SP(320);
		gather_template.center.y.v = (mugetsu_gather_center.y.v - TO_SP(10));
		gather_template.center.x.v = mugetsu_gather_center.x.v;
		gather_template.ring_points = 16;
		gather_template.col = 14;
		// fallthrough
	case 36:
rings:
		mugetsu_1802F();
		break;

	case 34:
		gather_template.col = 7;
		goto rings;

	case 48:
		circles_add_shrinking(
			gather_template.center.x.v, gather_template.center.y.v
		);
		circles_color = V_WHITE;
		break;
	}
}
