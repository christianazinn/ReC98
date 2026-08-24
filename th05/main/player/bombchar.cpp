/// The four playchars' bomb animation drivers
/// ------------------------------------------
/// One per playchar, and the four functions main() picks between when it sets
/// [playchar_bomb_func] (th05_main.asm, `mov _playchar_bomb_func, offset
/// bomb_reimu` and its three siblings). Unlike TH04, where a single
/// bomb_update_and_render() drives the animation and [playchar_bomb_func]
/// points at the playchar-specific *rendering*, TH05 gives each playchar its
/// own driver and lets it call its own …_bomb_update_and_render().
///
/// Each driver is one frame of a bomb, from the moment it is dropped to the
/// moment [bombing] clears again: the BB?.BB dissolve-in, the scroll and
/// palette changes that bracket the bomb proper, and the call into the
/// playchar-specific animation that does the actual damage. [bomb_frame] is
/// advanced at the end, unconditionally — TH05 has no [bombing] test here,
/// because th05_main.asm only reaches [playchar_bomb_func] while it is set.
///
/// (#included from th05/bombchar.cpp, which owns BOMBCHAR_TEXT — the head half
/// of what used to be MB_INV_TEXT's root contribution, split off by a
/// kb/codegen/0080 carve so that a C++ object can append to it. Bodies go into
/// this file in ORIGINAL ADDRESS ORDER, lowest first: the root shrinks from
/// its tail and the C++ contribution grows backwards into the hole, so every
/// byte keeps its address. kb/codegen/0114 + 0129.)

// master.lib, reached through the regular header rather than hand-copied the
// way th04/main/player/bombupd.cpp has to. That file's reason is a property of
// *its* translation unit — pc98_gfx.hpp would be the second inclusion of
// libs/master.lib/func.hpp in th04/main/player/shots_inv.cpp — and this object
// is its own translation unit with only one member. Measured, not assumed:
// this header brings only func.hpp, pc98.h and x86real.h, all three guarded or
// idempotent, and bomb_yuuka() is byte-identical across the switch.
//
// [PaletteTone] and graph_scrollup() are declared here exactly as this file
// used to spell them by hand. palette_show() is the immediate half of
// th03/hardware/palette.hpp's palette_settone_deferred(): the
// …_bomb_update_and_render() functions below flash the tone *within* a frame
// rather than at the end of one, so they write [PaletteTone] and call it
// directly, which is what pc98_gfx.hpp's own palette_settone() macro is.
#include "libs/master.lib/pc98_gfx.hpp"

#include "th02/snd/snd.h"
#include "th02/v_colors.hpp"
#include "th03/formats/cdg.h"
#include "th03/hardware/palette.hpp"
#include "th04/common.h"
#include "th04/hardware/grcg.hpp"
#include "th04/main/bg.hpp"
#include "th04/main/circle.hpp"
#include "th04/main/frames.h"
#include "th04/main/null.hpp"
#include "th04/main/player/player.hpp"
#include "th04/main/player/shot.hpp"
#include "th04/main/playfld.hpp"
#include "th04/main/scroll.hpp"
#include "th04/main/stage/stage.hpp"
#include "th04/math/randring.hpp"
#include "th04/math/vector.hpp"
#include "th04/sprites/main_cdg.h"
#include "th05/main/player/bomb.hpp"
#include "th05/main/player/bombanim.hpp"
#include "th05/sprites/main_pat.h"

// Declared here rather than through their own headers, both of which are
// unguarded and would drag further unguarded ones into this object.
// th04/main/player/bombupd.cpp does the same for [items_pull_to_player].
extern bool items_pull_to_player;
extern unsigned char tiles_bb_col;

// `extern "C"` and `pascal`, because the name is published UNDECORATED and
// all-uppercase (kb/codegen/0086 + 0102); `near`, because it ends in `retn 2`.
// Compiled from th04/main/player/bb_playchar_put.cpp for both games; TH05
// reaches it through th05/main010.cpp, i.e. mai_TEXT, which is in this same
// group.
extern "C" void pascal near bb_playchar_put(int cel);

// Defined in th05/main/player/bombanim.cpp, i.e. in mai_TEXT and therefore in
// this same group, which is what makes the call `near` rather than the large
// model's default `far`. C++ linkage, because the original calls the MANGLED
// `@bomb_dream_decay$qv`.
void near bomb_dream_decay(void);

// The playfield rows and columns that each playchar's BOMB?.CDG background
// does not cover, filled in that playchar's own color. All four are defined at
// the END of this file, in BOMB_BG_TEXT — the head half of the block this
// object's own segment was carved out of (kb/codegen/0080) — and therefore in
// this same group, which is what makes the calls below `near`.
//
//   …_reimu   48 columns down either edge, over every row, in white.
//   …_marisa  80 columns down either edge, over every row, in 1.
//   …_mima    48 rows along the top and bottom edges and 32 columns down
//             either edge of what is left, in 1 — plus the white 32-pixel gap
//             in the two horizontal bands that mima_bomb_update_and_render()
//             then draws its beam into.
//   …_yuuka   64 columns down either edge, over every row, in 14.
//
// The dump gave none of them a `public` of its own, so each needed the
// zero-byte `label near` alias kb/codegen/0123 prescribes while it was still
// ASM. Those aliases went away with the bodies: `extern "C"` with the project's
// default cdecl publishes exactly the same `_bomb_bg_margins_fill_…` spelling
// (kb/codegen/0086). Each has exactly one caller, the
// …_bomb_update_and_render() that its BOMB?.CDG belongs to.
//
// THE PRAGMA HAS TO BE HERE AS WELL AS AT THE DEFINITIONS, because it binds a
// function to a segment at its FIRST DECLARATION (kb/codegen/0155). It is not
// a precaution: without it all four were emitted at the end of BOMBCHAR_TEXT,
// this object contributed nothing but three padding bytes to BOMB_BG_TEXT, and
// the link succeeded. th04/main/scroll.hpp wraps a declaration the same way.
#pragma codeseg BOMB_BG_TEXT main_01
extern "C" void near bomb_bg_margins_fill_reimu(void);
extern "C" void near bomb_bg_margins_fill_marisa(void);
extern "C" void near bomb_bg_margins_fill_mima(void);
extern "C" void near bomb_bg_margins_fill_yuuka(void);
#pragma codeseg

// Reimu's own playchar-specific animation, the only one of the four that is
// already C++: th05/main/player/bombanim.cpp defines it in mai_TEXT, i.e. in
// this same group, which is what makes the call `near`. C++ linkage, because
// the original calls the MANGLED `@reimu_stars_update_and_render$qv` — and
// what the other three now copy.
void near reimu_stars_update_and_render(void);

// The spark burst that marisa_lasers_update_and_render() fires off with every
// laser. Defined in th05/main031.cpp, i.e. in group main_03, so this is the
// large model's default `far` call and not a near one — that file's own
// comment says why the indirection exists at all. Declared here rather than in
// a header, because it has exactly one caller and no header.
void sparks_add_circle_at_player(void);

// The GRCG-off sequence the original inlines after every filled region:
// `mov dx, 7Ch` / `mov al, 0` / `out dx, al`. Same spelling and the same
// reason as th04/main/player/bombchar.cpp's copy, except that this object does
// #include libs/master.lib/pc98_gfx.hpp and can therefore name [GC_OFF]
// rather than describing it. Borland's `outportb()` intrinsic always goes
// through DX, which is why the port fits in a byte and the sequence is still
// 6 bytes long — and why the name says which register it clobbers.
#define grcg_off_clobbering_dx() outportb(0x7C, GC_OFF)

// Stage 6 is Makai, which does not scroll: its background is a boss backdrop
// from the first frame, so the two `graph_scrollup()` calls that bracket a
// bomb would put a scrolled-in edge on screen. Spelled against
// [MAIN_STAGE_COUNT] the way th04/main/boss/boss.cpp:251 spells the same test.
// All four bomb_…() functions below carry it.
#define stage_scrolls() (stage_id != (MAIN_STAGE_COUNT - 1))

/// Reimu's per-frame bomb animation: the BOMB?.CDG background, the playfield
/// margins around it, the star field, and the palette. See
/// yuuka_bomb_update_and_render() below for the naming.
void near reimu_bomb_update_and_render(void)
{
	enum {
		// Reimu's background is the 288-pixel-wide center of the playfield
		// and covers its full height; bomb_bg_margins_fill_reimu() paints the
		// 48 columns on either side of it.
		BOMB_BG_LEFT = 48,

		// The [bomb_frame] at which bomb_reimu() starts calling this
		// function, i.e. that function's own [FRAME_BOMB].
		FRAME_BOMB = 32,

		// Until here, the palette tone walks down from [TONE_START] to
		// master.lib's neutral 100 — (64 - 32) * 3 == 96. From then until
		// [FRAME_SE_END], the same sound effect is played every 4th frame
		// instead, which is the same rate TH04's bomb_reimu() spawns its
		// circles at and the reason both spell the test against
		// [stage_frame_mod4].
		FRAME_FADE_END = 64,
		TONE_START = 196,
		TONE_PER_FRAME = 3,
		FRAME_SE_END = 112,

		// Reimu's growing circles are 9, the same as Yuuka's and as TH04's
		// bomb_reimu(). The stars themselves are 14 and 7, which
		// reimu_stars_update_and_render() picks for itself.
		COL_CIRCLES = 9,
	};

	cdg_put_noalpha_8(
		(PLAYFIELD_LEFT + BOMB_BG_LEFT), PLAYFIELD_TOP, CDG_BG_PLAYCHAR_BOMB
	);
	bomb_bg_margins_fill_reimu();
	reimu_stars_update_and_render();
	circles_color = COL_CIRCLES;
	if(bomb_frame <= FRAME_FADE_END) {
		palette_settone_deferred(
			TONE_START - ((bomb_frame - FRAME_BOMB) * TONE_PER_FRAME)
		);
	} else if((bomb_frame <= FRAME_SE_END) && (stage_frame_mod4 == 0)) {
		snd_se_play(9);
	}
}

extern "C" void pascal near bomb_reimu(void)
{
	enum {
		// The BB?.BB dissolve-in, 16 cels at 2 frames each — the same one
		// bomb_yuuka() below has, and the two Reimu-and-Yuuka-only halves of
		// this file's four otherwise identical drivers.
		BB_CEL_FRAMES = 2,
		FRAME_BOMB = 32,

		// The bomb proper runs from [FRAME_BOMB] to here: 96 frames, the
		// longest of the four.
		FRAME_FADE = 128,
		FRAME_END = 160,

		TONE_START = 192,
		TONE_PER_FRAME = 3,
		TONE_NORMAL = 100,

		FRAME_SCROLL_RESUME = (FRAME_FADE + 2),
	};

	if(bomb_frame < FRAME_BOMB) {
		tiles_bb_col = V_WHITE;
		bb_playchar_put(bomb_frame / BB_CEL_FRAMES);
	} else if(bomb_frame == FRAME_BOMB) {
		scroll_active = false;
		graph_scrollup(0);
		bg_render_bombing = nullfunc_near;
		goto playchar_bomb;
	} else if(bomb_frame < FRAME_FADE) {
playchar_bomb:
		reimu_bomb_update_and_render();
		bomb_dream_decay();
	} else if(bomb_frame == FRAME_FADE) {
		snd_se_play(15);
		if(stage_scrolls()) {
			scroll_active = true;
		}
		items_pull_to_player = false;
		goto restore;
	} else if(bomb_frame < FRAME_END) {
restore:
		bg_render_bombing = bg_render_bombing_func;
		palette_settone_deferred(
			TONE_START - ((bomb_frame - FRAME_FADE) * TONE_PER_FRAME)
		);
		if((bomb_frame == FRAME_SCROLL_RESUME) && stage_scrolls()) {
			graph_scrollup(scroll_line);
		}
	} else {
		bombing = false;
		palette_settone_deferred(TONE_NORMAL);
	}
	bomb_frame++;
}

/// The eight vertical laser beams that Marisa's bomb drops on the player, one
/// spawned every [SPAWN_PERIOD] frames into a slot picked by the frame number.
/// Each one is a three-color column from the top of the playfield down to
/// wherever the player was, capped by a circle, and it narrows as its radius
/// shrinks. C++ linkage for the reason yuuka_heart_update_and_render() below
/// gives.
void near marisa_lasers_update_and_render(void)
{
	enum {
		// bomb_marisa() starts calling marisa_bomb_update_and_render() on
		// frame 0, but that function only reaches this one from frame 16 —
		// so unlike Mima's, this reset runs on the first frame that can
		// spawn anything, not the frame before it.
		FRAME_RESET = 16,

		// Same spawn schedule as mima_circles_update_and_render(), down to
		// the `- 16` and the mask.
		SPAWN_PERIOD = 4,
		FRAME_SPAWN_BASE = 16,

		// The beam's half-width, in pixels, and the radius of the circle that
		// caps it. Three nested columns are drawn, each one pixel narrower
		// than the last, which is what gives the beam its shaded edge.
		RADIUS_START = 24,
		RADIUS_MIN = 4,

		TONE_FLASH = 170,
		TONE_NORMAL = 100,

		// Outer, middle, and the white core the third column leaves.
		COL_OUTER = 8,
		COL_MID = 9,
	};

	// SI and DI, as in mima_circles_update_and_render() — and for the same
	// reason the reset loop below is a subscript and the render loop a
	// pointer.
	marisa_laser_t near *laser;
	int i;

	// The `enter 6, 0` frame, in declaration order: [bp-2], [bp-4], [bp-6].
	// [half_w] is a stack local rather than a register because both register
	// variables are already taken, and it is written twice per iteration.
	pixel_t x;
	pixel_t y;
	pixel_t half_w;

	if(bomb_frame == FRAME_RESET) {
		palette_settone_deferred(TONE_NORMAL);
		i = 0;
		while(i < MARISA_LASER_COUNT) {
			bomb_anim.marisa[i].center.x.v = PIXEL_NONE;
			i++;
		}
	}

	// Unlike Mima's, this palette write is immediate on BOTH paths, so the
	// palette_show() below the `if` is unconditional rather than something an
	// arm has to `goto`.
	if((bomb_frame % SPAWN_PERIOD) == 0) {
		i = (
			((bomb_frame - FRAME_SPAWN_BASE) / SPAWN_PERIOD) &
			(MARISA_LASER_COUNT - 1)
		);
		laser = &bomb_anim.marisa[i];

		// ZUN bloat: one 32-bit move of exactly the two words the two
		// assignments after the call write again. Unlike
		// mima_circles_update_and_render()'s copy, which at least un-parks
		// the slot before a vector2_at() overwrites it, this one is dead —
		// sparks_add_circle_at_player() reads [player_pos], not [laser].
		reinterpret_cast<uint32_t near &>(laser->center) = (
			reinterpret_cast<uint32_t near &>(player_pos.cur)
		);
		sparks_add_circle_at_player();
		laser->center.x.v = player_pos.cur.x.v;
		laser->center.y.v = player_pos.cur.y.v;

		laser->radius = RADIUS_START;
		snd_se_play(15);
		PaletteTone = TONE_FLASH;
	} else {
		PaletteTone = TONE_NORMAL;
	}
	palette_show();

	grcg_setmode_rmw();
	laser = bomb_anim.marisa;
	i = 0;
	while(i < MARISA_LASER_COUNT) {
		if(laser->center.x.v != PIXEL_NONE) {
			// to_pixel_slow() and not to_pixel(): the original divides by
			// SUBPIXEL_FACTOR with a `cwd`/`idiv` pair rather than shifting,
			// which is exactly the distinction that function exists to carry
			// (th01/math/subpixel.hpp:77). Both calls share the one `mov bx,
			// 16`.
			x = (laser->center.x.to_pixel_slow() + PLAYFIELD_LEFT);
			y = (laser->center.y.to_pixel_slow() + PLAYFIELD_TOP);

			grcg_setcolor_direct(COL_OUTER);
			half_w = (laser->radius / 2);
			grcg_boxfill((x - half_w), PLAYFIELD_TOP, (x + half_w), y);
			grcg_setcolor_direct(COL_MID);
			half_w--;
			grcg_boxfill((x - half_w), PLAYFIELD_TOP, (x + half_w), y);
			grcg_setcolor_direct(V_WHITE);
			half_w--;
			grcg_boxfill((x - half_w), PLAYFIELD_TOP, (x + half_w), y);

			grcg_circlefill(x, y, laser->radius);
			laser->radius--;
			if(laser->radius < RADIUS_MIN) {
				laser->center.x.v = PIXEL_NONE;
			}
		}
		i++;
		laser++;
	}
	grcg_off_clobbering_dx();
}

/// Marisa's per-frame bomb animation: the BOMB?.CDG background, the playfield
/// margins around it, and either the opening palette fade or the lasers. See
/// yuuka_bomb_update_and_render() below for the naming.
void near marisa_bomb_update_and_render(void)
{
	enum {
		// Marisa's background is the 224-pixel-wide center of the playfield
		// and covers its full height; bomb_bg_margins_fill_marisa() paints
		// the 80 columns on either side of it.
		BOMB_BG_LEFT = 80,

		// Marisa's is the only one of the four drivers whose fade is measured
		// from frame 0 rather than from its caller's [FRAME_BOMB], because
		// bomb_marisa() starts calling this function on frame 0. Once the
		// tone reaches 196 - (16 * 6) == 100, the lasers take over.
		FRAME_FADE_END = 16,
		TONE_START = 196,
		TONE_PER_FRAME = 6,
	};

	cdg_put_noalpha_8(
		(PLAYFIELD_LEFT + BOMB_BG_LEFT), PLAYFIELD_TOP, CDG_BG_PLAYCHAR_BOMB
	);
	bomb_bg_margins_fill_marisa();
	if(bomb_frame < FRAME_FADE_END) {
		palette_settone_deferred(
			TONE_START - (bomb_frame * TONE_PER_FRAME)
		);
	} else {
		marisa_lasers_update_and_render();

		// Marisa cannot shoot while her lasers are out — the only one of the
		// four bombs that touches [shot_time] at all. Re-set on every frame,
		// because the player's own update clears it again.
		shot_time = SHOT_BLOCKED_FOR_THIS_FRAME;
	}
}

extern "C" void pascal near bomb_marisa(void)
{
	enum {
		// No BB?.BB dissolve-in, so the scroll stops two frames in —
		// bomb_mima() below uses the same 2.
		FRAME_SCROLL_STOP = 2,

		FRAME_FADE = 80,
		FRAME_END = 96,

		TONE_START = 192,
		TONE_PER_FRAME = 6,
		TONE_NORMAL = 100,

		FRAME_SCROLL_RESUME = (FRAME_FADE + 2),
	};

	if(bomb_frame < FRAME_FADE) {
		if(bomb_frame == FRAME_SCROLL_STOP) {
			scroll_active = false;
			graph_scrollup(0);
			bg_render_bombing = nullfunc_near;
		}
		marisa_bomb_update_and_render();
		bomb_dream_decay();
	} else if(bomb_frame == FRAME_FADE) {
		snd_se_play(15);
		if(stage_scrolls()) {
			scroll_active = true;
		}
		items_pull_to_player = false;
		goto restore;
	} else if(bomb_frame < FRAME_END) {
restore:
		bg_render_bombing = bg_render_bombing_func;
		palette_settone_deferred(
			TONE_START - ((bomb_frame - FRAME_FADE) * TONE_PER_FRAME)
		);
		if((bomb_frame == FRAME_SCROLL_RESUME) && stage_scrolls()) {
			graph_scrollup(scroll_line);
		}
	} else {
		bombing = false;
		palette_settone_deferred(TONE_NORMAL);
	}
	bomb_frame++;
}


/// The eight shrinking circles that Mima's bomb sprays around the player, one
/// spawned every [SPAWN_PERIOD] frames into a slot picked by the frame number,
/// each one closing in on the beam's mouth from wherever the player was.
/// C++ linkage for the reason yuuka_heart_update_and_render() below gives.
void near mima_circles_update_and_render(void)
{
	enum {
		// bomb_mima() starts calling mima_bomb_update_and_render() on frame
		// 0, so this is its second frame — the one where the slots are
		// cleared, before the first spawn on frame 4.
		FRAME_RESET = 1,

		// One circle every 4th frame, into slot
		// `((bomb_frame - 16) / 4) & 7`. That is 8 distinct slots over 32
		// frames and then a wrap, so a circle that has not shrunk away within
		// 32 frames is overwritten — and the first four spawns, on frames 4
		// to 12, land in slots -3 to 0 masked back into 5, 6, 7 and 0.
		SPAWN_PERIOD = 4,
		FRAME_SPAWN_BASE = 16,

		// The radius each circle is spawned at and shrinks from, in the same
		// pixels vector2_at() is fed below rather than the subpixels it
		// declares — [ORBIT_CENTER_X] in yuuka_heart_update_and_render()
		// says more about that. Only half of it is ever drawn.
		DISTANCE_START = 256,
		DISTANCE_PER_FRAME = 16,
		DISTANCE_MIN = 4,

		// The beam's mouth: the point every circle converges on, and the same
		// coordinates mima_bomb_update_and_render()'s own ring uses.
		ORBIT_CENTER_X = 224,
		ORBIT_CENTER_Y = 117,

		TONE_NORMAL = 100,
	};

	// SI and DI. The clear loop above indexes [bomb_anim] by [i] instead,
	// which is why it is spelled as a subscript and this one as a pointer.
	mima_circle_t near *circle;
	int i;

	if(bomb_frame == FRAME_RESET) {
		palette_settone_deferred(TONE_NORMAL);
		i = 0;
		while(i < MIMA_CIRCLE_COUNT) {
			bomb_anim.mima[i].center.x = PIXEL_NONE;
			i++;
		}
	}

	// kb/codegen/0128, as in yuuka_bomb_update_and_render() below: a modulo on
	// an `unsigned char` promotes to a signed `int` first, so the original is
	// `cwd`/`idiv` and not a mask. The `/ SPAWN_PERIOD` on the next line is
	// the same division again, recomputed rather than reused, because the
	// original recomputes it.
	if((bomb_frame % SPAWN_PERIOD) == 0) {
		i = (
			((bomb_frame - FRAME_SPAWN_BASE) / SPAWN_PERIOD) &
			(MIMA_CIRCLE_COUNT - 1)
		);
		circle = &bomb_anim.mima[i];

		// Just so that it is not PIXEL_NONE and the render loop below picks
		// the slot up: vector2_at() overwrites both coordinates on this very
		// frame, so the value copied here never reaches the screen. One
		// 32-bit move, spelled the way `SPPoint`'s own set_long() member
		// spells its own (th01/math/subpixel.hpp:104) — and the reason it has
		// to be spelled at all is that the destination is pixel-typed while
		// [player_pos] is not.
		reinterpret_cast<uint32_t near &>(circle->center) = (
			reinterpret_cast<uint32_t near &>(player_pos.cur)
		);

		circle->distance = DISTANCE_START;
		circle->angle = randring1_next16();
	}

	circle = bomb_anim.mima;
	i = 0;
	while(i < MIMA_CIRCLE_COUNT) {
		if(circle->center.x != PIXEL_NONE) {
			vector2_at(
				*reinterpret_cast<SPPoint near *>(&circle->center),
				ORBIT_CENTER_X,
				ORBIT_CENTER_Y,
				circle->distance,
				circle->angle
			);
			grcg_circlefill(
				circle->center.x, circle->center.y,
				(circle->distance / 2)
			);
			circle->distance -= DISTANCE_PER_FRAME;
			if(circle->distance < DISTANCE_MIN) {
				circle->center.x = PIXEL_NONE;
			}
		}
		i++;
		circle++;
	}
}

/// Mima's per-frame bomb animation: the BOMB?.CDG background, the playfield
/// margins around it, the two-tone vertical beam through them, the circles,
/// and the palette. See yuuka_bomb_update_and_render() below for the naming.
void near mima_bomb_update_and_render(void)
{
	enum {
		// Mima's background is inset by this much on all four sides:
		// 320×272 of the 384×368 playfield.
		BOMB_BG_LEFT = 32,
		BOMB_BG_TOP = 48,

		// The vertical beam, drawn into the [BOMB_BG_TOP]-row bands above and
		// below the background that bomb_bg_margins_fill_mima() leaves white.
		// Four 1-pixel lines, an inner pair in [COL_BEAM_INNER] and an outer
		// pair one pixel further out in [COL_BEAM_OUTER], so the white gap
		// between them reads as a 28-pixel-wide beam with a two-step edge.
		BEAM_INNER_LEFT = 209,
		BEAM_INNER_RIGHT = 238,
		BEAM_OUTER_LEFT = (BEAM_INNER_LEFT - 1),
		BEAM_OUTER_RIGHT = (BEAM_INNER_RIGHT + 1),
		COL_BEAM_INNER = 7,
		COL_BEAM_OUTER = 6,

		// The single ring that closes in on the beam's mouth over the whole
		// bomb. bomb_mima() stops calling this function at its own
		// [FRAME_FADE] of 64, so the radius only ever reaches 160.
		RING_CENTER_X = 224,
		RING_CENTER_Y = 117,
		RING_R_START = 224,

		TONE_NORMAL = 100,
	};

	// SI. `pixel_t` and therefore signed, which is what makes the `> 0` test
	// below an `or si, si` / `jle` rather than an unsigned one — grcg_circle()
	// takes an `unsigned` radius and would wrap.
	pixel_t ring_r;

	cdg_put_noalpha_8(
		(PLAYFIELD_LEFT + BOMB_BG_LEFT),
		(PLAYFIELD_TOP + BOMB_BG_TOP),
		CDG_BG_PLAYCHAR_BOMB
	);
	bomb_bg_margins_fill_mima();

	// master.lib's own grcg_setcolor() for the inner pair and
	// th04/hardware/grcg.hpp's mode-preserving grcg_setcolor_direct() for the
	// outer one and for the circles — which is exactly what the original
	// does, and the reason the first of the three is the only one that names a
	// GRCG mode.
	grcg_setcolor(GC_RMW, COL_BEAM_INNER);
	#define beam_vlines(x) \
		grcg_vline( \
			(x), PLAYFIELD_TOP, (PLAYFIELD_TOP + BOMB_BG_TOP - 1) \
		); \
		grcg_vline( \
			(x), (PLAYFIELD_BOTTOM - BOMB_BG_TOP), (PLAYFIELD_BOTTOM - 1) \
		);

	beam_vlines(BEAM_INNER_LEFT);
	beam_vlines(BEAM_INNER_RIGHT);
	grcg_setcolor_direct(COL_BEAM_OUTER);
	beam_vlines(BEAM_OUTER_LEFT);
	beam_vlines(BEAM_OUTER_RIGHT);
	#undef beam_vlines

	grcg_setcolor_direct(V_WHITE);
	mima_circles_update_and_render();

	ring_r = (RING_R_START - bomb_frame);
	if(ring_r > 0) {
		grcg_circle(RING_CENTER_X, RING_CENTER_Y, ring_r);
	}
	grcg_off_clobbering_dx();

	// Unlike the other three playchars, Mima's tone rises linearly with
	// [bomb_frame] for the whole animation instead of walking back down from
	// a flash, and it is bomb_mima() rather than this function that turns it
	// around at [FRAME_FADE].
	palette_settone_deferred(bomb_frame + TONE_NORMAL);
}

extern "C" void pascal near bomb_mima(void)
{
	enum {
		// Mima's bomb has no BB?.BB dissolve-in, so the scroll stops two
		// frames in rather than after one — bomb_marisa() uses the same 2.
		FRAME_SCROLL_STOP = 2,

		// The bomb ends: the scroll resumes, items stop being pulled in, and
		// the tone walks from [TONE_START] back down towards master.lib's
		// neutral [TONE_NORMAL], 6 units per frame until [FRAME_END]. Twice
		// bomb_yuuka()'s rate over half as many frames.
		FRAME_FADE = 64,
		FRAME_END = 80,

		TONE_START = 192,
		TONE_PER_FRAME = 6,
		TONE_NORMAL = 100,

		// The one frame on which the scroll is restarted, two frames into the
		// fade rather than one — bomb_yuuka() uses +1 for the same site.
		FRAME_SCROLL_RESUME = (FRAME_FADE + 2),
	};

	if(bomb_frame < FRAME_FADE) {
		if(bomb_frame == FRAME_SCROLL_STOP) {
			scroll_active = false;
			graph_scrollup(0);
			bg_render_bombing = nullfunc_near;
		}
		mima_bomb_update_and_render();
		bomb_dream_decay();
	} else if(bomb_frame == FRAME_FADE) {
		snd_se_play(15);
		if(stage_scrolls()) {
			scroll_active = true;
		}
		items_pull_to_player = false;

		// Forward `goto` into the next arm, for the reason bomb_yuuka() below
		// gives for its own two.
		goto restore;
	} else if(bomb_frame < FRAME_END) {
restore:
		bg_render_bombing = bg_render_bombing_func;
		palette_settone_deferred(
			TONE_START - ((bomb_frame - FRAME_FADE) * TONE_PER_FRAME)
		);
		if((bomb_frame == FRAME_SCROLL_RESUME) && stage_scrolls()) {
			graph_scrollup(scroll_line);
		}
	} else {
		bombing = false;
		palette_settone_deferred(TONE_NORMAL);
	}
	bomb_frame++;
}

/// The one heart sprite that Yuuka's bomb blits 16 times per frame, on a ring
/// that widens by [DISTANCE_PER_FRAME] pixels every frame. C++ linkage and no
/// `extern "C"`, matching reimu_stars_update_and_render() — the dump published
/// that one MANGLED and gave this one, marisa_lasers_…() and mima_circles_…()
/// no `public` at all, and after this parcel nothing outside this object
/// references any of the four.
void near yuuka_heart_update_and_render(void)
{
	enum {
		// One sprite, blitted at 16 evenly spaced angles — 0x100 / 0x10.
		HEART_COUNT = 16,
		ANGLE_PER_HEART = 0x10,

		// …and the whole ring rotates by this much per frame, which is what
		// makes it a spiral rather than a pulsing circle.
		ANGLE_PER_FRAME = 2,

		// The ring's radius, seeded on the first frame bomb_yuuka() calls
		// yuuka_bomb_update_and_render() and growing from there for as long
		// as it keeps doing so. [FRAME_BOMB] is that function's own.
		FRAME_BOMB = 32,
		DISTANCE_START = 16,
		DISTANCE_PER_FRAME = 2,

		// The ring's center. In *pixels*, which is what the dump's own
		// `; No subpixels!` annotation on this argument pair records:
		// vector2_at() declares subpixels, but the length below and the
		// result are pixels too, so the arithmetic is self-consistent and
		// only the declared types disagree.
		ORBIT_CENTER_X = 192,
		ORBIT_CENTER_Y = 160,

		// The sprite is 32×48 and the clip below tests its top-left corner,
		// so the bottom and right bounds carry its extent.
		HEART_W = 32,
		HEART_H = 48,
	};

	// SI, and the only reason this function has a stack frame at all.
	int i;

	// kb/codegen/0032: both angle advances keep the addition in AL and store
	// afterwards, where a plain `bomb_anim.yuuka.angle += …` compiles to a
	// single `add byte ptr [mem], imm8` — 5 bytes against 8, twice over, and
	// the whole difference between this body and the original.
	#define angle_advance(by) \
		_AL = bomb_anim.yuuka.angle; \
		_AL += (by); \
		bomb_anim.yuuka.angle = _AL;

	if(bomb_frame == FRAME_BOMB) {
		bomb_anim.yuuka.distance = DISTANCE_START;
	}

	// A `while` and not a `for`, because the original's `inc si` comes
	// *before* the angle advance: both are ordinary statements at the end of
	// the body, which is also how reimu_stars_update_and_render() in
	// th05/main/player/bombanim.cpp spells its own trail loop.
	i = 0;
	while(i < HEART_COUNT) {
		// One `screen_point_t` reinterpreted as the `SPPoint` vector2_at()
		// declares, the way th04/main/player/shot.hpp:92 and
		// th02/main/spark.cpp:22 reinterpret theirs. The two layouts are the
		// same four bytes; see [ORBIT_CENTER_X] above for why the values in
		// them are pixels either way.
		vector2_at(
			*reinterpret_cast<SPPoint near *>(&bomb_anim.yuuka.topleft),
			ORBIT_CENTER_X,
			ORBIT_CENTER_Y,
			bomb_anim.yuuka.distance,
			bomb_anim.yuuka.angle
		);

		// ZUN quirk: the two lower bounds are 0 rather than the playfield's
		// own left and top edge, so a heart whose top-left corner lands
		// between the two is still blitted — outside the playfield, into the
		// HUD. It cannot actually happen from [ORBIT_CENTER_X] and
		// [ORBIT_CENTER_Y] with the radii bomb_yuuka() allows, which is
		// presumably why it was never noticed. [inferred]
		//
		// Tested Y-last and X-first, which is the opposite order from
		// th02/main/playfld.hpp's playfield_encloses_*() pairs, so this is
		// spelled out rather than reduced to one of them.
		if(
			(bomb_anim.yuuka.topleft.x >= 0) &&
			(bomb_anim.yuuka.topleft.x <= (PLAYFIELD_RIGHT - HEART_W)) &&
			(bomb_anim.yuuka.topleft.y >= 0) &&
			(bomb_anim.yuuka.topleft.y <= (PLAYFIELD_BOTTOM - HEART_H))
		) {
			super_roll_put_1plane(
				bomb_anim.yuuka.topleft.x,
				bomb_anim.yuuka.topleft.y,
				PAT_PLAYCHAR_BOMB_SHAPE,
				0,
				super_plane(V_WHITE)
			);
		}
		i++;
		angle_advance(ANGLE_PER_HEART);
	}
	angle_advance(ANGLE_PER_FRAME);
	bomb_anim.yuuka.distance += DISTANCE_PER_FRAME;

	#undef angle_advance
}

/// Yuuka's per-frame bomb animation: the BOMB?.CDG background, the playfield
/// margins around it, the hearts, and the palette. Named after its three
/// already-named siblings in this same segment
/// (…_stars_/…_lasers_/…_circles_/yuuka_heart_update_and_render), and after
/// the four-way reading state/notes/bomb_dream_decay.md arrived at
/// independently from this function's own call site list.
extern "C" void near yuuka_bomb_update_and_render(void)
{
	enum {
		// Yuuka's background is the 256-pixel-wide center of the playfield,
		// and covers its full height; bomb_bg_margins_fill_yuuka() paints the
		// 64 columns on either side of it.
		BOMB_BG_LEFT = 64,

		// The [bomb_frame] at which bomb_yuuka() starts calling this
		// function, i.e. that function's own [FRAME_BOMB].
		FRAME_BOMB = 32,

		// Until here, the palette tone walks down from [TONE_START] to
		// master.lib's neutral 100 — (64 - 32) * 3 == 96. After it, the tone
		// is flashed up and back down on a [FLASH_PERIOD]-frame cycle
		// instead, for as long as bomb_yuuka() keeps calling this function.
		FRAME_FADE_END = 64,
		TONE_START = 196,
		TONE_PER_FRAME = 3,

		FLASH_PERIOD = 4,
		FLASH_UP_ON = 0,
		FLASH_DOWN_ON = 2,
		TONE_FLASH = 150,
		TONE_NORMAL = 100,

		// The growing circles Yuuka's bomb spawns are 9. Set on every frame
		// rather than once, the way TH04's two playchar bomb functions also
		// set it: [circles_color] is a single global shared with every other
		// circle spawner in the binary.
		COL_CIRCLES = 9,
	};

	cdg_put_noalpha_8(
		(PLAYFIELD_LEFT + BOMB_BG_LEFT), PLAYFIELD_TOP, CDG_BG_PLAYCHAR_BOMB
	);
	bomb_bg_margins_fill_yuuka();
	yuuka_heart_update_and_render();
	circles_color = COL_CIRCLES;
	if(bomb_frame <= FRAME_FADE_END) {
		palette_settone_deferred(
			TONE_START - ((bomb_frame - FRAME_BOMB) * TONE_PER_FRAME)
		);

	// kb/codegen/0128: a modulo and not a mask, because [bomb_frame] is an
	// `unsigned char` that promotes to a *signed* `int` first — the same
	// reason bomb_dream_decay() two files over spells its own two tests that
	// way. Both arms recompute it, because the original does: `mov bx, 4` and
	// the `idiv` after it appear twice.
	} else if((bomb_frame % FLASH_PERIOD) == FLASH_UP_ON) {
		snd_se_play(9);
		PaletteTone = TONE_FLASH;

		// The shared palette_show() call is a forward `goto` into the next
		// arm, for the reason bomb_yuuka() below gives for its own two.
		goto show;
	} else if((bomb_frame % FLASH_PERIOD) == FLASH_DOWN_ON) {
		PaletteTone = TONE_NORMAL;
show:
		palette_show();
	}
}

extern "C" void pascal near bomb_yuuka(void)
{
	enum {
		// The BB?.BB dissolve-in, 16 cels at 2 frames each. TH04 splits the
		// same 16 cels across two rates; TH05 does not.
		BB_CEL_FRAMES = 2,

		// The bomb proper starts: the scroll stops, the background renderer
		// is disabled, and …_bomb_update_and_render() takes over.
		FRAME_BOMB = 32,

		// The bomb ends: the scroll resumes, items stop being pulled in, and
		// the tone walks from [TONE_START] back down towards master.lib's
		// neutral [TONE_NORMAL], 3 units per frame until [FRAME_END].
		FRAME_FADE = 160,
		FRAME_END = 192,

		TONE_START = 192,
		TONE_PER_FRAME = 3,
		TONE_NORMAL = 100,
	};

	if(bomb_frame < FRAME_BOMB) {
		tiles_bb_col = V_WHITE;
		bb_playchar_put(bomb_frame / BB_CEL_FRAMES);
	} else if(bomb_frame == FRAME_BOMB) {
		scroll_active = false;
		graph_scrollup(0);
		bg_render_bombing = nullfunc_near;

		// Both of this function's shared tails are spelled as a forward
		// `goto` into the *next* arm, which is what the original's two `jmp`s
		// over an intervening `cmp` are. Same shape as TH04's
		// bomb_update_and_render(), and the same reason not to write the
		// statements out twice: Turbo C++'s tail merger stops at the first
		// conditional branch, so the [restore] block would only partly merge.
		goto playchar_bomb;
	} else if(bomb_frame < FRAME_FADE) {
playchar_bomb:
		yuuka_bomb_update_and_render();
		bomb_dream_decay();
	} else if(bomb_frame == FRAME_FADE) {
		snd_se_play(15);
		if(stage_scrolls()) {
			scroll_active = true;
		}
		items_pull_to_player = false;
		goto restore;
	} else if(bomb_frame < FRAME_END) {
restore:
		bg_render_bombing = bg_render_bombing_func;
		palette_settone_deferred(
			TONE_START - ((bomb_frame - FRAME_FADE) * TONE_PER_FRAME)
		);
		if((bomb_frame == (FRAME_FADE + 1)) && stage_scrolls()) {
			graph_scrollup(scroll_line);
		}
	} else {
		bombing = false;
		palette_settone_deferred(TONE_NORMAL);
	}
	bomb_frame++;
}

#undef stage_scrolls
#undef grcg_off_clobbering_dx

// The whole-screen page fill, at the end of BOMB_BG_TEXT -- the head half of
// what used to be MB_INV_TEXT's root contribution, split off by this parcel's
// kb/codegen/0080 carve so that a C++ object can append to it. It is not part
// of the bomb code at all; it is here because this object is the one that owns
// the segment, and giving it a translation unit of its own would have cost a
// Tupfile.lua line for one function (kb/codegen/0155).
//
// Bodies go into THIS block in original address order too, and this one is the
// highest address in the carved head, so anything lifted out of the five GRCG
// fills still above it in the dump goes AHEAD of it here.
//
// `-k-` because the original has no stack frame at all: its first instruction
// is the GRCG mode `out`, and the `push di` in the body is raw bytes rather
// than the compiler's own (kb/codegen/0050).
#pragma codeseg BOMB_BG_TEXT main_01
#pragma option -k-

/// The playfield margins each BOMB?.CDG does not cover
/// ---------------------------------------------------
/// One per playchar, called from the …_bomb_update_and_render() above whose
/// backdrop image it frames, and all four are kb/codegen/0050 BYTE ISLANDS:
/// every instruction that names DI is raw bytes, because the target saves DI
/// as its FIRST instruction and restores it BEFORE its four closing GRCG-off
/// bytes. Turbo C++'s own save would be in the right place, but its own
/// RESTORE always lands after the last statement of the function, i.e. after
/// the GRCG-off — so the compiler cannot be allowed to know DI is used at all.
/// What is left in readable inline assembly is exactly what kb/codegen/0050
/// permits: the port writes, which name no index register, and the loops'
/// backward branches, which need a label.
///
/// The 32-bit stores are raw bytes for the second reason that entry gives:
/// Turbo C++ 4.0J's inline assembler is 16-bit only (kb/codegen/0133), so
/// `MOV ES:[DI+disp], EAX` and `STOSD` have no spelling in it. EAX's upper
/// half is never loaded, in any of them — in the GRCG's TDW mode the CPU's
/// written data is ignored entirely and only the tile registers reach VRAM,
/// so a 32-bit store is purely a way to advance four bytes at a time.
///
/// These five were the last of ZUN's code in BOMB_BG_TEXT.

#define PUSH_DI() _asm { db 0x57; }
#define POP_DI()  _asm { db 0x5F; }
#define MOV_DI(imm) __emit__( \
	0xBF, (uint8_t)((imm) & 0xFF), (uint8_t)(((imm) >> 8) & 0xFF) \
)
#define SUB_DI(imm) __emit__(0x83, 0xEF, (uint8_t)(imm))
#define STOSD() __emit__(0x66, 0xAB)
#define STOSW() __emit__(0xAB)
#define REP_STOSD() __emit__(0xF3, 0x66, 0xAB)

// MOV ES:[DI], AX and MOV ES:[DI + a signed byte displacement], AX, and their
// 32-bit forms. ModR/M 05h addresses through DI with no displacement, 45h with
// a one-byte one; the 66h prefix widens the AX operand to EAX and the 26h one
// is the ES segment override.
#define MOV_ES_DI_AX()          __emit__(      0x26, 0x89, 0x05)
#define MOV_ES_DI_D_AX(disp)    __emit__(      0x26, 0x89, 0x45, (uint8_t)(disp))
#define MOV_ES_DI_EAX()         __emit__(0x66, 0x26, 0x89, 0x05)
#define MOV_ES_DI_D_EAX(disp)   __emit__(0x66, 0x26, 0x89, 0x45, (uint8_t)(disp))

// GRCG on, in TDW mode, with all four tile registers set to one color, and
// with the previous interrupt state restored rather than unconditionally
// reenabled — which is why none of th04/hardware/grcg.hpp's helpers fits.
// The color is not a parameter because the target writes the four tile
// registers by REUSING one AL, so the instruction sequence is a function of
// the color's bit pattern: 15 needs no second load at all, 1 and 14 need one
// `NOT AL` after the first `OUT`, and 14's first value is the `32 C0` spelling
// of zero rather than `MOV AL, 0` (kb/codegen 0037 + 0061). 80h is GC_TDW,
// hardcoded the way grcg_setmode_rmw_inlined() hardcodes 0C0h.
#define GRCG_TDW_NOINT_HEAD() _asm { \
	pushf; \
	cli; \
	mov 	al, 0x80; /* GC_TDW */ \
	out 	0x7C, al; \
	mov 	dx, 0x7E; \
}
#define GRCG_TDW_COL_15_NOINT() { \
	GRCG_TDW_NOINT_HEAD(); \
	_asm { mov al, 0xFF; out dx, al; out dx, al; out dx, al; out dx, al; popf; } \
}
#define GRCG_TDW_COL_1_NOINT() { \
	GRCG_TDW_NOINT_HEAD(); \
	_asm { \
		mov 	al, 0xFF; \
		out 	dx, al; \
		not 	al; \
		out 	dx, al; \
		out 	dx, al; \
		out 	dx, al; \
		popf; \
	} \
}
#define GRCG_TDW_COL_14_NOINT() { \
	GRCG_TDW_NOINT_HEAD(); \
	_asm { \
		db  	0x32, 0xC0; /* XOR AL, AL */ \
		out 	dx, al; \
		not 	al; \
		out 	dx, al; \
		out 	dx, al; \
		out 	dx, al; \
		popf; \
	} \
}

// GRCG off, in the four-byte inline spelling the target uses rather than a
// call to master.lib's GRCG_OFF (kb/codegen/0061).
#define GRCG_OFF_INLINE() _asm { \
	db  	0x32, 0xC0; /* XOR AL, AL */ \
	out 	0x7C, al; \
}

// The VRAM segment for playfield row [y], as all five functions below spell it.
#define PLAYFIELD_ROW_SEG(y) ( \
	SEG_PLANE_B + ((((y) + PLAYFIELD_TOP) * ROW_SIZE) / 16) \
)

// 48 columns down either edge of the playfield, over every row, in white.
extern "C" void near bomb_bg_margins_fill_reimu(void)
{
	PUSH_DI();
	GRCG_TDW_COL_15_NOINT();
	_ES = PLAYFIELD_ROW_SEG(0);
	MOV_DI(((PLAYFIELD_H - 1) * ROW_SIZE) + PLAYFIELD_VRAM_LEFT);
row:
	// The right edge first, then the left one via the STOSD/STOSW that also
	// walk DI back to the start of this row's left margin.
	MOV_ES_DI_D_EAX(352 / 8);
	STOSD();
	MOV_ES_DI_D_AX(304 / 8);
	STOSW();
	SUB_DI(ROW_SIZE + 6);
	_asm { jge short row; }
	POP_DI();
	GRCG_OFF_INLINE();
}

// No pad after this one: its body is 30h bytes, an even length. The three
// pads further down sit on exactly the three odd-length bodies.

// 80 columns down either edge, over every row, in 1.
extern "C" void near bomb_bg_margins_fill_marisa(void)
{
	PUSH_DI();
	GRCG_TDW_COL_1_NOINT();
	_ES = PLAYFIELD_ROW_SEG(0);
	MOV_DI(((PLAYFIELD_H - 1) * ROW_SIZE) + PLAYFIELD_VRAM_LEFT);
row:
	MOV_ES_DI_D_EAX(320 / 8);
	MOV_ES_DI_D_EAX(352 / 8);
	STOSD();
	STOSD();
	MOV_ES_DI_D_AX(240 / 8);
	STOSW();
	SUB_DI(ROW_SIZE + 10);
	_asm { jge short row; }
	POP_DI();
	GRCG_OFF_INLINE();
}

#pragma codestring "\x90"

// The 48 rows below ES:0000, filled in two 176-pixel-wide horizontal bands
// with a 32-pixel gap between them — the gap that
// mima_bomb_update_and_render() then draws its beam into. Mima's fill is the
// only one of the four that needs a helper, because it is the only one that
// runs the same pass over two different VRAM pages, and DI is both its input
// and its scratch, which is why it takes no parameters and why the whole body
// is bytes.
//
// [inferred] name, and it OWES A NAMING ROUND: the dump gave it no `public`
// and no comment, its only two callers are inside bomb_bg_margins_fill_mima()
// below, and nothing outside this file has ever referenced it.
void near bomb_bg_mima_bands_fill_48(void)
{
	MOV_DI((47 * ROW_SIZE) + PLAYFIELD_VRAM_LEFT);
left:
	_CX = (160 / (4 * 8));
	REP_STOSD();
	MOV_ES_DI_AX();
	SUB_DI((160 / 8) + ROW_SIZE);
	_asm { jge short left; }

	MOV_DI((47 * ROW_SIZE) + (PLAYFIELD_VRAM_LEFT + (208 / 8)));
right:
	_CX = (160 / (4 * 8));
	REP_STOSD();
	MOV_ES_DI_AX();
	SUB_DI((160 / 8) + ROW_SIZE);
	_asm { jge short right; }
}

#pragma codestring "\x90"

// 48 rows along the top and bottom edges and 32 columns down either edge of
// what is left, in 1 — plus the same 32-pixel gap in white, which is what the
// second GRCG setup in the middle of this function is for.
extern "C" void near bomb_bg_margins_fill_mima(void)
{
	PUSH_DI();
	GRCG_TDW_COL_1_NOINT();

	// The bottom 48 rows, then the top 48. Both on the same page as far as
	// the CPU is concerned; the segment is what selects the band.
	_ES = PLAYFIELD_ROW_SEG(320);
	bomb_bg_mima_bands_fill_48();
	_ES = PLAYFIELD_ROW_SEG(0);
	bomb_bg_mima_bands_fill_48();

	// The 32 columns down either edge of the 272 rows in between.
	_ES = PLAYFIELD_ROW_SEG(48);
	MOV_DI((271 * ROW_SIZE) + PLAYFIELD_VRAM_LEFT);
side:
	MOV_ES_DI_EAX();
	MOV_ES_DI_D_EAX(352 / 8);
	SUB_DI(ROW_SIZE);
	_asm { jge short side; }

	// Back to white, for the 32-pixel gap in the two horizontal bands. The
	// GC_TDW mode `out` is not repeated, only the four tile registers.
	_asm {
		pushf;
		cli;
		mov 	dx, 0x7E;
		mov 	al, 0xFF;
		out 	dx, al;
		out 	dx, al;
		out 	dx, al;
		out 	dx, al;
		popf;
	}
	_ES = PLAYFIELD_ROW_SEG(320);
	MOV_DI((47 * ROW_SIZE) + (208 / 8));
gap_bottom:
	STOSD();
	SUB_DI(ROW_SIZE + 4);
	_asm { jge short gap_bottom; }

	_ES = PLAYFIELD_ROW_SEG(0);
	MOV_DI((47 * ROW_SIZE) + (208 / 8));
gap_top:
	STOSD();
	SUB_DI(ROW_SIZE + 4);
	_asm { jge short gap_top; }

	POP_DI();
	GRCG_OFF_INLINE();
}

#pragma codestring "\x90"

// 64 columns down either edge, over every row, in 14.
extern "C" void near bomb_bg_margins_fill_yuuka(void)
{
	PUSH_DI();
	GRCG_TDW_COL_14_NOINT();
	_ES = PLAYFIELD_ROW_SEG(0);
	MOV_DI(((PLAYFIELD_H - 1) * ROW_SIZE) + PLAYFIELD_VRAM_LEFT);
row:
	MOV_ES_DI_D_EAX(320 / 8);
	MOV_ES_DI_D_EAX(352 / 8);
	STOSD();
	STOSD();
	SUB_DI(ROW_SIZE + 8);
	_asm { jge short row; }
	POP_DI();
	GRCG_OFF_INLINE();
}

// The shared body below defines its own copies of these four, so they are
// retired here rather than left to an identical redefinition.
#undef PUSH_DI
#undef POP_DI
#undef REP_STOSD
#undef GRCG_OFF_INLINE

#include "th04/main/graph2pg.cpp"

#pragma option -k.
#pragma codeseg
