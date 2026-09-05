/// Stage 5 boss - Mima
/// -------------------
/// Her entrance, both of her defeats, the two-line epilogue that ends the
/// binary, and the Stage 4 background scroller that sits directly below them.
/// Between them they are the WHOLE of what th02_main.asm used to contribute to
/// the nameless code segment that th02/main/midboss/m4.cpp and
/// th02/main/boss/b4.cpp already share, so that contribution is now empty and
/// this translation unit needs no split and no new segment - it just leads the
/// segment, ahead of m4.cpp. (kb/codegen/0099)
///
/// stage4_update_and_render() is NOT hers: it is Stage 4's own [stage_update_and_render], and
/// it lives here for the same reason the boss-entrance helpers live in b4.cpp -
/// this is the object that reaches its address.

// -G, because stage4_update_and_render()'s prolog is `push bp; mov bp, sp; sub sp, 4` rather
// than an `ENTER`. (kb/codegen/0011) No -a2: `[measured]` nothing in this
// object emits a generated jump table, so there is no alignment to pin.
#pragma option -zCmain_03__TEXT -zPmain_03 -G

#include "platform.h"
#include "pc98.h"
#include "decomp.hpp"
#include "planar.h"
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/resident.hpp"
#include "th02/core/globals.hpp"
#include "th02/hardware/pages.hpp"
#include "th02/hardware/frmdelay.h"
#include "th02/hardware/egc.hpp"
#include "th02/main/playfld.hpp"
#include "th02/main/scroll.hpp"
#include "th02/main/tile/tile.hpp"
#include "th02/main/frames.hpp"
#include "th02/main/boss/boss.hpp"
#include "th02/main/hiscore.hpp"
#include "th02/main/execl.hpp"
#include "th02/main/dialog/dialog.hpp"
#include "th02/main/bullet/bullet.hpp"
#include "th02/main/player/player.hpp"
#include "th02/main/player/shot.hpp"
#include "th02/sprites/main_pat.h"

// The sprite the boss and midboss renderers blit. Declared exactly the way
// th02/main/boss/b4.cpp already declares it.
extern "C" int patnum_2064E;

// th02/main/dialog/dialog.cpp. dialog.hpp declares every dialog_script_*
// function but neither of these two, which is how th02/main/boss/b4.cpp
// already declares dialog_pre().
void near dialog_pre(void);
void near dialog_post(void);

// The three shared boss-entrance helpers, in th02/main/boss/b4.cpp because
// that is the object reaching their addresses rather than because they are
// Marisa's. None of them has a header; b4.cpp defines them with C linkage and
// the root ASM still declares two of them for sigma_init()'s sake.
extern "C" void near boss_playfield_reset(void);
extern "C" void near boss_entrance_animate(void);
extern "C" void near boss_bg_rows_put(void);

// Still ASM, in BOSS_5_TEXT. It fills the rank-scaled boss parameter block
// that Mima's pattern functions read back (th02/main/boss/b4.cpp's
// [boss_rank_param]), and mima_init() below is its only caller.
extern "C" void near mima_180AC(void);

// th02/snd/snd.h, declared here rather than included for the same reason as
// snd_kaja_interrupt() above, and the same way th02/main/boss/b4.cpp declares
// this exact pair.
extern "C" void __cdecl snd_se_play(int new_se);
extern "C" void __cdecl snd_se_update(void);

// th02/main/boss/b1.cpp. b4.cpp declares it identically for its own call.
extern "C" void far boss_bgm_load(char *fn);

// th02/main/boss/b5_.cpp. It has no header of its own; this and mima_19C1D()
// below are the only two functions that reach it.
void near skill_calculate(void);

// libs/kaja/kaja.h and th02/snd/snd.h, spelled out here rather than included,
// the way th02/main/boss/b4.cpp already spells out snd_se_play(): neither
// header has an include guard, and snd.h pulls in three more unguarded ones
// for the sake of one call.
extern "C" void __cdecl snd_kaja_interrupt(int func_and_param);

// "maine", as it already exists in the root ASM's _DATA. A C++ string literal
// would add a second copy rather than reuse the one this call site owns, so
// the existing label is referenced directly - the same thing th04/main/end.cpp
// does for its own four copies. `aMaine_0` is the dump's own spelling and is
// not an IDA placeholder: PLACEHOLDER_RE's string-auto pattern needs a letter
// after the underscore.
extern "C" const char aMaine_0[];

// "mima.bft" and "mima1.bft", her 256x256 dialog portrait and her unsealed
// first-form sprites, and "stage3_b.btt" beside them, all as they exist in the
// root ASM's _DATA - a C++ string literal would add a second copy of each
// rather than reuse the one this call site owns. th02/main/boss/b4.cpp holds
// the *other* copy of two of these three strings, at different addresses, and
// keeps IDA's spelling for them because that spelling happens to escape
// PLACEHOLDER_RE's string auto-name pattern. These two do not escape it, so
// they are renamed, the way th03/hiscore/regist.cpp already names the
// registration screen's sheets.
extern "C" const char mima_bft[];
extern "C" const char mima1_bft[];
extern "C" const char aStage3_b_btt[];

// "mima.m", the song Mima's fight plays. IDA's own spelling is short enough to
// escape the string auto-name pattern, so it stays, exactly as
// th02/main/boss/b4.cpp keeps `aBoss3_m` for the same call one boss earlier.
extern "C" const char aMima_m[];

// "mima2.bft", the sprite sheet for Mima's second, winged form, as it exists
// in the root ASM's _DATA. A C++ string literal would add a second copy rather
// than reuse the one this call site owns. mima_19C8D() below holds the only
// reference left, so IDA's string auto-name is retired rather than aliased;
// the spelling follows the one th03/hiscore/regist.cpp already uses for the
// registration screen's two sheets.
extern "C" const char mima2_bft[];

// Raised at phase 7 and cleared by mima_init() and by her first defeat below.
// It both widens the pattern cycle from three patterns to eight and switches
// six of the pattern functions to their harder variant. A kb/codegen/0123
// alias for `byte_26CC0`, because twelve reads across her still-ASM pattern
// and render functions keep the dump's spelling.
extern "C" bool mima_all_patterns;

// The two pieces of shared boss state mima_init() clears, both already
// published and both declared exactly the way th02/main/boss/b4.cpp and
// th02/main/midboss/m4.cpp already declare them.
extern "C" uint8_t boss_phase;
extern "C" bool boss_hit_flash;

// Mima's fight state, all of it still read by her ASM pattern and render
// functions, so all of it kb/codegen/0123 aliases rather than renames.
//
// [mima_damage_multiplier] is 0 until the step's opening patterns are past and
// 1 afterwards, and the frame's shot damage is multiplied by it - so she cannot
// be damaged at all during those patterns.
extern "C" int16_t mima_damage_multiplier;
extern "C" int16_t mima_phase;
extern "C" int16_t mima_pattern;
extern "C" int16_t mima_patterns_this_phase;

// Her background: one ring of radius [mima_bg_ring_radius] around a fixed
// (224, 200), drawn three times per frame at three angles eight steps apart,
// and one filled circle of radius [mima_bg_circle_radius] at the same centre.
// `[measured]` The newest of the three ring passes is drawn in
// [mima_bg_ring_col_head] and the two lagging ones in
// [mima_bg_ring_col_tail], which is what makes the ring read as a comet;
// mima_init() starts them one step apart on the same colour ramp the dump
// already publishes for Marisa's particles, and only the tail colour is ever
// written again.
extern "C" uint8_t mima_bg_ring_radius;
extern "C" uint8_t mima_bg_circle_radius;
extern "C" uint8_t mima_bg_ring_col_head;
extern "C" uint8_t mima_bg_ring_col_tail;
extern "C" uint8_t mima_bg_circle_col;

// Her orbs, the same mechanism Marisa's four run on one boss earlier, down to
// the 24x32 hitbox and the corner-centred player box. 0 is absent, 1 alive and
// 2 "unblit me on the next frame" - deliberately NOT b4.hpp's MOF_ values, and
// deliberately not shared with them. `[measured]` Every writer bounds at four,
// so the upper four slots stay 0 for the whole fight, and the hit test's
// return value is discarded, so the orbs absorb player shots without ever
// taking damage.
extern "C" int16_t mima_orb_flag[8];

// The top of the first row of tiles stage4_update_and_render() copies, wrapped around the
// 400-line VRAM. A rename; that function holds every reference.
extern "C" vram_y_t stage4_tile_top;


// Her entrance: the portrait dialog, the handoff onto her unsealed first-form
// sprites, and every piece of fight state the eleven-step phase machine starts
// from. stage_init() installs it as Stage 5's [boss_init].
//
// marisa_init() in th02/main/boss/b4.cpp is this function's twin statement for
// statement down to the hand-spelled island; where the two differ, the
// difference is a constant.
extern "C" void far mima_init(void)
{
	register int i;

	boss_playfield_reset();
	dialog_pre();
	super_clean(128, 192);
	super_entry_bfnt(mima_bft);
	dialog_script_stage5_pre_intro_animate();
	vsync_Count1 = 0;
	frame_delay(10);
	boss_entrance_animate();

	// Her own three 48x96 patterns replace the 256x256 portrait.
	super_clean(128, 192);
	super_patnum = 128;
	super_entry_bfnt(mima1_bft);
	super_entry_bfnt(aStage3_b_btt);

	boss_left_on_page[0] = (PLAYFIELD_LEFT + (PLAYFIELD_W / 2) - 80);
	boss_left_on_page[1] = boss_left_on_page[0];
	boss_top_on_page[0] = (PLAYFIELD_TOP + 48);
	boss_top_on_page[1] = boss_top_on_page[0];
	patnum_2064E = 128;
	dialog_script_stage5_pre_unsealed_animate();
	palette_white_out(1);
	grc_setclip(PLAYFIELD_LEFT, 0, PLAYFIELD_RIGHT, (RES_Y - 1));
	grcg_setcolor(GC_RMW, 0);
	grcg_fill();
	grcg_off();
	super_put_rect(
		boss_left_on_page[page_back], boss_top_on_page[page_back], patnum_2064E
	);
	super_put_rect(
		(boss_left_on_page[page_back] + 48),
		boss_top_on_page[page_back],
		(patnum_2064E + 1)
	);
	super_put_rect(
		(boss_left_on_page[page_back] + 96),
		boss_top_on_page[page_back],
		(patnum_2064E + 2)
	);
	super_roll_put(player_topleft.x, player_topleft.y, PAT_PLAYCHAR_STILL);
	super_roll_put_tiny(
		player_option_left_topleft[page_back].x,
		player_option_left_topleft[page_back].y,
		PAT_OPTION_A
	);
	super_roll_put_tiny(
		(player_option_left_topleft[page_back].x +
			PLAYER_OPTION_TO_OPTION_DISTANCE
		),
		player_option_left_topleft[page_back].y,
		PAT_OPTION_A
	);

	// The same hand-spelled run marisa_init() carries, for the same two
	// reasons: boss_bgm_load() is reached through `nop; push cs; call near ptr`,
	// which no C++ far call reproduces, and snd_se_play()'s __cdecl cleanup is
	// deferred four statements down into the `add sp, 6` at the bottom. The
	// first inline-asm statement anywhere in the function turns that deferral
	// off, so the earlier argument has to be pushed by hand as well and Turbo
	// C++ must account for neither. (kb/codegen/0083)
	//
	// Nothing in here names a register, so [i] stays in SI.
	__emit__(0x6A, 0x0A);               // push 10
	_asm { call far ptr snd_se_play; }
	snd_se_update();
	boss_bg_rows_put();
	palette_white_in(3);
	__emit__(0x1E);                     // push ds
	_asm { push offset aMima_m; }
	__emit__(0x90);                     // nop
	__emit__(0x0E);                     // push cs
	_asm { call near ptr boss_bgm_load; }
	__emit__(0x83, 0xC4, 0x06);         // add sp, 6

	dialog_script_stage5_pre_winged_animate();
	dialog_post();
	shots_free_all();
	boss_damage = 0;
	boss_phase = 0;
	boss_phase_frame = 0;
	boss_hit_flash = false;
	mima_phase = 0;
	mima_damage_multiplier = 0;
	mima_patterns_this_phase = 0;
	tile_mode = TM_NONE;
	mima_bg_ring_radius = 0;
	mima_bg_circle_radius = 0;
	mima_bg_ring_col_head = 13;
	mima_bg_circle_col = 3;
	mima_bg_ring_col_tail = 12;
	graph_accesspage(page_front);
	graph_clear();
	graph_accesspage(page_back);
	graph_clear();

	// The two side columns, outside the playfield.
	grcg_setcolor(GC_RMW, 11);
	grc_setclip(PLAYFIELD_RIGHT, 0, (RES_X - 1), (RES_Y - 1));
	graph_accesspage(page_front);
	grcg_fill();
	graph_accesspage(page_back);
	grcg_fill();
	grcg_off();
	grc_setclip(PLAYFIELD_LEFT, 0, PLAYFIELD_RIGHT, (RES_Y - 1));

	mima_pattern = 0;
	for(i = 0; i < 8; i++) {
		mima_orb_flag[i] = 0;
	}
	mima_all_patterns = false;
	mima_180AC();
}


// The last thing this binary does: commit the run to the resident structure
// and launch MAINE.EXE over this process. mima_end() and mima_19C8D() below
// are its only two callers, and both reach it as a plain near call inside this
// one physical segment.
extern "C" void near mima_19C1D(void)
{
	palette_settone(0);
	resident->score_highest = ((resident->score_highest < score)
		? score
		: resident->score_highest
	);

	// ZUN quirk: The number of continues is *added* to the score rather than
	// stored beside it, which is what makes the resident score a decimal digit
	// wider than the in-game one. MAINE.EXE splits the two apart again.
	resident->score = ((score * 10) + resident->continues_used);

	// 127 is STAGE_ALL, from th02/formats/scoredat/scoredat.hpp. That header
	// cannot be included here: it re-includes the unguarded th02/score.h.
	resident->stage = 127;

	resident->rem_lives = lives;
	resident->rem_bombs = bombs;
	skill_calculate();
	GameExecl(aMaine_0);
}


// Her first defeat, at the end of phase 8. Returns `true` if the game ends
// right here - which it does on a continued run, where the winged second form
// is skipped entirely - and `false` if the fight goes on into that form.
extern "C" bool16 near mima_19C8D(void)
{
	shots_free_all();

	// bullets_clear() is far and lands in this same physical segment, so the
	// original reaches it through the linker-relaxed `nop; push cs;
	// call near ptr` form that no plain C++ far call reproduces, not even
	// within one group. (kb/codegen/0083) Turbo C++'s inline assembler
	// resolves the identifier as a *C* symbol and rejects the `$` in a mangled
	// one, which is why bullets_clear() has C linkage.
	__emit__(0x90);	// nop
	__emit__(0x0E);	// push cs
	_asm { call near ptr bullets_clear; }

	super_put_rect(
		boss_left_on_page[page_back], boss_top_on_page[page_back], patnum_2064E
	);
	super_put_rect(
		(boss_left_on_page[page_back] + 48),
		boss_top_on_page[page_back],
		(patnum_2064E + 1)
	);
	super_put_rect(
		(boss_left_on_page[page_back] + 96),
		boss_top_on_page[page_back],
		(patnum_2064E + 2)
	);
	dialog_pre();
	dialog_script_stage5_form1defeat_animate();
	if(resident->continues_used) {
		// `snd_kaja_func(KAJA_SONG_FADE, 10);` is the expression, but Turbo
		// C++ cleans this __cdecl call's single stack word with `add sp, 2`
		// *here*, while the original uses `pop cx`. The compiler picks between
		// the two forms per call site and nothing about the callee, the macro
		// or the flags changes its mind, so the three instructions are pinned
		// by hand - exactly as th02/main/entry.cpp already pins the same call.
		// (kb/codegen/0132)
		// 020Ah == ((KAJA_SONG_FADE << 8) | 10).
		_asm {
			push	020Ah;
			call	far ptr snd_kaja_interrupt;
			pop 	cx;
		}

		palette_white_out(10);
		frame_delay(50);
		score += 50000;
		mima_19C1D();
		return true;
	}
	super_clean(128, 192);
	super_patnum = 128;
	super_entry_bfnt(mima2_bft);
	dialog_post();
	graph_accesspage(page_front);
	grcg_setcolor(GC_RMW, 0);
	grcg_fill();
	graph_accesspage(page_back);
	grcg_fill();
	grcg_off();
	mima_all_patterns = false;
	return false;
}


// Her second and final defeat, and the end of the game.
extern "C" void far mima_end(void)
{
	// ZUN bug: This renders Mima's sprite to [page_back] on top of the boss
	// background rendered earlier, right before animating the defeat dialog on
	// [page_front] and launching into MAINE.EXE, which means that none of this
	// will ever show up. Given the fact that this code exists, it probably was
	// ZUN's intention to render the defeat dialog on top of what's rendered
	// here - i.e, before rendering any player shot, item, bullet, or spark
	// sprites - rather than blindly printing the text on top of whatever white
	// VRAM pixels that may have been in the text box area in the previous full
	// frame.
	super_put_rect(
		boss_left_on_page[page_back], boss_top_on_page[page_back], patnum_2064E
	);
	super_put_rect(
		(boss_left_on_page[page_back] + 48),
		boss_top_on_page[page_back],
		(patnum_2064E + 1)
	);
	super_put_rect(
		(boss_left_on_page[page_back] + 96),
		boss_top_on_page[page_back],
		(patnum_2064E + 2)
	);
	dialog_script_stage5_post_animate();
	score += 100000;
	if(!resident->continues_used) {
		scoredat_cleared_set();
	}
	mima_19C1D();
}


// Stage 5's background: the 6x6 block of tiles that scrolls behind the fight,
// copied through the EGC on [page_back] frames only.
//
// `[measured]` Stage 4 installs this as [stage_update_and_render] and only if
// [reduce_effects] is clear, so on a slow machine the stage runs without it.
extern "C" void far stage4_update_and_render(void)
{
	// [tile_image] first: Turbo C++ allocates stack locals in declaration
	// order, and the original's frame is `tile_image` at [bp-2] and `top` at
	// [bp-4]. (kb/codegen/0010)
	int tile_image;
	vram_y_t top;
	register screen_x_t x;
	int image;

	if(page_back != 0) {
		return;
	}
	egc_on();
	outport2(EGC_ACTIVEPLANEREG, 0xFFF7);
	outport2(EGC_READPLANEREG, 0x03FF);
	outport2(EGC_MASKREG, 0xFFFF);
	outport2(EGC_ADDRRESSREG, 0);
	outport2(EGC_BITLENGTHREG, 0x000F);
	// Literals rather than the constant names, because outport2() puts its
	// value straight into an `_asm` block where `|` is not an operator. This is
	// how th01/hardware/egc_impl.hpp already spells the same register.
	/* EGC_WS_PATREG | EGC_RL_MEMREAD */
	outport2(EGC_MODE_ROP_REG, 0x1100);
	tiles_invalidate_rect(176, 156, 96, 96);
	tiles_egc_render();
	/* EGC_WS_ROP | 11111100b */
	outport2(EGC_MODE_ROP_REG, 0x08FC);
	stage4_tile_top = 152;
	stage4_tile_top += scroll_line;
	if(stage4_tile_top >= RES_Y) {
		stage4_tile_top -= RES_Y;
	}
	top = stage4_tile_top;
	for(tile_image = 0; tile_image < 0x60; tile_image += 0x10) {
		for(x = 176, image = tile_image; x < 272; x += TILE_W, image++) {
			tile_egc_roll_copy_8(x, top, image);
		}
		top += TILE_H;
		if(top >= RES_Y) {
			top -= RES_Y;
		}
	}
	egc_off();
}
