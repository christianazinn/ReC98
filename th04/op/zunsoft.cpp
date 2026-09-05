#if (GAME == 5)
	#pragma option -zCCFG_TEXT -zPop_01
#else
	#pragma option -zCOP_MUSIC_TEXT -zPop_01
#endif

#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th01/math/subpixel.hpp"
#include "th03/math/polar.hpp"
#include "th04/hardware/bgimage.hpp"
#include "th04/snd/snd.h"
#if (GAME == 5)
	#include "th05/formats/pi.hpp"
	#include "th05/hardware/input.h"
#else
	#include "th03/formats/pi.hpp"
	#include "th04/hardware/input.h"
#endif
#include "th04/op/op.hpp"
#include "th04/op/zunsoft.h"

// Copy of the palette used during the logo, to allow non-blocking fades in
// contrast to master.lib's blocking palette_black_in() and palette_black_out()
// functions. (Then again, master.lib has the PaletteTone global for that...)
Palette8 zunsoft_palette;

// ZUN Soft logo explosion structure.
// These are only animated by increasing the distance; origin, angle, and speed
// stay constant.
struct pyro_t {
	bool alive;
	uint8_t age;
	SPPoint origin;
	Subpixel distance_prev; // ZUN bloat: Unused.
	Subpixel distance;
	Subpixel speed;
	unsigned char angle;
	unsigned char patnum_base; // displayed sprite is incremented every 4 frames
};

// Spawns [n] new explosions at the given screen-coordinate position.
extern "C" void pascal near zunsoft_pyro_new(
	screen_x_t origin_x, screen_y_t origin_y, int n, char patnum_base
);

extern "C" void pascal near zunsoft_update_and_render(void);

extern "C" void pascal near zunsoft_palette_update_and_show(int tone);

static const int PYRO_COUNT = 256;
pyro_t pyros[PYRO_COUNT];

char zun00_pi[] = "zun00.pi";
char logo[] = "logo";
char zun02_bft[] = "zun02.bft";
char zun04_bft[] = "zun04.bft";
char zun01_bft[] = "zun01.bft";
char zun03_bft[] = "zun03.bft";

extern "C" void pascal near zunsoft_pyro_new(
	screen_x_t origin_x, screen_y_t origin_y, int n, char patnum_base
)
{
	int i;
	int pyros_created;
	pyro_t near *pyro;

	pyros_created = 0;
	origin_y *= SUBPIXEL_FACTOR;
	origin_x *= SUBPIXEL_FACTOR;
	pyro = pyros;
	for(i = 0; i < PYRO_COUNT; i++, pyro++) {
		if(pyro->alive) {
			continue;
		}
		pyro->alive = true;
		pyro->age = 0;
		pyro->origin.x.v = origin_x;
		pyro->origin.y.v = origin_y;
		pyro->distance.v = 0;
		pyro->distance_prev.v = 0;
		pyro->speed.v = ((irand() % 224) + 64);
		pyro->angle = irand();
		pyro->patnum_base = patnum_base;
		pyros_created++;
		if(pyros_created >= n) {
			break;
		}
	}
}

extern "C" void pascal near zunsoft_update_and_render(void)
{
	pyro_t near *pyro;
	int i;
	int anim_sprite;
	int draw_x;
	int draw_y;
	int patnum;

	pyro = pyros;
	for(i = 0; i < PYRO_COUNT; i++, pyro++) {
		if(pyro->alive != true) {
			continue;
		}
		pyro->age++;
		anim_sprite = (pyro->age / 4);
		patnum = (pyro->patnum_base + anim_sprite);
		if(pyro->age >= 40) {
			pyro->alive = false;
			pyro->age = 0;
			continue;
		}
		if(pyro->age < 16) {
			super_put_rect(
				(pyro->origin.x.to_pixel_slow() - 8),
				(pyro->origin.y.to_pixel_slow() - 8),
				patnum
			);
		} else if(pyro->age < 32) {
			if(pyro->age == 16) {
				snd_se_play(15);
			}
			pyro->distance_prev.v = pyro->distance.v;
			pyro->distance.v += pyro->speed.v;
			draw_x = (polar_x(pyro->origin.x, pyro->distance, pyro->angle) - 128);
			draw_y = (polar_y(pyro->origin.y, pyro->distance, pyro->angle) - 128);
			super_put_rect((draw_x / 16), (draw_y / 16), patnum);
		} else {
			pyro->distance_prev.v = pyro->distance.v;
			pyro->distance.v += pyro->speed.v;
			draw_x = (polar_x(pyro->origin.x, pyro->distance, pyro->angle) - 256);
			draw_y = (polar_y(pyro->origin.y, pyro->distance, pyro->angle) - 256);
			super_put_rect((draw_x / 16), (draw_y / 16), patnum);
		}
	}
}

extern "C" void pascal near zunsoft_palette_update_and_show(int tone)
{
	int col;
	int comp;

	for(col = 0; col < (COLOR_COUNT - 1); col++) {
		for(comp = 0; comp < sizeof(RGB8); comp++) {
			Palettes[col].v[comp] = (
				(zunsoft_palette[col].v[comp] * tone) / 100
			);
		}
	}
	palette_show();
}

// kb/codegen/0139: pads TH04's generated switch table to an even offset.
#pragma option -a2

void near zunsoft_animate(void)
{
	page_t page;
	bool skip;
	uint8_t fade_in;
	uint8_t fade_out;
	pyro_t near *pyro;
	int i;
	int frame;

	skip = false;
	fade_in = 0;
	fade_out = 100;
	palette_settone(0);
	graph_accesspage(1);
	pi_load(0, zun00_pi);
	pi_palette_apply(0);
	pi_put_8(0, 0, 0);
	pi_free(0);
	graph_copy_page(0);
	bgimage_snap();
	graph_accesspage(1);
	graph_clear();
	graph_accesspage(0);
	graph_clear();
	for(i = 0; i < (COLOR_COUNT - 1); i++) {
		zunsoft_palette[i].c.r = Palettes[i].c.r;
		zunsoft_palette[i].c.g = Palettes[i].c.g;
		zunsoft_palette[i].c.b = Palettes[i].c.b;
		Palettes[i].c.r = 0;
		Palettes[i].c.g = 0;
		Palettes[i].c.b = 0;
	}
	snd_load(logo, SND_LOAD_SONG);
	snd_kaja_func(KAJA_SONG_PLAY, 0);
	pyro = pyros;
	i = 0;
	while(i < PYRO_COUNT) {
		// ZUN bloat: Clears [alive] and [age] with a single 16-bit write,
		// unlike the per-field reset in zunsoft_update_and_render().
		*reinterpret_cast<uint16_t near *>(pyro) = 0;
		i++;
		pyro++;
	}
	snd_delay_until_measure(2, 0);
	palette_settone(100);
	super_entry_bfnt(zun02_bft);
	super_entry_bfnt(zun04_bft);
	super_entry_bfnt(zun01_bft);
	super_entry_bfnt(zun03_bft);
	page = 0;
	graph_accesspage(1);
	graph_showpage(0);
	#if (GAME == 4)
		input_reset_sense();
	#endif
	for(frame = 0; frame < 170; frame++) {
		#if (GAME == 5)
			input_reset_sense_held();
		#else
			input_sense();
		#endif
		if(key_det != INPUT_NONE) {
			skip = true;
		}
		switch(frame) {
		case  0: zunsoft_pyro_new(180, 180, 20,  0); break;
		case 16: zunsoft_pyro_new(460, 220, 20, 10); break;
		case 24: zunsoft_pyro_new(220, 160, 20,  0); break;
		case 32: zunsoft_pyro_new(380, 240, 20, 10); break;
		case 40: zunsoft_pyro_new(200, 190, 20,  0); break;
		case 44: zunsoft_pyro_new(340, 200, 20, 10); break;
		case 48: zunsoft_pyro_new(280, 170, 20,  0); break;
		case 52: zunsoft_pyro_new(380, 260, 20, 10); break;
		case 56: zunsoft_pyro_new(200, 190, 20,  0); break;
		case 60: zunsoft_pyro_new(440, 210, 20, 10); break;
		case 64: zunsoft_pyro_new(320, 200, 64,  0); break;
		case 68: zunsoft_pyro_new(320, 200, 64, 10); break;
		}
		bgimage_put();
		zunsoft_update_and_render();
		#if (GAME == 4)
			input_reset_sense();
		#endif
		while(vsync_Count1 < 2) {}
		vsync_Count1 = 0;
		graph_accesspage(page);
		page = (1 - page);
		graph_showpage(page);
		if(!skip) {
			if((frame >= 16) && (fade_in < 100)) {
				fade_in += 2;
			}
			zunsoft_palette_update_and_show(fade_in);
		} else {
			// The branch structure here is not cosmetic: writing this
			// as an early exit instead loses the redundant reload of
			// [fade_out] that the original binary performs.
			if(fade_out > 0) {
				fade_out -= 2;
			} else {
				goto ret;
			}
			palette_settone(fade_out);
		}
		snd_se_update();
	}
	palette_black_out(1);
ret:
	super_free();
	bgimage_free();
}

#pragma option -a1
