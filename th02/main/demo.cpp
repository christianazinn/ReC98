/* ReC98
 * -----
 * TH02's demo replay playback. stage_loop() calls this once per frame, in
 * place of reading the actual keyboard, whenever [resident->demo_num] is set.
 */

#pragma option -zCDEMO_TEXT -zPmain_01 -G

#include "platform.h"
#include "pc98.h"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/snd/snd.h"
// Pulls in th02/hardware/input.hpp, which has no include guard.
#include "th02/main/demo.h"
#include "th02/main/main.hpp"

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
