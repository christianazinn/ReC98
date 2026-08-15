/* ReC98
 * -----
 * TH02's per-stage gameplay loop, installed as the [farfp_1F4A4] callback and
 * called once per stage from main().
 */

#pragma option -zCSTAGE_TEXT -zPmain_01 -G

#include "platform.h"
#include "pc98.h"
#include "decomp.hpp"
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "platform/x86real/pc98/page.hpp"
#include "th02/resident.hpp"
#include "th02/hardware/input.hpp"
#include "th02/hardware/pages.hpp"
#include "th02/snd/snd.h"
#include "th02/main/frames.hpp"
#include "th02/main/main.hpp"
#include "th02/main/playperf.hpp"
#include "th02/main/score.hpp"
#include "th02/main/scroll.hpp"
#include "th02/main/slowdown.hpp"
#include "th02/main/spark.hpp"
#include "th02/main/bullet/bullet.hpp"
#include "th02/main/hud/menu.hpp"
#include "th02/main/item/item.hpp"
#include "th02/main/midboss/midboss.hpp"
#include "th02/main/player/bomb.hpp"
#include "th02/main/player/player.hpp"
#include "th02/main/stage/stage.hpp"
#include "th02/main/tile/tile.hpp"

// Turbo C++ initializer template for stage_loop()'s local
// [scroll_line_on_page] array. Lives in the root ASM's `_DATA` contribution
// because its address is in the middle of that contribution, and can
// therefore not be regenerated from this translation unit.
extern long scroll_line_on_page_init;

// Per-frame callback slots, all defaulting to nullfunc_void() / _false().
// -----------------------------------------------------------------------

// Erases the stage title from TRAM at [stage_frame] == 160, then disables
// itself.
extern farfunc_t_near stage_title_unput;

extern farfunc_t_near enemies_invalidate;
extern farfunc_t_near enemies_update_and_render;

// Per-stage foreground/background effects. TH04 and TH05 declare the same
// pair of per-stage slots as stage_invalidate / stage_render in
// th04/main/stage/stage.hpp; TH02 runs one combined update-and-render pass,
// hence the second name.
extern farfunc_t_near stage_invalidate;
extern farfunc_t_near stage_update_and_render;

// Only installed for Stage 4 and Extra. What they render is not evidenced,
// hence the neutral names. [static]
extern farfunc_t_near farfp_23A72;
extern farfunc_t_near farfp_23A76;

// Starts the boss fight once the map has been scrolled to its end, then
// disables itself.
extern farfunc_t_near boss_activate_if_scroll_done;

// Returns `true` once the stage is over, which ends this loop.
// The slot's default installed function is `bool nullfunc_false(void)`, and
// the sibling slot above is a `bool` too, but this one has to be [bool16]:
// ZUN's code tests the result with `or ax, ax`, and a `bool` return compiles
// to `or al, al` — a real behavioral difference for any installed function
// that returns a nonzero high byte. (kb/codegen/0090)
extern bool16 (far pascal *stage_should_end)(void);
// -----------------------------------------------------------------------

extern void (far pascal *boss_bg_render)(void);
extern stage_progression_t (far pascal *boss_update)(void);

extern "C" void near bgm_show(void);
extern "C" void near sub_E2D9(void);
extern "C" void near sub_F1D8(void);
extern "C" void sub_16D9B(void);
extern "C" void pascal DemoPlay(void);

// Instance #1 of the static egc_start_copy(), emitted into main_01_TEXT.
void near egc_start_copy_1(void);

// Turbo C++ compiled ZUN's far calls to same-code-group functions as
// `nop; push cs; call near ptr`. Now that this function sits in its own
// segment, only inline ASM still emits those bytes. (kb/codegen/0014)
#define nopcall_same_group(func) _asm { \
	nop; \
	push	cs; \
	call	near ptr func; \
}

// master.lib's egc_off(), inlined.
#define egc_off_inlined() { \
	outport2(EGC_ACTIVEPLANEREG, 0xFFF0); \
	outport2(EGC_MASKREG, 0xFFFF); \
	_outportb_(0x6A, 0x07); /* EGC register write enable */ \
	_outportb_(0x6A, 0x04); /* GRCG-compatible mode */ \
	_outportb_(0x7C, 0x00); /* GRCG off */ \
	_outportb_(0x6A, 0x06); /* EGC register write disable */ \
}

// Runs the given stage until it's either cleared or quit out of. Returns
// `true` if the game should advance to the next stage.
bool16 stage_loop(void)
{
	// master.lib's graph_scroll() parameters, inlined below. master.lib
	// documents these two as belonging to the 上領域 ("upper region").
	int gdc_lines_upper;
	int gdc_sad_upper;

	union {
		long init;
		vram_y_t line[PAGE_COUNT];
	} scroll_line_on_page;

	register vram_y_t scroll_line_saved;

	scroll_line_on_page.init = scroll_line_on_page_init;

	while(!quit) {
		snd_se_update();
		stage_title_unput();
		bgm_show();

		// The unblitting pass has to use the [scroll_line] that the back page
		// was last blitted with, not the current one.
		scroll_line_saved = scroll_line;
		scroll_line = scroll_line_on_page.line[page_back];

		boss_bg_render();
		if(midboss_active) {
			midboss_active = midboss_invalidate();
		}
		enemies_invalidate();
		player_invalidate();
		bullets_invalidate();
		farfp_23A72();
		items_invalidate();
		sparks_invalidate();
		sub_E2D9();
		if(scroll_step_advanced) {
			sub_16D9B();
			scroll_step_advanced = false;
		}

		egc_start_copy_1();
		stage_invalidate();
		tiles_egc_render();
		stage_update_and_render();
		egc_off_inlined();

		if(scroll_step == midboss_scroll_step) {
			midboss_active = true;
		}

		scroll_delta = 0;
		scroll_line = scroll_line_saved;
		if(!scroll_done) {
			if(!(scroll_cycle % scroll_interval)) {
				scroll_line -= scroll_speed;
				if(scroll_line < 0) {
					scroll_line += RES_Y;
				}
				scroll_delta = scroll_speed;
			}
		}
		scroll_line_on_page.line[page_back] = scroll_line;

		input_reset_sense();
		if(resident->demo_num) {
			nopcall_same_group(DemoPlay);
		}

		sub_F1D8();
		bomb_update_and_render();
		enemies_update_and_render();
		stage_progression = boss_update();
		if(midboss_active) {
			midboss_update_and_render();
		}
		player_update_and_render();
		items_update_and_render();
		farfp_23A76();
		bullets_update_and_render();
		sparks_update_and_render();
		if(key_det & INPUT_CANCEL) {
			if(pause_menu()) {
				quit = true;
			}
		}

		if(scroll_delta) {
			// ZUN bloat: Manual strength reduction of a multiplication with
			// the (RES_X / 16) = 40 VRAM words per line, which Turbo C++
			// would have compiled into a single 386 IMUL.
			scroll_sad = ((scroll_line * 32) + (scroll_line * 8));
			if(!(scroll_line & 7)) {
				scroll_step++;
				scroll_step_advanced = true;
			}
			egc_start_copy_1();
			scroll_done = tiles_scroll_and_egc_render_both(scroll_speed);
			egc_off_inlined();
			if(scroll_done) {
				// On this final frame of the stage, we already were one pixel
				// past the end of the tile map. Pretend we didn't overshoot by
				// just not running the GDC SCROLL command this frame, and fix
				// up [scroll_line] so that all sprites blitted during the boss
				// fight appear at their correct Y coordinate.
				// ZUN bug: But we've just rendered a full frame of sprites *at*
				// the wrong [scroll_line] :zunpet: As a result, these will
				// appear one pixel higher than where they should be, since we
				// skip the GDC SCROLL that would compensate for it.
				scroll_line++;
				goto flip;
			}

			gdc_lines_upper = (RES_Y - scroll_line);
			gdc_sad_upper = scroll_sad;
			asm {
				mov 	cx, graph_VramZoom;
				mov 	cl, 4;
			}
		gdc_wait:
			asm {
				db  	0EBh, 000h; // jmp short $+2
				in  	al, 0A0h;
				test	al, cl;
				je  	gdc_wait;
				mov 	al, 70h;
				out 	0A2h, al;

				mov 	ax, gdc_sad_upper;
				out 	0A0h, al;
				db  	088h, 0E0h; // mov al, ah
				db  	0EBh, 000h; // jmp short $+2
				db  	0EBh, 000h; // jmp short $+2
				out 	0A0h, al;

				mov 	ax, gdc_lines_upper;
				shl 	ax, cl;
				db  	008h, 0ECh; // or ah, ch
				out 	0A0h, al;
				db  	088h, 0E0h; // mov al, ah
				db  	0EBh, 000h; // jmp short $+2
				db  	0EBh, 000h; // jmp short $+2
				out 	0A0h, al;

				mov 	ax, 0;
				out 	0A0h, al;
				db  	088h, 0E0h; // mov al, ah
				db  	0EBh, 000h; // jmp short $+2
				db  	0EBh, 000h; // jmp short $+2
				out 	0A0h, al;

				mov 	ax, RES_Y;
				shl 	ax, cl;
				db  	008h, 0ECh; // or ah, ch
				out 	0A0h, al;
				db  	088h, 0E0h; // mov al, ah
				db  	0EBh, 000h; // jmp short $+2
				db  	0EBh, 000h; // jmp short $+2
				out 	0A0h, al;
			}
			if(scroll_done) {
				goto flip;
			}
		}
		scroll_cycle++;

flip:
		if(slowdown_factor == 1) {
			vsync_Count1 = 0;
		}
		while(slowdown_factor > vsync_Count1) {
		}
		vsync_Count1 = 0;

		page_front = page_back;
		// page_show(page_back), reusing the value still live in AL.
		_asm { out 0A4h, al; }
		page_back ^= 1;
		page_access(page_back);

		boss_activate_if_scroll_done();
		resident->frame++;
		stage_frame++;
		if(stage_should_end() && !quit) {
			return true;
		}

		if((stage_frame % 1800) == 1500) {
			if(playperf_max > playperf) {
				playperf++;
				__emit__(0xEB, 0x00); // JMP SHORT $+2
			}
		}
		score_update_and_render();
	}
	nopcall_same_group(score_delta_commit);
	quit = false;
	return false;
}
