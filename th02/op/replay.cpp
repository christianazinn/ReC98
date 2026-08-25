// TH02 replay and Practice title surface.
//
// This file is included by op_01.cpp so that the existing OP link list stays
// untouched.  The pragma keeps all implementation in a trailing patch-owned
// code segment; all persistent state below is uninitialized BSS.

#pragma codeseg T2OPRPLY_TEXT PATCH

#include "th02/replay_format.hpp"

#define T2OP_LINE_CAPACITY 79
#define T2OP_SLOT_ROWS 10

enum t2op_word_t {
	T2OW_START,
	T2OW_EXTRA,
	T2OW_PRACTICE,
	T2OW_REPLAY,
	T2OW_HISCORE,
	T2OW_OPTIONS,
	T2OW_MUSIC_ROOM,
	T2OW_QUIT,
	T2OW_RANK,
	T2OW_EASY,
	T2OW_NORMAL,
	T2OW_HARD,
	T2OW_LUNATIC,
	T2OW_CHARACTER,
	T2OW_REIMU,
	T2OW_MARISA,
	T2OW_MIMA,
	T2OW_STAGE,
	T2OW_SECTION,
	T2OW_STAGE_START,
	T2OW_CHAPTER_2,
	T2OW_SCORE,
	T2OW_HIGH_SCORE,
	T2OW_POWER,
	T2OW_LIVES,
	T2OW_BOMBS,
	T2OW_SEED,
	T2OW_SKILL,
	T2OW_BGM,
	T2OW_REDUCED_EFFECTS,
	T2OW_OFF,
	T2OW_ON,
	T2OW_FM,
	T2OW_MIDI,
	T2OW_BROWSER,
	T2OW_SLOT,
	T2OW_NONE,
	T2OW_INVALID,
	T2OW_CLEAR,
	T2OW_GAME_OVER,
	T2OW_PAGE,
	T2OW_CONTROLS,
	T2OW_START_RUN,
	T2OW_COUNT,
};

enum t2op_main_choice_t {
	T2OMC_START,
	T2OMC_EXTRA,
	T2OMC_PRACTICE,
	T2OMC_REPLAY,
	T2OMC_HISCORE,
	T2OMC_OPTIONS,
	T2OMC_MUSIC,
	T2OMC_QUIT,
	T2OMC_COUNT,
};

enum t2op_practice_choice_t {
	T2OPC_STAGE,
	T2OPC_SECTION,
	T2OPC_RANK,
	T2OPC_CHARACTER,
	T2OPC_SCORE,
	T2OPC_HIGH_SCORE,
	T2OPC_POWER,
	T2OPC_LIVES,
	T2OPC_BOMBS,
	T2OPC_SEED,
	T2OPC_SKILL,
	T2OPC_BGM,
	T2OPC_EFFECTS,
	T2OPC_START,
	T2OPC_COUNT,
};

static char t2op_line[T2OP_LINE_CAPACITY + 1];
static char t2op_command_fn[10];
static char t2op_slot_fn[11];
static bool t2op_paths_ready;
static bool t2op_main_initialized;
static bool t2op_main_input_allowed;
static bool t2op_title_restore_needed;
static uint8_t t2op_main_sel;
static uint8_t t2op_browser_sel;
static uint8_t t2op_practice_sel;
static t2replay_header_t t2op_header;
static t2replay_start_t t2op_practice;

static void t2op_memclear(void far *buf, unsigned size)
{
	uint8_t far *p = reinterpret_cast<uint8_t far *>(buf);

	while(size != 0) {
		*p++ = 0;
		size--;
	}
}

static void t2op_paths_init(void)
{
	if(t2op_paths_ready) {
		return;
	}
	t2op_command_fn[0] = 'T';
	t2op_command_fn[1] = '2';
	t2op_command_fn[2] = 'R';
	t2op_command_fn[3] = 'P';
	t2op_command_fn[4] = 'Y';
	t2op_command_fn[5] = '.';
	t2op_command_fn[6] = 'C';
	t2op_command_fn[7] = 'F';
	t2op_command_fn[8] = 'G';
	t2op_command_fn[9] = '\0';
	t2op_slot_fn[0] = 'T';
	t2op_slot_fn[1] = 'H';
	t2op_slot_fn[2] = '2';
	t2op_slot_fn[3] = 'R';
	t2op_slot_fn[4] = '0';
	t2op_slot_fn[5] = '0';
	t2op_slot_fn[6] = '.';
	t2op_slot_fn[7] = 'R';
	t2op_slot_fn[8] = 'P';
	t2op_slot_fn[9] = 'Y';
	t2op_slot_fn[10] = '\0';
	t2op_paths_ready = true;
}

static void t2op_slot_set(uint8_t slot)
{
	t2op_slot_fn[4] = static_cast<char>('0' + (slot / 10));
	t2op_slot_fn[5] = static_cast<char>('0' + (slot % 10));
}

static bool t2op_bytes_zero(const uint8_t far *p, unsigned size)
{
	while(size != 0) {
		if(*p++ != 0) {
			return false;
		}
		size--;
	}
	return true;
}

static uint32_t t2op_fnv1a(uint32_t hash, const void far *buf, unsigned size)
{
	const uint8_t far *p = reinterpret_cast<const uint8_t far *>(buf);

	while(size != 0) {
		hash ^= static_cast<uint32_t>(*p++);
		hash *= T2REPLAY_FNV1A_PRIME;
		size--;
	}
	return hash;
}

static bool t2op_start_valid(const t2replay_start_t far *start)
{
	uint8_t practice_target = start->reserved[T2REPLAY_PRACTICE_TARGET_OFFSET];
	bool practice_target_valid = false;

	switch(practice_target) {
	case T2RPT_STAGE_START:
		practice_target_valid = true;
		break;
	case T2RPT_STAGE1_CHAPTER2:
		practice_target_valid = (start->stage == 0);
		break;
	case T2RPT_STAGE2_CHAPTER2:
		practice_target_valid = (start->stage == 1);
		break;
	case T2RPT_STAGE3_CHAPTER2:
		practice_target_valid = (start->stage == 2);
		break;
	default:
		break;
	}
	return (
		(start->stage >= 0) &&
		(start->stage < T2REPLAY_STAGE_COUNT) &&
		(start->rank <= RANK_EXTRA) &&
		((start->stage == (T2REPLAY_STAGE_COUNT - 1)) ==
		 (start->rank == RANK_EXTRA)) &&
		(start->rem_lives >= 0) &&
		(start->rem_lives <= 5) &&
		(start->rem_bombs >= 0) &&
		(start->rem_bombs <= 5) &&
		(start->start_lives >= 1) &&
		(start->start_lives <= 5) &&
		(start->start_bombs >= 1) &&
		(start->start_bombs <= 5) &&
		(start->start_power >= 0) &&
		(start->start_power <= 80) &&
		(start->random_seed == start->resident_frame) &&
		(start->shottype < SHOTTYPE_COUNT) &&
		(start->bgm_mode <= SND_BGM_MIDI) &&
		(start->reduce_effects <= 1) &&
		(start->debug == 0) &&
		practice_target_valid &&
		t2op_bytes_zero(
			&start->reserved[T2REPLAY_PRACTICE_RESERVED_OFFSET],
			T2REPLAY_PRACTICE_RESERVED_SIZE
		)
	);
}

static bool t2op_header_valid(void)
{
	uint32_t stored = t2op_header.header_checksum;
	uint32_t computed;
	uint8_t first_stage;
	uint8_t stage;

	if(
		(t2op_header.magic[0] != 'T') ||
		(t2op_header.magic[1] != '2') ||
		(t2op_header.magic[2] != 'R') ||
		(t2op_header.magic[3] != 'P') ||
		(t2op_header.magic[4] != 'Y') ||
		(t2op_header.magic[5] != '1') ||
		(t2op_header.magic[6] != '\0') ||
		(t2op_header.magic[7] != '\0') ||
		(t2op_header.version != T2REPLAY_VERSION) ||
		(t2op_header.header_size != T2REPLAY_HEADER_SIZE) ||
		(t2op_header.packet_size != T2REPLAY_PACKET_SIZE) ||
		(t2op_header.flags != T2REPLAY_KNOWN_FLAGS) ||
		(t2op_header.status != T2REPLAY_STATUS_FINALIZED) ||
		(t2op_header.game_id != 2) ||
		(t2op_header.ruleset != T2REPLAY_RULESET_STOCK) ||
		(t2op_header.input_semantics != T2REPLAY_INPUT_SEMANTICS_KEY_DET) ||
		(t2op_header.stage_count != T2REPLAY_STAGE_COUNT) ||
		(t2op_header.stage_reached >= T2REPLAY_STAGE_COUNT) ||
		(t2op_header.terminal_stage >= T2REPLAY_STAGE_COUNT) ||
		(t2op_header.end_reason < T2REPLAY_END_GAME_OVER) ||
		(t2op_header.end_reason > T2REPLAY_END_CLEAR) ||
		(t2op_header.input_offset != T2REPLAY_HEADER_SIZE) ||
		(t2op_header.input_size > T2REPLAY_INPUT_SIZE_MAX) ||
		(t2op_header.packet_count >
		 (T2REPLAY_INPUT_SIZE_MAX / T2REPLAY_PACKET_SIZE)) ||
		(t2op_header.input_size !=
		 (t2op_header.packet_count * T2REPLAY_PACKET_SIZE)) ||
		!t2op_start_valid(&t2op_header.start) ||
		!t2op_bytes_zero(t2op_header.reserved, sizeof(t2op_header.reserved))
	) {
		return false;
	}
	first_stage = static_cast<uint8_t>(t2op_header.start.stage);
	for(stage = 0; stage < T2REPLAY_STAGE_COUNT; stage++) {
		if(
			((stage < first_stage) || (stage > t2op_header.stage_reached)) &&
			(t2op_header.stage_scores[stage] != 0)
		) {
			return false;
		}
	}
	t2op_header.header_checksum = 0;
	computed = t2op_fnv1a(
		T2REPLAY_FNV1A_BASIS, &t2op_header, sizeof(t2op_header)
	);
	t2op_header.header_checksum = stored;
	return (stored == computed);
}

static bool t2op_header_read(uint8_t slot)
{
	int read;

	t2op_paths_init();
	t2op_slot_set(slot);
	if(!file_exist(t2op_slot_fn) || !file_ropen(t2op_slot_fn)) {
		return false;
	}
	t2op_memclear(&t2op_header, sizeof(t2op_header));
	read = file_read(&t2op_header, sizeof(t2op_header));
	file_close();
	if(read != sizeof(t2op_header)) {
		return false;
	}
	// MAIN owns full payload validation, including the exact file extent.  OP
	// only needs a safe, checksum-valid summary for its browser.
	return t2op_header_valid();
}

static bool t2op_command_write(
	uint8_t mode, uint8_t slot, uint8_t flags, const t2replay_start_t far *start
)
{
	t2replay_command_t command;
	int wrote;

	t2op_paths_init();
	t2op_memclear(&command, sizeof(command));
	command.magic[0] = 'T';
	command.magic[1] = '2';
	command.magic[2] = 'R';
	command.magic[3] = 'C';
	command.magic[4] = 'F';
	command.magic[5] = 'G';
	command.magic[6] = '2';
	command.magic[7] = '\0';
	command.mode = mode;
	command.slot = slot;
	command.flags = flags;
	if(start != 0) {
		command.start = *start;
	}
	if(!file_create(t2op_command_fn)) {
		return false;
	}
	wrote = file_write(&command, sizeof(command));
	file_close();
	return (wrote == sizeof(command));
}

static bool t2op_first_free_slot(uint8_t *slot)
{
	uint8_t i;

	t2op_paths_init();
	for(i = 0; i < T2REPLAY_SLOT_COUNT; i++) {
		t2op_slot_set(i);
		if(!file_exist(t2op_slot_fn)) {
			*slot = i;
			return true;
		}
	}
	return false;
}

static char *t2op_char(char *p, char c)
{
	*p++ = c;
	return p;
}

static char *t2op_word_append(char *p, t2op_word_t word)
{
	#define P(c) p = t2op_char(p, c)
	switch(word) {
	case T2OW_START: P('S'); P('t'); P('a'); P('r'); P('t'); break;
	case T2OW_EXTRA: P('E'); P('x'); P('t'); P('r'); P('a'); break;
	case T2OW_PRACTICE: P('P'); P('r'); P('a'); P('c'); P('t'); P('i'); P('c'); P('e'); break;
	case T2OW_REPLAY: P('R'); P('e'); P('p'); P('l'); P('a'); P('y'); break;
	case T2OW_HISCORE: P('H'); P('i'); P('S'); P('c'); P('o'); P('r'); P('e'); break;
	case T2OW_OPTIONS: P('O'); P('p'); P('t'); P('i'); P('o'); P('n'); P('s'); break;
	case T2OW_MUSIC_ROOM: P('M'); P('u'); P('s'); P('i'); P('c'); P(' '); P('R'); P('o'); P('o'); P('m'); break;
	case T2OW_QUIT: P('Q'); P('u'); P('i'); P('t'); break;
	case T2OW_RANK: P('R'); P('a'); P('n'); P('k'); break;
	case T2OW_EASY: P('E'); P('a'); P('s'); P('y'); break;
	case T2OW_NORMAL: P('N'); P('o'); P('r'); P('m'); P('a'); P('l'); break;
	case T2OW_HARD: P('H'); P('a'); P('r'); P('d'); break;
	case T2OW_LUNATIC: P('L'); P('u'); P('n'); P('a'); P('t'); P('i'); P('c'); break;
	case T2OW_CHARACTER: P('C'); P('h'); P('a'); P('r'); P('a'); P('c'); P('t'); P('e'); P('r'); break;
	case T2OW_REIMU: P('R'); P('e'); P('i'); P('m'); P('u'); break;
	case T2OW_MARISA: P('M'); P('a'); P('r'); P('i'); P('s'); P('a'); break;
	case T2OW_MIMA: P('M'); P('i'); P('m'); P('a'); break;
	case T2OW_STAGE: P('S'); P('t'); P('a'); P('g'); P('e'); break;
	case T2OW_SECTION: P('S'); P('e'); P('c'); P('t'); P('i'); P('o'); P('n'); break;
	case T2OW_STAGE_START: P('S'); P('t'); P('a'); P('g'); P('e'); P(' '); P('S'); P('t'); P('a'); P('r'); P('t'); break;
	case T2OW_CHAPTER_2: P('C'); P('h'); P('a'); P('p'); P('t'); P('e'); P('r'); P(' '); P('2'); break;
	case T2OW_SCORE: P('S'); P('c'); P('o'); P('r'); P('e'); break;
	case T2OW_HIGH_SCORE: P('H'); P('i'); P('g'); P('h'); P(' '); P('S'); P('c'); P('o'); P('r'); P('e'); break;
	case T2OW_POWER: P('P'); P('o'); P('w'); P('e'); P('r'); break;
	case T2OW_LIVES: P('L'); P('i'); P('v'); P('e'); P('s'); break;
	case T2OW_BOMBS: P('B'); P('o'); P('m'); P('b'); P('s'); break;
	case T2OW_SEED: P('S'); P('e'); P('e'); P('d'); break;
	case T2OW_SKILL: P('S'); P('k'); P('i'); P('l'); P('l'); break;
	case T2OW_BGM: P('B'); P('G'); P('M'); break;
	case T2OW_REDUCED_EFFECTS: P('R'); P('e'); P('d'); P('u'); P('c'); P('e'); P('d'); P(' '); P('E'); P('f'); P('f'); P('e'); P('c'); P('t'); P('s'); break;
	case T2OW_OFF: P('O'); P('f'); P('f'); break;
	case T2OW_ON: P('O'); P('n'); break;
	case T2OW_FM: P('F'); P('M'); break;
	case T2OW_MIDI: P('M'); P('I'); P('D'); P('I'); break;
	case T2OW_BROWSER: P('R'); P('e'); P('p'); P('l'); P('a'); P('y'); P(' '); P('B'); P('r'); P('o'); P('w'); P('s'); P('e'); P('r'); break;
	case T2OW_SLOT: P('S'); P('l'); P('o'); P('t'); break;
	case T2OW_NONE: P('N'); P('o'); P('n'); P('e'); break;
	case T2OW_INVALID: P('I'); P('n'); P('v'); P('a'); P('l'); P('i'); P('d'); break;
	case T2OW_CLEAR: P('C'); P('l'); P('e'); P('a'); P('r'); break;
	case T2OW_GAME_OVER: P('G'); P('a'); P('m'); P('e'); P(' '); P('O'); P('v'); P('e'); P('r'); break;
	case T2OW_PAGE: P('P'); P('a'); P('g'); P('e'); break;
	case T2OW_CONTROLS: P('A'); P('r'); P('r'); P('o'); P('w'); P('s'); P(' '); P('m'); P('o'); P('v'); P('e'); P('.'); P(' '); P('Z'); P(' '); P('s'); P('e'); P('l'); P('e'); P('c'); P('t'); P('s'); P('.'); P(' '); P('E'); P('s'); P('c'); P(' '); P('b'); P('a'); P('c'); P('k'); P('.'); break;
	case T2OW_START_RUN: P('S'); P('t'); P('a'); P('r'); P('t'); P(' '); P('R'); P('u'); P('n'); break;
	default: break;
	}
	#undef P
	return p;
}

static char *t2op_spaces_append(char *p, unsigned count)
{
	while(count != 0) {
		*p++ = ' ';
		count--;
	}
	return p;
}

static char *t2op_u32_append(char *p, uint32_t value, unsigned width)
{
	char digits[10];
	unsigned count = 0;
	unsigned i;

	do {
		digits[count++] = static_cast<char>('0' + (value % 10UL));
		value /= 10UL;
	} while(value != 0);
	p = t2op_spaces_append(p, (width > count) ? (width - count) : 0);
	for(i = count; i != 0; i--) {
		*p++ = digits[i - 1];
	}
	return p;
}

static char *t2op_i32_append(char *p, int32_t value, unsigned width)
{
	uint32_t magnitude = static_cast<uint32_t>(value);
	bool negative = (value < 0);

	if(negative) {
		magnitude = (0UL - magnitude);
		*p++ = '-';
		if(width != 0) {
			width--;
		}
	}
	return t2op_u32_append(p, magnitude, width);
}

static void t2op_text_put(tram_x_t x, tram_y_t y, tram_atrb2 attr, char *end)
{
	*end = '\0';
	text_putsa(x, y, reinterpret_cast<const shiftjis_t *>(t2op_line), attr);
}

static char *t2op_rank_append(char *p, uint8_t value)
{
	switch(value) {
	case RANK_EASY: return t2op_word_append(p, T2OW_EASY);
	case RANK_NORMAL: return t2op_word_append(p, T2OW_NORMAL);
	case RANK_HARD: return t2op_word_append(p, T2OW_HARD);
	case RANK_LUNATIC: return t2op_word_append(p, T2OW_LUNATIC);
	default: return t2op_word_append(p, T2OW_EXTRA);
	}
}

static char *t2op_character_append(char *p, uint8_t value)
{
	switch(value) {
	case 0: return t2op_word_append(p, T2OW_REIMU);
	case 1: return t2op_word_append(p, T2OW_MARISA);
	default: return t2op_word_append(p, T2OW_MIMA);
	}
}

static char *t2op_stage_append(char *p, int8_t stage)
{
	if(stage == (T2REPLAY_STAGE_COUNT - 1)) {
		return t2op_word_append(p, T2OW_EXTRA);
	}
	p = t2op_word_append(p, T2OW_STAGE);
	p = t2op_char(p, ' ');
	return t2op_char(p, static_cast<char>('1' + stage));
}

static char *t2op_bgm_append(char *p, uint8_t value)
{
	if(value == SND_BGM_OFF) {
		return t2op_word_append(p, T2OW_OFF);
	} else if(value == SND_BGM_FM) {
		return t2op_word_append(p, T2OW_FM);
	}
	return t2op_word_append(p, T2OW_MIDI);
}

static void t2op_title_background_restore(void)
{
	// OP's page 1 is the native title-background source.  Reblit it before
	// rebuilding patch text so leaving a browser never exposes stale graphics.
	palette_settone(0);
	text_clear();
	graph_showpage(0);
	graph_accesspage(1);
	pi_load_put_8_free_to(MENU_MAIN_BG_FN, 1);
	palette_entry_rgb_show(MENU_MAIN_PALETTE_FN);
	graph_copy_page(0);
	graph_accesspage(0);
	palette_100();
}

static void t2op_main_line_put(
	tram_y_t y, bool selected, t2op_word_t label, bool locked
)
{
	char *p = t2op_line;
	tram_atrb2 attr = (selected ? TX_WHITE : TX_YELLOW);

	if(locked) {
		attr = TX_BLUE;
	}
	if(selected) {
		p = t2op_char(p, '>');
		p = t2op_char(p, ' ');
	} else {
		p = t2op_char(p, ' ');
		p = t2op_char(p, ' ');
	}
	p = t2op_word_append(p, label);
	t2op_text_put(29, y, attr, p);
}

static void t2op_main_render(void)
{
	char *p;
	uint8_t row;

	if(t2op_title_restore_needed) {
		t2op_title_background_restore();
		t2op_title_restore_needed = false;
	} else {
		text_clear();
	}
	p = t2op_line;
	p = t2op_word_append(p, T2OW_REPLAY);
	p = t2op_char(p, ' ');
	p = t2op_word_append(p, T2OW_PRACTICE);
	t2op_text_put(31, 8, TX_GREEN, p);

	for(row = 0; row < T2OMC_COUNT; row++) {
		t2op_word_t label;
		bool locked = false;

		switch(row) {
		case T2OMC_START: label = T2OW_START; break;
		case T2OMC_EXTRA: label = T2OW_EXTRA; locked = !extra_unlocked; break;
		case T2OMC_PRACTICE: label = T2OW_PRACTICE; break;
		case T2OMC_REPLAY: label = T2OW_REPLAY; break;
		case T2OMC_HISCORE: label = T2OW_HISCORE; break;
		case T2OMC_OPTIONS: label = T2OW_OPTIONS; break;
		case T2OMC_MUSIC: label = T2OW_MUSIC_ROOM; break;
		default: label = T2OW_QUIT; break;
		}
		t2op_main_line_put(
			static_cast<tram_y_t>(10 + row),
			(t2op_main_sel == row), label, locked
		);
	}
	p = t2op_line;
	p = t2op_word_append(p, T2OW_RANK);
	p = t2op_char(p, ':');
	p = t2op_char(p, ' ');
	p = t2op_rank_append(p, static_cast<uint8_t>(rank));
	t2op_text_put(29, 20, TX_GREEN, p);
}

static void t2op_main_selection_step(int8_t direction)
{
	do {
		if(direction < 0) {
			t2op_main_sel = ((t2op_main_sel == 0)
				? (T2OMC_COUNT - 1)
				: (t2op_main_sel - 1)
			);
		} else {
			t2op_main_sel = ((t2op_main_sel == (T2OMC_COUNT - 1))
				? 0
				: (t2op_main_sel + 1)
			);
		}
	} while(!extra_unlocked && (t2op_main_sel == T2OMC_EXTRA));
}

static void t2op_practice_defaults(void)
{
	t2op_memclear(&t2op_practice, sizeof(t2op_practice));
	t2op_practice.resident_frame = static_cast<uint32_t>(resident->frame);
	t2op_practice.random_seed = t2op_practice.resident_frame;
	t2op_practice.score = 0;
	t2op_practice.score_highest = 0;
	t2op_practice.continues_used = 0;
	t2op_practice.skill = resident->skill;
	if(t2op_practice.skill < 0) {
		t2op_practice.skill = 0;
	} else if(t2op_practice.skill > 100) {
		t2op_practice.skill = 100;
	}
	t2op_practice.stage = 0;
	t2op_practice.rank = rank;
	if(t2op_practice.rank == RANK_EXTRA) {
		t2op_practice.rank = RANK_NORMAL;
	}
	t2op_practice.rem_lives = lives;
	t2op_practice.rem_bombs = bombs;
	t2op_practice.start_lives = lives;
	t2op_practice.start_bombs = bombs;
	t2op_practice.start_power = 1;
	t2op_practice.shottype = resident->shottype;
	t2op_practice.bgm_mode = snd_bgm_mode;
	t2op_practice.reduce_effects = (resident->reduce_effects ? 1 : 0);
}

static void t2op_practice_stage_set(int8_t stage)
{
	t2op_practice.stage = stage;
	t2op_practice.reserved[T2REPLAY_PRACTICE_TARGET_OFFSET] =
		T2RPT_STAGE_START;
	if(stage == (T2REPLAY_STAGE_COUNT - 1)) {
		t2op_practice.rank = RANK_EXTRA;
	} else if(t2op_practice.rank == RANK_EXTRA) {
		t2op_practice.rank = RANK_NORMAL;
	}
}

static void t2op_practice_rank_set(uint8_t value)
{
	t2op_practice.rank = value;
	if(value == RANK_EXTRA) {
		t2op_practice.stage = (T2REPLAY_STAGE_COUNT - 1);
		t2op_practice.reserved[T2REPLAY_PRACTICE_TARGET_OFFSET] =
			T2RPT_STAGE_START;
	} else if(t2op_practice.stage == (T2REPLAY_STAGE_COUNT - 1)) {
		t2op_practice.stage = 0;
		t2op_practice.reserved[T2REPLAY_PRACTICE_TARGET_OFFSET] =
			T2RPT_STAGE_START;
	}
}

static void t2op_practice_value_step(int8_t direction)
{
	uint32_t seed;
	uint32_t score_highest;

	switch(t2op_practice_sel) {
	case T2OPC_STAGE:
		if(direction < 0) {
			t2op_practice_stage_set((t2op_practice.stage == 0)
				? (T2REPLAY_STAGE_COUNT - 1)
				: (t2op_practice.stage - 1));
		} else {
			t2op_practice_stage_set(
				(t2op_practice.stage == (T2REPLAY_STAGE_COUNT - 1))
					? 0 : (t2op_practice.stage + 1)
			);
		}
		break;
	case T2OPC_SECTION:
		if(t2op_practice.reserved[T2REPLAY_PRACTICE_TARGET_OFFSET] !=
			T2RPT_STAGE_START) {
			t2op_practice.reserved[T2REPLAY_PRACTICE_TARGET_OFFSET] =
				T2RPT_STAGE_START;
		} else if(t2op_practice.stage <= 2) {
			t2op_practice.reserved[T2REPLAY_PRACTICE_TARGET_OFFSET] =
				static_cast<uint8_t>(t2op_practice.stage + 1);
		}
		break;
	case T2OPC_RANK:
		if(direction < 0) {
			t2op_practice_rank_set((t2op_practice.rank == RANK_EASY)
				? RANK_EXTRA : (t2op_practice.rank - 1));
		} else {
			t2op_practice_rank_set((t2op_practice.rank == RANK_EXTRA)
				? RANK_EASY : (t2op_practice.rank + 1));
		}
		break;
	case T2OPC_CHARACTER:
		if(direction < 0) {
			t2op_practice.shottype = ((t2op_practice.shottype == 0)
				? (SHOTTYPE_COUNT - 1) : (t2op_practice.shottype - 1));
		} else {
			t2op_practice.shottype = (
				(t2op_practice.shottype == (SHOTTYPE_COUNT - 1))
					? 0 : (t2op_practice.shottype + 1)
			);
		}
		break;
	case T2OPC_SCORE:
		if(direction < 0) {
			t2op_practice.score = ((t2op_practice.score < 1000) ? 0 :
				(t2op_practice.score - 1000));
		} else if(t2op_practice.score <= (2147483647L - 1000)) {
			t2op_practice.score += 1000;
		}
		if(t2op_practice.score_highest <
			static_cast<uint32_t>(t2op_practice.score)) {
			t2op_practice.score_highest = static_cast<uint32_t>(t2op_practice.score);
		}
		break;
	case T2OPC_HIGH_SCORE:
		score_highest = t2op_practice.score_highest;
		if(direction < 0) {
			if(score_highest >= (static_cast<uint32_t>(t2op_practice.score) + 1000UL)) {
				score_highest -= 1000UL;
			}
		} else if(score_highest <= (0xFFFFFFFFUL - 1000UL)) {
			score_highest += 1000UL;
		}
		t2op_practice.score_highest = score_highest;
		break;
	case T2OPC_POWER:
		if(direction < 0) {
			t2op_practice.start_power = ((t2op_practice.start_power == 1)
				? 80 : (t2op_practice.start_power - 1));
		} else {
			t2op_practice.start_power = ((t2op_practice.start_power == 80)
				? 1 : (t2op_practice.start_power + 1));
		}
		break;
	case T2OPC_LIVES:
		if(direction < 0) {
			t2op_practice.start_lives = ((t2op_practice.start_lives == 1)
				? 5 : (t2op_practice.start_lives - 1));
		} else {
			t2op_practice.start_lives = ((t2op_practice.start_lives == 5)
				? 1 : (t2op_practice.start_lives + 1));
		}
		t2op_practice.rem_lives = t2op_practice.start_lives;
		break;
	case T2OPC_BOMBS:
		if(direction < 0) {
			t2op_practice.start_bombs = ((t2op_practice.start_bombs == 1)
				? 5 : (t2op_practice.start_bombs - 1));
		} else {
			t2op_practice.start_bombs = ((t2op_practice.start_bombs == 5)
				? 1 : (t2op_practice.start_bombs + 1));
		}
		t2op_practice.rem_bombs = t2op_practice.start_bombs;
		break;
	case T2OPC_SEED:
		seed = t2op_practice.resident_frame;
		seed = (direction < 0) ? (seed - 1UL) : (seed + 1UL);
		t2op_practice.resident_frame = seed;
		t2op_practice.random_seed = seed;
		break;
	case T2OPC_SKILL:
		if(direction < 0) {
			t2op_practice.skill = ((t2op_practice.skill == 0)
				? 100 : (t2op_practice.skill - 1));
		} else {
			t2op_practice.skill = ((t2op_practice.skill == 100)
				? 0 : (t2op_practice.skill + 1));
		}
		break;
	case T2OPC_BGM:
		if(direction < 0) {
			t2op_practice.bgm_mode = ((t2op_practice.bgm_mode == SND_BGM_OFF)
				? SND_BGM_MIDI : (t2op_practice.bgm_mode - 1));
		} else {
			t2op_practice.bgm_mode = ((t2op_practice.bgm_mode == SND_BGM_MIDI)
				? SND_BGM_OFF : (t2op_practice.bgm_mode + 1));
		}
		break;
	case T2OPC_EFFECTS:
		t2op_practice.reduce_effects = (1 - t2op_practice.reduce_effects);
		break;
	default:
		break;
	}
}

static void t2op_practice_render(void)
{
	char *p;
	uint8_t row;

	text_clear();
	p = t2op_line;
	p = t2op_word_append(p, T2OW_PRACTICE);
	t2op_text_put(33, 2, TX_GREEN, p);

	for(row = 0; row < T2OPC_START; row++) {
		t2op_word_t label;
		p = t2op_line;
		switch(row) {
		case T2OPC_STAGE: label = T2OW_STAGE; break;
		case T2OPC_SECTION: label = T2OW_SECTION; break;
		case T2OPC_RANK: label = T2OW_RANK; break;
		case T2OPC_CHARACTER: label = T2OW_CHARACTER; break;
		case T2OPC_SCORE: label = T2OW_SCORE; break;
		case T2OPC_HIGH_SCORE: label = T2OW_HIGH_SCORE; break;
		case T2OPC_POWER: label = T2OW_POWER; break;
		case T2OPC_LIVES: label = T2OW_LIVES; break;
		case T2OPC_BOMBS: label = T2OW_BOMBS; break;
		case T2OPC_SEED: label = T2OW_SEED; break;
		case T2OPC_SKILL: label = T2OW_SKILL; break;
		case T2OPC_BGM: label = T2OW_BGM; break;
		default: label = T2OW_REDUCED_EFFECTS; break;
		}
		p = t2op_word_append(p, label);
		p = t2op_char(p, ':');
		p = t2op_char(p, ' ');
		switch(row) {
		case T2OPC_STAGE: p = t2op_stage_append(p, t2op_practice.stage); break;
		case T2OPC_SECTION:
			p = t2op_word_append(p,
				(t2op_practice.reserved[T2REPLAY_PRACTICE_TARGET_OFFSET] ==
				 T2RPT_STAGE_START) ? T2OW_STAGE_START : T2OW_CHAPTER_2
			);
			break;
		case T2OPC_RANK: p = t2op_rank_append(p, t2op_practice.rank); break;
		case T2OPC_CHARACTER: p = t2op_character_append(p, t2op_practice.shottype); break;
		case T2OPC_SCORE: p = t2op_i32_append(p, t2op_practice.score, 0); break;
		case T2OPC_HIGH_SCORE: p = t2op_u32_append(p, t2op_practice.score_highest, 0); break;
		case T2OPC_POWER: p = t2op_i32_append(p, t2op_practice.start_power, 0); break;
		case T2OPC_LIVES: p = t2op_u32_append(p, t2op_practice.start_lives, 0); break;
		case T2OPC_BOMBS: p = t2op_u32_append(p, t2op_practice.start_bombs, 0); break;
		case T2OPC_SEED: p = t2op_u32_append(p, t2op_practice.resident_frame, 0); break;
		case T2OPC_SKILL: p = t2op_i32_append(p, t2op_practice.skill, 0); break;
		case T2OPC_BGM: p = t2op_bgm_append(p, t2op_practice.bgm_mode); break;
		default: p = t2op_word_append(p, t2op_practice.reduce_effects ? T2OW_ON : T2OW_OFF); break;
		}
		t2op_text_put(16, static_cast<tram_y_t>(4 + row),
			(t2op_practice_sel == row) ? TX_WHITE : TX_YELLOW, p);
	}
	p = t2op_line;
	p = t2op_word_append(p, T2OW_START_RUN);
	t2op_text_put(32, static_cast<tram_y_t>(4 + T2OPC_START),
		(t2op_practice_sel == T2OPC_START) ? TX_WHITE : TX_GREEN, p);
	p = t2op_line;
	p = t2op_word_append(p, T2OW_CONTROLS);
	t2op_text_put(19, 21, TX_GREEN, p);
}

static void t2op_resident_apply(const t2replay_start_t far *start)
{
	resident->frame = static_cast<long>(start->resident_frame);
	resident->score = start->score;
	resident->score_highest = start->score_highest;
	resident->continues_used = start->continues_used;
	resident->skill = start->skill;
	resident->stage = start->stage;
	resident->rank = start->rank;
	resident->rem_lives = start->rem_lives;
	resident->rem_bombs = start->rem_bombs;
	resident->start_lives = start->start_lives;
	resident->start_bombs = start->start_bombs;
	resident->start_power = start->start_power;
	resident->shottype = start->shottype;
	resident->bgm_mode = start->bgm_mode;
	resident->reduce_effects = (start->reduce_effects != 0);
	resident->debug = false;
	resident->demo_num = 0;
	rank = start->rank;
	lives = start->start_lives;
	bombs = start->start_bombs;
	snd_bgm_mode = start->bgm_mode;
}

static void t2op_main_exec(void)
{
	char pi_fn[7];
	char main_fn[5];

	pi_fn[0] = 't'; pi_fn[1] = 's'; pi_fn[2] = '1';
	pi_fn[3] = '.'; pi_fn[4] = 'p'; pi_fn[5] = 'i'; pi_fn[6] = '\0';
	main_fn[0] = 'm'; main_fn[1] = 'a'; main_fn[2] = 'i'; main_fn[3] = 'n';
	main_fn[4] = '\0';
	pi_load(0, pi_fn);
	text_clear();
	snd_kaja_func(KAJA_SONG_FADE, 15);
	gaiji_restore();
	super_free();
	game_exit();
	execl(main_fn, main_fn, nullptr);
}

static void t2op_playback_start(uint8_t slot)
{
	// Playback owns its launch state in the replay header. Preserve the user's
	// title options before start_init() writes the transient resident fields.
	cfg_save();
	if(t2op_command_write(T2REPLAY_COMMAND_PLAYBACK, slot, 0, 0)) {
		start_init();
		t2op_main_exec();
	}
}

static void t2op_practice_start(void)
{
	uint8_t slot;

	// Keep the selected title options persistent. The Practice payload is a
	// one-run resident override consumed by MAIN, never a new configuration.
	cfg_save();
	start_init();
	t2op_resident_apply(&t2op_practice);
	if(t2op_first_free_slot(&slot)) {
		t2op_command_write(
			T2REPLAY_COMMAND_RECORD,
			slot,
			T2REPLAY_COMMAND_FLAG_PRACTICE,
			&t2op_practice
		);
	} else {
		// A full replay directory must not prevent a legitimate Practice run.
		t2op_command_write(
			T2REPLAY_COMMAND_PRACTICE,
			0,
			T2REPLAY_COMMAND_FLAG_PRACTICE,
			&t2op_practice
		);
	}
	t2op_main_exec();
}

static void t2op_record_then_start(bool extra)
{
	uint8_t slot;

	if(t2op_first_free_slot(&slot)) {
		t2op_command_write(T2REPLAY_COMMAND_RECORD, slot, 0, 0);
	}
	if(extra) {
		start_extra();
	} else {
		start_game();
	}
}

static void t2op_browser_slot_render(uint8_t slot, tram_y_t y)
{
	char *p = t2op_line;
	bool valid = t2op_header_read(slot);

	p = t2op_char(p, (slot == t2op_browser_sel) ? '>' : ' ');
	p = t2op_word_append(p, T2OW_SLOT);
	p = t2op_char(p, ' ');
	p = t2op_u32_append(p, slot, 2);
	p = t2op_spaces_append(p, 2);
	if(!valid) {
		p = t2op_word_append(p, file_exist(t2op_slot_fn) ? T2OW_INVALID : T2OW_NONE);
		t2op_text_put(5, y, (slot == t2op_browser_sel) ? TX_WHITE : TX_YELLOW, p);
		return;
	}
	p = t2op_character_append(p, t2op_header.start.shottype);
	p = t2op_spaces_append(p, 2);
	p = t2op_rank_append(p, t2op_header.start.rank);
	p = t2op_spaces_append(p, 2);
	p = t2op_i32_append(p, t2op_header.score_final, 10);
	p = t2op_spaces_append(p, 2);
	p = t2op_stage_append(p, static_cast<int8_t>(t2op_header.stage_reached));
	p = t2op_spaces_append(p, 1);
	p = t2op_word_append(
		p, (t2op_header.end_reason == T2REPLAY_END_CLEAR)
			? T2OW_CLEAR : T2OW_GAME_OVER
	);
	t2op_text_put(5, y, (slot == t2op_browser_sel) ? TX_WHITE : TX_YELLOW, p);
}

static void t2op_browser_render(void)
{
	char *p;
	uint8_t first = static_cast<uint8_t>(
		(t2op_browser_sel / T2OP_SLOT_ROWS) * T2OP_SLOT_ROWS
	);
	uint8_t i;

	text_clear();
	p = t2op_line;
	p = t2op_word_append(p, T2OW_BROWSER);
	t2op_text_put(31, 2, TX_GREEN, p);
	p = t2op_line;
	p = t2op_word_append(p, T2OW_SLOT);
	p = t2op_spaces_append(p, 5);
	p = t2op_word_append(p, T2OW_CHARACTER);
	p = t2op_spaces_append(p, 4);
	p = t2op_word_append(p, T2OW_RANK);
	p = t2op_spaces_append(p, 5);
	p = t2op_word_append(p, T2OW_SCORE);
	p = t2op_spaces_append(p, 7);
	p = t2op_word_append(p, T2OW_STAGE);
	t2op_text_put(5, 4, TX_GREEN, p);
	for(i = 0; i < T2OP_SLOT_ROWS; i++) {
		t2op_browser_slot_render(static_cast<uint8_t>(first + i), 6 + i);
	}
	p = t2op_line;
	p = t2op_word_append(p, T2OW_PAGE);
	p = t2op_char(p, ' ');
	p = t2op_u32_append(p, ((t2op_browser_sel / T2OP_SLOT_ROWS) + 1), 2);
	p = t2op_char(p, '/');
	p = t2op_u32_append(p, (T2REPLAY_SLOT_COUNT / T2OP_SLOT_ROWS), 2);
	t2op_text_put(34, 18, TX_GREEN, p);
	p = t2op_line;
	p = t2op_word_append(p, T2OW_CONTROLS);
	t2op_text_put(19, 21, TX_GREEN, p);
}

static void t2op_browser(void)
{
	bool input_allowed = false;

	t2op_browser_render();
	while(1) {
		input_reset_sense();
		if(key_det == INPUT_NONE) {
			input_allowed = true;
		}
		if(input_allowed) {
			if(key_det & INPUT_UP) {
				t2op_browser_sel = ((t2op_browser_sel == 0)
					? (T2REPLAY_SLOT_COUNT - 1) : (t2op_browser_sel - 1));
				t2op_browser_render();
			} else if(key_det & INPUT_DOWN) {
				t2op_browser_sel = ((t2op_browser_sel == (T2REPLAY_SLOT_COUNT - 1))
					? 0 : (t2op_browser_sel + 1));
				t2op_browser_render();
			} else if(key_det & INPUT_LEFT) {
				t2op_browser_sel = ((t2op_browser_sel < T2OP_SLOT_ROWS)
					? (t2op_browser_sel + (T2REPLAY_SLOT_COUNT - T2OP_SLOT_ROWS))
					: (t2op_browser_sel - T2OP_SLOT_ROWS));
				t2op_browser_render();
			} else if(key_det & INPUT_RIGHT) {
				t2op_browser_sel = ((t2op_browser_sel >=
					(T2REPLAY_SLOT_COUNT - T2OP_SLOT_ROWS))
					? (t2op_browser_sel - (T2REPLAY_SLOT_COUNT - T2OP_SLOT_ROWS))
					: (t2op_browser_sel + T2OP_SLOT_ROWS));
				t2op_browser_render();
			} else if(key_det & INPUT_CANCEL) {
				break;
			} else if((key_det & INPUT_SHOT) || (key_det & INPUT_OK)) {
				if(t2op_header_read(t2op_browser_sel)) {
					t2op_playback_start(t2op_browser_sel);
				}
			}
			if(key_det != INPUT_NONE) {
				input_allowed = false;
			}
		}
		frame_delay(1);
	}
	t2op_title_restore_needed = true;
	t2op_main_input_allowed = false;
	key_det = INPUT_NONE;
}

static void t2op_practice_menu(void)
{
	bool input_allowed = false;

	t2op_practice_defaults();
	t2op_practice_sel = T2OPC_STAGE;
	t2op_practice_render();
	while(1) {
		input_reset_sense();
		if(key_det == INPUT_NONE) {
			input_allowed = true;
		}
		if(input_allowed) {
			if(key_det & INPUT_UP) {
				t2op_practice_sel = ((t2op_practice_sel == 0)
					? (T2OPC_COUNT - 1) : (t2op_practice_sel - 1));
				t2op_practice_render();
			} else if(key_det & INPUT_DOWN) {
				t2op_practice_sel = ((t2op_practice_sel == (T2OPC_COUNT - 1))
					? 0 : (t2op_practice_sel + 1));
				t2op_practice_render();
			} else if(key_det & INPUT_LEFT) {
				t2op_practice_value_step(-1);
				t2op_practice_render();
			} else if(key_det & INPUT_RIGHT) {
				t2op_practice_value_step(+1);
				t2op_practice_render();
			} else if(key_det & INPUT_CANCEL) {
				break;
			} else if((key_det & INPUT_SHOT) || (key_det & INPUT_OK)) {
				if(t2op_practice_sel == T2OPC_START) {
					t2op_practice_start();
				}
			}
			if(key_det != INPUT_NONE) {
				input_allowed = false;
			}
		}
		frame_delay(1);
	}
	t2op_title_restore_needed = true;
	t2op_main_input_allowed = false;
	key_det = INPUT_NONE;
}

void replay_title_update_and_render(void)
{
	if(!t2op_main_initialized) {
		t2op_main_initialized = true;
		t2op_main_input_allowed = false;
		t2op_main_render();
	} else if(t2op_title_restore_needed) {
		t2op_main_render();
	}
	if(key_det == INPUT_NONE) {
		t2op_main_input_allowed = true;
	}
	if(!t2op_main_input_allowed) {
		return;
	}
	if(key_det & INPUT_UP) {
		t2op_main_selection_step(-1);
		t2op_main_render();
	} else if(key_det & INPUT_DOWN) {
		t2op_main_selection_step(+1);
		t2op_main_render();
	} else if((key_det & INPUT_SHOT) || (key_det & INPUT_OK)) {
		switch(t2op_main_sel) {
		case T2OMC_START:
			t2op_record_then_start(false);
			break;
		case T2OMC_EXTRA:
			if(extra_unlocked) {
				t2op_record_then_start(true);
			}
			break;
		case T2OMC_PRACTICE:
			t2op_practice_menu();
			t2op_main_render();
			break;
		case T2OMC_REPLAY:
			t2op_browser();
			t2op_main_render();
			break;
		case T2OMC_HISCORE:
			score_frames = 2000;
			text_clear();
			score_menu();
			t2op_title_restore_needed = true;
			t2op_main_render();
			break;
		case T2OMC_OPTIONS:
			menu_sel = 0;
			in_option = true;
			t2op_title_restore_needed = true;
			break;
		case T2OMC_MUSIC:
			text_clear();
			musicroom_menu();
			t2op_title_restore_needed = true;
			t2op_main_render();
			break;
		default:
			quit = true;
			break;
		}
	}
	if(key_det & INPUT_CANCEL) {
		quit = true;
	}
	if(key_det != INPUT_NONE) {
		t2op_main_input_allowed = false;
		idle_frame = 0;
	}
	if(idle_frame > 640) {
		start_demo();
	}
}

#pragma codeseg
