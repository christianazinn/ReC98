/// Power bar HUD row
/// -----------------
/// One name, one body: the TH04 and TH05 dumps of this function differ in
/// exactly one instruction, and that difference is pure link topology, not
/// semantics — TH04's hud_bar_put() is a far proc in the same original
/// segment, reached through the linker-relaxed `nop; push cs; call near ptr`
/// form, while TH05's is a near proc in its own object.
///
/// Unlike TH02, this does NOT also update [shot_level]; that half lives in
/// its own function, which tail-calls this one. Upstream's own comment at
/// th02/main/hud/hud.hpp calls TH02's fusion of the two misplaced.

#pragma option -zPmain_01 -G

#include "th02/main/player/player.hpp"
#include "th04/main/hud/hud.hpp"

// The row that hud_put() labels with gsREIRYOKU / gsPOWER at (62, 21).
// th02/main/hud/hud.hpp declares a different row under this same name, but
// only for TH02 itself.
//
// A macro rather than the `static const` that the sibling HUD_*_Y constants
// use, because Turbo C++ 4.0J's __emit__() only accepts literal constants:
// with `static const utram_y_t HUD_POWER_Y = 22;` the TH04 branch below fails
// to compile with "Illegal parameter to __emit__". (kb/codegen/0089)
#define HUD_POWER_Y 22

extern "C" void pascal hud_power_put(void)
{
	struct hack_colors {
		uint8_t x[SHOT_LEVEL_MAX + 1]; // ACTUAL TYPE: tram_atrb2[], truncated
	};
	extern struct hack_colors HUD_POWER_COLORS;

	const struct hack_colors COLORS = HUD_POWER_COLORS;

#if (GAME == 5)
	hud_bar_put(HUD_POWER_Y, power, COLORS.x[shot_level]);
#else
	// The original object reached TH04's same-segment far hud_bar_put()
	// through `nop; push cs; call near ptr`, and no plain C++ far call
	// reproduces that: Turbo C++ emits a real five-byte far-call instruction
	// even when the callee ends up in the same group. (kb/codegen 0014;
	// upstream's own TODO
	// in th04/main/demo.hpp says the same thing about GameExecl().) So the
	// arguments are pushed by hand, in pascal order.
	__emit__(0x6A, HUD_POWER_Y);
	_AX = power;			_asm { push ax; }
	_AX = COLORS.x[shot_level];	_asm { push ax; }
	_asm { nop; push cs; call near ptr hud_bar_put; }
#endif
}
