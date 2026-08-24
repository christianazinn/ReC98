/// Cleanup and process handoff
/// ---------------------------
/// The last thing MAIN.EXE runs before replacing itself with OP.EXE or
/// MAINE.EXE: it commits every gameplay metric MAINE.EXE will want into the
/// resident structure, frees every subsystem that owns heap or EMS memory,
/// and then `execl()`s the next binary.
///
/// ONE body for both games. TH04 additionally restores the gaiji table, and
/// reaches bb_boss_free() with a cross-group far call where TH05's is near.
/// Nothing else differs.

#include "platform.h"
#include "pc98.h"
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/main/execl.hpp"
#include "th04/main/frames.h"
#include "th04/main/replay.hpp"
#if (GAME == 5)
	#include "th05/resident.hpp"
#else
	#include "th04/resident.hpp"
#endif
#include <process.h>
#include <stddef.h>

extern "C" const char arg0[];

// Declared here rather than through th04/main/item/item.hpp,
// th04/main/enemy/enemy.hpp and th04/formats/{std,map}.hpp, none of which can
// be included together: they pull in th04/main/bullet/bullet.hpp twice and it
// is not include-guarded. Same reason as th04/main/stage/loop.cpp.
// ---------------------------------------------------------------------
extern unsigned int items_spawned;
extern unsigned int items_collected;
extern unsigned int total_point_items_collected;
extern unsigned int enemies_gone;
extern unsigned int enemies_killed;

// [total_max_valued_point_items_collected] under the <= 32-character alias
// that th04/main/item/items[data].asm now also publishes, because TLINK
// truncates C identifiers to 32 characters and the real name is 38
// (kb/codegen/0060).
extern unsigned int total_max_valued_point_items;

// master.lib's EMS handle, and its deallocator.
extern "C" {
	extern unsigned int Ems;
	int MASTER_RET ems_free(unsigned int handle);
}

// Hands MAINE.EXE the score this run ended on, by snapshotting every BCD digit
// of [score] into the resident structure.
//
// ONE body for both games. The copy loop is instruction-for-instruction
// identical in TH04 and TH05; only TH05's tail call is new.
//
// The loop deliberately runs over ALL of [score], index 0 included, even though
// index 0 is not a score digit: th04/main/score[bss].asm puts a zero-size
// `_continues_used` label on [score]'s first byte and th04/score.h types the
// two as a union, so digits[0] IS [score.continues_used]. Carrying it along is
// what lets MAINE.EXE read the continues count back out of
// [resident->score_last]. score_highest_update_and_reset() is the function
// that has to skip index 0 for the same reason.
#if (GAME == 5)
	// Commits [score] to [resident->score_highest] if it beats it, then zeroes
	// [score] and the score popup state. Lifted, in th05/main/score_highest.cpp
	// (HUD_PNT_TEXT); a near call reaches it because both segments are in group
	// main_01. Declared here rather than in a header because this TU cannot
	// reach that file, and because the ASM's `public _score_highest_update_and_
	// reset` is what pins the `extern "C"` (kb/codegen/0102).
	extern "C" void near score_highest_update_and_reset(void);
#endif

void near score_last_commit(void)
{
	for(int i = 0; i < SCORE_DIGITS; i++) {
		resident->score_last.digits[i] = score.digits[i];
	}
	#if (GAME == 5)
		score_highest_update_and_reset();
	#endif
}

extern "C" void pascal near bb_txt_free(void);
extern "C" void pascal cdg_free_all(void);
extern "C" void pascal near bb_playchar_free(void);

// Far in TH04 (group main_03), near in TH05 — a declaration difference, not a
// body difference. (kb/codegen/0083)
#if (GAME == 5)
	void near bb_boss_free(void);
#else
	void bb_boss_free(void);
#endif

void near dialog_free(void);
void near std_free(void);
void near map_free(void);
void game_exit(void);
// ---------------------------------------------------------------------

int pascal GameExecl(const char *binary_fn)
{
	if(replay_process_end()) {
		binary_fn = arg0;
	}
	score_last_commit();
	if(Ems) {
		ems_free(Ems);
	}

	resident->std_frames = total_std_frames;
	resident->items_spawned = items_spawned;
	resident->items_collected = items_collected;
	resident->point_items_collected = total_point_items_collected;
	resident->max_valued_point_items_collected = total_max_valued_point_items;
	resident->enemies_gone = enemies_gone;
	resident->enemies_killed = enemies_killed;
	resident->slow_frames = total_slow_frames;
	resident->frames = total_frames;

	bb_txt_free();
	cdg_free_all();
	bb_boss_free();
	dialog_free();
	bb_playchar_free();
	std_free();
	map_free();
	super_free();
	graph_hide();
	text_clear();
	#if (GAME != 5)
		gaiji_restore();
	#endif
	game_exit();
	return execl(
		const_cast<char *>(binary_fn),
		const_cast<char *>(binary_fn),
		nullptr
	);
}
