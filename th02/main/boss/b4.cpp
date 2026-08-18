/// Stage 4 Boss - Marisa
/// ---------------------
/// Only the stage-end callback so far. It is the last body of the nameless
/// code segment that also holds Marisa's other three callbacks and the Stage 5
/// boss, so it needs no split and no new segment - this translation unit just
/// contributes to that same segment after th02_main.asm's block.
/// (kb/codegen/0099)

// -G, because the original's prolog is `push bp; mov bp, sp` with no locals
// rather than an `ENTER`. (kb/codegen/0011)
#pragma option -zCmain_03__TEXT -zPmain_03 -G

#include "platform.h"
#include "pc98.h"
#include "th02/main/stage/stage.hpp"
#include "th02/main/stage/bonus.hpp"
#include "th02/main/dialog/dialog.hpp"
#include "th02/main/hud/overlay.hpp"

// th02/main/dialog/dialog.cpp. dialog.hpp declares every dialog_script_*
// function but not this one.
void near dialog_pre(void);

// Runs Marisa's post-battle dialog and the stage clear bonus, then advances to
// the Extra-Stage-eligible Stage 5. Installed into [boss_end] by stage_init().
extern "C" void far marisa_end(void)
{
	dialog_pre();
	dialog_script_stage4_post_animate();
	stage_clear_bonus_animate();
	overlay_stage_leave_animate();
	stage_id++;
}
