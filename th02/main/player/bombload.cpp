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
#include "th02/resident.hpp"
#include "th02/formats/pi.h"
#include "th02/snd/snd.h"
#include "th02/main/playfld.hpp"
#include "th02/main/playperf.hpp"
#include "th02/main/bullet/bullet.hpp"
#include "th02/main/hud/hud.hpp"
#include "th02/main/player/player.hpp"
#include "th02/main/player/bomb.hpp"

// The filenames still live in th02_main.asm's own _DATA contribution, so they
// have to be referenced rather than re-emitted. (kb/codegen/0084)
extern "C" const char aBombs_bft[];
extern "C" const char aBomb1_pi[];
extern "C" const char aBomb3_pi[];
extern "C" const char aBomb2_pi[];
extern "C" const char aBomb1_bft[];

// The shottype-specific bomb animations, all still ASM in th02_main.asm, which
// publishes them with __pascal *and* `extern "C"` name decoration
// (`public BOMB_REIMU_A`, not `@BOMB_REIMU_A$QV`). See kb/codegen/0086 and
// kb/codegen/0103. Their actual return type is [playchar_bomb_func]'s bool16;
// the reinterpret_cast below is only needed because th02_main.asm gives us no
// prototype to confirm that with.
extern "C" void pascal near bomb_reimu_a(void);
extern "C" void pascal near bomb_reimu_b(void);
extern "C" void pascal near bomb_reimu_c(void);

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
		playchar_bomb_func = reinterpret_cast<playchar_bomb_func_t>(
			bomb_reimu_a
		);
		return;
	}
	if(resident->shottype == 2) {
		pi_load(1, aBomb3_pi);
		playchar_bomb_func = reinterpret_cast<playchar_bomb_func_t>(
			bomb_reimu_c
		);
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
		playchar_bomb_func = reinterpret_cast<playchar_bomb_func_t>(
			bomb_reimu_b
		);
	}
}

void near bomb_free(void)
{
	pi_free(1);
	if(resident->shottype == 1) {
		hmem_free(reinterpret_cast<void __seg *>(bomb1_bft));
	}
}

void near bombs_reset(void)
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
