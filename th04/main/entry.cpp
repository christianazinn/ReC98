/// MAIN.EXE entry point
/// --------------------
/// Brings up every subsystem the gameplay portion of the game needs, then runs
/// one stage_loop() per stage for as long as [quit] asks for another one, and
/// finally hands the process over to OP.EXE.
///
/// Called main_entry() rather than main() because a C++ function literally
/// named `main` would come with a code segment of its own, shifting every
/// address in this group; th0N_main.asm exports the real `_main` as a TASM
/// alias for this function instead. (kb/codegen/0040)
///
/// ONE body for both games. TH05 has no gaiji and reserves less conventional
/// memory; nothing else differs.

#pragma option -zCSTAGE_TEXT -zPmain_01

#include "platform.h"
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/main/execl.hpp"
#include "th03/core/initexit.h"
#include "th04/main/quit.hpp"
#include "th04/snd/snd.h"
#if (GAME == 5)
	#include "th05/resident.hpp"
#else
	#include "th04/resident.hpp"
#endif

// String literals, still owned by the dump's data segment.
// ---------------------------------------------------------------------
#if (GAME == 5)
	extern "C" const unsigned char aKAIKIDAN2_DAT[];
	#define MAIN_PF_FN aKAIKIDAN2_DAT
#else
	extern "C" const unsigned char aUmx[];
	#define MAIN_PF_FN aUmx

	extern "C" const char GAIJI_FN[];
#endif

extern "C" const char aMiko[];

// The binary this process replaces itself with once gameplay is over.
extern "C" const char arg0[];
// ---------------------------------------------------------------------

// Declared here rather than through th03/formats/cfg.hpp and
// th04/main/ems.hpp: the former #undef's the CFG_FN that its own implementation
// header re-#define's, and the latter pulls in most of the main() include tree
// for two symbols we don't need. Same reason as th04/main/execl.cpp.
// ---------------------------------------------------------------------
resident_t __seg* near cfg_load_resident_ptr(void);

// Truncated to 32 characters by Turbo C++, hence the dump's
// `@ems_allocate_and_preload_eyecatc$qv`.
void near ems_allocate_and_preload_eyecatch(void);

void near stage_loop(void);

// Loads all assets and initializes the selected stage.
void near stage_setup(void);

// The handoff between two stages. Was the other half of the block above until
// it became th04/main/stage/transition.cpp, one shared body for both games,
// which is where the name this macro used to stand in for came from. Declared
// rather than reached through a header because the function has exactly one
// caller, right below, and heads no subsystem of its own.
void near stage_transition(void);
// ---------------------------------------------------------------------

extern "C" void far main_entry(void)
{
	if(!cfg_load_resident_ptr()) {
		return;
	}

	#if (GAME == 5)
		mem_assign_paras = (291200 >> 4);
	#else
		// ZUN landmine: This is roughly 3.8 KB below what this game would need
		// when running without an EMS driver, and thus causes the infamous
		// crash after Reimu's Stage 5 pre-battle dialog.
		// https://rec98.nmlgc.net/blog/2021-11-29 documents this issue in full
		// detail.
		mem_assign_paras = (320000 >> 4);
	#endif

	game_init_main(MAIN_PF_FN);
	random_seed = resident->rand;
	ems_allocate_and_preload_eyecatch();
	text_clear();
	#if (GAME != 5)
		gaiji_backup();
		gaiji_entry_bfnt(GAIJI_FN);
	#endif
	snd_determine_modes(resident->bgm_mode, resident->se_mode);
	snd_load(aMiko, SND_LOAD_SE);

	while(1) {
		stage_setup();
		stage_loop();
		if(quit != Q_NEXT_STAGE) {
			break;
		}
		stage_transition();
	}

	// A `nopcall` in both dumps, so it has to be hand-spelled.
	// (kb/codegen/0083)
	_asm {
		push	ds;
		push	offset arg0;
		nop;
		push	cs;
		call	near ptr GameExecl;
	}
}
