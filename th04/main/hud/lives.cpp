/// Remaining-lives HUD row
/// -----------------------
/// (#included from th04/main/hud/points.cpp, ahead of its own function, so
/// that this lands at its original address at the tail of HUD_PNT_TEXT.
/// This file deliberately has no #includes of its own: every header it needs
/// is already included by points.cpp, and 12 of the 22 in that closure have
/// no include guard, so including any of them again would be a hard error.
/// kb/codegen/0129.)
///
/// Because this file shares a translation unit with points.cpp and bombs.cpp,
/// its file-scope names are NOT file-local. Every one of them is prefixed.
///
/// Assembly in TH05, which spells the same row against MAIN-local `_lives`
/// instead of the resident structure; see the note for the shared-body
/// arithmetic. The `GAME == 4` guard below is therefore a LIFTING-ORDER
/// artifact, not a game difference -- unlike shot_reset()'s.

// kb/codegen/0011, derived from this function's own prolog and not from a
// neighbour: the original opens `push bp / mov bp, sp / sub sp, 2` and closes
// `pop si / leave / retf`, which is -G. Restored to -G- at the end of
// bombs.cpp, so that points.cpp's own hud_point_items_put() is unaffected.
#pragma option -G

// The tally is [HUD_LABELED_W] cells wide, i.e. this many gaiji. th02's
// hud.cpp derives its own file-local HUD_LABELED_GAIJI_W from exactly these
// two constants, but that is a different translation unit and this one cannot
// reach it -- so derive it the same way rather than respelling the 5.
//
// Deliberately `int`: the constants' type turns out not to matter here at all.
// What decides the encoding is the *local's* type -- see the declarations in
// the function below, and kb/codegen/0142.
static const int HUD_TALLY_GAIJI_W = (HUD_LABELED_W / GAIJI_TRAM_W);

// One past the widest tally that still fits the row: at or above this, the
// count is printed as a number instead.
static const int HUD_TALLY_OVERFLOW_AT = (HUD_TALLY_GAIJI_W + 1);

// At or above this, that number needs its tens digit written as well.
static const int HUD_TALLY_TWO_DIGITS_AT = 10;

// The `　　×　　` row printed instead of the tally when the count no longer
// fits. Zero-byte alias onto th04_main.asm's private `aB@b@bB@b@`
// (kb/codegen/0123); the string itself stays in the dump, because re-emitting
// it from C++ would shift every later byte of _DATA (kb/codegen/0084).
extern "C" const char HUD_LIVES_OVERFLOW[];

extern "C" void pascal hud_lives_put(void)
{
	// Declaration order is load-bearing: [count] is the byte at [bp-1] and
	// [i] the one at [bp-2]. [x] is the enregistered cursor the original
	// keeps in SI, which is why the frame is only 2 bytes.
	//
	// `signed char` rather than plain `char` is the whole of this parcel's
	// codegen content, and the two are NOT interchangeable here even though
	// Turbo C++ makes plain `char` signed (kb/codegen/0142). Measured on this
	// function, with the constants left `int` throughout:
	//
	//   signed char   `cmp byte ptr [bp-1], 6` / `jge`  <- the original
	//   char          `cbw` + `cmp ax, 6` / `jge`       <- widened, +2 bytes
	//   unsigned char `cmp byte ptr [bp-1], 6` / `jae`  <- unsigned branch,
	//                 and `add al, 0FFh` for the decrement below
	signed char count;
	signed char i;
	int x;

	// The row shows the *spare* lives, so one less than the player has.
	count = (resident->rem_lives - 1);

	// Signed, on the local -- `cmp [bp-1], 6` / `jge`. hud_bombs_put()
	// spells the same idea unsigned and on the member; the two functions
	// were written separately and are deliberately not one macro here.
	if(count < HUD_TALLY_OVERFLOW_AT) {
		i = 0;
		x = HUD_LABELED_LEFT;
		while(i < count) {
			gaiji_putca(x, HUD_LIVES_Y, gs_YINYANG, TX_WHITE);
			x += GAIJI_TRAM_W;
			i++;
		}
		// Continues with the same [i]; the blanks pad out the rest of the
		// row rather than restarting it.
		while(i < HUD_TALLY_GAIJI_W) {
			gaiji_putca(x, HUD_LIVES_Y, g_EMPTY, TX_WHITE);
			x += GAIJI_TRAM_W;
			i++;
		}
	} else {
		text_putsa(
			HUD_LABELED_LEFT, HUD_LIVES_Y, HUD_LIVES_OVERFLOW, TX_WHITE
		);
		if(count >= HUD_TALLY_TWO_DIGITS_AT) {
			gaiji_putca(
				(HUD_LABELED_LEFT + (3 * GAIJI_TRAM_W)),
				HUD_LIVES_Y,
				((count / 10) + gb_0),
				TX_WHITE
			);
			count = (count % 10);
		}
		gaiji_putca(
			(HUD_LABELED_LEFT + (4 * GAIJI_TRAM_W)),
			HUD_LIVES_Y,
			(count + gb_0),
			TX_WHITE
		);
	}
}
