/* TH01 patch-owned language preference runtime.
 *
 * Each process reads this compact, versioned configuration independently. It
 * is intentionally not a replay, resident, or score-file carrier.
 */

#pragma option -zCT1LANG_TEXT -G-

#include <stdio.h>
#include "th01/language.hpp"

#define T1LANG_CONFIG_SIZE 8
#define T1LANG_CONFIG_VERSION 1

struct t1_language_config_t {
	char magic[4];
	uint8_t version;
	uint8_t preference;
	uint8_t checksum;
	uint8_t checksum_inverse;
};

typedef char t1_language_config_size_check[
	(sizeof(t1_language_config_t) == T1LANG_CONFIG_SIZE) ? 1 : -1
];

// Zero-initialization intentionally means Japanese before the first load.
static t1_language_preference_t t1_language_runtime;

static void t1_language_config_fn_set(char *fn)
{
	fn[0] = 'T';
	fn[1] = '1';
	fn[2] = 'L';
	fn[3] = 'A';
	fn[4] = 'N';
	fn[5] = 'G';
	fn[6] = '.';
	fn[7] = 'C';
	fn[8] = 'F';
	fn[9] = 'G';
	fn[10] = '\0';
}

static uint8_t t1_language_config_checksum(const t1_language_config_t *config)
{
	const uint8_t *bytes = reinterpret_cast<const uint8_t *>(config);
	uint8_t sum = 0;
	uint8_t i;

	for(i = 0; i < 6; i++) {
		sum += bytes[i];
	}
	return sum;
}

void far t1_language_load(void)
{
	t1_language_config_t config;
	uint8_t extra;
	uint8_t checksum;
	char fn[11];
	char mode[3];
	FILE *file;
	bool exact_size;

	// A receiver must never retain a stale selection after a missing or invalid
	// file. The all-zero BSS state also deliberately means Japanese.
	t1_language_runtime = T1LANG_JAPANESE;
	t1_language_config_fn_set(fn);
	mode[0] = 'r';
	mode[1] = 'b';
	mode[2] = '\0';
	file = fopen(fn, mode);
	if(!file) {
		return;
	}
	exact_size = (
		(fread(&config, 1, sizeof(config), file) == sizeof(config)) &&
		(fread(&extra, 1, 1, file) == 0) &&
		(feof(file) != 0)
	);
	if(fclose(file) != 0) {
		exact_size = false;
	}
	if(!exact_size) {
		return;
	}
	checksum = t1_language_config_checksum(&config);
	if(
		(config.magic[0] != 'T') ||
		(config.magic[1] != '1') ||
		(config.magic[2] != 'L') ||
		(config.magic[3] != 'G') ||
		(config.version != T1LANG_CONFIG_VERSION) ||
		(config.preference > T1LANG_ENGLISH) ||
		(config.checksum != checksum) ||
		(config.checksum_inverse != static_cast<uint8_t>(~checksum))
	) {
		return;
	}
	t1_language_runtime = static_cast<t1_language_preference_t>(
		config.preference
	);
}

t1_language_preference_t far t1_language_get(void)
{
	return t1_language_runtime;
}

#pragma codeseg
