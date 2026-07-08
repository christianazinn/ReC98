#pragma option -zCREPLAY_TEXT

#include "libs/master.lib/master.hpp"
#include "platform.h"
#include "th02/math/randring.hpp"
#include "th03/main/defeat.hpp"
#include "th03/main/difficul.hpp"
#include "th03/main/player/stuff.hpp"
#include "th03/main/replay.hpp"
#include "th03/main/round.hpp"
#include "th03/main/score.hpp"
#include "th03/replay_format.hpp"
#include "th03/replay_handoff.hpp"
#include "th03/resident.hpp"

static const char T3_REPLAY_CFG_FN[] = "T3REPLAY.CFG";
static const char T3_INPUT_FN[] = "T3INPUT.BIN";
static const char T3_SPLIT_FN[] = "T3SPLIT.TSV";
static const char T3_DONE_FN[] = "T3DONE.TXT";
static const char T3_USER_REPLAY_DIR[] = "REPLAY";
static const char T3_USER_REPLAY_INDEX_FN[] = "REPLAY\\TH3R.IDX";
static char T3_USER_REPLAY_SLOT_FN[] = "REPLAY\\TH3R00.RPY";
static const char T3_USER_REPLAY_FALLBACK_FN[] = "TH3LAST.RPY";

enum replay_mode_t {
	REPLAY_DISABLED = 0,
	REPLAY_RECORD = 1,
	REPLAY_PLAYBACK = 2,
	REPLAY_USER_RECORD = 3,
	REPLAY_USER_PLAYBACK = 4,
	REPLAY_ERROR = 5,
};

struct replay_input_header_t {
	char magic[8];
	uint16_t version;
	uint16_t header_size;
	uint16_t sample_size;
	uint16_t flags;
	uint32_t sample_count;
	uint32_t initial_resident_rand;
	uint16_t game_mode;
	uint8_t rank;
	uint8_t key_mode;
	uint8_t playchar_p1;
	uint8_t playchar_p2;
	uint8_t story_stage;
	uint8_t reserved_1;
	uint32_t input_crc32;
	uint32_t split_crc32;
	uint32_t reserved_2;
	uint32_t reserved_3;
	uint32_t reserved_4;
	uint32_t reserved_5;
	uint32_t reserved_6;
	uint32_t reserved_7;
};

struct replay_input_sample_t {
	uint32_t frame_index;
	uint32_t round_frame;
	uint16_t round_or_result_frame;
	uint16_t input_mp_p1;
	uint16_t input_mp_p2;
	uint16_t input_sp;
};

static replay_mode_t replay_mode = REPLAY_DISABLED;
static replay_input_header_t replay_header;
static replay_user_header_t replay_user_header;
static replay_user_snapshot_t replay_user_snapshot;
static replay_user_index_header_t replay_user_index_header;
static replay_user_index_entry_t replay_user_index_entry;
static const char *replay_user_fn = T3_USER_REPLAY_FALLBACK_FN;
static uint8_t replay_user_slot = T3_REPLAY_USER_SLOT_NONE;
static uint32_t replay_sample_count = 0;
static uint32_t replay_global_frame = 0;
static uint8_t replay_last_route = 0;
static bool replay_done_written = false;

extern "C" unsigned char score[];
extern uint8_t byte_23B00;
extern uint8_t randring_p;
extern uint8_t formation_p[PLAYER_COUNT];
extern uint8_t __seg *formation_type_ring;
extern uint8_t __seg *formation_pos_type_ring;

static replay_mode_t replay_cfg_mode(void);
static replay_mode_t replay_resident_mode(void);

static void replay_memclear(void far *buf, unsigned size)
{
	uint8_t far *p = reinterpret_cast<uint8_t far *>(buf);
	while(size != 0) {
		*p++ = 0;
		size--;
	}
}

static bool replay_char_ieq(char a, char b)
{
	if((a >= 'A') && (a <= 'Z')) {
		a += ('a' - 'A');
	}
	if((b >= 'A') && (b <= 'Z')) {
		b += ('a' - 'A');
	}
	return (a == b);
}

static bool replay_buf_has_word(
	const char far *buf, unsigned buf_len, const char *word
)
{
	unsigned i;
	unsigned j;

	for(i = 0; i < buf_len; i++) {
		for(j = 0; word[j] != '\0'; j++) {
			if((i + j) >= buf_len) {
				return false;
			}
			if(buf[i + j] == '\0') {
				return false;
			}
			if(!replay_char_ieq(buf[i + j], word[j])) {
				goto next_i;
			}
		}
		return true;

	next_i:
		;
	}
	return false;
}

static void replay_write_bytes(const void far *buf, unsigned size)
{
	file_write(buf, size);
}

static bool replay_write_bytes_checked(const void far *buf, unsigned size)
{
	return (file_write(buf, size) != 0);
}

static void replay_write_cstr(const char *str)
{
	unsigned len = 0;

	while(str[len] != '\0') {
		len++;
	}
	replay_write_bytes(str, len);
}

static void replay_write_char(char c)
{
	replay_write_bytes(&c, 1);
}

static void replay_write_u32(uint32_t value)
{
	char digits[10];
	int i = 0;

	do {
		digits[i] = static_cast<char>('0' + (value % 10));
		value /= 10;
		i++;
	} while(value != 0);

	while(i != 0) {
		i--;
		replay_write_char(digits[i]);
	}
}

static void replay_write_i32(int32_t value)
{
	if(value < 0) {
		replay_write_char('-');
		replay_write_u32(static_cast<uint32_t>(-value));
	} else {
		replay_write_u32(static_cast<uint32_t>(value));
	}
}

static void replay_write_hex_nibble(uint8_t value)
{
	value &= 0x0F;
	if(value < 10) {
		replay_write_char(static_cast<char>('0' + value));
	} else {
		replay_write_char(static_cast<char>('A' + (value - 10)));
	}
}

static void replay_write_hex32(uint32_t value)
{
	int shift;

	for(shift = 28; shift >= 0; shift -= 4) {
		replay_write_hex_nibble(static_cast<uint8_t>(value >> shift));
	}
}

static void replay_write_score(const unsigned char near *digits)
{
	int digit;

	for(digit = (SCORE_DIGITS - 1); digit >= 0; digit--) {
		replay_write_char(static_cast<char>('0' + (digits[digit] % 10)));
	}
}

static void replay_dir_create(void)
{
	dos_axdx(0x3900, T3_USER_REPLAY_DIR);
}

static uint8_t replay_resident_slot(void)
{
	uint8_t slot = static_cast<uint8_t>(
		resident->unused_3[T3_REPLAY_RES_SLOT_INDEX]
	);
	if(slot < T3_REPLAY_USER_SLOT_COUNT) {
		return slot;
	}
	return T3_REPLAY_USER_SLOT_NONE;
}

static void replay_user_slot_fn_set(uint8_t slot)
{
	if(slot < T3_REPLAY_USER_SLOT_COUNT) {
		replay_user_slot = slot;
		T3_USER_REPLAY_SLOT_FN[11] = static_cast<char>('0' + (slot / 10));
		T3_USER_REPLAY_SLOT_FN[12] = static_cast<char>('0' + (slot % 10));
		replay_user_fn = T3_USER_REPLAY_SLOT_FN;
	} else {
		replay_user_slot = T3_REPLAY_USER_SLOT_NONE;
		replay_user_fn = T3_USER_REPLAY_FALLBACK_FN;
	}
}

static void replay_user_index_header_fill(uint8_t next_slot)
{
	replay_memclear(&replay_user_index_header, sizeof(replay_user_index_header));
	replay_user_index_header.magic[0] = 'T';
	replay_user_index_header.magic[1] = '3';
	replay_user_index_header.magic[2] = 'R';
	replay_user_index_header.magic[3] = 'I';
	replay_user_index_header.magic[4] = 'D';
	replay_user_index_header.magic[5] = 'X';
	replay_user_index_header.magic[6] = '2';
	replay_user_index_header.magic[7] = '\0';
	replay_user_index_header.version = T3_REPLAY_USER_INDEX_VERSION;
	replay_user_index_header.header_size = sizeof(replay_user_index_header);
	replay_user_index_header.entry_size = sizeof(replay_user_index_entry);
	replay_user_index_header.slot_count = T3_REPLAY_USER_SLOT_COUNT;
	replay_user_index_header.next_slot = next_slot;
}

static void replay_user_index_entry_fill(
	replay_user_status_t status, replay_user_end_reason_t end_reason
)
{
	replay_memclear(&replay_user_index_entry, sizeof(replay_user_index_entry));
	replay_user_index_entry.used = true;
	replay_user_index_entry.slot_id = replay_user_slot;
	replay_user_index_entry.status = status;
	replay_user_index_entry.end_reason = end_reason;
	replay_user_index_entry.game_mode = replay_user_header.game_mode;
	replay_user_index_entry.rank = replay_user_header.rank;
	replay_user_index_entry.key_mode = replay_user_header.key_mode;
	replay_user_index_entry.playchar_p1 = replay_user_header.playchar_p1;
	replay_user_index_entry.playchar_p2 = replay_user_header.playchar_p2;
	replay_user_index_entry.story_stage = replay_user_header.story_stage;
	replay_user_index_entry.is_cpu_p1 = replay_user_header.is_cpu_p1;
	replay_user_index_entry.is_cpu_p2 = replay_user_header.is_cpu_p2;
	replay_user_index_entry.sample_count = replay_user_header.sample_count;
	replay_user_index_entry.final_frame_count = (
		replay_user_header.final_frame_count
	);
	replay_user_index_entry.resident_rand = replay_user_header.resident_rand;
	replay_user_index_entry.random_seed_snapshot = (
		replay_user_header.random_seed_snapshot
	);
	replay_user_index_entry.input_crc32 = replay_user_header.input_crc32;
	replay_user_index_entry.snapshot_crc32 = replay_user_header.snapshot_crc32;
}

static bool replay_user_index_create(void)
{
	int slot;

	replay_user_index_header_fill(
		((replay_user_slot + 1) % T3_REPLAY_USER_SLOT_COUNT)
	);
	replay_memclear(&replay_user_index_entry, sizeof(replay_user_index_entry));

	if(!file_create(T3_USER_REPLAY_INDEX_FN)) {
		return false;
	}
	if(!replay_write_bytes_checked(
		&replay_user_index_header, sizeof(replay_user_index_header)
	)) {
		file_close();
		return false;
	}
	for(slot = 0; slot < T3_REPLAY_USER_SLOT_COUNT; slot++) {
		if(!replay_write_bytes_checked(
			&replay_user_index_entry, sizeof(replay_user_index_entry)
		)) {
			file_close();
			return false;
		}
	}
	file_close();
	return true;
}

static bool replay_user_index_slot_write(
	replay_user_status_t status, replay_user_end_reason_t end_reason
)
{
	uint32_t offset;
	bool ret;

	if(replay_user_slot >= T3_REPLAY_USER_SLOT_COUNT) {
		return false;
	}

	replay_dir_create();
	if(!file_append(T3_USER_REPLAY_INDEX_FN)) {
		if(!replay_user_index_create()) {
			return false;
		}
		if(!file_append(T3_USER_REPLAY_INDEX_FN)) {
			return false;
		}
	}

	replay_user_index_entry_fill(status, end_reason);
	offset = (
		static_cast<uint32_t>(sizeof(replay_user_index_header)) +
		(
			static_cast<uint32_t>(replay_user_slot) *
			static_cast<uint32_t>(sizeof(replay_user_index_entry))
		)
	);
	file_seek(offset, SEEK_SET);
	ret = replay_write_bytes_checked(
		&replay_user_index_entry, sizeof(replay_user_index_entry)
	);
	file_close();
	return ret;
}

static uint32_t replay_hash_u8(uint32_t hash, uint8_t value)
{
	return (((hash << 5) + hash) ^ value);
}

static uint32_t replay_hash_u16(uint32_t hash, uint16_t value)
{
	hash = replay_hash_u8(hash, static_cast<uint8_t>(value));
	hash = replay_hash_u8(hash, static_cast<uint8_t>(value >> 8));
	return hash;
}

static uint32_t replay_hash_u32(uint32_t hash, uint32_t value)
{
	hash = replay_hash_u16(hash, static_cast<uint16_t>(value));
	hash = replay_hash_u16(hash, static_cast<uint16_t>(value >> 16));
	return hash;
}

static uint32_t replay_hash_score(uint32_t hash, const unsigned char near *digits)
{
	int digit;

	for(digit = 0; digit < SCORE_DIGITS; digit++) {
		hash = replay_hash_u8(hash, digits[digit]);
	}
	return hash;
}

static uint32_t replay_hash_bytes(
	uint32_t hash, const void near *buf, unsigned size
)
{
	const uint8_t near *p = reinterpret_cast<const uint8_t near *>(buf);

	while(size != 0) {
		hash = replay_hash_u8(hash, *p++);
		size--;
	}
	return hash;
}

static uint32_t replay_hash_player(
	uint32_t hash, const player_stuff_t near *player
)
{
	hash = replay_hash_u16(hash, player->center.x.v);
	hash = replay_hash_u16(hash, player->center.y.v);
	hash = replay_hash_u8(hash, player->halfhearts);
	hash = replay_hash_u8(hash, player->invincibility_time);
	hash = replay_hash_u8(hash, player->shot_mode);
	hash = replay_hash_u8(hash, player->knockback_time);
	hash = replay_hash_u8(hash, player->move_lock_time);
	hash = replay_hash_u16(hash, player->gauge_charged);
	hash = replay_hash_u16(hash, player->gauge_avail);
	hash = replay_hash_u8(hash, player->bombs);
	hash = replay_hash_u8(hash, player->rounds_won);
	hash = replay_hash_u16(hash, player->cpu_frame);
	hash = replay_hash_u8(hash, player->hit_damage_next);
	hash = replay_hash_u8(hash, player->shot_active);
	hash = replay_hash_u8(hash, player->gauge_attacks_fired);
	hash = replay_hash_u8(hash, player->boss_attacks_fired);
	hash = replay_hash_u8(hash, player->boss_attacks_reversed);
	hash = replay_hash_u8(hash, player->boss_panics_fired);
	return hash;
}

static uint32_t replay_state_hash(void)
{
	uint32_t hash = 5381;

	hash = replay_hash_u32(hash, round_frame);
	hash = replay_hash_u16(hash, round_or_result_frame);
	hash = replay_hash_u8(hash, round_speed);
	hash = replay_hash_u8(hash, defeat_flag);
	hash = replay_hash_u8(hash, resident->pid_winner);
	hash = replay_hash_u16(hash, input_mp_p1);
	hash = replay_hash_u16(hash, input_mp_p2);
	hash = replay_hash_u16(hash, input_sp);
	hash = replay_hash_score(hash, score);
	hash = replay_hash_score(hash, (score + SCORE_DIGITS));
	hash = replay_hash_player(hash, &players[0]);
	hash = replay_hash_player(hash, &players[1]);
	hash = replay_hash_u32(hash, random_seed);
	return hash;
}

static void replay_split_write_header(void)
{
	static const char header[] =
		"event\tglobal_frame\tround_frame\tround_or_result_frame\t"
		"route\tgame_mode\tstory_stage\tround_id\twinner\tp1_score\t"
		"p2_score\tresident_rand\tround_speed\tstate_hash\r\n";

	file_create(T3_SPLIT_FN);
	replay_write_bytes(header, (sizeof(header) - 1));
	file_close();
}

static void replay_done_write(const char *status)
{
	if(replay_done_written) {
		return;
	}
	if(file_create(T3_DONE_FN)) {
		replay_write_cstr(status);
		replay_write_cstr("\r\n");
		file_close();
	}
	replay_done_written = true;
}

static void replay_split_row(const char *event, uint8_t route)
{
	if((replay_mode == REPLAY_DISABLED) || (replay_mode == REPLAY_ERROR)) {
		return;
	}
	if(!file_append(T3_SPLIT_FN)) {
		replay_mode = REPLAY_ERROR;
		replay_done_write("error:split-open");
		return;
	}

	replay_write_cstr(event);
	replay_write_char('\t');
	replay_write_u32(replay_global_frame);
	replay_write_char('\t');
	replay_write_u32(round_frame);
	replay_write_char('\t');
	replay_write_u32(round_or_result_frame);
	replay_write_char('\t');
	replay_write_u32(route);
	replay_write_char('\t');
	replay_write_u32(resident->game_mode);
	replay_write_char('\t');
	replay_write_u32(resident->story_stage);
	replay_write_char('\t');
	replay_write_u32(round_id);
	replay_write_char('\t');
	replay_write_i32(resident->pid_winner);
	replay_write_char('\t');
	replay_write_score(score);
	replay_write_char('\t');
	replay_write_score(score + SCORE_DIGITS);
	replay_write_char('\t');
	replay_write_i32(resident->rand);
	replay_write_char('\t');
	replay_write_u32(round_speed);
	replay_write_char('\t');
	replay_write_hex32(replay_state_hash());
	replay_write_cstr("\r\n");
	file_close();
}

static void replay_header_fill(void)
{
	replay_memclear(&replay_header, sizeof(replay_header));
	replay_header.magic[0] = 'T';
	replay_header.magic[1] = '3';
	replay_header.magic[2] = 'R';
	replay_header.magic[3] = 'I';
	replay_header.magic[4] = 'N';
	replay_header.magic[5] = 'P';
	replay_header.magic[6] = '1';
	replay_header.magic[7] = '\0';
	replay_header.version = 1;
	replay_header.header_size = sizeof(replay_header);
	replay_header.sample_size = sizeof(replay_input_sample_t);
	replay_header.sample_count = replay_sample_count;
	replay_header.initial_resident_rand = resident->rand;
	replay_header.game_mode = resident->game_mode;
	replay_header.rank = resident->rank;
	replay_header.key_mode = resident->key_mode;
	replay_header.playchar_p1 = resident->playchar_paletted[0].v;
	replay_header.playchar_p2 = resident->playchar_paletted[1].v;
	replay_header.story_stage = resident->story_stage;
}

static void replay_user_snapshot_fill(void)
{
	int i;
	int digit;

	replay_memclear(&replay_user_snapshot, sizeof(replay_user_snapshot));
	replay_user_snapshot.resident_rand = resident->rand;
	replay_user_snapshot.random_seed_snapshot = random_seed;
	replay_user_snapshot.rank = resident->rank;
	replay_user_snapshot.key_mode = resident->key_mode;
	replay_user_snapshot.game_mode = resident->game_mode;
	replay_user_snapshot.story_stage = resident->story_stage;
	replay_user_snapshot.story_lives = resident->story_lives;
	replay_user_snapshot.rem_credits = resident->rem_credits;
	replay_user_snapshot.skill = resident->skill;
	replay_user_snapshot.demo_num = resident->demo_num;
	replay_user_snapshot.pid_winner = resident->pid_winner;
	replay_user_snapshot.show_score_menu = resident->show_score_menu;
	replay_user_snapshot.op_animation_fast = resident->op_animation_fast;

	for(i = 0; i < PLAYER_COUNT; i++) {
		replay_user_snapshot.is_cpu[i] = resident->is_cpu[i];
		replay_user_snapshot.playchar_paletted[i] = (
			resident->playchar_paletted[i].v
		);
		for(digit = 0; digit < SCORE_DIGITS; digit++) {
			replay_user_snapshot.score_last[i][digit] = (
				resident->score_last[i].digits[digit]
			);
		}
	}
	for(i = 0; i < STAGE_COUNT; i++) {
		replay_user_snapshot.story_opponents[i] = resident->story_opponents[i].v;
	}
	replay_user_snapshot.randring_p = randring_p;
	for(i = 0; i < RANDRING_SIZE; i++) {
		replay_user_snapshot.randring[i] = randring[i];
	}
	for(i = 0; i < T3_REPLAY_USER_FORMATION_RING_SIZE; i++) {
		replay_user_snapshot.formation_type_ring[i] = formation_type_ring[i];
		replay_user_snapshot.formation_pos_type_ring[i] = formation_pos_type_ring[i];
	}
	for(i = 0; i < PLAYER_COUNT; i++) {
		replay_user_snapshot.formation_p[i] = formation_p[i];
		replay_user_snapshot.cpu_charge_at_avail_ring_p[i] = (
			players[i].cpu_charge_at_avail_ring_p
		);
		for(digit = 0; digit < CHARGE_AT_AVAIL_RING_SIZE; digit++) {
			replay_user_snapshot.cpu_charge_at_avail_ring[i][digit] = (
				players[i].cpu_charge_at_avail_ring[digit]
			);
		}
		replay_user_snapshot.player_center_x[i] = players[i].center.x.v;
		replay_user_snapshot.player_center_y[i] = players[i].center.y.v;
		replay_user_snapshot.player_halfhearts[i] = players[i].halfhearts;
		replay_user_snapshot.player_invincibility_time[i] = (
			players[i].invincibility_time
		);
		replay_user_snapshot.player_gauge_charge_speed[i] = (
			players[i].gauge_charge_speed
		);
		replay_user_snapshot.player_gauge_charged[i] = (
			players[i].gauge_charged
		);
		replay_user_snapshot.player_gauge_avail[i] = players[i].gauge_avail;
		replay_user_snapshot.player_bombs[i] = players[i].bombs;
		replay_user_snapshot.player_shot_active[i] = players[i].shot_active;
		replay_user_snapshot.player_cpu_frame[i] = players[i].cpu_frame;
	}
}

static void replay_user_header_fill(
	replay_user_status_t status, replay_user_end_reason_t end_reason
)
{
	replay_memclear(&replay_user_header, sizeof(replay_user_header));
	replay_user_header.magic[0] = 'T';
	replay_user_header.magic[1] = '3';
	replay_user_header.magic[2] = 'R';
	replay_user_header.magic[3] = 'P';
	replay_user_header.magic[4] = 'L';
	replay_user_header.magic[5] = 'Y';
	replay_user_header.magic[6] = '2';
	replay_user_header.magic[7] = '\0';
	replay_user_header.version = T3_REPLAY_USER_VERSION;
	replay_user_header.header_size = sizeof(replay_user_header);
	replay_user_header.sample_size = sizeof(replay_user_sample_t);
	replay_user_header.status = status;
	replay_user_header.end_reason = end_reason;
	replay_user_header.game_mode = replay_user_snapshot.game_mode;
	replay_user_header.rank = replay_user_snapshot.rank;
	replay_user_header.key_mode = replay_user_snapshot.key_mode;
	replay_user_header.playchar_p1 = replay_user_snapshot.playchar_paletted[0];
	replay_user_header.playchar_p2 = replay_user_snapshot.playchar_paletted[1];
	replay_user_header.story_stage = replay_user_snapshot.story_stage;
	replay_user_header.is_cpu_p1 = replay_user_snapshot.is_cpu[0];
	replay_user_header.is_cpu_p2 = replay_user_snapshot.is_cpu[1];
	replay_user_header.sample_count = replay_sample_count;
	replay_user_header.final_frame_count = replay_global_frame;
	replay_user_header.resident_rand = replay_user_snapshot.resident_rand;
	replay_user_header.random_seed_snapshot = (
		replay_user_snapshot.random_seed_snapshot
	);
	replay_user_header.snapshot_offset = sizeof(replay_user_header);
	replay_user_header.snapshot_size = sizeof(replay_user_snapshot);
	replay_user_header.input_offset = (
		static_cast<uint32_t>(sizeof(replay_user_header)) +
		static_cast<uint32_t>(sizeof(replay_user_snapshot))
	);
	replay_user_header.input_size = (
		replay_sample_count * static_cast<uint32_t>(sizeof(replay_user_sample_t))
	);
	replay_user_header.snapshot_crc32 = replay_hash_bytes(
		5381, &replay_user_snapshot, sizeof(replay_user_snapshot)
	);
}

static bool replay_header_write(void)
{
	replay_header.sample_count = replay_sample_count;
	if(!file_append(T3_INPUT_FN)) {
		return false;
	}
	file_seek(0, SEEK_SET);
	replay_write_bytes(&replay_header, sizeof(replay_header));
	file_close();
	return true;
}

static bool replay_user_header_write(
	replay_user_status_t status, replay_user_end_reason_t end_reason
)
{
	replay_user_header_fill(status, end_reason);
	if(!file_append(replay_user_fn)) {
		return false;
	}
	file_seek(0, SEEK_SET);
	if(!replay_write_bytes_checked(&replay_user_header, sizeof(replay_user_header))) {
		file_close();
		return false;
	}
	file_close();
	replay_user_index_slot_write(status, end_reason);
	return true;
}

static bool replay_user_header_valid(void)
{
	return (
		(replay_user_header.magic[0] == 'T') &&
		(replay_user_header.magic[1] == '3') &&
		(replay_user_header.magic[2] == 'R') &&
		(replay_user_header.magic[3] == 'P') &&
		(replay_user_header.magic[4] == 'L') &&
		(replay_user_header.magic[5] == 'Y') &&
		(replay_user_header.magic[6] == '2') &&
		(replay_user_header.version == T3_REPLAY_USER_VERSION) &&
		(replay_user_header.header_size == sizeof(replay_user_header)) &&
		(replay_user_header.sample_size == sizeof(replay_user_sample_t)) &&
		(replay_user_header.snapshot_offset == sizeof(replay_user_header)) &&
		(replay_user_header.snapshot_size == sizeof(replay_user_snapshot)) &&
		(replay_user_header.input_offset == (
			static_cast<uint32_t>(sizeof(replay_user_header)) +
			static_cast<uint32_t>(sizeof(replay_user_snapshot))
		)) &&
		(replay_user_header.sample_count != 0)
	);
}

static bool replay_user_read_from(const char *fn)
{
	replay_user_fn = fn;
	if(!file_ropen(replay_user_fn)) {
		return false;
	}
	if(
		file_read(&replay_user_header, sizeof(replay_user_header)) !=
		sizeof(replay_user_header)
	) {
		file_close();
		return false;
	}
	if(!replay_user_header_valid()) {
		file_close();
		return false;
	}
	if(
		file_read(&replay_user_snapshot, sizeof(replay_user_snapshot)) !=
		sizeof(replay_user_snapshot)
	) {
		file_close();
		return false;
	}
	file_close();
	return true;
}

static bool replay_user_read(void)
{
	uint8_t slot = replay_resident_slot();

	if(slot < T3_REPLAY_USER_SLOT_COUNT) {
		replay_user_slot_fn_set(slot);
		if(replay_user_read_from(replay_user_fn)) {
			return true;
		}
	}

	if(replay_user_read_from(T3_USER_REPLAY_FALLBACK_FN)) {
		replay_user_slot = T3_REPLAY_USER_SLOT_NONE;
		return true;
	}
	replay_user_slot_fn_set(0);
	return replay_user_read_from(replay_user_fn);
}

static void replay_user_snapshot_restore_resident(void)
{
	int i;
	int digit;

	resident->rand = replay_user_snapshot.resident_rand;
	resident->rank = replay_user_snapshot.rank;
	resident->key_mode = replay_user_snapshot.key_mode;
	resident->game_mode = replay_user_snapshot.game_mode;
	resident->story_stage = replay_user_snapshot.story_stage;
	resident->story_lives = replay_user_snapshot.story_lives;
	resident->rem_credits = replay_user_snapshot.rem_credits;
	resident->skill = replay_user_snapshot.skill;
	resident->demo_num = replay_user_snapshot.demo_num;
	resident->pid_winner = replay_user_snapshot.pid_winner;
	resident->show_score_menu = replay_user_snapshot.show_score_menu;
	resident->op_animation_fast = replay_user_snapshot.op_animation_fast;

	for(i = 0; i < PLAYER_COUNT; i++) {
		resident->is_cpu[i] = replay_user_snapshot.is_cpu[i];
		resident->playchar_paletted[i].v = (
			replay_user_snapshot.playchar_paletted[i]
		);
		for(digit = 0; digit < SCORE_DIGITS; digit++) {
			resident->score_last[i].digits[digit] = (
				replay_user_snapshot.score_last[i][digit]
			);
		}
	}
	for(i = 0; i < STAGE_COUNT; i++) {
		resident->story_opponents[i].v = replay_user_snapshot.story_opponents[i];
	}
}

static void replay_user_snapshot_restore_runtime(void)
{
	int i;
	int digit;

	random_seed = replay_user_snapshot.random_seed_snapshot;
	randring_p = replay_user_snapshot.randring_p;
	for(i = 0; i < RANDRING_SIZE; i++) {
		randring[i] = replay_user_snapshot.randring[i];
	}
	for(i = 0; i < T3_REPLAY_USER_FORMATION_RING_SIZE; i++) {
		formation_type_ring[i] = replay_user_snapshot.formation_type_ring[i];
		formation_pos_type_ring[i] = (
			replay_user_snapshot.formation_pos_type_ring[i]
		);
	}
	for(i = 0; i < PLAYER_COUNT; i++) {
		formation_p[i] = replay_user_snapshot.formation_p[i];
		players[i].cpu_charge_at_avail_ring_p = (
			replay_user_snapshot.cpu_charge_at_avail_ring_p[i]
		);
		for(digit = 0; digit < CHARGE_AT_AVAIL_RING_SIZE; digit++) {
			players[i].cpu_charge_at_avail_ring[digit] = (
				replay_user_snapshot.cpu_charge_at_avail_ring[i][digit]
			);
		}
		players[i].center.x.v = replay_user_snapshot.player_center_x[i];
		players[i].center.y.v = replay_user_snapshot.player_center_y[i];
		players[i].halfhearts = replay_user_snapshot.player_halfhearts[i];
		players[i].invincibility_time = (
			replay_user_snapshot.player_invincibility_time[i]
		);
		players[i].gauge_charge_speed = (
			replay_user_snapshot.player_gauge_charge_speed[i]
		);
		players[i].gauge_charged = replay_user_snapshot.player_gauge_charged[i];
		players[i].gauge_avail = replay_user_snapshot.player_gauge_avail[i];
		players[i].bombs = replay_user_snapshot.player_bombs[i];
		players[i].shot_active = static_cast<shot_active_t>(
			replay_user_snapshot.player_shot_active[i]
		);
		players[i].cpu_frame = replay_user_snapshot.player_cpu_frame[i];
	}
}

static bool replay_user_create(void)
{
	uint8_t slot = replay_resident_slot();

	replay_user_snapshot_fill();
	replay_user_header_fill(RUS_RECORDING, RUER_PARTIAL);
	replay_user_slot_fn_set(slot);
	if(slot < T3_REPLAY_USER_SLOT_COUNT) {
		replay_dir_create();
	}
	if(!file_create(replay_user_fn)) {
		replay_user_slot_fn_set(T3_REPLAY_USER_SLOT_NONE);
		if(!file_create(replay_user_fn)) {
			return false;
		}
	}
	if(!replay_write_bytes_checked(&replay_user_header, sizeof(replay_user_header))) {
		file_close();
		return false;
	}
	if(!replay_write_bytes_checked(&replay_user_snapshot, sizeof(replay_user_snapshot))) {
		file_close();
		return false;
	}
	file_close();
	replay_user_index_slot_write(RUS_RECORDING, RUER_PARTIAL);
	return true;
}

static bool replay_record_sample(void)
{
	replay_input_sample_t sample;
	uint32_t offset;

	sample.frame_index = replay_global_frame;
	sample.round_frame = round_frame;
	sample.round_or_result_frame = round_or_result_frame;
	sample.input_mp_p1 = input_mp_p1;
	sample.input_mp_p2 = input_mp_p2;
	sample.input_sp = input_sp;

	offset = (
		static_cast<uint32_t>(sizeof(replay_header)) +
		(replay_sample_count * static_cast<uint32_t>(sizeof(sample)))
	);
	if(!file_append(T3_INPUT_FN)) {
		return false;
	}
	file_seek(offset, SEEK_SET);
	replay_write_bytes(&sample, sizeof(sample));
	file_close();
	replay_sample_count++;
	return true;
}

static bool replay_user_record_sample(void)
{
	replay_user_sample_t sample;
	uint32_t offset;

	sample.frame_index = replay_global_frame;
	sample.input_mp_p1 = input_mp_p1;
	sample.input_mp_p2 = input_mp_p2;
	sample.input_sp = input_sp;
	sample.round_or_result_frame = round_or_result_frame;
	sample.round_frame = round_frame;

	offset = (
		replay_user_header.input_offset +
		(replay_sample_count * static_cast<uint32_t>(sizeof(sample)))
	);
	if(!file_append(replay_user_fn)) {
		return false;
	}
	file_seek(offset, SEEK_SET);
	if(!replay_write_bytes_checked(&sample, sizeof(sample))) {
		file_close();
		return false;
	}
	file_close();
	replay_sample_count++;
	return true;
}

static bool replay_user_play_sample(void)
{
	replay_user_sample_t sample;
	uint32_t offset;

	if(replay_sample_count >= replay_user_header.sample_count) {
		return false;
	}

	offset = (
		replay_user_header.input_offset +
		(replay_sample_count * static_cast<uint32_t>(sizeof(sample)))
	);
	if(!file_ropen(replay_user_fn)) {
		return false;
	}
	file_seek(offset, SEEK_SET);
	if(file_read(&sample, sizeof(sample)) != sizeof(sample)) {
		file_close();
		return false;
	}
	file_close();

	if(
		(sample.frame_index != replay_global_frame) ||
		(sample.round_frame != round_frame) ||
		(sample.round_or_result_frame != round_or_result_frame)
	) {
		return false;
	}

	input_mp_p1 = sample.input_mp_p1;
	input_mp_p2 = sample.input_mp_p2;
	input_sp = sample.input_sp;
	replay_sample_count++;
	if(
		(replay_sample_count >= replay_user_header.sample_count) &&
		(replay_user_header.status == RUS_FINALIZED) &&
		(replay_user_header.end_reason == RUER_MENU_RETURN)
	) {
		// Q is read by TH03's blocking Esc prompt, outside this frame stream.
		input_sp = 0;
		byte_23B00 = 1;
	}
	return true;
}

static bool replay_play_sample(void)
{
	replay_input_sample_t sample;
	uint32_t offset;

	if(replay_sample_count >= replay_header.sample_count) {
		return false;
	}

	offset = (
		static_cast<uint32_t>(sizeof(replay_header)) +
		(replay_sample_count * static_cast<uint32_t>(sizeof(sample)))
	);
	if(!file_ropen(T3_INPUT_FN)) {
		return false;
	}
	file_seek(offset, SEEK_SET);
	if(file_read(&sample, sizeof(sample)) != sizeof(sample)) {
		file_close();
		return false;
	}
	file_close();

	if(
		(sample.frame_index != replay_global_frame) ||
		(sample.round_frame != round_frame) ||
		(sample.round_or_result_frame != round_or_result_frame)
	) {
		return false;
	}

	input_mp_p1 = sample.input_mp_p1;
	input_mp_p2 = sample.input_mp_p2;
	input_sp = sample.input_sp;
	replay_sample_count++;
	return true;
}

static bool replay_header_read(void)
{
	if(!file_ropen(T3_INPUT_FN)) {
		return false;
	}
	if(file_read(&replay_header, sizeof(replay_header)) != sizeof(replay_header)) {
		file_close();
		return false;
	}
	file_close();

	return (
		(replay_header.magic[0] == 'T') &&
		(replay_header.magic[1] == '3') &&
		(replay_header.magic[2] == 'R') &&
		(replay_header.magic[3] == 'I') &&
		(replay_header.magic[4] == 'N') &&
		(replay_header.magic[5] == 'P') &&
		(replay_header.magic[6] == '1') &&
		(replay_header.version == 1) &&
		(replay_header.header_size == sizeof(replay_header)) &&
		(replay_header.sample_size == sizeof(replay_input_sample_t))
	);
}

static replay_mode_t replay_cfg_mode(void)
{
	char cfg[64];
	unsigned read_len;

	replay_memclear(cfg, sizeof(cfg));
	if(!file_ropen(T3_REPLAY_CFG_FN)) {
		return REPLAY_DISABLED;
	}
	read_len = file_read(cfg, (sizeof(cfg) - 1));
	file_close();

	if(replay_buf_has_word(cfg, read_len, "record")) {
		return REPLAY_RECORD;
	}
	if(
		replay_buf_has_word(cfg, read_len, "playback") ||
		replay_buf_has_word(cfg, read_len, "play")
	) {
		return REPLAY_PLAYBACK;
	}
	if((read_len != 0) && ((cfg[0] == 'v') || (cfg[0] == 'V'))) {
		return REPLAY_USER_PLAYBACK;
	}
	return REPLAY_DISABLED;
}

static replay_mode_t replay_resident_mode(void)
{
	if(
		(resident->unused_3[0] != T3_REPLAY_RES_MAGIC_0) ||
		(resident->unused_3[1] != T3_REPLAY_RES_MAGIC_1) ||
		(resident->unused_3[2] != T3_REPLAY_RES_MAGIC_2) ||
		(resident->unused_3[3] != T3_REPLAY_RES_MAGIC_3)
	) {
		return REPLAY_DISABLED;
	}
	if(resident->unused_3[T3_REPLAY_RES_MODE_INDEX] == T3_REPLAY_RES_MODE_RECORD) {
		return REPLAY_RECORD;
	}
	if(resident->unused_3[T3_REPLAY_RES_MODE_INDEX] == T3_REPLAY_RES_MODE_PLAYBACK) {
		return REPLAY_PLAYBACK;
	}
	if(resident->unused_3[T3_REPLAY_RES_MODE_INDEX] == T3_REPLAY_RES_MODE_USER_RECORD) {
		return REPLAY_USER_RECORD;
	}
	if(resident->unused_3[T3_REPLAY_RES_MODE_INDEX] == T3_REPLAY_RES_MODE_USER_PLAYBACK) {
		return REPLAY_USER_PLAYBACK;
	}
	return REPLAY_DISABLED;
}

static void replay_resident_handoff_clear(void)
{
	resident->unused_3[0] = 0;
	resident->unused_3[1] = 0;
	resident->unused_3[2] = 0;
	resident->unused_3[3] = 0;
	resident->unused_3[T3_REPLAY_RES_MODE_INDEX] = 0;
	resident->unused_3[T3_REPLAY_RES_SLOT_INDEX] = T3_REPLAY_USER_SLOT_NONE;
}

void far replay_session_start(void)
{
	replay_mode = replay_resident_mode();
	if(replay_mode == REPLAY_DISABLED) {
		replay_mode = replay_cfg_mode();
	}
	replay_sample_count = 0;
	replay_global_frame = 0;
	replay_last_route = 0;
	replay_done_written = false;
	replay_user_slot = T3_REPLAY_USER_SLOT_NONE;

	if(replay_mode == REPLAY_DISABLED) {
		return;
	}

	file_create(T3_DONE_FN);
	file_close();
	replay_split_write_header();

	if(replay_mode == REPLAY_RECORD) {
		replay_header_fill();
		if(file_create(T3_INPUT_FN)) {
			replay_write_bytes(&replay_header, sizeof(replay_header));
			file_close();
		} else {
			replay_mode = REPLAY_ERROR;
			replay_done_write("error:input-create");
			return;
		}
	} else if(replay_mode == REPLAY_USER_RECORD) {
		if(!replay_user_create()) {
			replay_mode = REPLAY_ERROR;
			replay_done_write("error:user-create");
			return;
		}
	} else if(replay_mode == REPLAY_USER_PLAYBACK) {
		if(!replay_user_read()) {
			replay_mode = REPLAY_ERROR;
			replay_done_write("error:user-header");
			return;
		}
		replay_user_snapshot_restore_resident();
		replay_user_snapshot_restore_runtime();
	} else if(!replay_header_read()) {
		replay_mode = REPLAY_ERROR;
		replay_done_write("error:input-header");
		return;
	}

	replay_split_row("start", 0);
}

void far replay_round_start(void)
{
	replay_split_row("round_start", replay_last_route);
}

void far replay_frame_io(void)
{
	bool ok = true;

	if(replay_mode == REPLAY_DISABLED) {
		return;
	}

	if(replay_mode == REPLAY_RECORD) {
		ok = replay_record_sample();
	} else if(replay_mode == REPLAY_USER_RECORD) {
		ok = replay_user_record_sample();
	} else if(replay_mode == REPLAY_USER_PLAYBACK) {
		if(replay_sample_count >= replay_user_header.sample_count) {
			replay_split_row("input_end", replay_last_route);
			input_sp |= INPUT_CANCEL;
			replay_done_write("ok:user-input-end");
			replay_resident_handoff_clear();
			replay_mode = REPLAY_DISABLED;
			return;
		}
		ok = replay_user_play_sample();
	} else if(replay_mode == REPLAY_PLAYBACK) {
		if(replay_sample_count >= replay_header.sample_count) {
			replay_split_row("input_end", replay_last_route);
			input_sp |= INPUT_CANCEL;
			replay_done_write("ok:input-end");
			replay_mode = REPLAY_DISABLED;
			return;
		}
		ok = replay_play_sample();
	}

	if(!ok) {
		replay_split_row("error", replay_last_route);
		replay_mode = REPLAY_ERROR;
		input_sp |= INPUT_CANCEL;
		replay_done_write("error:frame-io");
		return;
	}

	if((replay_global_frame & 63) == 0) {
		replay_split_row("checkpoint", replay_last_route);
		if(replay_mode == REPLAY_RECORD) {
			replay_header_write();
		} else if(replay_mode == REPLAY_USER_RECORD) {
			replay_user_header_write(RUS_RECORDING, RUER_PARTIAL);
		}
	}
	replay_global_frame++;
}

void far replay_route(uint8_t route)
{
	replay_last_route = route;
	replay_split_row("route", route);
}

void far replay_finish(uint8_t route)
{
	replay_split_row("finish", route);
	if(replay_mode == REPLAY_RECORD) {
		replay_header_write();
	} else if(replay_mode == REPLAY_USER_RECORD) {
		replay_user_header_write(
			((route == 0) ? RUS_FINALIZED : RUS_PARTIAL),
			((route == 0) ? RUER_MENU_RETURN : RUER_PARTIAL)
		);
		replay_resident_handoff_clear();
	} else if(replay_mode == REPLAY_USER_PLAYBACK) {
		replay_resident_handoff_clear();
	}
	if(replay_mode != REPLAY_DISABLED) {
		if(replay_mode == REPLAY_USER_RECORD) {
			replay_done_write((route == 0) ? "ok:menu-return" : "ok:partial");
		} else if(replay_mode == REPLAY_USER_PLAYBACK) {
			replay_done_write("ok:user-playback");
		} else {
			replay_done_write("ok");
		}
	}
	replay_mode = REPLAY_DISABLED;
}
