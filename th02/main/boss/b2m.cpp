/// Stage 2 Boss - Meira, her second object
/// ---------------------------------------
/// Thirteen of her twenty procs: her background renderer, her defeat animation,
/// her hittest-and-render half, her 40-slot slash pool with the four dash
/// patterns that feed it, and phase 0's and phase 1's remaining danmaku. Her two
/// entry points, her [boss_update] callback and her last two phases are
/// th02/main/boss/b2.cpp, and nothing of hers is left in th02_main.asm.
///
/// AND THE ONLY REASON THIS IS NOT th02/main/boss/b2.cpp IS ONE PAD BYTE.
/// `[measured 2026-08-23]` meira_update() over there compiles to a body plus a
/// `db 0` plus a four-entry jump table, and that pad is `-a2`'s: Turbo C++
/// emits it exactly when the table's natural offset in its OWN object is ODD
/// (kb/codegen/0096 + 0154 + 0160). The four procs this object started as were
/// `0x3C3` bytes - odd - so prepending them into b2.cpp moved that offset from
/// odd to even, the pad vanished, and every byte after it came out one early.
/// obj_probe.py read the object's segment length as 0x8D7 against a target of
/// 0x8D8, and meira_update's own published span shrank from 0x124 to 0x123,
/// which is the whole diagnosis.
///
/// SO THE PAD PROVES WHERE ZUN'S OBJECT BOUNDARY WAS, which is kb/codegen/0164
/// read forwards: an ODD table offset is impossible under `-a2`, therefore
/// meira_update()'s prefix in ZUN's own object was EVEN, therefore something
/// ends between meira_14C76() and meira_update(). b2.cpp's prefix is `0x2F8`
/// today and that is the parity it has to keep.
///
/// A consequence for whoever lifts the next group out of BOSS_5_TEXT: it goes
/// HERE, not into b2.cpp, and this object has no parity to protect because it
/// emits no generated table at all. Check that before adding a body with a
/// `switch` in it - four or more dense cases would give this object a table,
/// and then kb/codegen/0157 starts applying to it too.

// -zC, because the segment name would otherwise come from this file's own
// basename and be B2M_TEXT (kb/codegen/0105). -zPmain_03 for the near calls
// that leave this segment. -G, because every prolog here is `push bp; mov bp,
// sp` with no locals or a `sub sp, N` rather than an `enter`
// (kb/codegen/0011). NO FILE-WIDE -a2: nothing here emits a generated jump
// table, so this object has no alignment to pin and no parity to protect. The
// slash pool's stride needs `-a2`'s OTHER job and gets it scoped to the one
// declaration, exactly the way th02/main/midboss/mx.cpp scopes its queue's
// (kb/codegen/0170).
#pragma option -zCBOSS_5_TEXT -zPmain_03 -G

#include "platform.h"
#include "pc98.h"
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th01/rank.h"
#include "th01/math/subpixel.hpp"
#include "th02/core/globals.hpp"
#include "th02/hardware/pages.hpp"
#include "th02/main/entity.hpp"
#include "th02/main/explode.hpp"
#include "th02/main/score.hpp"
#include "th02/main/spark.hpp"
#include "th02/main/playfld.hpp"
#include "th02/main/scroll.hpp"
#include "th02/main/boss/boss.hpp"
#include "th02/main/bullet/bullet.hpp"
#include "th02/main/player/player.hpp"
#include "th02/main/player/shot.hpp"
#include "th02/main/tile/tile.hpp"
#include "th02/snd/snd.h"
#include "th02/sprites/main_pat.h"
#include "th02/v_colors.hpp"

// The sprite the boss and midboss renderers blit, shared by all of them and
// written from ~150 sites across th02_main.asm. `patnum_2064E` is the dump's own
// spelling and is not an IDA placeholder; retiring the address suffix means
// ruling on all of those sites at once, which is its own parcel.
extern "C" int patnum_2064E;

// The five-byte rank-scaled parameter block every boss and midboss init
// function fills a contiguous prefix of, documented in full at
// th02/main/boss/b4.cpp and state/notes/th02-boss-rank-param.md.
extern "C" uint8_t boss_rank_param[5];

// th02/main/bullet/bullet.cpp, which owns the `[inferred]` licence for this
// name: the binary-wide "the boss on screen has been defeated" flag, still a
// kb/codegen/0123 alias because ~20 dump sites read and write it.
// meira_145E1() below is what raises it for Meira.
extern "C" uint8_t boss_phase;

// Her defeat animation's own clock, and a PLAIN RENAME: every reference the
// dump had to it was inside meira_14519() below. It is a `dw 0` in
// th02_main.asm's _DATA rather than _BSS, so the storage stays there.
// th02/main/boss/b4.cpp calls Marisa's [marisa_defeat_frame].
extern "C" int meira_defeat_frame;

// Where she stands when she is not attacking, and the same expression
// meira_init() seeds [boss_left_on_page] and [boss_top_on_page] with. Two of
// the patterns below walk her back to [MEIRA_HOME_X] a few pixels a frame and
// end when she arrives, and two of them stop rising at [MEIRA_HOME_Y].
static const screen_x_t MEIRA_HOME_X = (
	PLAYFIELD_LEFT + (PLAYFIELD_W / 2) - 32
);
static const screen_y_t MEIRA_HOME_Y = (PLAYFIELD_TOP + 32);

// Her sprite's extent, and BOTH are derived rather than declared:
// meira_bg_render() below invalidates a 64x64 rect at her top-left, and the
// teleport dash in th02/main/boss/b2.cpp draws its target from a range of
// exactly PLAYFIELD_W minus the width. The width is also the right-hand bound
// meira_148FD() stops dash-slashing at, so her whole sprite stays inside the
// playfield either way.
static const pixel_t MEIRA_W = 64;
static const pixel_t MEIRA_H = 64;

/// Her four dash-slash sprites, and they are DIRECTIONAL
/// -----------------------------------------------------
/// `[measured 2026-08-23]` Every one of the seven sites that picks one of these
/// writes it as a base plus or minus [meira_player_is_right], immediately before
/// a dash whose x step meira_slash_step() mirrors on the same flag - so the
/// sprite is a function of the direction she is ABOUT to move in, and the four
/// numbers fall out of the six sites in meira_148FD() and meira_14C76() below
/// with no contradiction at all:
///
///	dash step	flag 0		flag 1
///	(-8, -8)	144 up-left	145 up-right
///	( 8, -8)	145 up-right	144 up-left
///	( 8,  8)	149 down-right	148 down-left
///	(-8,  8)	148 down-left	149 down-right
///
/// ZUN BUG (`[measured]`): meira_14A39() breaks it. Its one dash step is
/// (-8, 8), the same as meira_14C76()'s first, but it picks
/// `MEIRA_PAT_DASH_DOWN_RIGHT - flag` where that one picks
/// `MEIRA_PAT_DASH_DOWN_LEFT + flag` - so she faces the wrong way for the whole
/// of phase 0's third pattern. One arm of the table above is simply mirrored
/// there; the sprite is still in the binary and therefore still in the C++.
static const main_patnum_t MEIRA_PAT_DASH_UP_LEFT = 144;
static const main_patnum_t MEIRA_PAT_DASH_UP_RIGHT = 145;
static const main_patnum_t MEIRA_PAT_DASH_DOWN_LEFT = 148;
static const main_patnum_t MEIRA_PAT_DASH_DOWN_RIGHT = 149;
/// -----------------------------------------------------

/// Her slash pool
/// --------------
/// 40 slots of 12 bytes. A slot is claimed at her centre with a bullet group, a
/// speed and a trail length, and then does three things in order: it blits a
/// half-size trail sprite at that fixed point for [trail_frames] frames and is
/// LETHAL for every one of them; then it jumps 8 pixels up and left and blooms
/// through an 8-cel sprite animation at 4 frames a cel, harmlessly; and then it
/// fires its bullet group and frees itself.
///
/// `slash` for the slot, because that is what the four patterns that spawn them
/// are: meira_slash_step() below steps her one diagonal step and leaves exactly
/// one of these behind at her new centre, so a pattern's run of them is the
/// trail of a sword swing. The pool is what makes the swing outlive the frame
/// she swung on.

// Instruction-derived, and pinned by the walkers rather than by the 480 bytes
// the dump reserves: all three functions below advance their walk pointer by
// twelve bytes a slot and stop when their signed counter reaches forty.
static const int MEIRA_SLASH_COUNT = 40;

// The rect meira_slashes_invalidate() hands back to the tile layer, which is
// the only statement of a slash's extent anywhere: the bloom is blitted with
// super_roll_put() and the trail with super_roll_put_tiny(), and neither takes
// a size.
static const pixel_t MEIRA_SLASH_W = 32;
static const pixel_t MEIRA_SLASH_H = 32;

// The trail's hitbox, and it is the HALVED sprite rather than the rect above -
// super_roll_put_tiny() blits a 32x32 pattern at half size. `[measured]` The
// test is NOT symmetric between the axes: x is inclusive at both ends and y is
// exclusive at both, so a slash whose top-left is exactly the player's top-left
// hits on x and misses on y.
static const pixel_t MEIRA_SLASH_TRAIL_W = 16;
static const pixel_t MEIRA_SLASH_TRAIL_H = 16;

// The two trail sprites, and they are the two DIAGONALS rather than the four
// directions the bloom's parent dash has: `[measured]` 120 is picked for the
// (-8, 8) and (8, -8) steps and 121 for (-8, -8) and (8, 8), i.e. one sprite
// per slash line. Spelled as a base plus or minus [meira_player_is_right] at
// every site, the same way the four sprites above are.
static const main_patnum_t MEIRA_SLASH_TRAIL_PATNUM_SLASH = 120;
static const main_patnum_t MEIRA_SLASH_TRAIL_PATNUM_BACKSLASH = 121;

// Where the bloom's first cel sits, how many cels it has, and how long each of
// them lasts. `[measured]` The cel is [age] >> 2 rather than a separate field,
// and the pattern number is not in th02/sprites/main_pat.h: 26 falls inside
// miko.bft's 0..33 block, which that header only names the playchar and laser
// charge cels of.
static const main_patnum_t MEIRA_SLASH_BLOOM_PATNUM = 26;
static const uint8_t MEIRA_SLASH_BLOOM_FRAMES = 32;

// How far up and left the slash jumps on the frame its trail runs out, which
// re-centres the 32x32 bloom over the 16x16 trail it replaces.
static const pixel_t MEIRA_SLASH_BLOOM_OFFSET = 8;

// Where a claimed slot sits relative to her top-left, and where its bullets
// come out relative to the slot. `[measured]` Both are 12 and neither is half
// of anything: her sprite is 64 wide and a slash 32, so 12 is not her centre
// and not the slash's either.
static const pixel_t MEIRA_SLASH_SPAWN_OFFSET = 12;
static const pixel_t MEIRA_SLASH_PELLET_OFFSET = 12;

// -a2 FOR THIS DECLARATION ONLY. `[measured]` The slot is 12 bytes - all three
// walkers advance by twelve - and these nine fields pack to 11 under this
// tree's default byte alignment. Adding 11 to a register and adding 12 to it
// are the SAME THREE BYTES, so the object's SEGDEF length comes out at exactly
// the bytes the root gave up either way, and only the LEDATA disassembly can
// tell the two apart. kb/codegen/0170.
#pragma option -a2
struct meira_slash_t {
	entity_flag_t flag;

	// What the slash fires when its bloom runs out. `[measured]` Per-slot
	// rather than global even though every spawn reads the one global
	// [meira_burst_group] for it, which is what lets a pattern change the group
	// while its earlier slashes are still in flight.
	uint8_t group; // ACTUAL TYPE: bullet_group_or_special_motion_t

	screen_x_t left;

	// A SCREEN y and not a VRAM row, unlike th02/main/midboss/mx.cpp's queue:
	// both renderers below add [scroll_line] and fold at the moment they blit,
	// and the bullet spawn does not fold at all. So a slash stays where SHE was
	// while the map scrolls under it.
	screen_y_t top;

	// `[measured]` Always 0. Every spawn site is meira_slash_step() below and it
	// passes the literal; the field exists because the pool record mirrors
	// bullets_add_pellet()'s argument list.
	unsigned char angle;

	// Frames since the trail ran out, i.e. the bloom's clock. Not touched while
	// the trail is still drawing, so a slot's total lifetime is
	// [trail_frames] + MEIRA_SLASH_BLOOM_FRAMES.
	uint8_t age;

	// A subpixel_t narrowed to a byte for storage, the way mx.cpp's queue
	// narrows its own.
	uint8_t speed;

	uint8_t trail_patnum; // ACTUAL TYPE: main_patnum_t

	// How many more frames the lethal trail sprite is blitted for. `[measured]`
	// UNSIGNED - the test that ends it is a `jbe`.
	uint8_t trail_frames;
};
// Back to this tree's byte alignment, so that no later declaration in this
// translation unit silently grows.
#pragma option -a-

// `[measured]` Exclusive to the three functions below - they held every
// reference the dump had to this address, each of them the `offset` a near walk
// pointer is seeded from - so IDA's placeholder is RETIRED rather than aliased.
// `near`, like [midbossx_bursts] and [shots], because the original walks it with
// a bare 16-bit SI.
extern "C" meira_slash_t near meira_slashes[MEIRA_SLASH_COUNT];

/// The globals a spawn reads instead of taking as arguments
/// --------------------------------------------------------
/// Five of meira_slash_t's nine fields are seeded from file-scope state rather
/// than from meira_slash_step()'s arguments, and ALL FIVE are plain renames as
/// of this parcel: every reference the dump had to any of them was inside the
/// six procs it lifted.

// Which sprite of a pair every site that picks one picks, and the sign
// meira_slash_step() mirrors its x step with. `[measured]` Seeded at the start
// of three patterns from `(*boss_left_on_back_page + 16) < player_topleft.x`,
// i.e. from which side of her the player is standing on, and then used four
// ways: as the +/-1 on the six dash sprites and the two trail sprites, as the
// negation of meira_slash_step()'s x step, as the playfield edge that step
// clamps against, and as the direction of the walk home in meira_14A39().
//
// state/notes/th02-meira-tail.md recorded that "its POLARITY is not consistent
// across those three, which is exactly why one word cannot name it". THAT IS
// REFUTED, and the pool is what refutes it: the six dash sites and the four
// trail sites resolve to exactly four directional sprites and two diagonal
// ones with a single polarity throughout (see MEIRA_PAT_DASH_UP_LEFT above).
// The one site that disagrees is meira_14A39()'s, and it disagrees with
// meira_14C76() on the same dash step - so it is a bug in that one pattern
// rather than an ambiguity in this flag.
extern "C" uint8_t meira_player_is_right;

// The trail sprite the next spawn gets, one of the two diagonals above.
extern "C" uint8_t meira_slash_trail_patnum; // ACTUAL TYPE: main_patnum_t

// How long the next spawn's trail lasts. `[measured]` Seeded to 0x10, 0x14 or
// 0x50 at the start of a pattern and then stepped by +2 or -1 per spawn, so a
// pattern's slashes do not all live equally long: meira_14A39()'s trail gets
// shorter as the swing goes on and meira_14C76()'s and meira_148FD()'s get
// longer.
extern "C" int16_t meira_slash_trail_frames;

// Which slash burst this is, and it exists ONLY to halve the bullets on Easy.
// `[measured]` Incremented once per slash that reaches its fire frame and never
// reset, and the gate is `(this & 1) <= rank` - so on RANK_EASY every other
// burst is dropped and on every other rank all of them fire. Signed on the
// right of that comparison, because [rank] is a `char`; before the lift, its
// read was sign-extended with `cbw` (`2cac6ace:th02_main.asm:2715`).
extern "C" uint8_t meira_slash_burst_i;

// The group and the speed of the bursts her patterns fire, written by five of
// them and read through meira_slashes_add().
//
// THE SECOND OF THE PAIR WAS NAMED FOR THE RECORD'S ANGLE FIELD until this
// parcel, and the pool is what decided otherwise: the byte lands in
// meira_slash_t::speed, which meira_slashes_update_and_render() below passes to
// bullets_add_pellet() as its own speed argument, while the record's angle
// field takes the literal 0 that meira_slash_step() passes. The two values it
// is ever given are to_sp(4.0f) and to_sp(3.0f), which is what a speed at that
// address looks like. The old name was marked `[inferred]` for exactly the
// reason that no proc then lifted could check it.
extern "C" uint8_t meira_burst_group; // ACTUAL TYPE: bullet_group_or_special_motion_t
extern "C" uint8_t meira_burst_speed; // A subpixel_t, narrowed for storage.
/// --------------------------------------------------------

/// Her two ramping pellet speeds
/// -----------------------------
/// One per pattern, and each is private to the one pattern that uses it - so
/// both are PLAIN RENAMES rather than kb/codegen/0123 aliases. `[measured]`
/// Both start at to_sp(2.0) and both add 11 subpixels per pellet, which is what
/// makes each pattern's stream accelerate; nothing resets them except the next
/// run of their own pattern.
extern "C" subpixel_t meira_252E8;
extern "C" subpixel_t meira_252EA;
/// -----------------------------


/// Her afterimage trail, from the renderer's side
/// ----------------------------------------------
/// Three slots of past positions per VRAM page, pushed at every 8th frame of
/// her teleport dash (th02/main/boss/b2.cpp) and blitted there as single-colour
/// silhouettes. THIS is the half that keeps them: the renderer below unputs the
/// back page's three and then copies the front page's over them, which is the
/// same double-buffered history every TH02 renderer keeps.
///
/// `[measured]` TWO PARALLEL ARRAYS and not an array of points: every walker
/// indexes them with `page * 6 + slot * 2` against two separate `offset`s, and a
/// point array would be one `offset` and a stride of 4.
///
/// AND THE RENDERER DOES NOT HOIST THE ROW, where b2.cpp's dash does: the dash
/// takes `meira_afterimage_left[page_back]` into a near pointer once, and this
/// one recomputes `page * 6 + slot * 2` at every single one of its eight reads.
/// So the subscripts have to be written out in full here.

static const int MEIRA_AFTERIMAGE_SLOTS = 3;

extern "C" screen_x_t near meira_afterimage_left[PAGE_COUNT][
	MEIRA_AFTERIMAGE_SLOTS
];
extern "C" screen_y_t near meira_afterimage_top[PAGE_COUNT][
	MEIRA_AFTERIMAGE_SLOTS
];

// NAMED AT LAST, and this parcel is what made it nameable rather than a
// judgement call: it carried an address suffix for four parcels, on the stated
// grounds that its one reader was a renderer nobody had lifted. That reader is
// meira_bg_render() below. `[measured]` meira_14E9D() raises it on the
// way into her last phase - the phase whose only remaining pattern is the
// teleport dash - and meira_init() clears it, and nothing else writes it. So it
// is exactly "the trail is on", and the six slots above are stale for the whole
// of phases 0 and 1.
extern "C" bool16 meira_afterimages_active;
/// ----------------------------------------------

// How far into her sprite her defeat animation centres its explosion rings and
// its sparks. `[measured]` 24 for the rings and 48 for the sparks, and her
// sprite is 64x64, so NEITHER is her centre - the rings sit up and left of it
// and the sparks down and right.
static const pixel_t MEIRA_EXPLODE_OFFSET = 24;
static const pixel_t MEIRA_SPARK_OFFSET = 48;

// How long her defeat animation runs, and the two numbers the zoom is built
// from. `[measured]` The zoomed sprite is a PATTERN NUMBER and not a zoom
// factor: it is super_zoom()'s third argument, which both
// libs/master.lib/pc98_gfx.hpp and libs/master.lib/super_zoom.asm's own header
// comment call `num`, with the factor fourth and fixed at 2 here. (th02/main/
// boss/b4.cpp's marisa_1AC7B() calls its copy of this local `zoom`; that is a
// comment defect over there, not a match defect, and it should not be ported.)
static const int MEIRA_DEFEAT_FRAMES = 96;
static const int MEIRA_DEFEAT_ZOOM_PATNUM = 10;
static const int MEIRA_DEFEAT_ZOOM_FRAMES_PER_PATNUM = 10;

// The frame her defeat animation switches from her still sprite to the zoom.
// `[measured]` [boss_phase_frame] and NOT the defeat counter the zoom's own
// pattern number is derived from, so the two clocks that drive this one
// animation are unrelated: nothing resets [boss_phase_frame] when she is
// defeated, so which of the two branches the first frames take depends on where
// in her last pattern she died.
static const int MEIRA_DEFEAT_ZOOM_FROM_PHASE_FRAME = 32;

// The [boss_damage] her fight ends at, and what it is worth. Spelled as the
// literals the original holds, the way th02/main/boss/b4.cpp spells Marisa's
// 900 and 20000.
static const int MEIRA_DAMAGE_MAX = 2400;

// Her hitbox against the player, and it is neither her sprite nor a square:
// `[measured]` 32 wide against 48 tall, and the tall axis starts 16 pixels
// ABOVE her top edge. Her sprite is 64x64, so the right half of her cannot
// touch the player at all.
static const pixel_t MEIRA_PLAYER_HITBOX_W = 32;
static const pixel_t MEIRA_PLAYER_HITBOX_TOP = 16;
static const pixel_t MEIRA_PLAYER_HITBOX_BOTTOM = 32;

// The box her own shots are tested against, and the offset it sits at. Same
// 48x48 as Marisa's, but 8 pixels in from her left edge rather than 24.
static const pixel_t MEIRA_SHOT_HITBOX_LEFT = 8;
static const pixel_t MEIRA_SHOT_HITBOX_W = 48;
static const pixel_t MEIRA_SHOT_HITBOX_H = 48;

// Defined below, and reached from above: meira_bg_render() is the pool's only
// invalidate caller and sits at a lower address than the pool itself.
static void near meira_slashes_invalidate(void);


// Her [boss_bg_render] callback: re-point the two back-page indirections at
// this frame's back page, hand her own rect and her trail's three back to the
// tile layer, roll the trail's history forward a page, copy her position over
// from the front page, and then do the same for the slash pool. Installed into
// [boss_bg_render_func] by stage_init().
extern "C" void far meira_bg_render(void)
{
	register int i;

	boss_left_on_back_page = &boss_left_on_page[page_back];
	boss_top_on_back_page = &boss_top_on_page[page_back];
	tiles_invalidate_rect(
		*boss_left_on_back_page, *boss_top_on_back_page, MEIRA_W, MEIRA_H
	);
	if(meira_afterimages_active) {
		for(i = 0; i < MEIRA_AFTERIMAGE_SLOTS; i++) {
			// A slot that still holds her CURRENT position is not unput,
			// because her own rect above already covers it. `[measured]`
			// EITHER axis matching is enough, which is the same test the dash
			// uses to decide not to DRAW that slot.
			if(
				(meira_afterimage_left[page_back][i] !=
					*boss_left_on_back_page) &&
				(meira_afterimage_top[page_back][i] !=
					*boss_top_on_back_page)
			) {
				tiles_invalidate_rect(
					meira_afterimage_left[page_back][i],
					meira_afterimage_top[page_back][i],
					MEIRA_W,
					MEIRA_H
				);
			}
			meira_afterimage_left[page_back][i] =
				meira_afterimage_left[page_front][i];
			meira_afterimage_top[page_back][i] =
				meira_afterimage_top[page_front][i];
		}
	}
	*boss_left_on_back_page = boss_left_on_page[page_front];
	*boss_top_on_back_page = boss_top_on_page[page_front];
	meira_slashes_invalidate();
}


// One frame of her defeat animation: two explosion rings 24 frames apart, one
// spark, and either her still sprite or a growing zoom of it. Returns true on
// the frame the animation runs out, which is where meira_update() leaves the
// fight with SP_CLEAR.
//
// `[measured]` NEITHER RING NOR SPARK IS GUARDED, unlike marisa_1AC7B()'s three
// - the second ring is simply handed a negative frame number for the first 24
// frames and boss_explode_render() documents that it does nothing with one. So
// the second ring is 24 frames behind the first for the whole animation, and one
// spark is added on every single frame of it.
extern "C" bool16 near meira_14519(void)
{
	register int zoom_patnum;
	register vram_y_t vram_y;

	boss_explode_render(
		(*boss_left_on_back_page + MEIRA_EXPLODE_OFFSET),
		(*boss_top_on_back_page + MEIRA_EXPLODE_OFFSET),
		meira_defeat_frame
	);
	boss_explode_render(
		(*boss_left_on_back_page + MEIRA_EXPLODE_OFFSET),
		(*boss_top_on_back_page + MEIRA_EXPLODE_OFFSET),
		// 24, the spacing th02/main/explode.hpp documents and the literal
		// b4.cpp's marisa_1AC7B() also spells - it is NOT
		// EXPLODE_PHASE_1_FRAMES, which is 30.
		(meira_defeat_frame - 24)
	);
	sparks_add(
		(*boss_left_on_back_page + MEIRA_SPARK_OFFSET),
		(*boss_top_on_back_page + MEIRA_SPARK_OFFSET),
		((3 << 4) + 12),
		1,
		false
	);
	// TWO STATEMENTS, and the base is NOT an initializer on the declaration
	// above. Before the lift, `mov di, 0Ah` came HERE, after the three calls,
	// followed by `add di, ax` (`5d16a538:th02_main.asm:2410`). An initializer
	// emits it in the prolog, which is 3 bytes
	// in the wrong place and shifts the whole body. (th02/main/boss/b4.cpp's
	// marisa_1AC7B() does declare its copy with an initializer, and its original
	// wants it there - so this is per-function and not a rule.)
	zoom_patnum = MEIRA_DEFEAT_ZOOM_PATNUM;
	zoom_patnum += (
		(meira_defeat_frame - MEIRA_DEFEAT_ZOOM_FROM_PHASE_FRAME) /
		MEIRA_DEFEAT_ZOOM_FRAMES_PER_PATNUM
	);
	meira_defeat_frame++;
	if(meira_defeat_frame >= MEIRA_DEFEAT_FRAMES) {
		meira_defeat_frame = 0;
		return true;
	}
	vram_y = *boss_top_on_back_page;
	vram_y += scroll_line;
	if(vram_y >= RES_Y) {
		vram_y -= RES_Y;
	}
	if(boss_phase_frame < MEIRA_DEFEAT_ZOOM_FROM_PHASE_FRAME) {
		super_roll_put(*boss_left_on_back_page, vram_y, patnum_2064E);
	} else {
		super_zoom(*boss_left_on_back_page, vram_y, zoom_patnum, 2);
	}
	return false;
}


// Her hittest-and-render half, called by meira_update() on every frame she is
// alive: the player's collision, then her own, then her sprite - flashed white
// on the frames she was hit. Ends the fight at [MEIRA_DAMAGE_MAX].
extern "C" void near meira_145E1(void)
{
	int damage;

	// `register` EXPLICITLY, because it has to outrank [damage]: the original
	// keeps the y in SI and [damage] on the frame, and kb/codegen/0143 measured
	// that a `damage` with three references takes SI on its own if nothing
	// claims it first.
	register vram_y_t vram_y;

	vram_y = *boss_top_on_back_page;
	if(
		(*boss_left_on_back_page <= player_topleft.x) &&
		((*boss_left_on_back_page + MEIRA_PLAYER_HITBOX_W) >
			player_topleft.x) &&
		((vram_y - MEIRA_PLAYER_HITBOX_TOP) < player_topleft.y) &&
		((vram_y + MEIRA_PLAYER_HITBOX_BOTTOM) > player_topleft.y)
	) {
		player_is_hit = PLAYER_HIT;
	}
	vram_y += scroll_line;
	if(vram_y >= RES_Y) {
		vram_y -= RES_Y;
	}

	// ASSIGNED AND TESTED IN ONE EXPRESSION, which is kb/codegen/0143 and the
	// same shape th02/main/boss/b4.cpp's marisa_1AA60() needs for the identical
	// call: at TWO mentions of [damage] Turbo C++ leaves it on the frame, and at
	// three it enregisters it - here into DI, since [vram_y] already holds SI.
	// Before the lift, the prolog was `sub sp, 2` / `push si`
	// (`5d16a538:th02_main.asm:2468`), so the frame slot is what it wants; the
	// three-mention spelling came out 2 bytes short.
	//
	// NOT [vram_y] for the top, either, and this is the one place in her fight
	// where the difference is observable: the shot hitbox is tested in SCREEN
	// space while the sprite two lines down is blitted at the FOLDED row.
	if((damage = shots_hittest(
		(*boss_left_on_back_page + MEIRA_SHOT_HITBOX_LEFT),
		*boss_top_on_back_page,
		MEIRA_SHOT_HITBOX_W,
		MEIRA_SHOT_HITBOX_H
	)) != 0) {
		boss_damage += damage;
		snd_se_play(4);
		super_roll_put_1plane(
			*boss_left_on_back_page, vram_y, patnum_2064E, 0,
			super_plane(V_WHITE)
		);
		if(boss_damage >= MEIRA_DAMAGE_MAX) {
			boss_phase = 1;
			score_delta += 30000;
			player_invincibility_time = BOSS_DEFEAT_INVINCIBILITY_FRAMES;
		}
	} else {
		super_roll_put(*boss_left_on_back_page, vram_y, patnum_2064E);
	}
}


// Claims the first free slot for a slash at (left, top). Silently does nothing
// if all 40 are busy, and no caller could tell either way: this returns
// nothing.
//
// Plural pool noun plus `_add`, which is the house shape by seventeen
// precedents to one (bullets_add_pellet, items_add, lasers_add, sparks_add,
// enemies_add, sigma_blasts_add, midbossx_bursts_add). `pascal`, which is what
// the original's callee-popping return of twelve argument bytes says, and
// `static`, because meira_slash_step() below is its only caller anywhere and
// the dump never published it.
//
// `[measured]` The argument list is bullets_add_pellet()'s five, in the same
// order, plus the trail length - which is the strongest single piece of evidence
// for the record's field names, because four of the six go straight into the
// bullet call at the other end of the slot's life.
static void pascal near meira_slashes_add(
	screen_x_t left, screen_y_t top, unsigned char angle, uint8_t group,
	uint8_t speed, uint8_t trail_frames
)
{
	register meira_slash_t near *slash;
	int i;

	slash = meira_slashes;
	for(i = 0; i < MEIRA_SLASH_COUNT; i++, slash++) {
		if(slash->flag == F_FREE) {
			slash->flag = F_ALIVE;
			slash->group = group;
			slash->left = left;
			slash->top = top;
			slash->angle = angle;
			slash->age = 0;
			slash->speed = speed;
			slash->trail_patnum = meira_slash_trail_patnum;
			slash->trail_frames = trail_frames;
			return;
		}
	}
}


// Marks the tiles behind every claimed slash for redrawing, and frees the ones
// that meira_slashes_update_and_render() flagged on the previous frame.
//
// `static` again: the slash pool parcel had to publish this for the still-ASM
// meira_bg_render(), and that renderer is above in this same object now, so the
// publish and the head-of-segment `extrn` that reached it are both refunded.
static void near meira_slashes_invalidate(void)
{
	register meira_slash_t near *slash;
	register int i;

	slash = meira_slashes;
	for(i = 0; i < MEIRA_SLASH_COUNT; i++, slash++) {
		if(slash->flag == F_FREE) {
			continue;
		}
		tiles_invalidate_rect(
			slash->left, slash->top, MEIRA_SLASH_W, MEIRA_SLASH_H
		);
		if(slash->flag == F_REMOVE) {
			slash->flag = F_FREE;
		}
	}
}


// Advances and blits every claimed slash: the lethal trail while it lasts, then
// the bloom, then the bullets. Called unconditionally at the bottom of
// meira_update() (th02/main/boss/b2.cpp), i.e. it keeps running through her
// defeat animation and for as long as it takes the last slash to resolve.
extern "C" void near meira_slashes_update_and_render(void)
{
	int i;
	int patnum;
	register meira_slash_t near *slash;

	// ONE local for two different kinds of y, which is what its use in the
	// bullet spawn at the bottom proves rather than a style guess: the two
	// renderers want a VRAM ROW and fold [scroll_line] into it, and
	// bullets_add_pellet() wants a SCREEN y and gets one that was never
	// folded. `[measured]` It has to be a named local and not an argument
	// expression down there - Turbo C++ pushes a `pascal` call's arguments
	// left to right, so an inline `slash->top + N` in argument 2 comes out
	// AFTER argument 1 in AX, where the original computes it into DI FIRST.
	register int y;

	slash = meira_slashes;
	for(i = 0; i < MEIRA_SLASH_COUNT; i++, slash++) {
		if(slash->flag != F_ALIVE) {
			continue;
		}
		if(slash->trail_frames > 0) {
			y = slash->top;
			y += scroll_line;
			if(y >= RES_Y) {
				y -= RES_Y;
			}
			super_roll_put_tiny(slash->left, y, slash->trail_patnum);
			slash->trail_frames--;

			// The asymmetry between the axes is the original's, not a
			// transcription slip - see MEIRA_SLASH_TRAIL_W above.
			if(
				((slash->left - MEIRA_SLASH_TRAIL_W) <= player_topleft.x) &&
				(slash->left >= player_topleft.x) &&
				((slash->top - MEIRA_SLASH_TRAIL_H) < player_topleft.y) &&
				(slash->top > player_topleft.y)
			) {
				player_is_hit = PLAYER_HIT;
			}
		} else if(slash->age == 0) {
			snd_se_play(10);
			slash->left -= MEIRA_SLASH_BLOOM_OFFSET;
			slash->top -= MEIRA_SLASH_BLOOM_OFFSET;

			// A FORWARD `goto` INTO THE NEXT ARM and not a duplicated block,
			// which is kb/codegen/0144's rule rather than a choice: the
			// original jumps over the `age < MEIRA_SLASH_BLOOM_FRAMES` test
			// into the head of that arm's body, and `-O`'s tail merger cannot
			// produce that jump because the shared block contains a
			// conditional branch of its own (the RES_Y fold). Duplicating it
			// per kb/codegen/0097 would merge only the last few instructions
			// and come out tens of bytes long.
			goto bloom;
		} else if(slash->age < MEIRA_SLASH_BLOOM_FRAMES) {
bloom:
			patnum = ((slash->age >> 2) + MEIRA_SLASH_BLOOM_PATNUM);
			y = slash->top;
			y += scroll_line;
			if(y >= RES_Y) {
				y -= RES_Y;
			}
			super_roll_put(slash->left, y, patnum);
			slash->age++;
		} else {
			if((meira_slash_burst_i & 1) <= rank) {
				// TWO STATEMENTS, for the reason th02/main/boss/b2.cpp's
				// meira_update() already records: written as one
				// expression Turbo C++ accumulates in AX and adds a
				// `mov di, ax`, which is exactly 2 bytes long.
				y = slash->top;
				y += MEIRA_SLASH_PELLET_OFFSET;
				bullets_add_pellet(
					(slash->left + MEIRA_SLASH_PELLET_OFFSET),
					y,
					slash->angle,
					slash->group,
					slash->speed
				);
			}
			meira_slash_burst_i++;
			slash->flag = F_REMOVE;
		}
	}
}


// Phase 0 pattern 0, and the only one of her eleven that does not attack at
// all: two telegraph sounds 40 frames apart, and then 40 frames of falling 6
// pixels each. `[measured]` Nothing here spawns a slash, fires a bullet or
// touches the player, and nothing here brings her back up either - the 240
// pixels it drops her are undone by meira_148FD() below, which is the pattern
// that follows it.
extern "C" void near meira_1483B(void)
{
	if(boss_phase_frame < 10) {
	} else if(boss_phase_frame == 10) {
		snd_se_play(9);
		patnum_2064E = 142;
	} else if(boss_phase_frame < 50) {
	} else if(boss_phase_frame == 50) {
		snd_se_play(3);
		patnum_2064E = 143;
	} else if(boss_phase_frame <= 90) {
		*boss_top_on_back_page += 6;
	} else {
		patnum_2064E = 141;
		boss_phase_frame = 0;
	}
}


// One step of a dash slash: move her one step along x with a playfield clamp,
// one step along y without one, and leave a slash at her new centre. The shared
// body of every dash in her fight - meira_14A39(), meira_14C76() and
// meira_148FD() below call nothing else to move.
//
// [mirrored] WAS NAMED FOR A DIRECTION until this parcel, and the rename is the
// flag's own doing: it NEGATES [delta_x] rather than naming one, so at
// delta_x = 8 it does mean leftward and at delta_x = -8 it means rightward, and
// both spellings are in use below.
//
// ZUN QUIRK (`[measured]`): the clamp is picked by [mirrored] and not by the
// direction she actually ends up moving in, so exactly one of the two edges is
// enforced per call. `meira_slash_step(0, -8, ...)` walks her left with only
// the RIGHT edge checked, which is the shape meira_14A39()'s twenty-frame
// (-8, 8) swing runs in.
static void pascal near meira_slash_step(
	unsigned char mirrored, int delta_x, int delta_y
)
{
	if(mirrored == 0) {
		*boss_left_on_back_page += delta_x;
		if(*boss_left_on_back_page > (PLAYFIELD_RIGHT - 48)) {
			*boss_left_on_back_page = (PLAYFIELD_RIGHT - 48);
		}
	} else {
		*boss_left_on_back_page -= delta_x;
		if(*boss_left_on_back_page < (PLAYFIELD_LEFT - 16)) {
			*boss_left_on_back_page = (PLAYFIELD_LEFT - 16);
		}
	}
	*boss_top_on_back_page += delta_y;
	meira_slashes_add(
		(*boss_left_on_back_page + MEIRA_SLASH_SPAWN_OFFSET),
		(*boss_top_on_back_page + MEIRA_SLASH_SPAWN_OFFSET),
		0x00,
		meira_burst_group,
		meira_burst_speed,
		meira_slash_trail_frames
	);
}


// Phase 0 pattern 1: three dash slashes, up-left then up-right then down-right,
// and the last of them repeats until she is back at [MEIRA_HOME_Y] - which is
// what undoes meira_1483B()'s fall. `[measured]` The second dash is the one
// that repeats, and it repeats by writing [boss_phase_frame] back to 56, so the
// pattern has no fixed length either.
extern "C" void near meira_148FD(void)
{
	register screen_y_t top;

	if(boss_phase_frame < 20) {
		return;
	}
	top = *boss_top_on_back_page;
	if(boss_phase_frame == 20) {
		// A BARE COMPARISON and not a `? 1 : 0`, for the reason
		// meira_14C76() below already records.
		meira_player_is_right = (
			(*boss_left_on_back_page + 16) < player_topleft.x
		);
		patnum_2064E = (meira_player_is_right + MEIRA_PAT_DASH_UP_LEFT);
		meira_slash_trail_frames = 0x10;
		meira_slash_trail_patnum = (
			MEIRA_SLASH_TRAIL_PATNUM_BACKSLASH - meira_player_is_right
		);
		meira_burst_group = BG_1_AIMED;
		meira_burst_speed = to_sp(4.0f);
	} else if(boss_phase_frame <= 32) {
		// THE `meira_slash_step()` CALL AND THE `+= 2` ARE WRITTEN OUT IN BOTH
		// ARMS ON PURPOSE, here and 60 lines down. kb/codegen/0097: `-O`
		// tail-merges the two, leaving one copy plus a jump out of this arm
		// into the middle of that one, which is exactly the original's `jmp`
		// over the second `push`. The shared tail is branch-free, so 0144's
		// exception does not apply and a `goto` would be wrong here.
		meira_slash_step(meira_player_is_right, -8, -8);
		meira_slash_trail_frames += 2;
	} else if(boss_phase_frame < 56) {
		patnum_2064E = 141;
	} else if(boss_phase_frame == 56) {
		patnum_2064E = (MEIRA_PAT_DASH_UP_RIGHT - meira_player_is_right);
		meira_slash_trail_patnum = (
			MEIRA_SLASH_TRAIL_PATNUM_SLASH + meira_player_is_right
		);
		meira_slash_trail_frames = 0x10;
	} else if(boss_phase_frame == 57) {
		meira_slash_step(meira_player_is_right, 8, -8);
		meira_slash_trail_frames += 2;

		// The rewind, and it tests the y she came INTO this frame with rather
		// than the one the step above just gave her - so she always overshoots
		// [MEIRA_HOME_Y] by the last step.
		if(top > MEIRA_HOME_Y) {
			boss_phase_frame = 56;
		}
	} else if(boss_phase_frame < 80) {
		patnum_2064E = 141;
	} else if(boss_phase_frame == 80) {
		patnum_2064E = (MEIRA_PAT_DASH_DOWN_RIGHT - meira_player_is_right);
		meira_slash_trail_patnum = (
			MEIRA_SLASH_TRAIL_PATNUM_BACKSLASH - meira_player_is_right
		);
		meira_slash_trail_frames = 0x10;
	} else if(
		(*boss_left_on_back_page > PLAYFIELD_LEFT) &&
		(*boss_left_on_back_page < (PLAYFIELD_RIGHT - MEIRA_W))
	) {
		meira_slash_step(meira_player_is_right, 8, 8);
		meira_slash_trail_frames += 2;
	} else if(top > MEIRA_HOME_Y) {
		*boss_top_on_back_page -= 2;
		patnum_2064E = 141;
	} else {
		boss_phase_frame = 0;
	}
}


// Phase 0 pattern 2: two dash slashes away from the player, then a walk back to
// [MEIRA_HOME_X] that ends the pattern when she arrives - so this one has no
// fixed length, and a slash that leaves her near an edge makes it longer.
extern "C" void near meira_14A39(void)
{
	if(boss_phase_frame < 50) {
	} else if(boss_phase_frame == 50) {
		snd_se_play(9);
		patnum_2064E = 142;
		meira_burst_group = boss_rank_param[0];
	} else if(boss_phase_frame < 99) {
	} else if(boss_phase_frame == 99) {
		snd_se_play(3);

		// THE MIRRORED ONE. Its dash step below is (-8, 8), the same as
		// meira_14C76()'s first, and that one picks
		// MEIRA_PAT_DASH_DOWN_LEFT + [meira_player_is_right]. See the sprite
		// table at the top of this file.
		patnum_2064E = (MEIRA_PAT_DASH_DOWN_RIGHT - meira_player_is_right);

		meira_slash_trail_patnum = (
			MEIRA_SLASH_TRAIL_PATNUM_SLASH + meira_player_is_right
		);
		meira_slash_trail_frames = 0x50;
	} else if(boss_phase_frame < 120) {
		meira_slash_step(meira_player_is_right, -8, 8);
		meira_slash_trail_frames--;
	} else if(boss_phase_frame < 136) {
		patnum_2064E = 142;
		patnum_2064E = (meira_player_is_right + MEIRA_PAT_DASH_UP_LEFT);
		meira_slash_trail_patnum = (
			MEIRA_SLASH_TRAIL_PATNUM_BACKSLASH - meira_player_is_right
		);
		meira_slash_trail_frames--;
		if(boss_phase_frame == 135) {
			snd_se_play(3);
		}
	} else if(boss_phase_frame < 156) {
		meira_slash_step(meira_player_is_right, -8, -8);
		meira_slash_trail_frames--;
	} else if(*boss_left_on_back_page != MEIRA_HOME_X) {
		patnum_2064E = 141;
		*boss_left_on_back_page += ((meira_player_is_right == 0) ? 2 : -2);
	} else {
		boss_phase_frame = 0;
	}
}


// Phase 1 pattern 0: a stationary stream of pellets straight down, every other
// frame for 17 frames, accelerating as it goes.
extern "C" void near meira_14B33(void)
{
	if(boss_phase_frame < 50) {
	} else if(boss_phase_frame == 50) {
		snd_se_play(9);
		patnum_2064E = 142;
	} else if(boss_phase_frame < 99) {
	} else if(boss_phase_frame == 99) {
		snd_se_play(3);
		patnum_2064E = 143;
		meira_252E8 = to_sp(2.0f);
	} else if(boss_phase_frame < 116) {
		if((boss_phase_frame & 1) != 0) {
			bullets_add_pellet(
				(*boss_left_on_back_page + 24),
				(*boss_top_on_back_page + 32),
				0x40,
				boss_rank_param[1],
				meira_252E8
			);
			meira_252E8 += 11;
		}
	} else {
		patnum_2064E = 141;
		boss_phase_frame = 0;
	}
}


// Phase 1 pattern 1: the same accelerating stream as meira_14B33(), aimed
// straight up instead of down, and fired while she slides back towards
// [MEIRA_HOME_X] eight pixels a frame.
extern "C" void near meira_14BC2(void)
{
	if(boss_phase_frame < 50) {
	} else if(boss_phase_frame == 50) {
		snd_se_play(9);
		patnum_2064E = 142;
		meira_burst_group = BG_1_RANDOM_ANGLE;
	} else if(boss_phase_frame < 99) {
	} else if(boss_phase_frame == 99) {
		snd_se_play(3);
		patnum_2064E = 143;
		meira_252EA = to_sp(2.0f);
	} else if(boss_phase_frame < 120) {
		if((boss_phase_frame & 1) != 0) {
			bullets_add_pellet(
				(*boss_left_on_back_page + 24),
				(*boss_top_on_back_page + 32),
				0x00,
				boss_rank_param[2],
				meira_252EA
			);
			meira_252EA += 11;
		}
		if(*boss_left_on_back_page != MEIRA_HOME_X) {
			*boss_left_on_back_page += (
				(*boss_left_on_back_page < MEIRA_HOME_X) ? 8 : -8
			);
		}
	} else {
		patnum_2064E = 141;
		boss_phase_frame = 0;
	}
}


// Phase 1 pattern 2, and the longest pattern in her fight: FOUR dash slashes,
// one per diagonal, each 16 frames of dashing followed by 20 frames of recovery
// with a sound effect on the last of them. `[measured]` The four sprites are
// down-left, down-right, up-right and up-left in that order, every one of them
// mirrored on [meira_player_is_right], so the whole run reads as one continuous
// figure - and it is the site that settles the sprite table at the top of this
// file.
extern "C" void near meira_14C76(void)
{
	if(boss_phase_frame < 50) {
	} else if(boss_phase_frame == 50) {
		// A BARE COMPARISON and not a `? 1 : 0`. `[measured]` Turbo C++
		// materialises a relational operator used as a value in the whole of
		// AX (`mov ax, 1` / `xor ax, ax`) and then stores AL; the ternary
		// narrows the same expression to `mov al, 1` / `mov al, 0` and comes
		// out one byte short.
		meira_player_is_right = (
			(*boss_left_on_back_page + 16) < player_topleft.x
		);
		snd_se_play(9);
		patnum_2064E = 142;
		meira_burst_group = BG_1_RANDOM_ANGLE;
		meira_burst_speed = to_sp(3.0f);
	} else if(boss_phase_frame < 99) {
	} else if(boss_phase_frame == 99) {
		snd_se_play(3);
		patnum_2064E = (meira_player_is_right + MEIRA_PAT_DASH_DOWN_LEFT);
		meira_slash_trail_patnum = (
			MEIRA_SLASH_TRAIL_PATNUM_SLASH + meira_player_is_right
		);
		meira_slash_trail_frames = 0x14;
	} else if(boss_phase_frame < 116) {
		meira_slash_step(meira_player_is_right, -8, 8);
		meira_slash_trail_frames += 2;
	} else if(boss_phase_frame < 136) {
		// A DEAD STORE, and it is in the binary three more times below: every
		// recovery arm writes 142 and then immediately overwrites it with the
		// mirrored sprite it actually wants.
		patnum_2064E = 142;
		patnum_2064E = (MEIRA_PAT_DASH_DOWN_RIGHT - meira_player_is_right);
		meira_slash_trail_patnum = (
			MEIRA_SLASH_TRAIL_PATNUM_BACKSLASH - meira_player_is_right
		);
		meira_slash_trail_frames = 0x14;
		if(boss_phase_frame == 135) {
			snd_se_play(3);
		}
	} else if(boss_phase_frame < 152) {
		meira_slash_step(meira_player_is_right, 8, 8);
		meira_slash_trail_frames += 2;
	} else if(boss_phase_frame < 172) {
		patnum_2064E = 142;
		patnum_2064E = (MEIRA_PAT_DASH_UP_RIGHT - meira_player_is_right);
		meira_slash_trail_patnum = (
			MEIRA_SLASH_TRAIL_PATNUM_SLASH + meira_player_is_right
		);
		meira_slash_trail_frames = 0x14;
		if(boss_phase_frame == 171) {
			snd_se_play(3);
		}
	} else if(boss_phase_frame < 188) {
		meira_slash_step(meira_player_is_right, 8, -8);
		meira_slash_trail_frames += 2;
	} else if(boss_phase_frame < 208) {
		patnum_2064E = 142;
		patnum_2064E = (meira_player_is_right + MEIRA_PAT_DASH_UP_LEFT);
		meira_slash_trail_patnum = (
			MEIRA_SLASH_TRAIL_PATNUM_BACKSLASH - meira_player_is_right
		);
		meira_slash_trail_frames = 0x14;
		if(boss_phase_frame == 207) {
			snd_se_play(3);
		}
	} else if(boss_phase_frame < 224) {
		meira_slash_step(meira_player_is_right, -8, -8);
		meira_slash_trail_frames += 2;
	} else {
		patnum_2064E = 141;
		boss_phase_frame = 0;
	}
}
