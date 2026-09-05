/* ReC98
 * -----
 * 1st part of code segment #3 of TH05's MAIN.EXE
 */

// There is deliberately NO `-zP` group pragma, matching this binary's two
// other main_03 wrappers (th05/main032.cpp carried none until a `switch` jump
// table appeared in it, th05/main033.cpp still carries none) and th04's
// main_035.cpp. The group comes from th05_main.asm's own GRPDEF
// (kb/codegen/0105); `-zP` is only observable for a TU that emits a
// `cs:`-relative switch table (kb/codegen/0104, 0138), and this one emits
// none. `-zC` *is* required, because the basename default would name the
// segment MAIN031_TEXT rather than main_031_TEXT.
#pragma option -zCmain_031_TEXT

#include "th04/main/player/player.hpp"
#include "th04/main/spark.hpp"

// The far thunk that lets non-main_03 code spawn the standard player-centered
// spark burst. `sparks_add_circle()` is `near` and lives in SPARK_A_TEXT,
// which is inside group main_03; the one caller of this function,
// marisa_lasers_update_and_render(), is in group main_01 and so cannot reach
// it directly. `[inferred]` name: the body pins the position and both
// constants, so it describes exactly what it does and asserts nothing about
// the caller.
void sparks_add_circle_at_player(void)
{
	sparks_add_circle(player_pos.cur.x, player_pos.cur.y, TO_SP(12), 24);
}
