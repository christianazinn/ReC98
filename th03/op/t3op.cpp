#pragma option -zCT3CASE_OP_TEXT

#include "libs/master.lib/master.hpp"
#include "platform.h"
#include "th03/op/t3case.hpp"
#include "th03/resident.hpp"
#include "th03/t3case.hpp"

// Keep this module free of initialized data. Original TH03 code addresses
// DGROUP through raw offsets, so filenames and work buffers live in the final
// BSS contribution and are filled at runtime.
static char t3case_op_cfg_fn[11];
static char t3case_op_bin_fn[11];
static char t3case_op_cfg[64];
static uint8_t t3case_op_chunk[64];
static t3case_header_t t3case_op_header;
static t3case_startup_t t3case_op_startup;

static uint32_t t3case_op_fnv1a(
	uint32_t hash, const void far *buf, unsigned size
)
{
	const uint8_t far *p = reinterpret_cast<const uint8_t far *>(buf);

	while(size != 0) {
		hash ^= static_cast<uint32_t>(*p++);
		hash *= T3CASE_FNV1A_PRIME;
		size--;
	}
	return hash;
}

static void t3case_op_fn_set(char near *fn, char extension_0)
{
	fn[0] = 'T'; fn[1] = '3'; fn[2] = 'C'; fn[3] = 'A'; fn[4] = 'S';
	fn[5] = 'E'; fn[6] = '.'; fn[7] = extension_0;
}

static bool t3case_op_cfg_playback(void)
{
	unsigned read_len;
	unsigned i;

	t3case_op_fn_set(t3case_op_cfg_fn, 'C');
	t3case_op_cfg_fn[8] = 'F'; t3case_op_cfg_fn[9] = 'G';
	t3case_op_cfg_fn[10] = '\0';
	if(!file_ropen(t3case_op_cfg_fn)) {
		return false;
	}
	read_len = file_read(t3case_op_cfg, sizeof(t3case_op_cfg));
	file_close();
	for(i = 0; i < read_len; i++) {
		if(
			(t3case_op_cfg[i] != ' ') && (t3case_op_cfg[i] != '\t') &&
			(t3case_op_cfg[i] != '\r') && (t3case_op_cfg[i] != '\n')
		) {
			return (
				(t3case_op_cfg[i] == 'p') || (t3case_op_cfg[i] == 'P')
			);
		}
	}
	return false;
}

static bool t3case_op_header_valid(void)
{
	if(
		(t3case_op_header.magic[0] != 'T') ||
		(t3case_op_header.magic[1] != '3') ||
		(t3case_op_header.magic[2] != 'C') ||
		(t3case_op_header.magic[3] != 'A') ||
		(t3case_op_header.magic[4] != 'S') ||
		(t3case_op_header.magic[5] != 'E') ||
		(t3case_op_header.magic[6] != '1') ||
		(t3case_op_header.magic[7] != '\0') ||
		(t3case_op_header.version != T3CASE_VERSION) ||
		(t3case_op_header.header_size != sizeof(t3case_header_t)) ||
		(t3case_op_header.startup_size != sizeof(t3case_startup_t)) ||
		(t3case_op_header.record_size != sizeof(t3case_record_t)) ||
		(t3case_op_header.input_semantics != T3CASE_INPUT_SEMANTICS) ||
		(t3case_op_header.ruleset_id != T3CASE_RULESET_CLASSIC) ||
		(t3case_op_header.first_process != T3CASE_PROCESS_MAIN) ||
		(t3case_op_header.source_kind != T3CASE_SOURCE_NORMALIZED_V11) ||
		(t3case_op_header.flags & ~T3CASE_KNOWN_FLAGS) ||
		!(t3case_op_header.flags & T3CASE_FLAG_SNAPSHOT) ||
		(
			t3case_op_header.payload_offset !=
			(static_cast<uint32_t>(T3CASE_PREFIX_SIZE) + T3CASE_SNAPSHOT_SIZE)
		) ||
		(
			t3case_op_header.payload_size !=
			(
				t3case_op_header.record_count *
				static_cast<uint32_t>(T3CASE_RECORD_SIZE)
			)
		) ||
		(t3case_op_header.sample_count > t3case_op_header.record_count) ||
		(
			t3case_op_header.total_size !=
			(t3case_op_header.payload_offset + t3case_op_header.payload_size)
		)
	) {
		return false;
	}
	return true;
}

static bool t3case_op_startup_valid(void)
{
	unsigned i;

	if(
		(t3case_op_startup.post_init_flags != 0) ||
		(t3case_op_startup.post_init_randring_p != 0) ||
		(t3case_op_startup.autofire & 0xFC)
	) {
		return false;
	}
	for(i = 0; i < sizeof(t3case_op_startup.reserved); i++) {
		if(t3case_op_startup.reserved[i] != 0) {
			return false;
		}
	}
	return true;
}

static bool t3case_op_prefix_read(void)
{
	uint32_t stored;
	uint32_t hash;
	unsigned remaining = T3CASE_SNAPSHOT_SIZE;
	unsigned size;

	t3case_op_fn_set(t3case_op_bin_fn, 'B');
	t3case_op_bin_fn[8] = 'I'; t3case_op_bin_fn[9] = 'N';
	t3case_op_bin_fn[10] = '\0';
	if(!file_ropen(t3case_op_bin_fn)) {
		return false;
	}
	if(
		(
			file_read(&t3case_op_header, sizeof(t3case_op_header)) !=
			sizeof(t3case_op_header)
		) ||
		(
			file_read(&t3case_op_startup, sizeof(t3case_op_startup)) !=
			sizeof(t3case_op_startup)
		) ||
		!t3case_op_header_valid() ||
		!t3case_op_startup_valid()
	) {
		file_close();
		return false;
	}
	stored = t3case_op_header.header_checksum;
	t3case_op_header.header_checksum = 0;
	hash = t3case_op_fnv1a(
		T3CASE_FNV1A_BASIS, &t3case_op_header, sizeof(t3case_op_header)
	);
	hash = t3case_op_fnv1a(
		hash, &t3case_op_startup, sizeof(t3case_op_startup)
	);
	while(remaining != 0) {
		size = (
			(remaining < sizeof(t3case_op_chunk)) ?
			remaining : sizeof(t3case_op_chunk)
		);
		if(file_read(t3case_op_chunk, size) != size) {
			file_close();
			return false;
		}
		hash = t3case_op_fnv1a(hash, t3case_op_chunk, size);
		remaining -= size;
	}
	file_close();
	t3case_op_header.header_checksum = stored;
	return (hash == stored);
}

static void t3case_op_startup_apply(void)
{
	int i;
	int digit;

	resident->rand = t3case_op_startup.resident_rand;
	resident->rank = t3case_op_startup.rank;
	resident->key_mode = t3case_op_startup.key_mode;
	resident->game_mode = t3case_op_startup.game_mode;
	resident->story_stage = t3case_op_startup.story_stage;
	resident->story_lives = t3case_op_startup.story_lives;
	resident->rem_credits = t3case_op_startup.rem_credits;
	resident->skill = t3case_op_startup.skill;
	resident->demo_num = t3case_op_startup.demo_num;
	resident->pid_winner = t3case_op_startup.pid_winner;
	resident->show_score_menu = (t3case_op_startup.show_score_menu != 0);
	resident->op_animation_fast = (t3case_op_startup.op_animation_fast != 0);
	for(i = 0; i < T3CASE_PLAYER_COUNT; i++) {
		resident->is_cpu[i] = (t3case_op_startup.is_cpu[i] != 0);
		resident->playchar_paletted[i].v =
			t3case_op_startup.playchar_paletted[i];
		for(digit = 0; digit < T3CASE_SCORE_DIGITS; digit++) {
			resident->score_last[i].digits[digit] =
				t3case_op_startup.score_last[i][digit];
		}
	}
	for(i = 0; i < T3CASE_STAGE_COUNT; i++) {
		resident->story_opponents[i].v = t3case_op_startup.story_opponents[i];
	}
}

void far t3case_op_scenario_apply(void)
{
	if(!t3case_op_cfg_playback()) {
		return;
	}
	if(t3case_op_prefix_read()) {
		t3case_op_startup_apply();
	}
}
