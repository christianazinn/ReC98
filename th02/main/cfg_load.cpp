/* ReC98
 * -----
 * TH02 MAIN.EXE's copy of cfg_load(). The only thing this process wants out of
 * the config file is the segment of the resident structure that OP.EXE left
 * behind; the settings for the first attempt then come out of that structure
 * rather than out of the file.
 *
 * ZUN compiled it into its own object, linked ahead of th02/pointnum.cpp's into
 * the same POINTNUM_TEXT segment, and it has to stay a separate object here:
 * Turbo C++ word-aligns th02/main/pointnum/pointnum.cpp's `switch` jump table
 * relative to the start of the object it emits, and this function's 0x81 bytes
 * are an odd number, so merging the two through an #include changes that
 * table's parity and drops the original's one-byte alignment pad.
 * (kb/codegen/0119)
 */

// -zC/-zP only take effect before any code is generated, so the segment and
// group are named here. -G- (optimize for size) is what turns the prolog into
// the single `ENTER 2, 0` the original has. (kb/codegen/0011)
#pragma option -zCPOINTNUM_TEXT -zPmain_01 -G-

#include "platform.h"
#include "libs/master.lib/master.hpp"
#include "th02/resident.hpp"
#include "th02/core/globals.hpp"
#include "th02/main/item/item.hpp"
#include "th02/main/playperf.hpp"
#include "th02/main/player/player.hpp"
#include "th02/main/stage/stage.hpp"

// MAIN.EXE's copy of the config file name, still owned by the dump's data
// segment and reached through the `public` line th02_main.asm now carries.
// (kb/codegen/0123)
extern "C" const char cfg_fn[];

// The file position of the resident structure's segment: th02/formats/cfg.hpp's
// 5-byte [cfg_options_t] is everything ahead of it, so this is that header's
// `offsetof(cfg_t, resident)`. Spelled out rather than taken from the header
// because this translation unit cannot include it: it declares the __cdecl, far
// cfg_load() that OP.EXE and MAINE.EXE compile from C++, while MAIN.EXE's is
// near and `extern "C"`. th02/main/entry.cpp gives the same reason for
// declaring the prototype by hand.
static const long CFG_OFFSET_RESIDENT = 5;

// Points [resident] at the segment OP.EXE stored in the config file, and copies
// the settings for the first attempt out of that structure. Returns 0 if the
// file names no resident segment, which makes main_entry() quit immediately.
extern "C" int near cfg_load(void)
{
	resident_t __seg *resident_seg;

	file_ropen(cfg_fn);
	file_seek(CFG_OFFSET_RESIDENT, 0);
	file_read(&resident_seg, sizeof(resident_seg));
	file_close();

	// MAINE.EXE's cfg_load() returns early on the same condition; this one
	// wraps the entire body instead, and the compiler puts the `return 0` at
	// the bottom. (th02/maine_03.cpp)
	if(resident_seg) {
		resident = resident_seg;
		stage_id = resident->stage;
		lives = resident->start_lives;
		bombs = resident->start_bombs;
		rank = resident->rank;
		power = resident->start_power;

		// A [start_power] of 0 still starts the run with 1.
		if(power == 0) {
			power++;
		}
		playperf = 0;
		item_bigpower_override = 0;
		return 1;
	}
	return 0;
}
