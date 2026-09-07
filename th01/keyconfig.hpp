#ifndef TH01_KEYCONFIG_HPP
#define TH01_KEYCONFIG_HPP

#include "platform.h"

#define T1_KEYCONFIG_FN "TH1KEY.CFG"
#define T1_KEYCONFIG_TEMP_FN "TH1KEY.$$$"
#define T1_KEYCONFIG_BACKUP_FN "TH1KEY.BAK"
#define T1_KEYCONFIG_VERSION 1
#define T1_KEYCONFIG_KEY_UNBOUND 0xFF
#define T1_KEYCONFIG_KEY_GROUP_COUNT 15
#define T1_KEYCONFIG_BINDINGS_PER_ACTION 2

enum keyconfig_action_t {
	KCA_UP,
	KCA_DOWN,
	KCA_LEFT,
	KCA_RIGHT,
	KCA_SHOT,
	KCA_STRIKE,
	KCA_COUNT,
};

#define T1_KEYCONFIG_BINDING_COUNT \
	(KCA_COUNT * T1_KEYCONFIG_BINDINGS_PER_ACTION)

struct keyconfig_file_t {
	char magic[8];
	uint8_t version;
	uint8_t size;
	uint8_t bindings[T1_KEYCONFIG_BINDING_COUNT];
	uint8_t reserved[9];
	uint8_t checksum;
};

typedef char keyconfig_file_size_check[
	(sizeof(keyconfig_file_t) == 32) ? 1 : -1
];

struct keyconfig_input_groups_t {
	uint8_t group_0;
	uint8_t group_3;
	uint8_t group_5;
	uint8_t group_6;
	uint8_t group_7;
	uint8_t group_8;
	uint8_t group_9;
};

inline uint8_t keyconfig_key(uint8_t group, uint8_t bit)
{
	return static_cast<uint8_t>((group << 3) | bit);
}

void far keyconfig_input_sense(keyconfig_input_groups_t far *groups);

#if (BINARY == 'O')
bool far keyconfig_menu(void);
void far keyconfig_option_update(void);
#endif

#endif /* TH01_KEYCONFIG_HPP */
