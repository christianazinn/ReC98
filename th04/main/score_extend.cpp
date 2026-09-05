/// Extends
/// -------
/// One frame of the score-based extend check: awards a life the first time the
/// score passes each of five thresholds, in order.
///
/// (#included from th04/main/hud/points.cpp, at the very FRONT of it, ahead of
/// score_reset.cpp, lives.cpp, bombs.cpp and that file's own function -- the
/// address order all five bodies have in HUD_PNT_TEXT. This proc, its one-byte
/// alignment pad and its generated jump table were the last things
/// th04_main.asm contributed to that segment, so the object grows backwards
/// into the hole and every byte above it keeps its address
/// (kb/codegen 0099 + 0112 + 0114).
///
/// It is the first row of state/re/JUMP_TABLE_TAILS.md's class to land in
/// TH04: the harness queue tool files a segment whose last emitting item is a
/// `dw offset loc_...` run under TAIL=`data` and drops it out of every
/// liftable section, but that table is not independent data -- it is this
/// function's own codegen, and lifting the function emits it.
/// kb/codegen/0129 is why this is a separate file rather than more of
/// points.cpp: this translation unit's header closure has unguarded members,
/// so a prepended body has to arrive as an #include and spell what it needs.)
///
/// Assembly in TH05, and NOT the same function: th05_main.asm's proc in this
/// role (sub_16F05) extends off [extend_point_items_collected] rather than off
/// the score, with no switch and no table. Related idea, different body.

// kb/codegen/0011, derived from this function's own prolog and not from a
// neighbour: the original opens `push bp / mov bp, sp / sub sp, 2` and closes
// `leave / retn`, which is -G. Restored to -G- at the end of this file, so
// that score_reset.cpp below is unaffected.
//
// -a2 is what emits the one padding byte between this function's epilogue and
// its generated jump table, and here it needs NO object prefix to do it.
// Restored at the end of this file as well.
//
// `[measured]` with tcc -S over this object, and it is worth stating because
// it is the opposite of what costing this row from kb/codegen/0154 predicts.
// This body is 0x9F bytes and it is the first thing the object emits, so the
// natural table offset is 0x9F, ODD -- the case 0154's table says -a2 leaves
// alone. It does not: -a2 emits `db 1 dup (0)` here at a zero prefix, and
// emits NOTHING once a one-byte #pragma codestring makes the offset even.
// Both parities were probed, and the pad follows the odd one.
//
// So no `retn` had to be borrowed off @orange_backdrop_colorfill$qv above,
// and this lift costs no codestring and no second translation unit's byte.
// What that does to 0154's rule is a question for whoever hits the next one:
// this parcel measured its own object and took the parity that reproduced the
// dump, which is the only thing either entry is for. What both entries agree
// on, and what actually matters, is kb/codegen/0119 -- get the parity wrong
// and every function body is still byte-identical while the pad silently
// evaporates, which a per-function funcdiff cannot see. Diff the whole
// segment and the map's contribution length.
#pragma option -G -a2

// Every one of these is published by a root-dump BSS or data block, or by a
// header this translation unit cannot reach, for the reason
// th04/main/score_reset.cpp gives for its own such block.
//
// [score] itself needs nothing -- th04/score.h is already in points.cpp's
// closure through th04/resident.hpp.
// ---------------------------------------------------------------------
extern unsigned char extends_gained;
extern unsigned char bullet_clear_time;

// th04/main/hud/overlay.hpp IS in this translation unit now, ahead of this
// file: MATCH-TH04-MAIN-HUD-PNT-DRAIN gave th04/gaiji/gaiji.h an include
// guard, which is the only thing that made that header a hard error here, and
// th04/main/boss/b4m_fg.cpp above reaches it through
// th04/main/boss/boss.hpp. So [overlay_popup_id_new] is its real byte-sized
// `popup_id_t` rather than the storage type this file used to spell, and
// POPUP_ID_EXTEND below is the header's own enumerator rather than a mirrored
// 1. The store is the same byte either way -- `_popup_id_t_FORCE_UINT8` is
// what keeps the enum byte-sized.
//
// [overlay2] and overlay_popup_update_and_render() keep their declarations
// here even so, DELIBERATELY: the header declares the function inside a
// `#pragma codeseg HUD_OVRL_TEXT main_01` block, and whether that changes how
// its address is taken is not something this parcel measured. The two
// declarations are identical to the header's, so having both is legal and
// costs nothing.
extern nearfunc_t_near overlay2;
void pascal near overlay_popup_update_and_render(void);

// `extern "C"` because th04/main/playperf.asm exports the undecorated,
// Pascal-cased PLAYPERF_RAISE, exactly as th04/main/playperf.hpp says --
// another header in the unreachable set above. Reached through the island
// below rather than by a plain call.
extern "C" void pascal playperf_raise(char delta);

// Defined further down in THIS translation unit, by th04/main/hud/lives.cpp.
// Declared here because the island below has to name it before then.
extern "C" void pascal hud_lives_put(void);

// `extern "C"` and `pascal` are both what th02/snd/snd.h says; this
// translation unit does not reach that header either.
extern "C" void pascal snd_se_play(int new_se);
// ---------------------------------------------------------------------

// Turbo C++ compiled ZUN's far calls to same-code-group functions as
// `nop; push cs; call near ptr`, which no plain C++ far call reproduces
// (kb/codegen/0014, kb/codegen/0083). Same spelling as
// th04/main/player/miss.cpp's and th04/main/continue.cpp's; #undef'd below,
// because this file shares a translation unit with three others.
#define nopcall_same_group(func) _asm { \
	nop; \
	push	cs; \
	call	near ptr func; \
}

/// The five thresholds
/// -------------------
/// [score] is a little-endian BCD digit array whose element 0 is
/// [continues_used], so element 7 is the millions digit and element 6 the
/// hundred-thousands one. The five cases are therefore 300,000 / 800,000 /
/// 1,500,000 / 2,200,000 / 3,000,000, and [extends_gained] indexes them.
///
/// ZUN quirk: cases 0 and 1 look at the hundred-thousands digit ALONE. A run
/// that crosses 1,000,000 while still on case 1 has to reach x,800,000 for its
/// second extend, because 1,200,000 leaves element 6 at 2. The millions digit
/// only enters the test from case 2 onwards.
extern "C" void pascal near score_extend_update_and_render(void)
{
	// [bp-1], with the odd byte at [bp-2] left as padding (kb/codegen/0010).
	unsigned char extend;

	extend = 0;
	switch(extends_gained) {
	case 0:
		if(score.digits[6] >= 3) {
			extend = 1;
		}
		break;
	case 1:
		if(score.digits[6] >= 8) {
			extend = 1;
		}
		break;
	case 2:
		if((score.digits[7] >= 1) && (score.digits[6] >= 5)) {
			extend = 1;
		}
		break;
	case 3:
		if((score.digits[7] >= 2) && (score.digits[6] >= 2)) {
			extend = 1;
		}
		break;
	case 4:
		if(score.digits[7] >= 3) {
			extend = 1;
		}
		break;
	}
	if(!extend) {
		return;
	}

	// `__emit__(0x6A, 4)` rather than a call argument or `_asm push 4`: the
	// argument is pushed AHEAD of the island in the original, and the inline
	// assembler is free to pick the 3-byte `68 imm16` form for a `push` it
	// assembles itself (kb/codegen 0083 + 0089). Same shape as
	// th04/main/player/miss.cpp's playperf_lower() call.
	__emit__(0x6A, 4);
	nopcall_same_group(playperf_raise);

	// Counted even when the life itself is refused below, so the next
	// threshold is still the one this run has not reached yet.
	extends_gained++;

	// Nested rather than an early exit on the negated condition, and that
	// is codegen rather than style: the original loads the [resident] far
	// pointer ONCE and keeps ES:BX live across the branch. An early return
	// ends the basic block and makes Turbo C++ reload it for the increment,
	// which is 4 bytes the original does not have (kb/codegen/0002).
	if(resident->rem_lives <= 99) {
		resident->rem_lives++;
		if(bullet_clear_time < 20) {
			bullet_clear_time = 20;
		}
		nopcall_same_group(hud_lives_put);
		overlay_popup_id_new = POPUP_ID_EXTEND;
		overlay2 = overlay_popup_update_and_render;
		snd_se_play(7);
	}
}

#undef nopcall_same_group

// Back to the translation unit's baseline, so that score_reset.cpp,
// lives.cpp, bombs.cpp and points.cpp's own function are unaffected. (-G is
// set again by lives.cpp for its own two bodies.)
#pragma option -G- -a-
