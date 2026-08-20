/// Mai & Yuki's ball bullets: the renderer
/// ---------------------------------------
/// Blits every live ball. The state half of the same loop is still ZUN's
/// assembly; this is the only one of the four functions
/// th05/main/bullet/b4ball.hpp declares that has been decompiled.
///
/// The cel is picked in one place for two independent reasons — the 4-frame
/// idle animation and the damage flash — and only for balls whose base patnum
/// is below the flash cel set, i.e. the two ordinary colours. A ball already
/// showing a flash cel is blitted as it is.
///
/// (#included from th05/b6cbull.cpp, ahead of th05/main/boss/b4_fg.cpp. The
/// module this replaces was the last thing th05_main.asm contributed to
/// MIDBOSSX_TEXT, and this object is the segment's next contribution, so the
/// C++ side grows backwards into the hole and every byte above it keeps its
/// address. kb/codegen 0112 + 0114.)

#include "th04/formats/super.h"
#include "th04/main/custom.hpp"
#include "th05/main/bullet/b4ball.hpp"
#include "th05/sprites/main_pat.h"

// `extern "C"` + `pascal`, because the module published the undecorated
// upper-case `B4BALLS_RENDER` and th05_main.asm takes its address at two
// separate boss setup sites. Plain C++ linkage would emit a mangled name and
// leave both `offset` sites unresolved (kb/codegen 0081 + 0102); b4ball.hpp's
// declaration is corrected to match.
extern "C" void pascal near b4balls_render(void)
{
	#define left	_AX
	#define top 	_DX

	b4ball_t near *ball;
	int patnum;
	int i;

	_ES = SEG_PLANE_B;
	ball = b4balls;
	for(i = 1; (i < (1 + B4BALL_COUNT)); (i++, ball++)) {
		if(ball->flag == 0) {
			continue;
		}
		patnum = ball->patnum_tiny_base;
		if(patnum < PAT_B4BALL_SNOW_HIT) {
			patnum += (ball->age & (B4BALL_CELS - 1));
			if(ball->damaged_this_frame) {
				patnum += (PAT_B4BALL_SNOW_HIT - PAT_B4BALL_SNOW);
			}
			ball->damaged_this_frame = 0;
		}
		top = scroll_subpixel_y_to_vram_seg1(ball->pos.cur.y);
		left = ball->pos.cur.to_screen_left(B4BALL_W);
		z_super_roll_put_tiny_32x32_raw(patnum);
	}

	#undef top
	#undef left
}
