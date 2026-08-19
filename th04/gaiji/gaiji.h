// ReC98
// -----
// Gaiji available to TH04 and TH05

#include "th02/gaiji/from_2.h"

#define OVERLAY_FADE_CELS 8
#define RETURN_KEY_CELS 4u
#define STAFF_FADE_CELS 8

typedef enum {
	g_NULL = '\0',
	g_EMPTY = 0x02,
	gs_NOTES, // ♫

	gs_HEART_2 = 0x06, // 🎔 (duplicated)
	gs_EXCLAMATION, // !
	gs_QUESTION, // ?
	gs_SWEAT, // 💦
	gs_DOUBLE_EXCLAMATION, // ‼
	gs_EXCLAMATION_QUESTION, // ⁉

#if (GAME == 5)
	ga_RETURN_KEY = 0x1C,
	ga_RETURN_KEY_last = (ga_RETURN_KEY + RETURN_KEY_CELS - 1),
#endif

	gaiji_bar(0x20),

	// A completely filled, 128-pixel bar, stored in 8 consecutive gaiji
	// characters. TH05 has *MAX♡ drawn on the last three.
	g_BAR_MAX_0,
	g_BAR_MAX_1,
	g_BAR_MAX_2,
	g_BAR_MAX_3,
	g_BAR_MAX_4,
	g_BAR_MAX_5,
	g_BAR_MAX_6,
	g_BAR_MAX_7,

	g_OVERLAY_FADE,
	g_OVERLAY_FADE_last = (g_OVERLAY_FADE + OVERLAY_FADE_CELS - 1),

#if (GAME == 5)
	// A second brightness ramp, structurally identical to the one above and
	// ordered the same way — g_STAFF_FADE fully covers the pixel behind it,
	// g_STAFF_FADE_last barely darkens it. Only TH05's staff roll uses these,
	// as the curtain credit_fade_put() (th05/space.cpp) fades each credit
	// image in and out behind.
	//
	// [inferred] The cel artwork was not inspected; the range is read off the
	// two literals in that function, 0x98 and 0x9F, and off the fact that
	// nothing between 0x40 and 0x9F is named.
	g_STAFF_FADE = 0x98,
	g_STAFF_FADE_last = (g_STAFF_FADE + STAFF_FADE_CELS - 1),
#endif

	gaiji_boldfont(0xA0),
	gs_DOT = 0xC4,
	gaiji_symbols_th02(0xC9),
	gs_BOMB = 0xD3, // ◉? ⦿? 🎯? 🖸? Or simply 💣?
	gs_YINYANG, // ☯
	gs_END, // "End"
	gs_TEN = 0xE6, // 点
	gs_YUME, // 夢
	gs_TAMA, // 弾
	gs_ALL, // "All"
	g_HISCORE_STAGE_EMPTY = 0xEF,
	g_NONE = 0xFF,
} gaiji_th04_t;
