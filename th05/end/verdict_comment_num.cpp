/// The verdict screen's second comment line, chosen
/// ------------------------------------------------
/// `_ude.txt` is a flat array of 30-byte records. The verdict screen picks
/// two of them: the first from [skill] alone, inline in the still-ASM screen
/// body, and the second from this function, whose return value the caller
/// scales by 30 and offsets by 780 — i.e. this is a record index into the
/// file's SECOND block of comments, which starts at record 26.
///
/// `[measured]` This was the SECOND proc of th05_maine.asm's SCORE_TEXT
/// block, and it follows skill_apply_and_graph_guts() out of the head into
/// the tail of th05/regist.cpp's contribution, keeping its original order
/// relative to it (kb/codegen/0098 + 0114). No carve, no new segment, no
/// group-list edit and no Tupfile.lua line.
///
/// `[measured]` TH04 has no counterpart at all: its verdict screen renders a
/// single `_ude.txt` line whose record it picks inline. This is TH05-only.
///
/// No `#include`s of its own; th05/regist.cpp's chain already provides
/// th05/resident.hpp, th01/rank.h and th04/end/end.h (all via
/// th04/hiscore/regist_menu.cpp).

/// `[measured]` The original keeps a BP frame for a function with neither
/// parameters nor locals, which the build's global `-k-` would otherwise drop
/// — four bytes, and every following byte of the segment with them. Wrapped
/// per kb/codegen/0042 rather than by splitting the translation unit
/// (kb/codegen/0081), and restored immediately after so that nothing appended
/// to this chain inherits the frame silently.
#pragma option -k.

unsigned char near verdict_comment_2_num(void)
{
	if(
		(resident->end_sequence == ES_GOOD) ||
		(resident->end_sequence == ES_EXTRA)
	) {
		if(resident->rank == RANK_EASY) {
			return 4;
		}
		// `[measured]` The second-highest LEBCD digit of the run's final
		// score, i.e. a "did you even break 80 million" test.
		if(resident->score_last.digits[7] <= 7) {
			return 1;
		}
		if(resident->miss_count >= 6) {
			return 7;
		}
		if(resident->unknown >= 15) {
			return 8;
		}
		return 0;
	}
	if(resident->end_sequence == ES_BAD) {
		return 2;
	}
	// `[measured]` Signed, and it has to be: both fields are `unsigned char`,
	// so both promote to `int` before the comparison, and the original's `jg`
	// says so.
	if(resident->bombs_used <= (resident->miss_count * 2)) {
		return 5;
	}
	if((resident->stage >= 4) && (resident->point_items_collected <= 350)) {
		return 6;
	}
	return 3;
}

#pragma option -k-
