#pragma option -WX -zCSHARED -k-

#include "x86real.h"
#include "libs/master.lib/master.hpp"
#include "platform/x86real/flags.hpp"
#include "platform/x86real/pc98/keyboard.hpp"
#include "th03/hardware/input.h"
#include "th03/keyconfig.hpp"

void input_reset_sense_key_held(void)
{
	js_stat[0] = input_sp = input_mp_p2 = input_mp_p1 = INPUT_NONE;
	resident->input_charge = 0;
	_DX = _DX;	// This moves [loops] to BL. Just remove this line.
	_asm { jmp sense; } sense: // And this one.

	// The key state is checked twice, 614.4 µs apart, to ignore the momentary
	// "key released" events sent by PC-98 keyboards at the typematic rate if
	// a key is held down. This ensures that the game consistently sees that
	// specific input being pressed. See the HOLDKEY example in the
	// `Research/` subdirectory for more explanation and sample code showing
	// off this effect.
	register unsigned char loops = 2;
	_ES = 0;
	do {
		_AH = peekb(_ES, KEYGROUP_7);
		if(_AH & K7_ARROW_UP) {
			input_sp |= INPUT_UP;
		}
		if(_AH & K7_ARROW_DOWN) {
			input_sp |= INPUT_DOWN;
		}
		if(_AH & K7_ARROW_LEFT) {
			input_sp |= INPUT_LEFT;
		}
		if(_AH & K7_ARROW_RIGHT) {
			input_sp |= INPUT_RIGHT;
		}

		_AH = peekb(_ES, KEYGROUP_9);
		if(_AH & K9_NUM_6) {
			input_sp |= INPUT_RIGHT;
		}
		if(_AH & K9_NUM_1) {
			input_sp |= INPUT_DOWN_LEFT;
		}
		if(_AH & K9_NUM_2) {
			input_sp |= INPUT_DOWN;
		}
		if(_AH & K9_NUM_3) {
			input_sp |= INPUT_DOWN_RIGHT;
		}

		_AH = peekb(_ES, KEYGROUP_8);
		if(_AH & K8_NUM_4) {
			input_sp |= INPUT_LEFT;
		}
		if(_AH & K8_NUM_7) {
			input_sp |= INPUT_UP_LEFT;
		}
		if(_AH & K8_NUM_8) {
			input_sp |= INPUT_UP;
		}
		if(_AH & K8_NUM_9) {
			input_sp |= INPUT_UP_RIGHT;
		}

		_AH = peekb(_ES, KEYGROUP_5);
		if(_AH & K5_Z) {
			input_sp |= INPUT_SHOT;
		}
		if(_AH & K5_X) {
			input_sp |= INPUT_BOMB;
		}

		_AH = peekb(_ES, KEYGROUP_2);
		if(_AH & K2_Q) {
			input_sp |= INPUT_Q;
		}

		_AH = peekb(_ES, KEYGROUP_0);
		if(_AH & K0_ESC) {
			input_sp |= INPUT_CANCEL;
		}

		_AH = peekb(_ES, KEYGROUP_3);
		if(_AH & K3_RETURN) {
			input_sp |= INPUT_OK;
		}

		_AH = peekb(_ES, KEYGROUP_6);
		if(_AH & K6_SPACE) {
			input_sp |= INPUT_SHOT;
		}

		_asm { push es; }
		keyconfig_input_apply();
		_asm { pop es; }

		loops--;
		if(FLAGS_ZERO) {
			break;
		}
		_CX = 1024; // * 0.6 µs
		delay_loop: asm {
			out 	0x5F, al;
			loop	delay_loop;
		}
	} while(1);
}
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
