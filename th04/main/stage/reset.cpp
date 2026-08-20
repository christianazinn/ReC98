/// Per-stage reset of the playfield simulation's global state
/// ----------------------------------------------------------
/// (#included from th04/main_012.cpp and th05/main011.cpp, the only C++
/// objects contributing to th04_main.asm's main_012_TEXT and th05_main.asm's
/// main_TEXT respectively. TLINK lays a segment's contributions out in link
/// order with the root dump first, so each object lands at its segment's tail
/// by construction -- which is where this function already was in both games.
/// kb/codegen/0105 + 0112.)
///
/// Called first thing by stage_init(), the per-stage initialization function
/// in DEMO_TEXT, which resets the player, scroll, HUD and tile state around
/// it. TH04's is C++ since 2026-08-19 -- th04/main/stage/init.cpp, and this
/// file's prose is where its name came from; TH05's twin of that one is still
/// `sub_B55A` in th05_main.asm. [inferred] The split between the two looks
/// like a source-file boundary rather than a conceptual one: they live in
/// different original objects.
///
/// TH05's arm was written on 2026-08-19, when the proc IDA had left unnamed
/// became the *tail* of main_TEXT's root contribution as well as its head, and
/// so no longer needed a carve. It adds one unnamed word that nothing reads,
/// `hitshot_next_free_id`, `slowdown_caused_by_bullets`, and `hitshots` and
/// `lasers` at the head of the zero-fill list.

#include "platform.h"
#include "pc98.h"
#include "x86real.h"
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th03/hardware/palette.hpp"
#include "th04/main/circle.hpp"
#include "th04/main/custom.hpp"
#include "th04/main/frames.h"
#include "th04/main/gather.hpp"
#include "th04/main/playfld.hpp"
#include "th04/main/quit.hpp"
#include "th04/main/score.hpp"
#include "th04/main/slowdown.hpp"
#include "th04/main/spark.hpp"
#include "th04/main/bullet/clearzap.hpp"
// Both spellings also bring bullet.hpp and item.hpp. TH05's enemy_t has to be
// declared before th04/main/enemy/enemy.hpp's `extern enemies[]`, so in that
// game this header may only be reached through th05/main/enemy/enemy.hpp --
// the same conditional every other shared enemy TU carries.
#if (GAME == 5)
	#include "th05/main/enemy/enemy.hpp"
#else
	#include "th04/main/enemy/enemy.hpp"
#endif
#include "th04/main/player/shot.hpp"
#include "th04/main/pointnum/pointnum.hpp"
#if (GAME == 5)
	#include "th05/main/bullet/laser.hpp"
#endif

// Zeroes [dword_count] doublewords starting at [dst], which has to point into
// DGROUP. Still assembly in both games (`sub_C34E` in th04_main.asm's
// CIRCLE_TEXT; th05_main.asm's copy sits in SCORE_TEXT, is likewise still
// under IDA's spelling, and differs from TH04's by one instruction --
// `xor ax, ax` against `xor eax, eax`): each reads both of its parameters
// through a `mov bx, sp` frame, which lies outside what a C++ signature can
// express, so dwords_clear() is reached through the zero-byte `label` alias
// each dump publishes for it (kb/codegen/0123). Every one of its call sites in
// either binary is below. (The sentence above is about dwords_clear() alone,
// and says so explicitly because it sits directly above the definition of
// stage_state_reset() as well.)
extern "C" void pascal near dwords_clear(void near *dst, int dword_count);

#if (GAME == 5)
	// Next free array element in [hitshots], in the dump's own words
	// (th05/main/player/hitshot_from[bss].asm, which publishes it). Belongs
	// beside `hitshots` in th04/main/player/shot.hpp; it is declared here
	// instead because that file is claimed whole by another parcel
	// (MATCH-TH04-MAIN-CARVE-TAILS-1), and a plain `extern` costs nothing to
	// move later.
	extern unsigned int hitshot_next_free_id;

	// Written to 0 here and read nowhere in MAIN.EXE -- a second unused
	// global beside [frames_unused], and left under IDA's spelling for that
	// reason. The failed search is recorded in
	// state/notes/th05-main-tail-lifts.md, per
	// kb/conventions/naming-precedents.md §3.
	extern "C" unsigned int word_2D05E;
#endif

void near stage_state_reset(void)
{
	#if (GAME == 5)
		// One `xor ax, ax` feeds both stores, so this is a chain rather than
		// two statements, and the original stores [word_2D05E] first -- which
		// puts it on the right (kb/codegen/0092).
		hitshot_next_free_id = word_2D05E = 0;
	#endif
	frames_unused = 0;
	stage_frame = 0;
	stage_frame_mod2 = 0;
	stage_frame_mod4 = 0;
	stage_frame_mod8 = 0;
	stage_frame_mod16 = 0;
	slowdown_factor = 1;
	#if (GAME == 5)
		slowdown_caused_by_bullets = false;
	#endif
	quit = Q_KEEP_RUNNING;
	palette_changed = false;
	bullet_zap.active = false;
	stage_graze = 0;

	// Left as the raw value ZUN wrote. The two named constants for this
	// variable, COL_CIRCLES (9) and COL_CIRCLES_AFTER_BOMB (13), are both
	// file-local to the bomb code and mean "the color during/after a bomb";
	// neither describes a stage-start default.
	circles_color = 13;

	grc_setclip(
		PLAYFIELD_LEFT, PLAYFIELD_TOP,
		(PLAYFIELD_RIGHT - 1), (PLAYFIELD_BOTTOM - 1)
	);

	#if (GAME == 5)
		dwords_clear(hitshots, sizeof(hitshots) / 4);
		dwords_clear(lasers, sizeof(lasers) / 4);
	#endif
	dwords_clear(shots, sizeof(shots) / 4);
	dwords_clear(enemies, sizeof(enemies) / 4);
	dwords_clear(sparks, sizeof(sparks) / 4);
	dwords_clear(bullets, sizeof(bullets) / 4);
	dwords_clear(custom_entities, sizeof(custom_entities) / 4);
	dwords_clear(circles, sizeof(circles) / 4);
	dwords_clear(items, sizeof(items) / 4);
	dwords_clear(pointnums, sizeof(pointnums) / 4);
	dwords_clear(gather_circles, sizeof(gather_circles) / 4);

	gather_template.ring_points = 8;
	gather_template.col = 9;
	gather_template.radius.set(GATHER_RADIUS_START);
	gather_template.angle_delta = 0x02;

	// Two separate word stores in the original, so gather_template_init()'s
	// set_long() shape (th04/main/bullet/add.cpp:518) would not match here.
	gather_template.velocity.x.v = 0;
	gather_template.velocity.y.v = 0;
}
