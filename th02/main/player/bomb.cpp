#include "libs/master.lib/pc98_gfx.hpp"
#include "th01/math/overlap.hpp"
#include "th01/math/polar.hpp"
#include "th02/v_colors.hpp"
#include "th02/formats/pi.h"
#include "th02/snd/snd.h"
#include "th02/hardware/pages.hpp"
#include "th02/math/randring.hpp"
#include "th02/main/playfld.hpp"
#include "th02/main/scroll.hpp"
#include "th02/main/slowdown.hpp"
#include "th02/main/spark.hpp"
#include "th02/main/bullet/bullet.hpp"
#include "th02/main/hud/overlay.hpp"
#include "th02/main/tile/tile.hpp"
#include "th02/main/player/player.hpp"
#include "th02/main/player/bomb.hpp"
#include "th02/sprites/bombpart.h"

// Function ordering fails
// -----------------------

// These assume the GRCG to be set to RMW mode, with a tile in the intended
// color.
void pascal near bomb_circle_point_put(screen_x_t left, screen_y_t top);
void pascal near bomb_particle_put_8(screen_x_t left, screen_y_t top, int cel);
void pascal near bomb_smear_put_8(screen_x_t left, screen_y_t column_bottom);
void pascal near bomb_bft_8tiles_put_8(
	screen_x_t left, screen_y_t top, dots8_t dots
);
// -----------------------

extern "C" bool reduce_effects;

// Both allocated by bomb_load(), and both still declared per-TU because
// th02_main.asm publishes them as label aliases rather than from a header.
extern uint8_t *bomb_bft;
extern uint8_t *bomb1_bft;

// The number of points sampled around the bomb circle, and the angle step that
// spreads them over the full turn. (Same two constants as in bombload.cpp,
// whose bomb_invalidate() walks the same ring.)
static const int BOMB_CIRCLE_POINTS = 64;
static const unsigned char BOMB_CIRCLE_ANGLE_STEP = 4;

// One BOMBS.BFT cel is a 1bpp mask of the 24×24 tile grid that covers the
// playfield: 24 rows of 3 bytes.
static const unsigned BOMB_BFT_CEL_SIZE = 72;

// One BOMB1.BFT cel is 24 TRAM rows of 12 bytes, each nibble one 8×16 cell.
#define BOMB1_BFT_CEL_SIZE 288
#define BOMB1_BFT_CELS 18

// Shottype A's and B's sparkles. (The cel count comes from bombpart.h.)
// Shottype C's vertical smear columns, 8 pixels wide each.
// ZUN bloat: Write-only. bomb_circle_update_and_render() stores [scroll_line]
// here once per page, and nothing in the binary ever reads it back.
extern "C" vram_y_t bomb_circle_scroll_line_unused[PAGE_COUNT];

// The sparkles rendered by bomb_particles_update_and_render(). [cel] is 0 for
// a free slot, or the sBOMB_PARTICLES cel to render plus 1.
extern "C" point_t bomb_particle_pos[BOMB_PARTICLE_COUNT];
extern "C" uint8_t bomb_particle_cel[BOMB_PARTICLE_COUNT];

// One of shottype C's downward-growing smear columns.
// How far each of shottype C's smear columns grows per frame. (kb/codegen/0084:
// this initializer template has to stay in th02_main.asm's own _DATA.)
extern "C" const int BOMB_SMEAR_SPEEDS[BOMB_SMEAR_COLUMNS];

// [tile_mode] as it was before a shottype's bomb animation blanked the
// playfield. One slot per shottype, none of them shared.

// The two spaces that shottype B's TRAM effect prints per 16×16 cell.
// (kb/codegen/0084 again — a string literal in the dump's own _DATA.)
extern "C" const char bomb_reimu_b_tram_spaces[];

// The original's prologs down to bomb_reimu_b() are all
// `push bp; mov bp, sp; sub sp, N`, which is -G. bomb_update_and_render() and
// everything after it has no stack locals at all, so it is unaffected either
// way and keeps the file's original setting. (kb/codegen/0011)
#pragma option -G

void near bomb_circle_update_and_render(void)
{
	screen_x_t left;
	screen_y_t top;
	unsigned char angle;
	int radius;
	int i;

	bomb_circle_frame++;
	bomb_circle_scroll_line_unused[page_back] = scroll_line;
	grcg_setcolor(GC_RMW, V_WHITE);
	radius = (256 - (bomb_frame * 8));
	for(i = 0, angle = bomb_frame; i < BOMB_CIRCLE_POINTS; i++,
		angle = (angle + BOMB_CIRCLE_ANGLE_STEP)
	) {
		left = polar_x_fast(bomb_circle_center.x, radius, angle);
		top = polar_y_fast(bomb_circle_center.y, radius, angle);
		bomb_circle_point_put(left, top);
	}
	grcg_off();
}

// Shottype B's expanding rings, drawn with the same sampling as the initial
// bomb circle but at a caller-controlled radius.
void pascal near bomb_b_ring_put(int radius_step)
{
	screen_x_t left;
	screen_y_t top;
	unsigned char angle;
	int radius;
	int i;

	grcg_setcolor(GC_RMW, 14);
	radius = (radius_step * 32);
	for(i = 0, angle = radius_step; i < BOMB_CIRCLE_POINTS; i++,
		angle = (angle + BOMB_CIRCLE_ANGLE_STEP)
	) {
		left = polar_x_fast(bomb_circle_center.x, radius, angle);
		top = polar_y_fast(bomb_circle_center.y, radius, angle);
		bomb_circle_point_put(left, top);
	}
	grcg_off();
}

// Spawns two sparkles per frame into the two slots selected by [bomb_frame],
// then animates and renders all of them. Each slot only advances on the frames
// where the low 2 bits of [bomb_frame] equal the low 2 bits of its own index,
// which is what staggers the animation across the array.
void near bomb_particles_update_and_render(void)
{
	int slot_base;
	int i;

	slot_base = ((bomb_frame & 15) * 2);
	for(i = 0; i < 2; i++) {
		bomb_particle_pos[slot_base + i].x = (
			(randring1_next16() % PLAYFIELD_W) + PLAYFIELD_LEFT
		);
		bomb_particle_pos[slot_base + i].y = (
			(randring1_next16() % PLAYFIELD_H) + PLAYFIELD_TOP
		);
		bomb_particle_cel[slot_base + i] = 1;
	}
	for(i = 0; i < BOMB_PARTICLE_COUNT; i++) {
		if(bomb_particle_cel[i] == 0) {
			continue;
		}
		if((bomb_frame & 3) == (i & 3)) {
			bomb_particle_cel[i]++;
			if(bomb_particle_cel[i] >= (BOMB_PARTICLE_CELS + 1)) {
				bomb_particle_cel[i] = 0;
				continue;
			}
		}
		bomb_particle_put_8(
			bomb_particle_pos[i].x,
			bomb_particle_pos[i].y,
			(bomb_particle_cel[i] - 1)
		);
	}
}

// Renders the BOMBS.BFT cel for the current [bomb_frame] over the whole
// playfield, one 128×16 strip of eight tiles at a time.
void near bomb_bft_put(void)
{
	uint8_t *p;
	int cel;
	screen_y_t top;

	cel = ((bomb_frame - 1) >> 1);
	p = (bomb_bft + (cel * BOMB_BFT_CEL_SIZE));
	for(top = PLAYFIELD_TOP; top < (PLAYFIELD_BOTTOM + TILE_H); top += TILE_H) {
		bomb_bft_8tiles_put_8((PLAYFIELD_LEFT + (0 * 8 * TILE_W)), top, p[0]);
		bomb_bft_8tiles_put_8((PLAYFIELD_LEFT + (1 * 8 * TILE_W)), top, p[1]);
		bomb_bft_8tiles_put_8((PLAYFIELD_LEFT + (2 * 8 * TILE_W)), top, p[2]);
		p += 3;
	}
}

extern "C" bool16 pascal near bomb_reimu_a(void)
{
	screen_y_t top;

	// One slot, two jobs: the palette component index below, and the
	// grayscale tone in the [bomb_frame] branches after it. The tone is read
	// back a byte at a time, which no 16-bit register can do, so this variable
	// can never be a register variable — which is why [top] gets SI and DI is
	// never pushed.
	int component;

	grcg_setcolor(GC_RMW, 0);
	if(bomb_frame == 1) {
		for(component = 0; component < COMPONENT_COUNT; component++) {
			col0_before_bomb_a.v[component] = Palettes[0].v[component];
			col3_before_bomb_a.v[component] = Palettes[3].v[component];
		}
		palette_set(3, 64, 16, 64);
		palette_show();
	}
	if(bomb_frame < BOMB_CIRCLE_FRAMES) {
		grcg_setcolor(GC_RMW, 3);
		bomb_bft_put();
	} else if(bomb_frame == BOMB_CIRCLE_FRAMES) {
		palette_set(
			3, col3_before_bomb_a.c.r, col3_before_bomb_a.c.g,
			col3_before_bomb_a.c.b
		);
		palette_set(0, 128, 0, 128);
		palette_show();
		tile_mode_before_bomb_a = tile_mode;
		tile_mode = TM_NONE;
		grcg_boxfill(PLAYFIELD_LEFT, 0, (PLAYFIELD_RIGHT - 1), (RES_Y - 1));
	} else if(bomb_frame < 136) {
		if(bomb_frame > 64) {
			grcg_setcolor(GC_RMW, 11);
		}
		grcg_boxfill(PLAYFIELD_LEFT, 0, (PLAYFIELD_RIGHT - 1), (RES_Y - 1));
		grcg_off();
		if(bomb_frame <= 64) {
			component = (128 - ((bomb_frame - BOMB_CIRCLE_FRAMES) * 4));
			palette_set(0, component, 0, component);
			if(bomb_frame == 64) {
				palette_settone(200);
				palette_set(0, 255, 255, 255);
			}
			palette_show();
		} else {
			sparks_add(
				(PLAYFIELD_LEFT + 180), (PLAYFIELD_TOP + 174),
				((3 << 4) + 2), 1, false
			);
			if(bomb_frame == 70) {
				palette_settone(100);
			} else if(!reduce_effects || (bomb_frame & 1)) {
				top = 90;
				top += scroll_line;
				if(top >= RES_Y) {
					top -= RES_Y;
				}
				pi_put_8(112, top, 1);
			}
			if(bomb_frame >= 86) {
				if(bomb_frame % 3) {
					component = 255;
				} else {
					component = 160;
				}
				palette_set(0, component, component, component);
				PaletteTone = ((bomb_frame * 2) - 72);
				palette_show();
				if(bomb_frame > 111) {
					slowdown_factor = 2;
				}
			}
		}
		if(bomb_frame < 86) {
			grcg_setcolor(GC_RMW, V_WHITE);
			if(!(bomb_frame & 3)) {
				snd_se_play(15);
			}
			bomb_particles_update_and_render();
		}
		if(bomb_frame == 86) {
			snd_se_play(17);
		}
	} else {
		if(bomb_frame == 136) {
			palette_set(
				0, col0_before_bomb_a.c.r, col0_before_bomb_a.c.g,
				col0_before_bomb_a.c.b
			);
			palette_show();
			snd_se_play(16);
			tile_mode = tile_mode_before_bomb_a;
			slowdown_factor = 1;
			palette_settone(200);
			bullets_clear();
		} else if(bomb_frame < 156) {
			palette_settone(200 - ((bomb_frame * 5) - 680));
		} else {
			grcg_off();
			return true;
		}
	}
	grcg_off();
	return false;
}

extern "C" bool16 pascal near bomb_reimu_c(void)
{
	screen_y_t top;
	int i;
	int speeds[BOMB_SMEAR_COLUMNS];

	// kb/codegen/0084: [BOMB_SMEAR_SPEEDS] has to stay in th02_main.asm's own
	// _DATA contribution, so this is spelled out rather than written as the
	// aggregate initializer it originally was. kb/codegen/0109: a plain copy
	// assignment would come out as a far call to F_SCOPY@ here, because
	// th02/main is not built with -G at the Tupfile level.
	asm { mov si, offset BOMB_SMEAR_SPEEDS; }
	asm { lea di, speeds; }
	asm { push ss; pop es; }
	asm { mov cx, BOMB_SMEAR_COLUMNS; }
	asm { rep movsw; }

	grcg_setcolor(GC_RMW, 2);
	if(bomb_frame < BOMB_CIRCLE_FRAMES) {
		bomb_bft_put();
	} else if(bomb_frame == BOMB_CIRCLE_FRAMES) {
		tile_mode_before_bomb_c = tile_mode;
		tile_mode = TM_NONE;
		grcg_boxfill(PLAYFIELD_LEFT, 0, (PLAYFIELD_RIGHT - 1), (RES_Y - 1));
		for(i = 0; i < BOMB_SMEAR_COLUMNS; i++) {
			bomb_smears[i].bottom = 20;
		}
		snd_se_play(17);
	} else if(bomb_frame < 100) {
		grcg_boxfill(PLAYFIELD_LEFT, 0, (PLAYFIELD_RIGHT - 1), (RES_Y - 1));
		if(bomb_frame >= 80) {
			palette_settone((bomb_frame * 5) - 300);
			slowdown_factor = 2;
		}
		grcg_setcolor(GC_RMW, 14);
		bomb_particles_update_and_render();
		grcg_setcolor(GC_RMW, 1);
		for(i = 0; i < BOMB_SMEAR_COLUMNS; i++) {
			bomb_smears[i].bottom += speeds[i];
			bomb_smear_put_8(
				((i * 8) + PLAYFIELD_LEFT), bomb_smears[i].bottom
			);
		}
	} else if(bomb_frame < 112) {
		palette_settone(100);
		if(bomb_frame & 1) {
			grcg_off();
			top = PLAYFIELD_TOP;
			top += scroll_line;
			if(top >= RES_Y) {
				top -= RES_Y;
			}
			pi_put_8(32, top, 1);
			snd_se_play(16);
		} else {
			grcg_setcolor(GC_RMW, V_WHITE);
			grcg_boxfill(PLAYFIELD_LEFT, 0, (PLAYFIELD_RIGHT - 1), (RES_Y - 1));
		}
	} else if(bomb_frame == 112) {
		tile_mode = tile_mode_before_bomb_c;
		slowdown_factor = 1;
		palette_settone(200);
		bullets_clear();
	} else if(bomb_frame < 128) {
		palette_settone(200 - ((bomb_frame * 5) - 540));
	} else {
		grcg_off();
		return true;
	}
	grcg_off();
	return false;
}

// Renders one BOMB1.BFT cel into TRAM, two spaces per nibble.
void pascal near bomb_b_tram_put(int cel)
{
	int y;
	int x;
	int atrb;

	// The compound assignment is load-bearing: `cel = (cel * N)` folds into the
	// 386 three-operand `imul ax, ax, N` (kb/codegen/0053), while the original
	// has the generic `mov ax, N; imul word ptr [bp+4]`.
	cel *= BOMB1_BFT_CEL_SIZE;
	for(y = 1; y <= 23; y++) {
		for(x = 4; x < 52; ) {
			if(bomb1_bft[cel] & 0xF0) {
				atrb = (TX_RED | TX_REVERSE);
			} else {
				atrb = TX_WHITE;
			}
			text_putsa(x, y, bomb_reimu_b_tram_spaces, atrb);
			x += 2;
			if(bomb1_bft[cel] & 0x0F) {
				atrb = (TX_RED | TX_REVERSE);
			} else {
				atrb = TX_WHITE;
			}
			text_putsa(x, y, bomb_reimu_b_tram_spaces, atrb);
			x += 2;
			cel++;
		}
	}
}

extern "C" bool16 pascal near bomb_reimu_b(void)
{
	screen_y_t top;

	grcg_setcolor(GC_RMW, V_WHITE);
	if(bomb_frame < BOMB_CIRCLE_FRAMES) {
		bomb_bft_put();
	} else if(bomb_frame == BOMB_CIRCLE_FRAMES) {
		bomb_b_cel = 0;
		tile_mode_before_bomb_b = tile_mode;
		tile_mode = TM_NONE;
		grcg_boxfill(PLAYFIELD_LEFT, 0, (PLAYFIELD_RIGHT - 1), (RES_Y - 1));
	} else if(bomb_frame < 132) {
		if((bomb_frame > BOMB_CIRCLE_FRAMES) && !(bomb_frame & 3)) {
			if((bomb_frame & 7) == 4) {
				snd_se_play(16);
			}
			bomb_b_tram_put(bomb_b_cel);
			bomb_b_cel++;
			if(bomb_b_cel >= BOMB1_BFT_CELS) {
				overlay_wipe();
				bomb_frame = 132;
			}
		}
		grcg_boxfill(PLAYFIELD_LEFT, 0, (PLAYFIELD_RIGHT - 1), (RES_Y - 1));
		grcg_setcolor(GC_RMW, 4);
		bomb_particles_update_and_render();
	} else if(bomb_frame < 164) {
		grcg_boxfill(PLAYFIELD_LEFT, 0, (PLAYFIELD_RIGHT - 1), (RES_Y - 1));
		slowdown_factor = 2;
		if(!reduce_effects || (bomb_frame & 1)) {
			grcg_off();
			top = 84;
			top += scroll_line;
			if(top >= RES_Y) {
				top -= RES_Y;
			}
			pi_put_8(32, top, 1);
		}
		if(!(bomb_frame & 3)) {
			snd_se_play(15);
		}
		if(bomb_frame <= 140) {
			_AX = (bomb_frame - 132);
		} else if(bomb_frame <= 148) {
			_AX = (bomb_frame - 140);
		} else if(bomb_frame <= 156) {
			_AX = (bomb_frame - 148);
		} else {
			_AX = (bomb_frame - 156);
		}
		bomb_b_ring_put(_AX);
		_AX = ((bomb_frame * 3) - 396);
		// This function has no stack frame at all, so the tone delta computed
		// by both branches lives in AX across the backward goto below. DX has
		// to be spelled out: `palette_settone(200 - _AX)` puts the 200 into AX
		// itself and degenerates into `sub ax, ax`.
tone:
		_DX = 200;
		_DX -= _AX;
		PaletteTone = _DX;
		palette_show();
	} else if(bomb_frame == 164) {
		snd_se_play(16);
		tile_mode = tile_mode_before_bomb_b;
		slowdown_factor = 1;
		palette_settone(200);
		bullets_clear();
	} else if(bomb_frame < 184) {
		_AX = ((bomb_frame * 5) - 820);
		goto tone;
	} else {
		grcg_off();
		return true;
	}
	grcg_off();
	return false;
}

#pragma option -G-

void near bomb_update_and_render(void)
{
	bool16 done = false;

	if(!bombing) {
		return;
	}
	bomb_frame++;
	if(bomb_circle_done == false) {
		if(bomb_frame <= BOMB_CIRCLE_FRAMES) {
			bomb_circle_update_and_render();
		} else if(bomb_circle_frame == 0) {
			bomb_frame = 0;
			bomb_circle_done++;
		}
	} else if(bomb_circle_done == true) {
		done = playchar_bomb_func();
	}
	if(done) {
		bombing = false;
		player_invincible_via_bomb = false;
		player_invincibility_time = 70;
		palette_100();
	}
}

// ZUN bloat: Needed to circumvent 16-bit promotion in a single comparison.
inline pixel_delta_8_t bomb_particle_h(void) {
	return BOMB_PARTICLE_H;
}

void pascal near bomb_circle_point_put(screen_x_t left, screen_y_t top)
{
	#define vram_top          	static_cast<vram_y_t>(_DX)
	#define first_bit_mirrored	_DX

	register const bomb_particle_dots_t near* sprite;
	pixel_delta_8_t y;
	unsigned int first_bit;

	if(!overlap_xy_ltrb_lt_gt(
		left, top,
		(PLAYFIELD_LEFT - BOMB_PARTICLE_W),
		(PLAYFIELD_TOP  - BOMB_PARTICLE_H),
		PLAYFIELD_RIGHT,
		PLAYFIELD_BOTTOM
	)) {
		return;
	}

	_ES = SEG_PLANE_B;
	vram_top = scroll_screen_y_to_vram(vram_top, top);
	_BX = left;
	vram_offset_shift_fast_asm(di, bx, vram_top);

	// ZUN bloat: Turbo C++ 4.0J would otherwise only ever generate an 8-bit
	// AND.
	asm { and	bx, BYTE_MASK; }

	first_bit = _BX;
	first_bit_mirrored = (sizeof(dots16_t) * BYTE_DOTS);

	// ZUN bloat: first_bit_mirrored -= _BX;
	asm { sub	dx, bx; }

	sprite = sBOMB_CIRCLE;
	y = 0;
	do {
		// ZUN bloat: A manual 16-bit right rotation. Turbo C++ 4.0J has
		// intrinsics for that, which compile into a single ROR instruction:
		//
		// 	*reinterpret_cast<dots16_t __es *>(vo) = __rotr__(
		// 		*sprite, first_bit
		// );
		asm { xor	ax, ax; }
		_AL = *sprite;
		asm { mov	bx, ax; }
		_CX = first_bit;
		_BX >>= _CL;
		asm { mov	cx, dx; }
		_AX <<= _CL;
		asm { add	ax, bx; }
		*reinterpret_cast<dots16_t __es *>(_DI) = _AX;

		vram_offset_add_and_roll(_DI, ROW_SIZE);
		sprite++;
		y++;
	} while(y < bomb_particle_h());

	#undef first_bit_mirrored
	#undef vram_top
}

void pascal near bomb_particle_put_8(screen_x_t left, screen_y_t top, int cel)
{
	#define _DI	static_cast<vram_offset_t>(_DI)

	_ES = SEG_PLANE_B;
	_DX = scroll_screen_y_to_vram(static_cast<vram_y_t>(_DX), top);
	vram_offset_shift_fast_asm(di, left, _DX);

	// ZUN bloat: _SI  = &sBOMB_PARTICLES[cel];
	_SI = reinterpret_cast<uint16_t>(&sBOMB_PARTICLES[0][0]);
	_AX = (cel * sizeof(sBOMB_PARTICLES[0]));
	asm { add	si, ax; }

	_CX = BOMB_PARTICLE_H;
	loop: {
		asm { movsb; }
		vram_offset_add_and_roll(
			_DI, (ROW_SIZE - (BOMB_PARTICLE_W / BYTE_DOTS))
		);
		asm { loop loop; }
	}

	#undef _DI
}

void pascal near bomb_smear_put_8(screen_x_t left, screen_y_t column_bottom_)
{
	#define y            	static_cast<vram_y_t>(_DX)
	#define column_bottom	static_cast<screen_y_t>(_BX)

	_ES = SEG_PLANE_B;
	y = scroll_screen_y_to_vram(y, PLAYFIELD_TOP);
	vram_offset_shift_fast_asm(di, left, y);

	// ZUN bloat: &sBOMB_CIRCLE[BOMB_PARTICLE_H / 2];
	register const bomb_particle_dots_t near* sprite = &sBOMB_CIRCLE[0];
	sprite += ((BOMB_PARTICLE_H / 2) * sizeof(bomb_particle_dots_t));

	_CX = (BOMB_PARTICLE_H / 2);
	y = PLAYFIELD_TOP;
	column_bottom = column_bottom_;
	_AL = static_cast<bomb_particle_dots_t>(-1);
	loop: {
		*reinterpret_cast<bomb_particle_dots_t __es *>(_DI) = _AL;
		vram_offset_add_and_roll(_DI, ROW_SIZE);
		y++;

		// ZUN bloat: if(y >= column_bottom)
		asm { cmp	dx, bx; }
		asm { jl 	still_in_column; }

		// We're in the bottom part; switch to drawing the sprite rather than
		// the constant 0xFF set before the loop.
		_AL = *sprite++;
		_CX--;

	still_in_column:
		// ZUN bloat: do { … } while(_CX > 0);
		asm { cmp	cx, 0; }
		asm { ja 	loop; }
	}

	#undef column_bottom
	#undef y
}

void pascal near bomb_bft_tile_put_8(vram_offset_t vo)
{
	_CX = TILE_H;
	_DI = vo;
	_BX = static_cast<dots_t(TILE_W)>(-1);
	loop: {
		*reinterpret_cast<dots_t(TILE_W) __es *>(_DI) = _BX;
		vram_offset_add_and_roll(_DI, ROW_SIZE);
		asm { loop loop; }
	}
}

// Renders [dots] from BOMBS.BFT as a 8×1 row of 16×16 blocks, covering a total
// size of 128×16 pixels, to (⌊left/8⌋*8, top). Each tile is filled with the
// current GRCG tile if its corresponding bit is 1, or skipped otherwise.
// Conceptually identical to the tiles_bb_*() functions from TH04 and TH05.
void pascal near bomb_bft_8tiles_put_8(
	screen_x_t left, screen_y_t top, dots8_t dots
)
{
	#define tile_cur	static_cast<dots8_t>(_AL)
	#define tiles   	static_cast<dots8_t>(_AH)

	_ES = SEG_PLANE_B;
	_DX = scroll_screen_y_to_vram(static_cast<vram_y_t>(_DX), top);
	vram_offset_shift_fast_asm(di, left, _DX);
	tile_cur = 0x80;
	tiles = dots;
	asm { xor	si, si; }
	do {
		if(tile_cur & tiles) {
			bomb_bft_tile_put_8(_DI);
		}
		tile_cur >>= 1;
		_DI += TILE_VRAM_W;
		_SI++;
	} while(static_cast<int>(_SI) < BYTE_DOTS);

	#undef tiles
	#undef tile_cur
}
