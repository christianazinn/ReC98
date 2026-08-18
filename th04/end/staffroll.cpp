/// The staff roll
/// --------------
/// Two .PI backgrounds and nine .CDG credit images, dissolved in and out on a
/// schedule tied to the BGM's measure counter rather than to a frame count.
///
/// `[measured]` The last proc of th04_maine.asm's MAINE_01_TEXT contribution
/// to come out, and the one that empties that dump: `th04_maine.asm` no longer
/// contributes a single byte of code to TH04's MAINE.EXE.

/// Everything else this file needs — the CDG, bgimage, palette, page and
/// polar APIs — comes from th04/end/staff_dissolve.cpp, which precedes it in
/// the same translation unit.
#include "th03/formats/pi.hpp"
#include "th04/snd/snd.h"

/// `[measured]` All eleven filenames stay `_DATA` bytes of the root dump for
/// the reason th04/hiscore/regist_menu.cpp documents at length, and are
/// published there by renaming their IDA labels (kb/codegen/0123).
extern "C" const char sff1_pi[];
extern "C" const char sff2_pi[];
extern "C" const char staff_bgm[];
extern "C" const char sff1_cdg[];
extern "C" const char sff1b_cdg[];
extern "C" const char sff2_cdg[];
extern "C" const char sff2b_cdg[];
extern "C" const char sff3_cdg[];
extern "C" const char sff3b_cdg[];
extern "C" const char sff4_cdg[];
extern "C" const char sff4b_cdg[];
extern "C" const char sff5_cdg[];
extern "C" const char sff5b_cdg[];
extern "C" const char sff6_cdg[];
extern "C" const char sff6b_cdg[];
extern "C" const char sff7_cdg[];
extern "C" const char sff7b_cdg[];
extern "C" const char sff8_cdg[];
extern "C" const char sff8b_cdg[];
extern "C" const char sff9_cdg[];
extern "C" const char sff9b_cdg[];

/// Loads a credit image and its no-alpha companion into two consecutive slots.
#define staffroll_cdg_load(slot, fn, fn_noalpha) { \
	cdg_load_single(slot, fn, 0); \
	cdg_load_single_noalpha((slot + 1), fn_noalpha, 0); \
}

/// Shows the given .PI as the new background for everything the dissolve
/// effect unblits, on both pages.
#define staffroll_bg_put(fn) { \
	graph_accesspage(1); \
	pi_load(0, fn); \
	pi_palette_apply(0); \
	pi_put_8(0, 0, 0); \
	pi_free(0); \
	graph_copy_page(0); \
	bgimage_snap(); \
}

void near staffroll_animate(void)
{
	palette_settone(0);
	staffroll_bg_put(sff1_pi);

	snd_kaja_func(KAJA_SONG_STOP, 0);
	snd_load(staff_bgm, SND_LOAD_SONG);
	snd_kaja_func(KAJA_SONG_PLAY, 0);
	palette_black_in(12);

	staffroll_cdg_load(0, sff1_cdg, sff1b_cdg);
	snd_delay_until_measure(3, 64);
	cdg_slot = 0;
	dissolve_put_func = dissolve_put_radial;
	dissolve_in_animate(352, 160);

	staffroll_cdg_load(2, sff2_cdg, sff2b_cdg);
	snd_delay_until_measure(7, 160);
	dissolve_put_func = dissolve_put_diagonal;
	dissolve_out_animate(352, 160);
	cdg_slot = 2;
	dissolve_put_func = dissolve_put_horizontal;
	dissolve_in_animate(192, 128);

	graph_accesspage(0);
	cdg_slot = 0;
	staffroll_cdg_load(0, sff3_cdg, sff3b_cdg);
	snd_delay_until_measure(11, 160);
	dissolve_in_animate(288, 200);

	snd_delay_until_measure(19, 160);
	dissolve_put_func = dissolve_put_diagonal;
	dissolve_out_2_animate(192, 128, 288, 200);

	palette_black_out(4);
	cdg_free_all();
	staffroll_bg_put(sff2_pi);
	palette_black_in(4);

	staffroll_cdg_load(2, sff4_cdg, sff4b_cdg);
	snd_delay_until_measure(23, 160);
	cdg_slot = 2;
	dissolve_put_func = dissolve_put_horizontal;
	dissolve_in_animate(32, 112);

	cdg_free(2);
	staffroll_cdg_load(4, sff5_cdg, sff5b_cdg);
	snd_delay_until_measure(27, 160);
	cdg_slot = 4;
	dissolve_put_func = dissolve_put_diagonal;
	dissolve_in_animate(32, 184);

	staffroll_cdg_load(0, sff8_cdg, sff8b_cdg);
	snd_delay_until_measure(31, 160);
	dissolve_put_func = dissolve_put_horizontal;
	dissolve_out_animate(32, 184);
	cdg_slot = 0;
	dissolve_in_animate(64, 184);

	staffroll_cdg_load(4, sff9_cdg, sff9b_cdg);
	snd_delay_until_measure(35, 160);
	dissolve_put_func = dissolve_put_radial;
	dissolve_out_animate(64, 184);
	cdg_slot = 4;
	dissolve_in_animate(64, 184);

	staffroll_cdg_load(0, sff6_cdg, sff6b_cdg);
	snd_delay_until_measure(39, 160);
	dissolve_put_func = dissolve_put_diagonal;
	dissolve_out_animate(64, 184);
	cdg_slot = 0;
	dissolve_in_animate(32, 184);

	snd_delay_until_measure(43, 160);
	dissolve_put_func = dissolve_put_horizontal;
	dissolve_out_2_animate(32, 112, 32, 184);

	staffroll_cdg_load(0, sff7_cdg, sff7b_cdg);
	cdg_slot = 0;
	dissolve_in_animate(32, 336);

	snd_delay_until_measure(48, 160);
	bgimage_free();
	cdg_free_all();
	palette_black_out(4);
}
