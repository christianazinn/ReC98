/// TH04 per-stage reset of the playfield simulation's global state
/// ---------------------------------------------------------------
/// (#included from th04/main_012.cpp, the only C++ object contributing to
/// th04_main.asm's main_012_TEXT segment. TLINK lays a segment's
/// contributions out in link order with the root dump first, so that object
/// lands at the segment's tail by construction -- which is where this
/// function already was. kb/codegen/0105 + 0112.)
///
/// Called first thing by stage_init(), the per-stage initialization function
/// in DEMO_TEXT, which resets the player, scroll, HUD and tile state around
/// it. That one is C++ too since 2026-08-19 -- th04/main/stage/init.cpp, and
/// this file's prose is where its name came from. [inferred] The split between
/// the two looks like a source-file boundary rather than a conceptual one:
/// they live in different original objects.
///
/// TH05's twin is still ASM -- `sub_EACE`, at the *head* of th05_main.asm's
/// main_TEXT, so lifting it needs a kb/codegen/0080 carve. It is this body
/// plus `hitshot_next_free_id`, `slowdown_caused_by_bullets`, and `hitshots`
/// and `lasers` at the head of the zero-fill list. No `#if (GAME == 5)` arm is
/// written here on purpose: this file is compiled into TH04's MAIN.EXE alone,
/// so such an arm would never be graded by the oracle.

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
#include "th04/main/enemy/enemy.hpp" // also brings bullet.hpp and item.hpp
#include "th04/main/player/shot.hpp"
#include "th04/main/pointnum/pointnum.hpp"

// Zeroes [dword_count] doublewords starting at [dst], which has to point into
// DGROUP. Still assembly (`sub_C34E` in th04_main.asm's CIRCLE_TEXT): it reads
// both of its parameters through a `mov bx, sp` frame, which lies outside what
// a C++ signature can express, so dwords_clear() is reached through the
// zero-byte `label` alias that dump publishes for it (kb/codegen/0123). Every
// one of its call sites is below. (The sentence above is about dwords_clear()
// alone, and says so explicitly because it sits directly above the definition
// of stage_state_reset() as well.)
extern "C" void pascal near dwords_clear(void near *dst, int dword_count);

void near stage_state_reset(void)
{
	frames_unused = 0;
	stage_frame = 0;
	stage_frame_mod2 = 0;
	stage_frame_mod4 = 0;
	stage_frame_mod8 = 0;
	stage_frame_mod16 = 0;
	slowdown_factor = 1;
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
