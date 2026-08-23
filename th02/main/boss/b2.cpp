/// Stage 2 Boss - Meira
/// --------------------
/// The fight's [boss_init] and [boss_end] entry points. They are the last two
/// bodies of th02_main.asm's contribution to BOSS_5_TEXT, so the C++ that
/// replaces them has to be the first thing that contribution's successor emits
/// (kb/codegen/0099).
///
/// A NEW OBJECT rather than a prepend into th02/main/midboss/mx.cpp, which is
/// the successor the map names today. Two reasons, and the second is the one
/// that decides it: Meira is a stage boss and mx.cpp is the Extra Stage
/// midboss, so one file would be about two unrelated things; and mx.cpp carries
/// a scoped `-a2` for its pool struct, which is exactly the kind of neighbour
/// tools/pi-audit/carve_free_tails.py flags a PARITY RISK against for an
/// odd-length prepend. `[measured]` Every BOSS_5_TEXT contribution carries
/// ACBP=28 in obj/th02/main.map - BYTE alignment - so a new object exactly as
/// long as the bytes the root gives up leaves every later contribution at the
/// offset it had, and it costs one Tupfile.lua line between th02/dialog.cpp
/// and th02/main/midboss/mx.cpp.
///
/// Everything meira_update() dispatches to is still in the dump above, and so
/// is meira_update() itself, which is what the next parcel out of this block
/// takes. It carries a generated jump table with a one-byte pad, so THIS object
/// will need `-a2` when it grows to hold it, and that pad's parity will then be
/// a function of this object's own prefix (kb/codegen/0119 + 0160).

// -zC, because the segment name would otherwise come from this file's own
// basename and be B2_TEXT (kb/codegen/0105). -zPmain_03 for the near calls
// that leave this segment: every dialog_* entry point below is in DIALOG_TEXT
// and reachable near only because BOSS_5_TEXT is in the same group, which is
// also how th02_main.asm reached them from these very two procs. -G, because
// both prologs are `push bp; mov bp, sp` rather than an `enter`
// (kb/codegen/0011). No -a2: neither body here emits a generated jump table,
// and neither declares a struct whose stride an alignment could change
// (kb/codegen/0170).
#pragma option -zCBOSS_5_TEXT -zPmain_03 -G

#include "platform.h"
#include "pc98.h"
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th01/rank.h"
#include "th02/core/globals.hpp"
#include "th02/hardware/pages.hpp"
#include "th02/hardware/input.hpp"
#include "th02/main/playfld.hpp"
#include "th02/main/scroll.hpp"
#include "th02/main/boss/boss.hpp"
#include "th02/main/bullet/bullet.hpp"
#include "th02/main/explode.hpp"
#include "th02/main/spark.hpp"
#include "th02/main/stage/stage.hpp"
#include "th02/main/stage/bonus.hpp"
#include "th02/main/dialog/dialog.hpp"
#include "th02/main/hud/overlay.hpp"

// th02/main/dialog/dialog.cpp. dialog.hpp declares every dialog_script_*
// function but neither of these two, which is how th02/main/boss/b3.cpp, b4.cpp
// and b5.cpp already declare them.
void near dialog_pre(void);
void near dialog_post(void);

// th02/main/stage/init.cpp, which declares it identically - including the
// non-const parameter, which is why [aBoss4_m] below is not const either.
// `[measured]` Stops the current KAJA song, snd_load()s [fn] over it with
// SND_LOAD_SONG, and starts it again; every boss init function in this binary
// switches its BGM through it.
extern "C" void far sub_13ABB(char *fn);

// The Stage 2 boss BGM's file name, kept where th02_main.asm's own `_DATA`
// contribution defines it rather than re-emitted as a literal here: this
// translation unit contributes no initialized data at all, so a literal would
// land after the dump's whole `_DATA` block and shift every byte between.
// Declared exactly the way b3.cpp declares [aBoss2_m] and b4.cpp [aBoss3_m]
// for the same call one and two bosses later.
extern "C" char aBoss4_m[];

// The sprite the boss and midboss renderers blit, shared by all of them and
// written from ~150 sites across th02_main.asm. `patnum_2064E` is the dump's
// own spelling and is not an IDA placeholder; retiring the address suffix means
// ruling on all of those sites at once, which is its own parcel.
extern "C" int patnum_2064E;

// The five-byte rank-scaled parameter block every boss and midboss init
// function fills a contiguous prefix of, documented in full at
// th02/main/boss/b4.cpp and state/notes/th02-boss-rank-param.md. Meira uses
// three cells and all three of hers really are bullet groups: cell 1 goes
// straight to bullets_add_pellet() from meira_14BC2(), cell 2 from
// meira_14C76() and meira_14E30(), and cell 0 is copied into her own pattern
// state by meira_14A39().
extern "C" uint8_t boss_rank_param[5];

// th02/main/bullet/bullet.cpp, which owns the `[inferred]` licence for this
// name. NOT a per-boss progression counter - see [meira_phase] below - but the
// binary-wide "the boss on screen has been defeated" flag: 0 while Meira is
// fighting, and meira_update() switches to her defeat tail on the frame it
// turns 1.
extern "C" uint8_t boss_phase;

/// Meira's own fight state
/// -----------------------
/// All three are kb/codegen/0123 label aliases for now, because meira_update()
/// and meira_bg_render() are still ASM and hold references to every one of
/// them. They collapse into ordinary renames when this chain empties, exactly
/// the way Sigma's three did (state/notes/sigma_update.md).

// Which of her three pattern GROUPS she is in, advanced by meira_update() when
// [boss_damage] passes 700 and then 1500. NOT [boss_phase], which is the defeat
// flag above - the same distinction th02/main/boss/b3.cpp draws for
// [stones_phase], b5m.cpp for [mima_phase] and b6.cpp for [sigma_phase], and
// this is the fourth holder of that shape in this binary.
extern "C" uint8_t meira_phase;

// Which of the current group's patterns runs this frame; meira_update() cycles
// it and wraps it at four, three and two patterns respectively, and resets it
// to 0 at every group change. Same role as [sigma_pattern], [mima_pattern] and
// [marisa_pattern], and like them it is NOT [meira_phase].
extern "C" uint8_t meira_pattern;

// KEEPS ITS ADDRESS SUFFIX ON PURPOSE, which is the shape b4.cpp's
// [marisa_1AA60] already uses for a symbol whose name is a separate decision.
// `[measured]` It is a `dw` written only 0 and 1 - meira_14E9D() raises it and
// meira_init() below clears it - and read at exactly one site, where it gates
// meira_bg_render()'s pass over three per-page background slots. Naming it
// needs that renderer and that pattern, and neither is in this parcel; naming
// it from the gate alone would be a guess about what those three slots are.
extern "C" int16_t meira_250FE;
/// -----------------------


// Runs Meira's post-battle dialog and the stage clear bonus, then advances to
// Stage 3. Installed into [boss_end] by stage_init(); stones_end() in
// th02/main/boss/b3.cpp is the same function one stage later, and differs only
// in the key_delay() and the [spark_accel_x] reset below.
extern "C" void far meira_end(void)
{
	dialog_pre();
	dialog_script_generic_part_animate(DS_POSTBOSS);
	stage_clear_bonus_animate();
	key_delay();
	overlay_stage_leave_animate();
	stage_id++;

	// Her stage tilts the sparks sideways; Stage 3 gets them back upright.
	spark_accel_x.v = 0;
}


// Runs Meira's pre-battle dialog, flashes her onto both pages, switches to her
// BGM and resets the fight. Installed into [boss_init] by stage_init().
extern "C" void far meira_init(void)
{
	// The VRAM row her entrance frame is blitted at, which is not her screen y
	// - see below.
	register vram_y_t vram_y;

	patnum_2064E = 141;
	dialog_pre();
	dialog_script_stage2_pre_intro_animate();

	boss_left_on_page[0] = (PLAYFIELD_LEFT + (PLAYFIELD_W / 2) - 32);
	boss_left_on_page[1] = boss_left_on_page[0];
	boss_top_on_page[0] = (PLAYFIELD_TOP + 32);
	boss_top_on_page[1] = (PLAYFIELD_TOP + 32);
	boss_left_on_back_page = &boss_left_on_page[page_back];
	boss_top_on_back_page = &boss_top_on_page[page_back];

	palette_white_out(1);

	// Her y is a SCREEN y in [boss_top_on_page] and a VRAM ROW here, and the
	// wrap is the conversion: the tile layer scrolls under her, so the row she
	// has to be blitted at moves with [scroll_line] even though she does not.
	vram_y = (PLAYFIELD_TOP + 32);
	vram_y += scroll_line;
	if(vram_y >= RES_Y) {
		vram_y -= RES_Y;
	}
	super_roll_put(*boss_left_on_back_page, vram_y, patnum_2064E);

	// sub_13ABB() is far and lands in this same physical segment, so the
	// original reaches it through the linker-relaxed `nop; push cs; call near
	// ptr` form that no plain C++ far call reproduces (kb/codegen/0083). That
	// form cannot see the C++ expressions either, so its far pointer argument
	// and its __cdecl cleanup are hand-spelled with it.
	//
	// `[measured]` The cleanup is this call's own 4 bytes and nothing else -
	// every other call in this body either takes no argument or is `pascal` and
	// cleans itself - so the island does NOT have to reach backwards the way
	// marisa_init()'s does (kb/codegen/0083's addendum). Nothing in it names a
	// register, so [vram_y] stays in SI.
	__emit__(0x1E);	// push ds
	_asm { push offset aBoss4_m; }
	__emit__(0x90);	// nop
	__emit__(0x0E);	// push cs
	_asm { call near ptr sub_13ABB; }
	__emit__(0x83, 0xC4, 0x04);	// add sp, 4

	palette_white_in(1);
	dialog_script_generic_part_animate(DS_PREBOSS);
	dialog_post();

	boss_damage = 0;
	boss_phase = 0;

	// Her aimed spreads are the special motion this bounds, and 2 turns is the
	// lowest any boss in this binary sets it to.
	bullet_special.u3.turns_max = 2;

	boss_explode_angle_offset = 0;
	meira_phase = 0;
	meira_pattern = 0;
	meira_250FE = 0;

	// `[measured]` Cell 0 is rank-INVARIANT, so the difficulty of her fight is
	// carried entirely by the two spreads: five bullets wide and medium-aimed
	// above Easy, three of each on it.
	if(rank != RANK_EASY) {
		boss_rank_param[0] = BG_1_RANDOM_ANGLE;
		boss_rank_param[1] = BG_5_SPREAD_WIDE;
		boss_rank_param[2] = BG_5_SPREAD_MEDIUM_AIMED;
	} else {
		boss_rank_param[0] = BG_1_RANDOM_ANGLE;
		boss_rank_param[1] = BG_3_SPREAD_WIDE;
		boss_rank_param[2] = BG_3_SPREAD_MEDIUM_AIMED;
	}
}
