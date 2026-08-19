/// HUD bar rendering
/// -----------------
/// The one low-level bar renderer behind the HP, power and (in TH05) dream
/// rows: turns a 0–BAR_MAX value into a gaiji string of up to 8 bar cells plus
/// a terminator, and prints it at ([HUD_LEFT], [y]) in the given attribute.
///
/// TH04 only. TH05's hud_bar_put() shares the name and the semantics and
/// nothing else — it is `near`, takes its arguments in registers through
/// `arg_bx`, and builds the row in the shared [hud_gaiji_row] buffer rather
/// than on the stack (th05/hud_bar.asm).
///
/// Its own object rather than an #include into th04/hud_put.cpp, because this
/// function needs `-G` (its frame is `push bp` / `mov bp, sp` / `sub sp`,
/// kb/codegen/0011) and hud_put() in th04/main/hud/hud.cpp does not. Toggling
/// `-G` back off mid-translation-unit would put hud_put()'s prolog at the
/// mercy of a pragma two files away.

/// This file deliberately has no #includes of its own any more: every header
/// it needs is already included by th04/main/hud/hp_put.cpp, which shares its
/// translation unit and is included ahead of it, and two of the three reach
/// unguarded files (th04/gaiji/gaiji.h and th04/main/hud/hud.hpp, the latter
/// via th02/main/hud/hud.hpp), so including them again would be a hard error.
/// Its file-scope names are therefore NOT file-local. kb/codegen/0129.

// The completely filled bar, as its own gaiji string, in
// th04/main/hud/bar_put[data].asm. Read through the `struct` wrapper that
// kb/codegen/0084 needs: the original copies all 9 bytes into a stack local
// before using them, which only a local aggregate initializer produces.
// Cells in a full bar, not counting the terminator.
static const int BAR_GAIJI_W = (BAR_MAX / BAR_GAIJI_MAX);

struct hack_bar {
	char x[BAR_GAIJI_W + 1]; // ACTUAL TYPE: gaiji_th04_t[]
};

extern "C" struct hack_bar hud_bar_max;

extern "C" void pascal hud_bar_put(utram_y_t y, int value, tram_atrb2 atrb)
{
	int value_rem;
	const struct hack_bar BAR_MAX_ROW = hud_bar_max;
	struct hack_bar bar_notfull;
	int i;

	if(value >= BAR_MAX) {
		gaiji_putsa(HUD_LEFT, y, BAR_MAX_ROW.x, atrb);
		return;
	}

	value_rem = value;
	value_rem -= BAR_GAIJI_MAX;
	i = 0;
	while(value_rem > 0) {
		bar_notfull.x[i] = g_BAR_16W;
		value_rem -= BAR_GAIJI_MAX;
		i++;
	}

	// ZUN landmine: A [value] of 0 comes out as a full 16-pixel cell, the same
	// off-by-one TH02's own power bar has. Every caller keeps its value at 1
	// or above.
	value_rem = ((value - 1) & (BAR_GAIJI_MAX - 1));
	bar_notfull.x[i] = (g_BAR_01W + value_rem);

	// The increment is the loop's own, not part of its condition: the
	// original compares SI directly, and `while(++i <= …)` instead
	// materializes the expression's value into AX first, costing 2 bytes.
	// The initial `i++` and the loop's are ONE instruction in the original
	// because `-O` cross-jumps the two paths that meet at the test
	// (kb/codegen/0097) -- write both out and let it merge them.
	for(i++; i <= (BAR_GAIJI_W - 1); i++) {
		bar_notfull.x[i] = g_EMPTY;
	}
	bar_notfull.x[BAR_GAIJI_W] = g_NULL;

	gaiji_putsa(HUD_LEFT, y, bar_notfull.x, atrb);
}
