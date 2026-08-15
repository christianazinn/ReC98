/* ReC98
 * -----
 * TH02's pause menu and its quit confirmation. stage_loop() enters this
 * whenever [key_det] carries INPUT_CANCEL, and quits the stage if it returns
 * `true`.
 *
 * Unlike TH01's pause_menu(), this one drives both of its menus from a single
 * per-frame state machine rather than a blocking input loop per menu.
 */

// The original's prolog is a single `ENTER 2, 0`, which is what -G- (optimize
// for size) emits. -G would give `push bp; mov bp, sp; sub sp, 2`.
// (kb/codegen/0011)
#pragma option -zCMENU_TEXT -zPmain_01 -G-

#include "platform.h"
#include "pc98.h"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/hardware/frmdelay.h"
#include "th02/hardware/input.hpp"
#include "th02/main/hud/menu.hpp"

// Menu data
// ---------
// ZUN kept the two menus' strings in pointer variables rather than passing the
// literals straight to text_putsa(), and both those pointers and the two
// attribute bytes below sit in the middle of th02_main.asm's own `_DATA` and
// `_BSS` contributions. Re-emitting any of them from this translation unit
// would shift every later byte of those segments, so they stay in the dump and
// we only reference them. (kb/codegen/0084)

// "Resume game", the first choice of the pause menu.
extern const char far *const PAUSE_CHOICE_RESUME;

// "Quit game", its second choice.
extern const char far *const PAUSE_CHOICE_QUIT;

// "Are you really quitting?", the quit confirmation's title.
extern const char far *const QUIT_TITLE;

// "Just kidding, sorry." – the confirmation's first choice, i.e. *not*
// quitting.
extern const char far *const QUIT_CHOICE_NO;

// "Yes, I'll quit." – its second choice.
extern const char far *const QUIT_CHOICE_YES;

// Gaiji title, blinked above whichever menu is currently shown, and blanked
// with g11SPACES.
extern char gPAUSE_MENU[];
extern char g11SPACES[];

// 0 for the first of the two choices, 1 for the second. Shared between both
// menus, and reset when switching from one to the other.
extern uint8_t pause_sel;

// TRAM attributes for the currently shown menu's two choice rows. Only ever
// used inside pause_menu(), but ZUN put them into `_BSS` rather than onto the
// stack.
extern uint8_t pause_atrb_0;
extern uint8_t pause_atrb_1;
// ---------

// Coordinates
// -----------
static const unsigned PAUSE_TITLE_LEFT = 18;
static const unsigned PAUSE_TITLE_Y = 12;

// The pause menu is rendered flush against the bottom of the playfield, the
// quit confirmation in the middle of the screen.
static const unsigned PAUSE_CHOICE_LEFT = 23;
static const unsigned PAUSE_CHOICE_0_Y = 15;

static const unsigned QUIT_LEFT = 17;
static const unsigned QUIT_TITLE_Y = 14;
static const unsigned QUIT_CHOICE_0_Y = 15;
// -----------

// Blinking cycle of the gaiji title, in frames, and the number of those frames
// it is shown for.
static const unsigned TITLE_BLINK_CYCLE = 64;
static const unsigned TITLE_BLINK_SHOWN = 50;

static const unsigned ATRB_SELECTED = (TX_WHITE | TX_UNDERLINE);
static const unsigned ATRB_UNSELECTED = TX_MAGENTA;

// pause_menu()'s state machine. Every state renders and reads input for at
// most one frame before the shared frame_delay(1) at the bottom of the loop.
static const uint8_t STATE_PAUSE_RELEASE = 0; // pause menu, waiting for release
static const uint8_t STATE_PAUSE_INPUT   = 1; // pause menu, reading input
static const uint8_t STATE_RESUME        = 2; // leaving, waiting for release
static const uint8_t STATE_QUIT_RELEASE  = 3; // confirmation, waiting
static const uint8_t STATE_QUIT_INPUT    = 4; // confirmation, reading input

// Moves the cursor to the other choice, and assigns the highlighted attribute
// to the newly selected row.
inline void pause_sel_toggle(void)
{
	pause_sel = (1 - pause_sel);
	if(pause_sel == 0) {
		pause_atrb_0 = ATRB_SELECTED;
		pause_atrb_1 = ATRB_UNSELECTED;
	} else {
		pause_atrb_0 = ATRB_UNSELECTED;
		pause_atrb_1 = ATRB_SELECTED;
	}
}

bool16 near pause_menu(void)
{
	register unsigned frame = 0;
	uint8_t state = STATE_PAUSE_RELEASE;

	text_putsa(PAUSE_CHOICE_LEFT, PAUSE_CHOICE_0_Y, PAUSE_CHOICE_RESUME, ATRB_SELECTED);
	text_putsa(PAUSE_CHOICE_LEFT, (PAUSE_CHOICE_0_Y + 1), PAUSE_CHOICE_QUIT, ATRB_UNSELECTED);
	palette_settone(70);

	while(1) {
		if((frame % TITLE_BLINK_CYCLE) < TITLE_BLINK_SHOWN) {
			gaiji_putsa(PAUSE_TITLE_LEFT, PAUSE_TITLE_Y, gPAUSE_MENU, TX_WHITE);
		} else {
			gaiji_putsa(PAUSE_TITLE_LEFT, PAUSE_TITLE_Y, g11SPACES, TX_WHITE);
		}
		input_reset_sense();
		frame++;

		if((state == STATE_PAUSE_RELEASE) && (key_det == INPUT_NONE)) {
			state = STATE_PAUSE_INPUT;
		}

		if(state == STATE_PAUSE_INPUT) {
			if((key_det & INPUT_UP) || (key_det & INPUT_DOWN)) {
				pause_sel_toggle();
				text_putsa(
					PAUSE_CHOICE_LEFT, PAUSE_CHOICE_0_Y,
					PAUSE_CHOICE_RESUME, pause_atrb_0
				);
				text_putsa(
					PAUSE_CHOICE_LEFT, (PAUSE_CHOICE_0_Y + 1),
					PAUSE_CHOICE_QUIT, pause_atrb_1
				);
				state = STATE_PAUSE_RELEASE;
			}
			if((key_det & INPUT_OK) || (key_det & INPUT_SHOT)) {
				if(pause_sel == 0) {
					state = STATE_RESUME;
				} else {
					text_putsa(
						QUIT_LEFT, QUIT_TITLE_Y, QUIT_TITLE,
						(TX_WHITE | TX_BLINK)
					);
					text_putsa(
						QUIT_LEFT, QUIT_CHOICE_0_Y, QUIT_CHOICE_NO, TX_WHITE
					);
					text_putsa(
						QUIT_LEFT, (QUIT_CHOICE_0_Y + 1), QUIT_CHOICE_YES,
						TX_MAGENTA
					);
					state = STATE_QUIT_RELEASE;
					pause_sel = 0;
				}
			}
			if(key_det & INPUT_CANCEL) {
				state = STATE_RESUME;
			}
		} else if(state == STATE_RESUME) {
			if(key_det == INPUT_NONE) {
				palette_settone(100);
				key_det = INPUT_NONE;
				gaiji_putsa(PAUSE_TITLE_LEFT, PAUSE_TITLE_Y, g11SPACES, TX_WHITE);
				gaiji_putsa(QUIT_LEFT, QUIT_TITLE_Y, g11SPACES, TX_WHITE);
				gaiji_putsa(QUIT_LEFT, QUIT_CHOICE_0_Y, g11SPACES, TX_WHITE);
				gaiji_putsa(QUIT_LEFT, (QUIT_CHOICE_0_Y + 1), g11SPACES, TX_WHITE);
				return false;
			}
		} else if(state == STATE_QUIT_RELEASE) {
			if(key_det == INPUT_NONE) {
				state++; // -> STATE_QUIT_INPUT
			}
		} else if(state == STATE_QUIT_INPUT) {
			if((key_det & INPUT_UP) || (key_det & INPUT_DOWN)) {
				pause_sel_toggle();
				text_putsa(
					QUIT_LEFT, QUIT_CHOICE_0_Y, QUIT_CHOICE_NO, pause_atrb_0
				);
				text_putsa(
					QUIT_LEFT, (QUIT_CHOICE_0_Y + 1), QUIT_CHOICE_YES,
					pause_atrb_1
				);
				state = STATE_QUIT_RELEASE;
			}
			if((key_det & INPUT_OK) || (key_det & INPUT_SHOT)) {
				if(pause_sel == 0) {
					gaiji_putsa(QUIT_LEFT, QUIT_TITLE_Y, g11SPACES, TX_WHITE);
					gaiji_putsa(QUIT_LEFT, QUIT_CHOICE_0_Y, g11SPACES, TX_WHITE);
					gaiji_putsa(
						QUIT_LEFT, (QUIT_CHOICE_0_Y + 1), g11SPACES, TX_WHITE
					);
					text_putsa(
						PAUSE_CHOICE_LEFT, PAUSE_CHOICE_0_Y,
						PAUSE_CHOICE_RESUME, ATRB_SELECTED
					);
					text_putsa(
						PAUSE_CHOICE_LEFT, (PAUSE_CHOICE_0_Y + 1),
						PAUSE_CHOICE_QUIT, ATRB_UNSELECTED
					);
					state = STATE_PAUSE_RELEASE;
				} else {
					return true;
				}
			}
			if(key_det & INPUT_CANCEL) {
				state = STATE_RESUME;
			}
		}
		frame_delay(1);
	}
}
