/// The verdict screen's two comment lines
/// --------------------------------------
/// `[measured]` This was the last proc of th05_maine.asm's SCORE_TEXT block
/// once verdict_animate() came out of it, so it lands by the same tail lift:
/// th05/staff.cpp is the object immediately behind the dump in TH05's
/// MAINE.EXE link order, and this file is included at the head of that
/// object's SCORE_TEXT contribution. Third body out of this tail; the order
/// of the three `#include`s in the host IS the emitted order, and it has to
/// stay the dump's original one.
///
/// This file owns the includes for the whole tail chain. th04/hardware/
/// grppsafx.h and th05/resident.hpp have **no include guards**, so the two
/// files behind it rely on this one having pulled them in — the same idiom
/// th04/end/verdict_digits.cpp uses for th04/gaiji/gaiji.h, and the reason
/// this file must stay first.

#include "th04/hardware/grppsafx.h"
#include "th05/resident.hpp"

/// `[measured]` The verdict overlay's origin, published by zero-byte
/// `label word` aliases in th05_maine.asm (kb/codegen/0123) beside
/// [verdict_col]. staffroll_animate() overwrites all four of these before it
/// re-runs the same renderers against a different origin, which is why TH05
/// hoists them into _DATA where TH04 hardcodes them.
extern "C" screen_x_t verdict_left;
extern "C" vram_y_t verdict_top;

/// `[measured]` The SECOND of the four, and a distinct colour from
/// [verdict_col]: the two default to 6 and 2 respectively, and only
/// staffroll_animate() ever makes them equal (it sets both to 13). Every
/// free-text line on the verdict screen uses this one; the numbers use
/// [verdict_col].
extern "C" vc2 verdict_comment_col;

/// `[measured]` The two comment lines, each a 30-byte record read straight
/// out of `_ude.txt` by the still-ASM screen body, which seeks between them.
/// They are contiguous in the dump's `.data?` — the second begins exactly 30
/// bytes after the first — so they are one `[2][30]` array in the original.
/// They stay two separate `extern`s until that reader is lifted and can
/// settle the shape from the read calls themselves rather than from adjacency.
extern "C" shiftjis_t verdict_comment_1[];
extern "C" shiftjis_t verdict_comment_2[];

/// Renders both `_ude.txt` lines under the stats block. `[measured]` The
/// guard is a test on the FIRST byte of the first line, not a separate flag:
/// a `_ude.txt` that could not be read leaves the buffer zeroed, and a
/// zero-length first line suppresses both.
void near verdict_comment_put(void)
{
	if(verdict_comment_1[0]) {
		graph_putsa_fx(
			(verdict_left + 48), (verdict_top + 296), verdict_comment_col,
			verdict_comment_1
		);
		graph_putsa_fx(
			(verdict_left + 48), (verdict_top + 312), verdict_comment_col,
			verdict_comment_2
		);
	}
}
