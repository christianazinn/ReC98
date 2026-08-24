/// MAINE.EXE's high score name registration screen
/// -----------------------------------------------
/// ONE body for both games. This is the last proc of the SCORE_TEXT root
/// block in either dump — TH04 `0A05:27C4` (`0x3D3` bytes), TH05
/// `0A54:1C9D` (`0x40A`) — and in both it is a kb/codegen/0098 head lift into
/// the object that already sits immediately before it: th04/regist.cpp for
/// the merged TH04 product and
/// th05/regist.cpp for TH05, which had to go first because TH05's version
/// calls three functions that live there. No carve, no new segment, no
/// Tupfile.lua line in either game.
///
/// `[measured]` One body rather than two was a *decision*, and the obvious
/// instrument argues against it: the two originals share only 28.3% of their
/// bytes. That number is meaningless here — name_put(), a confirmed single
/// shared body in this same cluster, shares 24.0%, i.e. less. Comparing
/// allocation-insensitive instruction *shapes* instead (registers and
/// bp-offsets collapsed to one token, since those follow from each game's
/// differing variable set and cost no `#if`), regist_menu() is 52.4% shared,
/// three times name_put's 17.9%. Full working in
/// state/notes/th0405-maine-regist-menu.md.
///
/// The divergence that motivated the question — TH05 flies each entered glyph
/// to the table as a "glyph ball" and TH04 has no such animation — turns out
/// to be leaf-local: it lands inside the entry dispatch and the two
/// backspace arms, not across the skeleton.

#if (GAME == 5)
#include "th01/rank.h"
#include "th04/end/end.h"
#include "th05/resident.hpp"
#include "th05/hardware/input.h"
#include "platform/x86real/pc98/grp_surf.hpp"
#else
#include "th02/hardware/frmdelay.h"
#include "th04/hardware/input.h"
#include "th03/formats/pi.hpp"
#endif
#include "th04/common.h"
#include "th04/hardware/grppsafx.h"
#if (GAME == 5)
extern "C" void pascal snd_load(const char *fn, snd_load_func_t func);
#else
#include "th04/snd/snd.h"
#endif

extern unsigned char rank;

#if (GAME == 5)
/// [measured] In the classic build, these four live in th05/hi_end.cpp's
/// SCORE_TEXT contribution. The merged product supplies them earlier in
/// th05/regist.cpp instead. Their original `near` contracts stay unchanged.
bool pascal near hiscore_scoredat_load_for(int playchar);
void near hiscore_scoredat_save(void);
void near regist_score_enter_from_resident(void);
void pascal near places_put(int pc);
void pascal near alphabet_putca(int col, int row, tram_atrb2 atrb);
#endif

/// These strings remain bytes of the root dump rather than literals of this
/// translation unit. The debloated product stores the identical shadow and
/// foreground message once and passes the same address to both calls.
extern const char hi01_pi[];
extern const char BGM_NAME_FN[];
#if (GAME == 5)
extern const char scnum_bft[];
extern const char sctm0_bft[];
extern const char sctm1_bft[];
#else
extern const char scnum2_bft[];
#endif

// ZUN bloat: The original stores the same string twice for its drop shadow.
extern const shiftjis_t SLOW_MODE_MSG[];

/// [measured] The alphabet cursor wraps at both ends, but not through any of
/// th01/math/clamp.hpp's ring macros: those all step *and* wrap in one go,
/// while both originals apply every pressed direction first and only then
/// bring the result back into range. Written out to match.
#define ring_clamp(v, ring_max) { \
	if((v) < 0) { \
		(v) = (ring_max); \
	} else if((v) > (ring_max)) { \
		(v) = 0; \
	} \
}

#if (GAME == 5)
// Requests removal of the glyph ball in [slot], if there is one. The original
// recomputes the subscript for the store, so this must not cache it.
#define glyphball_remove_request(slot) { \
	if(glyphballs[slot].phase != GBP_FREE) { \
		glyphballs[slot].phase = GBP_REMOVE_REQUEST; \
	} \
}

// TH05 keeps the cursor in a global because the glyph ball code reads it.
#define cursor entered_name_cursor

// kb/codegen/0091: places_put()'s formal is 16-bit here, so the ordinary
// promotion already emits the zero-extension.
#define playchar_arg(pc) (pc)
#else
/// kb/codegen/0034: ...and byte-sized here, which is the whole
/// discriminator. Without the _AX assignment Turbo C++ pushes the global
/// as a raw `push word ptr [playchar]` instead of zero-extending through
/// AX, two bytes shorter. `static_cast<int>()` does NOT do it -- that is
/// 0034's own listed counter-shape, and it cost this parcel a cycle.
/// It has to be the _AL/_AH pair, not `_AX = `: TH04's playchar_t has no
/// forced-unsigned member, so Turbo C++ types it `signed char` and a plain
/// _AX assignment emits `cbw` where the original zero-extends.
#define playchar_arg(pc) (_AL = (pc), _AH = 0, _AX)
#endif

void near regist_menu(void)
{
	/// [measured] Declaration order is pinned by both frames at once, and by
	/// th04/hiscore/regist_view.cpp's place_put() rule — "Turbo C++ hands the
	/// one free register to whichever of the two is declared first". TH04
	/// spends SI on [cursor] and DI on [col], leaving [row] to spill;
	/// dropping [cursor] for TH05 shifts SI to [col] and DI to [row], which
	/// is exactly what that dump does. Everything that spills then lands in
	/// declaration order from bp-2 downwards, words before bytes. TH04's frame
	/// is 10 bytes for row/input_locked/i/j/input_delay/glyph; TH05's is 8
	/// bytes for the same list minus [row].
#if (GAME != 5)
	int cursor = 0;
#endif
	int col;
	int row;
	int input_locked;

	// Reused: TH05 counts player characters with [i] before either loop below
	// uses it as the alphabet row.
	int i;
	int j;

	unsigned char input_delay = 0;
	unsigned char glyph;

#if (GAME == 5)
	graph_accesspage(0);
	graph_showpage(0);
#endif
	palette_settone(0);
	graph_accesspage(1);
#if (GAME == 5)
	GrpSurface_BlitBackgroundPI(&Palettes, hi01_pi);
#else
	pi_load(0, hi01_pi);
	pi_palette_apply(0);
	pi_put_8(0, 0, 0);
	pi_free(0);
#endif

	graph_copy_page(0);
#if (GAME == 5)
	super_entry_bfnt(scnum_bft);
	super_entry_bfnt(sctm0_bft);
	super_entry_bfnt(sctm1_bft);
#else
	super_entry_bfnt(scnum2_bft);
#endif

	rank = ((resident->stage == STAGE_EXTRA)
		? RANK_EXTRA
		: resident->rank
	);

#if (GAME == 5)
	playchar = static_cast<playchar_t>(resident->playchar);

	// Every *other* character's table is drawn once, from that character's
	// own high score file, before the current one's is loaded back over it.
	for(i = 0; i < PLAYCHAR_COUNT; i++) {
		if(playchar != i) {
			hiscore_scoredat_load_for(i);
			places_put(i);
		}
	}
	hiscore_scoredat_load_for(playchar);
#else
	// ZUN bloat: TH04 stores the player character as an ASCII digit in the
	// resident structure and has to map it back here.
	playchar = static_cast<playchar_t>(
		resident->playchar_ascii == ('0' + PLAYCHAR_MARISA)
	);

	// With only two characters, "every other one" is a single subtraction.
	// [measured] The cast is not decoration: the original does this one in
	// byte width because the parameter is a 1-byte enum, and the places_put()
	// line right below it in word width because that parameter is an `int`.
	hiscore_scoredat_load_for(static_cast<playchar_t>(1 - playchar));
	places_put((_AL = playchar, _AH = 0, _DX = 1, _DX -= _AX, _DX));
	hiscore_scoredat_load_for(playchar);
#endif

	/// `[measured]` This branch is CONSISTENT, and a ZUN-bug "Backwards?" note
	/// here was retracted on 2026-08-18: it rested on reading `[turbo_mode]`
	/// nonzero as "slow mode", which is inverted. Nonzero is TURBO mode —
	/// `command_put(..., CDG_OPTION_SLOW - resident->turbo_mode)`
	/// (th04/op/m_main.cpp) renders the SLOW label only when [turbo_mode] is 0,
	/// CDG_OPTION_SLOW being CDG_OPTION_TURBO + 1 (th04/sprites/op_cdg.hpp), and
	/// the bullet-density slowdown is gated on `turbo_mode == false`
	/// (th04/main/bullet/update.cpp). So `cmp turbo_mode, 0` / `jnz` reaching
	/// regist_score_enter_from_resident() — which is measured and correct — is
	/// the TURBO path entering the score, while Slow Mode falls to the `else`
	/// and shows SLOW_MODE_MSG. Message and behaviour agree.
	if(resident->turbo_mode || (rank == RANK_EXTRA)) {
		regist_score_enter_from_resident();
		places_put(playchar_arg(playchar));
	} else {
		entered_place = -1;
		places_put(playchar_arg(playchar));
		graph_putsa_fx(124, 196, 9, SLOW_MODE_MSG);
		graph_putsa_fx(120, 192, 2, SLOW_MODE_MSG);
	}

#if (GAME == 5)
	bgimage_snap();
	if(
		(resident->end_sequence >= ES_EXTRA) &&
		!resident->score_last.digits[0]
	) {
		hi.score.cleared = SCOREDAT_CLEARED;
	}
	graph_copy_page(1);
	bgimage_snap();
#else
	if(
		(resident->end_sequence == ES_GOOD) ||
		(resident->end_sequence == ES_EXTRA) ||
		(rank == RANK_EASY)
	) {
		// ZUN bloat: The shot type ternary is spelled out in both branches
		// rather than once above the `if`, and the original duly emits it
		// twice.
		glyph = hi.score.cleared;
		if(glyph >= (SCOREDAT_CLEARED_BOTH + 1)) {
			glyph = ((resident->shottype == SHOTTYPE_A)
				? SCOREDAT_CLEARED_A
				: SCOREDAT_CLEARED_B
			);
		} else {
			glyph |= ((resident->shottype == SHOTTYPE_A)
				? SCOREDAT_CLEARED_A
				: SCOREDAT_CLEARED_B
			);
		}
		hi.score.cleared = glyph;
	}
#endif

	snd_kaja_func(KAJA_SONG_STOP, 0);
	snd_load(BGM_NAME_FN, SND_LOAD_SONG);
	snd_kaja_func(KAJA_SONG_PLAY, 0);
	palette_black_in(2);

	if(entered_place != static_cast<uint8_t>(-1)) {
#if (GAME == 5)
		// Both pages need the name row before the first flip, or the very
		// first frame shows whatever the other page still held.
		name_put(entered_place, playchar, 0);
		graph_accesspage(0);
		name_put(entered_place, playchar, 0);
		graph_accesspage(1);
#endif

		// ZUN bloat: alphabet_putca() exists and is called throughout the
		// loop below, but the initial full draw inlines it.
		for(i = 0; i < ALPHABET_ROWS; i++) {
			for(j = 0; j < ALPHABET_COLS; j++) {
				gaiji_putca(
					(ALPHABET_LEFT + (j * GAIJI_TRAM_W)),
					(ALPHABET_TOP + i),
					gALPHABET[i][j],
					TX_WHITE
				);
			}
		}
		gaiji_putca(
			ALPHABET_LEFT, ALPHABET_TOP, gALPHABET[0][0],
			(TX_GREEN | TX_REVERSE)
		);

		col = 0;
		row = 0;
#if (GAME != 5)
		input_reset_sense();
#endif
		input_locked = 1;

		while(1) {
#if (GAME == 5)
			input_reset_sense_held();
#else
			input_sense();
#endif
			if(!input_locked) {
				// [measured] A byte test: the mask's high byte is zero, so
				// this is the four cardinal directions only, not
				// INPUT_MOVEMENT's diagonals as well.
				if(key_det & (
					INPUT_UP | INPUT_DOWN | INPUT_LEFT | INPUT_RIGHT
				)) {
					alphabet_putca(col, row, TX_WHITE);
					if(key_det & INPUT_UP) {
						row--;
					}
					if(key_det & INPUT_DOWN) {
						row++;
					}
					if(key_det & INPUT_LEFT) {
						col--;
					}
					if(key_det & INPUT_RIGHT) {
						col++;
					}
					ring_clamp(row, (ALPHABET_ROWS - 1));
					ring_clamp(col, (ALPHABET_COLS - 1));
					alphabet_putca(col, row, (TX_GREEN | TX_REVERSE));
#if (GAME == 5)
					snd_se_play(1);
#endif
				}
				if((key_det & INPUT_SHOT) || (key_det & INPUT_OK)) {
					glyph = gALPHABET[row][col];

					/// [measured] kb/codegen/0135's form-selection table is
					/// why one `switch` serves both games: with gs_SPACE =
					/// 0xCD, gs_ARROW_LEFT/RIGHT = 0xCE/0xCF and gs_END =
					/// 0xD5, TH04's four cases span 0..8 off gs_SPACE and get
					/// the dense-range jump table, while TH05's three get the
					/// `cmp`/`jz` chain. The one `#if` below is the whole
					/// difference; the two code shapes fall out of it.
					switch(glyph) {
#if (GAME != 5)
					// TH05 has no case for this because its glyph balls do
					// the same substitution on arrival, in glyphball_spawn().
					case gs_SPACE:
						glyph = g_EMPTY;
						goto regular;
#endif
					case gs_ARROW_LEFT:
						hi.score.g_name[entered_place][cursor] = g_EMPTY;
#if (GAME == 5)
						glyphball_remove_request(cursor);
#endif
						if(cursor > 0) {
							cursor--;
						}
#if (GAME == 5)
						glyphball_remove_request(cursor);
						snd_se_play(4);
#endif
						break;

					case gs_ARROW_RIGHT:
#if (GAME == 5)
						snd_se_play(11);
#endif
						if(cursor < (SCOREDAT_NAME_LEN - 1)) {
							cursor++;
						}
						break;

					case gs_END:
						goto enter;

					default:
#if (GAME != 5)
					regular:
						hi.score.g_name[entered_place][cursor] = glyph;
#else
						snd_se_play(11);
						glyphball_spawn(
							col, row, entered_place, playchar, cursor
						);
#endif
						if(cursor == (SCOREDAT_NAME_LEN - 1)) {
							alphabet_putca(col, row, TX_WHITE);
							col = ALPHABET_ENTER_COL;
							row = ALPHABET_ENTER_ROW;
							alphabet_putca(
								col, row, (TX_GREEN | TX_REVERSE)
							);
						}
						if(cursor < (SCOREDAT_NAME_LEN - 1)) {
							cursor++;
						}
						break;
					}
#if (GAME != 5)
					name_put(entered_place, playchar, cursor);
#endif
				}
				if(key_det & INPUT_BOMB) {
					hi.score.g_name[entered_place][cursor] = g_EMPTY;
#if (GAME == 5)
					glyphball_remove_request(cursor);
#endif
					if(cursor > 0) {
						cursor--;
					}
#if (GAME == 5)
					glyphball_remove_request(cursor);
					snd_se_play(4);
#else
					name_put(entered_place, playchar, cursor);
#endif
				}
				if(key_det & INPUT_CANCEL) {
enter:
#if (GAME == 5)
					glyphballs_rush_and_wait();
#endif
					goto save;
				}
				input_locked = key_det;
			} else {
				// Auto-repeat: hold an input for half a second, and it starts
				// retriggering on every other frame.
				if(key_det == input_locked) {
					input_delay++;
					if((input_delay > 30) && ((input_delay & 1) == 0)) {
						input_locked = 0;
					}
				} else {
#if (GAME == 5)
					if(key_det == INPUT_NONE) {
						input_locked = 0;
					}
					input_delay = 0;
#else
					if(key_det != INPUT_NONE) {
						input_delay = 0;
					} else {
						input_locked = 0;
						input_delay = 0;
					}
#endif
				}
			}
#if (GAME == 5)
			regist_frame_and_flip();
#else
			input_reset_sense();
			frame_delay(1);
#endif
		}

save:
		hiscore_scoredat_save();
	} else {
		hiscore_scoredat_save();
		input_wait_for_change(0);
	}

#if (GAME == 5)
	bgimage_free();
#endif
	super_free();
	text_clear();
	palette_black_out(1);
}

#if (GAME == 5)
#undef cursor
#endif
