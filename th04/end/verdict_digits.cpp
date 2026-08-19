/// The verdict screen's 3-digit gaiji number renderer
/// --------------------------------------------------
/// `[measured]` This was the FIRST proc of th05_maine.asm's SCORE_TEXT block,
/// and th05/regist.cpp is the object immediately ahead of it in TH05's
/// MAINE.EXE link order — the same kb/codegen/0098 head lift that put the six
/// high score table functions and regist_menu() into th05/hi_end.cpp and
/// th05/regist.cpp before it. No carve, no new segment, no Tupfile.lua line.
///
/// TH04's copies came out of MAINE_01_TEXT's tail four at a time, in dump
/// order, which is exactly this file's order — so th04/staff.cpp includes it
/// whole, ahead of th04/end/verdict_guts.cpp. `[measured]` They had to come
/// out together: the guts row alone leaves its generated jump table one byte
/// early, and only an odd-length prefix in the same object fixes that
/// (kb/codegen/0096). The four in front of it supply one.

/// th04/gaiji/gaiji.h has no include guard and every translation unit that can
/// reach this file has already included it — th05/regist.cpp through
/// th04/hiscore/regist_menu.cpp. TH04's own chain, th04/staff.cpp, does not:
/// this file is the first thing in it that needs a gaiji constant, so under
/// GAME==4 it is the one that includes it. Including it twice is 26 "Multiple
/// declaration" errors.
/// The same applies to th04/hardware/grppsafx.h, which TH05 reaches through
/// th05/regist.cpp and TH04 does not reach at all until here.
#if (GAME == 4)
#include "th04/gaiji/gaiji.h"
#include "th04/hardware/grppsafx.h"
#include "th04/resident.hpp"
#endif
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
/// `[measured]`, and now oracle-verified: TH04's copies push the immediate 14 at every one of
/// the seven sites in this file and the ten more in th04/end/verdict_stats.cpp
/// and th04/end/verdict_guts.cpp, which is what makes it a `#define` rather
/// than a variable.
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

/// `[measured]` The 点 label this function appends stays a `_DATA` byte of the
/// root dump, published there as `_POINT_MSG` (kb/codegen/0123). It has to:
/// the dump holds a SECOND, byte-identical copy of the same Shift-JIS string
/// twenty lines further down, and the build's `-d` would merge a C++ literal
/// pair into one and shift every following byte of that contribution.
extern "C" const shiftjis_t POINT_MSG[];

#if (GAME == 4)
/// TH04's own score renderer, and it is NOT the same body as TH05's below.
/// `[measured]` It takes no parameters at all — it hardcodes
/// `resident->score_last` and (160, 96) — renders EIGHT digits rather than
/// nine, and reads each one raw, with none of TH05's `/ 10` and `% 10` split
/// for the tens place of the topmost byte. Head-to-head that is 105 bytes
/// against 170, which is why the two are written out separately rather than
/// `#if`-woven.
///
/// ZUN bug: a final score of exactly 0 renders as an entirely blank field.
/// [digit_seen] never becomes true, so every one of the eight cells gets
/// g_EMPTY, including the ones place. TH05's copy fixes this with the
/// `(i == SCORE_DIGITS)` term in the same test; TH04 has no counterpart to it.
/// Reachable, and Slow Mode has nothing to do with it: Slow Mode skips the
/// high score *table* entry (th04/main/hiscore.cpp, th04/hiscore/regist_menu.cpp)
/// and never writes [score] or [score_last]. `[measured]`
/// score_last_commit() (th04/main/execl.cpp) is the only writer of
/// [resident->score_last] in TH04 and copies [score] unconditionally, so all
/// eight bytes are 0 exactly when the run scored nothing AND used no continues
/// — digits[0] is [score.continues_used] (th04/score.h), which
/// th04/main/scoreupd.asm's adder skips and score_reset() preserves. The field
/// renders on every MAINE.EXE path: th04/end/entry.cpp always reaches
/// verdict_animate(), ahead of the SLOWDOWN_NO_VERDICT_MSG fork in
/// th04/end/verdict_stats.cpp. `[inferred]` A game over on the first credit
/// without shooting, grazing or collecting anything is such a run; not yet
/// confirmed on an emulator. It renders wrongly rather than crashing, which is
/// the discriminator kb/conventions/rec98-taxonomy.md uses.
extern "C" void near graph_score_and_ten_put(void)
{
	// `[measured]` Declaration order is load-bearing: Turbo C++ gives the
	// first-declared local the slot closest to BP, and the original's frame is
	// [digit] at `bp-1`, [digit_seen] at `bp-2` and the string at `bp-12`,
	// with `bp-3` left as the array's even-alignment pad.
	unsigned char digit;
	unsigned char digit_seen;
	char g_str[SCORE_DIGITS + 1];
	register int i;

	digit_seen = false;
	for(i = 0; i < SCORE_DIGITS; i++) {
		digit = resident->score_last.digits[(SCORE_DIGITS - 1) - i];
		digit_seen |= digit;
		if(digit_seen) {
			g_str[i] = static_cast<char>(digit + gb_0);
		} else {
			g_str[i] = g_EMPTY;
		}
	}

	g_str[SCORE_DIGITS] = g_NULL;
	graph_gaiji_puts(160, 96, GAIJI_W, g_str, VERDICT_COL);

	// ZUN bloat: dead store. Nothing reads [digit_seen] again, and the
	// function returns two statements later. TH05's copy sets the same flag
	// where it is actually used.
	digit_seen = true;

	graph_putsa_fx(288, 96, VERDICT_COL, POINT_MSG);
}
#else
/// Renders a raw (i.e. NOT gaiji-offsetted) LEBCD score as nine right-aligned
/// boldface gaiji digits with leading zeroes blanked, followed by 「点」 at a
/// fixed 144-pixel offset. The ninth digit is the tens place of the topmost
/// byte, which is what gives the score its 959,999,999 ceiling — the same
/// packing th04/hiscore/regist_view.cpp's score_put() renders out of the
/// persisted table, except that this one's digits carry no [gb_0] offset and
/// therefore none of that function's inherited sign-promotion trap.
///
/// TH04's own eight-digit, parameterless variant is the arm above; the two
/// are separate bodies, settled by reading both.
extern "C" void pascal near graph_score_and_ten_put(
	screen_x_t left, vram_y_t top, const score_lebcd_t far *score
)
{
	unsigned char digit;
	unsigned char digit_seen;
	char g_str[SCORE_DIGITS + 2];
	register int i;

	if(score->digits[SCORE_DIGITS - 1] >= 10) {
		digit = (score->digits[SCORE_DIGITS - 1] / 10);
		g_str[0] = static_cast<char>(digit + gb_0);
		digit_seen = true;
	} else {
		g_str[0] = g_EMPTY;
		digit_seen = false;
	}

	for(i = 1; i <= SCORE_DIGITS; i++) {
		digit = (score->digits[SCORE_DIGITS - i] % 10);
		digit_seen |= digit;

		// The last digit is always shown, even for a score of 0.
		if(digit_seen || (i == SCORE_DIGITS)) {
			g_str[i] = static_cast<char>(digit + gb_0);
		} else {
			g_str[i] = g_EMPTY;
		}
	}

	g_str[SCORE_DIGITS + 1] = g_NULL;
	graph_gaiji_puts(left, top, GAIJI_W, g_str, VERDICT_COL);
	graph_putsa_fx((left + 144), top, VERDICT_COL, POINT_MSG);
}
#endif

/// The verdict's [skill] accumulator, and the two flags that steer it
/// ------------------------------------------------------------------

/// Published by th04/gaiji/verdict[data].asm and the root dumps' own `.data?`.
///
/// `[measured]` Signed, despite what "skill" suggests: TH04's
/// verdict_stats_put_and_wait() clamps it against 0 with a signed compare and
/// divides it with a sign-extending division, while the same statement's upper
/// bound comes out unsigned because the cap it is compared against is an
/// `unsigned long`. Byte-inert for TH05, whose only uses here are the
/// sign-agnostic addition and subtraction below.
extern "C" int32_t skill;

/// If `true`, skill_apply_and_graph_percentage() subtracts the calculated
/// fraction from [skill] rather than adding it.
extern "C" bool skill_subtract;

#if (GAME == 5)
/// `[measured]` If `true`, skill_apply_and_graph_percentage() ALSO stashes a
/// quarter of the calculated fraction in [skill_quarter]. The verdict screen
/// sets it around exactly one row — the one that scores `std_frames` against
/// 46,000 (12,800 on the Extra Stage) — and adds [skill_quarter] back into
/// [skill] *after* dividing [skill] by 12, so that row's percentage ends up
/// counting three times as much as every other one.
///
/// `[measured]` TH04 has the flag at the same structural position in its own
/// `.data?` and writes it 1/0 around the same row, but **nothing in TH04
/// MAINE.EXE ever reads it, and TH04 has no [skill_quarter] at all** — the
/// stash block is the whole of that game's 0x17-byte shortfall against this
/// function. `[inferred]` Vestigial in TH04, finished in TH05.
extern "C" bool skill_stash_quarter;
extern "C" uint32_t skill_quarter;
#endif

/// `[measured]` Both of these stay `_DATA` bytes of the root dump, published
/// there by zero-byte aliases, for the same reason POINT_MSG does: the dump
/// holds a byte-identical SECOND copy of each (which is why the fraction
/// renderer below uses DOT_MSG_0 while the percentage renderer uses DOT_MSG),
/// and `-d` would merge a C++ literal pair into one. The `_0` suffix is the
/// dump's own convention for such a duplicate — see its four identical
/// `_SCOREDAT_FN*` copies.
extern "C" const shiftjis_t DOT_MSG[];
extern "C" const shiftjis_t DOT_MSG_0[];
extern "C" const shiftjis_t PERCENT_MSG[];

/// Calculates ([share] / [total]), applies the result × 1,000,000 to [skill],
/// and renders it as a right-aligned 3.2-digit percentage to the given VRAM
/// position.
void pascal near skill_apply_and_graph_percentage(
	screen_x_t left, screen_y_t top, uint16_t total, uint16_t share
)
{
	uint16_t digits;
	uint32_t fraction;

	// `[measured]` Both of these are if/else, not a ternary: the original
	// stores into [fraction] inside each arm, where a ternary makes Turbo C++
	// compute the value in EAX and store once at the merge point. The
	// [skill] assignment below is the opposite case and IS a ternary.
	if(total != 0) {
		fraction = 1000000;
	} else {
		fraction = 0;
	}
	if(total != share) {
		if(total != 0) {
			fraction = (fraction / total);
		} else {
			fraction = 0;
		}
		fraction = (share * fraction);
	}
	skill = (!skill_subtract ? (skill + fraction) : (skill - fraction));
#if (GAME == 5)
	// `[measured]` The whole of TH04's 0x17-byte shortfall against this
	// function. TH04 writes the flag around the same row and never reads it,
	// and has no [skill_quarter] at all.
	if(skill_stash_quarter) {
		skill_quarter = (fraction >> 2);
	}
#endif

	digits = (fraction / 10000);
	graph_3_digit_put(left, top, digits);
	// `[measured]` Compound assignment, not `fraction = (fraction % 10000)`:
	// the original loads the divisor into EBX BEFORE the dividend into EAX,
	// which is what `%=` emits and the spelled-out form does not.
	fraction %= 10000;
	digits = (fraction / 100);
	graph_3_digit_put_as_fixed_2_digit = true;
	graph_3_digit_put((left + 48), top, digits);
	graph_3_digit_put_as_fixed_2_digit = false;
	graph_putsa_fx((left + 48), top, VERDICT_COL, DOT_MSG);
	graph_putsa_fx((left + 96), top, VERDICT_COL, PERCENT_MSG);
}

/// Calculates ([num] / 1,000,000) and renders the result as a right-aligned
/// 3.2-digit fraction to the given VRAM position.
void pascal near graph_fraction_of_million_put(
	screen_x_t left, screen_y_t top, uint32_t num
)
{
	uint16_t digits;

	digits = (num / 10000);
	graph_3_digit_put(left, top, digits);
	// `[measured]` `%=` for the divisor-first load order, as above.
	num %= 10000;
	digits = (num / 100);
	graph_3_digit_put_as_fixed_2_digit = true;
	graph_3_digit_put((left + 48), top, digits);
	graph_3_digit_put_as_fixed_2_digit = false;
	graph_putsa_fx((left + 48), top, VERDICT_COL, DOT_MSG_0);
}
