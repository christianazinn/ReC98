#pragma option -zCKEYCFG_I_TEXT
#pragma option -zPgroup_KEYCONFIG_INPUT

#include <stddef.h>
#include "x86real.h"
#include "libs/master.lib/master.hpp"
#include "platform/x86real/pc98/keyboard.hpp"
#include "th04/hardware/input.h"
#include "th04/keyconfig.hpp"

#include "th01/keydos.inc"

// Materialize into appended BSS; ordinary literals would grow stock DATA.
struct keyconfig_strings_t {
	char text_t5key001[9];
	char text_t4key001[9];
	char text_th4key_cfg[11];
};
static keyconfig_strings_t keyconfig_strings;
static bool keyconfig_strings_ready;

static void keyconfig_strings_init(void)
{
	if(keyconfig_strings_ready) return;
	keyconfig_strings.text_t5key001[0] = 84; keyconfig_strings.text_t5key001[1] = 53;
	keyconfig_strings.text_t5key001[2] = 75; keyconfig_strings.text_t5key001[3] = 69;
	keyconfig_strings.text_t5key001[4] = 89; keyconfig_strings.text_t5key001[5] = 48;
	keyconfig_strings.text_t5key001[6] = 48; keyconfig_strings.text_t5key001[7] = 49;
	keyconfig_strings.text_t5key001[8] = 0; keyconfig_strings.text_t4key001[0] = 84;
	keyconfig_strings.text_t4key001[1] = 52; keyconfig_strings.text_t4key001[2] = 75;
	keyconfig_strings.text_t4key001[3] = 69; keyconfig_strings.text_t4key001[4] = 89;
	keyconfig_strings.text_t4key001[5] = 48; keyconfig_strings.text_t4key001[6] = 48;
	keyconfig_strings.text_t4key001[7] = 49; keyconfig_strings.text_t4key001[8] = 0;
	keyconfig_strings.text_th4key_cfg[0] = 84; keyconfig_strings.text_th4key_cfg[1] = 72;
#if (GAME == 5)
	keyconfig_strings.text_th4key_cfg[2] = 53;
#else
	keyconfig_strings.text_th4key_cfg[2] = 52;
#endif
	keyconfig_strings.text_th4key_cfg[3] = 75; keyconfig_strings.text_th4key_cfg[4] = 69;
	keyconfig_strings.text_th4key_cfg[5] = 89; keyconfig_strings.text_th4key_cfg[6] = 46;
	keyconfig_strings.text_th4key_cfg[7] = 67; keyconfig_strings.text_th4key_cfg[8] = 70;
	keyconfig_strings.text_th4key_cfg[9] = 71; keyconfig_strings.text_th4key_cfg[10] = 0;

	keyconfig_strings_ready = true;
}

static bool keyconfig_loaded;
static uint8_t keyconfig_bindings[KEYCONFIG_BINDING_COUNT];

static uint8_t keyconfig_default_binding(uint8_t action, uint8_t alternate)
{
	if(alternate) {
		switch(action) {
		case KCA_UP:    return keyconfig_key(8, 3); // Num8
		case KCA_DOWN:  return keyconfig_key(9, 3); // Num2
		case KCA_LEFT:  return keyconfig_key(8, 6); // Num4
		case KCA_RIGHT: return keyconfig_key(9, 0); // Num6
		case KCA_SHOT:  return keyconfig_key(6, 4); // Space
		default:        return KEYCONFIG_KEY_UNBOUND;
		}
	}
	switch(action) {
	case KCA_UP_LEFT:    return keyconfig_key(8, 2); // Num7
	case KCA_UP:         return keyconfig_key(7, 2); // Up arrow
	case KCA_UP_RIGHT:   return keyconfig_key(8, 4); // Num9
	case KCA_LEFT:       return keyconfig_key(7, 3); // Left arrow
	case KCA_RIGHT:      return keyconfig_key(7, 4); // Right arrow
	case KCA_DOWN_LEFT:  return keyconfig_key(9, 2); // Num1
	case KCA_DOWN:       return keyconfig_key(7, 5); // Down arrow
	case KCA_DOWN_RIGHT: return keyconfig_key(9, 4); // Num3
	case KCA_SHOT:       return keyconfig_key(5, 1); // Z
	default:             return keyconfig_key(5, 2); // X
	}
}

static uint8_t keyconfig_key_mask(uint8_t group)
{
	switch(group) {
	case 0:  return 0xFE;
	case 2:  return 0xF7;
	case 10: return 0x7F;
	case 11: return 0x00;
	case 13: return 0x0F;
	case 14: return (K14_SHIFT | K14_GRPH | K14_CTRL);
	default: return 0xFF;
	}
}

static bool keyconfig_key_valid(uint8_t key)
{
	if(key == KEYCONFIG_KEY_UNBOUND) {
		return true;
	}
	return (
		((key >> 3) < KEYCONFIG_KEY_GROUP_COUNT) &&
		(keyconfig_key_mask(key >> 3) & (1 << (key & 7)))
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

static bool keyconfig_file_valid(const keyconfig_file_t __ss& cfg)
{
	uint8_t action;
	uint8_t i;
	uint8_t j;
	bool bound;
#if (GAME == 5)
	const char *magic = keyconfig_strings.text_t5key001;
#else
	const char *magic = keyconfig_strings.text_t4key001;
#endif

	if(
		(keyconfig_mem_compare(cfg.magic, magic, 8) != 0) ||
		(cfg.version != KEYCONFIG_VERSION) ||
		(cfg.size != sizeof(cfg)) || (cfg.reserved != 0) ||
		(cfg.checksum != keyconfig_checksum(cfg))
	) {
		return false;
	}
	for(i = 0; i < KEYCONFIG_BINDING_COUNT; i++) {
		if(!keyconfig_key_valid(cfg.bindings[i])) {
			return false;
		}
		if(cfg.bindings[i] != KEYCONFIG_KEY_UNBOUND) {
			for(j = 0; j < i; j++) {
				if(cfg.bindings[i] == cfg.bindings[j]) {
					return false;
				}
			}
		}
	}
	for(action = 0; action < KCA_COUNT; action++) {
		bound = false;
		for(i = 0; i < KEYCONFIG_BINDINGS_PER_ACTION; i++) {
			if(cfg.bindings[(action * 2) + i] != KEYCONFIG_KEY_UNBOUND) {
				bound = true;
			}
		}
		if(!bound) {
			return false;
		}
	}
	return true;
}

static void keyconfig_defaults_set(void)
{
	for(uint8_t action = 0; action < KCA_COUNT; action++) {
		for(uint8_t alternate = 0;
			alternate < KEYCONFIG_BINDINGS_PER_ACTION; alternate++) {
			keyconfig_bindings[(action * 2) + alternate] =
				keyconfig_default_binding(action, alternate);
		}
	}
}

static void keyconfig_load(void)
{
	keyconfig_strings_init();
	keyconfig_file_t cfg;

	if(keyconfig_loaded) {
		return;
	}
	keyconfig_loaded = true;
	keyconfig_defaults_set();
	if(keyconfig_file_read_exact(keyconfig_strings.text_th4key_cfg, &cfg, sizeof(cfg)) &&
		keyconfig_file_valid(cfg)) {
		keyconfig_mem_copy(keyconfig_bindings, cfg.bindings, sizeof(keyconfig_bindings));
	}
}

static input_t keyconfig_action_input(uint8_t action)
{
	switch(action) {
	case KCA_UP_LEFT:    return INPUT_UP_LEFT;
	case KCA_UP:         return INPUT_UP;
	case KCA_UP_RIGHT:   return INPUT_UP_RIGHT;
	case KCA_LEFT:       return INPUT_LEFT;
	case KCA_RIGHT:      return INPUT_RIGHT;
	case KCA_DOWN_LEFT:  return INPUT_DOWN_LEFT;
	case KCA_DOWN:       return INPUT_DOWN;
	case KCA_DOWN_RIGHT: return INPUT_DOWN_RIGHT;
	case KCA_SHOT:       return INPUT_SHOT;
	default:             return INPUT_BOMB;
	}
}

void far keyconfig_gameplay_apply(void)
{
	uint8_t groups[KEYCONFIG_KEY_GROUP_COUNT];
	input_t mapped = INPUT_NONE;
	const input_t gameplay = (INPUT_MOVEMENT | INPUT_SHOT | INPUT_BOMB);

	keyconfig_load();
	for(uint8_t group = 0; group < KEYCONFIG_KEY_GROUP_COUNT; group++) {
		groups[group] = peekb(0, (KEYGROUP_0 + group));
	}
	for(uint8_t action = 0; action < KCA_COUNT; action++) {
		for(uint8_t alternate = 0;
			alternate < KEYCONFIG_BINDINGS_PER_ACTION; alternate++) {
			uint8_t key = keyconfig_bindings[(action * 2) + alternate];
			if(
				(key != KEYCONFIG_KEY_UNBOUND) &&
				(groups[key >> 3] & (1 << (key & 7)))
			) {
				mapped |= keyconfig_action_input(action);
				break;
			}
		}
	}
	key_det = static_cast<input_t>(
		(key_det & ~gameplay) | (js_stat[0] & gameplay) | mapped
	);
}
