#pragma option -zCKEYCFG_I_TEXT
#pragma option -zPgroup_KEYCONFIG_INPUT

#include <stddef.h>
#include "x86real.h"
#include "platform/x86real/pc98/keyboard.hpp"
#include "th01/keyconfig.hpp"

#include "th01/keydos.inc"

// Materialize into appended BSS; ordinary literals would grow stock DATA.
struct keyconfig_strings_t {
	char text_t1key001[9];
	char text_th1key_cfg[11];
};
static keyconfig_strings_t keyconfig_strings;
static bool keyconfig_strings_ready;

static void keyconfig_strings_init(void)
{
	if(keyconfig_strings_ready) return;
	keyconfig_strings.text_t1key001[0] = 84; keyconfig_strings.text_t1key001[1] = 49;
	keyconfig_strings.text_t1key001[2] = 75; keyconfig_strings.text_t1key001[3] = 69;
	keyconfig_strings.text_t1key001[4] = 89; keyconfig_strings.text_t1key001[5] = 48;
	keyconfig_strings.text_t1key001[6] = 48; keyconfig_strings.text_t1key001[7] = 49;
	keyconfig_strings.text_t1key001[8] = 0; keyconfig_strings.text_th1key_cfg[0] = 84;
	keyconfig_strings.text_th1key_cfg[1] = 72; keyconfig_strings.text_th1key_cfg[2] = 49;
	keyconfig_strings.text_th1key_cfg[3] = 75; keyconfig_strings.text_th1key_cfg[4] = 69;
	keyconfig_strings.text_th1key_cfg[5] = 89; keyconfig_strings.text_th1key_cfg[6] = 46;
	keyconfig_strings.text_th1key_cfg[7] = 67; keyconfig_strings.text_th1key_cfg[8] = 70;
	keyconfig_strings.text_th1key_cfg[9] = 71; keyconfig_strings.text_th1key_cfg[10] = 0;

	keyconfig_strings_ready = true;
}

static bool keyconfig_loaded;
static uint8_t keyconfig_bindings[T1_KEYCONFIG_BINDING_COUNT];

static uint8_t keyconfig_default_binding(uint8_t action, uint8_t alternate)
{
	if(alternate) {
		switch(action) {
		case KCA_UP:    return keyconfig_key(8, 3); // Num8
		case KCA_DOWN:  return keyconfig_key(9, 3); // Num2
		case KCA_LEFT:  return keyconfig_key(8, 6); // Num4
		case KCA_RIGHT: return keyconfig_key(9, 0); // Num6
		default:        return T1_KEYCONFIG_KEY_UNBOUND;
		}
	}
	switch(action) {
	case KCA_UP:     return keyconfig_key(7, 2); // Up arrow
	case KCA_DOWN:   return keyconfig_key(7, 5); // Down arrow
	case KCA_LEFT:   return keyconfig_key(7, 3); // Left arrow
	case KCA_RIGHT:  return keyconfig_key(7, 4); // Right arrow
	case KCA_SHOT:   return keyconfig_key(5, 1); // Z
	default:         return keyconfig_key(5, 2); // X
	}
}

static uint8_t keyconfig_key_mask(uint8_t group)
{
	switch(group) {
	case 0:  return 0xFE; // Esc is always the physical replay abort key.
	case 2:  return 0xF7; // R is always the physical replay-menu key.
	case 10: return 0x7F;
	case 11: return 0x00;
	case 13: return 0x0F;
	case 14: return (K14_SHIFT | K14_GRPH | K14_CTRL);
	default: return 0xFF;
	}
}

static bool keyconfig_key_valid(uint8_t key)
{
	if(key == T1_KEYCONFIG_KEY_UNBOUND) {
		return true;
	}
	return (
		((key >> 3) < T1_KEYCONFIG_KEY_GROUP_COUNT) &&
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

	if(
		(keyconfig_mem_compare(cfg.magic, keyconfig_strings.text_t1key001, 8) != 0) ||
		(cfg.version != T1_KEYCONFIG_VERSION) ||
		(cfg.size != sizeof(cfg)) || (cfg.checksum != keyconfig_checksum(cfg))
	) {
		return false;
	}
	for(i = 0; i < sizeof(cfg.reserved); i++) {
		if(cfg.reserved[i] != 0) {
			return false;
		}
	}
	for(i = 0; i < T1_KEYCONFIG_BINDING_COUNT; i++) {
		if(!keyconfig_key_valid(cfg.bindings[i])) {
			return false;
		}
		if(cfg.bindings[i] != T1_KEYCONFIG_KEY_UNBOUND) {
			for(j = 0; j < i; j++) {
				if(cfg.bindings[i] == cfg.bindings[j]) {
					return false;
				}
			}
		}
	}
	for(action = 0; action < KCA_COUNT; action++) {
		bound = false;
		for(i = 0; i < T1_KEYCONFIG_BINDINGS_PER_ACTION; i++) {
			if(cfg.bindings[(action * 2) + i] != T1_KEYCONFIG_KEY_UNBOUND) {
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
			alternate < T1_KEYCONFIG_BINDINGS_PER_ACTION; alternate++) {
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
	if(keyconfig_file_read_exact(keyconfig_strings.text_th1key_cfg, &cfg, sizeof(cfg)) &&
		keyconfig_file_valid(cfg)) {
		keyconfig_mem_copy(keyconfig_bindings, cfg.bindings, sizeof(keyconfig_bindings));
	}
}

static bool keyconfig_action_pressed(
	uint8_t action, const uint8_t __ss *groups
)
{
	for(uint8_t alternate = 0;
		alternate < T1_KEYCONFIG_BINDINGS_PER_ACTION; alternate++) {
		uint8_t key = keyconfig_bindings[(action * 2) + alternate];
		if(
			(key != T1_KEYCONFIG_KEY_UNBOUND) &&
			(groups[key >> 3] & (1 << (key & 7)))
		) {
			return true;
		}
	}
	return false;
}

void far keyconfig_input_sense(keyconfig_input_groups_t far *out)
{
	uint8_t groups[T1_KEYCONFIG_KEY_GROUP_COUNT];

	keyconfig_load();
	for(uint8_t group = 0; group < T1_KEYCONFIG_KEY_GROUP_COUNT; group++) {
		groups[group] = peekb(0, (KEYGROUP_0 + group));
	}
	out->group_0 = (groups[0] & K0_ESC);
	out->group_3 = (groups[3] & K3_RETURN);
	out->group_5 = 0;
	out->group_6 = (groups[6] & (K6_ROLL_UP | K6_ROLL_DOWN));
	out->group_7 = 0;
	out->group_8 = 0;
	out->group_9 = 0;
	if(keyconfig_action_pressed(KCA_UP, groups)) {
		out->group_7 |= K7_ARROW_UP;
	}
	if(keyconfig_action_pressed(KCA_DOWN, groups)) {
		out->group_7 |= K7_ARROW_DOWN;
	}
	if(keyconfig_action_pressed(KCA_LEFT, groups)) {
		out->group_7 |= K7_ARROW_LEFT;
	}
	if(keyconfig_action_pressed(KCA_RIGHT, groups)) {
		out->group_7 |= K7_ARROW_RIGHT;
	}
	if(keyconfig_action_pressed(KCA_SHOT, groups)) {
		out->group_5 |= K5_Z;
	}
	if(keyconfig_action_pressed(KCA_STRIKE, groups)) {
		out->group_5 |= K5_X;
	}
}
