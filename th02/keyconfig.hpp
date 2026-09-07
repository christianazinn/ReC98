#ifndef TH02_KEYCONFIG_HPP
#define TH02_KEYCONFIG_HPP

#include "platform.h"

#define T2_KEYCONFIG_FN "TH2KEY.CFG"
#define T2_KEYCONFIG_TEMP_FN "TH2KEY.$$$"
#define T2_KEYCONFIG_BACKUP_FN "TH2KEY.BAK"
#define T2_KEYCONFIG_VERSION 1
#define T2_KEYCONFIG_KEY_UNBOUND 0xFF
#define T2_KEYCONFIG_KEY_GROUP_COUNT 15
#define T2_KEYCONFIG_BINDINGS_PER_ACTION 2

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

#define T2_KEYCONFIG_BINDING_COUNT \
	(KCA_COUNT * T2_KEYCONFIG_BINDINGS_PER_ACTION)

struct keyconfig_file_t {
	char magic[8];
	uint8_t version;
	uint8_t size;
	uint8_t bindings[T2_KEYCONFIG_BINDING_COUNT];
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

#endif /* TH02_KEYCONFIG_HPP */
