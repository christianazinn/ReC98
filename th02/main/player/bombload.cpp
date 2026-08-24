/* ReC98
 * -----
 * TH02's bomb lifecycle, as opposed to th02/main/player/bomb.cpp's rendering:
 * loading and freeing the bomb graphics for the current shottype, resetting
 * the per-stage bomb counter, and the bomb trigger itself.
 */

// The original's prologs are all a plain `push bp; mov bp, sp` with no locals,
// which is -G. -G- would emit `ENTER 0, 0` even with no frame to set up.
// (kb/codegen/0011)
// -zPmain_01 keeps every code reference framed on the group rather than on the
// segment. (kb/codegen/0104)
#pragma option -zCBOMB_TEXT -zPmain_01 -G

#include "platform.h"
#include "pc98.h"
#include "libs/master.lib/master.hpp"
#include "th01/math/polar.hpp"
#include "th02/resident.hpp"
#include "th02/formats/pi.h"
#include "th02/snd/snd.h"
#include "th02/main/playfld.hpp"
#include "th02/main/playperf.hpp"
#include "th02/main/bullet/bullet.hpp"
#include "th02/main/hud/hud.hpp"
#include "th02/main/tile/tile.hpp"
#include "th02/main/player/player.hpp"
#include "th02/main/player/bomb.hpp"

// The filenames still live in th02_main.asm's own _DATA contribution, so they
// have to be referenced rather than re-emitted. (kb/codegen/0084)
extern "C" const char aBombs_bft[];
extern "C" const char aBomb1_pi[];
extern "C" const char aBomb3_pi[];
extern "C" const char aBomb2_pi[];
extern "C" const char aBomb1_bft[];

// The shottype-specific bomb animations, defined in bomb.cpp. All three return
// true once their animation is over, matching [playchar_bomb_func].
extern "C" bool16 pascal near bomb_reimu_a(void);
extern "C" bool16 pascal near bomb_reimu_b(void);
extern "C" bool16 pascal near bomb_reimu_c(void);

// 15 cels of 72 bytes, read past a 32-byte header.
static const unsigned BOMB_BFT_SIZE = 1080;

// Read past a 80-byte header. Only allocated for shottype 1.
static const unsigned BOMB1_BFT_SIZE = 5184;

extern uint8_t *bomb1_bft;
extern uint8_t *bomb_bft;

void near bomb_load(void)
{
	file_ropen(aBombs_bft);
	file_seek(0x20, SEEK_SET);
	bomb_bft = reinterpret_cast<uint8_t __seg *>(
		hmem_allocbyte(BOMB_BFT_SIZE)
	);
	file_read(bomb_bft, BOMB_BFT_SIZE);
	file_close();
	if(resident->shottype == 0) {
		pi_load(1, aBomb1_pi);
		playchar_bomb_func = bomb_reimu_a;
		return;
	}
	if(resident->shottype == 2) {
		pi_load(1, aBomb3_pi);
		playchar_bomb_func = bomb_reimu_c;
		return;
	}
	if(resident->shottype == 1) {
		pi_load(1, aBomb2_pi);
		file_ropen(aBomb1_bft);
		file_seek(0x50, SEEK_SET);
		bomb1_bft = reinterpret_cast<uint8_t __seg *>(
			hmem_allocbyte(BOMB1_BFT_SIZE)
		);
		file_read(bomb1_bft, BOMB1_BFT_SIZE);
		file_close();
		playchar_bomb_func = bomb_reimu_b;
	}
}

void near bomb_free(void)
{
	pi_free(1);
	if(resident->shottype == 1) {
		hmem_free(reinterpret_cast<void __seg *>(bomb1_bft));
	}
}

void near bomb_reset(void)
{
	stage_bombs_used = 0;
	bombing = false;
}

extern "C" void pascal near player_bomb(void)
{
	if(bombing || (bombs == 0)) {
		return;
	}
	bombing = true;
	player_invincible_via_bomb = true;
	bombs--;
	hud_bombs_put();
	bomb_frame = 0;
	stage_bombs_used++;
	total_bombs_used++;
	snd_se_play(9);
	bomb_circle_center.x = (PLAYFIELD_LEFT + (PLAYFIELD_W / 2) - 4);
	bomb_circle_center.y = (PLAYFIELD_TOP + (PLAYFIELD_H / 2) - 4);
	bomb_circle_frame = 0;
	bomb_circle_done = false;
	bullets_clear();
}

// The number of points sampled around the bomb circle, and the angle step that
// spreads them over the full turn.
static const int BOMB_CIRCLE_POINTS = 64;
static const unsigned char BOMB_CIRCLE_ANGLE_STEP = 4;

// Marks the tiles under the expanding bomb circle for redrawing, and forces a
// full tile redraw on the two frames where the shottype's own bomb animation
// changes the size of what it covers. stage_loop() calls this once per frame.
void near bomb_invalidate(void)
{
	int radius;
	screen_x_t left;
	unsigned char angle;
	screen_y_t top;
	int i;

	if(!bombing) {
		return;
	}
	if(bomb_circle_frame > 1) {
		// ZUN quirk: Decremented here and incremented again at the end of the
		// branch, so that both readers inside it see one frame less than the
		// rest of the game does.
		//
		// • The ring is sampled one radius step behind what
		//   bomb_circle_update_and_render() actually drew: [bomb_frame] is only
		//   incremented there, and stage_loop() runs that function *after* this
		//   one, so the undecremented value would already be the correct one.
		// • The [BOMB_CIRCLE_FRAMES] test below therefore zeroes
		//   [bomb_circle_frame] one frame later than it otherwise would, which
		//   delays the end of the circle phase in bomb_update_and_render() by
		//   that same frame.
		//
		// The second effect is what makes this a quirk rather than a rendering
		// bug: it moves the length of the bomb, hence the frame on which
		// [bombing] clears and [player_invincibility_time] is set, so fixing it
		// would desync a replay — the middle column of CONTRIBUTING.md's
		// summary table. [inferred, static evidence only]
		bomb_frame--;

		radius = (256 - (bomb_frame * 8));
		for(i = 0, angle = bomb_frame; i < BOMB_CIRCLE_POINTS; i++,
			angle = (angle + BOMB_CIRCLE_ANGLE_STEP)
		) {
			left = polar_x_fast(bomb_circle_center.x, radius, angle);
			top  = polar_y_fast(bomb_circle_center.y, radius, angle);
			if(top <= 8) {
				continue;
			}
			if(top >= PLAYFIELD_BOTTOM) {
				continue;
			}
			tiles_invalidate_rect(left, top, 8, 8);
		}
		if(bomb_frame >= BOMB_CIRCLE_FRAMES) {
			bomb_circle_frame = 0;
		}
		bomb_frame++;
	}
	if((bomb_circle_done == true) && (bomb_frame <= BOMB_CIRCLE_FRAMES)) {
		tiles_egc_render_all = true;
	}
	if(resident->shottype == 0) {
		if((bomb_frame == 136) || (bomb_frame == 137)) {
			tiles_egc_render_all = true;
		}
	} else if(resident->shottype == 1) {
		if((bomb_frame == 164) || (bomb_frame == 165)) {
			tiles_egc_render_all = true;
		}
	} else {
		if((bomb_frame == 112) || (bomb_frame == 113)) {
			tiles_egc_render_all = true;
		}
	}
}
