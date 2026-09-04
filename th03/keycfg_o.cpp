#pragma option -zPgroup_KEYCONFIG_INPUT

#include "x86real.h"
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "platform/x86real/flags.hpp"
#include "platform/x86real/pc98/keyboard.hpp"
#include "th02/formats/pi.h"
#include "th02/hardware/frmdelay.h"
#include "th02/v_colors.hpp"
#include "th03/hardware/input.h"
#include "th03/keyconfig.hpp"
#include "th03/menu_font.hpp"
#include "th03/op_patch.hpp"
#include "th03/pixel_capture.hpp"
#include "th03/shiftjis/fns.hpp"
#include <mem.h>
#include <stddef.h>

#define T3_KEYCONFIG_FILE_VERSION 2
#define T3_KEYCONFIG_CAPTURE_CANCEL 0xFE

enum keyconfig_menu_color_t {
	KEYCONFIG_COLOR_HEADER = 14,
	KEYCONFIG_COLOR_FOOTER = 14,
	KEYCONFIG_COLOR_SELECTED = 11,
	KEYCONFIG_COLOR_LABEL = 12,
	KEYCONFIG_COLOR_VALUE = V_WHITE,
};

#define KEYCONFIG_TEXT_FIELD(name, value) char name[sizeof(value)]
#pragma option -a1
struct keyconfig_text_t {
	KEYCONFIG_TEXT_FIELD(fn, T3_KEYCONFIG_FN);
	KEYCONFIG_TEXT_FIELD(temp_fn, T3_KEYCONFIG_TEMP_FN);
	KEYCONFIG_TEXT_FIELD(backup_fn, T3_KEYCONFIG_BACKUP_FN);
	KEYCONFIG_TEXT_FIELD(minus, "Minus");
	KEYCONFIG_TEXT_FIELD(circumflex, "Circumflex");
	KEYCONFIG_TEXT_FIELD(yen, "Yen");
	KEYCONFIG_TEXT_FIELD(backspace, "Backspace");
	KEYCONFIG_TEXT_FIELD(tab, "Tab");
	KEYCONFIG_TEXT_FIELD(at, "At");
	KEYCONFIG_TEXT_FIELD(lbracket, "LBracket");
	KEYCONFIG_TEXT_FIELD(return_key, "Return");
	KEYCONFIG_TEXT_FIELD(plus, "Plus");
	KEYCONFIG_TEXT_FIELD(asterisk, "Asterisk");
	KEYCONFIG_TEXT_FIELD(rbracket, "RBracket");
	KEYCONFIG_TEXT_FIELD(comma, "Comma");
	KEYCONFIG_TEXT_FIELD(period, "Period");
	KEYCONFIG_TEXT_FIELD(slash, "Slash");
	KEYCONFIG_TEXT_FIELD(underscore, "Underscore");
	KEYCONFIG_TEXT_FIELD(space, "Space");
	KEYCONFIG_TEXT_FIELD(xfer, "XFER");
	KEYCONFIG_TEXT_FIELD(roll_up, "Roll Up");
	KEYCONFIG_TEXT_FIELD(roll_down, "Roll Down");
	KEYCONFIG_TEXT_FIELD(insert_key, "Insert");
	KEYCONFIG_TEXT_FIELD(delete_key, "Delete");
	KEYCONFIG_TEXT_FIELD(up_arrow, "Up Arrow");
	KEYCONFIG_TEXT_FIELD(left_arrow, "Left Arrow");
	KEYCONFIG_TEXT_FIELD(right_arrow, "Right Arrow");
	KEYCONFIG_TEXT_FIELD(down_arrow, "Down Arrow");
	KEYCONFIG_TEXT_FIELD(home_clear, "Home/Clear");
	KEYCONFIG_TEXT_FIELD(end_key, "End");
	KEYCONFIG_TEXT_FIELD(num_prefix, "Num ");
	KEYCONFIG_TEXT_FIELD(num_minus, "Num Minus");
	KEYCONFIG_TEXT_FIELD(num_divide, "Num Divide");
	KEYCONFIG_TEXT_FIELD(num_multiply, "Num Multiply");
	KEYCONFIG_TEXT_FIELD(num_plus, "Num Plus");
	KEYCONFIG_TEXT_FIELD(num_equals, "Num Equals");
	KEYCONFIG_TEXT_FIELD(num_comma, "Num Comma");
	KEYCONFIG_TEXT_FIELD(num_period, "Num Period");
	KEYCONFIG_TEXT_FIELD(nfer, "NFER");
	KEYCONFIG_TEXT_FIELD(stop, "STOP");
	KEYCONFIG_TEXT_FIELD(copy, "COPY");
	KEYCONFIG_TEXT_FIELD(shift, "Shift");
	KEYCONFIG_TEXT_FIELD(grph, "GRPH");
	KEYCONFIG_TEXT_FIELD(ctrl, "Ctrl");
	KEYCONFIG_TEXT_FIELD(unbound, "Unbound");
	KEYCONFIG_TEXT_FIELD(up_left, "Up-Left");
	KEYCONFIG_TEXT_FIELD(up, "Up");
	KEYCONFIG_TEXT_FIELD(up_right, "Up-Right");
	KEYCONFIG_TEXT_FIELD(left, "Left");
	KEYCONFIG_TEXT_FIELD(right, "Right");
	KEYCONFIG_TEXT_FIELD(down_left, "Down-Left");
	KEYCONFIG_TEXT_FIELD(down, "Down");
	KEYCONFIG_TEXT_FIELD(down_right, "Down-Right");
	KEYCONFIG_TEXT_FIELD(shot, "Shot");
	KEYCONFIG_TEXT_FIELD(bomb, "Bomb");
	KEYCONFIG_TEXT_FIELD(charge, "Charge");
	KEYCONFIG_TEXT_FIELD(autofire, "Autofire");
	KEYCONFIG_TEXT_FIELD(on, "On");
	KEYCONFIG_TEXT_FIELD(off, "Off");
	KEYCONFIG_TEXT_FIELD(defaults, "Defaults for this player");
	KEYCONFIG_TEXT_FIELD(defaults_story, "Defaults for Story movement");
	KEYCONFIG_TEXT_FIELD(apply, "Apply and return");
	KEYCONFIG_TEXT_FIELD(cancel, "Cancel");
	KEYCONFIG_TEXT_FIELD(header, "KEY CONFIGURATION     [ P");
	KEYCONFIG_TEXT_FIELD(header_end, " ]");
	KEYCONFIG_TEXT_FIELD(story_header, "KEY CONFIGURATION     [ STORY ]");
	KEYCONFIG_TEXT_FIELD(footer,
		"Left/Right: Column/Page   Z/Return: Edit   Esc: Cancel"
	);
	KEYCONFIG_TEXT_FIELD(discard, "Discard changes?  Z: Yes  Esc: No");
	KEYCONFIG_TEXT_FIELD(capture, "Press a key. Esc cancels capture.");
	KEYCONFIG_TEXT_FIELD(save_error, "Could not save TH3KEY.CFG");
	KEYCONFIG_TEXT_FIELD(asset_pf_fn, "azinn.dat");
	KEYCONFIG_TEXT_FIELD(restore_pf_fn, OP_AND_END_PF_FN);
};
#pragma option -a2

static const keyconfig_text_t far keyconfig_text = {
	T3_KEYCONFIG_FN, T3_KEYCONFIG_TEMP_FN, T3_KEYCONFIG_BACKUP_FN,
	"Minus", "Circumflex", "Yen", "Backspace", "Tab", "At",
	"LBracket", "Return", "Plus", "Asterisk", "RBracket", "Comma",
	"Period", "Slash", "Underscore", "Space", "XFER", "Roll Up",
	"Roll Down", "Insert", "Delete", "Up Arrow", "Left Arrow",
	"Right Arrow", "Down Arrow", "Home/Clear", "End", "Num ",
	"Num Minus", "Num Divide", "Num Multiply", "Num Plus", "Num Equals",
	"Num Comma", "Num Period", "NFER", "STOP", "COPY", "Shift", "GRPH",
	"Ctrl", "Unbound", "Up-Left", "Up", "Up-Right", "Left", "Right",
	"Down-Left", "Down", "Down-Right", "Shot", "Bomb", "Charge",
	"Autofire", "On", "Off", "Defaults for this player",
	"Defaults for Story", "Apply and return", "Cancel",
	"[ P", " ]",
	"[ STORY ]",
	"Left/Right: Column/Page   Z/Return: Edit   Esc: Cancel",
	"Discard changes?  Z: Yes  Esc: No",
	"Press a key. Esc cancels capture.", "Could not save TH3KEY.CFG",
	"azinn.dat", OP_AND_END_PF_FN
};
#undef KEYCONFIG_TEXT_FIELD

#pragma option -a1
struct keyconfig_file_t {
	char magic[8];
	uint16_t version;
	uint16_t size;
	uint8_t autofire;
	uint8_t bindings[T3_KEYCONFIG_BINDING_COUNT];
	uint8_t checksum;
};
#pragma option -a2

typedef char keyconfig_file_size_check[
	(sizeof(keyconfig_file_t) == 40) ? 1 : -1
];

struct keyconfig_menu_t {
	uint8_t autofire;
	uint8_t bindings[T3_KEYCONFIG_BINDING_COUNT];
};

static page_t keyconfig_page_front;

enum keyconfig_row_t {
	KCR_AUTOFIRE,
	KCR_UP_LEFT,
	KCR_UP,
	KCR_UP_RIGHT,
	KCR_LEFT,
	KCR_RIGHT,
	KCR_DOWN_LEFT,
	KCR_DOWN,
	KCR_DOWN_RIGHT,
	KCR_SHOT,
	KCR_BOMB,
	KCR_CHARGE,
	KCR_DEFAULTS,
	KCR_APPLY,
	KCR_CANCEL,
	KCR_COUNT,
};

enum keyconfig_story_row_t {
	KCSR_UP,
	KCSR_LEFT,
	KCSR_RIGHT,
	KCSR_DOWN,
	KCSR_SHOT,
	KCSR_BOMB,
	KCSR_CHARGE,
	KCSR_AUTOFIRE,
	KCSR_DEFAULTS,
	KCSR_APPLY,
	KCSR_CANCEL,
	KCSR_COUNT,
};

enum keyconfig_page_t {
	KCP_P1,
	KCP_P2,
	KCP_STORY,
	KCP_COUNT,
};

enum keyconfig_layout_t {
	KEYCONFIG_HEADER_TOP = 6,
	KEYCONFIG_ROWS_TOP = 8,
	KEYCONFIG_PLAYER_ACTION_TOP = 10,
	KEYCONFIG_COMMAND_TOP = 16,
	KEYCONFIG_FOOTER_TOP = 21,
	KEYCONFIG_STORY_LABEL_LEFT = 216,
	KEYCONFIG_STORY_VALUE_LEFT = 312,
	KEYCONFIG_PLAYER_ACTION_LABEL_LEFT = 100,
	KEYCONFIG_PLAYER_ACTION_VALUE_LEFT = 207,
	KEYCONFIG_PLAYER_MOVEMENT_LABEL_LEFT = 337,
	KEYCONFIG_PLAYER_MOVEMENT_VALUE_LEFT = 444,
	KEYCONFIG_COMMAND_CENTER = (RES_X / 2),
	KEYCONFIG_CURSOR_GAP = 16,
};

static uint8_t keyconfig_key_mask(uint8_t group)
{
	switch(group) {
	case 0:  return 0xFE; // Esc remains a fixed cancel key.
	case 10: return 0x7F;
	case 11: return 0x00;
	case 13: return 0x0F;
	case 14: return (K14_SHIFT | K14_GRPH | K14_CTRL);
	default: return 0xFF;
	}
}

static bool keyconfig_key_valid(uint8_t key)
{
	uint8_t group;
	uint8_t bit;

	if(key == T3_KEYCONFIG_KEY_UNBOUND) {
		return true;
	}
	group = (key >> 3);
	bit = (1 << (key & 7));
	return (
		(group < T3_KEYCONFIG_KEY_GROUP_COUNT) &&
		((keyconfig_key_mask(group) & bit) != 0)
	);
}

static uint8_t keyconfig_checksum(const keyconfig_file_t __ss& cfg)
{
	const uint8_t __ss *p = reinterpret_cast<const uint8_t __ss *>(&cfg);
	uint8_t checksum = 0xA7;

	for(unsigned int i = 0; i < offsetof(keyconfig_file_t, checksum); i++) {
		checksum ^= p[i];
	}
	return checksum;
}

static void keyconfig_defaults_set(
	keyconfig_menu_t __ss& cfg, uint8_t pid, bool all_players
)
{
	uint8_t first = (all_players ? 0 : pid);
	uint8_t end = (all_players ? T3_KEYCONFIG_PLAYER_COUNT : (pid + 1));

	for(uint8_t player = first; player < end; player++) {
		cfg.autofire &= ~(1 << player);
		for(uint8_t action = 0; action < T3_KEYCONFIG_ACTION_COUNT; action++) {
			cfg.bindings[(player * T3_KEYCONFIG_ACTION_COUNT) + action] =
				keyconfig_default_binding(player, action);
		}
	}
}

static void keyconfig_story_defaults_set(keyconfig_menu_t __ss& cfg)
{
	for(uint8_t action = 0; action < T3_KEYCONFIG_STORY_ACTION_COUNT; action++) {
		cfg.bindings[T3_KEYCONFIG_STORY_BINDINGS_INDEX + action] =
			keyconfig_default_story_binding(action);
	}
	cfg.autofire &= ~1;
	for(uint8_t player_action = KCA_SHOT; player_action <= KCA_CHARGE; player_action++) {
		cfg.bindings[player_action] = keyconfig_default_binding(0, player_action);
	}
}

static void keyconfig_resident_store(const keyconfig_menu_t __ss& cfg)
{
	keyconfig_resident_u8_set(
		T3_KEYCONFIG_RES_MAGIC_0_INDEX, T3_KEYCONFIG_RES_MAGIC_0
	);
	keyconfig_resident_u8_set(
		T3_KEYCONFIG_RES_MAGIC_1_INDEX, T3_KEYCONFIG_RES_MAGIC_1
	);
	keyconfig_resident_u8_set(
		T3_KEYCONFIG_RES_VERSION_INDEX, T3_KEYCONFIG_VERSION
	);
	for(uint8_t i = 0; i < T3_KEYCONFIG_BINDING_COUNT; i++) {
		keyconfig_resident_u8_set(
			T3_KEYCONFIG_RES_BINDINGS_INDEX + i, cfg.bindings[i]
		);
	}
	resident->autofire = (cfg.autofire & 0x03);
}

static void keyconfig_resident_load(keyconfig_menu_t __ss& cfg)
{
	cfg.autofire = (resident->autofire & 0x03);
	for(uint8_t i = 0; i < T3_KEYCONFIG_BINDING_COUNT; i++) {
		cfg.bindings[i] = keyconfig_resident_u8(
			T3_KEYCONFIG_RES_BINDINGS_INDEX + i
		);
	}
}

static bool keyconfig_binding_range_valid(
	const keyconfig_file_t __ss& cfg, uint8_t first, uint8_t end
)
{
	for(uint8_t i = first; i < end; i++) {
		if(!keyconfig_key_valid(cfg.bindings[i])) {
			return false;
		}
		if(cfg.bindings[i] == T3_KEYCONFIG_KEY_UNBOUND) {
			continue;
		}
		for(uint8_t j = first; j < i; j++) {
			if(cfg.bindings[i] == cfg.bindings[j]) {
				return false;
			}
		}
	}
	return true;
}

static bool keyconfig_file_valid(const keyconfig_file_t __ss& cfg)
{
	if(
		(cfg.magic[0] != 'T') || (cfg.magic[1] != '3') ||
		(cfg.magic[2] != 'K') || (cfg.magic[3] != 'E') ||
		(cfg.magic[4] != 'Y') || (cfg.magic[5] != '0') ||
		(cfg.magic[6] != '0') || (cfg.magic[7] != '2') ||
		(cfg.version != T3_KEYCONFIG_FILE_VERSION) ||
		(cfg.size != sizeof(cfg)) ||
		(cfg.autofire > 0x03) ||
		(cfg.checksum != keyconfig_checksum(cfg))
	) {
		return false;
	}
	return (
		keyconfig_binding_range_valid(
			cfg, 0, T3_KEYCONFIG_PLAYER_BINDING_COUNT
		) &&
		keyconfig_binding_range_valid(
			cfg, T3_KEYCONFIG_STORY_BINDINGS_INDEX,
			T3_KEYCONFIG_BINDING_COUNT
		)
	);
}

static bool keyconfig_file_rename(
	const char far *old_fn, const char far *new_fn
)
{
	asm {
		push	ds
		push	es
		push	di
		lds 	dx, old_fn
		les 	di, new_fn
		mov 	ax, 5600h
		int 	21h
		pop 	di
		pop 	es
		pop 	ds
	}
	return !FLAGS_CARRY;
}

static void keyconfig_file_delete(const char far *fn)
{
	asm {
		push	ds
		lds 	dx, fn
		mov 	ah, 41h
		int 	21h
		pop 	ds
	}
}

static bool keyconfig_file_save(const keyconfig_menu_t __ss& menu)
{
	keyconfig_file_t cfg;
	bool had_old;

	cfg.magic[0] = 'T'; cfg.magic[1] = '3';
	cfg.magic[2] = 'K'; cfg.magic[3] = 'E';
	cfg.magic[4] = 'Y'; cfg.magic[5] = '0';
	cfg.magic[6] = '0'; cfg.magic[7] = '2';
	cfg.version = T3_KEYCONFIG_FILE_VERSION;
	cfg.size = sizeof(cfg);
	cfg.autofire = (menu.autofire & 0x03);
	for(uint8_t i = 0; i < T3_KEYCONFIG_BINDING_COUNT; i++) {
		cfg.bindings[i] = menu.bindings[i];
	}
	cfg.checksum = keyconfig_checksum(cfg);

	keyconfig_file_delete(keyconfig_text.temp_fn);
	if(!file_create(keyconfig_text.temp_fn)) {
		return false;
	}
	if(!file_write(&cfg, sizeof(cfg))) {
		file_close();
		keyconfig_file_delete(keyconfig_text.temp_fn);
		return false;
	}
	file_close();

	keyconfig_file_delete(keyconfig_text.backup_fn);
	had_old = (file_ropen(keyconfig_text.fn) != 0);
	if(had_old) {
		file_close();
	}
	if(
		had_old &&
		!keyconfig_file_rename(keyconfig_text.fn, keyconfig_text.backup_fn)
	) {
		keyconfig_file_delete(keyconfig_text.temp_fn);
		return false;
	}
	if(!keyconfig_file_rename(keyconfig_text.temp_fn, keyconfig_text.fn)) {
		if(had_old) {
			keyconfig_file_rename(keyconfig_text.backup_fn, keyconfig_text.fn);
		}
		keyconfig_file_delete(keyconfig_text.temp_fn);
		return false;
	}
	keyconfig_file_delete(keyconfig_text.backup_fn);
	return true;
}

void far keyconfig_load(bool legacy_autofire)
{
	keyconfig_file_t file_cfg;
	keyconfig_menu_t menu;
	bool present = false;
	bool loaded = false;

	if(file_ropen(keyconfig_text.fn)) {
		present = true;
		loaded = (
			(file_read(&file_cfg, sizeof(file_cfg)) == sizeof(file_cfg)) &&
			keyconfig_file_valid(file_cfg)
		);
		file_close();
	}
	if(loaded) {
		menu.autofire = file_cfg.autofire;
		for(uint8_t i = 0; i < T3_KEYCONFIG_BINDING_COUNT; i++) {
			menu.bindings[i] = file_cfg.bindings[i];
		}
	} else {
		menu.autofire = 0;
		keyconfig_defaults_set(menu, 0, true);
		keyconfig_story_defaults_set(menu);
		if(!present && legacy_autofire) {
			menu.autofire = 0x03;
		}
	}
	keyconfig_resident_store(menu);
}

static void keyconfig_line_clear(char __ss *line)
{
	for(uint8_t i = 0; i < 64; i++) {
		line[i] = ' ';
	}
	line[64] = '\0';
}

static uint8_t keyconfig_line_puts(
	char __ss *line, uint8_t at, const char far *s
)
{
	while(*s != '\0') {
		line[at++] = *s++;
	}
	return at;
}

static char keyconfig_key_letter(uint8_t key)
{
	switch(key) {
	case 16: return 'Q'; case 17: return 'W'; case 18: return 'E';
	case 19: return 'R'; case 20: return 'T'; case 21: return 'Y';
	case 22: return 'U'; case 23: return 'I'; case 24: return 'O';
	case 25: return 'P'; case 29: return 'A'; case 30: return 'S';
	case 31: return 'D'; case 32: return 'F'; case 33: return 'G';
	case 34: return 'H'; case 35: return 'J'; case 36: return 'K';
	case 37: return 'L'; case 41: return 'Z'; case 42: return 'X';
	case 43: return 'C'; case 44: return 'V'; case 45: return 'B';
	case 46: return 'N'; case 47: return 'M'; default: return '\0';
	}
}

static uint8_t keyconfig_line_put_key_name(
	char __ss *line, uint8_t at, uint8_t key
)
{
	char letter = keyconfig_key_letter(key);

	if((key >= 1) && (key <= 9)) {
		line[at++] = static_cast<char>('0' + key);
		return at;
	}
	if(key == 10) {
		line[at++] = '0';
		return at;
	}
	if(letter != '\0') {
		line[at++] = letter;
		return at;
	}
	if(
		((key >= 66) && (key <= 68)) ||
		((key >= 70) && (key <= 72)) ||
		((key >= 74) && (key <= 76)) ||
		(key == 78)
	) {
		at = keyconfig_line_puts(line, at, keyconfig_text.num_prefix);
		if(key <= 68) {
			line[at++] = static_cast<char>('7' + (key - 66));
		} else if(key <= 72) {
			line[at++] = static_cast<char>('4' + (key - 70));
		} else if(key <= 76) {
			line[at++] = static_cast<char>('1' + (key - 74));
		} else {
			line[at++] = '0';
		}
		return at;
	}
	if((key >= 82) && (key <= 86)) {
		line[at++] = 'V';
		line[at++] = 'F';
		line[at++] = static_cast<char>('1' + (key - 82));
		return at;
	}
	if((key >= 98) && (key <= 107)) {
		line[at++] = 'F';
		if(key == 107) {
			line[at++] = '1';
			line[at++] = '0';
		} else {
			line[at++] = static_cast<char>('1' + (key - 98));
		}
		return at;
	}
	switch(key) {
	case 11: return keyconfig_line_puts(line, at, keyconfig_text.minus);
	case 12: return keyconfig_line_puts(line, at, keyconfig_text.circumflex);
	case 13: return keyconfig_line_puts(line, at, keyconfig_text.yen);
	case 14: return keyconfig_line_puts(line, at, keyconfig_text.backspace);
	case 15: return keyconfig_line_puts(line, at, keyconfig_text.tab);
	case 26: return keyconfig_line_puts(line, at, keyconfig_text.at);
	case 27: return keyconfig_line_puts(line, at, keyconfig_text.lbracket);
	case 28: return keyconfig_line_puts(line, at, keyconfig_text.return_key);
	case 38: return keyconfig_line_puts(line, at, keyconfig_text.plus);
	case 39: return keyconfig_line_puts(line, at, keyconfig_text.asterisk);
	case 40: return keyconfig_line_puts(line, at, keyconfig_text.rbracket);
	case 48: return keyconfig_line_puts(line, at, keyconfig_text.comma);
	case 49: return keyconfig_line_puts(line, at, keyconfig_text.period);
	case 50: return keyconfig_line_puts(line, at, keyconfig_text.slash);
	case 51: return keyconfig_line_puts(line, at, keyconfig_text.underscore);
	case 52: return keyconfig_line_puts(line, at, keyconfig_text.space);
	case 53: return keyconfig_line_puts(line, at, keyconfig_text.xfer);
	case 54: return keyconfig_line_puts(line, at, keyconfig_text.roll_up);
	case 55: return keyconfig_line_puts(line, at, keyconfig_text.roll_down);
	case 56: return keyconfig_line_puts(line, at, keyconfig_text.insert_key);
	case 57: return keyconfig_line_puts(line, at, keyconfig_text.delete_key);
	case 58: return keyconfig_line_puts(line, at, keyconfig_text.up_arrow);
	case 59: return keyconfig_line_puts(line, at, keyconfig_text.left_arrow);
	case 60: return keyconfig_line_puts(line, at, keyconfig_text.right_arrow);
	case 61: return keyconfig_line_puts(line, at, keyconfig_text.down_arrow);
	case 62: return keyconfig_line_puts(line, at, keyconfig_text.home_clear);
	case 63: return keyconfig_line_puts(line, at, keyconfig_text.end_key);
	case 64: return keyconfig_line_puts(line, at, keyconfig_text.num_minus);
	case 65: return keyconfig_line_puts(line, at, keyconfig_text.num_divide);
	case 69: return keyconfig_line_puts(line, at, keyconfig_text.num_multiply);
	case 73: return keyconfig_line_puts(line, at, keyconfig_text.num_plus);
	case 77: return keyconfig_line_puts(line, at, keyconfig_text.num_equals);
	case 79: return keyconfig_line_puts(line, at, keyconfig_text.num_comma);
	case 80: return keyconfig_line_puts(line, at, keyconfig_text.num_period);
	case 81: return keyconfig_line_puts(line, at, keyconfig_text.nfer);
	case 96: return keyconfig_line_puts(line, at, keyconfig_text.stop);
	case 97: return keyconfig_line_puts(line, at, keyconfig_text.copy);
	case 112: return keyconfig_line_puts(line, at, keyconfig_text.shift);
	case 115: return keyconfig_line_puts(line, at, keyconfig_text.grph);
	case 116: return keyconfig_line_puts(line, at, keyconfig_text.ctrl);
	default: return keyconfig_line_puts(line, at, keyconfig_text.unbound);
	}
}

static uint8_t keyconfig_line_put_action_name(
	char __ss *line, uint8_t at, uint8_t action
)
{
	switch(action) {
	case KCA_UP_LEFT: return keyconfig_line_puts(line, at, keyconfig_text.up_left);
	case KCA_UP: return keyconfig_line_puts(line, at, keyconfig_text.up);
	case KCA_UP_RIGHT: return keyconfig_line_puts(line, at, keyconfig_text.up_right);
	case KCA_LEFT: return keyconfig_line_puts(line, at, keyconfig_text.left);
	case KCA_RIGHT: return keyconfig_line_puts(line, at, keyconfig_text.right);
	case KCA_DOWN_LEFT: return keyconfig_line_puts(line, at, keyconfig_text.down_left);
	case KCA_DOWN: return keyconfig_line_puts(line, at, keyconfig_text.down);
	case KCA_DOWN_RIGHT: return keyconfig_line_puts(line, at, keyconfig_text.down_right);
	case KCA_SHOT: return keyconfig_line_puts(line, at, keyconfig_text.shot);
	case KCA_BOMB: return keyconfig_line_puts(line, at, keyconfig_text.bomb);
	default: return keyconfig_line_puts(line, at, keyconfig_text.charge);
	}
}

static void keyconfig_background_load(void)
{
	char pi_fn[8];
	uint32_t __ss *pi_fn_quads = reinterpret_cast<uint32_t __ss *>(pi_fn);

	pfend();
	pfstart(reinterpret_cast<const unsigned char far *>(keyconfig_text.asset_pf_fn));
	pi_fn_quads[0] = 0x2E316762UL; // "bg1."
	pi_fn_quads[1] = 0x00006970UL; // "pi"
	// The title screen releases slot 0 by value, leaving its pointer stale.
	// KeyConfig owns a fresh full-screen PI and must not let pi_load() release
	// whichever CDG or archive block later reused that segment.
	pi_buffers[0] = nullptr;
	pi_load(0, pi_fn);
	pfend();
	pfstart(reinterpret_cast<const unsigned char far *>(keyconfig_text.restore_pf_fn));

	// Keep the first complete KeyConfig frame black until its fade-in begins.
	PaletteTone = 0;
	palette_set_all(pi_headers[0].palette);
	palette_show();
}

// Preserve later menu entry points after removing BG1 palette overrides.
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90"

static void keyconfig_background_put(void)
{
	pi_put_8(0, 0, 0);
}

static void keyconfig_graphics_row_put(
	char __ss *line,
	uint8_t label_end,
	uint8_t value_at,
	screen_x_t label_left,
	screen_x_t value_left,
	uint8_t top,
	bool selected,
	bool restore
)
{
	vram_y_t y = (top * GLYPH_H);
	int label_color = (
		selected ? KEYCONFIG_COLOR_SELECTED : KEYCONFIG_COLOR_LABEL
	);

	if(restore) {
		menu_font_restore_rect(0, y, RES_X, GLYPH_H);
	}
	line[label_end] = '\0';
	if(value_at != 0) {
		menu_font_put(
			value_left, y, &line[value_at],
			(selected ? KEYCONFIG_COLOR_SELECTED : KEYCONFIG_COLOR_VALUE)
		);
	} else {
		label_left = (
			KEYCONFIG_COMMAND_CENTER - (menu_font_width(&line[2]) / 2)
		);
	}
	if(selected) {
		line[1] = '\0';
		menu_font_put(
			(label_left - KEYCONFIG_CURSOR_GAP), y, line,
			KEYCONFIG_COLOR_SELECTED
		);
	}
	menu_font_put(label_left, y, &line[2], label_color);
}

static void keyconfig_text_putsa(
	tram_x_t left, tram_y_t top, const char far *s, tram_atrb2 atrb
)
{
	char line[65];
	uint8_t at;

	if(menu_font) {
		graph_accesspage(keyconfig_page_front);
		menu_font_put_centered(
			(RES_X / 2), (top * GLYPH_H), s, KEYCONFIG_COLOR_FOOTER
		);
		return;
	}
	keyconfig_line_clear(line);
	text_putsa(8, top, line, TX_WHITE);
	at = keyconfig_line_puts(line, 0, s);
	line[at] = '\0';
	text_putsa(left, top, line, atrb);
}

static void keyconfig_player_row_put(
	keyconfig_menu_t __ss& cfg, uint8_t pid, uint8_t row, bool selected,
	bool restore
)
{
	char line[65];
	uint8_t at = 2;
	uint8_t label_end;
	uint8_t value_at = 0;
	uint8_t top;
	screen_x_t label_left;
	screen_x_t value_left;

	if(row == KCR_AUTOFIRE) {
		top = (KEYCONFIG_PLAYER_ACTION_TOP + 3);
		label_left = KEYCONFIG_PLAYER_ACTION_LABEL_LEFT;
		value_left = KEYCONFIG_PLAYER_ACTION_VALUE_LEFT;
	} else if(row <= KCR_DOWN_RIGHT) {
		top = (KEYCONFIG_ROWS_TOP + (row - KCR_UP_LEFT));
		label_left = KEYCONFIG_PLAYER_MOVEMENT_LABEL_LEFT;
		value_left = KEYCONFIG_PLAYER_MOVEMENT_VALUE_LEFT;
	} else if(row <= KCR_CHARGE) {
		top = (KEYCONFIG_PLAYER_ACTION_TOP + (row - KCR_SHOT));
		label_left = KEYCONFIG_PLAYER_ACTION_LABEL_LEFT;
		value_left = KEYCONFIG_PLAYER_ACTION_VALUE_LEFT;
	} else {
		top = (KEYCONFIG_COMMAND_TOP + (row - KCR_DEFAULTS));
		label_left = 0;
		value_left = 0;
	}

	keyconfig_line_clear(line);
	line[0] = (selected ? '>' : ' ');
	if(row == KCR_AUTOFIRE) {
		at = keyconfig_line_puts(line, at, keyconfig_text.autofire);
		label_end = at;
		at = 24;
		value_at = at;
		at = keyconfig_line_puts(
			line,
			at,
			((cfg.autofire & (1 << pid)) ? keyconfig_text.on : keyconfig_text.off)
		);
	} else if((row >= KCR_UP_LEFT) && (row <= KCR_CHARGE)) {
		uint8_t action = (row - KCR_UP_LEFT);
		at = keyconfig_line_put_action_name(line, at, action);
		label_end = at;
		at = 24;
		value_at = at;
		at = keyconfig_line_put_key_name(
			line,
			at,
			cfg.bindings[(pid * T3_KEYCONFIG_ACTION_COUNT) + action]
		);
	} else if(row == KCR_DEFAULTS) {
		at = keyconfig_line_puts(line, at, keyconfig_text.defaults);
		label_end = at;
	} else if(row == KCR_APPLY) {
		at = keyconfig_line_puts(line, at, keyconfig_text.apply);
		label_end = at;
	} else {
		at = keyconfig_line_puts(line, at, keyconfig_text.cancel);
		label_end = at;
	}
	if(menu_font) {
		line[at] = '\0';
		keyconfig_graphics_row_put(
			line, label_end, value_at, label_left, value_left, top, selected,
			restore
		);
	} else {
		text_putsa(8, top, line, (selected ? TX_CYAN : TX_WHITE));
	}
}

static void keyconfig_story_row_put(
	keyconfig_menu_t __ss& cfg, uint8_t row, bool selected, bool restore
)
{
	char line[65];
	uint8_t at = 2;
	uint8_t label_end;
	uint8_t value_at = 0;

	keyconfig_line_clear(line);
	line[0] = (selected ? '>' : ' ');
	if(row <= KCSR_DOWN) {
		uint8_t player_action;
		switch(row) {
		case KCSR_UP:    player_action = KCA_UP;    break;
		case KCSR_LEFT:  player_action = KCA_LEFT;  break;
		case KCSR_RIGHT: player_action = KCA_RIGHT; break;
		default:         player_action = KCA_DOWN;  break;
		}
		at = keyconfig_line_put_action_name(line, at, player_action);
		label_end = at;
		at = 24;
		value_at = at;
		at = keyconfig_line_put_key_name(
			line, at,
			cfg.bindings[T3_KEYCONFIG_STORY_BINDINGS_INDEX + row]
		);
	} else if((row >= KCSR_SHOT) && (row <= KCSR_CHARGE)) {
		uint8_t player_action = (KCA_SHOT + (row - KCSR_SHOT));
		at = keyconfig_line_put_action_name(line, at, player_action);
		label_end = at;
		at = 24;
		value_at = at;
		at = keyconfig_line_put_key_name(
			line, at, cfg.bindings[player_action]
		);
	} else if(row == KCSR_AUTOFIRE) {
		at = keyconfig_line_puts(line, at, keyconfig_text.autofire);
		label_end = at;
		at = 24;
		value_at = at;
		at = keyconfig_line_puts(
			line, at, ((cfg.autofire & 1) ? keyconfig_text.on : keyconfig_text.off)
		);
	} else if(row == KCSR_DEFAULTS) {
		at = keyconfig_line_puts(line, at, keyconfig_text.defaults_story);
		label_end = at;
	} else if(row == KCSR_APPLY) {
		at = keyconfig_line_puts(line, at, keyconfig_text.apply);
		label_end = at;
	} else {
		at = keyconfig_line_puts(line, at, keyconfig_text.cancel);
		label_end = at;
	}
	if(menu_font) {
		line[at] = '\0';
		keyconfig_graphics_row_put(
			line, label_end, value_at, KEYCONFIG_STORY_LABEL_LEFT,
			KEYCONFIG_STORY_VALUE_LEFT, (KEYCONFIG_ROWS_TOP + row), selected,
			restore
		);
	} else {
		text_putsa(
			8, (KEYCONFIG_ROWS_TOP + row), line,
			(selected ? TX_CYAN : TX_WHITE)
		);
	}
}

static uint8_t keyconfig_page_row_count(uint8_t page)
{
	return ((page == KCP_STORY) ? KCSR_COUNT : KCR_COUNT);
}

static uint8_t keyconfig_player_selected_row(bool movement_column, uint8_t at)
{
	if(movement_column) {
		return (
			(at < 8) ? (KCR_UP_LEFT + at) : (KCR_DEFAULTS + (at - 8))
		);
	}
	if(at < 3) {
		return (KCR_SHOT + at);
	}
	return ((at == 3) ? KCR_AUTOFIRE : (KCR_DEFAULTS + (at - 4)));
}

static void keyconfig_row_put(
	keyconfig_menu_t __ss& cfg, uint8_t page, uint8_t row, bool selected,
	bool restore
)
{
	if(page == KCP_STORY) {
		keyconfig_story_row_put(cfg, row, selected, restore);
	} else {
		keyconfig_player_row_put(cfg, page, row, selected, restore);
	}
}

static void keyconfig_screen_put(
	keyconfig_menu_t __ss& cfg, uint8_t page, uint8_t selected
)
{
	char line[65];
	uint8_t at;
	bool restore = false;

	text_clear();
	// Compose on the hidden page while the previous complete frame remains
	// visible. This avoids both graph_copy_page()'s allocation and a blank
	// intermediate frame.
	graph_accesspage(!keyconfig_page_front);
	keyconfig_background_put();
	keyconfig_line_clear(line);
	at = 0;
	if(page == KCP_STORY) {
		at = keyconfig_line_puts(line, at, keyconfig_text.story_header);
	} else {
		at = keyconfig_line_puts(line, at, keyconfig_text.header);
		line[at++] = static_cast<char>('1' + page);
		at = keyconfig_line_puts(line, at, keyconfig_text.header_end);
	}
	line[at] = '\0';
	if(menu_font) {
		menu_font_put_centered(
			(RES_X / 2), (KEYCONFIG_HEADER_TOP * GLYPH_H), line,
			KEYCONFIG_COLOR_HEADER
		);
	} else {
		text_putsa(25, KEYCONFIG_HEADER_TOP, line, TX_WHITE);
	}
	for(uint8_t row = 0; row < keyconfig_page_row_count(page); row++) {
		keyconfig_row_put(cfg, page, row, (row == selected), restore);
	}
	keyconfig_line_clear(line);
	at = 0;
	at = keyconfig_line_puts(line, at, keyconfig_text.footer);
	line[at] = '\0';
	if(menu_font) {
		menu_font_put_centered(
			(RES_X / 2), (KEYCONFIG_FOOTER_TOP * GLYPH_H), line,
			KEYCONFIG_COLOR_FOOTER
		);
	} else {
		text_putsa(12, KEYCONFIG_FOOTER_TOP, line, TX_WHITE);
	}
	vsync_wait();
	graph_showpage(!keyconfig_page_front);
	keyconfig_page_front = !keyconfig_page_front;
	graph_accesspage(keyconfig_page_front);
}

// Preserve all following KeyConfig entry points after removing the allocating
// full-page copy.
#pragma codestring "\x90\x90"

static void keyconfig_screen_clear(void)
{
	keyconfig_palette_fade_out();
	text_clear();
	pi_free(0);
	graph_accesspage(0);
	graph_clear();
	graph_accesspage(1);
	graph_clear();
	graph_showpage(0);
	graph_accesspage(0);
}

static uint8_t keyconfig_raw_first(void)
{
	if(peekb(0, KEYGROUP_0) & K0_ESC) {
		return T3_KEYCONFIG_CAPTURE_CANCEL;
	}
	for(uint8_t group = 0; group < T3_KEYCONFIG_KEY_GROUP_COUNT; group++) {
		uint8_t pressed = (peekb(0, (KEYGROUP_0 + group)) &
			keyconfig_key_mask(group));
		for(uint8_t bit = 0; bit < 8; bit++) {
			if(pressed & (1 << bit)) {
				return keyconfig_key(group, bit);
			}
		}
	}
	return T3_KEYCONFIG_KEY_UNBOUND;
}

static void keyconfig_raw_wait_release(void)
{
	while(keyconfig_raw_first() != T3_KEYCONFIG_KEY_UNBOUND) {
		frame_delay(1);
	}
}

static uint8_t keyconfig_capture(void)
{
	uint8_t key;

	keyconfig_raw_wait_release();
	while(1) {
		key = keyconfig_raw_first();
		if(key != T3_KEYCONFIG_KEY_UNBOUND) {
			break;
		}
		frame_delay(1);
	}
	keyconfig_raw_wait_release();
	return key;
}

static void keyconfig_binding_assign(
	keyconfig_menu_t __ss& cfg, uint8_t index, uint8_t key
)
{
	uint8_t old_key = cfg.bindings[index];
	uint8_t first = (
		(index < T3_KEYCONFIG_PLAYER_BINDING_COUNT) ?
		0 : T3_KEYCONFIG_STORY_BINDINGS_INDEX
	);
	uint8_t end = (
		(index < T3_KEYCONFIG_PLAYER_BINDING_COUNT) ?
		T3_KEYCONFIG_PLAYER_BINDING_COUNT : T3_KEYCONFIG_BINDING_COUNT
	);

	for(uint8_t i = first; i < end; i++) {
		if((i != index) && (cfg.bindings[i] == key)) {
			cfg.bindings[i] = old_key;
			break;
		}
	}
	cfg.bindings[index] = key;
}

static bool keyconfig_menu_equal(
	const keyconfig_menu_t __ss& a, const keyconfig_menu_t __ss& b
)
{
	if(a.autofire != b.autofire) {
		return false;
	}
	for(uint8_t i = 0; i < T3_KEYCONFIG_BINDING_COUNT; i++) {
		if(a.bindings[i] != b.bindings[i]) {
			return false;
		}
	}
	return true;
}

static bool keyconfig_discard_confirm(void)
{
	input_t input_prev = input_sp;

	keyconfig_text_putsa(25, 20, keyconfig_text.discard, TX_CYAN);
	while(1) {
		input_mode_interface();
		if(input_prev == INPUT_NONE) {
			if(input_sp & (INPUT_OK | INPUT_SHOT)) {
				return true;
			}
			if(input_sp & INPUT_CANCEL) {
				return false;
			}
		}
		input_prev = input_sp;
		frame_delay(1);
	}
}

#pragma codestring "\x90"

bool far keyconfig_menu(void)
{
	t3pix_scene_set(T3PIX_SCENE_KEY_CONFIG);
	keyconfig_menu_t original;
	keyconfig_menu_t cfg;
	uint8_t page = KCP_STORY;
	uint8_t selected = KCSR_UP;
	uint8_t player_at = 0;
	bool movement_column = false;
	input_t input_prev;

	keyconfig_resident_load(original);
	cfg = original;
	text_clear();
	graph_accesspage(0);
	graph_clear();
	graph_accesspage(1);
	graph_clear();
	graph_showpage(0);
	graph_accesspage(0);
	keyconfig_page_front = 0;
	keyconfig_background_load();
	keyconfig_screen_put(cfg, page, selected);
	keyconfig_palette_fade_in();

	input_mode_interface();
	input_prev = input_sp;
	while(1) {
		input_mode_interface();
		if(input_prev == INPUT_NONE) {
			if(input_sp & INPUT_UP) {
				if(page == KCP_STORY) {
					selected = (
						(selected == 0) ? (KCSR_COUNT - 1) : (selected - 1)
					);
				} else {
					uint8_t count = (movement_column ? 11 : 7);
					player_at = (
						(player_at == 0) ? (count - 1) : (player_at - 1)
					);
					selected = keyconfig_player_selected_row(
						movement_column, player_at
					);
				}
				keyconfig_screen_put(cfg, page, selected);
			} else if(input_sp & INPUT_DOWN) {
				if(page == KCP_STORY) {
					selected = (
						(selected == (KCSR_COUNT - 1)) ? 0 : (selected + 1)
					);
				} else {
					uint8_t count = (movement_column ? 11 : 7);
					player_at = (
						(player_at == (count - 1)) ? 0 : (player_at + 1)
					);
					selected = keyconfig_player_selected_row(
						movement_column, player_at
					);
				}
				keyconfig_screen_put(cfg, page, selected);
			} else if(input_sp & (INPUT_LEFT | INPUT_RIGHT)) {
				bool left = ((input_sp & INPUT_LEFT) != 0);
				bool page_changed = false;
				bool command = (
					(page != KCP_STORY) && (selected >= KCR_DEFAULTS)
				);

				if(
					!command && (page != KCP_STORY) &&
					((left && movement_column) || (!left && !movement_column))
				) {
					if(movement_column) {
						movement_column = false;
						player_at = (
							(player_at < 2) ? 0 :
							((player_at > 5) ? 3 : (player_at - 2))
						);
					} else {
						movement_column = true;
						player_at += 2;
					}
					selected = keyconfig_player_selected_row(
						movement_column, player_at
					);
				} else if(left) {
					page = ((page == 0) ? (KCP_COUNT - 1) : (page - 1));
					page_changed = true;
				} else {
					page = ((page == (KCP_COUNT - 1)) ? 0 : (page + 1));
					page_changed = true;
				}
				if(page_changed) {
					if(page == KCP_STORY) {
						selected = KCSR_UP;
					} else {
						movement_column = false;
						player_at = 0;
						selected = KCR_SHOT;
					}
				}
				keyconfig_screen_put(cfg, page, selected);
			} else if(input_sp & (INPUT_OK | INPUT_SHOT)) {
				if((page != KCP_STORY) && (selected == KCR_AUTOFIRE)) {
					cfg.autofire ^= (1 << page);
					keyconfig_screen_put(cfg, page, selected);
				} else if(
					(page == KCP_STORY) && (selected == KCSR_AUTOFIRE)
				) {
					cfg.autofire ^= 1;
					keyconfig_screen_put(cfg, page, selected);
				} else if(
					(page != KCP_STORY) &&
					(selected >= KCR_UP_LEFT) &&
					(selected <= KCR_CHARGE)
				) {
					uint8_t action = (selected - KCR_UP_LEFT);
					uint8_t key;
					keyconfig_text_putsa(18, 20, keyconfig_text.capture, TX_CYAN);
					key = keyconfig_capture();
					if(key != T3_KEYCONFIG_CAPTURE_CANCEL) {
						keyconfig_binding_assign(
							cfg,
							(page * T3_KEYCONFIG_ACTION_COUNT) + action,
							key
						);
					}
					keyconfig_screen_put(cfg, page, selected);
					input_prev = INPUT_NONE;
				} else if((page == KCP_STORY) && (selected <= KCSR_CHARGE)) {
					uint8_t index = (
						(selected <= KCSR_DOWN) ?
						(T3_KEYCONFIG_STORY_BINDINGS_INDEX + selected) :
						(KCA_SHOT + (selected - KCSR_SHOT))
					);
					uint8_t key;
					keyconfig_text_putsa(18, 20, keyconfig_text.capture, TX_CYAN);
					key = keyconfig_capture();
					if(key != T3_KEYCONFIG_CAPTURE_CANCEL) {
						keyconfig_binding_assign(cfg, index, key);
					}
					keyconfig_screen_put(cfg, page, selected);
					input_prev = INPUT_NONE;
				} else if(
					((page != KCP_STORY) && (selected == KCR_DEFAULTS)) ||
					((page == KCP_STORY) && (selected == KCSR_DEFAULTS))
				) {
					if(page == KCP_STORY) {
						keyconfig_story_defaults_set(cfg);
					} else {
						keyconfig_defaults_set(cfg, page, false);
					}
					keyconfig_screen_put(cfg, page, selected);
				} else if(
					((page != KCP_STORY) && (selected == KCR_APPLY)) ||
					((page == KCP_STORY) && (selected == KCSR_APPLY))
				) {
					if(keyconfig_file_save(cfg)) {
						keyconfig_resident_store(cfg);
						keyconfig_screen_clear();
						return true;
					}
					keyconfig_text_putsa(23, 20, keyconfig_text.save_error, TX_CYAN);
				} else if(
					keyconfig_menu_equal(cfg, original) ||
					keyconfig_discard_confirm()
				) {
					keyconfig_screen_clear();
					return false;
				} else {
					keyconfig_screen_put(cfg, page, selected);
					input_prev = INPUT_NONE;
				}
			} else if(input_sp & INPUT_CANCEL) {
				if(
					keyconfig_menu_equal(cfg, original) ||
					keyconfig_discard_confirm()
				) {
					keyconfig_screen_clear();
					return false;
				}
				keyconfig_screen_put(cfg, page, selected);
				input_prev = INPUT_NONE;
			}
		}
		input_prev = input_sp;
		frame_delay(1);
	}
}

// Preserve the paragraph phase of all following code segments.
#pragma codestring "\x90"
