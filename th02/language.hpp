#ifndef TH02_LANGUAGE_HPP
#define TH02_LANGUAGE_HPP

#include "platform.h"
// Patch-owned presentation preference. It deliberately stays outside replay,
// resident, score-file, configuration, Practice, and simulation state.
enum t2_language_preference_t {
	T2LANG_JAPANESE = 0,
	T2LANG_ENGLISH = 1,
};

// Each executable can independently validate the on-disk preference before its
// first language-dependent presentation. Invalid files always select Japanese.
void far t2_language_load(void);
t2_language_preference_t far t2_language_get(void);

// OP's Language option uses this transactional writer. It stays separate from
// replay, resident, score-file, and gameplay state.
bool far t2_language_set(t2_language_preference_t preference);

// T2EN.DAT is presentation-only. Its directory must have the exact audited
// shape before any later scoped packfile transaction may select English.
bool far t2_language_overlay_valid(void);
bool far t2_language_english_ready(void);

// OP-only presentation wrappers. Each English resource load is a complete
// overlay transaction and restores the stock archive before returning.
int far pascal t2_language_pi_load(int slot, const char *fn);
int far pascal t2_language_gaiji_entry_bfnt(const char *fn);
int far pascal t2_language_file_ropen(const char *fn);
void far pascal t2_language_file_close(void);
void far pascal t2_language_option_text(char *label, char *value);

// MAIN-only presentation wrappers. EYE.PI and MIKOFT.BFT each use complete
// transactions; dialogue text keeps the selected archive active until close.
int far pascal t2_language_main_pi_load(int slot, const char *fn);
int far pascal t2_language_main_gaiji_entry_bfnt(const char *fn);
int far pascal t2_language_main_file_ropen(const char *fn);
void far pascal t2_language_main_file_close(void);

// MAINE-only presentation wrappers. The eligible ending assets are loaded
// through T2EN.DAT as one balanced transaction; every other file stays in the
// Japanese stock archive.
int far __cdecl t2_language_maine_pi_load(int slot, const char *fn);
int far pascal t2_language_maine_gaiji_entry_bfnt(const char *fn);
int far pascal t2_language_maine_file_ropen(const char *fn);
void far pascal t2_language_maine_file_close(void);

// OP's executable-resident presentation tables are selected independently
// from packfile routing. The Japanese pointers remain the stock defaults.
void far t2_language_op_tables_apply(void);

enum t2_language_op_bridge_func_t {
	T2LOB_OPTION_SHADOW,
	T2LOB_OPTION_PUT,
	T2LOB_BGM_RESTART,
	T2LOB_OPTION_RESET,
	T2LOB_CFG_SAVE,
	T2LOB_START_INIT,
	T2LOB_START_GAME,
	T2LOB_START_EXTRA,
	T2LOB_START_DEMO,
	T2LOB_SHOTTYPE_MENU,
	T2LOB_SCORE_MENU,
	T2LOB_MUSICROOM_MENU,
	T2LOB_TITLE_BG_LOAD,
};

// Patch-owned OP segments are outside OP_01_TEXT. Route calls to OP's stock
// near functions through this far entry point in the original code segment.
void far pascal t2_language_op_bridge(
	t2_language_op_bridge_func_t func, int sel, int value
);
void far pascal t2_language_option_update_and_render(void);

#endif /* TH02_LANGUAGE_HPP */
