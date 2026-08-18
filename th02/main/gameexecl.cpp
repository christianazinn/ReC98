/* ReC98
 * -----
 * TH02's cleanup-and-handoff function: the last thing MAIN.EXE runs before
 * replacing itself with MAINE.EXE. It is contiguous with the point number
 * code in this code segment, which is why the two are compiled into one
 * translation unit - see th02/pointnum.cpp.
 */

#include "platform.h"
#include "pc98.h"
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/formats/pi.h"
#include "th02/core/initexit.h"
#include "th02/gaiji/loadfree.h"
#include "th02/main/execl.hpp"
#include <process.h>
#include <stddef.h>

// Declared here rather than through th02/formats/mpn.hpp,
// th02/main/player/bomb.hpp and th02/main/enemy/enemy.hpp, to keep this file's
// header closure disjoint from the three that follow it in this translation
// unit. Same reason as th04/main/execl.cpp.
extern "C" void mpn_free(void); // th02/formats/mpn.hpp has it in an extern "C" block
void near bomb_free(void);
extern "C" void far enemy_stagedata_free(void);

int GameExecl(const char *binary_fn)
{
	// Only slot 0 is ever freed here. TH02 loads its other PI slots in
	// OP.EXE, which frees its own.
	pi_free(0);

	bomb_free();
	mpn_free();
	enemy_stagedata_free();
	super_free();
	graph_clear();
	text_clear();
	gaiji_free();
	game_exit();
	return execl(
		const_cast<char *>(binary_fn),
		const_cast<char *>(binary_fn),
		nullptr
	);
}
