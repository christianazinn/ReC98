/// Background rendering code for TH04's bosses
/// -------------------------------------------
/// ZUN's object for this code segment held every one of TH04's boss
/// background renderers, plus the two Stage 6 background shape helpers wedged
/// in between and Elly's tile invalidator (kb/codegen/0112). **All of them
/// are C++ now**, in ZUN's own address order, and th04_main.asm's
/// `BOSS_BG_TEXT` contribution is empty — this object owns the whole segment.
/// The order below is ZUN's, which is Stage 4, 2, 3, 1, 5, 6, Extra, and not
/// stage order.
///
/// TH05's counterpart is th05/main/boss/render.cpp, which holds that game's
/// five macro-shaped renderers and its two hand-rolled ones.

#include "platform.h"
#include "pc98.h"
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th03/formats/cdg.h"
#include "th03/hardware/palette.hpp"
#include "th04/hardware/grcg.hpp"
#include "th04/formats/bb.h"
#include "th04/formats/super.h"
#include "th04/math/randring.hpp"
#include "th04/math/vector.hpp"
#include "th04/main/checkerb.hpp"
#include "th04/main/null.hpp"
#include "th04/main/stage/stage.hpp"
#include "th04/main/boss/boss.hpp"
#include "th04/main/boss/bosses.hpp"
#include "th04/main/boss/backdrop.hpp"
#include "th04/main/boss/impl.hpp"
#include "th04/main/boss/b6.cpp"
#include "th04/main/tile/tile.hpp"
#include "th04/main/tile/bb.hpp"
#include "th04/sprites/main_cdg.h"

// playfield_fill() fills the playfield with the current GRCG tile register,
// assuming TDW mode; unlike TH05's boss_bg_fill_col_0() it neither enables
// nor disables the GRCG, both being the caller's job. th04/main/checkerb.cpp
// now, kept in main_013_TEXT by a #pragma codeseg block and declared HERE
// rather than in th04/main/checkerb.hpp, whose inclusion at the head of that
// TU would bind it to CHECKERB_TEXT and defeat the pragma (kb/codegen 0155).
extern "C" void near playfield_fill(void);

/// Still ASM
/// ---------
// See th04/main/tile/tile.hpp for the reason why this declaration is
// necessary. elly_invalidate() below is an SPPoint caller.
extern "C" void pascal near tiles_invalidate_around(const SPPoint center);

// Elly's single thrown entity and the byte that tracks it, which
// elly_invalidate() below is the second C++ reader of. Both are private
// th04_main.asm `.data?` labels behind zero-byte `label` aliases
// (kb/codegen/0123); th04/main/boss/b3_fg.cpp documents the flag's three
// states.
extern "C" unsigned char elly_boomerang_flag;
extern "C" PlayfieldMotion elly_boomerang_pos;

// That state machine's variables, reset in yuuka6_bg_render() and written
// nowhere else outside it. [yuuka6_bg_state] is the state index, 0…0x11, and
// bounds two `cs:` jump tables (`ja` at 0x0C, `jb` at 0x11);
// [yuuka6_bg_state_frame] is its per-state frame counter, which doubles as the
// fade ramp — below 0x80 it is used directly, at or above it as `255 - it`,
// giving a triangle wave. Both are th04_main.asm `.data?` labels with no
// `public` of ZUN's, so they needed a zero-byte `label` alias to become
// linkable (kb/codegen/0123).
extern "C" unsigned char yuuka6_bg_state;
extern "C" unsigned char yuuka6_bg_state_frame;

// The one-shot latch that ends the final fade to blue. Set once the ramp above
// first reaches its 0x7F peak in state 0x10 or 0x11, and never cleared — so
// the Phase 6 background's color 0 stays where the explosion left it.
// th04_main.asm `.data`, and reached from nothing but the state machine, so it
// needed the same zero-byte alias (kb/codegen/0123).
//
// [inferred] name: a naming round is owed for it, on the same terms as the
// three in state/notes/_yuuka6_bg_render_qv.md. The binary shows a byte that
// is compared against 0, written 1 exactly once, and gates nothing but the
// three Palettes[0] stores under it.
extern "C" bool yuuka6_bg_fade_done;
/// ---------

// Same value as TH05's ENTRANCE_BB_FRAMES_PER_CEL
// (th05/main/boss/bosses.hpp), which TH04 has no header constant for because
// these are the only TH04 functions that have needed it so far.
static const int ENTRANCE_BB_FRAMES_PER_CEL = 4;


/// Stages 1 – 4
/// ------------
// The four bosses below all transition into their backdrop through a .BB
// animation whose cel index is `boss.phase_frame >> 1` — an arithmetic SHIFT,
// not the `/ ENTRANCE_BB_TRANSITION_FRAMES_PER_CEL` division that
// th04/main/boss/impl.hpp's macro and TH05's sara_bg_render() spell. Borland
// C++ 4.0 strength-reduces a signed `/ 2` to a three-instruction,
// sign-corrected sequence; every one of these functions has only the direct
// arithmetic right shift. `[verified: ASM]`, and see kb/codegen/0134 for the
// same distinction one factor up.
static const int ENTRANCE_BB_TRANSITION_CEL_SHIFT = 1;

/// Stage 4 — Orange
/// ----------------
// Position of ST04BK.CDG within the playfield.
static const screen_x_t ORANGE_BACKDROP_LEFT = 32;
static const vram_y_t ORANGE_BACKDROP_TOP = 136;

// boss_bg_render_entrance_bb_transition_and_backdrop()
// (th04/main/boss/impl.hpp) written out longhand, for the reasons
// yuuka5_bg_render() below gives: the macro spells the phase constant
// `PHASE_BOSS_EXPLODE_BIG`, which th04/main/phase.hpp only defines under
// `#if (GAME == 5)`, and it divides where this game shifts.
//
// The HP-fill arm is the only one that differs from the macro: while Orange's
// HP bar fills up, the tiles are rendered to both pages unconditionally for
// the first 192 frames.
void pascal near orange_bg_render(void)
{
	if(boss.phase == PHASE_HP_FILL) {
		if(boss.phase_frame >= 192) {
			tiles_render_all();
		} else {
			tiles_render_after_custom(boss.phase_frame);
		}
	} else if(boss.phase == PHASE_BOSS_ENTRANCE_BB) {
		boss_backdrop_render(ORANGE_BACKDROP_LEFT, ORANGE_BACKDROP_TOP, 1);
		tiles_bb_invalidate(
			bb_boss_seg, (boss.phase_frame >> ENTRANCE_BB_TRANSITION_CEL_SHIFT)
		);
		tiles_redraw_invalidated();
	} else if(boss.phase < PHASE_EXPLODE_BIG) {
		boss_backdrop_render(ORANGE_BACKDROP_LEFT, ORANGE_BACKDROP_TOP, 1);
	} else if(boss.phase == PHASE_EXPLODE_BIG) {
		tiles_render_all();
	} else /* if(boss.phase == PHASE_NONE) */ {
		tiles_render_after_custom(boss.phase_frame);
	}
}
/// ----------------

/// Stage 2 — Kurumi
/// ----------------
// Position of ST02BK.CDG within the playfield.
static const screen_x_t KURUMI_BACKDROP_LEFT = 32;
static const vram_y_t KURUMI_BACKDROP_TOP = 96;

// The same shape as orange_bg_render() above, with the plain
// tiles_render_all() that the macro's `on_hp_fill` parameter was made for.
void pascal near kurumi_bg_render(void)
{
	if(boss.phase == PHASE_HP_FILL) {
		tiles_render_all();
	} else if(boss.phase == PHASE_BOSS_ENTRANCE_BB) {
		boss_backdrop_render(KURUMI_BACKDROP_LEFT, KURUMI_BACKDROP_TOP, 0);
		tiles_bb_invalidate(
			bb_boss_seg, (boss.phase_frame >> ENTRANCE_BB_TRANSITION_CEL_SHIFT)
		);
		tiles_redraw_invalidated();
	} else if(boss.phase < PHASE_EXPLODE_BIG) {
		boss_backdrop_render(KURUMI_BACKDROP_LEFT, KURUMI_BACKDROP_TOP, 0);
	} else if(boss.phase == PHASE_EXPLODE_BIG) {
		tiles_render_all();
	} else /* if(boss.phase == PHASE_NONE) */ {
		tiles_render_after_custom(boss.phase_frame);
	}
}
/// ----------------

/// Stage 3 — Elly
/// --------------
// Position of ST03BK.CDG within the playfield.
static const screen_x_t ELLY_BACKDROP_LEFT = 32;
static const vram_y_t ELLY_BACKDROP_TOP = 16;

// Elly and her boomerang are the only entities drawn on top of the stage
// tiles before the backdrop appears, so the tile-based background needs their
// previous positions invalidated by hand on the frames where elly_bg_render()
// below renders tiles at all. [elly_boomerang_flag] is tested against 0 and
// not against EBF_THROWN, which is what buys EBF_CAUGHT its one extra
// invalidation frame (th04/main/boss/b3_fg.cpp).
//
// The box is 64×64 screen pixels for both, wider and taller than either
// sprite.
static const pixel_t ELLY_INVALIDATE_W = 64;
static const pixel_t ELLY_INVALIDATE_H = 64;

void pascal near elly_invalidate(void)
{
	tile_invalidate_box.x = ELLY_INVALIDATE_W;
	tile_invalidate_box.y = ELLY_INVALIDATE_H;
	tiles_invalidate_around(boss.pos.prev);
	if(elly_boomerang_flag) {
		tiles_invalidate_around(elly_boomerang_pos.prev);
	}
}

// Same shape as the two above, with two quirks of its own:
//
// 1) The .BB transition runs during phase 2 rather than during
//    PHASE_BOSS_ENTRANCE_BB, which is why phases 0 and 1 share one arm here.
//    `[verified: ASM]`: the original compares against PHASE_BOSS_ENTRANCE_BB
//    with `ja` and then against a bare 2.
// 2) That shared arm is tiles_render_after_custom() with elly_invalidate()
//    wedged into its `else` branch rather than a call to the inline function
//    — which is also why its two halves tail-merge into this function's
//    shared tiles_render_all() and tiles_render() exits.
void pascal near elly_bg_render(void)
{
	if(boss.phase <= PHASE_BOSS_ENTRANCE_BB) {
		if(boss.phase_frame <= 2) {
			tiles_render_all();
		} else {
			elly_invalidate();
			tiles_render();
		}
	} else if(boss.phase == 2) {
		boss_backdrop_render(ELLY_BACKDROP_LEFT, ELLY_BACKDROP_TOP, 0);
		tiles_bb_invalidate(
			bb_boss_seg, (boss.phase_frame >> ENTRANCE_BB_TRANSITION_CEL_SHIFT)
		);
		tiles_redraw_invalidated();
	} else if(boss.phase < PHASE_EXPLODE_BIG) {
		boss_backdrop_render(ELLY_BACKDROP_LEFT, ELLY_BACKDROP_TOP, 0);
	} else if(boss.phase == PHASE_EXPLODE_BIG) {
		tiles_render_all();
	} else /* if(boss.phase == PHASE_NONE) */ {
		tiles_render_after_custom(boss.phase_frame);
	}
}
/// --------------

/// Stage 1 — Reimu and Marisa
/// --------------------------
// Position of ST01BK.CDG within the playfield.
static const screen_x_t REIMU_MARISA_BACKDROP_LEFT = 96;
static const vram_y_t REIMU_MARISA_BACKDROP_TOP = 72;

// Twice as slow as every other boss's, and the only .BB entrance in TH04 that
// is not ENTRANCE_BB_FRAMES_PER_CEL. This remains a signed 16-bit division by
// 8 in the original, exactly like yuuka5_bg_render()'s division by 4.
static const int REIMU_MARISA_ENTRANCE_BB_FRAMES_PER_CEL = 8;

// boss_bg_render_entrance_bb_opaque_and_backdrop() written out longhand, for
// the same two reasons yuuka5_bg_render() gives — the two are otherwise the
// same code with different constants.
void pascal near reimu_marisa_bg_render(void)
{
	if(boss.phase == PHASE_HP_FILL) {
		tiles_render_after_custom(boss.phase_frame);
	} else if(boss.phase == PHASE_BOSS_ENTRANCE_BB) {
		unsigned char entrance_cel = (
			boss.phase_frame / REIMU_MARISA_ENTRANCE_BB_FRAMES_PER_CEL
		);
		if(entrance_cel < (TILES_BB_CELS / 2)) {
			tiles_render_all();
		} else {
			// A copy of boss_backdrop_render()…
			grcg_setmode_tdw();
			grcg_setcolor_direct(1);
			// … that probably predated [boss_backdrop_colorfill]?
			reimu_marisa_backdrop_colorfill();
			grcg_off();

			cdg_put_noalpha_8(
				REIMU_MARISA_BACKDROP_LEFT,
				REIMU_MARISA_BACKDROP_TOP,
				CDG_BG_BOSS
			);
		}
		tiles_bb_put(bb_boss_seg, entrance_cel);
	} else if(boss.phase < PHASE_EXPLODE_BIG) {
		boss_backdrop_render(
			REIMU_MARISA_BACKDROP_LEFT, REIMU_MARISA_BACKDROP_TOP, 1
		);
	} else if(boss.phase == PHASE_EXPLODE_BIG) {
		tiles_render_all();
	} else /* if(boss.phase == PHASE_NONE) */ {
		tiles_render_after_custom(boss.phase_frame);
	}
}
/// --------------------------
/// Stage 5 — Yuuka
/// ---------------
// Position of ST05BK.CDG within the playfield.
static const screen_x_t YUUKA5_BACKDROP_LEFT = 128;
static const vram_y_t YUUKA5_BACKDROP_TOP = 128;

// boss_bg_render_entrance_bb_opaque_and_backdrop() (th04/main/boss/impl.hpp)
// written out longhand, for the two reasons mugetsu_gengetsu_bg_render() at
// the bottom of this file gives at length: the macro emits a
// `tiles_bb_col = bb_col;` that no TH04 boss background renderer has, and it
// spells the phase constant `PHASE_BOSS_EXPLODE_BIG`, which
// th04/main/phase.hpp only defines under `#if (GAME == 5)`. Everything else
// here IS the macro, unchanged, including the shared
// tiles_render_after_custom() in the HP-fill and PHASE_NONE arms -- which is
// why the original tail-merges the two.
void pascal near yuuka5_bg_render(void)
{
	if(boss.phase == PHASE_HP_FILL) {
		tiles_render_after_custom(boss.phase_frame);
	} else if(boss.phase == PHASE_BOSS_ENTRANCE_BB) {
		unsigned char entrance_cel = (
			boss.phase_frame / ENTRANCE_BB_FRAMES_PER_CEL
		);
		if(entrance_cel < (TILES_BB_CELS / 2)) {
			tiles_render_all();
		} else {
			// A copy of boss_backdrop_render()…
			grcg_setmode_tdw();
			grcg_setcolor_direct(0);
			// … that probably predated [boss_backdrop_colorfill]?
			yuuka5_backdrop_colorfill();
			grcg_off();

			cdg_put_noalpha_8(
				YUUKA5_BACKDROP_LEFT, YUUKA5_BACKDROP_TOP, CDG_BG_BOSS
			);
		}
		tiles_bb_put(bb_boss_seg, entrance_cel);
	} else if(boss.phase < PHASE_EXPLODE_BIG) {
		boss_backdrop_render(YUUKA5_BACKDROP_LEFT, YUUKA5_BACKDROP_TOP, 0);
	} else if(boss.phase == PHASE_EXPLODE_BIG) {
		tiles_render_all();
	} else /* if(boss.phase == PHASE_NONE) */ {
		tiles_render_after_custom(boss.phase_frame);
	}
}
/// ---------------

/// Stage 6 — Yuuka
/// ---------------
// The two implementations [bg_shape_clip] is ever pointed at, in the order
// yuuka6_bg_update_and_render() below points at them. `_respawn_in_cen` is the
// whole name and not a truncation of the source's: `-i32` cuts an identifier
// before it is mangled (kb/codegen/0060), so this is the only spelling that
// reaches the linker and the only one the dump's `public` ever carried.
//
// They also give this object an EVEN-length initialized prefix ahead of the
// state machine, which is what makes that function's `-a2` pad land in the
// object at all. See the comment on the `#pragma option -a2` below.

// Off the bottom edge, off the top, or off either side: back to the middle of
// the playfield at the flyout speed, which is what makes the shapes stream
// outwards from Yuuka in the phases that use this one.
void pascal near bg_shape_clip_and_respawn_in_cen(bg_shape_t near &shape)
{
	shape.speed.v++;
	if(
		(shape.pos.x.v <= TO_SP(-(BG_SHAPE_W / 2))) ||
		(shape.pos.x.v >= TO_SP(PLAYFIELD_W + (BG_SHAPE_W / 2))) ||
		(shape.pos.y.v <= TO_SP(-(BG_SHAPE_H / 2))) ||

		// ZUN quirk: Not halved, where the other three bounds are half a
		// sprite. Same shape as the one in th04/main/player/bombanim.cpp.
		(shape.pos.y.v >= TO_SP(PLAYFIELD_H + BG_SHAPE_H))
	) {
		shape.pos.x.v = TO_SP(PLAYFIELD_W / 2);
		shape.pos.y.v = TO_SP(PLAYFIELD_H / 2);
		shape.speed.v = bg_shape_flyout_speed.v;
	}
}

// Wraps around instead, and keeps the shape's speed.
void pascal near bg_shape_clip_and_wrap(bg_shape_t near &shape)
{
	if(shape.pos.x.v <= TO_SP(-(BG_SHAPE_W / 2))) {
		shape.pos.x.v += TO_SP(PLAYFIELD_W + BG_SHAPE_W);
	} else if(shape.pos.x.v >= TO_SP(PLAYFIELD_W + (BG_SHAPE_W / 2))) {
		shape.pos.x.v -= TO_SP(PLAYFIELD_W + BG_SHAPE_W);
	}
	if(shape.pos.y.v <= TO_SP(-(BG_SHAPE_H / 2))) {
		// ZUN quirk? One and a half sprites, where the X axis wraps by a
		// whole playfield plus one sprite. Both Y branches agree with each
		// other, so a shape that leaves through the top comes back at a
		// different height than one that leaves through the side.
		shape.pos.y.v += TO_SP(PLAYFIELD_H + ((BG_SHAPE_H / 2) * 3));
	} else if(shape.pos.y.v >= TO_SP(PLAYFIELD_H + BG_SHAPE_H)) {
		shape.pos.y.v -= TO_SP(PLAYFIELD_H + ((BG_SHAPE_H / 2) * 3));
	}
}

// Yuuka's Phase 6 background, and the only thing that draws it during the
// fight: an 18-state machine that ramps palette color 0, re-seeds and re-aims
// all [bg_shapes] and swaps [bg_shape_clip] on every state change, then
// advances each shape and blits it as a 16x16 mono sprite.
//
// [inferred] name: it advances this subsystem's state for the frame and then
// draws it, which is what the tree's other `_update_and_render` symbols mean,
// and every global it touches is already `bg_shape_*`. A naming round is owed
// for it and for the state variables above; the evidence is in
// state/notes/_yuuka6_bg_render_qv.md.
//
// The state index selects the transition rule and [boss.phase] decides whether
// that rule fires: every state either waits for its phase window and then
// jumps forward, re-seeding the shapes for the new pattern, or ticks the index
// one step up or down between two neighboring states while it waits. Both
// `switch`es are dense `cs:` jump tables -- the first over 0...0x0C, the
// second over 0...0x10 -- which is why lifting this function moves the
// trailing table run out of the dump with it.
//
// `#pragma option -a2` for the one padding byte the first of those two tables
// carries in the original, and it only works because of the two functions
// above. `[measured]` `-a2` alone spells the pad `db 1 dup (?)` in a `tcc -S`
// listing and then emits NOTHING for it into the object -- an uninitialized
// pad is a listing artifact, and the object is a byte short with every
// instruction still identical. With initialized code ahead of the function in
// the same object it becomes `db 1 dup (0)` and reaches the binary. The two
// clip functions are 0x36 + 0x3A = 0x70 bytes, EVEN, so they supply that
// prefix without moving either table's parity.
//
// kb/codegen/0157 then covers the rest: the rule runs over both tables against
// a running offset, so the pad the first one takes flips the second one's
// parity and the second is emitted unpadded, which is exactly the original.
#pragma option -a2
extern "C" void near yuuka6_bg_update_and_render(void)
{
	screen_x_t left;
	int patnum;
	subpixel_t vector_x;
	subpixel_t vector_y;
	unsigned char v;
	bg_shape_t near *shape;
	int i;

	if(yuuka6_bg_state_frame == 0) {
		shape = bg_shapes; // ZUN bloat
		switch(yuuka6_bg_state) {
		case 0x0: case 0x8: case 0xC:
			bg_shape_clip = bg_shape_clip_and_wrap;
			break;
		case 0x6: case 0xA:
			bg_shape_clip = bg_shape_clip_and_respawn_in_cen;
			break;
		}
	}
	grcg_setmode_rmw();

	// Triangle wave over the state's frame counter.
	if(yuuka6_bg_state_frame < 0x80) {
		v = yuuka6_bg_state_frame;
	} else {
		v = (255 - yuuka6_bg_state_frame);
	}
	if(yuuka6_bg_state < 0x10) {
		grcg_setcolor_direct(8);
		Palettes[0].c.b = ((yuuka6_bg_state & 1)
			? (Palettes[0].c.r = v)
			: ((v * 3) / 2)
		);
	} else {
		grcg_setcolor_direct(9);
		if(!yuuka6_bg_fade_done) {
			Palettes[0].c.r = 0;
			Palettes[0].c.g = 0;
			Palettes[0].c.b = ((v * 3) / 2);
			if(v >= 0x7F) {
				yuuka6_bg_fade_done = true;
			}
		}
	}
	palette_changed = true;

	switch(yuuka6_bg_state) {
	case 0x0: case 0x1: case 0x2: case 0x3:
		if(boss.phase > 2) {
			yuuka6_bg_state_frame++;
			if(yuuka6_bg_state_frame >= 254) {
				bg_shape_patnum = static_cast<main_patnum_t>(120);
				yuuka6_bg_state = 0x4;
				shape = bg_shapes;
				for(i = 0; i < BG_SHAPE_COUNT; i++, shape++) {
					shape->angle = 0x40;
					shape->speed.v = TO_SP(4);
				}
				yuuka6_bg_state_frame = 255;
				bg_shape_flyout_speed.v = TO_SP(4);
			}
		} else if(yuuka6_bg_state_frame == 255) {
			bg_shape_patnum++;
			yuuka6_bg_state++;
			if(yuuka6_bg_state >= 0x4) {
				yuuka6_bg_state = 0x0;
				bg_shape_patnum = static_cast<main_patnum_t>(120);
			}
			shape = bg_shapes;
			for(i = 0; i < BG_SHAPE_COUNT; i++, shape++) {
				shape->angle = (0x80 - shape->angle);
			}
		}
		break;

	case 0x4: case 0x5:
		if(boss.phase > 4) {
			yuuka6_bg_state_frame++;
			if(yuuka6_bg_state_frame >= 254) {
				bg_shape_patnum = static_cast<main_patnum_t>(120);
				yuuka6_bg_state = 0x6;
				shape = bg_shapes;
				for(i = 0; i < BG_SHAPE_COUNT; i++, shape++) {
					shape->angle = iatan2(
						(shape->pos.y.v - TO_SP(
							(PLAYFIELD_H / 2) + (BG_SHAPE_H / 2)
						)),
						(shape->pos.x.v - TO_SP(
							(PLAYFIELD_W / 2) + (BG_SHAPE_W / 2)
						))
					);
					shape->speed.v = TO_SP(1);
				}
				bg_shape_flyout_speed.v = TO_SP(1);
				yuuka6_bg_state_frame = 255;
			}
		} else if(yuuka6_bg_state_frame == 255) {
			if(yuuka6_bg_state == 0x4) {
				yuuka6_bg_state++;
			} else {
				yuuka6_bg_state--;
			}
		}
		break;

	case 0x6: case 0x7: case 0xA: case 0xB:
		if(
			(boss.phase == 7) || (boss.phase == 8) ||
			(boss.phase == 11) || (boss.phase == 12)
		) {
			yuuka6_bg_state_frame++;
			if(yuuka6_bg_state_frame >= 254) {
				bg_shape_patnum = static_cast<main_patnum_t>(120);
				yuuka6_bg_state = ((yuuka6_bg_state < 0xA) ? 0x8 : 0xC);
				shape = bg_shapes;
				for(i = 0; i < BG_SHAPE_COUNT; i++, shape++) {
					shape->pos.x.v = randring1_next16_mod(TO_SP(PLAYFIELD_W));
					shape->pos.y.v = randring1_next16_mod(TO_SP(PLAYFIELD_H));

					// kb/codegen/0032, the same shape as
					// th04/main/player/bombanim.cpp's: the original adds the
					// constant to the returned byte in AL and then stores it,
					// where a plain `and(0xF) - 0x48` widens to a 16-bit
					// `add ax, 0FFB8h`.
					_AL = randring1_next16_and(0xF);
					_AL += -0x48;
					shape->angle = _AL;

					shape->speed.v = 0x48;
				}
				bg_shape_flyout_speed.v = TO_SP(4);
				yuuka6_bg_state_frame = 255;
			}
		} else if(yuuka6_bg_state_frame == 255) {
			if(yuuka6_bg_state & 1) {
				yuuka6_bg_state--;
			} else {
				yuuka6_bg_state++;
			}
		}
		break;

	case 0x8: case 0x9: case 0xC: case 0xD:
		if((boss.phase == 9) || (boss.phase == 10) || (boss.phase >= 13)) {
			yuuka6_bg_state_frame++;
			if(yuuka6_bg_state_frame >= 254) {
				bg_shape_patnum = static_cast<main_patnum_t>(120);
				yuuka6_bg_state = ((yuuka6_bg_state < 0xC) ? 0xA : 0xE);
				shape = bg_shapes;
				for(i = 0; i < BG_SHAPE_COUNT; i++, shape++) {
					shape->angle = iatan2(
						(shape->pos.y.v - TO_SP(
							(PLAYFIELD_H / 2) + (BG_SHAPE_H / 2)
						)),
						(shape->pos.x.v - TO_SP(
							(PLAYFIELD_W / 2) + (BG_SHAPE_W / 2)
						))
					);
					if(yuuka6_bg_state != 0xE) {
						shape->speed.v = TO_SP(1);
					} else {
						shape->speed.v = TO_SP(4);
					}
				}
				bg_shape_flyout_speed.v = TO_SP(4);
				yuuka6_bg_state_frame = 255;
			}
		} else if(yuuka6_bg_state_frame == 255) {
			if(yuuka6_bg_state & 1) {
				yuuka6_bg_state--;
			} else {
				yuuka6_bg_state++;
			}
		}
		break;

	case 0xE: case 0xF:
		shape = bg_shapes;
		v = ((yuuka6_bg_state == 0xE) ? 2 : -2);
		for(i = 0; i < BG_SHAPE_COUNT; i++, shape++) {
			shape->angle += v;
		}
		if(boss.phase >= 15) {
			yuuka6_bg_state_frame++;
			if(yuuka6_bg_state_frame >= 254) {
				bg_shape_patnum = static_cast<main_patnum_t>(124);
				yuuka6_bg_state = 0x10;
				yuuka6_bg_state_frame = 255;
				shape = bg_shapes;
				for(i = 0; i < BG_SHAPE_COUNT; i++, shape++) {
					shape->pos.x.v = randring1_next16_mod(TO_SP(PLAYFIELD_W));
					shape->pos.y.v = randring1_next16_mod(TO_SP(PLAYFIELD_H));
					shape->angle = 0x40;
					shape->speed.v = TO_SP(12);
				}
				bg_shape_flyout_speed.v = TO_SP(12);
			}
		} else if(yuuka6_bg_state_frame == 255) {
			if(yuuka6_bg_state & 1) {
				yuuka6_bg_state--;
			} else {
				yuuka6_bg_state++;
			}
		}
		break;

	case 0x10:
		if(boss.phase >= PHASE_EXPLODE_BIG) {
			bg_shape_patnum = static_cast<main_patnum_t>(125);
			yuuka6_bg_state = 0x11;
			yuuka6_bg_state_frame = 255;
			shape = bg_shapes;
			for(i = 0; i < BG_SHAPE_COUNT; i++, shape++) {
				shape->angle = 0x40;
				shape->speed.v = TO_SP(1);
			}
			bg_shape_flyout_speed.v = TO_SP(1);
		}
		break;
	}

	yuuka6_bg_state_frame++;
	shape = bg_shapes;
	for(i = 0; i < BG_SHAPE_COUNT; i++, shape++) {
		vector2(vector_x, vector_y, shape->angle, shape->speed.v);
		shape->pos.x.v += vector_x;
		shape->pos.y.v += vector_y;
		bg_shape_clip(*shape);
	}

	// th04/formats/super.h: ES is the plane, and the GRCG was left in RMW mode
	// at the top of this function.
	_ES = SEG_PLANE_B;
	shape = bg_shapes;
	for(i = 0; i < BG_SHAPE_COUNT; i++, shape++) {
		patnum = bg_shape_patnum;

		// The two explosion states spread the shapes over three consecutive
		// sprites instead of showing one.
		if(yuuka6_bg_state >= 0x11) {
			patnum += (i % 3);
		}
		left = (shape->pos.x.to_pixel() + (PLAYFIELD_LEFT - (BG_SHAPE_W / 2)));
		z_super_put_16x16_mono(
			left,
			(shape->pos.y.to_pixel() + (PLAYFIELD_TOP - (BG_SHAPE_H / 2))),
			patnum
		);
	}
	grcg_off();
}
#pragma option -a1

// Nothing in Phase 6's background is made of tiles or of a backdrop image, so
// this one shares none of the impl.hpp macros' body — only their phase chain.
// Three structural differences are worth naming, because each is why a macro
// could not have been used:
//
// 1) grcg_setmode_tdw() is hoisted ABOVE the phase test and runs once for
//    every arm. The macros only ever set TDW inside their entrance branch.
// 2) There are FOUR arms, not five: the `>= PHASE_EXPLODE_BIG` arm covers
//    both PHASE_EXPLODE_BIG and PHASE_NONE, and there is no
//    tiles_render_after_custom() at all.
// 3) The last two arms share a trailing yuuka6_bg_update_and_render() call,
//    and the HP-fill arm carries the one-shot shape re-seed. Neither has a
//    macro slot.
void pascal near yuuka6_bg_render(void)
{
	grcg_setmode_tdw();
	if(boss.phase == PHASE_HP_FILL) {
		grcg_setcolor_direct(1);
		playfield_fill();
		grcg_off();

		// Frame 2 rather than frame 0: boss_reset() runs on frame 0, and the
		// HP fill is the only phase long enough for a one-shot to be safe
		// here. [inferred] — the binary only shows the comparison.
		if(boss.phase_frame == 2) {
			bg_shape_t near *shape = bg_shapes;
			int i;

			// kb/codegen/0003's near-pointer iterator, with one addition: the
			// two increments are emitted in SOURCE order, so `shape++` has to
			// stay in the for-increment expression after `i++`. Written as a
			// trailing statement in the body instead, it becomes `ADD SI, 6` /
			// `INC DI` — the same two instructions, swapped, and the only
			// thing that separated a 2/70 diff from IDENTICAL.
			for(i = 0; i < BG_SHAPE_COUNT; i++, shape++) {
				shape->pos.x.v = randring1_next16_mod(TO_SP(PLAYFIELD_W));
				shape->pos.y.v = randring1_next16_mod(TO_SP(PLAYFIELD_H));
				shape->angle = 0x60;
				shape->speed.v = TO_SP(1);
			}
			bg_shape_flyout_speed.v = TO_SP(1);

			// Not one of the PAT_* constants: 120 is below PAT_STAGE, in the
			// range th04/sprites/main_pat.h leaves unnamed.
			bg_shape_patnum = static_cast<main_patnum_t>(120);

			yuuka6_bg_state = 0;
			yuuka6_bg_state_frame = 0;
		}
	} else if(boss.phase == PHASE_BOSS_ENTRANCE_BB) {
		unsigned char entrance_cel = (
			boss.phase_frame / ENTRANCE_BB_FRAMES_PER_CEL
		);
		grcg_setcolor_direct(1);
		if(entrance_cel < (TILES_BB_CELS / 2)) {
			playfield_fill();
		} else {
			playfield_checkerboard_grcg_tdw_update_and_render();
		}
		tiles_bb_put(bb_boss_seg, entrance_cel);
	} else {
		if(boss.phase < PHASE_EXPLODE_BIG) {
			playfield_checkerboard_grcg_tdw_update_and_render();
		} else {
			grcg_setcolor_direct(1);
			playfield_fill();
			grcg_off();
		}
		yuuka6_bg_update_and_render();
	}
}
/// ---------------

/// Extra Stage backdrop
/// --------------------
// Mugetsu and Gengetsu share one background, so they also share this one
// renderer; [boss_bg_render] is pointed at it by both stagex_setup() and the
// Gengetsu transition. Position of ST06BK.CDG within the playfield.
static const screen_x_t MUGETSU_GENGETSU_BACKDROP_LEFT = 32;
static const vram_y_t MUGETSU_GENGETSU_BACKDROP_TOP = 16;
/// --------------------

// This is boss_bg_render_entrance_bb_opaque_and_backdrop()
// (th04/main/boss/impl.hpp) with three documented deviations, written out
// because none of the three can be expressed through that macro:
//
// 1) The macro's [bb_col] parameter emits `tiles_bb_col = bb_col;` before the
//    .BB blit. NO TH04 boss background renderer does that — TH04 sets
//    [tiles_bb_col] once at boss setup time instead, and TH05 is the game
//    that moved the write into the renderer. So the assignment is absent
//    here, and the macro cannot express its absence.
// 2) The macro spells the phase constant `PHASE_BOSS_EXPLODE_BIG`, which
//    th04/main/phase.hpp only defines under `#if (GAME == 5)`. TH04's name
//    for the same value is `PHASE_EXPLODE_BIG`.
// 3) The HP-fill arm is not one statement. It disables the stage's own
//    renderer on the same frames it redraws every tile, and shares its
//    tiles_render_all() call with the PHASE_EXPLODE_BIG arm below — a single
//    `boss.phase_frame <= 2` test, not the two that
//    tiles_render_after_custom() plus a separate `if` would produce.
//
// Everything else — the phase chain and its order, the `unsigned char` cel
// counter, the `< (TILES_BB_CELS / 2)` half-animation split, and the inlined
// copy of boss_backdrop_render() with its arguments in the opposite order —
// is the macro, unchanged.
void pascal near mugetsu_gengetsu_bg_render(void)
{
	if(boss.phase == PHASE_HP_FILL) {
		// The Extra Stage has no scrolling background of its own, so the
		// stage renderer is retired for good as soon as the boss appears.
		if(boss.phase_frame <= 2) {
			stage_render = nullfunc_near;
			tiles_render_all();
		} else {
			tiles_render();
		}
	} else if(boss.phase == PHASE_BOSS_ENTRANCE_BB) {
		unsigned char entrance_cel = (
			boss.phase_frame / ENTRANCE_BB_FRAMES_PER_CEL
		);
		if(entrance_cel < (TILES_BB_CELS / 2)) {
			tiles_render_all();
		} else {
			// A copy of boss_backdrop_render()…
			grcg_setmode_tdw();
			grcg_setcolor_direct(1);
			// … that probably predated [boss_backdrop_colorfill]?
			mugetsu_gengetsu_backdrop_colorfill();
			grcg_off();

			cdg_put_noalpha_8(
				MUGETSU_GENGETSU_BACKDROP_LEFT,
				MUGETSU_GENGETSU_BACKDROP_TOP,
				CDG_BG_BOSS
			);
		}
		tiles_bb_put(bb_boss_seg, entrance_cel);
	} else if(boss.phase < PHASE_EXPLODE_BIG) {
		boss_backdrop_render(
			MUGETSU_GENGETSU_BACKDROP_LEFT,
			MUGETSU_GENGETSU_BACKDROP_TOP,
			1
		);
	} else if(boss.phase == PHASE_EXPLODE_BIG) {
		tiles_render_all();
	} else /* if(boss.phase == PHASE_NONE) */ {
		tiles_render_after_custom(boss.phase_frame);
	}
}

// The two spawners at the head of Yuuka's OTHER segment, hosted here rather
// than in a new translation unit of their own: this object already names two
// code segments besides its default through the same `#pragma codeseg`
// mechanism, and it is the only one with th04/main/boss/b6.cpp's closure.
// Included LAST so that it renumbers no line-anchored citation into this file.
// (kb/codegen 0080 + 0155; th04/main/boss/b6_spawn.cpp says the rest.)
#include "th04/main/boss/b6_spawn.cpp"
