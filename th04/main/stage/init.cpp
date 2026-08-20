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
/// TH05's twin is `sub_B55A`: the same body in the same order, minus
/// [dream_items_collected], shot_reset() and the four write-only words below,
/// and with a [shot_time] reset where TH04 has none. It is deliberately NOT
/// shared -- it sits mid-segment in th05_main.asm and would need a
/// kb/codegen/0080 carve first, and this file is compiled into TH04's MAIN.EXE
/// alone, so a `#if (GAME == 5)` arm here would never be graded by the oracle.
/// Same rule, and same reason, that th04/main/stage/reset.cpp states.
///
/// (#included from th04/demo.cpp, AHEAD of th04/main/stage/transition.cpp,
/// because this function is now the last thing th04_main.asm contributes to
/// DEMO_TEXT. Growing that object backwards once more puts the body back at
/// its original address and moves nothing else (kb/codegen 0099 + 0114); no
/// carve, no new segment name, no Tupfile.lua line -- the third lift this one
/// wrapper has absorbed on those terms, after pause() and stage_transition().)
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
extern unsigned char stage_point_items_collected;
extern unsigned char dream_items_collected;
extern bool (near* std_update)(void);
extern nearfunc_t_near bg_render_bombing_func;

bool near std_update_frames_then_animate_dialog_and_activate_boss_if_done(void);
void near stage_state_reset(void);
void near shot_reset(void);
void near bomb_reset(void);
void near tiles_invalidate_reset(void);
void pascal near tiles_render_all(void);
// th04/main/pointnum/inv_upd.asm publishes this one as `public POINTNUMS_INIT`
// -- upper case, i.e. `extern "C"` + `pascal`, not the C++-mangled name that
// th04/main/pointnum/pointnum.hpp declared for it. That declaration had never
// been compiled against, because this is its first C++ caller; the header is
// corrected in the same change.
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

// Four words that this function is the ONLY writer of, and that nothing in
// either TH04 dump or anywhere in the tree ever reads. `[measured]` -- the
// four assignments below and the `dw ?` definitions in th04_main.asm's _BSS
// are their complete reference set.
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
// ---------------------------------------------------------------------

// The frames during which a miss locks the player out of moving.
// th04/main/player/bomb.cpp, which reads it, spells the same two lines and is
// where this name comes from; th04_main.asm still names the variable itself
// after its address, and publishes a zero-byte alias for both callers.
extern "C" unsigned char byte_259A3;
#define miss_move_lock_time byte_259A3

// Two far functions that this body was the only ASM caller of in the whole
// dump, reached through the zero-byte aliases th04_main.asm now publishes
// beside them (kb/codegen/0123). Neither body was read for this parcel and
// neither is named here, because a call site is not evidence of what a
// function does; both keep the spelling the dump gives them.
extern "C" void far sub_1DA1B(void);
extern "C" void far sub_15D74(void);

// Derives [shot_level] from [power], installs [playchar_shot_func] and
// tail-calls hud_power_put(); still ASM, and still deliberately unnamed. The
// failed search is recorded in state/notes/th04_continue_prompt.md, and
// th04/main/continue.cpp spells this same declaration for the same reason.
extern "C" void near sub_11DE6(void);

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
	stage_init_unused_0 = 0x40;
	stage_init_unused_1 = 0x40;
	stage_init_unused_2 = 0x30;
	stage_init_unused_3 = 0x30;
	miss_move_lock_time = 0;
	miss_time = 0;
	player_is_hit = false;
	player_invincibility_time = STAGE_START_INVINCIBILITY_FRAMES;
	stage_point_items_collected = 0;
	dream_items_collected = 0;
	std_update = std_update_frames_then_animate_dialog_and_activate_boss_if_done;
	scroll_active = true;
	shot_reset();
	nopcall_same_group(sub_11DE6);
	randring_fill();
	sub_1DA1B();
	bomb_reset();
	sparks_init();
	hud_score_put();
	sub_15D74();
	pointnums_init();
	nopcall_same_group(hud_put);
	bg_render_bombing_func = tiles_render_all;
	tiles_invalidate_reset();
}

// Undefined again because this file is textually first in its translation
// unit, and the three files behind it are not this function's business.
#undef miss_move_lock_time
#undef nopcall_same_group
