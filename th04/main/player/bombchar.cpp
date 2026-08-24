/// The playchar-specific bomb animations
/// -------------------------------------
/// The two functions main() picks between when it sets [playchar_bomb_func]
/// (th04_main.asm, `mov _playchar_bomb_func, offset bomb_reimu` and
/// `offset bomb_marisa`). bomb_update_and_render() calls the chosen one on
/// every frame of the bomb proper, from [FRAME_BOMB] until [FRAME_FADE].
///
/// Both have the same three-part shape: fill the playfield rows that the
/// BOMB?.CDG background does not cover with a playchar-specific color, put
/// that background back on top, and hand the shared star field to
/// bomb_stars_update_and_render_for(). What differs is the two colors, the
/// tone the palette fades down from, and where the growing circles are
/// spawned.
///
/// (#included from th04/main/player/shots_inv.cpp, ahead of bombupd.cpp,
/// bombanim.cpp and that file's own two functions, which is this segment's
/// original address order: this body is the tail of SHOT_INV_TEXT's root
/// contribution and the C++ object begins exactly where it ends.
/// kb/codegen/0129 + 0098 + 0114.)
///
/// Because this file shares a translation unit with shots_inv.cpp, its
/// file-scope names are NOT file-local.
///
/// TH04-only. th05_main.asm publishes its own `BOMB_REIMU` and `BOMB_MARISA`,
/// but they belong to a different subsystem: TH05 has four playchars, its own
/// th05/main/player/bombanim.cpp, and no shared star field.

// master.lib, declared here rather than through libs/master.lib/pc98_gfx.hpp
// for the reason th04/main/player/bombupd.cpp gives for its own copy of this
// same line. That file is #included *after* this one, so its declaration
// cannot serve this one; a repeated `extern` declaration costs nothing.
extern "C" {
	extern unsigned int __cdecl PaletteTone;
}

// The GRCG-off sequence the original inlines around every filled region here:
// `mov dx, 7Ch` / `mov al, 0` / `out dx, al`. Same spelling as
// th04/main/stage/loop.cpp:165 and th04/hiscore/regist_view.cpp:52, except
// that both of those get the 0 from master.lib's GC_OFF in
// libs/master.lib/pc98_gfx.hpp, which this translation unit does not
// #include. Borland's `outportb()` intrinsic always goes through DX, which is
// why the port fits in a byte and the sequence is still 6 bytes long.
#define grcg_off_clobbering_dx() outportb(0x7C, 0 /* master.lib's GC_OFF */)

// A `near` helper in th04_main.asm's main_013_TEXT that fills the playfield
// rows above and below the BOMB?.CDG background with the current GRCG tile.
// Its name is the box it leaves alone, in playfield coordinates: x=0, y=40,
// w=384, h=274, which is exactly [BOMB_BG_TOP] and TH04's [BOMB_BG_H_MAX].
// bomb_reimu() and bomb_marisa() are its only callers in this binary.
//
// The dump gave it no `public` of its own, so the lift needed the zero-byte
// `label near` alias kb/codegen/0123 prescribes; `extern "C"` with the
// project's default cdecl is what spells that alias `_playfield_fillm_…`
// (kb/codegen/0086). The dump's own remaining call site keeps the bare name.
extern "C" void near playfield_fillm_0_40_384_274(void);

// Defined in th04/main/player/bombanim.cpp, which shots_inv.cpp #includes
// after this file — so this is a forward declaration into the same object,
// and the call below is an ordinary `near` one. See that file for why the
// parameter is `int` and not `playchar_t`, and why the name is `extern "C"`.
extern "C" void pascal near bomb_stars_update_and_render_for(int playchar);

// The BOMB?.CDG background, loaded into [CDG_BG_PLAYCHAR_BOMB] by
// dialog_load(). It is [BOMB_BG_H_MAX] (274) rows tall and starts this far
// down the playfield; playfield_fillm_0_40_384_274() paints the 40 rows above
// it and the 54 below.
static const pixel_t BOMB_BG_TOP = 40;

extern "C" void pascal near bomb_reimu(void)
{
	enum {
		// Same five as bomb_marisa() below, with the same meanings, and
		// deliberately not shared: this function is emitted first and each
		// body carries its own copy in the original, which is why both of
		// them reload [bomb_frame] for every one of their own tests.
		FRAME_BOMB = 48,
		FRAME_FLASH_END = 80,
		FRAME_CIRCLES_END = 160,
		TONE_START = 196,
		TONE_PER_FRAME = 3,

		// Colors. Reimu's bomb fills the playfield margins in white and draws
		// the star field in 14; her circles are 9.
		COL_FILL = V_WHITE,
		COL_STARS = 14,
		COL_CIRCLES = 9,

		// The two circles are spawned on a ring this far from the playfield's
		// center, at [angle] and at its mirror image across the vertical
		// axis, so they converge on the player from both sides — the same
		// idea bombanim.cpp's star spawn keeps the middle third clear for.
		CIRCLE_RADIUS = 128,
		ANGLE_MIRROR = 0x80,
	};

	// A byte local at [bp-1], with the frame rounded up to one word.
	// kb/codegen/0131: byte-sized, so no amount of use would have put it in a
	// register — which is also why the two inline-ASM islands below cannot
	// demote anything (kb/codegen/0143).
	unsigned char angle;

	grcg_setmode_tdw();
	grcg_setcolor_direct(COL_FILL);
	playfield_fillm_0_40_384_274();
	grcg_off_clobbering_dx();
	cdg_put_noalpha_8(
		PLAYFIELD_LEFT, (PLAYFIELD_TOP + BOMB_BG_TOP), CDG_BG_PLAYCHAR_BOMB
	);

	if(bomb_frame <= FRAME_FLASH_END) {
		circles_color = COL_CIRCLES;
		palette_settone_deferred(
			TONE_START - ((bomb_frame - FRAME_BOMB) * TONE_PER_FRAME)
		);
	} else if(
		(bomb_frame <= FRAME_CIRCLES_END) && (stage_frame_mod4 == 0)
	) {
		// kb/codegen/0032: the shift stays in AL and the store comes after
		// it, which is what the pseudo-register spelling buys. The byte load
		// from a *word* [stage_frame] is codegen and not a narrowing:
		// `((stage_frame & 0xFF) << 2) & 0xFF` and `(stage_frame << 2) & 0xFF`
		// are the same value. Either way the angle sweeps a full turn every
		// 64 frames, 16 units per circle, since only every 4th frame spawns
		// one.
		_AL = stage_frame;
		_AL <<= 2;
		angle = _AL;

		// kb/codegen/0034 + 0023: AL is still live across the three constant
		// pushes, so the zero-extension has to be part of the last argument
		// expression rather than a statement of its own. A plain `angle`
		// argument would reload it from [bp-1] and add three bytes.
		vector2_at(
			drawpoint,
			TO_SP(PLAYFIELD_W / 2),
			TO_SP(PLAYFIELD_H / 2),
			TO_SP(CIRCLE_RADIUS),
			(_AH = 0, _AX)
		);

		// kb/codegen/0083, as in bomb_marisa() below — except that here the
		// two arguments are memory words rather than register values, so the
		// pushes are `FF 36` against [drawpoint] itself. `_AX = drawpoint.x.v;
		// push ax` is the same 4 bytes and the wrong ones.
		_asm {
			push	word ptr [drawpoint];
			push	word ptr [drawpoint + 2];
			nop;
			push	cs;
			call	near ptr circles_add_growing;
		}

		_AL = ANGLE_MIRROR;
		_AL -= angle;
		angle = _AL;
		vector2_at(
			drawpoint,
			TO_SP(PLAYFIELD_W / 2),
			TO_SP(PLAYFIELD_H / 2),
			TO_SP(CIRCLE_RADIUS),
			(_AH = 0, _AX)
		);
		_asm {
			push	word ptr [drawpoint];
			push	word ptr [drawpoint + 2];
			nop;
			push	cs;
			call	near ptr circles_add_growing;
		}

		snd_se_play(9);
	}

	grcg_setmode_rmw();
	grcg_setcolor_direct(COL_STARS);
	bomb_stars_update_and_render_for(PLAYCHAR_REIMU);
	grcg_off_clobbering_dx();
}

extern "C" void pascal near bomb_marisa(void)
{
	enum {
		// Frame numbers are [bomb_frame], i.e. the same counter
		// bomb_update_and_render() tests; it only reaches this function
		// between that file's [FRAME_BOMB] and [FRAME_FADE].
		FRAME_BOMB = 48,

		// Until here, the palette tone walks back down from [TONE_START] to
		// master.lib's neutral 100 — (80 - 48) * 3 == 96 — and no circles are
		// spawned yet.
		FRAME_FLASH_END = 80,

		// Circles are spawned on every 4th frame between [FRAME_FLASH_END]
		// and here, and the arithmetic below is written in terms of the
		// frames LEFT, hence the +1.
		FRAME_CIRCLES_END = 160,
		FRAME_AFTER_CIRCLES = (FRAME_CIRCLES_END + 1),

		// Halfway through the circle phase, the horizontal spread stops
		// growing with the elapsed frames and starts shrinking with the
		// remaining ones, which is what makes the trail converge.
		FRAME_SPREAD_FLIP = 120,

		TONE_START = 196,
		TONE_PER_FRAME = 3,

		// Colors. Marisa's bomb fills the playfield margins in 1 and draws
		// the star field in 8; the growing circles are white.
		COL_FILL = 1,
		COL_STARS = 8,

		// The trail's slope and its distance from the playfield's left edge,
		// both in pixels per elapsed/remaining frame.
		X_PER_FRAME = 4,
		Y_PER_FRAME = 3,
		SPREAD_PER_FRAME = 8,
		Y_BOTTOM = 40,
	};

	// [x] is the SI register variable and [y] the single word of the frame,
	// which is what four C-level references against two buys
	// here. kb/codegen/0143: the two `push ax` statements below are inline
	// ASM and therefore invisible to that count, but [x] still has enough
	// references left to keep SI.
	pixel_t x;
	pixel_t y;

	grcg_setmode_tdw();
	grcg_setcolor_direct(COL_FILL);
	playfield_fillm_0_40_384_274();
	grcg_off_clobbering_dx();
	cdg_put_noalpha_8(
		PLAYFIELD_LEFT, (PLAYFIELD_TOP + BOMB_BG_TOP), CDG_BG_PLAYCHAR_BOMB
	);

	if(bomb_frame <= FRAME_FLASH_END) {
		circles_color = V_WHITE;
		palette_settone_deferred(
			TONE_START - ((bomb_frame - FRAME_BOMB) * TONE_PER_FRAME)
		);
	} else if(
		(bomb_frame <= FRAME_CIRCLES_END) && (stage_frame_mod4 == 0)
	) {
		x = ((bomb_frame - FRAME_FLASH_END) * X_PER_FRAME);
		y = (
			((FRAME_AFTER_CIRCLES - bomb_frame) * Y_PER_FRAME) + Y_BOTTOM
		);

		// Both arms reduce: the first one is just
		// `randring1_next16_mod(…) - 64`, because the base [x] above and the
		// term subtracted here cancel except for a constant. ZUN did not
		// write it that way, and it shows — Turbo C++ cannot fold across the
		// branch, so the original reloads [bomb_frame] and recomputes
		// `(bomb_frame - …) * 4` in both arms. Spelling out the reduced form
		// would delete four instructions per arm.
		if(bomb_frame < FRAME_SPREAD_FLIP) {
			x += (
				randring1_next16_mod(
					(bomb_frame - FRAME_FLASH_END) * SPREAD_PER_FRAME
				) - ((bomb_frame - 64) * X_PER_FRAME)
			);
		} else {
			x += (
				randring1_next16_mod(
					(FRAME_AFTER_CIRCLES - bomb_frame) * SPREAD_PER_FRAME
				) - ((FRAME_AFTER_CIRCLES - bomb_frame) * X_PER_FRAME)
			);
		}

		// kb/codegen/0083: circles_add_growing() is `far` and lives in
		// another segment of this same group (CIRCLE_TEXT), so a plain C++
		// call emits a real `9A` far call where the original has TLINK's
		// same-frame relaxation, `90 0E E8` — same length, wrong bytes. The
		// arguments therefore have to be pushed by hand too
		// (kb/codegen/0122), and the call names the C identifier rather than
		// its C++-decorated linker spelling (kb/codegen/0014).
		// th05/main/player/bombanim.cpp hand-spells the identical call.
		_AX = TO_SP(x);
		_asm { push ax; }
		_AX = TO_SP(y);
		_asm { push ax; }
		_asm {
			nop;
			push	cs;
			call	near ptr circles_add_growing;
		}

		snd_se_play(9);
	}

	grcg_setmode_rmw();
	grcg_setcolor_direct(COL_STARS);
	bomb_stars_update_and_render_for(PLAYCHAR_MARISA);
	grcg_off_clobbering_dx();
}
