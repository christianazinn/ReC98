/// Shot level and shot-function installation
/// -----------------------------------------
/// Shared by TH04 and TH05. Both originals are instruction-for-instruction
/// identical and frameless. The including translation unit must compile this
/// body under `-k-`.

#include "platform.h"
#include "x86real.h"

static const int SHOT_LEVEL_COUNT = 9;

extern "C" uint8_t power;
extern "C" uint8_t shot_level;
extern "C" uint16_t SHOT_LEVEL_TO_POWER[SHOT_LEVEL_COUNT];
extern "C" nearfunc_t_near near *playchar_shot_funcs;
extern "C" nearfunc_t_near playchar_shot_func;
extern "C" void pascal hud_power_put(void);

#define nopcall_same_group(func) asm { \
	nop; \
	push cs; \
	call near ptr func; \
}

extern "C" void far player_shot_level_update(void)
{
	_BX = 0;
	_AX = 0;
	_AL = power;
	_CX = SHOT_LEVEL_COUNT;

threshold_scan:
	if(
		_AX <
		*reinterpret_cast<uint16_t near *>(
			reinterpret_cast<uint8_t near *>(SHOT_LEVEL_TO_POWER) + _BX
		)
	) {
		goto install;
	}
	_BX += sizeof(SHOT_LEVEL_TO_POWER[0]);

	// Turbo C++ emits DEC CX / JNE for every ordinary loop spelling. Its
	// inline assembler is the smallest semantic pin for the original LOOP.
	asm { loop threshold_scan; }

install:
	_DX = _BX;
	_DX >>= 1;
	shot_level = _DL;

	_BX += reinterpret_cast<uint16_t>(playchar_shot_funcs);
	playchar_shot_func = *reinterpret_cast<nearfunc_t_near near *>(_BX);
	nopcall_same_group(hud_power_put);
}

#undef nopcall_same_group
