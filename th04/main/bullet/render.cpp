/// Bullet rendering
/// ----------------
/// Blits every 16×16 bullet and, depending on whether bullets are currently
/// being zapped or cleared, either the pellet delay clouds plus the two
/// hand-written pellet blitters, or every pellet's own decay sprite.
///
/// ONE name, one body, two games — and unlike th04/main/item/render.cpp next
/// door, this one does NOT come out with zero `#if (GAME == 5)` sites. Four
/// places in the body differ in a way no header constant accounts for, and
/// each is marked `DIFFERENCE n/4` below: the cloud's right clip bound, how the
/// cloud's colour is picked, the pellet-cloud render list (TH05 only), and the
/// GRCG colour the bottom pellet blitter runs under. A fifth `#if` picks the
/// game's own sprite sheet header. The register allocation differs too — the
/// two dumps are mirrored over which of [patnum] and [i] wins DI — but that
/// one is a *consequence* of the third difference and needs no `#if`; see the
/// declaration block.
///
/// TWO DIFFERENT HOSTS, FOR TWO DIFFERENT REASONS:
///
/// • TH05: #included from th04/main/playfld.cpp, ahead of th04/main/scroll.cpp,
///   which is kb/codegen/0129's host-source form. This function was the last
///   proc of th05_main.asm's PLAYFLD_TEXT root contribution and
///   th05/playfld.cpp already owned everything after it (kb/codegen/0114), so
///   the #include is the original address order and costs neither a carve nor a
///   Tupfile.lua line.
///
/// • TH04: its OWN object, th04/bullet_r.cpp, ahead of th04/boss_fg.cpp in the
///   link list. The dump's copy is the whole of th04_main.asm's BOSS_FG_TEXT
///   root contribution, and th04/boss_fg.cpp is the next contribution there, so
///   the cheap kb/codegen/0114 route would have been to #include this file at
///   the front of that object. It is NOT taken, on two measured grounds:
///   `th02/v_colors.hpp` (an enum) and `th04/hardware/grcg.hpp` (an inline
///   function) are unguarded and already in that object's include closure
///   through th04/main/boss/fg.cpp, and th04/formats/super.h — also unguarded —
///   is already in it through th04/main/item/render.cpp. Since this body has to
///   come FIRST there, no reordering avoids the second inclusion. On top of
///   that the body is 0x10B = 267 bytes, which is odd, so kb/codegen/0119 would
///   have to be re-checked for a closure the host object does not currently
///   have. A separate object is 0119's own prescribed fix and has no include
///   closure to share.
///
/// Because this file shares a translation unit with th04/main/playfld.cpp on
/// the TH05 side, every #pragma option it sets keeps applying to everything the
/// object generates after it (kb/codegen/0112 trap 0) — and the two functions
/// after this one are already matched. The one option this file needs is
/// therefore bracketed and restored at the bottom; do not add an unbracketed
/// one. TH04's object holds nothing but this file, so the bracket is a no-op
/// there rather than a second convention.

// ZUN compiled this as its own object, and it wants the opposite speed/size
// setting from the two functions after it in this one: the original's prolog
// is `55 8B EC 83 EC 02`, a manual BP frame with two bytes of locals under `-G`,
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
#if (GAME == 5)
	#include "th05/sprites/main_pat.h"
#else
	#include "th04/sprites/main_pat.h"
#endif

// TH05 reaches the PlayfieldPoint accessors through th04/main/playfld.cpp, the
// rest of its translation unit. TH04's object is this file alone, so it has to
// name the header itself; it carries an include guard, so the TH05 side is
// unaffected by naming it here.
#include "th04/main/playfld.hpp"

// The delay clouds for 16×16 bullets come from miko32.bft and are twice the
// size of the bullet they announce, which is why they are blitted with the
// 32×32 function and at a different origin than everything else here.
static const pixel_t BULLET16_CLOUD_W = 32;
static const pixel_t BULLET16_CLOUD_H = 32;

// `extern "C"` + `pascal` is what gives this definition an all-uppercase,
// undecorated linker symbol; plain C++ linkage would decorate the name instead
// (kb/codegen/0081). Both dumps used to export exactly that spelling for their
// own copies and neither does any more — TH05's lift took its copy, and TH04's
// took the last one — but the two keywords stay, because the declaration
// th04/main/stage/loop.cpp makes for the call site spells it the same way and
// nothing in the compiler checks that the pair agrees.
extern "C" void pascal near bullets_render(void)
{
	#define left	_AX
	#define top 	_DX

	// [bullet] takes SI by how often it is dereferenced as a base
	// (kb/codegen/0117), in both games. The remaining register goes to
	// whichever of [patnum] and [i] is mentioned more often, and the two dumps
	// come out MIRRORED: TH05 keeps [patnum] in DI and homes [i] on the stack,
	// TH04 the other way round. The cause is the pellet cloud loop below, which
	// TH05 has and TH04 does not — it mentions [patnum] twice more and [i] not
	// at all, which is exactly enough to flip the ranking. So this declaration
	// list needs no `#if`: in TH05 the two tie and kb/codegen/0146's
	// declaration-order rule hands DI to the first-declared [patnum], while in
	// TH04 [i] wins outright on count. Measured off each dump; "first-declared
	// takes the stack slot" does not generalise.
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
			// playfield_encloses*() macros.
			//
			// DIFFERENCE 1/4: in TH05 the result is asymmetric — the top,
			// left and bottom bounds are the playfield's own, but the right
			// one is 16 pixels *past* it, so a cloud can be drawn half a
			// sprite into the HUD. TH04's four bounds are all the
			// playfield's. ZUN quirk, pending an emulator observation.
			if(
				(bullet->pos.cur.y >= 0) &&
				(bullet->pos.cur.y < TO_SP(PLAYFIELD_H)) &&
				(bullet->pos.cur.x >= 0) &&
				#if (GAME == 5)
					(bullet->pos.cur.x < TO_SP(PLAYFIELD_W + 16))
				#else
					(bullet->pos.cur.x < TO_SP(PLAYFIELD_W))
				#endif
			) {
				// DIFFERENCE 2/4: blue for everything blue-ish, red for
				// everything else — but the two games spell "blue-ish"
				// differently, because TH05 renumbered this sprite sheet.
				//
				// TH05's blue set is three contiguous ranges: the
				// non-directional bullets below PAT_BULLET16_N_RED are the
				// blue half of that set, and the two ranges after it are the
				// blue directional and the blue vector bullets. (The upper
				// bound of the last one is PAT_CLOUD_PELLET, i.e. everything
				// up to the end of the vector bullets.)
				//
				// TH04's is three loose patnums and one range, which is why
				// it comes out as a `switch` plus a separate `if` rather than
				// one condition: the dump loads [patnum] into AX once for the
				// three-way compare chain and then re-reads it from memory
				// for the range test, which is what a `switch` statement and
				// a following `if` generate and what a single `||` chain does
				// not. The three are consecutive on TH04's sheet apart from
				// the one hole at PAT_BULLET16_N_BALL_RED, and only the
				// second test can overrule the first.
				#if (GAME == 5)
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
				#else
					switch(bullet->patnum) {
					case PAT_BULLET16_N_OUTLINED_BALL_GREEN:
					case PAT_BULLET16_N_OUTLINED_BALL_BLUE:
					case PAT_BULLET16_N_BALL_BLUE:
						patnum = (PAT_CLOUD_BULLET16_BLUE - 1);
						break;
					default:
						patnum = (PAT_CLOUD_BULLET16_RED - 1);
					}
					if(
						(bullet->patnum >= PAT_BULLET16_D_BLUE) &&
						(bullet->patnum < PAT_BULLET16_D_YELLOW)
					) {
						patnum = (PAT_CLOUD_BULLET16_BLUE - 1);
					}
				#endif
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
		// DIFFERENCE 3/4: TH05 gave pellets a delay cloud of their own and a
		// separate render list for it. TH04 has neither, and goes straight to
		// the two blitters.
		#if (GAME == 5)
			// [bullet] has walked all the way down to the last pellet by now,
			// but this branch never uses it as one — the cloud list holds its
			// own pointers, and the two blitters below read [pellets_render].
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
		#endif

		grcg_setcolor_direct(V_WHITE);
		pellets_render_top();

		// DIFFERENCE 4/4: TH05 made the bottom half's colour a per-stage
		// variable; TH04's is this one hardcoded palette index, and no header
		// in either game gives it a name.
		#if (GAME == 5)
			grcg_setcolor_direct(pellet_bottom_col);
		#else
			grcg_setcolor_direct(9);
		#endif
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

// Restores what th04/main/playfld.cpp had before this file on the TH05 side:
// the command line passes no -G, and both functions after this point need
// `ENTER`. On the TH04 side nothing follows in the object, so this line
// generates nothing — it is kept unconditional so that the bracket cannot be
// half-removed by a later edit to either host.
#pragma option -G-
