/// Per-stage initialization
/// ------------------------
/// Resets the playfield simulation for a new stage: the stage-global state
/// (through stage_state_reset(), which th04/main/stage/reset.cpp owns because
/// it sat in a different original object), then the scroll, the player, the
/// per-stage item counters and the .STD update function, and finally every
/// subsystem that keeps a reset function of its own. stage_setup() calls it
/// first thing, which is the whole of its own body's purpose.
///
/// The name is TH02's, for TH02's function in the same role at the same place
/// in its own call chain (th02/main/stage/init.cpp). The split against
/// stage_state_reset() looks like a source-file boundary rather than a
/// conceptual one -- [inferred], and already recorded as such at the head of
/// th04/main/stage/reset.cpp, whose prose called this function "the per-stage
/// initialization function" back while it was still assembly.
///
/// TH05's twin is sub_B55A, and this file is now SHARED with it under
/// `#if (GAME == 5)`: the same body in the same order, minus
/// [dream_items_collected], shot_reset(), thicklasers_reset() and the four write-only
/// words below, and with a [shot_time] reset where TH04 has none.
///
/// This paragraph used to say the opposite -- that the TH05 half was
/// "deliberately NOT shared" because sub_B55A "sits mid-segment in
/// th05_main.asm and would need a kb/codegen/0080 carve first", and because
/// Tupfile.lua compiles this source for TH04 alone so a GAME == 5 arm "would
/// never be graded by the oracle". Both halves expired without being
/// re-measured. MATCH-TH05-MAIN-TAILS-1 lifted the two procs that followed
/// sub_B55A out of DEMO_TEXT, which left it the carve-free TAIL of that root
/// contribution rather than something mid-segment; and th05/demo.cpp #includes
/// this file exactly the way th05/main011.cpp already #includes
/// th04/main/stage/reset.cpp, which is what puts the TH05 arm in front of the
/// oracle. Transferable, and the second time this rule has been paid for after
/// sub_EACE (state/notes/th05-main-tail-lifts.md): a "needs a carve" verdict is
/// a statement about what FOLLOWS a proc, and every lift in the same segment
/// can retire it.
///
/// ZUN's own name for the TH05 half survives in th05_main.asm as an IDA comment
/// spelling a CStage method, InitChara, immediately above the proc, and
/// state/notes/sub_C5B0.md section 2 calls it the best naming evidence in its
/// dossier. It is recorded, NOT adopted: stage_init() is the name a naming
/// round already gave TH04's half from TH02's function in the same role, one
/// shared body can only carry one name, and re-opening it is a naming round's
/// parcel and not a lift's.
///
/// (#included from th04/demo.cpp and th05/demo.cpp, AHEAD of
/// th04/main/stage/transition.cpp in both, because this function is now the
/// last thing either dump contributes to DEMO_TEXT. Growing that object
/// backwards once more puts the body back at its original address and moves
/// nothing else (kb/codegen 0099 + 0114); no carve, no new segment name, no
/// Tupfile.lua line -- the third lift the TH04 wrapper has absorbed on those
/// terms, after pause() and stage_transition(), and the third the TH05 one
/// has.)
///
/// Being textually first in that translation unit, this file may only reach
/// GUARDED headers, and owns every unguarded one it would pull. th04/main/
/// playfld.hpp is guarded and brings the whole scroll block behind it; every
/// other declaration below is spelled out instead, exactly as
/// th04/main/continue.cpp and th04/main/stage/transition.cpp spell theirs
/// (kb/codegen/0129).

#include "platform.h"
#include "pc98.h"
#include "th04/main/playfld.hpp"
#include "th04/main/replay.hpp"

// Declarations for the unguarded headers this file cannot reach:
// th04/main/frames.h, th04/main/player/player.hpp, th02/main/player/player.hpp,
// th04/main/player/bomb.hpp, th04/main/player/shot.hpp, th04/main/tile/tile.hpp,
// th04/main/item/item.hpp, th04/main/dialog/dialog.hpp, th04/main/bg.hpp,
// th04/main/hud/hud.hpp, th04/main/pointnum/pointnum.hpp, th04/main/spark.hpp,
// th02/main/player/bomb.hpp and th02/math/randring.hpp.
// ---------------------------------------------------------------------
extern unsigned int stage_frame;
extern PlayfieldMotion player_pos;
extern unsigned char miss_time;
extern bool player_is_hit;
extern unsigned char player_invincibility_time;
extern bool bombing_disabled;
extern int tile_ring_row_filled;
// TH04 stores this in a single byte, TH05 in a word -- the same split
// th04/main/item/item.hpp already carries, respelled here because this file may
// not reach an unguarded header. Getting it wrong is not a warning: the reset
// below is the ONLY use of the variable in this body, so the type shows up as
// nothing but a word-store opcode against a byte-store one, and a one-byte
// shift of every instruction after it. Measured, one build cycle.
#if (GAME == 5)
	extern unsigned int stage_point_items_collected;
#else
	extern unsigned char stage_point_items_collected;
#endif
#if (GAME == 4)
	extern unsigned char dream_items_collected;
#else
	// TH05 resets this one where TH04 does not; th04/main/player/shot.hpp
	// declares it for everyone else, and this file may not reach a header.
	extern unsigned char shot_time;
#endif
extern bool (near* std_update)(void);
extern nearfunc_t_near bg_render_bombing_func;

bool near std_update_frames_then_animate_dialog_and_activate_boss_if_done(void);
void near stage_state_reset(void);
#if (GAME == 4)
	void near shot_reset(void);
#endif
void near bomb_reset(void);
void near tiles_invalidate_reset(void);
void pascal near tiles_render_all(void);
// Defined in th04/main/pointnum/inv_upd.cpp and exported with the original
// C/Pascal ABI. Kept local because this shared stage-init translation unit
// cannot safely include the point-number header's full closure.
extern "C" void pascal near pointnums_init(void);
void near randring_fill(void);
extern "C" void near sparks_init(void);
extern "C" void pascal near hud_score_put(void);
extern "C" void pascal hud_put(void);
// ---------------------------------------------------------------------

// Frames of invincibility the player starts a stage with. th04/shared.inc
// defines the same number for the dump under this same name; there is no C++
// header that carries it.
static const int STAGE_START_INVINCIBILITY_FRAMES = 64;

#if (GAME == 4)
	// Four words that this function is the ONLY writer of, and that nothing in
	// either TH04 dump or anywhere in the tree ever reads. `[measured]` -- the
	// four assignments below and the `dw ?` definitions in th04_main.asm's _BSS
	// are their complete reference set. TH05 does not have them at all.
	//
	// The name therefore records the only two facts the binary supports -- this
	// function writes them, nothing reads them -- and asserts nothing else. ZUN
	// bloat, named exactly as th04_main.asm's own [shot_unused] is named for
	// shot_reset(), which is TH02's [scroll_unused_2] device in turn.
	// ---------------------------------------------------------------------
	extern "C" uint16_t stage_init_unused_0;
	extern "C" uint16_t stage_init_unused_1;
	extern "C" uint16_t stage_init_unused_2;
	extern "C" uint16_t stage_init_unused_3;
#endif
// ---------------------------------------------------------------------

// The frames during which a miss locks the player out of moving. Both games
// place this byte between player invincibility and power and use it through the
// same stage-start, deathbomb, and per-frame update paths.
extern "C" unsigned char miss_move_lock_time;

// Far functions that this body was the only ASM caller of in the whole dump,
// reached through the zero-byte aliases each dump publishes beside them
// (kb/codegen/0123). No body was read for the parcel that wrote this and none
// was named here, because a call site is not evidence of what a function does.
//
// One of the two has since been read. The proc that stood between
// randring_fill() and bomb_reset() in BOTH games -- TH04's sub_1DA1B and
// TH05's sub_16D67, byte-identical apart from one address -- is now
// items_init() in th04/main/item/init.cpp. It turned out to be a call site
// whose position had told the truth: it sits with the other per-stage
// subsystem resets because that is what it is. Respelled here rather than
// reached through th04/main/item/item.hpp, which is one of the unguarded
// headers this file may not pull; the declaration below is the one that
// header carries.
#if (GAME == 4)
	extern "C" void far thicklasers_reset(void);
#endif
extern "C" void far items_init(void);

// Shared, byte-identical in both games.
extern "C" void near player_shot_level_update(void);

// Turbo C++ compiled ZUN's far calls to same-code-group functions as
// `nop; push cs; call near ptr`, which no plain C++ far call reproduces.
// (kb/codegen/0014, kb/codegen/0083)
#define nopcall_same_group(func) _asm { \
	nop; \
	push	cs; \
	call	near ptr func; \
}

void near stage_init(void)
{
	stage_state_reset();
	stage_frame = 0;
	bombing_disabled = false;
	scroll_line = 0;
	tile_ring_row_filled = 0;
	scroll_line_on_page[0] = 0;
	scroll_line_on_page[1] = 0;
	scroll_subpixel_line.v = 0;
	scroll_lines_pending = 0;
	scroll_lines_prev_frame = 0;
	playfield_shake_x = 0;
	playfield_shake_y = 0;
	player_pos.cur.x.v = (192 * 16);
	player_pos.cur.y.v = (320 * 16);
	player_pos.prev.x.v = (192 * 16);
	player_pos.prev.y.v = (320 * 16);
#if (GAME == 4)
	stage_init_unused_0 = 0x40;
	stage_init_unused_1 = 0x40;
	stage_init_unused_2 = 0x30;
	stage_init_unused_3 = 0x30;
#endif
	miss_move_lock_time = 0;
	miss_time = 0;
	player_is_hit = false;
	player_invincibility_time = STAGE_START_INVINCIBILITY_FRAMES;
	stage_point_items_collected = 0;
#if (GAME == 4)
	dream_items_collected = 0;
#else
	shot_time = 0;
#endif
	replay_practice_start_apply_and_stage_activate();
#if (GAME == 4)
	shot_reset();
	nopcall_same_group(player_shot_level_update);
	randring_fill();
	items_init();
	replay_practice_items_ready();
#else
	nopcall_same_group(player_shot_level_update);
	randring_fill();
	items_init();
	replay_practice_items_ready();
#endif
	bomb_reset();
	sparks_init();
	hud_score_put();
#if (GAME == 4)
	thicklasers_reset();
#endif
	pointnums_init();
	nopcall_same_group(hud_put);
	bg_render_bombing_func = tiles_render_all;
	tiles_invalidate_reset();
}

// replay_practice_start_apply_and_stage_activate() is one byte shorter than
// the native assignments it replaces. Preserve stage_transition()'s offset.
#pragma codestring "\x90"

// Undefined again because this file is textually first in its translation
// unit, and the three files behind it are not this function's business.
#undef nopcall_same_group
