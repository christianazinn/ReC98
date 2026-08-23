/// Stage 2 Boss - Meira, her second object
/// ---------------------------------------
/// Four of her danmaku patterns: phase 0's third, all three of phase 1's, and
/// the shared parameter block they seed for her still-ASM slash pool.
///
/// AND THE ONLY REASON THIS IS NOT th02/main/boss/b2.cpp IS ONE PAD BYTE.
/// `[measured 2026-08-23]` meira_update() over there compiles to a body plus a
/// `db 0` plus a four-entry jump table, and that pad is `-a2`'s: Turbo C++
/// emits it exactly when the table's natural offset in its OWN object is ODD
/// (kb/codegen/0096 + 0154 + 0160). These four procs are `0x3C3` bytes - odd -
/// so prepending them into b2.cpp moved that offset from odd to even, the pad
/// vanished, and every byte after it came out one early. obj_probe.py read the
/// object's segment length as 0x8D7 against a target of 0x8D8, and
/// meira_update's own published span shrank from 0x124 to 0x123, which is the
/// whole diagnosis.
///
/// SO THE PAD PROVES WHERE ZUN'S OBJECT BOUNDARY WAS, which is kb/codegen/0164
/// read forwards: an ODD table offset is impossible under `-a2`, therefore
/// meira_update()'s prefix in ZUN's own object was EVEN, therefore something
/// ends between meira_14C76() and meira_update(). b2.cpp's prefix is `0x2F8`
/// today and that is the parity it has to keep.
///
/// A consequence for whoever lifts the next group out of this block: it goes
/// HERE, not into b2.cpp, and this object has no parity to protect because it
/// emits no generated table at all. Check that before adding a fifth pattern -
/// a `switch` with four or more dense cases would give this object one, and
/// then kb/codegen/0157 starts applying to it too.

// -zC, because the segment name would otherwise come from this file's own
// basename and be B2M_TEXT (kb/codegen/0105). -zPmain_03 for the near calls
// that leave this segment. -G, because every prolog here is `push bp; mov bp,
// sp` with no locals rather than an `enter` (kb/codegen/0011). NO -a2, and that
// is the point of the file: nothing here emits a generated jump table, so this
// object has no alignment to pin and no parity to protect.
#pragma option -zCBOSS_5_TEXT -zPmain_03 -G

#include "platform.h"
#include "pc98.h"
#include "th02/main/playfld.hpp"
#include "th02/main/boss/boss.hpp"
#include "th02/main/bullet/bullet.hpp"
#include "th02/main/player/player.hpp"
#include "th02/snd/snd.h"
#include "th01/math/subpixel.hpp"

// The sprite the boss and midboss renderers blit, shared by all of them and
// written from ~150 sites across th02_main.asm. `patnum_2064E` is the dump's own
// spelling and is not an IDA placeholder; retiring the address suffix means
// ruling on all of those sites at once, which is its own parcel.
extern "C" int patnum_2064E;

// The five-byte rank-scaled parameter block every boss and midboss init
// function fills a contiguous prefix of, documented in full at
// th02/main/boss/b4.cpp and state/notes/th02-boss-rank-param.md.
extern "C" uint8_t boss_rank_param[5];

// Where she stands when she is not attacking, and the same expression
// meira_init() seeds [boss_left_on_page] with. Two of the patterns below walk
// her back to it a few pixels a frame and end when she arrives.
static const screen_x_t MEIRA_HOME_X = (
	PLAYFIELD_LEFT + (PLAYFIELD_W / 2) - 32
);

/// The parameter block of her still-ASM slash pool
/// -----------------------------------------------
/// meira_1469C() is a 40-slot pool with a 12-byte record, and three of its
/// fields come from globals rather than from its arguments. ALL THREE KEEP
/// ADDRESS-SUFFIXED HAND NAMES, which is `th02/main/boss/b4.cpp`'s
/// [marisa_1AA60] device and the same one [meira_250FE] above already uses.
/// The reason is stated rather than assumed: naming them needs meira_1469C()
/// and whatever renders that pool, neither of them is contiguous with this
/// block's tail, and every candidate name would be a guess about a record this
/// parcel cannot see. What IS measured about each of them is written down here
/// so the parcel that lifts the pool does not have to re-derive it.
///
/// They are aliases and not renames because four patterns above still read
/// them.

// A 0/1 flag, seeded at the start of three of her patterns from
// `(*boss_left_on_back_page + 16) < player_topleft.x` - i.e. from which side of
// her the player is standing on. Then used three ways at once: as a +/-1 mirror
// on four sprite numbers, as the clamp selector meira_1489C() picks its
// playfield edge with, and as the sign of the walk home in meira_14A39(). Its
// POLARITY is not consistent across those three, which is exactly why one word
// cannot name it.
extern "C" uint8_t meira_252E6;

// Goes into byte +9 of the pool record. `[measured]` Always
// `0x78 + meira_252E6` or `0x79 - meira_252E6`, so it is one of two adjacent
// values and the flag above only mirrors it.
extern "C" uint8_t meira_252E0;

// Goes into byte +0x0A of the pool record, as its low byte only. `[measured]`
// Seeded to 0x10, 0x14 or 0x50 at the start of a pattern and then stepped by
// +2 or -1 per spawn, so it is per-pattern and cumulative.
extern "C" int16_t meira_252E4;
/// -----------------------------------------------

// The group of the bursts her patterns fire, written by four of them and read
// through meira_1469C(). Its low byte only; the angle below is the high one.
extern "C" uint8_t meira_burst_group; // ACTUAL TYPE: bullet_group_or_special_motion_t

// The angle field of the burst parameter pair whose group half is
// [meira_burst_group]. `[inferred]`, and marked so: it lands in the pool record
// slot that every bullets_add_*() call in this binary fills with an angle, and
// the two values it is ever given are 0x40 and 0x30 - but nothing this parcel
// can reach uses it as one.
extern "C" uint8_t meira_burst_angle;

// The shared body of her four dash slashes: step her one way along x with a
// playfield clamp, step her down or up along y, and spawn one slash into
// meira_1469C()'s pool at her new centre. Still ASM, and published for this
// object because meira_14A39() and meira_14C76() below are its only callers.
extern "C" void pascal near meira_1489C(
	unsigned char leftward, int delta_x, int delta_y
);

/// Her two ramping pellet speeds
/// -----------------------------
/// One per pattern, and each is private to the one pattern that uses it - so
/// both are PLAIN RENAMES rather than kb/codegen/0123 aliases. `[measured]`
/// Both start at to_sp(2.0) and both add 11 subpixels per pellet, which is what
/// makes each pattern's stream accelerate; nothing resets them except the next
/// run of their own pattern.
extern "C" subpixel_t meira_252E8;
extern "C" subpixel_t meira_252EA;
/// -----------------------------


// Phase 0 pattern 2: two dash slashes away from the player, then a walk back to
// [MEIRA_HOME_X] that ends the pattern when she arrives - so this one has no
// fixed length, and a slash that leaves her near an edge makes it longer.
extern "C" void near meira_14A39(void)
{
	if(boss_phase_frame < 50) {
	} else if(boss_phase_frame == 50) {
		snd_se_play(9);
		patnum_2064E = 142;
		meira_burst_group = boss_rank_param[0];
	} else if(boss_phase_frame < 99) {
	} else if(boss_phase_frame == 99) {
		snd_se_play(3);
		patnum_2064E = (149 - meira_252E6);
		meira_252E0 = (0x78 + meira_252E6);
		meira_252E4 = 0x50;
	} else if(boss_phase_frame < 120) {
		meira_1489C(meira_252E6, -8, 8);
		meira_252E4--;
	} else if(boss_phase_frame < 136) {
		patnum_2064E = 142;
		patnum_2064E = (meira_252E6 + 144);
		meira_252E0 = (0x79 - meira_252E6);
		meira_252E4--;
		if(boss_phase_frame == 135) {
			snd_se_play(3);
		}
	} else if(boss_phase_frame < 156) {
		meira_1489C(meira_252E6, -8, -8);
		meira_252E4--;
	} else if(*boss_left_on_back_page != MEIRA_HOME_X) {
		patnum_2064E = 141;
		*boss_left_on_back_page += ((meira_252E6 == 0) ? 2 : -2);
	} else {
		boss_phase_frame = 0;
	}
}


// Phase 1 pattern 0: a stationary stream of pellets straight down, every other
// frame for 17 frames, accelerating as it goes.
extern "C" void near meira_14B33(void)
{
	if(boss_phase_frame < 50) {
	} else if(boss_phase_frame == 50) {
		snd_se_play(9);
		patnum_2064E = 142;
	} else if(boss_phase_frame < 99) {
	} else if(boss_phase_frame == 99) {
		snd_se_play(3);
		patnum_2064E = 143;
		meira_252E8 = to_sp(2.0f);
	} else if(boss_phase_frame < 116) {
		if((boss_phase_frame & 1) != 0) {
			bullets_add_pellet(
				(*boss_left_on_back_page + 24),
				(*boss_top_on_back_page + 32),
				0x40,
				boss_rank_param[1],
				meira_252E8
			);
			meira_252E8 += 11;
		}
	} else {
		patnum_2064E = 141;
		boss_phase_frame = 0;
	}
}


// Phase 1 pattern 1: the same accelerating stream as meira_14B33(), aimed
// straight up instead of down, and fired while she slides back towards
// [MEIRA_HOME_X] eight pixels a frame.
extern "C" void near meira_14BC2(void)
{
	if(boss_phase_frame < 50) {
	} else if(boss_phase_frame == 50) {
		snd_se_play(9);
		patnum_2064E = 142;
		meira_burst_group = BG_1_RANDOM_ANGLE;
	} else if(boss_phase_frame < 99) {
	} else if(boss_phase_frame == 99) {
		snd_se_play(3);
		patnum_2064E = 143;
		meira_252EA = to_sp(2.0f);
	} else if(boss_phase_frame < 120) {
		if((boss_phase_frame & 1) != 0) {
			bullets_add_pellet(
				(*boss_left_on_back_page + 24),
				(*boss_top_on_back_page + 32),
				0x00,
				boss_rank_param[2],
				meira_252EA
			);
			meira_252EA += 11;
		}
		if(*boss_left_on_back_page != MEIRA_HOME_X) {
			*boss_left_on_back_page += (
				(*boss_left_on_back_page < MEIRA_HOME_X) ? 8 : -8
			);
		}
	} else {
		patnum_2064E = 141;
		boss_phase_frame = 0;
	}
}


// Phase 1 pattern 2, and the longest pattern in her fight: FOUR dash slashes,
// one per diagonal, each 16 frames of dashing followed by 20 frames of recovery
// with a sound effect on the last of them. `[measured]` The four sprite pairs
// are 148/149, 149/148, 145/144 and 144/145, so every dash mirrors its
// neighbour and the whole run reads as one continuous figure.
extern "C" void near meira_14C76(void)
{
	if(boss_phase_frame < 50) {
	} else if(boss_phase_frame == 50) {
		// A BARE COMPARISON and not a `? 1 : 0`. `[measured]` Turbo C++
		// materialises a relational operator used as a value in the whole of
		// AX (`mov ax, 1` / `xor ax, ax`) and then stores AL; the ternary
		// narrows the same expression to `mov al, 1` / `mov al, 0` and comes
		// out one byte short.
		meira_252E6 = ((*boss_left_on_back_page + 16) < player_topleft.x);
		snd_se_play(9);
		patnum_2064E = 142;
		meira_burst_group = BG_1_RANDOM_ANGLE;
		meira_burst_angle = 0x30;
	} else if(boss_phase_frame < 99) {
	} else if(boss_phase_frame == 99) {
		snd_se_play(3);
		patnum_2064E = (meira_252E6 + 148);
		meira_252E0 = (0x78 + meira_252E6);
		meira_252E4 = 0x14;
	} else if(boss_phase_frame < 116) {
		meira_1489C(meira_252E6, -8, 8);
		meira_252E4 += 2;
	} else if(boss_phase_frame < 136) {
		// A DEAD STORE, and it is in the binary three more times below: every
		// recovery arm writes 142 and then immediately overwrites it with the
		// mirrored sprite it actually wants.
		patnum_2064E = 142;
		patnum_2064E = (149 - meira_252E6);
		meira_252E0 = (0x79 - meira_252E6);
		meira_252E4 = 0x14;
		if(boss_phase_frame == 135) {
			snd_se_play(3);
		}
	} else if(boss_phase_frame < 152) {
		meira_1489C(meira_252E6, 8, 8);
		meira_252E4 += 2;
	} else if(boss_phase_frame < 172) {
		patnum_2064E = 142;
		patnum_2064E = (145 - meira_252E6);
		meira_252E0 = (0x78 + meira_252E6);
		meira_252E4 = 0x14;
		if(boss_phase_frame == 171) {
			snd_se_play(3);
		}
	} else if(boss_phase_frame < 188) {
		meira_1489C(meira_252E6, 8, -8);
		meira_252E4 += 2;
	} else if(boss_phase_frame < 208) {
		patnum_2064E = 142;
		patnum_2064E = (meira_252E6 + 144);
		meira_252E0 = (0x79 - meira_252E6);
		meira_252E4 = 0x14;
		if(boss_phase_frame == 207) {
			snd_se_play(3);
		}
	} else if(boss_phase_frame < 224) {
		meira_1489C(meira_252E6, -8, -8);
		meira_252E4 += 2;
	} else {
		patnum_2064E = 141;
		boss_phase_frame = 0;
	}
}
