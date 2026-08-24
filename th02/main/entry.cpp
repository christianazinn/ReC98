/// MAIN.EXE entry point
/// --------------------
/// Loads the configuration, brings up the sound driver and every gameplay
/// subsystem, then runs one stage_init() + stage_loop() pair per stage for as
/// long as stage_loop() asks for another one, and finally hands the process
/// back to OP.EXE.
///
/// Called main_entry() rather than main() because a C++ function literally
/// named `main` would come with a code segment of its own, shifting every
/// address in this group; th02_main.asm exports the real `_main` as a TASM
/// alias for this function instead. (kb/codegen/0040)

// The original's prolog is a plain `push bp; mov bp, sp` with no locals,
// which is -G. (kb/codegen/0011)
#pragma option -zCmain_01_TEXT -zPmain_01 -G

#include "platform.h"
#include "pc98.h"
#include "shiftjis.hpp"
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/core/initexit.h"
#include "th02/core/zunerror.h"
// Brings in th02/hardware/input.hpp, which has no include guard and must
// therefore not be included directly alongside this one.
#include "th02/main/demo.h"
#include "th02/main/hud/overlay.hpp"
#include "th02/main/stage/stage.hpp"
#include "th02/resident.hpp"
#include "th02/snd/snd.h"

// Declared here rather than through th02/formats/cfg.hpp: that header declares
// the __cdecl far cfg_load() compiled into MAINE.EXE, while MAIN.EXE compiles
// this near C-linkage variant from th02/main/cfg_load.cpp. Before that lift,
// the root dump exposed the same C symbol through a zero-byte alias
// (`e3ef0149:th02_main.asm:488`).
// ---------------------------------------------------------------------
extern "C" int near cfg_load(void);

// th02/main/stage/init.cpp
void near gameplay_init(void);
void near stage_init(void);
void near continue_resume(void);

// The [farfp_1F4A4] slot, which gameplay_init() points at stage_loop().
extern "C" bool16 (far *stage_loop_func)(void);

// The post-stage continue prompt. Returns whether the player asked to
// continue; ZUN tests the result with `or ax, ax`, hence [bool16] rather than
// [bool] (kb/codegen/0090). th02/main/continue.cpp.
bool16 near continue_prompt(void);

// Set by stage_init(); still owned by the dump's data segment.
extern "C" uint8_t stage1_gaiji_halflen;
extern "C" shiftjis_t near *stage_title;
extern "C" uint8_t stage_title_halflen;

// Gaiji string literals and the binary this process replaces itself with once
// gameplay is over, all still owned by the dump's data segment and reached
// through zero-byte `label` aliases. (kb/codegen/0123)
extern "C" const char gStage1[];
extern "C" const char gEXTRA_STAGE[];
extern "C" const char gDEMO_PLAY[];
extern "C" const char arg0[];

// A `nopcall` in the dump, so it has to be hand-spelled. (kb/codegen/0014)
int GameExecl(const char *binary_fn);
// ---------------------------------------------------------------------

// Turbo C++ compiled ZUN's far calls to same-code-group functions as
// `nop; push cs; call near ptr`. (kb/codegen/0014)
#define nopcall_same_group(func) _asm { \
	nop; \
	push	cs; \
	call	near ptr func; \
}

// The stage title is right-aligned against this TRAM column.
static const int TITLE_RIGHT = 0x1C;

extern "C" int far main_entry(void)
{
	if(!cfg_load()) {
		return 1;
	}
	if(game_init_main()) {
		zun_error(ERROR_OUT_OF_MEMORY);
		return 1;
	}

	if(resident->bgm_mode == SND_BGM_FM) {
		snd_pmd_resident();
		snd_midi_active = false;
		snd_determine_mode();
	} else if(resident->bgm_mode == SND_BGM_MIDI) {
		snd_pmd_resident();
		snd_mmd_resident();
		snd_midi_active = snd_midi_possible;
		snd_determine_mode();
	}

	if(resident->demo_num) {
		nopcall_same_group(demo_load);
	}
	gameplay_init();

stage:
	random_seed = resident->frame;
	stage_init();
	nopcall_same_group(overlay_stage_enter_animate);

	if(!resident->demo_num) {
		if(stage_id == 5) {
			gaiji_putsa(16, 12, gEXTRA_STAGE, TX_YELLOW);
		} else {
			gaiji_putsa(
				(TITLE_RIGHT - stage1_gaiji_halflen), 12, gStage1, TX_YELLOW
			);
		}
		text_putsa(
			(TITLE_RIGHT - stage_title_halflen), 13, stage_title, TX_WHITE
		);
	} else {
		gaiji_putsa(18, 12, gDEMO_PLAY, (TX_YELLOW + TX_BLINK));
	}
	key_det = 0;

frame:
	if(stage_loop_func()) {
		// `snd_kaja_func(KAJA_SONG_FADE, 40);` is the expression, but Turbo
		// C++ cleans this __cdecl call's single stack word with `add sp, 2`
		// *here*, while the original uses `pop cx` — as does every other
		// snd_kaja_*() site in this binary, including the byte-identical one
		// th02/main/dialog/dialog.cpp compiles from the same macro. The
		// difference is this call site's context, not the call, so the three
		// instructions are pinned by hand. (kb/codegen/0083)
		// 0228h == ((KAJA_SONG_FADE << 8) | 40).
		_asm {
			push	0228h;
			call	far ptr snd_kaja_interrupt;
			pop 	cx;
		}
		goto stage;
	}

	if(!resident->demo_num) {
		if(continue_prompt()) {
			continue_resume();
			goto frame;
		}
		resident->stage = stage_id;
	} else {
		hmem_free(reinterpret_cast<void __seg *>(DemoBuf));
	}

	palette_settone(0);
	_asm {
		push	ds;
		push	offset arg0;
		nop;
		push	cs;
		call	near ptr GameExecl;
		add 	sp, 4;
	}
	return 0;
}
