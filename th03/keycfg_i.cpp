#pragma option -zPgroup_KEYCONFIG_INPUT

#include "x86real.h"
#include "libs/master.lib/master.hpp"
#include "platform/x86real/pc98/keyboard.hpp"
#include "th03/hardware/input.h"
#include "th03/keyconfig.hpp"

uint8_t far keyconfig_default_binding(uint8_t pid, uint8_t action)
{
	if(pid == 0) {
		switch(action) {
		case KCA_UP_LEFT:    return keyconfig_key(2, 3);  // R
		case KCA_UP:         return keyconfig_key(2, 4);  // T
		case KCA_UP_RIGHT:   return keyconfig_key(2, 5);  // Y
		case KCA_LEFT:       return keyconfig_key(4, 0);  // F
		case KCA_RIGHT:      return keyconfig_key(4, 2);  // H
		case KCA_DOWN_LEFT:  return keyconfig_key(5, 4);  // V
		case KCA_DOWN:       return keyconfig_key(5, 5);  // B
		case KCA_DOWN_RIGHT: return keyconfig_key(5, 6);  // N
		case KCA_SHOT:       return keyconfig_key(5, 1);  // Z
		case KCA_BOMB:       return keyconfig_key(5, 2);  // X
		default:             return keyconfig_key(14, 0); // Shift
		}
	}

	switch(action) {
	case KCA_UP_LEFT:    return keyconfig_key(8, 2); // Num7
	case KCA_UP:         return keyconfig_key(8, 3); // Num8
	case KCA_UP_RIGHT:   return keyconfig_key(8, 4); // Num9
	case KCA_LEFT:       return keyconfig_key(8, 6); // Num4
	case KCA_RIGHT:      return keyconfig_key(9, 0); // Num6
	case KCA_DOWN_LEFT:  return keyconfig_key(9, 2); // Num1
	case KCA_DOWN:       return keyconfig_key(9, 3); // Num2
	case KCA_DOWN_RIGHT: return keyconfig_key(9, 4); // Num3
	case KCA_SHOT:       return keyconfig_key(7, 3); // Left arrow
	case KCA_BOMB:       return keyconfig_key(7, 4); // Right arrow
	default:             return keyconfig_key(8, 7); // Num5
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

void far keyconfig_input_apply(void)
{
	uint8_t groups[T3_KEYCONFIG_KEY_GROUP_COUNT];
	uint8_t pid;
	uint8_t action;
	uint8_t key;
	input_t near *input;
	bool configured = keyconfig_resident_valid();

	for(uint8_t group = 0; group < T3_KEYCONFIG_KEY_GROUP_COUNT; group++) {
		groups[group] = peekb(0, (KEYGROUP_0 + group));
	}
	for(pid = 0; pid < T3_KEYCONFIG_PLAYER_COUNT; pid++) {
		input = ((pid == 0) ? &input_mp_p1 : &input_mp_p2);
		for(action = 0; action < T3_KEYCONFIG_ACTION_COUNT; action++) {
			key = (
				configured ?
				keyconfig_resident_binding(pid, action) :
				keyconfig_default_binding(pid, action)
			);
			if(
				(key == T3_KEYCONFIG_KEY_UNBOUND) ||
				!(groups[key >> 3] & (1 << (key & 7)))
			) {
				continue;
			}
			if(action == KCA_CHARGE) {
				resident->input_charge |= (1 << pid);
			} else {
				*input |= keyconfig_action_input(action);
			}
		}
	}
}

void far keyconfig_charge_mask_human(void)
{
	uint8_t mask = (resident->input_charge & 0x03);

	if(resident->is_cpu[0]) {
		mask &= ~0x01;
	}
	if(resident->is_cpu[1]) {
		mask &= ~0x02;
	}
	if(js_bexist) {
		if(resident->key_mode == KM_JOY_KEY) {
			mask &= ~0x01;
		} else if(resident->key_mode == KM_KEY_JOY) {
			mask &= ~0x02;
		}
	}
	resident->input_charge = mask;
}
