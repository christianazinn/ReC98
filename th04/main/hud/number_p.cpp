/// TH04's two bold-gaiji number renderers
/// --------------------------------------
/// Both build their digits in a stack buffer and hardcode TX_WHITE, which is
/// the whole of what separates them from TH05's same-named pair in
/// th05/main/hud/number_p.asm -- that game renders through the shared
/// [hud_gaiji_row] and, for the 5-digit one, takes the attribute as a
/// parameter. th04/main/hud/hud.hpp already carried that split for
/// hud_5_digit_put(); this parcel adds hud_points_put() to the same `#else`.
///
/// One file for both, because they are adjacent in the dump and because TH05
/// keeps its pair in one module too; the file name is TH05's.
///
/// Not compiled on its own: th04/hudnum.cpp is its object.

/// Prints [points] using the bold gaiji font, right-aligned at the fixed
/// column 34 of row [y], in white, with a hardcoded trailing `0` that makes
/// every figure read as a multiple of 10 -- the same trick TH05's
/// hud_points_put() plays with its "continues used" digit. Only the two stage
/// bonus tallies call it, and both do so from inline assembly, because the
/// running total is still in EAX where they call from (kb/codegen/0122).
extern "C" void pascal near hud_points_put(utram_y_t y, unsigned long points)
{
	// Declaration order is load-bearing: [str] is the buffer at [bp-0Ah],
	// [divisor] the dword at [bp-0Eh] and [digit] the one at [bp-12h], which
	// is the order the original's 18-byte stack frame allocates them.
	// [i] and [nonzero] are the two register variables, SI then DI -- again
	// declaration order, and the divisor can never be one because it is a
	// dword.
	gaiji_th04_t str[10];
	unsigned long divisor;
	unsigned long digit;
	int i;
	int nonzero;

	divisor = 1000000;
	i = 0;
	nonzero = 0;
	while(divisor > 1) {
		// Two statements, not one `ldiv()`: the original divides twice, which
		// is exactly what a separate `/` and `%` over the same operands emit.
		digit = (points / divisor);
		points = (points % divisor);

		// [nonzero] is an `int` and [digit] a `long`, so this takes the low
		// word only -- and it is a separate statement from the test below,
		// which is why the original re-tests DI with `or di, di` after an
		// `or` that has already set the flags.
		nonzero |= digit;
		if(nonzero) {
			str[i] = (gaiji_th04_t)(gb_0 + digit);
		} else {
			str[i] = g_EMPTY;
		}
		divisor /= 10;
		i++;
	}
	str[6] = (gaiji_th04_t)(gb_0 + points);
	str[7] = gb_0;
	str[8] = g_NULL;
	gaiji_putsa(34, y, (const char *)str, TX_WHITE);
}

/// Prints [val] using the bold gaiji font, right-aligned at ([left+8], [y]),
/// in white.
void pascal hud_5_digit_put(utram_x_t left, utram_y_t y, uint16_t val)
{
	// Same rule as above: [str] at [bp-6], [digit] at [bp-8] and [nonzero] at
	// [bp-0Ah]. Here the divisor is a word, so it takes SI and the index
	// takes DI -- the mirror of the function above, and the reason the other
	// two locals are memory-homed rather than enregistered.
	gaiji_th04_t str[6];
	int digit;
	int nonzero;
	unsigned int divisor;
	int i;

	divisor = 10000;
	i = 0;
	nonzero = 0;
	while(divisor > 1) {
		digit = (val / divisor);
		val = (val % divisor);
		nonzero |= digit;
		if(nonzero != 0) {
			str[i] = (gaiji_th04_t)(gb_0 + digit);
		} else {
			str[i] = g_EMPTY;
		}
		divisor /= 10;
		i++;
	}
	str[4] = (gaiji_th04_t)(gb_0 + val);
	str[5] = g_NULL;
	gaiji_putsa(left, y, (const char *)str, TX_WHITE);
}
