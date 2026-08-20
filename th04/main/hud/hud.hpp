#include "th02/main/hud/hud.hpp"

#define BAR_MAX 128

// Rows
// ----
// Declared here rather than privately in each row's own translation unit,
// because hud_put() needs the same number to place the label or icon in front
// of a row that the row's own renderer needs to place the value. Every one of
// these used to be spelled out twice with the same value, and
// [HUD_POINT_ITEMS_STAGE_Y] had a third spelling, HUD_POINT_ITEMS_Y.
//
// "hud_put() draws at" below means th04/main/hud/hud.cpp for TH04, but
// th05_main.asm for TH05, whose hud_put() is still assembly — the `GAME == 5`
// coordinates were read back out of that dump's `gaiji_putca`/`gaiji_putsa`
// call sites, not assumed from TH04's.
//
// [HUD_POWER_Y] is deliberately NOT here: it has to stay a `#define` in
// th04/main/hud/power.cpp, because Turbo C++ 4.0J's __emit__() takes literal
// constants only and rejects a file-scope `static const` outright.
// (kb/codegen/0089)

// Both games put these two rows on the same lines. They used to be private to
// th04/main/hud/hud.cpp, which is where hud_put() places the label; the row's
// own renderer now lives in a different translation unit
// (th04/main/hud/bombs.cpp, th04/main/hud/lives.cpp) and needs the same number,
// which is exactly the condition this block exists for.
static const tram_y_t HUD_BOMBS_Y = 11;
static const tram_y_t HUD_LIVES_Y = 13;

#if (GAME == 5)
// Below the gsRUIKEI ("cumulative") label that hud_put() draws at (57, 15).
static const utram_y_t HUD_POINT_ITEMS_EXTEND_Y = 15;

// Below the gs_TEN label that hud_put() draws at (58, 16).
static const utram_y_t HUD_POINT_ITEMS_STAGE_Y = 16;

// Below the gs_YUME label that hud_put() draws at (63, 19).
static const utram_y_t HUD_DREAM_Y = 20;

// The row of the gs_TAMA label that hud_put() draws at (58, 18).
static const utram_y_t HUD_GRAZE_Y = 18;
#else
// Below the gs_TEN label that hud_put() draws at (58, 15).
static const utram_y_t HUD_POINT_ITEMS_STAGE_Y = 15;

// The row of the gs_YUME label that hud_put() draws at (58, 17).
static const utram_y_t HUD_DREAM_Y = 17;

// The row of the gs_TAMA label that hud_put() draws at (58, 19).
static const utram_y_t HUD_GRAZE_Y = 19;
#endif
// ----

// Low-level
// ---------

// Renders a HUD bar at ([HUD_LEFT], [y]), filled according to [value] out of
// BAR_MAX, in the given attribute. Assembly in TH05 (th05/hud_bar.asm), C++ in
// TH04 since th04/main/hud/bar_put.cpp was lifted, and `extern "C"` because
// both games spell the export as an undecorated, uppercased `HUD_BAR_PUT`.
#if (GAME == 5)
extern "C" void pascal near hud_bar_put(
	utram_y_t y,
	int value, // ACTUAL TYPE: unsigned char
	tram_atrb2 atrb
);
#else
extern "C" void pascal hud_bar_put(
	utram_y_t y,
	int value, // ACTUAL TYPE: unsigned char
	tram_atrb2 atrb
);
#endif

// Renders the "Enemy!!" caption and the HP bar, showing the given [bar_value]
// between 0 and BAR_MAX.
void pascal hud_hp_put(int bar_value);

#if (GAME == 5)
// Prints [points] using the bold gaiji font, right-aligned at
// 	([left+8], [y]),
// in white, using up to 7 digits (8 if the "continue" digit is included).
// Larger numbers will overflow the most significant digit into the A-Z range.
void pascal hud_points_put(
	utram_x_t left, utram_y_t y, unsigned long points
);

// Prints [val] using the bold gaiji font, right-aligned at
// 	([left+8], [y]),
// with the given attribute.
void pascal hud_5_digit_put(
	utram_x_t left, utram_y_t y, uint16_t val, tram_atrb2 atrb
);
#else
// Prints [val] using the bold gaiji font, right-aligned at
// 	([left+8], [y]),
// in white. Shares nothing but the name and the semantics with TH05's
// function above: TH04 builds the digits in a stack buffer and hardcodes
// TX_WHITE, TH05 uses the shared [hud_gaiji_row] and takes the attribute as a
// parameter.
void pascal hud_5_digit_put(utram_x_t left, utram_y_t y, uint16_t val);
#endif
// ---------

// High-level
// ----------

// Renders the HP bar at the fraction of ([hp_cur] / [hp_max]), or instead
// fills up the bar by a single fill step if its previous value was lower.
void pascal near hud_hp_update_and_render(int hp_cur, int hp_max);

static const int HUD_HP_FILL_FRAMES = BAR_MAX;

// Renders both the current and the high score, including the continues digit.
// Assembly in both games (th04/main/scoreupd.asm).
extern "C" void pascal near hud_score_put(void);

// Displays the remaining bombs, and the remaining *spare* lives
// ([rem_lives] - 1), as a tally of up to [HUD_LABELED_W] / GAIJI_TRAM_W gaiji
// padded out with g_EMPTY -- or, once the count no longer fits that row, as a
// `　　×　　` label with the number written over its last two cells. Assembly
// in TH05; TH04's are th04/main/hud/bombs.cpp and th04/main/hud/lives.cpp.
// The two are NOT one shape: they differ in statement order and in whether
// the guard is signed, so each is written out separately.
extern "C" void pascal hud_bombs_put(void);
extern "C" void pascal hud_lives_put(void);

// Displays the point item counter(s).
extern "C" void pascal hud_point_items_put(void);

// Displays the dream row: TH04's per-point-item score, TH05's dream meter.
extern "C" void pascal hud_dream_put(void);

// Displays [stage_graze] in the graze row.
void hud_graze_put();

// Displays [power] as a bar.
extern "C" void pascal hud_power_put(void);

// Renders the entire HUD, reflecting all current values.
extern "C" void pascal hud_put(void);
// ----------
