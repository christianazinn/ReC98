/// The miss animation
/// ------------------
/// (#included from th04/main_0.cpp, at the very front of it, ahead of
/// sub_10ABF() and player_render() -- the address order all three bodies had
/// in main_0_TEXT. This proc was the last thing th04_main.asm contributed to
/// that segment, so the object grows backwards into the hole and every byte
/// above it keeps its address (kb/codegen 0099 + 0112 + 0114). It is also the
/// last one: the root dump's contribution to main_0_TEXT is now EMPTY.)
///
/// The function keeps IDA's spelling, and the failed name search is recorded
/// in state/notes/th04-main-carve-tails-2.md per kb/conventions/
/// naming-precedents.md section 3. th04/main/player/update.cpp has called it
/// `sub_10988()` since before this lift, its only other call site is nowhere,
/// and TH05's counterpart in the identical role -- sub_12017(), still ASM in
/// th05_main.asm's SHOT_INV_TEXT and reached by th05/shot_inv.cpp through the
/// same kind of alias -- is equally unnamed. Naming one without the other is
/// what this lane declined for sub_10ABF()/sub_1214A() one parcel ago.
///
/// One frame of the miss: at the top of the animation it drops the miss item
/// set, taxes power and the dream bonus, and spends the life; then it advances
/// the explosion ring every frame, flashes the palette over the last four, and
/// finally either respawns the player or ends the game.
///
/// Most of the externs below are spelled here rather than reached through
/// th04/main/item/item.hpp, th04/main/playperf.hpp, th04/main/hud/hud.hpp,
/// th04/main/bullet/clearzap.hpp and th02/snd/snd.h. Every one of those is
/// UNGUARDED, and th04/main/player/update.cpp expands the last of them further
/// down in THIS object, so a second expansion here would reject every
/// `static const` and struct they declare (kb/codegen/0129). Same block, and
/// the same reason, as th04/main/continue.cpp and th04/main/execl.cpp.
///
/// libs/master.lib/pc98_gfx.hpp is the exception and IS included, for the
/// reason given at the include itself: it is guarded, so re-expanding it in
/// th04/main/player/render.cpp below costs nothing, and this object needs what
/// it reaches to be parsed ahead of the first `_asm`.

#include "platform.h"
#include "pc98.h"

// For [PaletteTone] -- but it also has to come BEFORE the first `_asm` in this
// object, and that is not a style point. Turbo C++ 4.02 stops expanding
// inline functions for the REST of a module once it has seen inline assembly,
// and emits every `inline` DEFINED after that point out of line instead.
// x86real.h, which this header reaches, defines `_inportb_` as
// `__emit__(0xE4, port)` over a function PARAMETER -- legal only when it is
// expanded at a call site with a literal, and a hard compile error about an
// illegal parameter when it is not. Nothing in this object calls it; it is enough
// that the definition is parsed on the wrong side of the island below. That
// cost this parcel its one build cycle. th04/main/continue.cpp gets away with
// the same islands only because its own include block happens to sit first.
#include "libs/master.lib/pc98_gfx.hpp"

#include "th04/resident.hpp"
#include "th04/main/quit.hpp"
#include "th04/main/player/player.hpp"

// ---------------------------------------------------------------------
extern bool palette_changed;

// `extern "C"` and `pascal` are both what th02/snd/snd.h says further down in
// this same object -- it declares this one DEFCONV, which is __pascal from
// TH03 onwards, inside an `extern "C"` block. Getting either half wrong here
// is a compile error rather than a silent one, since the two declarations
// meet in this translation unit.
extern "C" void pascal snd_se_play(int new_se);

extern uint8_t power;
extern int power_overflow;
extern unsigned char dream_items_collected;
extern unsigned int dream_score;
extern unsigned char playperf;
extern unsigned char bullet_clear_time;

// Published by th04/main/item/items[data].asm. One entry per possible value of
// [dream_items_collected], which is why the index needs no clamp.
extern "C" const unsigned int DREAM_SCORE_PER_ITEMS[];

// `far`, and NOT what th04/main/item/item.hpp used to say. That header
// declared this one `near` and without `extern "C"`, which is wrong twice
// over -- th04/main/item/miss_add.asm defines it `proc far` and exports the
// undecorated, all-caps ITEMS_MISS_ADD (kb/codegen 0081 + 0086) -- and nothing
// had ever graded it, because no translation unit both included that header
// and called the function. The header is corrected in this parcel; this
// declaration is the one the corrected version agrees with.
extern "C" void pascal far items_miss_add(void);

// All five reached through the hand-spelled island below rather than by a
// plain call: see the comment on nopcall_same_group().
extern "C" void pascal playperf_lower(char delta);
extern "C" void pascal hud_dream_put(void);
extern "C" void pascal hud_lives_put(void);
extern "C" void pascal hud_bombs_put(void);

// Derives [shot_level] from [power] and installs [playchar_shot_func]. Still
// ASM, as sub_11DE6 in th04_main.asm's main_012_TEXT, which publishes a
// zero-byte `_sub_11DE6` alias for references like this one (kb/codegen/0123).
// Its own body is an attempted and unsolved match; see
// state/notes/th04-main-sub-11DE6.md.
extern "C" void near sub_11DE6(void);

// The Game Over sequence, in th04/main/gameover.cpp. Declared by no header,
// and this is a plain near call in the original because both bodies end up in
// group main_01.
unsigned char near gameover(void);
// ---------------------------------------------------------------------

// MISS_ANIM_FLASH_AT comes from th04/main/player/player.hpp above, where this
// parcel added it: th04/main/player/player.inc has always carried the value,
// but the compare below was the only thing in either dump that read it and no
// C++ had ever needed a spelling for it.

// Turbo C++ compiled ZUN's far calls to same-code-group functions as
// `nop; push cs; call near ptr`, which no plain C++ far call reproduces --
// not by declaring the callee `far`, not by compiling this object into the
// callee's group, and not by a `procdesc` in the root ASM.
// (kb/codegen/0014, kb/codegen/0083.) Same spelling as
// th04/main/continue.cpp's.
#define nopcall_same_group(func) _asm { \
	nop; \
	push	cs; \
	call	near ptr func; \
}

extern "C" void near sub_10988(void)
{
	// [bp-1], with the odd byte at [bp-2] left as padding (kb/codegen/0010).
	unsigned char power_lost;

	miss_time--;

	// [miss_time] counts down from (MISS_ANIM_FRAMES + DEATHBOMB_WINDOW), so
	// anything above MISS_ANIM_FRAMES is still the deathbomb window and the
	// animation has not started. th04/main/player/bomb.cpp owns that half.
	if(miss_time > MISS_ANIM_FRAMES) {
		return;
	}
	if(miss_time == MISS_ANIM_FRAMES) {
		player_pos.velocity.x.v = 0;
		player_pos.velocity.y.v = 0;
		power_overflow = 0;
		miss_explosion_radius = 0;
		items_miss_add();

		// A quarter of the current power, capped at 16. Both operands are
		// bytes but the division is signed against a 16-bit divisor, which is
		// what the `mov bx, 4` / `cwd` / `idiv bx` in the original is.
		power_lost = (power / 4);
		if(power_lost > 16) {
			power_lost = 16;
		}
		power -= power_lost;

		// One point item's worth of the dream bonus is lost, and the new
		// per-item value is re-read out of the table rather than scaled.
		if(dream_items_collected != 0) {
			dream_items_collected--;
		}
		dream_score = DREAM_SCORE_PER_ITEMS[dream_items_collected];
		nopcall_same_group(hud_dream_put);
		nopcall_same_group(sub_11DE6);
		snd_se_play(2);

		// Rank is knocked down twice: clamped to 21 if it was at or above 22,
		// and then lowered by a further 4 regardless.
		if(playperf >= 22) {
			playperf = 21;
		}

		// `__emit__(0x6A, 4)` rather than a call argument or `_asm push 4`:
		// the argument is pushed AHEAD of the island in the original, and the
		// inline assembler is free to pick the 3-byte `68 imm16` form for a
		// `push` it assembles itself (kb/codegen 0083 + 0089).
		__emit__(0x6A, 4);
		nopcall_same_group(playperf_lower);

		resident->miss_count++;
	}

	// The explosion ring, advanced every frame of the animation.
	miss_explosion_radius += MISS_EXPLOSION_RADIUS_VELOCITY;

	// `static_cast<int>` is load-bearing and kb/codegen/0094 is the reason: a
	// BYTE-typed constant addend folds the whole thing into `ADD mem, imm8`,
	// while an int-typed one forces the AL round trip the original uses. The
	// header spells the constant `uint8_t`, so the width has to be restored
	// here.
	miss_explosion_angle += static_cast<int>(MISS_EXPLOSION_ANGLE_VELOCITY);

	if(miss_time >= (MISS_ANIM_FRAMES - MISS_ANIM_FLASH_AT)) {
		return;
	}

	// The last-life flash: on the final four frames, and only while the player
	// still has a life left to lose. Deferred to the end of the frame rather
	// than shown immediately, which is why this is not palette_settone().
	if(resident->rem_lives > 1) {
		if(miss_time & 1) {
			PaletteTone = 150;
		} else {
			PaletteTone = 100;
		}
		palette_changed = true;
	}
	if(miss_time != 0) {
		return;
	}

	// Respawn, at the bottom center of the playfield and drifting upwards.
	player_pos.cur.x.v = TO_SP(192);
	player_pos.prev.x.v = TO_SP(192);
	player_pos.cur.y.v = TO_SP(368);
	player_pos.prev.y.v = TO_SP(368);
	player_pos.velocity.x.v = 0;
	player_pos.velocity.y.v = TO_SP(-2);

	// [rem_lives] is one-based: 1 means this was the last one. The [resident]
	// far pointer is re-loaded after the island because a call clobbers ES.
	if(resident->rem_lives > 1) {
		resident->rem_lives--;
		nopcall_same_group(hud_lives_put);
		resident->rem_bombs = resident->credit_bombs;
		nopcall_same_group(hud_bombs_put);
		bullet_clear_time = 32;
		return;
	}
	quit = static_cast<quit_t>(gameover());
}
