/// Stage 6 Boss - Shinki, custom bullet rendering
/// ----------------------------------------------
/// Shinki's [boss_custombullets_render] callback: her 32×32 ball bullets, in
/// their two visual states — the white cloud they grow out of, drawn with
/// master.lib's circle primitive, and the sprite they turn into — followed by
/// the same unrelated tail call to cheetos_render() that EX-Alice's callback
/// in th05/main/boss/bx_custombullets.cpp ends with.
///
/// The state half of this loop is th05/main/bullet/b6ball.cpp.
///
/// (Its own object, th05/b6cbull.cpp, listed ahead of th05/stages.cpp. This
/// module was the LAST thing th05_main.asm contributed to MIDBOSSX_TEXT, so
/// an object at the segment's next link position lands exactly where the
/// root's block ended and every byte above it keeps its address — no carve,
/// no new segment name, no group-list edit (kb/codegen 0112 + 0114). It is
/// NOT prepended to th05/stages.cpp, which is that next position today:
/// hoisting this file's five headers above th05/main/boss/b6_fg.cpp would put
/// declarations in scope earlier for code that is already matched, and that
/// alone has moved an unrelated function's switch dispatch before.)

#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/v_colors.hpp"
#include "th04/formats/super.h"
#include "th04/hardware/grcg.hpp"
#include "th04/main/custom.hpp"
#include "th05/main/bullet/b6ball.hpp"
#include "th05/main/bullet/cheeto.hpp"

void pascal near shinki_custombullets_render(void)
{
	#define left	_AX
	#define top 	_DX

	b6ball_t near *ball;
	int i;

	ball = b6balls;
	for(i = 1; (i < (1 + B6BALL_COUNT)); (i++, ball++)) {
		if(ball->flag == B6BF_FREE) {
			continue;
		}
		if(ball->flag == B6BF_CLOUD) {
			grcg_setcolor_direct(V_WHITE);
			grcg_circlefill(
				ball->pos.cur.to_screen_left(),
				ball->pos.cur.to_screen_top(),
				ball->cloud_radius.to_pixel_slow()
			);
		} else {
			_ES = SEG_PLANE_B;
			top = ball->pos.cur.y.to_pixel();
			left = ball->pos.cur.to_screen_left(B6BALL_W);

			// No bottom, left or right clip: the sprite is blitted with
			// master.lib's vertically wrapping tiny-format function, and
			// ZUN only rejects the coordinate it cannot wrap.
			if(static_cast<pixel_t>(top) < 0) {
				continue;
			}
			z_super_roll_put_tiny_32x32_raw(ball->patnum_tiny);
		}
	}
	cheetos_render();

	#undef top
	#undef left
}
