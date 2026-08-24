/* ReC98
 * -----
 * TH02's demo replays: loading one of the three recordings, and replaying it
 * one frame at a time in place of the actual keyboard.
 */

#pragma option -zCDEMO_TEXT -zPmain_01 -G

#include "platform.h"
#include "pc98.h"
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/resident.hpp"
#include "th02/snd/snd.h"
// Pulls in th02/hardware/input.hpp, which has no include guard.
#include "th02/main/demo.h"
#include "th02/main/main.hpp"
#include "th02/main/playperf.hpp"
#include "th02/main/player/player.hpp"
#include "th02/main/stage/stage.hpp"

// The recording filenames still live in th02_main.asm's own _DATA
// contribution, so they have to be referenced rather than re-emitted.
extern "C" const char demo_fn_1[];
extern "C" const char demo_fn_2[];
extern "C" const char demo_fn_3[];

// Allocates [DemoBuf] and reads the recording selected by
// [resident->demo_num] into it, along with the run's fixed starting state.
// main() calls this through a same-code-group `nopcall` alias.
void demo_load(void)
{
	DemoBuf = reinterpret_cast<input_t __seg *>(
		hmem_allocbyte(DEMO_N * sizeof(input_t))
	);
	power = POWER_MAX;
	playperf = 12;
	resident->frame = 18;
	if(resident->demo_num == 1) {
		stage_id = 3;
		file_ropen(demo_fn_2);
		resident->shottype = 0;
	} else if(resident->demo_num == 2) {
		stage_id = 2;
		file_ropen(demo_fn_2);
		resident->shottype = 2;
	} else if(resident->demo_num == 3) {
		stage_id = 1;
		file_ropen(demo_fn_3);
		resident->shottype = 1;
	}
	file_read(DemoBuf, (DEMO_N * sizeof(input_t)));
	file_close();
}

// Replays one frame of [DemoBuf] into [key_det], and ends the demo 50 frames
// before the buffer would run out.
// ZUN quirk: The per-frame input is only consumed if [key_det] happens to be
// 0 at this point. Since stage_loop() calls input_reset_sense() immediately
// before this, that is normally the case -- but any real keypress during a
// demo both skips a recorded frame and immediately ends the replay, which is
// what makes a demo quittable with any key rather than a specific one.
extern "C" void pascal DemoPlay(void)
{
	if(!key_det) {
		key_det = DemoBuf[demo_frame];
		demo_frame++;
		if(demo_frame < (DEMO_N - 50)) {
			return;
		}
	}
	key_det = 0;
	palette_black_out(10);
	quit = true;
	snd_se_reset();
}

#include "th02/main/randfill.cpp"
