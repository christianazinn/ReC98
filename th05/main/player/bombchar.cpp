/// The four playchars' bomb animation drivers
/// ------------------------------------------
/// One per playchar, and the four functions main() picks between when it sets
/// [playchar_bomb_func] (th05_main.asm, `mov _playchar_bomb_func, offset
/// bomb_reimu` and its three siblings). Unlike TH04, where a single
/// bomb_update_and_render() drives the animation and [playchar_bomb_func]
/// points at the playchar-specific *rendering*, TH05 gives each playchar its
/// own driver and lets it call its own …_bomb_update_and_render().
///
/// Each driver is one frame of a bomb, from the moment it is dropped to the
/// moment [bombing] clears again: the BB?.BB dissolve-in, the scroll and
/// palette changes that bracket the bomb proper, and the call into the
/// playchar-specific animation that does the actual damage. [bomb_frame] is
/// advanced at the end, unconditionally — TH05 has no [bombing] test here,
/// because th05_main.asm only reaches [playchar_bomb_func] while it is set.
///
/// (#included from th05/bombchar.cpp, which owns BOMBCHAR_TEXT — the head half
/// of what used to be MB_INV_TEXT's root contribution, split off by a
/// kb/codegen/0080 carve so that a C++ object can append to it. Bodies go into
/// this file in ORIGINAL ADDRESS ORDER, lowest first: the root shrinks from
/// its tail and the C++ contribution grows backwards into the hole, so every
/// byte keeps its address. kb/codegen/0114 + 0129.)

#include "th02/snd/snd.h"
#include "th02/v_colors.hpp"
#include "th03/hardware/palette.hpp"
#include "th04/common.h"
#include "th04/main/bg.hpp"
#include "th04/main/null.hpp"
#include "th04/main/scroll.hpp"
#include "th04/main/stage/stage.hpp"
#include "th05/main/player/bomb.hpp"

// master.lib. The two declarations are copied out of
// libs/master.lib/pc98_gfx.hpp rather than reached through it, for the reason
// th04/main/player/bombupd.cpp gives for its own copy of these same lines;
// libs/master.lib/func.hpp is still needed for [MASTER_RET], and is the only
// unguarded header this object takes twice-removed.
#include "libs/master.lib/func.hpp"

extern "C" {
	extern unsigned int __cdecl PaletteTone;
	void MASTER_RET graph_scrollup(unsigned line);
}

// Declared here rather than through their own headers, both of which are
// unguarded and would drag further unguarded ones into this object.
// th04/main/player/bombupd.cpp does the same for [items_pull_to_player].
extern bool items_pull_to_player;
extern unsigned char tiles_bb_col;

// `extern "C"` and `pascal`, because the name is published UNDECORATED and
// all-uppercase (kb/codegen/0086 + 0102); `near`, because it ends in `retn 2`.
// Compiled from th04/main/player/bb_playchar_put.cpp for both games; TH05
// reaches it through th05/main010.cpp, i.e. mai_TEXT, which is in this same
// group.
extern "C" void pascal near bb_playchar_put(int cel);

// Defined in th05/main/player/bombanim.cpp, i.e. in mai_TEXT and therefore in
// this same group, which is what makes the call `near` rather than the large
// model's default `far`. C++ linkage, because the original calls the MANGLED
// `@bomb_dream_decay$qv`.
void near bomb_dream_decay(void);

// Yuuka's per-frame bomb animation: the BOMB?.CDG background, the playfield
// margins around it, the hearts, and the palette flashes. Still ZUN's
// assembly, immediately above this object in BOMBCHAR_TEXT; the dump grew the
// zero-byte `label near` alias kb/codegen/0123 prescribes, because it has no
// `public` of its own. Named after its three already-named siblings in this
// same segment (…_stars_/…_lasers_/…_circles_/yuuka_heart_update_and_render).
extern "C" void near yuuka_bomb_update_and_render(void);

extern "C" void pascal near bomb_yuuka(void)
{
	enum {
		// The BB?.BB dissolve-in, 16 cels at 2 frames each. TH04 splits the
		// same 16 cels across two rates; TH05 does not.
		BB_CEL_FRAMES = 2,

		// The bomb proper starts: the scroll stops, the background renderer
		// is disabled, and …_bomb_update_and_render() takes over.
		FRAME_BOMB = 32,

		// The bomb ends: the scroll resumes, items stop being pulled in, and
		// the tone walks from [TONE_START] back down towards master.lib's
		// neutral [TONE_NORMAL], 3 units per frame until [FRAME_END].
		FRAME_FADE = 160,
		FRAME_END = 192,

		TONE_START = 192,
		TONE_PER_FRAME = 3,
		TONE_NORMAL = 100,
	};

	// Stage 6 is Makai, which does not scroll: its background is a boss
	// backdrop from the first frame, so the two `graph_scrollup()` calls that
	// bracket the bomb would put a scrolled-in edge on screen. Spelled
	// against [MAIN_STAGE_COUNT] the way th04/main/boss/boss.cpp:251 spells
	// the same test.
	#define stage_scrolls() (stage_id != (MAIN_STAGE_COUNT - 1))

	if(bomb_frame < FRAME_BOMB) {
		tiles_bb_col = V_WHITE;
		bb_playchar_put(bomb_frame / BB_CEL_FRAMES);
	} else if(bomb_frame == FRAME_BOMB) {
		scroll_active = false;
		graph_scrollup(0);
		bg_render_bombing = nullfunc_near;

		// Both of this function's shared tails are spelled as a forward
		// `goto` into the *next* arm, which is what the original's two `jmp`s
		// over an intervening `cmp` are. Same shape as TH04's
		// bomb_update_and_render(), and the same reason not to write the
		// statements out twice: Turbo C++'s tail merger stops at the first
		// conditional branch, so the [restore] block would only partly merge.
		goto playchar_bomb;
	} else if(bomb_frame < FRAME_FADE) {
playchar_bomb:
		yuuka_bomb_update_and_render();
		bomb_dream_decay();
	} else if(bomb_frame == FRAME_FADE) {
		snd_se_play(15);
		if(stage_scrolls()) {
			scroll_active = true;
		}
		items_pull_to_player = false;
		goto restore;
	} else if(bomb_frame < FRAME_END) {
restore:
		bg_render_bombing = bg_render_bombing_func;
		palette_settone_deferred(
			TONE_START - ((bomb_frame - FRAME_FADE) * TONE_PER_FRAME)
		);
		if((bomb_frame == (FRAME_FADE + 1)) && stage_scrolls()) {
			graph_scrollup(scroll_line);
		}
	} else {
		bombing = false;
		palette_settone_deferred(TONE_NORMAL);
	}
	bomb_frame++;

	#undef stage_scrolls
}
