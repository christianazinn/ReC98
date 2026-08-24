/// The bomb animation driver
/// -------------------------
/// One frame of a bomb, from the moment it is dropped to the moment [bombing]
/// clears again: the BB?.BB dissolve-in, the scroll and palette changes that
/// bracket the bomb proper, and the call into the playchar-specific animation
/// that does the actual damage. Called unconditionally every frame; the
/// [bombing] test at the top is what makes it a no-op the rest of the time,
/// and [bomb_frame] does not advance while it is false.
///
/// (#included from th04/main/player/shots_inv.cpp, ahead of bombanim.cpp and
/// that file's own two functions, which is this segment's original address
/// order: this body is the tail of SHOT_INV_TEXT's root contribution and the
/// C++ object begins exactly where it ends. kb/codegen/0129 + 0098 + 0114.)
///
/// Because this file shares a translation unit with shots_inv.cpp and
/// bombanim.cpp, its file-scope names are NOT file-local.
///
/// TH04-only: bomb.hpp guards the declaration with `#if (GAME == 4)`, and
/// th05_main.asm publishes no counterpart under any spelling.

// master.lib, declared here rather than through libs/master.lib/pc98_gfx.hpp,
// which pulls in libs/master.lib/func.hpp a second time; that header has no
// include guard and is already in this translation unit's closure. Copied
// verbatim from pc98_gfx.hpp, `extern "C"` included, so the linkage cannot
// drift from the real declarations. Same reason th04/main/player/bomb.cpp
// gives for its own local block.
extern "C" {
	extern Palette8 __cdecl Palettes;
	extern unsigned int __cdecl PaletteTone;
	void MASTER_RET graph_scrollup(unsigned line);
}

// `extern "C"` and `pascal`, because th04/main/player/bb_playchar_put.cpp
// publishes the undecorated, all-uppercase linker name
// (kb/codegen/0086 + 0102); `near`, because the function removes its argument.
// No header
// declares it -- this is its first C++ caller in the tree, and a header would
// have to be shared with TH05, which calls the same proc from its own dump.
extern "C" void pascal near bb_playchar_put(int cel);

// The regular value of palette color [COL_BOMB], saved on the frame the bomb
// overwrites that color and restored on the frame the bomb starts fading out.
// A private label in th04_main.asm's BSS (`rgb_257D6 rgb_t <?>`) with no
// `public` of its own, so the dump grew the zero-byte alias kb/codegen/0123
// prescribes. Named for what it demonstrably holds; nothing here identifies
// what color 14 is used for outside a bomb, so the index stays in the name.
extern "C" RGB8 bomb_col14_backup;

void near bomb_update_and_render(void)
{
	enum {
		// The BB?.BB dissolve-in, 16 cels: the first 8 at 4 frames each, then
		// the remaining 8 at 2. [FRAME_BB_FAST] is where the rate changes and
		// [BB_CEL_AT_FAST] is the cel reached by then, which is why the second
		// arm subtracts it.
		BB_CEL_FRAMES_SLOW = 4,
		BB_CEL_FRAMES_FAST = 2,
		FRAME_BB_FAST = 32,
		BB_CEL_AT_FAST = (FRAME_BB_FAST / BB_CEL_FRAMES_SLOW),

		// The bomb proper starts: the scroll stops, the background renderer is
		// disabled, color [COL_BOMB] turns pink and [playchar_bomb_func] takes
		// over. bombanim.cpp's [BOMB_STARS_SPAWN_FRAME] is the same 48 and the
		// same moment, but that file is #included *after* this one, so the
		// constant cannot be shared without moving it.
		FRAME_BOMB = 48,

		// The bomb ends: the scroll resumes, the palette is put back, and the
		// tone walks from [TONE_WHITE] down towards [TONE_NORMAL] two units
		// per frame until [FRAME_END].
		FRAME_FADE = 176,
		FRAME_END = 226,
		TONE_PER_FRAME = 2,

		COL_BOMB = 14,
		COL_BOMB_R = 240,
		COL_BOMB_G = 176,
		COL_BOMB_B = 192,

		TONE_WHITE = 200,
		TONE_NORMAL = 100,

		// Left behind for whatever draws circles next; nothing in this
		// function reads it back.
		COL_CIRCLES_AFTER_BOMB = 13,
	};
	if(!bombing) {
		return;
	}
	if(bomb_frame < FRAME_BB_FAST) {
		bb_playchar_put(bomb_frame / BB_CEL_FRAMES_SLOW);
	} else if(bomb_frame < FRAME_BOMB) {
		bb_playchar_put((bomb_frame / BB_CEL_FRAMES_FAST) - BB_CEL_AT_FAST);
	} else if(bomb_frame == FRAME_BOMB) {
		scroll_active = false;
		graph_scrollup(0);
		bg_render_bombing = nullfunc_near;
		bomb_col14_backup.c.r = Palettes[COL_BOMB].c.r;
		bomb_col14_backup.c.g = Palettes[COL_BOMB].c.g;
		bomb_col14_backup.c.b = Palettes[COL_BOMB].c.b;
		Palettes[COL_BOMB].c.r = COL_BOMB_R;
		Palettes[COL_BOMB].c.g = COL_BOMB_G;
		Palettes[COL_BOMB].c.b = COL_BOMB_B;

		// Both of this function's shared tails are spelled as a forward
		// `goto` into the *next* arm, which is what the original's two
		// `jmp`s over an intervening `cmp` are. kb/codegen/0097 says to
		// write the statements out twice and let `-O` cross-jump them
		// instead -- MEASURED, that is right for the one-call tail here and
		// WRONG for [restore] below: Turbo C++'s tail merger walks backwards
		// and stops at the first CONDITIONAL branch, so a shared tail that
		// contains an `if` merges only the part after that `if`'s test. The
		// duplicated spelling built and linked and left the function 0x36
		// bytes TOO LONG, with most of that block emitted twice.
		goto playchar_bomb;
	} else if(bomb_frame < FRAME_FADE) {
playchar_bomb:
		playchar_bomb_func();
	} else if(bomb_frame == FRAME_FADE) {
		snd_se_play(15);
		scroll_active = true;
		items_pull_to_player = false;
		goto restore;
	} else if(bomb_frame < FRAME_END) {
restore:
		Palettes[COL_BOMB].c.r = bomb_col14_backup.c.r;
		Palettes[COL_BOMB].c.g = bomb_col14_backup.c.g;
		Palettes[COL_BOMB].c.b = bomb_col14_backup.c.b;
		bg_render_bombing = bg_render_bombing_func;
		palette_settone_deferred(
			TONE_WHITE - ((bomb_frame - FRAME_FADE) * TONE_PER_FRAME)
		);
		if(bomb_frame == (FRAME_FADE + 1)) {
			graph_scrollup(scroll_line);
		}
	} else {
		bombing = false;
		palette_settone_deferred(TONE_NORMAL);
		circles_color = COL_CIRCLES_AFTER_BOMB;
	}
	bomb_frame++;
}
