/// Playchar bomb animation (.BB + .CDG)
/// ------------------------------------
/// The playchar-specific half of the same lifecycle bb_boss_load() /
/// bb_boss_free() implement for bosses, and structurally the same code — with
/// one addition: both filenames are patched in place with the playchar's ASCII
/// digit before either is opened.
///
/// TH04 only. TH05's bb_playchar_load() in th05/formats/bb_pchar.cpp keeps one
/// filename instead of two, loads no .CDG here at all, and reaches bb_load()
/// rather than opening the file itself.
///
/// No segment pragma here: this file is #included first, so `-zC`/`-zP` have to
/// be set by the wrapper before it, in th04/player_b.cpp. Repeating them here
/// is NOT an option — a second `-zC` after code generation has begun is a hard
/// error, "Incorrect pragma directive option", measured on the first build of
/// this parcel. (kb/codegen/0112 trap 0, kb/codegen/0138.)

#include "libs/master.lib/master.hpp"
#include "th03/formats/cdg.h"
#include "th04/formats/bb.h"
#include "th04/resident.hpp"
#include "th04/sprites/main_cdg.h"

// Declared here rather than added to th04/formats/bb.h beside [bb_boss_seg],
// which every other lane's parcels also read: nothing outside this file needs
// them from C++. th04/main/execl.cpp already declares bb_playchar_free() the
// same way, for the same reason.
extern bb_tiles8_t __seg *bb_playchar_seg;

// "BB0.BB" and "BB0.CDG", whose third character is overwritten below.
// Writable, and far, because the dump stores each as a dword pointer into
// _DATA rather than as an array.
extern char far *bb_playchar_bb_fn;
extern char far *bb_playchar_cdg_fn;

extern "C" void pascal near bb_playchar_load(void)
{
	// Written as two statements that each re-read the resident structure,
	// which is what the original does: `-O` shares the loaded byte between
	// them but re-emits the `LES` for the second, leaving a dead
	// `les bx, _resident` immediately before the second filename's own load.
	// The literal index is the tree's unanimous convention for this exact
	// statement -- th04/main/ems.cpp's `bbname[2]` patches the same filename,
	// and th04/end/entry.cpp, th04/formats/dialog.cpp and
	// th04/hiscore/score_ld.cpp all spell their own offsets the same way. Five
	// sites, no named constant among them.
	bb_playchar_bb_fn[2] = resident->playchar_ascii;
	bb_playchar_cdg_fn[2] = resident->playchar_ascii;

	file_ropen(bb_playchar_bb_fn);
	bb_playchar_seg = HMem<bb_tiles8_t>::alloc(BB_SIZE);
	file_read(bb_playchar_seg, BB_SIZE);
	file_close();

	cdg_load_single_noalpha(CDG_BG_PLAYCHAR_BOMB, bb_playchar_cdg_fn, 0);
}

extern "C" void pascal near bb_playchar_free(void)
{
	if(bb_playchar_seg) {
		HMem<bb_tiles8_t>::free(bb_playchar_seg);
		bb_playchar_seg = 0;
	}
}
