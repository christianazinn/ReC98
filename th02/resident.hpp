#include "th02/score.h"

static const int8_t LIVES_MAX = 5;
static const int8_t BOMBS_MAX = 5;

#define RES_ID "MIKOConfig"
struct resident_t {
	char id[sizeof(RES_ID)];
	unsigned char stage;
	char debug;
	int8_t padding_1;
	score_t score;
	unsigned int continues_used;
	char rem_bombs;
	char rem_lives;
	char rank;
	char start_power;

	// [uint8_t] rather than [char] for the same reason as [demo_num] below:
	// main() compares it against 1 and 2, and Turbo C++ only keeps those as
	// direct memory-byte compares for an unsigned type (kb/codegen/0029).
	// Every other reference in TH02 is a store, which is signedness-neutral.
	uint8_t bgm_mode;
	uint8_t start_bombs;
	uint8_t start_lives;
	int8_t padding_2;
	long frame;
	int unused_1;
	char unused_2;
	unsigned char op_main_retval;
	bool reduce_effects;
	char unused_3;
	uint8_t shottype;

	// The demo to replay, 1-3, or 0 for a regular game. [uint8_t] rather than
	// [char] because demo_load() compares it against 1/2/3 and Turbo C++ only
	// keeps those as direct memory-byte compares for an unsigned type
	// (kb/codegen/0029).
	uint8_t demo_num;
	int skill;
	int unused_4;
	long score_highest;
};

extern resident_t far *resident;

// Redundant copies of resident structure fields to static data
// ------------------------------------------------------------

extern int8_t bombs;
extern int8_t lives;
extern bool reduce_effects;
// ------------------------------------------------------------
