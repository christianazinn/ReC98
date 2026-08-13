#pragma option -zCSHARED

#include "libs/master.lib/master.hpp"
#include "th02/hardware/frmdelay.h"
#if (GAME == 3) && defined(TH03_PIXEL_CAPTURE)
#include "th03/pixel_capture.hpp"
#endif

void pascal frame_delay_2(int frames)
{
#if (GAME == 3) && defined(TH03_PIXEL_CAPTURE)
	t3pix_publish();
#endif
	vsync_Count1 = 0;
	while(vsync_Count1 < frames) {}
}
