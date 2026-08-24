/// Stage 6 Boss - Yuuka, foreground rendering
/// ------------------------------------------
/// (#included from th04/y6_fg.cpp, which owns the Y6_FG_TEXT segment this
/// kb/codegen/0080 carve split off the head of main_012_TEXT. ZUN's object for
/// that segment held these two functions, then shots_add(),
/// th04/main/player/shot_velocity.asm, player_shot_level_update(),
/// elly_fg_render(), and stage_state_reset() -- that an original object held
/// several unrelated sources is kb/codegen/0112. Only shot_velocity_set()
/// remains in the root contribution; the C++ wrappers preserve the original
/// order around it without re-pointing main_012_TEXT.)
///
/// Yuuka is the sixth member of the [boss_fg_render] family to land in C++, and
/// the only one that renders a second, fully independent copy of the boss: the
/// mirror image [yuuka6_25A0C], which her patterns spawn their second half
/// from and which takes its own damage. Everything else she shares with the
/// family -- the PHASE_NONE bail-out, the PHASE_EXPLODE_BIG zoom, the damage
/// flash, and the explosion animations at the end -- except that, like
/// yuuka5_fg_render() (th04/main/boss/b5r.cpp), she also renders the thick
/// lasers, and unlike any of them she owns a per-frame overlay pass over
/// [custom_entities].

#include "platform.h"
#include "pc98.h"
#include "libs/master.lib/pc98_gfx.hpp"
// AFTER pc98_gfx.hpp on purpose: it #undefs master.lib's grcg_off() and
// redefines it as the inline `outportb(0x7C, 0)` the customs pass ends on --
// `mov dx, 7Ch` / `mov al, 0` / `out dx, al`, which is master.lib's
// GRCG_OFF_CLOBBERING macro. master.lib's own grcg_off() is a far call. Same
// order, and for the same reason, as th04/main/boss/b5r.cpp and
// th04/main/bullet/laser_render.cpp.
#include "th01/hardware/grcg.hpp"
#include "th02/v_colors.hpp"
#include "th04/hardware/grcg.hpp"
#include "th04/sprites/main_pat.h"
#include "th04/main/frames.h"
#include "th04/main/bullet/laser_t.hpp"
#include "th04/main/boss/boss.hpp"
// PAT_ENEMY_KILL and ENEMY_KILL_CELS have to be visible first, which is why
// this one comes after th04/sprites/main_pat.h: b6ent.hpp derives
// CCF_KILL_ANIM from both and includes neither.
#include "th04/main/boss/b6ent.hpp"

/// Still ASM
/// ---------
extern "C" {
	// th04_main.asm `.data?`, with no `public` of ZUN's. The first four are
	// shared with th04/main/boss/b6_next.cpp and th04/main/boss/b6_upd.cpp and
	// keep the address-suffixed spellings those two already use; **a naming
	// round is owed** for all of them (kb/codegen/0123).

	// Where Yuuka's mirror image is, mirrored across the playfield's vertical
	// center line by yuuka6_teleport_to() and gated by [yuuka6_25A1B] == 2.
	// The patterns in th04/main/boss/b6_next.cpp spawn their second half from
	// it, and it takes its own shot damage into [yuuka6_25A1E].
	extern PlayfieldPoint yuuka6_25A0C;
	extern unsigned char yuuka6_25A1B;

	// The damage the mirror image took this frame, assigned from
	// shots_hittest() by yuuka6_customs_update() and subtracted from [boss.hp]
	// there. This function is what resets it, exactly the way it resets
	// [boss.damage_this_frame].
	extern unsigned char yuuka6_25A1E;

	// Set once Yuuka's pattern phase 4 is over, and cleared again by
	// yuuka6_return_to_center(); i.e. true for the whole second half of the
	// fight, which is exactly when the animation below is drawn behind her.
	extern unsigned char yuuka6_25A08;

	// The two frame counters the two damage flashes blink off, each one
	// incremented on every frame its subject took damage and never reset.
	// Both are new publications rather than retired ones: this function was
	// their only reader AND their only writer, so the dump has no reference to
	// either left.
	extern unsigned char yuuka6_25A03;
	extern unsigned char yuuka6_25A04;
}
/// ---------

/// Yuuka's sprite
/// --------------
/// [inferred] Only the folded constants -16 and -32 survive into the binary, so
/// the split between "sprite box" and "extra offset" is not recoverable -- the
/// same note orange_fg_render() (th04/main/boss/render.cpp),
/// mugetsu_fg_render() and elly_fg_render() all carry. 96x96 is the reading
/// that needs no leftover in either axis, and it is corroborated: the second
/// half of the sprite pair below is blitted exactly 48 pixels to the right,
/// which is this width halved, and Gengetsu -- the other TH04 boss drawn as a
/// pair of halves -- is 96x96 for the same reason
/// (th04/main/boss/bosses.hpp).
static const pixel_t YUUKA6_W = 96;
static const pixel_t YUUKA6_H = 96;

// The color both damage flashes blit in. NOT V_WHITE, which is what every
// other TH04 boss flashes in and what the chasing crosses below use; this is
// master.lib's GC_R plane, the same device REIMU_AFTERIMAGE_COL
// (th04/main/boss/fg.cpp) uses for its own non-white single-plane blit.
static const vc_t YUUKA6_FLASH_COL = 2;

// Zoom level for the PHASE_EXPLODE_BIG blit -- 3, where super_large_put(), the
// call the other bosses' renderers make, would be a fixed 2. yuuka5_fg_render()
// is the only other TH04 boss renderer that reaches super_zoom() directly.
static const int YUUKA6_EXPLODE_ZOOM = 3;
/// --------------

/// Yuuka's second half
/// -------------------
/// The four-cel animation drawn behind Yuuka, centered in her sprite box, for
/// as long as [yuuka6_25A08] is set.
///
/// [inferred] THE STEM ONLY, and the same withholding th04/main/boss/b3_fg.cpp
/// documents for PAT_ELLY_BOOMERANG: "aura" names where and when this thing is
/// drawn -- behind her, centered, for the whole second half of the fight --
/// not a reading of the artwork, which nothing in the dump ties to an asset.
///
/// [measured] The RANGE, on the other hand, is derived rather than guessed, and
/// it closes exactly. th04/sprites/main_pat.h ends st05.bb7 at
/// PAT_YUUKA6_VANISH_3 = 180 -- a two-patnum sprite, so 180 and 181 -- and
/// opens st05.bb9 at PAT_YUUKA6_CHASECROSS = 186, and that header's own
/// preamble states the invariant that makes the gap readable: the enum's order
/// is the order the files are loaded in. 182...185 is therefore one four-cel
/// single-patnum animation out of the one Stage 6 boss .bb? file the enum has
/// no block for. Four cels also fall straight out of the code, since
/// [stage_frame_mod16] divided by four spans 0...3.
///
/// Kept local to this translation unit rather than opened as a new block in
/// main_pat.h, for the reason PAT_ELLY_BOOMERANG gives: 16 sources and one
/// shared header include that enum directly, and adding a section to it
/// belongs to the parcel that next edits it.
static const int PAT_YUUKA6_AURA = 182;
static const int YUUKA6_AURA_FRAMES_PER_CEL = 4;

// Centered in Yuuka's 96x96 box, which makes the cels 48x48 -- the same size
// as each half of her own sprite.
static const pixel_t YUUKA6_AURA_OFFSET = 24;
/// -------------------

// Yuuka's two [custom_entities] overlays, rendered in one pass: every chasing
// cross, and then the safety circle. Called from yuuka6_fg_render() below, and
// the render-side counterpart of yuuka6_customs_update()
// (th04/main/boss/b6_next.cpp), whose comment explains the pointer this
// function shares between the two halves in the same terms:
//
// ZUN walks [p] across the chasing crosses and then keeps using it for the
// safety circle, which works because the circle occupies the slot immediately
// past the last cross -- so the loop leaves the pointer aimed at it. [circle]
// is a cast of that same pointer rather than a second variable.
//
// Named after that update function. ZUN gave it no `public`, and with
// yuuka6_fg_render() lifted alongside it the dump has no reference to it left
// at all, so this name replaces a placeholder outright rather than aliasing
// one.
static void near yuuka6_customs_render(void)
{
	chasecross_t near *p;
	int i;
	screen_x_t left;
	vram_y_t top;

	#define circle (reinterpret_cast<safetycircle_t near *>(p))

	p = chasecrosses;
	for(i = 0; i < CHASECROSS_COUNT; i++, p++) {
		if(p->flag == CCF_FREE) {
			continue;
		}
		left = p->center.to_screen_left(CHASECROSS_W);
		top = p->center.to_screen_top(CHASECROSS_H);
		if(p->flag == CCF_ALIVE) {
			// The cel expression is spelled out in both branches rather than
			// hoisted into a third local: the frame really is the two
			// coordinates and nothing else. [top] is also pushed out of AX,
			// where the assignment above left it, in the first branch only --
			// that is the one the compiler reaches in a straight line, so its
			// register tracking is still valid there and it needs no source
			// shape. (kb/codegen/0126)
			if(p->damage_this_frame == 0) {
				super_put(left, top, (
					((p->age >> 1) & (YUUKA6_CHASECROSS_CELS - 1)) +
					PAT_YUUKA6_CHASECROSS
				));
			} else {
				super_put_1plane(left, top, (
					((p->age >> 1) & (YUUKA6_CHASECROSS_CELS - 1)) +
					PAT_YUUKA6_CHASECROSS
				), 0, super_plane(V_WHITE));
			}
		} else {
			// The kill animation runs off the flag itself, which doubles as
			// its own frame counter from CCF_KILL_ANIM up: the division turns
			// it back into the PAT_ENEMY_KILL cel it started at.
			super_put(left, top, (p->flag / CHASECROSS_KILL_FRAMES_PER_CEL));
			reinterpret_cast<unsigned char &>(p->flag)++;
			if(p->flag >= CCF_KILL_ANIM_END) {
				p->flag = CCF_FREE;
			}
		}
	}

	if(circle->flag != SCF_FREE) {
		grcg_setmode_rmw();
		grcg_setcolor_direct(YUUKA6_FLASH_COL);
		grcg_circlefill(
			circle->center.x, circle->center.y, circle->radius_filled
		);

		// Only the circle that is no longer growing gets the ring that shows
		// where its edge is headed -- and the GRCG is therefore left on for
		// the growing one, the same asymmetry b5r.cpp's ZUN inconsistency note
		// describes for Yuuka's Stage 5 warp circles.
		if(circle->flag != SCF_GROW) {
			grcg_setcolor_direct(circle->col_ring);
			grcg_circle(circle->center.x, circle->center.y, (
				circle->radius_ring_distance + circle->radius_filled
			));
			grcg_off();
		}
	}

	#undef circle
}

void pascal near yuuka6_fg_render(void)
{
	screen_x_t left;
	vram_y_t top;

	// Computed once, ahead of the phase chain, the way elly_fg_render() and
	// yuuka5_fg_render() also do it: both of the branches that use them use
	// the same pair.
	left = boss.pos.cur.to_screen_left(YUUKA6_W);
	top = boss.pos.cur.to_screen_top(YUUKA6_H);

	// `[measured]` The early `return` is load-bearing, and it is the exact
	// inverse of kb/codegen/0126. Written as a nested
	// `if(boss.phase != PHASE_NONE) { if(... == PHASE_EXPLODE_BIG) { ... } }`,
	// the whole function is byte-exact except that the [top] argument below is
	// pushed out of AX -- where the assignment above left it -- instead of out
	// of the DI the variable lives in. `return` makes Turbo C++ emit a label
	// for the statement after it, which drops the -Z tracking that prefers AX;
	// -O then folds the jump away again, so the dump shows no label at all and
	// the divergence is one byte, `50` against `57`.
	if(boss.phase == PHASE_NONE) {
		return;
	}
	if(boss.phase == PHASE_EXPLODE_BIG) {
		super_zoom(left, top, boss.sprite, YUUKA6_EXPLODE_ZOOM);
	} else {
		if(yuuka6_25A08 != 0) {
			super_put(
				(left + YUUKA6_AURA_OFFSET), (top + YUUKA6_AURA_OFFSET),
				((stage_frame_mod16 / YUUKA6_AURA_FRAMES_PER_CEL) +
					PAT_YUUKA6_AURA)
			);
		}

		// Yuuka is the one TH04 boss whose sprite can be 0 during a
		// regular phase: her two vanishes blank it, and everything from
		// here down to the explosions is skipped while it is.
		if(boss.sprite != 0) {
			// Blinks the flash off on every other damaged frame instead of
			// showing it on all of them, which is what the counter is for.
			if((boss.damage_this_frame == 0) || (yuuka6_25A03 & 1)) {
				super_put(left, top, boss.sprite);
				super_put(
					(left + (YUUKA6_W / 2)), top, (boss.sprite + 1)
				);
			} else {
				super_put_1plane(
					left, top, boss.sprite, 0,
					super_plane(YUUKA6_FLASH_COL)
				);
				super_put_1plane(
					(left + (YUUKA6_W / 2)), top, (boss.sprite + 1), 0,
					super_plane(YUUKA6_FLASH_COL)
				);
			}
			yuuka6_25A03 += (boss.damage_this_frame != 0);
			boss.damage_this_frame = 0;

			// The mirror image, drawn from the same [boss.sprite] pair and
			// flashed off its own counter. Only [yuuka6_25A1B] == 2 says
			// that it exists; yuuka6_teleport_to()
			// (th04/main/boss/b6_next.cpp) is what sets that.
			if(yuuka6_25A1B == 2) {
				left = yuuka6_25A0C.to_screen_left(YUUKA6_W);
				top = yuuka6_25A0C.to_screen_top(YUUKA6_H);
				if((yuuka6_25A1E == 0) || (yuuka6_25A04 & 1)) {
					super_put(left, top, boss.sprite);
					super_put(
						(left + (YUUKA6_W / 2)), top, (boss.sprite + 1)
					);
				} else {
					super_put_1plane(
						left, top, boss.sprite, 0,
						super_plane(YUUKA6_FLASH_COL)
					);
					super_put_1plane(
						(left + (YUUKA6_W / 2)), top, (boss.sprite + 1), 0,
						super_plane(YUUKA6_FLASH_COL)
					);
				}
				yuuka6_25A04 += (yuuka6_25A1E != 0);
				yuuka6_25A1E = 0;
			}
		}
		explosions_small_update_and_render();
		explosions_big_update_and_render();
		thicklasers_render();
		yuuka6_customs_render();
	}
}
