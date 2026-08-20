#ifndef TH02_SPRITES_MAIN_PAT_H
#define TH02_SPRITES_MAIN_PAT_H

#define MISS_ANIM_CELS 6
#define LASER_CHARGE_CELS 4

/// Sprite sizes
/// ------------

#define FACE_W 48
#define FACE_H 48

#define DIALOG_BOX_PART_W 32
#define DIALOG_BOX_PART_H 32

#define DIALOG_BOX_FACE_BORDER 8

#define DIALOG_BOX_LEFT_W ( \
	(DIALOG_BOX_FACE_BORDER * 2) + FACE_W + DIALOG_BOX_PART_W \
)

#define DIALOG_BOX_LEFT_PARTS (DIALOG_BOX_LEFT_W / DIALOG_BOX_PART_W)
/// ------------

/// Stage-independent pattern numbers for the super_*() functions
/// -------------------------------------------------------------
typedef enum {
	// miko.bft
	// --------
	PAT_PLAYCHAR_STILL = 0,
	PAT_PLAYCHAR_LEFT = 2,
	PAT_PLAYCHAR_RIGHT,
	PAT_PLAYCHAR_MISS,
	PAT_PLAYCHAR_MISS_last = (PAT_PLAYCHAR_MISS + MISS_ANIM_CELS - 1),

	// 32×32 charge animation of the vertical boss lasers, indexed by
	// laser_t::charge_cel. This file is the only place that ties it to a
	// source file: gameplay_init() enters miko.bft, miko32.bft and miko16.bft
	// back to back from patnum 0, and the miko32.bft block below starts at 34,
	// so miko.bft has to be exactly 34 patterns wide and 30…33 are its tail.
	PAT_LASER_CHARGE = 30,
	PAT_LASER_CHARGE_last = (PAT_LASER_CHARGE + LASER_CHARGE_CELS - 1),
	// --------
	// miko32.bft
	// ----------
	PAT_DIALOG_BOX_LEFT_TOP = 34,
	PAT_DIALOG_BOX_LEFT_TOP_last = (
		PAT_DIALOG_BOX_LEFT_TOP + DIALOG_BOX_LEFT_PARTS - 1
	),
	PAT_DIALOG_BOX_MIDDLE_TOP,
	PAT_DIALOG_BOX_RIGHT_TOP,

	PAT_DIALOG_BOX_LEFT_BOTTOM = 42,
	PAT_DIALOG_BOX_LEFT_BOTTOM_last = (
		PAT_DIALOG_BOX_LEFT_BOTTOM + DIALOG_BOX_LEFT_PARTS - 1
	),
	PAT_DIALOG_BOX_MIDDLE_BOTTOM,
	PAT_DIALOG_BOX_RIGHT_BOTTOM,
	// ----------
	// miko16.bft
	// ----------
	PAT_ITEM_POWER = 68,
	PAT_ITEM_POINT,
	PAT_ITEM_BOMB,
	PAT_ITEM_BIGPOWER,
	PAT_OPTION_A,
	PAT_OPTION_B,
	PAT_OPTION_C,

	PAT_BULLET16_OUTLINED_BALL_BEIGE = 80,
	PAT_BULLET16_OUTLINED_BALL_RED,
	PAT_BULLET16_OUTLINED_BALL_GREEN,
	PAT_BULLET16_OUTLINED_BALL_PURPLE, // unused
	PAT_BULLET16_NOROI, // 呪
	PAT_BULLET16_STAR,

	// Lavender-orange in the Five Magic Stones and Evil Eye Σ fights, blue in
	// the Mima fight.
	PAT_BULLET16_BALL,

	PAT_ITEM_1UP,

	// Base of the 16×16 cap at the top of a vertical boss laser's beam. The
	// rendered pattern is this plus laser_t::phase, which is ≥ 1 whenever the
	// cap is drawn, so this slot itself is never blitted.
	PAT_LASER_HEAD = 91,

	PAT_BULLET16_BILLIARD_BALL_RED = 122,
	PAT_BULLET16_BILLIARD_BALL_PURPLE,
	// ----------

	// INT16, not UINT8, and it is `[measured]` from ZUN's own call sites
	// rather than assumed. Under -b- Turbo C++ 4.02 gives an enum the
	// narrowest type its values fit, and -3 merges a pair of adjacent
	// constant `pascal` arguments into one 32-bit push ONLY when both formals
	// are word-sized (kb/codegen/0125, and the probe in
	// state/notes/marisa_1BC43.md). th02_main.asm therefore measures the
	// width directly, twice over and in opposite directions:
	//
	//	@bullets_add_pellet$qiiucuci   73 call sites, formals (…, uc, i),
	//	                               NEVER merged
	//	@bullets_add_16x16$…13main_patnum_ti
	//	                               29 call sites, formals
	//	                               (…, main_patnum_t, i), ALWAYS merged
	//
	// The two differ in exactly one formal, so a byte-wide [main_patnum_t]
	// cannot produce the second shape. The byte-wide SLOTS that used to be
	// the reason for the UINT8 force ([bullet_t::patnum], [ITEM_PATNUM],
	// [player_option_patnum]) are spelled `uint8_t` with an ACTUAL TYPE
	// comment instead, the way th02/main/player/shot.hpp already spells its
	// own. TH04's and TH05's copies of this header force INT16 already.
	_main_patnum_t_FORCE_INT16 = 0x7FFF
} main_patnum_t;
/// -------------------------------------------------------------

#endif /* TH02_SPRITES_MAIN_PAT_H */
