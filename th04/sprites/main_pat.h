#ifndef TH04_SPRITES_MAIN_PAT_H
#define TH04_SPRITES_MAIN_PAT_H

#include "th04/sprites/cels.h"

typedef enum {
	M4C_STILL,
	M4C_CAST_1,
	M4C_CAST_2,
	M4C_CAST_3,
	M4C_CELS,
} midboss4_cel_t;

#define REIMU_ORB_CELS 4
#define YUUKA6_CHASECROSS_CELS 4
#define MIDBOSSX_CELS 4

/// Pattern numbers for the super_*() functions.
/// Since super_entry_bfnt() doesn't take a "start patnum" parameter, the
/// order in which the files are loaded has to match the order here.
typedef enum {
	/// Stage-independent
	/// =================
	// mikod.bft
	// ---------
	PAT_EXPLOSION_BIG = 3,
	// ----------
	// miko32.bft
	// ----------
	PAT_ENEMY_KILL,
	PAT_ENEMY_KILL_last = (PAT_ENEMY_KILL + ENEMY_KILL_CELS - 1),
	PAT_CLOUD_BULLET16_BLUE = 20,
	PAT_CLOUD_BULLET16_BLUE_last = (PAT_CLOUD_BULLET16_BLUE + BULLET_CLOUD_CELS - 1),
	PAT_CLOUD_BULLET16_RED,
	// ------------------------
	// reimu16.bft / mari16.bft
	// ------------------------
	// `[measured]` the whole 28-37 run at once, by a census of every
	// [patnum_base] store in th04_main.asm's thirty-two shot control procs:
	// 0x1C in all sixteen Reimu ones, 0x1E in shot_reimu_a_* alone, 0x20 in
	// shot_reimu_b_* alone, 0x22 in all sixteen Marisa ones and 0x24 in
	// shot_marisa_b_* alone. Five two-cel sprites, which is exactly the gap
	// between this bank and PAT_OPTION_REIMU below. Marisa's two stay unnamed
	// until the parcel that lifts her patterns has read them.
	PAT_SHOT_REIMU = 28,
	PAT_SHOT_REIMU_last = (PAT_SHOT_REIMU + SHOT_CELS - 1),
	// Shottype A's option shot, which homes in on [homing_target].
	PAT_SHOT_REIMU_SUB_A,
	PAT_SHOT_REIMU_SUB_A_last = (PAT_SHOT_REIMU_SUB_A + SHOT_CELS - 1),
	// Shottype B's option shot, fired at a fixed angle to either side.
	PAT_SHOT_REIMU_SUB_B,
	PAT_SHOT_REIMU_SUB_B_last = (PAT_SHOT_REIMU_SUB_B + SHOT_CELS - 1),
	// 0x22, the value the census above found in all sixteen of Marisa's shot
	// control procs. Named by MATCH-TH04-MAIN-SHOT-MARISA-HEAD, the parcel
	// that lifted the first two of them.
	PAT_SHOT_MARISA,
	PAT_SHOT_MARISA_last = (PAT_SHOT_MARISA + SHOT_CELS - 1),
	// 0x24, the census's fifth bank. Shottype B's option shot, fired straight
	// up or at a random forward angle from an offset to either side; shottype
	// A has no option shot at all and spends its option on the laser instead.
	// `[measured]` by the same census and confirmed against all sixteen lifted
	// bodies: shot_marisa_b_l2..l9 are the only code in either dump that
	// reaches this bank, and shot_marisa_a_l2..l9 put PAT_SHOT_MARISA in every
	// shot they fire — which is the question MATCH-TH04-MAIN-SHOT-MARISA-HEAD
	// left open here, now answered by MATCH-TH04-MAIN-SHOT-MARISA-B.
	PAT_SHOT_MARISA_SUB_B,
	PAT_SHOT_MARISA_SUB_B_last = (PAT_SHOT_MARISA_SUB_B + SHOT_CELS - 1),
	// ----------
	// miko16.bft
	// ----------
	PAT_OPTION_REIMU = 38,
	PAT_OPTION_MARISA,
	PAT_HITSHOT,
	PAT_HITSHOT_last = (PAT_HITSHOT + HITSHOT_CELS - 1),
	PAT_ITEM,

	PAT_BULLET16_N_OUTLINED_BALL_WHITE = 52,
	PAT_BULLET16_N_OUTLINED_BALL_RED,
	PAT_BULLET16_N_OUTLINED_BALL_GREEN,
	PAT_BULLET16_N_OUTLINED_BALL_BLUE,
	PAT_BULLET16_N_STAR,
	PAT_BULLET16_N_BALL_BLUE,
	PAT_BULLET16_N_SMALL_BALL_YELLOW,
	PAT_BULLET16_N_CROSS_YELLOW,
	PAT_BULLET16_N_SMALL_BALL_RED,
	PAT_BULLET16_N_BALL_RED,
	PAT_BULLET16_N_HEART_BALL_RED,

	PAT_EXPLOSION_SMALL = 68,

	PAT_SHOT_LASER_RING = 70,
	PAT_SHOT_LASER_RING_last = (PAT_SHOT_LASER_RING + SHOT_CELS - 1),

	PAT_BULLET_ZAP,
	PAT_BULLET_ZAP_last = (PAT_BULLET_ZAP + BULLET_ZAP_CELS - 1),
	PAT_BULLET16_D,
	PAT_BULLET16_D_BLUE = PAT_BULLET16_D,
	PAT_BULLET16_D_BLUE_last = (PAT_BULLET16_D_BLUE + BULLET_D_CELS - 1),
	PAT_BULLET16_D_YELLOW,
	PAT_BULLET16_D_YELLOW_last = (PAT_BULLET16_D_YELLOW + BULLET_D_CELS - 1),

	PAT_DECAY_PELLET,
	PAT_DECAY_PELLET_last = (PAT_DECAY_PELLET + BULLET_DECAY_CELS - 1),
	PAT_DECAY_BULLET16,
	PAT_DECAY_BULLET16_last = (PAT_DECAY_BULLET16 + BULLET_DECAY_CELS - 1),
	/// =================

	PAT_STAGE = 128,

	/// Stage 2 – Kurumi
	/// ================
	// No source-file comment: nothing in the dump ties this patnum to a
	// specific .bmt/.bb?, and naming one would be a guess.
	//
	// Base of Kurumi's cel range. Unlike every other TH04 boss,
	// stage2_setup() seeds [boss.sprite] with 0 rather than an absolute
	// patnum, so kurumi_fg_render() adds this base on every blit.
	PAT_KURUMI = (PAT_STAGE + 18),
	/// ================

	/// Stage 4
	/// =======
	// st03b.bmt
	// ---------
	PAT_MIDBOSS4_STILL_LEFT = (PAT_STAGE + 28 + M4C_STILL),
	PAT_MIDBOSS4_STILL_LEFT_last = (PAT_MIDBOSS4_STILL_LEFT + M4C_CELS - 1),
	PAT_MIDBOSS4_STILL_RIGHT,
	PAT_MIDBOSS4_STILL_RIGHT_last = (PAT_MIDBOSS4_STILL_RIGHT + M4C_CELS - 1),
	// ---------
	/// =======

	/// Stage 4 – Reimu
	/// ===============
	// st03b.bbt
	// ---------
	// [inferred] The one cel range reimu_fg_render() animates: her
	// [boss.sprite] is an absolute patnum, and this exact value is the only
	// one it turns into a REIMU_FRAMES_PER_CEL run rather than blitting as a
	// single pose. What the four cels depict is not recoverable from the
	// binary.
	PAT_REIMU_ANIMATED = (PAT_STAGE + 8),

	PAT_REIMU_ORB_BLUE = (PAT_STAGE + 12),
	PAT_REIMU_ORB_BLUE_last = (PAT_REIMU_ORB_BLUE + REIMU_ORB_CELS - 1),
	PAT_REIMU_ORB_YELLOW,
	PAT_REIMU_ORB_YELLOW_last = (PAT_REIMU_ORB_YELLOW + REIMU_ORB_CELS - 1),
	// ---------
	/// ===============

	/// Stage 4 – Marisa
	/// ================
	// st03b22.bbt
	// -----------
	PAT_MARISA_BIT = (PAT_STAGE + 8),
	// -----------
	/// ================

	/// Stage 6
	/// =======
	// st05.bb1
	// --------
	PAT_YUUKA6_PARASOL_BACK_OPEN = 128,
	PAT_YUUKA6_PARASOL_BACK_HALFOPEN = 130,
	PAT_YUUKA6_PARASOL_BACK_HALFCLOSED = 132,
	PAT_YUUKA6_PARASOL_BACK_CLOSED = 134,
	// --------
	// st05.bb2
	// --------
	PAT_YUUKA6_PARASOL_LEFT_PULL = 136,
	PAT_YUUKA6_PARASOL_FORWARD_CLOSED = 138,
	PAT_YUUKA6_PARASOL_FORWARD_OPEN = 140,
	// --------
	// st05.bb3
	// --------
	PAT_YUUKA6_PARASOL_SHIELD_0 = 142,
	PAT_YUUKA6_PARASOL_SHIELD_1 = 144,
	PAT_YUUKA6_PARASOL_SHIELD_2 = 146,
	PAT_YUUKA6_PARASOL_SHIELD_3 = 148,
	// --------
	// st05.bb4
	// --------
	PAT_YUUKA6_PARASOL_LEFT = 150,
	PAT_YUUKA6_PARASOL_LEFT_FORWARD_PULL = 152,
	PAT_YUUKA6_PARASOL_SPIN_BACK_0 = 154,
	PAT_YUUKA6_PARASOL_SPIN_BACK_1 = 156,
	// --------
	// st05.bb5
	// --------
	PAT_YUUKA6_PARASOL_SPIN_BACK_2 = 158,
	PAT_YUUKA6_PARASOL_SPIN_BACK_3 = 160,
	PAT_YUUKA6_PARASOL_SPIN_BACK_4 = 162,
	PAT_YUUKA6_PARASOL_SPIN_BACK_5 = 164,
	// --------
	// st05.bb6
	// --------
	PAT_YUUKA6_PARASOL_SPIN_BACK_6 = 166,
	PAT_YUUKA6_PARASOL_SPIN_BACK_7 = 168,
	PAT_YUUKA6_PARASOL_SPIN_BACK_8 = 170,
	PAT_YUUKA6_PARASOL_SPIN_BACK_9 = 172,
	// --------
	// st05.bb7
	// --------
	PAT_YUUKA6_VANISH_0 = 174,
	PAT_YUUKA6_VANISH_1 = 176,
	PAT_YUUKA6_VANISH_2 = 178,
	PAT_YUUKA6_VANISH_3 = 180,
	// --------
	// st05.bb9
	// --------
	PAT_YUUKA6_CHASECROSS = 186,
	PAT_YUUKA6_CHASECROSS_last = (
		PAT_YUUKA6_CHASECROSS + YUUKA6_CHASECROSS_CELS - 1
	),
	// --------
	/// =======

	/// Extra Stage
	/// ===========
	// No source-file comment like the blocks above: nothing in the dump ties
	// this patnum range to a specific .bmt/.bb?, and naming one would be a
	// guess. Omitted deliberately, not forgotten.
	//
	// Idle animation of the Extra Stage midboss, cycled by midbossx_render()
	// through [stage_frame_mod16].
	PAT_MIDBOSSX = (PAT_STAGE + 20),
	PAT_MIDBOSSX_last = (PAT_MIDBOSSX + MIDBOSSX_CELS - 1),
	/// ===========

	/// Extra Boss 2 – Gengetsu
	/// =======================
	// st06.bb2
	// --------
	PAT_GENGETSU_TIPPING = PAT_STAGE,
	// --------
	/// =======================

	PAT_STAGE_last = 255,

	_main_patnum_t_FORCE_INT16 = 0x7FFF,
} main_patnum_t;

#endif /* TH04_SPRITES_MAIN_PAT_H */
