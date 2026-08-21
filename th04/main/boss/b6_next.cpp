/// Extra Stage Boss - Yuuka: ending a phase
/// ----------------------------------------
/// (#included from th04/b6_next.cpp, which is its own object for the reason
/// that file gives.)
///
/// Until this parcel, this function was still assembled from a module that
/// th04_main.asm `include`d, and that `include` was the last emitting item of
/// the dump's main_034_TEXT contribution -- which is why the seventeen
/// `yuuka6_1?????` patterns and helpers above it could never become the tail.
/// It sat immediately below them and immediately above yuuka6_update(), which
/// is where this object puts it back.

#include "platform.h"
#include "pc98.h"
#include "th04/sprites/main_pat.h"
#include "th04/main/bullet/clearzap.hpp"
#include "th04/main/boss/boss.hpp"
#include "th04/main/boss/explode.hpp"

/// Still ASM
/// ---------
extern "C" {
	// th04_main.asm `.data?`, shared with the seventeen patterns still there
	// and with th04/main/boss/b6_upd.cpp.
	extern int yuuka6_anim_frame;
	extern unsigned char yuuka6_sprite_flag;
}

// th04/main/boss/b6.cpp's yuuka6_sprite_flag_t, restated as the single
// enumerator this function needs rather than included: that file declares
// Yuuka's whole fight and is expanded by th04/main/boss/bg.cpp, another
// object, in the same way th04/main/boss/b6_upd.cpp restates it.
static const int Y6SF_PARASOL_BACK_OPEN = 1;
/// ---------

// Ends the current phase: starts a bullet clear if one is not already running,
// plays the given explosion, and resets every piece of per-phase state. The
// phase this moves on to will itself end at [next_end_hp].
//
// `extern "C"`, because that is the only spelling that reproduces the plain,
// undecorated public the dump used to export for this function -- which is
// what th04/main/boss/b6_upd.cpp declares and links against. `pascal` alone
// does NOT suffice: measured from this object's own PUBDEF, C++ linkage
// upper-cases the name by the calling convention and then STILL appends the
// mangled argument suffix. TLINK reports the demangled form, so the failure
// reads as if the body never compiled. kb/codegen/0027 carries this as a
// counter-shape; state/notes/yuuka6_phase_next.md has the exact two
// spellings. (th04/main/boss/b6.cpp declares this function without
// `extern "C"`, so that declaration has never resolved against anything. It
// is harmless only because th04/main/boss/bg.cpp, which expands that file,
// does not call it.)
extern "C" void pascal near yuuka6_phase_next(
	explosion_type_t explosion_type, int next_end_hp
)
{
	if(bullet_clear_time < 20) {
		bullet_clear_time = 20;
	}
	boss_explode_small(explosion_type);
	boss.phase++;
	boss.phase_frame = 0;
	boss.phase_state.patterns_seen = 0;
	boss.mode = 0;
	boss.hp = boss.phase_end_hp;
	boss.phase_end_hp = next_end_hp;
	yuuka6_anim_frame = 0;
	boss.sprite = PAT_YUUKA6_PARASOL_BACK_OPEN;
	yuuka6_anim_frame = 0; // ZUN bloat
	yuuka6_sprite_flag = Y6SF_PARASOL_BACK_OPEN;
}
