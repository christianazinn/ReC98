#ifndef TH04_KEYCONFIG_HPP
#define TH04_KEYCONFIG_HPP

#include "platform.h"

#if (GAME == 5)
	#define KEYCONFIG_FN "TH5KEY.CFG"
	#define KEYCONFIG_TEMP_FN "TH5KEY.$$$"
	#define KEYCONFIG_BACKUP_FN "TH5KEY.BAK"
#else
	#define KEYCONFIG_FN "TH4KEY.CFG"
	#define KEYCONFIG_TEMP_FN "TH4KEY.$$$"
	#define KEYCONFIG_BACKUP_FN "TH4KEY.BAK"
#endif

#define KEYCONFIG_VERSION 1
#define KEYCONFIG_KEY_UNBOUND 0xFF
#define KEYCONFIG_KEY_GROUP_COUNT 15
#define KEYCONFIG_BINDINGS_PER_ACTION 2

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
	KCA_COUNT,
};

#define KEYCONFIG_BINDING_COUNT \
	(KCA_COUNT * KEYCONFIG_BINDINGS_PER_ACTION)

struct keyconfig_file_t {
	char magic[8];
	uint8_t version;
	uint8_t size;
	uint8_t bindings[KEYCONFIG_BINDING_COUNT];
	uint8_t reserved;
	uint8_t checksum;
};

typedef char keyconfig_file_size_check[
	(sizeof(keyconfig_file_t) == 32) ? 1 : -1
];

inline uint8_t keyconfig_key(uint8_t group, uint8_t bit)
{
	return static_cast<uint8_t>((group << 3) | bit);
}

void far keyconfig_gameplay_apply(void);

#if (BINARY == 'O')
bool far keyconfig_menu(void);
#endif

#endif /* TH04_KEYCONFIG_HPP */
