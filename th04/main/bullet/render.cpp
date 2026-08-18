/// Bullet rendering
/// ----------------
/// Blits every 16×16 bullet and, depending on whether bullets are currently
/// being zapped or cleared, either the pellet delay clouds plus the two
/// hand-written pellet blitters, or every pellet's own decay sprite.
///
/// (#included from th04/main/playfld.cpp, ahead of th04/main/scroll.cpp, which
/// is kb/codegen/0129's host-source form. On the TH05 side this function was
/// the last proc of th05_main.asm's PLAYFLD_TEXT root contribution and
/// th05/playfld.cpp already owned everything after it (kb/codegen/0114), so
/// the #include is the original address order and costs neither a carve nor a
/// Tupfile.lua line. TH04 keeps its own copy in th04_main.asm's BOSS_FG_TEXT,
/// which is a different segment with a different host, so this file is
/// TH05-only for now.)
///
/// Because this file shares a translation unit with th04/main/playfld.cpp,
/// every #pragma option it sets keeps applying to everything the object
/// generates after it (kb/codegen/0112 trap 0) — and the two functions after
/// this one are already matched. The one option this file needs is therefore
/// bracketed and restored at the bottom; do not add an unbracketed one.

// ZUN compiled this as its own object, and it wants the opposite speed/size
// setting from the two functions after it in this one: the original's prolog
// is `55 8B EC 83 EC 02` (`push bp; mov bp, sp; sub sp, 2`), which is `-G`,
// while th04/main/scroll.cpp and playfield_shake_update_and_render() below
// both come out of the command line's `-G-` as `ENTER`. Bracketed and
// restored at the bottom of this file (kb/codegen/0011 + 0112 trap 1).
#pragma option -G

#include "th02/v_colors.hpp"
#include "th02/sprites/bullet16.h"
#include "th04/formats/super.h"
#include "th04/hardware/grcg.hpp"
#include "th04/main/bullet/pellet_r.hpp"
#include "th04/main/bullet/clearzap.hpp"
#include "th05/sprites/main_pat.h"

// The delay clouds for 16×16 bullets come from miko32.bft and are twice the
// size of the bullet they announce, which is why they are blitted with the
// 32×32 function and at a different origin than everything else here.
static const pixel_t BULLET16_CLOUD_W = 32;
static const pixel_t BULLET16_CLOUD_H = 32;

// `extern "C"` + `pascal` is what spells the all-uppercase, undecorated
// `BULLETS_RENDER` that th04_main.asm exports for TH04's copy; plain C++
// linkage would emit `@BULLETS_RENDER$QV` instead (kb/codegen/0081).
extern "C" void pascal near bullets_render(void)
{
	#define left	_AX
	#define top 	_DX

	// [bullet] takes SI by how often it is dereferenced as a base
	// (kb/codegen/0117). [patnum] and [i] are mentioned the same number of
	// times, so the remaining register is decided by declaration order and
	// [patnum] has to be declared first to win DI (kb/codegen/0146); the
	// store order below is unaffected by that. [i] is then the single
	// `sub sp, 2` stack local.
	//
	// This is the *mirror* of TH04's copy of this function, which homes
	// [patnum] on the stack and keeps its counter in DI — TH04 has no pellet
	// cloud loop, so its [patnum] is mentioned twice less. Measured off each
	// dump; "first-declared takes the stack slot" does not generalise.
	int patnum;
	bullet_t near *bullet;
	int i;

	_ES = SEG_PLANE_B;
	bullet = &bullets[BULLET_COUNT - 1];

	for(i = 0; i < BULLET16_COUNT; (i++, bullet--)) {
		if(bullet->flag != F_ALIVE) {
			continue;
		}
		if(bullet->spawn_flag <= BSF_CLOUD_BACKWARDS) {
			// Not spawned yet, or alive: the bullet itself.
			top = bullet->pos.cur.to_vram_top_scrolled_seg1(BULLET16_H);
			left = bullet->pos.cur.to_screen_left(BULLET16_W);
			z_super_roll_put_tiny_16x16(left, top, bullet->patnum);
		} else {
			// The delay cloud. ZUN clips it manually rather than through the
			// playfield_encloses*() macros, and the result is asymmetric:
			// the top, left and bottom bounds are the playfield's own, but
			// the right one is 16 pixels *past* it, so a cloud can be drawn
			// half a sprite into the HUD.
			// ZUN quirk, pending an emulator observation.
			if(
				(bullet->pos.cur.y >= 0) &&
				(bullet->pos.cur.y < TO_SP(PLAYFIELD_H)) &&
				(bullet->pos.cur.x >= 0) &&
				(bullet->pos.cur.x < TO_SP(PLAYFIELD_W + 16))
			) {
				// Blue for everything blue-ish, red for everything else. The
				// non-directional bullets below PAT_BULLET16_N_RED are the
				// blue half of that set; the two ranges after it are the blue
				// directional and the blue vector bullets. (The upper bound
				// of the last one is PAT_CLOUD_PELLET, i.e. everything up to
				// the end of the vector bullets.)
				if(
					(bullet->patnum < PAT_BULLET16_N_RED) ||
					(
						(bullet->patnum >= PAT_BULLET16_D_BLUE) &&
						(bullet->patnum < PAT_BULLET16_D_GREEN)
					) || (
						(bullet->patnum >= PAT_BULLET16_V_BLUE) &&
						(bullet->patnum < (
							PAT_CLOUD_PELLET + BULLET_CLOUD_CELS
						))
					)
				) {
					patnum = (PAT_CLOUD_BULLET16_BLUE - 1);
				} else {
					patnum = (PAT_CLOUD_BULLET16_RED - 1);
				}
				patnum += (
					bullet->spawn_flag / (BSF_CLOUD_FRAMES / BULLET_CLOUD_CELS)
				);

				top = bullet->pos.cur.to_vram_top_scrolled_seg1(
					BULLET16_CLOUD_H
				);
				left = bullet->pos.cur.to_screen_left(BULLET16_CLOUD_W);
				z_super_roll_put_tiny_32x32(left, top, patnum);
			}
		}
	}

	if(!bullet_zap.active && !bullet_clear_time) {
		// [bullet] has walked all the way down to the last pellet by now, but
		// this branch never uses it as one — the cloud list holds its own
		// pointers, and the two blitters below read [pellets_render].
		while(pellet_clouds_render_count--) {
			bullet = pellet_clouds_render[pellet_clouds_render_count];
			patnum = (
				(bullet->spawn_flag /
					(BSF_CLOUD_FRAMES / BULLET_CLOUD_CELS)
				) + (PAT_CLOUD_PELLET - 1)
			);
			top = bullet->pos.cur.to_vram_top_scrolled_seg1(BULLET16_H);
			left = bullet->pos.cur.to_screen_left(BULLET16_W);
			z_super_roll_put_tiny_16x16(left, top, patnum);
		}

		grcg_setcolor_direct(V_WHITE);
		pellets_render_top();
		grcg_setcolor_direct(pellet_bottom_col);
		pellets_render_bottom();
	} else {
		// Zapping or clearing replaces both hand-written pellet blitters with
		// a regular sprite blit of each pellet's own decay animation, which
		// continues [bullet]'s descending walk into the pellet half of
		// [bullets].
		for(i = 0; i < PELLET_COUNT; (i++, bullet--)) {
			if(bullet->flag != F_ALIVE) {
				continue;
			}
			top = bullet->pos.cur.to_vram_top_scrolled_seg1(BULLET16_H);
			left = bullet->pos.cur.to_screen_left(BULLET16_W);
			z_super_roll_put_tiny_16x16(left, top, bullet->patnum);
		}
	}

	#undef top
	#undef left
}

// Restores what th04/main/playfld.cpp had before this file: the command line
// passes no -G, and both functions after this point need `ENTER`.
#pragma option -G-
