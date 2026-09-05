/// TH05's high score name registration screen
/// ------------------------------------------
/// Both pragmas are mandatory and both have to precede every emitted byte
/// (kb/codegen/0138), which is why they sit above the includes:
///
/// • -zC, because this file is listed in Tupfile.lua by full path with no
///   wrapper, so kb/codegen/0105's default would name its code segment
///   REGIST_TEXT. It has to be SCORE_TEXT: this object sits immediately
///   before th05_maine.asm in the TH05 MAINE.EXE link order and immediately
///   after th05/hi_end.cpp's own SCORE_TEXT contribution, so anything it
///   emits into that segment lands exactly at the head of the root dump's
///   SCORE_TEXT block. A kb/codegen/0098 head lift: no carve, no new
///   segment, no group-list edit, no Tupfile.lua line.
/// • -zP, because glyphball_spawn() and glyphballs_update_and_render() both
///   dispatch through `jmp cs:` switch tables, and without the group the
///   compiler frames those displacements on SCORE_TEXT instead of group_01
///   (kb/codegen/0104). th05_maine.asm declares
///   `group_01 group CUTSCENE_TEXT, maine_01_TEXT, SCORE_TEXT, maine_01__TEXT`.
#pragma option -zCSCORE_TEXT -zPgroup_01

#include "th04/language_overlay.hpp"
#include "th01/math/subpixel.hpp"
#include "th04/math/motion.hpp"
#include "th04/gaiji/gaiji.h"
#include "th04/formats/scoredat/scoredat.hpp"
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/hardware/frmdelay.h"
#include "th02/hiscore/regist.h"
#include "th02/snd/snd.h"
#include "th04/hardware/bgimage.hpp"
#include "th04/math/vector.hpp"
#include "th05/playchar.h"

/// Pattern numbers for the super_*() functions
/// -------------------------------------------
static const int GLYPHBALL_CELS = 4;

typedef enum {
	// sctm0.bft
	// ---------
	PAT_GLYPHBALL_CLOUD = 20,
	PAT_GLYPHBALL_CLOUD_last = (PAT_GLYPHBALL_CLOUD + GLYPHBALL_CELS - 1),
	PAT_GLYPHBALL_SPLASH,
	PAT_GLYPHBALL_SPLASH_last = (PAT_GLYPHBALL_SPLASH + GLYPHBALL_CELS - 1),
	// ---------
	// sctm1.bft
	// ---------
	PAT_GLYPHBALL,
	PAT_GLYPHBALL_last = (PAT_GLYPHBALL + GLYPHBALL_CELS - 1),
	// ---------
} regist_patnum_t;
/// -------------------------------------------

/// Floating balls carrying the individual name glyphs
/// --------------------------------------------------
#define GLYPHBALL_CLOUD_SPLASH_W 32
#define GLYPHBALL_CLOUD_SPLASH_H 32
#define GLYPHBALL_W 16
#define GLYPHBALL_H 16

#if (GLYPHBALL_CLOUD_SPLASH_W < GLYPHBALL_W)
#error Original code assumes GLYPHBALL_CLOUD_SPLASH_W >= GLYPHBALL_W
#endif
#if (GLYPHBALL_CLOUD_SPLASH_H < GLYPHBALL_H)
#error Original code assumes GLYPHBALL_CLOUD_SPLASH_H >= GLYPHBALL_H
#endif

typedef enum {
	GBP_FREE = 0,
	GBP_CLOUD_AT_ORIGIN = 1,

	// Also writes [glyph] to the high score structure once it reached
	// [target], immediately before advancing to the next phase.
	GBP_FLOAT_TO_TARGET = 2,

	GBP_SPLASH_AT_TARGET = 3,
	GBP_DONE = 4,

	// Non-[GBP_FREE] glyph ball should be removed
	GBP_REMOVE_REQUEST = 5,

	_glyphball_phase_t_FORCE_UINT8 = 0xFF
} glyphball_phase_t;

struct glyphball_t {
	glyphball_phase_t phase;
	gaiji_th04_t glyph;
	MotionBase<SPPoint> pos; // Relative to the top-left corner of the screen.
	SPPoint target;
	unsigned char angle;
	SubpixelLength8 speed;

	/// [measured] `unsigned`, not `int`: all three reads of this field in
	/// glyphballs_update_and_render() are `shr`, not `sar`
	/// (`0A54:1F0E`, `0A54:1FD0` and `0A54:20E7` in the original).
	unsigned int phase_frame;

	int8_t padding[6];
};

// 1 additional unused one, for some reason?
extern glyphball_t glyphballs[SCOREDAT_NAME_LEN + 1];
/// --------------------------------------------------

/// Externals that have no header
/// -----------------------------
/// Every one of these is spelled exactly as the translation unit that already
/// owns it spells it; none of them is declared in a header anywhere in the
/// tree, so repeating the declaration is the tree's own convention rather
/// than a shortcut.

// th04/hiscore/regist_enter.cpp's own declaration.
extern uint8_t entered_place;

/// [measured] ASM-only so far (`_entered_name_cursor dw 0`, th05_maine.asm's
/// _DATA). regist_menu() is its only other user and is still ASM; it is a
/// word there, and is inc/dec'd and compared against 0 and
/// (SCOREDAT_NAME_LEN - 1).
extern int entered_name_cursor;

/// [measured] The page-flip toggle, which reached this parcel as an unnamed
/// IDA byte label in th05_maine.asm's _DATA with no `public` — i.e. a
/// `static page_t` in ZUN's source. It cannot move into this file: it sits
/// between th04/hiscore/alphabet[data].asm and _entered_name_cursor inside
/// that _DATA contribution, and re-homing it would shift every following byte
/// of it. So the storage stays in the dump and is published there under this
/// name instead. kb/codegen/0123 is the same problem one step further along:
/// there the private label needed a zero-byte `label` alias because the dump
/// still referenced it, while here regist_frame_and_flip() was its only
/// reader, so renaming it outright costs nothing and leaves one spelling.
extern page_t regist_page_shown;

// th04/hiscore/regist_view.cpp's own declaration.
extern const unsigned char gALPHABET[ALPHABET_ROWS][ALPHABET_COLS];

/// th04/hiscore/regist_view.cpp, at the tail of th05/hi_end.cpp's SCORE_TEXT
/// contribution — a different object, but the same segment and the same
/// group, so `near` is correct.
void pascal near name_put(int place, playchar_t pc, unsigned char cursor);
/// -----------------------------

/// Coordinates
/// -----------
/// [measured] Duplicated from th04/hiscore/regist_view.cpp, which in turn
/// duplicates th04/hiscore/view.cpp's, for the reason that file gives: these
/// are `static const` in a translation unit that MAINE.EXE links from a
/// different object, and there is no header to share them through. Every
/// literal in this dump's block falls out of them:
/// 8 and 328 are the two [NAME_LEFT] columns, 88/96 and 224/232 the two
/// tables' place rows, 23 and 21 the alphabet grid's top-left TRAM cell.
static const pixel_t DIGIT_W = 16;
static const pixel_t NAME_W = (SCOREDAT_NAME_LEN * GAIJI_W);
static const pixel_t NAME_PADDED_W = (NAME_W + 6 + DIGIT_W);
static const pixel_t SCORE_PADDED_W = ((DIGIT_W * SCORE_DIGITS) + 8);
static const pixel_t STAGE_PADDED_W = (GAIJI_W + 8);
static const pixel_t COLUMN_W = (
	NAME_PADDED_W + SCORE_PADDED_W + STAGE_PADDED_W + 10
);

static const screen_x_t NAME_LEFT = 8;

static const screen_y_t TABLE_1_TOP = 88;
static const screen_y_t TABLE_2_TOP = 224;
static const pixel_t PLACE_1_PADDING_BOTTOM = 8;

#define top_for_place(table_top, place) ( \
	((place) == 0) \
		? (table_top) \
		: ((table_top) + PLACE_1_PADDING_BOTTOM + ((place) * GLYPH_H)) \
)

static const tram_x_t ALPHABET_LEFT = 23;
static const tram_y_t ALPHABET_TOP = 21;
/// -----------

static void near glyphballs_update_and_render(void);

/// One frame of the registration screen: advance the sound effects, redraw
/// the name row and every live glyph ball onto the page that is *not* being
/// shown, wait for the next frame, then flip the two pages.
void near regist_frame_and_flip(void)
{
	snd_se_update();
	glyphballs_update_and_render();
	frame_delay(1);
	graph_accesspage(regist_page_shown);
	graph_showpage(regist_page_shown = (1 - regist_page_shown));
}

/// kb/codegen/0096 + 0139: both of the two functions below generate a
/// `jmp cs:` switch table, and the original pads both onto the next offset.
/// Scoped over exactly those two, and restored to -a1 at the end of the file
/// so that the next lift appended here does not inherit the padding silently.
#pragma option -a2

/// Launches the glyph at ([alphabet_col], [alphabet_row]) of the name entry
/// grid towards [slot] of the name being registered at [place] in [pc]'s high
/// score table, blocking until that slot is free.
void pascal near glyphball_spawn(
	int alphabet_col, int alphabet_row, int place, playchar_t pc,
	unsigned char slot
)
{
	glyphball_t near *p = &glyphballs[slot];

	if(p->phase != GBP_FREE) {
		p->phase = GBP_REMOVE_REQUEST;
	}
	while(p->phase != GBP_FREE) {
		regist_frame_and_flip();
	}

	p->phase = GBP_CLOUD_AT_ORIGIN;
	p->phase_frame = 0;

	p->glyph = static_cast<gaiji_th04_t>(gALPHABET[alphabet_row][alphabet_col]);
	if(p->glyph == gs_SPACE) {
		p->glyph = g_EMPTY;
	}

	p->pos.cur.x.v = (
		(TO_SP(ALPHABET_LEFT + (alphabet_col * GAIJI_TRAM_W)) * GLYPH_HALF_W) +
		TO_SP(GAIJI_W / 2)
	);
	p->pos.cur.y.v = (
		(TO_SP(ALPHABET_TOP + alphabet_row) * GLYPH_H) + TO_SP(GLYPH_H / 2)
	);

	p->angle = irand();
	p->speed.v = ((irand() % TO_SP(4)) + TO_SP(4));

	// Same 2×2 table layout as score_put(), stage_put() and place_put(). A
	// [pc] outside it leaves the alphabet grid coordinates in place, which is
	// unreachable — PLAYCHAR_COUNT is 4.
	switch(pc) {
	case PLAYCHAR_REIMU:
		alphabet_col = (NAME_LEFT + (0 * COLUMN_W));
		alphabet_row = top_for_place(TABLE_1_TOP, place);
		break;
	case PLAYCHAR_MARISA:
		alphabet_col = (NAME_LEFT + (1 * COLUMN_W));
		alphabet_row = top_for_place(TABLE_1_TOP, place);
		break;
	case PLAYCHAR_MIMA:
		alphabet_col = (NAME_LEFT + (0 * COLUMN_W));
		alphabet_row = top_for_place(TABLE_2_TOP, place);
		break;
	case PLAYCHAR_YUUKA:
		alphabet_col = (NAME_LEFT + (1 * COLUMN_W));
		alphabet_row = top_for_place(TABLE_2_TOP, place);
		break;
	}

	p->target.x.v = (
		TO_SP(alphabet_col) + TO_SP(slot * GAIJI_W) + TO_SP(GAIJI_W / 2)
	);
	p->target.y.v = (TO_SP(alphabet_row) + TO_SP(GLYPH_H / 2));
}

static void near glyphballs_update_and_render(void)
{
	int patnum;
	screen_x_t left;
	screen_y_t top;
	unsigned char angle_delta;
	unsigned char target_angle;
	int i;
	glyphball_t near *p;

	p = glyphballs;
	for(i = 0; i < SCOREDAT_NAME_LEN; i++, p++) {
		if(p->phase == GBP_FREE) {
			continue;
		}
		if(p->phase == GBP_DONE) {
			p->phase = GBP_FREE;
		}
		if(p->pos.prev.x < 0) {
			p->pos.prev.x.v = 0;
		}
		if(p->pos.prev.y < 0) {
			p->pos.prev.y.v = 0;
		}
		bgimage_put_rect_16(
			(p->pos.prev.x.to_pixel_slow() - (GLYPHBALL_CLOUD_SPLASH_W / 2)),
			(p->pos.prev.y.to_pixel_slow() - (GLYPHBALL_CLOUD_SPLASH_H / 2)),
			GLYPHBALL_CLOUD_SPLASH_W,
			GLYPHBALL_CLOUD_SPLASH_H
		);
	}

	name_put(entered_place, playchar, entered_name_cursor);

	p = glyphballs;
	for(i = 0; i < SCOREDAT_NAME_LEN; i++, p++) {
		if(p->phase == GBP_FREE) {
			continue;
		}
		switch(p->phase) {
		case GBP_CLOUD_AT_ORIGIN:
			p->pos.prev = p->pos.cur;
			left = (
				p->pos.cur.x.to_pixel_slow() - (GLYPHBALL_CLOUD_SPLASH_W / 2)
			);
			top = (
				p->pos.cur.y.to_pixel_slow() - (GLYPHBALL_CLOUD_SPLASH_H / 2)
			);
			patnum = ((p->phase_frame >> 1) + PAT_GLYPHBALL_CLOUD);
			if(patnum < (PAT_GLYPHBALL_CLOUD + GLYPHBALL_CELS)) {
				break;
			}
			p->phase++;
			// fall through

		case GBP_FLOAT_TO_TARGET:
			p->pos.prev = p->pos.cur;
			vector2_at(p->pos.velocity, 0, 0, p->speed, p->angle);
			p->pos.cur.x.v += p->pos.velocity.x.v;
			p->pos.cur.y.v += p->pos.velocity.y.v;

			left = (p->pos.cur.x.to_pixel_slow() - (GLYPHBALL_W / 2));
			top = (p->pos.cur.y.to_pixel_slow() - (GLYPHBALL_H / 2));
			if(left < 0) {
				left = 0;
				p->angle = (0x80 - p->angle);
				p->pos.cur.x.v = TO_SP(GLYPHBALL_W / 2);
			} else if(left >= (RES_X - GLYPHBALL_W)) {
				left = (RES_X - GLYPHBALL_W);
				p->angle = (0x80 - p->angle);
				p->pos.cur.x.v = TO_SP(RES_X - (GLYPHBALL_W / 2));
			}
			if(top < 0) {
				top = 0;
				p->angle = -p->angle;
				p->pos.cur.y.v = TO_SP(GLYPHBALL_H / 2);
			} else if(top >= (RES_Y - GLYPHBALL_H)) {
				top = (RES_Y - GLYPHBALL_H);
				p->angle = -p->angle;
				p->pos.cur.y.v = TO_SP(RES_Y - (GLYPHBALL_H / 2));
			}

			patnum = (((p->phase_frame >> 2) & 3) + PAT_GLYPHBALL);
			target_angle = iatan2(
				(p->target.y.v - p->pos.cur.y.v),
				(p->target.x.v - p->pos.cur.x.v)
			);
			angle_delta = (p->angle - target_angle);
			if(angle_delta >= 0x80) {
				if(angle_delta >= static_cast<unsigned char>(-2)) {
					p->angle = target_angle;
					if(p->speed < TO_SP(8)) {
						p->speed.v += 2;
					}
				} else if(angle_delta > static_cast<unsigned char>(-0x10)) {
					angle_delta = 1;
					if(p->speed < TO_SP(8)) {
						p->speed.v += 2;
					}
					p->angle += angle_delta;
				} else {
					angle_delta = ((0x100 - angle_delta) / 0x10);
					if(p->speed > 8) {
						p->speed.v += -2;
					}
					p->angle += angle_delta;
				}
			} else {
				if(angle_delta <= 2) {
					p->angle = target_angle;
					if(p->speed < TO_SP(8)) {
						p->speed.v += 2;
					}
				} else if(angle_delta < 0x10) {
					angle_delta = 1;
					if(p->speed < TO_SP(8)) {
						p->speed.v += 2;
					}
					p->angle -= angle_delta;
				} else {
					angle_delta = (angle_delta / 0x10);
					if(p->speed > 8) {
						p->speed.v += -2;
					}
					p->angle -= angle_delta;
				}
			}

			if(
				(static_cast<unsigned int>(
					(p->pos.cur.x.v - p->target.x.v) + TO_SP(4)
				) < TO_SP(8)) &&
				(static_cast<unsigned int>(
					(p->pos.cur.y.v - p->target.y.v) + TO_SP(4)
				) < TO_SP(8))
			) {
				p->phase++;
				p->phase_frame = 0;
				hi.score.g_name[entered_place][i] = p->glyph;
			}
			break;

		case GBP_SPLASH_AT_TARGET:
			patnum = ((p->phase_frame >> 2) + PAT_GLYPHBALL_SPLASH);
			if(patnum >= (PAT_GLYPHBALL_SPLASH + GLYPHBALL_CELS)) {
				p->phase++;
				continue;
			}
			p->pos.prev = p->target;
			left = (p->target.x.to_pixel_slow() - (GLYPHBALL_CLOUD_SPLASH_W / 2));
			top = (p->target.y.to_pixel_slow() - (GLYPHBALL_CLOUD_SPLASH_H / 2));
			break;

		case GBP_REMOVE_REQUEST:
			p->phase--;
			continue;
		}

		p->phase_frame++;
		super_put_rect(left, top, patnum);
	}
}

#pragma option -a1

/// The player confirmed the name: give every glyph ball still on screen a
/// straight line to its own slot at a fixed speed — "rushing" it, as opposed
/// to glyphballs_update_and_render()'s gradual steering, which only turns the
/// angle by degrees and accelerates in steps — and then block until all of
/// them have splashed down and freed themselves.
void near glyphballs_rush_and_wait(void)
{
	unsigned char live = 0;
	int i;
	glyphball_t near *p;

	snd_se_play(11);

	p = glyphballs;
	for(i = 0; i < SCOREDAT_NAME_LEN; i++, p++) {
		if(p->phase == GBP_CLOUD_AT_ORIGIN) {
			p->phase = GBP_FLOAT_TO_TARGET;
		}
		if(p->phase == GBP_FLOAT_TO_TARGET) {
			p->angle = iatan2(
				(p->target.y.v - p->pos.cur.y.v),
				(p->target.x.v - p->pos.cur.x.v)
			);
			p->speed.v = TO_SP(6);
		}
	}

	do {
		live = 0;
		for(i = 0, p = glyphballs; i < SCOREDAT_NAME_LEN; i++, p++) {
			if(p->phase != GBP_FREE) {
				live++;
			}
		}
		regist_frame_and_flip();
	} while(live != 0);
}

// The screen that drives all of the above. ONE body for both games; TH04
// includes the same file from the tail of th04/hiscore/end.cpp.
#include "th04/hiscore/regist_menu.cpp"

// ...and the verdict screen's 3-digit gaiji renderer, which was the first
// proc of the root dump's SCORE_TEXT block, i.e. immediately after this
// object's contribution. Another kb/codegen/0098 head lift.
#include "th04/end/verdict_digits.cpp"

// The next two out of that same head, in the dump's own order. Each one only
// lands at its original address because the one above it already did, so
// nothing may be inserted between them or above verdict_digits.cpp — and
// anything lifted next goes BELOW this line, never above it.
#include "th05/end/verdict_guts.cpp"
#include "th05/end/verdict_comment_num.cpp"

// ...and the screen those two feed, which was the LAST proc of that block.
// With it gone, th05_maine.asm contributes zero bytes to SCORE_TEXT and this
// object's contribution runs straight into th05/staff.cpp's. Nothing may be
// appended below this line without measuring what it does to that seam.
#include "th05/end/verdict_stats.cpp"
