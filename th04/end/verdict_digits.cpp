/// The verdict screen's 3-digit gaiji number renderer
/// --------------------------------------------------
/// `[measured]` This was the FIRST proc of th05_maine.asm's SCORE_TEXT block,
/// and th05/regist.cpp is the object immediately ahead of it in TH05's
/// MAINE.EXE link order — the same kb/codegen/0098 head lift that put the six
/// high score table functions and regist_menu() into th05/hi_end.cpp and
/// th05/regist.cpp before it. No carve, no new segment, no Tupfile.lua line.
///
/// TH04 has the same function at `0A05:0DBC`, but in `MAINE_01_TEXT` and as
/// the NINTH of that block's fifteen procs, so its copy cannot be lifted until
/// the eight ahead of it are. When it can, this file takes it unchanged except
/// for the colour: see VERDICT_COL below.

/// th04/gaiji/gaiji.h has no include guard and every translation unit that can
/// reach this file has already included it — th05/regist.cpp through
/// th04/hiscore/regist_menu.cpp. Including it again is 26 "Multiple
/// declaration" errors, so this file relies on its host, which is the idiom
/// for every other .cpp fragment in the chain.
#include "libs/master.lib/pc98_gfx.hpp"

/// `[measured]` TH05 hoists the verdict overlay's origin and colours into
/// `_DATA`, because it draws the same verdict twice — standalone from
/// verdict_animate(), and again inside staffroll_animate(), which overwrites
/// all four. TH04 draws it once and hardcodes every one of them.
///
/// Published as `_verdict_col` by a zero-byte `label word` alias in front of
/// the private _DATA word it aliases in th05_maine.asm (kb/codegen/0123); the
/// thirty-odd references still inside that dump keep its original IDA spelling,
/// which is why the alias exists instead of a rename.
#if (GAME == 5)
extern "C" vc2 verdict_col;
#define VERDICT_COL verdict_col
#else
/// `[inferred]` TH04's copy pushes the immediate 14 here. NOT oracle-verified:
/// nothing compiles this arm yet, and it must be re-measured against
/// `th04_maine.asm` when that copy is lifted.
#define VERDICT_COL 14
#endif

/// ZUN bloat: Turn into a parameter of graph_3_digit_put().
///
/// `[measured]` The length of this name is load-bearing and lengthening or
/// shortening it breaks the link. Turbo C++ keeps 32 significant characters of
/// an identifier before prepending the C underscore, so these 34 characters are
/// exported as the 33-character
/// `_graph_3_digit_put_as_fixed_2_dig` that th04/gaiji/verdict[data].asm
/// publishes and both dumps reference (kb/codegen/0060).
extern "C" bool graph_3_digit_put_as_fixed_2_digit;

/// Assumes that [num] is a 3-digit number and renders it right-aligned to the
/// given VRAM position, blanking leading zeroes unless
/// [graph_3_digit_put_as_fixed_2_digit] is set — in which case the tens digit
/// is always shown and only the hundreds digit can blank.
void pascal near graph_3_digit_put(
	screen_x_t left, screen_y_t top, uint16_t num
)
{
	register int digit_seen = 0;
	// th02/op_01.cpp's gbZUN idiom: a gaiji string is a char array, because
	// that is what graph_gaiji_puts() takes.
	char g_str[4];
	uint16_t digit;

	g_str[0] = g_EMPTY;
	digit = (num / 100);
	num = (num % 100);
	if(!graph_3_digit_put_as_fixed_2_digit) {
		digit_seen |= digit;
		if(digit_seen) {
			g_str[0] = static_cast<char>(digit + gb_0);
		} else {
			// ZUN bloat: Already assigned above.
			g_str[0] = g_EMPTY;
		}
	}

	digit = (num / 10);
	num = (num % 10);
	digit_seen |= digit;
	digit_seen |= graph_3_digit_put_as_fixed_2_digit;
	if(digit_seen) {
		g_str[1] = static_cast<char>(digit + gb_0);
	} else {
		g_str[1] = g_EMPTY;
	}

	g_str[2] = static_cast<char>(num + gb_0);
	g_str[3] = g_NULL;
	graph_gaiji_puts(left, top, GAIJI_W, g_str, VERDICT_COL);
}
