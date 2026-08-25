/// Stage loading and setup
/// -----------------------
/// Shared semantic root for TH04 and TH05. The games perform the same outer
/// operation, but differ in their character assets, debug/demo policy, and
/// stage-specific loaders, so those parts remain explicitly game-specific.

#include "platform.h"
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th03/formats/cdg.h"
#include "th04/main/null.hpp"
#include "th04/main/replay.hpp"
#include "th04/snd/snd.h"
#include "th04/sprites/main_cdg.h"
#if (GAME == 5)
	#include "th05/playchar.h"
	#include "th05/resident.hpp"
	#include "th05/sprites/main_pat.h"
#else
	#include "th04/playchar.h"
	#include "th04/resident.hpp"
#endif

extern unsigned char stage_id;
extern "C" unsigned char power;
extern nearfunc_t_near demo_update;
extern nearfunc_t_near overlay1;
extern nearfunc_t_near overlay2;
extern nearfunc_t_near stage_render;
extern nearfunc_t_near bg_render_not_bombing;
extern nearfunc_t_near bg_render_bombing_func;
extern unsigned char page_back;
extern unsigned char page_front;

extern "C" {
	extern bool16 stage_is_first;
	extern char far *bgmname;

	void pascal near bb_playchar_load(void);
	void pascal hud_put(void);
	void pascal near mpn_load(const char *fn);
	void pascal near tiles_fill_initial(void);
}

void near gameplay_init(void);
void near demo_load(void);
void near DemoPlay(void);
void near graph_both_pages_fill_col_1(void);
void near overlay_wipe(void);
void near overlay_black(void);
void near stage_init(void);
void near eyecatch_animate(void);
void midboss_reset(void);
void near bomb_bg_load__ems_preload_playchar_cdgs(void);
void near map_load(void);
void near std_load(void);
void near dialog_load(void);
void pascal near tiles_render_all(void);
void tiles_activate(void);
void pascal near overlay_stage_enter_update_and_render(void);

#if (GAME == 5)
	extern bool debug_mode_active;
	extern int pellet_bottom_col;

	extern "C" void pascal near bb_cheeto_load(void);
	void pascal near ems_preload_boss_faceset(const char *fn);
	extern "C" void near player_shot_level_update(void);
	void pascal near shinki_bg_render(void);
	void pascal near stage1_setup(void);
	void pascal near stage2_setup(void);
	void pascal near stage3_setup(void);
	void pascal near stage4_setup(void);
	void pascal near stage5_setup(void);
	void pascal near stage6_setup(void);
	void pascal near stagex_setup(void);
#else
	extern "C" uint16_t stage_setup_unused;
	void pascal far stage1_setup(void);
	void pascal far stage2_setup(void);
	void pascal far stage3_setup(void);
	void pascal far stage4_setup(void);
	void pascal far stage5_setup(void);
	void pascal far stage6_setup(void);
	void pascal far stagex_setup(void);
	void pascal near stage4_render(void);
#endif

extern "C" {
	extern const char EYE_RGB_FN[];
	extern const char PLAYCHAR_REIMU_BFNT_FN[];
	extern const char PLAYCHAR_MARISA_BFNT_FN[];
	extern const char MIKOD_BFNT_FN[];
	extern const char MIKO32_BFNT_FN[];
	extern const char MIKO16_BFNT_FN[];

	extern const char STAGE1_BOSS_FACESET_FN[];
	extern const char STAGE1_BFNT_FN[];
	extern const char STAGE1_MPN_FN[];
	extern const char STAGE2_BOSS_FACESET_FN[];
	extern const char STAGE2_BFNT_FN[];
	extern const char STAGE2_MPN_FN[];
	extern const char STAGE3_BOSS_FACESET_FN[];
	extern const char STAGE3_BFNT_FN[];
	extern const char STAGE3_MPN_FN[];
	extern const char STAGE4_BFNT_FN[];
	extern const char STAGE4_MPN_FN[];
	extern const char STAGE5_BOSS_FACESET_FN[];
	extern const char STAGE5_BFNT_FN[];
	extern const char STAGE5_MPN_FN[];
	extern const char STAGE6_BOSS_FACESET_FN[];
	extern const char STAGE6_BFNT_FN[];
	extern const char STAGEX_BOSS_FACESET_FN[];
	extern const char STAGEX_BFNT_FN[];
	extern const char STAGEX_MPN_FN[];

#if (GAME == 5)
	extern const char PLAYCHAR_MIMA_BFNT_FN[];
	extern const char PLAYCHAR_YUUKA_BFNT_FN[];
	extern const char REIMU16_BFNT_FN[];
	extern const char MARISA16_BFNT_FN[];
	extern const char MIMA16_BFNT_FN[];
	extern const char YUUKA16_BFNT_FN[];
	extern const char BOMB_SHAPE_FN[];
	extern const char BOMB_SHAPE_YUUKA_FN[];
	extern const char STAGE4_BOSS_FACESET_FN[];
#else
	extern const char STAGE1_REIMU_MPN_FN[];
	extern const char STAGE1_MARISA_MPN_FN[];
	extern const char STAGE4_REIMU_FACESET_FN[];
	extern const char STAGE4_MARISA_FACESET_FN[];
	extern const char STAGE6_MPN_FN[];
#endif
}

static const uint8_t POWER_MAX = 128;

#define nopcall_same_group(func) _asm { \
	nop; \
	push cs; \
	call near ptr func; \
}

#pragma option -a2

void near stage_setup(void)
{
	register int i;

	stage_is_first = false;
	vsync_Count2 = 0;
	stage_id = resident->stage;
	if(replay_stage_is_first(stage_id)) {
		stage_is_first = true;
		text_fillca(' ', (TX_BLACK | TX_REVERSE));
		demo_update = nullfunc_near;
		gameplay_init();

#if (GAME == 5)
		if(resident->debug) {
			resident->stage = resident->debug_stage;
			stage_id = resident->stage;
			power = resident->debug_power;
			resident->debug = false;
			debug_mode_active = true;
		}
#endif

		if(resident->demo_num != 0) {
			demo_load();
			stage_id = resident->stage = resident->demo_stage;
#if (GAME == 5)
			if(resident->demo_num != 5) {
				power = POWER_MAX;
			}
#else
			power = POWER_MAX;
			resident->stage_ascii = ('0' + stage_id);
#endif
			demo_update = reinterpret_cast<nearfunc_t_near>(DemoPlay);
			random_seed = 318;
		}
	}

	graph_both_pages_fill_col_1();
	graph_accesspage(0);
	graph_showpage(_AL);
	palette_entry_rgb(EYE_RGB_FN);
	palette_show();
	PaletteTone = 0;
	palette_show();
	graph_both_pages_fill_col_1();
	overlay_wipe();
	stage_init();
	nopcall_same_group(hud_put);
	eyecatch_animate();
	midboss_reset();

	if(stage_is_first) {
#if (GAME == 5)
		bb_cheeto_load();
#endif
		bomb_bg_load__ems_preload_playchar_cdgs();
		bb_playchar_load();

#if (GAME == 5)
		switch(playchar) {
		case PLAYCHAR_REIMU:
			super_entry_bfnt(PLAYCHAR_REIMU_BFNT_FN);
			break;
		case PLAYCHAR_MARISA:
			super_entry_bfnt(PLAYCHAR_MARISA_BFNT_FN);
			break;
		case PLAYCHAR_MIMA:
			super_entry_bfnt(PLAYCHAR_MIMA_BFNT_FN);
			break;
		case PLAYCHAR_YUUKA:
			super_entry_bfnt(PLAYCHAR_YUUKA_BFNT_FN);
			break;
		}
		super_entry_bfnt(MIKOD_BFNT_FN);
		super_entry_bfnt(MIKO32_BFNT_FN);

		switch(playchar) {
		case PLAYCHAR_REIMU:
			super_entry_bfnt(REIMU16_BFNT_FN);
			break;
		case PLAYCHAR_MARISA:
			super_entry_bfnt(MARISA16_BFNT_FN);
			break;
		case PLAYCHAR_MIMA:
			super_entry_bfnt(MIMA16_BFNT_FN);
			break;
		case PLAYCHAR_YUUKA:
			super_entry_bfnt(YUUKA16_BFNT_FN);
			break;
		}
		super_entry_bfnt(MIKO16_BFNT_FN);
		for(i = 12; i < TINY_MIKO16_END; i++) {
			super_convert_tiny(i);
		}
		if(playchar == PLAYCHAR_YUUKA) {
			super_entry_bfnt(BOMB_SHAPE_YUUKA_FN);
		} else {
			super_entry_bfnt(BOMB_SHAPE_FN);
		}
#else
		if(playchar == PLAYCHAR_REIMU) {
			super_entry_bfnt(PLAYCHAR_REIMU_BFNT_FN);
		} else {
			super_entry_bfnt(PLAYCHAR_MARISA_BFNT_FN);
		}
		super_entry_bfnt(MIKOD_BFNT_FN);
		super_entry_bfnt(MIKO32_BFNT_FN);
		super_entry_bfnt(MIKO16_BFNT_FN);
		for(i = 20; i < 120; i++) {
			super_convert_tiny(i);
		}
#endif
	}

#if (GAME == 5)
	nopcall_same_group(tiles_activate);
	pellet_bottom_col = 9;
#else
	bgmname[2] = '0';
	bgmname[3] = resident->stage_ascii;
#endif

	switch(stage_id) {
	case 0:
#if (GAME == 5)
		ems_preload_boss_faceset(STAGE1_BOSS_FACESET_FN);
#else
		// ZUN bloat: This word is assigned 9 in every valid stage arm and is
		// read nowhere in the binary or tree.
		stage_setup_unused = 9;
		cdg_load_all(CDG_FACESET_BOSS, STAGE1_BOSS_FACESET_FN);
		bgmname[2] = resident->playchar_ascii;
#endif
		super_entry_bfnt(STAGE1_BFNT_FN);
		stage1_setup();
#if (GAME == 5)
		mpn_load(STAGE1_MPN_FN);
#else
		if(resident->playchar_ascii == ('0' + PLAYCHAR_REIMU)) {
			mpn_load(STAGE1_REIMU_MPN_FN);
		} else {
			mpn_load(STAGE1_MARISA_MPN_FN);
		}
#endif
		break;

	case 1:
#if (GAME == 5)
		ems_preload_boss_faceset(STAGE2_BOSS_FACESET_FN);
#else
		stage_setup_unused = 9;
		cdg_load_all(CDG_FACESET_BOSS, STAGE2_BOSS_FACESET_FN);
#endif
		super_entry_bfnt(STAGE2_BFNT_FN);
		stage2_setup();
		mpn_load(STAGE2_MPN_FN);
		break;

	case 2:
#if (GAME == 5)
		ems_preload_boss_faceset(STAGE3_BOSS_FACESET_FN);
#else
		stage_setup_unused = 9;
		cdg_load_all(CDG_FACESET_BOSS, STAGE3_BOSS_FACESET_FN);
#endif
		super_entry_bfnt(STAGE3_BFNT_FN);
		stage3_setup();
		mpn_load(STAGE3_MPN_FN);
		break;

	case 3:
#if (GAME == 5)
		ems_preload_boss_faceset(STAGE4_BOSS_FACESET_FN);
#else
		stage_setup_unused = 9;
		if(playchar == PLAYCHAR_REIMU) {
			cdg_load_all(CDG_FACESET_BOSS, STAGE4_REIMU_FACESET_FN);
		} else {
			cdg_load_all(CDG_FACESET_BOSS, STAGE4_MARISA_FACESET_FN);
		}
#endif
		super_entry_bfnt(STAGE4_BFNT_FN);
		stage4_setup();
		mpn_load(STAGE4_MPN_FN);
#if (GAME == 4)
		stage_render = stage4_render;
#endif
		break;

	case 4:
#if (GAME == 5)
		ems_preload_boss_faceset(STAGE5_BOSS_FACESET_FN);
#else
		stage_setup_unused = 9;
		cdg_load_all(CDG_FACESET_BOSS, STAGE5_BOSS_FACESET_FN);
#endif
		super_entry_bfnt(STAGE5_BFNT_FN);
		stage5_setup();
		mpn_load(STAGE5_MPN_FN);
		break;

	case 5:
#if (GAME == 5)
		ems_preload_boss_faceset(STAGE6_BOSS_FACESET_FN);
		super_entry_bfnt(STAGE6_BFNT_FN);
		stage6_setup();
		bg_render_not_bombing = shinki_bg_render;
		bg_render_bombing_func = shinki_bg_render;
#else
		stage_setup_unused = 9;
		super_entry_bfnt(STAGE6_BFNT_FN);
		cdg_load_all(CDG_FACESET_BOSS, STAGE6_BOSS_FACESET_FN);
		stage6_setup();
		mpn_load(STAGE6_MPN_FN);
#endif
		break;

	case 6:
#if (GAME == 5)
		nopcall_same_group(player_shot_level_update);
		ems_preload_boss_faceset(STAGEX_BOSS_FACESET_FN);
#else
		stage_setup_unused = 9;
		super_entry_bfnt(STAGEX_BFNT_FN);
		cdg_load_all(CDG_FACESET_BOSS, STAGEX_BOSS_FACESET_FN);
#endif
#if (GAME == 5)
		super_entry_bfnt(STAGEX_BFNT_FN);
#endif
		stagex_setup();
		mpn_load(STAGEX_MPN_FN);
		break;
	}

	map_load();
	std_load();
	dialog_load();
	tiles_fill_initial();
	graph_accesspage(0);
	while(vsync_Count2 < 0x80) {
	}
	palette_black_out(1);
	PaletteTone = 100;
	palette_show();
	overlay_black();
	tiles_render_all();
	page_back = 1;
	page_front = 0;
	graph_accesspage(1);
	graph_showpage(0);
	tiles_render_all();

#if (GAME == 5)
	if((resident->demo_num == 0) || (resident->demo_num == 5)) {
		bgmname[2] = '0';
		bgmname[3] = ('0' + stage_id);
		snd_load(bgmname, SND_LOAD_SONG);
		snd_kaja_func(KAJA_SONG_PLAY, 0);
	}
#else
	if(resident->demo_num == 0) {
		snd_load(bgmname, SND_LOAD_SONG);
		snd_kaja_func(KAJA_SONG_PLAY, 0);
	}
	nopcall_same_group(tiles_activate);
#endif

	overlay1 = overlay_stage_enter_update_and_render;
	overlay2 = nullfunc_near;
}

#pragma option -a1


#undef nopcall_same_group
