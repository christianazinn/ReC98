/// Per-stage gameplay loop
/// -----------------------
/// TH04 and TH05's per-frame gameplay loop. main() calls it once per stage,
/// and it runs until [quit] leaves Q_KEEP_RUNNING; main() then inspects that
/// same variable to decide between the next stage and a return to OP.EXE.
///
/// The direct descendant of TH02's stage_loop() (th02/main/stage/loop.cpp),
/// which is where the name comes from. Unlike TH02's, this one is `near`,
/// returns nothing, and does not own the scrolling or the VSync wait.
///
/// ONE body for both games. The `#if (GAME == 5)` sites below are the
/// complete list of per-frame differences between Lotus Land Story and
/// Mystic Square.

#pragma option -zCSTAGE_TEXT -zPmain_01

#include "platform.h"
#include "pc98.h"
#include "x86real.h"
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/hardware/frmdelay.h"
#include "th02/hardware/pages.hpp"
#include "th03/hardware/palette.hpp"
#include "th04/hardware/grcg.hpp"
#if (GAME == 5)
	#include "th05/hardware/input.h"
#else
	#include "th04/hardware/input.h"
#endif
#include "th04/main/bg.hpp"
#include "th04/main/circle.hpp"
#include "th04/main/frames.h"
#include "th04/main/gather.hpp"
#include "th04/main/pause.h"
#include "th04/main/playfld.hpp"
#include "th04/main/playperf.hpp"
#include "th04/main/quit.hpp"
#include "th04/main/replay.hpp"
#include "th04/main/score.hpp"
#include "th04/main/slowdown.hpp"
#include "th04/main/stage/stage.hpp"
#include "th04/snd/snd.h"
#if (GAME == 5)
	#include "th05/resident.hpp"
#else
	#include "th04/resident.hpp"
#endif

// Everything below would come from th04/formats/std.hpp,
// th04/main/dialog/dialog.hpp, th04/main/{boss,enemy,midboss}/*.hpp,
// th04/main/hud/overlay.hpp and th04/main/player/*.hpp, but several of those
// pull in th04/main/bullet/bullet.hpp or th04/main/item/item.hpp a second
// time, and neither of those is include-guarded. Declared here instead, the
// same way th02/main/stage/loop.cpp declares its own callback slots.
// ---------------------------------------------------------------------

// Per-frame demo playback hook. Only ever nullfunc_near() or DemoPlay(); the
// stage setup code installs the latter whenever [resident->demo_num] is
// nonzero. [static]
extern nearfunc_t_near demo_update;

// Runs one frame of the current stage's .STD script.
extern func_t_near stage_vm;

// Set to the pre-boss frame function while the stage is running.
extern bool (near* std_update)(void);

// Rendered on top of everything else, in this order.
extern nearfunc_t_near overlay1;
extern nearfunc_t_near overlay2;

extern func_t_near midboss_update;
extern nearfunc_t_near midboss_render;
extern func_t_near boss_update;
extern nearfunc_t_near boss_fg_render;

extern bool bombing;
extern bool player_is_hit;

// The unguarded spark and point-number headers declare the four functions
// below, but cannot be included here for the collision described above.
// pointnums_render() also still lacks the `extern "C"` required by its dump
// export; correcting that unrelated declaration remains a naming parcel.
extern "C" void near sparks_update(void);
extern "C" void near sparks_render(void);
extern "C" void pascal near pointnums_update(void);
extern "C" void pascal near pointnums_render(void);

extern "C" void pascal enemies_update(void);
extern "C" void pascal items_update(void);
extern "C" void pascal near enemies_render(void);
extern "C" void pascal near items_render(void);
extern "C" void pascal near player_render(void);
void bullets_update(void);

// `pascal`, not because the no-argument call needs it, but because both dumps
// export this one with the all-uppercase linker spelling @SHOTS_RENDER$QV,
// which is how Turbo C++ spells a __pascal C++ function. (kb/codegen/0081)
void pascal near shots_render(void);

// `pascal` for the same reason: while the dumps still owned this function they
// exported it under its all-uppercase, undecorated spelling. BOTH games' copies
// are C++ functions in th04/main/bullet/render.cpp as of this branch and
// neither dump exports the name any more, so the two keywords now only have to
// agree with that definition — which they do, and which nothing in the compiler
// checks. A parameterless call is byte-identical under either convention
// anyway (kb/codegen/0081, and the "one symbol, one declaration" note in
// kb/conventions/agent-working-discipline.md).
extern "C" void pascal near bullets_render(void);
void pascal near tiles_render_all(void);
// ---------------------------------------------------------------------

#if (GAME == 5)
	extern nearfunc_t_near playchar_bomb_func;
	extern nearfunc_t_near boss_custombullets_render;

	void near lasers_update(void);
	void near lasers_render(void);

	extern "C" void near sub_1214A(void);
	extern "C" void near sub_1240B(void);

	// Set from [resident_t.debug_mode] once, at stage setup time, and never
	// reset. Gates the Q key toggle below. [static]
	//
	// NOT named [debug_mode]: that name already means something else in three
	// places, one of which is two instructions away from this variable's only
	// writer, where th05_main.asm reads
	// 	mov es:[bx+resident_t.debug_mode], 0
	// 	mov _debug_mode_active, 1
	// and would otherwise read as `debug_mode = 0; debug_mode = 1`. The other
	// two are th04/th04.inc's and th05/th05.inc's own resident_t field, and
	// TH01's 0-3 command-line selector in th01/op_01.cpp.
	extern bool debug_mode_active;

	// Debug fast-forward, toggled with the Q key: one press turns it on and
	// the next one turns it off, with the two intermediate states below
	// tracking the key's press/release edge. Any state other than
	// DEBUG_FF_OFF skips the frame delay and keeps [player_is_hit] clear.
	//
	// IDA renders the two high values as -2 and -1, but nothing ever compares
	// this variable with a signed operator, and an [int8_t] would widen every
	// one of the equality tests below through CBW (kb/codegen/0029).
	// [static]
	extern unsigned char debug_fast_forward;

	#define DEBUG_FF_OFF          0x00
	#define DEBUG_FF_TURNING_ON   0xFE
	#define DEBUG_FF_ON           0xFF
	#define DEBUG_FF_TURNING_OFF  0x01
#else
	// Truncated to 32 characters by Turbo C++, hence the dump's
	// `@midboss_activate_if_stage_frame_$qv`.
	void midboss_activate_if_stage_frame_is_midboss_start_frame(void);

	void near bomb_update_and_render(void);

	extern "C" void near sub_10ABF(void);

	// Not from th04/main/player/shot.hpp, for the reason the block above
	// gives: that header reaches th04/main/player/player.hpp, and this file
	// declares rather than includes. TH05's counterpart in this slot is the
	// still-unnamed sub_1240B() above.
	void near shots_update(void);

	// # of frames between two playperf_raise() calls at the lowest and the
	// highest life count. TH05 uses a constant interval instead.
	#define PLAYPERF_INTERVAL_MIN 1000
	#define PLAYPERF_INTERVAL_BASE 6000
	#define PLAYPERF_INTERVAL_PER_LIFE 500
	#define PLAYPERF_LIVES_MAX 10
#endif

// master.lib's GRCG_OFF_CLOBBERING macro, which spills the port number to DX
// instead of using the immediate-port form that _outportb_() would emit.
#define grcg_off_clobbering_dx() outportb(0x7C, GC_OFF)

void near stage_loop(void)
{
	#if (GAME == 4)
		register int playperf_interval;
	#endif

	replay_stage_start();
	slowdown_factor = 1;
	if(replay_practice_preroll_active()) {
		_asm { mov ah, 41h; int 18h; }
	}
	frame_delay(1);
	input_reset_sense();

	do {
		if(replay_practice_preroll_boundary()) {
			if(replay_practice_direct_redraw_take()) {
				graph_accesspage(page_front);
				tiles_render_all();
				graph_accesspage(page_back);
			}
			_asm { mov ah, 40h; int 18h; }
			if(quit != Q_KEEP_RUNNING) {
				break;
			}
		}
		input_sense();
		demo_update();
		replay_gameplay_input();
		if(quit != Q_KEEP_RUNNING) {
			break;
		}
		if(key_det & INPUT_CANCEL) {
			if(pause()) {
				quit = Q_QUIT_TO_OP;
			}
		}
		std_update();
		#if (GAME == 4)
			midboss_activate_if_stage_frame_is_midboss_start_frame();
		#endif
		stage_vm();
		if(!bombing) {
			bg_render_not_bombing();
		} else {
			bg_render_bombing();
		}
		pointnums_update();
		circles_update();
		sparks_update();
		#if (GAME == 5)
			sub_1214A();
			sub_1240B();
			lasers_update();
		#else
			sub_10ABF();
			shots_update();
		#endif
		bullets_update();
		enemies_update();
		midboss_update();
		boss_update();
		items_update();
		gather_update();
		stage_render();
		#if (GAME == 5)
			if(bombing) {
				playchar_bomb_func();
			}
		#else
			bomb_update_and_render();
		#endif
		boss_fg_render();
		midboss_render();
		enemies_render();
		shots_render();
		player_render();
		grcg_setmode_rmw();
		#if (GAME == 5)
			boss_custombullets_render();
			lasers_render();
		#endif
		gather_render();
		sparks_render();
		items_render();
		pointnums_render();
		bullets_render();
		circles_render();
		grcg_off_clobbering_dx();
		overlay1();
		overlay2();
		playfield_shake_update_and_render();
		replay_input_reset_sense_tail();
		if(replay_frame_pacing_should_delay()) {
			slowdown_frame_delay();
		}

		if(palette_changed) {
			palette_show();
			palette_changed = false;
			__emit__(0xEB, 0x00); // JMP SHORT $+2
		}
		scroll_update_and_render();

		graph_accesspage(page_front);
		graph_showpage(page_back);
		page_front = _AL;
		page_back ^= 1;

		snd_se_update();
		if(replay_stage_frame_advance_should_raise()) {
			__emit__(0x6A, 1); // push 1
			_asm { nop; push cs; call near ptr playperf_raise; }
			__emit__(0xEB, 0x00); // JMP SHORT $+2
		}
		score_update_and_render();
	} while(quit == Q_KEEP_RUNNING);
	if(replay_practice_preroll_active()) {
		_asm { mov ah, 40h; int 18h; }
	}
}

// Keep the following stock [main_01] code at its original raw offsets.
#if (GAME == 4)
	// The direct-seek redraw consumes the previous 26-byte offset reserve.
#else
	#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
	#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
	#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
	#pragma codestring "\x90\x90\x90\x90\x90\x90\x90"
#endif
