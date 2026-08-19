/// Extra Stage Boss - EX-Alice, custom bullet rendering
/// -----------------------------------------------------
/// EX-Alice's [boss_custombullets_render] callback: the two fire waves that
/// crawl up the left and right playfield edges, and — for no reason connected
/// to either — a tail call to cheetos_render().
///
/// (#included from th05/midboss5.cpp, ahead of th05/main/boss/bx_fg.cpp. This
/// was the ONLY proc of th05_main.asm's main_0_TEXT root contribution, and
/// that object already owned everything after it (kb/codegen/0114), so the
/// #include is the original address order and costs no carve, no new segment
/// and no Tupfile.lua line. th05_main.asm's contribution to main_0_TEXT is
/// now zero bytes.
///
/// The fire wave STATE lives in th05/main/boss/bx.cpp — same boss, different
/// segment: firewaves_add() and firewaves_update() are BX_UPDATE_TEXT, this
/// renderer is main_0_TEXT. That split is ZUN's, so firewave_t is declared in
/// both files rather than moved to a header both compile.

#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
// th03/math/polar.hpp's polar_y() is the out-of-line `int pascal polar()`,
// NOT th01/math/polar.hpp's inline template of the same name. This function
// calls it, so this file must be the one that decides, and nothing else in
// this translation unit pulls th01's.
#include "th03/math/polar.hpp"
#include "th04/main/playfld.hpp"
#include "th04/main/player/player.hpp"
#include "th04/formats/super.h"
#include "th05/main/bullet/cheeto.hpp"
#include "th05/sprites/main_pat.h"

// Kept in sync with th05/main/boss/bx.cpp, which owns the update half.
struct near firewave_t {
	bool alive;
	bool is_right;
	vram_y_t bottom;
	pixel_t amp;
};

#define FIREWAVE_COUNT 2

extern firewave_t firewaves[FIREWAVE_COUNT];

// Half-height of the player's hitbox for the fire wave, in pixels, and also
// the row step of the wave itself.
static const pixel_t FIREWAVE_ROW_H = 16;

// How far into the playfield the wave's own body reaches past the edge column
// it is drawn from. The hit test uses it on both sides.
static const pixel_t FIREWAVE_REACH = 32;

// `extern "C"` + `pascal`, because th05_main.asm exports this one as the bare
// uppercase `EXALICE_CUSTOMBULLETS_RENDER` and still takes its address there
// (`mov _boss_custombullets_render, offset exalice_custombullets_render`).
// Plain C++ linkage would emit `@EXALICE_CUSTOMBULLETS_RENDER$QV` instead and
// leave that `offset` unresolved (kb/codegen/0081).
extern "C" void pascal near exalice_custombullets_render(void)
{
	// The original's frame is `ENTER 0Ah, 0` / `push si` / `push di`: seven
	// 16-bit locals, two enregistered and five on the stack. The two register
	// variables have to be declared FIRST, and the five stack locals then
	// take -2, -4, -6, -8 and -0Ah in the order below (kb/codegen/0010 for
	// the slots, 0117 + 0146 for the registers).
	//
	// **The register order here is SI then DI, which is the OPPOSITE of what
	// th05/main/boss/bx_fg.cpp records two functions later in this very
	// object.** Declaring [firewave] first — the natural spelling, and the
	// one the pointer-earns-SI reading of 0117 predicts — put the pointer in
	// SI and swapped every one of the 20 differing instruction slots. Neither
	// "first-declared takes SI" nor "first-declared takes DI" is a law; the
	// only reliable move is to read the original's first `MOV` into a
	// register and order the declarations to match it.
	vram_y_t top;
	firewave_t near *firewave;

	int i;
	int angle;
	screen_x_t x;
	int patnum;
	screen_x_t left;

	firewave = firewaves;
	for(i = 0; i < FIREWAVE_COUNT; (i++, firewave++)) {
		if(!firewave->alive) {
			continue;
		}

		// The low nibble of [bottom] doubles as the wave's phase: it picks
		// the starting angle, and is then masked back out of the row the wave
		// is actually drawn at. So a wave whose 4-pixel-per-frame descent has
		// not yet crossed a 16-pixel row still animates.
		top = firewave->bottom;
		angle = ((top & (FIREWAVE_ROW_H - 1)) / 2);
		top &= ~(FIREWAVE_ROW_H - 1);

		patnum = (PAT_FIREWAVE_LEFT + firewave->is_right);

		for(; ((top >= PLAYFIELD_TOP) && (angle < 0x80));
		      (top -= FIREWAVE_ROW_H, angle += 8)) {
			if(top >= (PLAYFIELD_TOP + PLAYFIELD_H)) {
				continue;
			}

			// A sine of the row's phase, scaled by the wave's amplitude. The
			// center is 0, so this is a signed offset from the edge.
			x = polar_y(0, firewave->amp, angle);
			if(!firewave->is_right) {
				x += FIREWAVE_ROW_H;
			} else {
				x = (PLAYFIELD_RIGHT - x);
			}
			left = x;

			if(!firewave->is_right) {
				// Everything between the playfield's left edge and the wave
				// is solid fire, byte-aligned.
				if(x > FIREWAVE_REACH) {
					grcg_byteboxfill_x(
						PLAYFIELD_VRAM_LEFT,
						top,
						((x - 1) / BYTE_DOTS),
						(top + (FIREWAVE_ROW_H - 1))
					);
				}
				if(
					(static_cast<unsigned int>(
						player_pos.cur.y.to_pixel_slow() - top
					) < FIREWAVE_ROW_H) &&
					(player_pos.cur.x.to_pixel_slow() < (x - FIREWAVE_REACH))
				) {
					player_is_hit = true;
				}
			} else {
				x += FIREWAVE_ROW_H;
				if(x < PLAYFIELD_RIGHT) {
					grcg_byteboxfill_x(
						(x / BYTE_DOTS),
						top,
						(PLAYFIELD_VRAM_RIGHT - 1),
						(top + (FIREWAVE_ROW_H - 1))
					);
				}
				if(
					(static_cast<unsigned int>(
						player_pos.cur.y.to_pixel_slow() - top
					) < FIREWAVE_ROW_H) &&
					(player_pos.cur.x.to_pixel_slow() > (x - FIREWAVE_REACH))
				) {
					player_is_hit = true;
				}
			}

			_ES = SEG_PLANE_B;
			z_super_roll_put_tiny_16x16(left, top, patnum);
		}
	}

	// ZUN bloat: Nothing about the cheeto bullets belongs to EX-Alice's
	// custom bullets. She is simply the only boss whose callback also has to
	// render them, because th05_main.asm's stage loop does not.
	cheetos_render();
}
