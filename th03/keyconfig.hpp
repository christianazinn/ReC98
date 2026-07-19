#ifndef TH03_KEYCONFIG_HPP
#define TH03_KEYCONFIG_HPP

#include "platform.h"
#include "th03/replay_handoff.hpp"
#include "th03/resident.hpp"

#define T3_KEYCONFIG_FN "TH3KEY.CFG"
#define T3_KEYCONFIG_TEMP_FN "TH3KEY.$$$"
#define T3_KEYCONFIG_BACKUP_FN "TH3KEY.BAK"

#define T3_KEYCONFIG_PLAYER_COUNT 2
#define T3_KEYCONFIG_ACTION_COUNT 11
#define T3_KEYCONFIG_PLAYER_BINDING_COUNT ( \
	T3_KEYCONFIG_PLAYER_COUNT * T3_KEYCONFIG_ACTION_COUNT \
)
#define T3_KEYCONFIG_STORY_ACTION_COUNT 4
#define T3_KEYCONFIG_STORY_BINDINGS_INDEX T3_KEYCONFIG_PLAYER_BINDING_COUNT
#define T3_KEYCONFIG_BINDING_COUNT ( \
	T3_KEYCONFIG_PLAYER_BINDING_COUNT + T3_KEYCONFIG_STORY_ACTION_COUNT \
)

#define T3_KEYCONFIG_KEY_UNBOUND 0xFF
#define T3_KEYCONFIG_KEY_GROUP_COUNT 15

#define T3_KEYCONFIG_RES_START_INDEX 69
#define T3_KEYCONFIG_RES_MAGIC_0_INDEX 69
#define T3_KEYCONFIG_RES_MAGIC_1_INDEX 70
#define T3_KEYCONFIG_RES_VERSION_INDEX 71
#define T3_KEYCONFIG_RES_BINDINGS_INDEX 72
#define T3_KEYCONFIG_RES_END_INDEX ( \
	T3_KEYCONFIG_RES_BINDINGS_INDEX + T3_KEYCONFIG_BINDING_COUNT \
)

#define T3_KEYCONFIG_RES_MAGIC_0 'K'
#define T3_KEYCONFIG_RES_MAGIC_1 'C'
#define T3_KEYCONFIG_VERSION 2

#if (T3_KEYCONFIG_RES_START_INDEX <= (T3_REPLAY_RES_MAINL_VSYNC_INDEX + 1))
#error Key configuration overlaps replay handoff state
#endif
#if (T3_KEYCONFIG_RES_END_INDEX > 160)
#error Key configuration overlaps Practice state
#endif
#if (T3_KEYCONFIG_RES_END_INDEX != 98)
#error Key configuration resident layout changed without an audited version bump
#endif

enum keyconfig_action_t {
	KCA_UP_LEFT,
	KCA_UP,
	KCA_UP_RIGHT,
	KCA_LEFT,
	KCA_RIGHT,
	KCA_DOWN_LEFT,
	KCA_DOWN,
	KCA_DOWN_RIGHT,
	KCA_SHOT,
	KCA_BOMB,
	KCA_CHARGE,
};

enum keyconfig_story_action_t {
	KCSA_UP,
	KCSA_LEFT,
	KCSA_RIGHT,
	KCSA_DOWN,
};

inline uint8_t keyconfig_key(uint8_t group, uint8_t bit)
{
	return static_cast<uint8_t>((group << 3) | bit);
}

inline uint8_t keyconfig_resident_u8(unsigned int index)
{
	return static_cast<uint8_t>(resident->unused_3[index]);
}

inline void keyconfig_resident_u8_set(unsigned int index, uint8_t value)
{
	resident->unused_3[index] = value;
}

inline bool keyconfig_resident_valid(void)
{
	return (
		(keyconfig_resident_u8(T3_KEYCONFIG_RES_MAGIC_0_INDEX) ==
		 T3_KEYCONFIG_RES_MAGIC_0) &&
		(keyconfig_resident_u8(T3_KEYCONFIG_RES_MAGIC_1_INDEX) ==
		 T3_KEYCONFIG_RES_MAGIC_1) &&
		(keyconfig_resident_u8(T3_KEYCONFIG_RES_VERSION_INDEX) ==
		 T3_KEYCONFIG_VERSION)
	);
}

inline uint8_t keyconfig_resident_binding(uint8_t pid, uint8_t action)
{
	return keyconfig_resident_u8(
		T3_KEYCONFIG_RES_BINDINGS_INDEX +
		(pid * T3_KEYCONFIG_ACTION_COUNT) + action
	);
}

inline void keyconfig_resident_binding_set(
	uint8_t pid, uint8_t action, uint8_t key
)
{
	keyconfig_resident_u8_set(
		T3_KEYCONFIG_RES_BINDINGS_INDEX +
		(pid * T3_KEYCONFIG_ACTION_COUNT) + action,
		key
	);
}

uint8_t far keyconfig_default_binding(uint8_t pid, uint8_t action);
uint8_t far keyconfig_default_story_binding(uint8_t action);
void far keyconfig_input_apply(void);
void far keyconfig_story_input_merge(void);
extern "C" void far keyconfig_restart_request_poll(void);
void far keyconfig_charge_mask_human(void);

#if (BINARY == 'O')
void far keyconfig_load(bool legacy_autofire);
bool far keyconfig_menu(void);
#endif

#endif /* TH03_KEYCONFIG_HPP */
