/// Extra Stage Boss #1 - Mugetsu, foreground rendering
/// ---------------------------------------------------
/// (#included from th04/main_01.cpp, ahead of th04/main/boss/shield.cpp and
/// therefore in its original address order. ZUN's object for main_01_TEXT
/// held the loader now in th04/formats/bb_txt_load.cpp, this renderer, and
/// then the barrier;
/// that an original object held several unrelated sources is
/// kb/codegen/0112. th04/main_01.cpp also explains why that object's
/// Tupfile.lua line is position-critical.)
///
/// Mugetsu keeps the three-way [boss_fg_render] contract that
/// th04/main/boss/render.cpp documents for Orange and Kurumi, reduced to the
/// smallest form any TH04 boss has: no entrance branch at all — a zero
/// [boss.sprite] skips the whole blit, exactly as it does for Gengetsu — and
/// no cel animation, so [boss.sprite] is blitted as-is on every frame.
///
/// Two things she shares with Gengetsu and with no other TH04 boss:
///
/// • the bomb-invincibility barrier, drawn from the undamaged branch through
///   mugetsu_gengetsu_shield_render() (th04/main/boss/shield.cpp);
/// • the white damage flash firing only on every *other* damage frame, via
///   her own [mugetsu_damage_frames]. gengetsu_fg_render()
///   (th04/main/boss/fg.cpp) is the other half of that pair, with its own
///   counter.
///
/// Unlike Gengetsu, she is a single sprite rather than a pair, and has no
/// teleport animation; like Gengetsu and Reimu, and unlike Orange and Kurumi,
/// she does reset [boss.damage_this_frame] after the flash.

#include "platform.h"
#include "pc98.h"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/v_colors.hpp"
#include "th04/main/boss/boss.hpp"
#include "th04/main/boss/bosses.hpp"

// Defined in th04/main/boss/shield.cpp, which th04/main_01.cpp compiles into
// the same object *after* this file — so it needs a declaration here.
// `extern "C"` because the mangled spelling would be 35 characters
// (kb/codegen/0123).
extern "C" void near mugetsu_gengetsu_shield_render(void);

/// Still ASM
/// ---------
// Counts the frames on which Mugetsu took damage, and nothing else — its
// parity is the only thing ever read out of it, and this function is its only
// writer as well as its only reader. Gengetsu has the identical byte under
// [gengetsu_damage_frames] (th04/main/boss/fg.cpp), and no other TH04 boss
// halves its flash rate this way. A th04_main.asm `.data?` label with no
// `public` of ZUN's, so it needed a zero-byte `label` alias to become
// linkable (kb/codegen/0123). [inferred] name.
extern "C" unsigned char mugetsu_damage_frames;
/// ---------

// [inferred] Only `PLAYFIELD_LEFT - (MUGETSU_W / 2)` and
// `PLAYFIELD_TOP - (MUGETSU_H / 2)` survive into the binary, as the folded
// constants 0 and -32, so the split between "sprite box" and "extra offset"
// is not recoverable — the same note orange_fg_render()
// (th04/main/boss/render.cpp) and mugetsu_gengetsu_shield_render() carry.
// Reading the whole of it as a sprite box needs no leftover in either axis
// and puts her at the same height as Gengetsu, whose GENGETSU_H is also 96
// and whose cels come out of the same st06.bmt; the width is BOSS_W because
// she is one sprite where Gengetsu is a pair.
static const pixel_t MUGETSU_W = 64;
static const pixel_t MUGETSU_H = 96;

void pascal near mugetsu_fg_render(void)
{
	// The two register variables, and the only two: the original's prolog is
	// `PUSH BP` / `MOV BP, SP` / `PUSH SI` / `PUSH DI` with no `SUB SP` at
	// all, so this function has no stack locals for a third one to live in.
	// [left] is declared first because it takes SI. (kb/codegen/0010, 0117)
	screen_x_t left;
	vram_y_t top;

	if(boss.sprite != 0) {
		left = boss.pos.cur.to_screen_left(MUGETSU_W);
		top = boss.pos.cur.to_screen_top(MUGETSU_H);
		if(boss.phase < PHASE_EXPLODE_BIG) {
			if(boss.damage_this_frame == 0) {
				super_put(left, top, boss.sprite);
				mugetsu_gengetsu_shield_render();
			} else {
				mugetsu_damage_frames++;
				if(mugetsu_damage_frames & 1) {
					super_put(left, top, boss.sprite);
				} else {
					super_put_1plane(
						left, top, boss.sprite, 0, super_plane(V_WHITE)
					);
				}
				boss.damage_this_frame = 0;
			}
		} else if(boss.phase == PHASE_EXPLODE_BIG) {
			super_large_put(left, top, boss.sprite);
		}
	}
	explosions_small_update_and_render();
	explosions_big_update_and_render();
}
