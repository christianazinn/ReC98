/// Remaining-bombs HUD row
/// -----------------------
/// (#included from th04/main/hud/points.cpp, immediately after lives.cpp and
/// ahead of points.cpp's own function, which is these three functions'
/// original address order. No #includes of its own, for the reason lives.cpp
/// states; [HUD_TALLY_GAIJI_W] and the -G pragma both come from there.
/// kb/codegen/0129.)
///
/// The same row as hud_lives_put(), and NOT the same code. The two differ in
/// statement order -- this one zeroes the loop index before the overflow test,
/// hud_lives_put() zeroes it inside the tally branch -- and in the guard's
/// operator: `ja` unsigned on the resident member here, `jge` signed on a byte
/// local there. A single macro cannot emit both, which is why the two rows are
/// written out separately instead of sharing one.
///
/// The cause is visible rather than arbitrary: lives needs `rem_lives - 1` for
/// the test itself, so its count becomes a local before the branch and the
/// index can wait; bombs tests its counter straight out of memory with no
/// arithmetic, so it needs no count local yet and the index initialiser is
/// what lands first.

// The `　　×　　` row printed instead of the tally when the count no longer
// fits (kb/codegen/0123). Byte-for-byte identical to [HUD_LIVES_OVERFLOW],
// which it duplicates rather than shares: ZUN bloat, 11 wasted bytes.
extern "C" const char HUD_BOMBS_OVERFLOW[];

extern "C" void pascal hud_bombs_put(void)
{
	// `signed char`, not plain `char`, for the reason lives.cpp measures out.
	signed char count;
	signed char i;
	int x;

	i = 0;

	// The guard reads the `unsigned char` member itself rather than [count],
	// so this one comparison is unsigned -- `cmp es:[bx+8], 5` / `ja` -- while
	// every other test in the function is signed off [count] and [i]. Both
	// narrow to a byte compare against the same `int` constant; the operand's
	// type is what picks the branch family (kb/codegen/0142, 0095).
	if(resident->rem_bombs <= HUD_TALLY_GAIJI_W) {
		x = HUD_LABELED_LEFT;

		// Reuses the ES:BX the guard above already loaded.
		count = resident->rem_bombs;

		while(i < count) {
			gaiji_putca(x, HUD_BOMBS_Y, gs_BOMB, TX_WHITE);
			x += GAIJI_TRAM_W;
			i++;
		}
		while(i < HUD_TALLY_GAIJI_W) {
			gaiji_putca(x, HUD_BOMBS_Y, g_EMPTY, TX_WHITE);
			x += GAIJI_TRAM_W;
			i++;
		}
	} else {
		// kb/codegen/0002: this arm is a separate basic block, so the
		// original RELOADS `les bx, _resident` here rather than reusing the
		// pointer the guard left live. Spelling the three reads as three
		// separate `resident->rem_bombs` expressions is what reproduces the
		// reuse above and the reload here; one accessor cannot.
		count = resident->rem_bombs;

		text_putsa(
			HUD_LABELED_LEFT, HUD_BOMBS_Y, HUD_BOMBS_OVERFLOW, TX_WHITE
		);
		if(count >= HUD_TALLY_TWO_DIGITS_AT) {
			gaiji_putca(
				(HUD_LABELED_LEFT + (3 * GAIJI_TRAM_W)),
				HUD_BOMBS_Y,
				((count / 10) + gb_0),
				TX_WHITE
			);
			count = (count % 10);
		}
		gaiji_putca(
			(HUD_LABELED_LEFT + (4 * GAIJI_TRAM_W)),
			HUD_BOMBS_Y,
			(count + gb_0),
			TX_WHITE
		);
	}
}

// Back to the translation unit's baseline, so that points.cpp's own
// hud_point_items_put() keeps the `ENTER` prolog it already matched with.
#pragma option -G-
