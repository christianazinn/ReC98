#pragma option -zCPRACTICEO_TEXT

#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th01/math/subpixel.hpp"
#include "th01/rank.h"
#include "th02/hardware/frmdelay.h"
#include "th02/v_colors.hpp"
#include "th03/main/difficul.hpp"
#include "th03/main/player/gba.hpp"
#include "th03/main/player/stuff.hpp"
#include "th03/main/player/cpu.hpp"
#include "th03/main/round.hpp"
#include "th03/main/score.hpp"
#include "th03/menu_font.hpp"
#include "th03/op/practice_bg.hpp"
#include "th03/practice.hpp"
#include "th03/rpyfont.hpp"

enum practice_menu_color_t {
	PRACTICE_COLOR_HEADER = 10,
	PRACTICE_COLOR_FOOTER = PRACTICE_COLOR_HEADER,
	PRACTICE_COLOR_LABEL = 13,
	PRACTICE_COLOR_SELECTED = 14,
	PRACTICE_COLOR_VALUE = V_WHITE,
};

static uint8_t practice_default_round_speed(
	uint8_t rank, uint8_t round
)
{
	unsigned int speed;

	switch(rank) {
	case RANK_EASY:
		speed = (round << 4);
		break;
	case RANK_NORMAL:
		speed = (round << 5);
		break;
	case RANK_HARD:
		speed = ((round << 5) + 0x20);
		break;
	default:
		speed = 0x60;
		break;
	}
	if(speed > ROUND_SPEED_MAX) {
		speed = ROUND_SPEED_MAX;
	}
	return static_cast<uint8_t>(speed);
}

static uint8_t practice_default_bullet_speed(uint8_t rank)
{
	if(rank == RANK_HARD) {
		return 8;
	}
	if(rank == RANK_LUNATIC) {
		return 0x18;
	}
	return 0;
}

static uint8_t practice_default_boss_level(
	uint8_t rank, uint8_t stage, uint8_t round
)
{
	unsigned int level;

	switch(rank) {
	case RANK_EASY:
		level = ((stage / 2) + round);
		break;
	case RANK_NORMAL:
		level = (stage + (round * 2));
		break;
	case RANK_HARD:
		level = (stage + (round * 2) + 2);
		break;
	default:
		level = (stage + (round * 2) + 8);
		break;
	}
	if(level > GBA_BOSS_LEVEL_MAX) {
		level = GBA_BOSS_LEVEL_MAX;
	}
	return static_cast<uint8_t>(level);
}

static uint8_t practice_default_cpu_damage(uint8_t stage)
{
	if(stage < 2) {
		return 3;
	}
	if(stage < 4) {
		return 2;
	}
	if(stage < 6) {
		return 1;
	}
	return 0;
}

void far practice_resident_clear(void)
{
	for(
		int i = T3_PRACTICE_RES_START_INDEX;
		i < T3_PRACTICE_RES_END_INDEX;
		i++
	) {
		resident->unused_3[i] = 0;
	}
}

enum practice_row_t {
	PR_PRESET,
	PR_STAGE,
	PR_ROUND,
	PR_CPU_TIMER,
	PR_ROUND_SPEED,
	PR_BULLET_SPEED,
	PR_P1_SPELL,
	PR_CPU_SPELL,
	PR_BOSS_LEVEL,
	PR_CPU_DAMAGE,
	PR_STOCK,
	PR_EXTENDS,
	PR_RESET,
	PR_START,
	PR_COUNT,
};

struct practice_menu_t {
	uint8_t preset;
	uint8_t stage;
	uint8_t round;
	uint8_t cpu_timer;
	uint8_t round_speed;
	uint8_t bullet_speed;
	uint8_t p1_spell;
	uint8_t cpu_spell;
	uint8_t boss_level;
	uint8_t cpu_damage;
	uint8_t stock;
	uint8_t extends_gained;
};

static uint8_t practice_fixed_stage(void)
{
	playchar_t p1 = resident->playchar_paletted[0].char_id();
	playchar_t cpu = resident->playchar_paletted[1].char_id();

	if(cpu == PLAYCHAR_CHIYURI) {
		return STAGE_CHIYURI;
	}
	if(cpu == PLAYCHAR_YUMEMI) {
		return STAGE_YUMEMI;
	}
	if(cpu == practice_stage7_opponent(p1)) {
		return STAGE_DECISIVE;
	}
	return STAGE_NONE;
}

static bool practice_stage_allowed(uint8_t stage)
{
	uint8_t fixed_stage = practice_fixed_stage();

	if(fixed_stage != STAGE_NONE) {
		return (stage == fixed_stage);
	}
	return (stage < STAGE_DECISIVE);
}

static uint8_t practice_first_stage(void)
{
	uint8_t fixed_stage = practice_fixed_stage();

	if(fixed_stage != STAGE_NONE) {
		return fixed_stage;
	}
	return 0;
}

static void practice_defaults_set(practice_menu_t __ss& cfg)
{
	if(!practice_stage_allowed(cfg.stage)) {
		cfg.stage = practice_first_stage();
	}
	if(cfg.preset == PRACTICE_PRESET_VS_DEFAULT) {
		cfg.stage = 0;
		cfg.round = 0;
		cfg.cpu_timer = PRACTICE_CPU_TIMER_VS_DEFAULT;
		cfg.p1_spell = GBA_GAUGE_LEVEL_MIN;
		cfg.cpu_spell = GBA_GAUGE_LEVEL_MIN;
		cfg.cpu_damage = 0;
		cfg.stock = T3_PRACTICE_STOCK_VS_RULES;
		cfg.extends_gained = 0;
	} else {
		cfg.cpu_timer = PRACTICE_CPU_TIMER_STORY_NATIVE;
		cfg.p1_spell = (cfg.stage + 1);
		cfg.cpu_spell = (cfg.stage + 1);
		cfg.cpu_damage = practice_default_cpu_damage(cfg.stage);
		cfg.stock = CREDIT_LIVES;
		cfg.extends_gained = 0;
	}
	cfg.round_speed = practice_default_round_speed(resident->rank, cfg.round);
	cfg.bullet_speed = practice_default_bullet_speed(resident->rank);
	cfg.boss_level = practice_default_boss_level(
		resident->rank, cfg.stage, cfg.round
	);
}

static void practice_line_clear(char __ss *line)
{
	for(int i = 0; i < 64; i++) {
		line[i] = ' ';
	}
	line[64] = '\0';
}

static int practice_line_u16(char __ss *line, int at, uint16_t value)
{
	char digits[5];
	int count = 0;

	do {
		digits[count++] = static_cast<char>('0' + (value % 10));
		value /= 10;
	} while(value != 0);
	while(count != 0) {
		line[at++] = digits[--count];
	}
	return at;
}

static int practice_line_q4(char __ss *line, int at, uint8_t value)
{
	uint16_t fraction = static_cast<uint16_t>((value & 0x0F) * 625);

	at = practice_line_u16(line, at, (value >> 4));
	line[at++] = '.';
	line[at++] = static_cast<char>('0' + ((fraction / 1000) % 10));
	line[at++] = static_cast<char>('0' + ((fraction / 100) % 10));
	line[at++] = static_cast<char>('0' + ((fraction / 10) % 10));
	line[at++] = static_cast<char>('0' + (fraction % 10));
	return at;
}

static void practice_graphics_row_put(
	char __ss *line, int label_end, int value_at, uint8_t row, bool selected,
	bool restore, bool fixed_digits
)
{
	enum {
		LABEL_LEFT = ((RES_X / 2) - 208),
		VALUE_LEFT = ((RES_X / 2) + 16),
		CURSOR_GAP = 16,
	};
	vram_y_t top = ((4 + row) * GLYPH_H);
	int color = (
		selected ? PRACTICE_COLOR_SELECTED : PRACTICE_COLOR_VALUE
	);

	if(restore) {
		menu_font_restore_rect(0, top, RES_X, GLYPH_H);
	}
	line[label_end] = '\0';
	if(selected) {
		line[1] = '\0';
		menu_font_put(
			(LABEL_LEFT - CURSOR_GAP), top, line, PRACTICE_COLOR_SELECTED
		);
	}
	menu_font_put(
		LABEL_LEFT, top, &line[2],
		(selected ? PRACTICE_COLOR_SELECTED : PRACTICE_COLOR_LABEL)
	);
	if(value_at != 0) {
		if(fixed_digits) {
			replay_font_put_fixed_n(
				VALUE_LEFT, top, &line[value_at], 1, color
			);
			menu_font_put_n(
				(VALUE_LEFT + 16), top, &line[value_at + 1], 1, color
			);
			replay_font_put_fixed_n(
				(
					VALUE_LEFT + 16 +
					menu_font_width_n(&line[value_at + 1], 1)
				),
				top, &line[value_at + 2], 4, color
			);
		} else {
			menu_font_put(
				VALUE_LEFT, top, &line[value_at],
				color
			);
		}
	}
}

static void practice_row_put(
	practice_menu_t __ss& cfg, uint8_t row, bool selected, bool restore
)
{
	char line[65];
	int at;
	int label_end;
	int value_at = 0;

#define P(c) line[at++] = (c)
#define VALUE_COLUMN() { \
	label_end = at; \
	while(at < 18) { P(' '); } \
	value_at = at; \
}

	practice_line_clear(line);
	line[0] = (selected ? '>' : ' ');
	at = 2;
	switch(row) {
	case PR_PRESET:
		P('P'); P('r'); P('e'); P('s'); P('e'); P('t');
		VALUE_COLUMN();
		if(cfg.preset == PRACTICE_PRESET_VS_DEFAULT) {
			P('V'); P('S'); P(' '); P('D'); P('e'); P('f'); P('a'); P('u');
			P('l'); P('t');
		} else {
			P('S'); P('t'); P('o'); P('r'); P('y'); P(' '); P('N'); P('a');
			P('t'); P('i'); P('v'); P('e');
		}
		break;
	case PR_STAGE:
		P('S'); P('t'); P('a'); P('g'); P('e');
		VALUE_COLUMN();
		at = practice_line_u16(line, at, (cfg.stage + 1));
		break;
	case PR_ROUND:
		P('R'); P('o'); P('u'); P('n'); P('d');
		VALUE_COLUMN();
		at = practice_line_u16(line, at, (cfg.round + 1));
		if(cfg.round == 5) {
			line[at++] = '+';
		}
		break;
	case PR_CPU_TIMER:
		P('C'); P('P'); P('U'); P(' '); P('T'); P('i'); P('m'); P('e'); P('r');
		VALUE_COLUMN();
		if(cfg.cpu_timer == PRACTICE_CPU_TIMER_VS_DEFAULT) {
			P('V'); P('S'); P(' '); P('D'); P('e'); P('f'); P('a'); P('u');
			P('l'); P('t');
		} else if(cfg.cpu_timer == PRACTICE_CPU_TIMER_STORY_NATIVE) {
			P('S'); P('t'); P('o'); P('r'); P('y'); P(' '); P('N'); P('a');
			P('t'); P('i'); P('v'); P('e');
		} else {
			P('I'); P('n'); P('f'); P('i'); P('n'); P('i'); P('t'); P('e');
		}
		break;
	case PR_ROUND_SPEED:
		P('R'); P('o'); P('u'); P('n'); P('d'); P(' '); P('S'); P('p');
		P('e'); P('e'); P('d');
		VALUE_COLUMN();
		at = practice_line_q4(line, at, cfg.round_speed);
		break;
	case PR_BULLET_SPEED:
		P('B'); P('u'); P('l'); P('l'); P('e'); P('t'); P(' '); P('B');
		P('o'); P('n'); P('u'); P('s');
		VALUE_COLUMN();
		at = practice_line_q4(line, at, cfg.bullet_speed);
		break;
	case PR_P1_SPELL:
		P('P'); P('1'); P(' '); P('S'); P('p'); P('e'); P('l'); P('l');
		P(' '); P('R'); P('a'); P('n'); P('k');
		VALUE_COLUMN();
		at = practice_line_u16(line, at, cfg.p1_spell);
		break;
	case PR_CPU_SPELL:
		P('C'); P('P'); P('U'); P(' '); P('S'); P('p'); P('e'); P('l');
		P('l'); P(' '); P('R'); P('a'); P('n'); P('k');
		VALUE_COLUMN();
		at = practice_line_u16(line, at, cfg.cpu_spell);
		break;
	case PR_BOSS_LEVEL:
		P('B'); P('o'); P('s'); P('s'); P(' '); P('R'); P('a'); P('n'); P('k');
		VALUE_COLUMN();
		at = practice_line_u16(
			line, at, ((cfg.boss_level >= 15) ? 16 : (cfg.boss_level + 1))
		);
		P(' '); P(' '); P('r'); P('a'); P('w'); P(' ');
		at = practice_line_u16(line, at, cfg.boss_level);
		break;
	case PR_CPU_DAMAGE:
		P('C'); P('P'); P('U'); P(' '); P('D'); P('a'); P('m'); P('a');
		P('g'); P('e');
		VALUE_COLUMN();
		at = practice_line_u16(line, at, cfg.cpu_damage);
		break;
	case PR_STOCK:
		P('S'); P('t'); P('o'); P('c'); P('k');
		VALUE_COLUMN();
		if(cfg.stock == T3_PRACTICE_STOCK_VS_RULES) {
			P('V'); P('S'); P(' '); P('R'); P('u'); P('l'); P('e'); P('s');
		} else {
			at = practice_line_u16(line, at, cfg.stock);
		}
		break;
	case PR_EXTENDS:
		P('E'); P('x'); P('t'); P('e'); P('n'); P('d'); P('s'); P(' ');
		P('G'); P('a'); P('i'); P('n'); P('e'); P('d');
		VALUE_COLUMN();
		if(cfg.preset == PRACTICE_PRESET_VS_DEFAULT) {
			P('V'); P('S'); P(' '); P('R'); P('u'); P('l'); P('e'); P('s');
		} else {
			at = practice_line_u16(line, at, cfg.extends_gained);
		}
		break;
	case PR_RESET:
		P('R'); P('e'); P('s'); P('e'); P('t'); P(' '); P('D'); P('e');
		P('f'); P('a'); P('u'); P('l'); P('t'); P('s');
		break;
	default:
		P('S'); P('t'); P('a'); P('r'); P('t');
		break;
	}
	if(value_at == 0) {
		label_end = at;
	}
	if(menu_font) {
		line[at] = '\0';
		practice_graphics_row_put(
			line, label_end, value_at, row, selected, restore,
			((row == PR_ROUND_SPEED) || (row == PR_BULLET_SPEED))
		);
	} else {
		text_putsa(8, (4 + row), line, (selected ? TX_CYAN : TX_WHITE));
	}

#undef VALUE_COLUMN
#undef P
}

static void practice_rows_put(
	practice_menu_t __ss& cfg, uint8_t selected, bool restore
)
{
	for(uint8_t row = 0; row < PR_COUNT; row++) {
		practice_row_put(cfg, row, (row == selected), restore);
	}
}

static uint8_t practice_stage_step(uint8_t stage, bool forward)
{
	do {
		if(forward) {
			stage = ((stage + 1) % STAGE_COUNT);
		} else {
			stage = ((stage == 0) ? (STAGE_COUNT - 1) : (stage - 1));
		}
	} while(!practice_stage_allowed(stage));
	return stage;
}

static void practice_value_step(
	practice_menu_t __ss& cfg, uint8_t row, bool forward
)
{
	switch(row) {
	case PR_PRESET:
		cfg.preset = (1 - cfg.preset);
		practice_defaults_set(cfg);
		break;
	case PR_STAGE:
		if(cfg.preset == PRACTICE_PRESET_STORY_NATIVE) {
			cfg.stage = practice_stage_step(cfg.stage, forward);
			practice_defaults_set(cfg);
		}
		break;
	case PR_ROUND:
		if(cfg.preset == PRACTICE_PRESET_STORY_NATIVE) {
			if(forward) {
				cfg.round = ((cfg.round == 5) ? 0 : (cfg.round + 1));
			} else {
				cfg.round = ((cfg.round == 0) ? 5 : (cfg.round - 1));
			}
			practice_defaults_set(cfg);
		}
		break;
	case PR_CPU_TIMER:
		if(forward) {
			cfg.cpu_timer = (
				(cfg.cpu_timer == PRACTICE_CPU_TIMER_INFINITE) ?
				PRACTICE_CPU_TIMER_VS_DEFAULT : (cfg.cpu_timer + 1)
			);
		} else {
			cfg.cpu_timer = (
				(cfg.cpu_timer == PRACTICE_CPU_TIMER_VS_DEFAULT) ?
				PRACTICE_CPU_TIMER_INFINITE : (cfg.cpu_timer - 1)
			);
		}
		break;
	case PR_ROUND_SPEED:
		if(forward && (cfg.round_speed < ROUND_SPEED_MAX)) {
			cfg.round_speed++;
		} else if(!forward && (cfg.round_speed != 0)) {
			cfg.round_speed--;
		}
		break;
	case PR_BULLET_SPEED:
		if(forward && (cfg.bullet_speed < ROUND_SPEED_MAX)) {
			cfg.bullet_speed++;
		} else if(!forward && (cfg.bullet_speed != 0)) {
			cfg.bullet_speed--;
		}
		break;
	case PR_P1_SPELL:
		if(forward) {
			cfg.p1_spell = (
				(cfg.p1_spell == GBA_GAUGE_LEVEL_MAX) ?
				GBA_GAUGE_LEVEL_MIN : (cfg.p1_spell + 1)
			);
		} else {
			cfg.p1_spell = (
				(cfg.p1_spell == GBA_GAUGE_LEVEL_MIN) ?
				GBA_GAUGE_LEVEL_MAX : (cfg.p1_spell - 1)
			);
		}
		break;
	case PR_CPU_SPELL:
		if(forward) {
			cfg.cpu_spell = (
				(cfg.cpu_spell == GBA_GAUGE_LEVEL_MAX) ?
				GBA_GAUGE_LEVEL_MIN : (cfg.cpu_spell + 1)
			);
		} else {
			cfg.cpu_spell = (
				(cfg.cpu_spell == GBA_GAUGE_LEVEL_MIN) ?
				GBA_GAUGE_LEVEL_MAX : (cfg.cpu_spell - 1)
			);
		}
		break;
	case PR_BOSS_LEVEL:
		if(forward && (cfg.boss_level < GBA_BOSS_LEVEL_MAX)) {
			cfg.boss_level++;
		} else if(!forward && (cfg.boss_level != 0)) {
			cfg.boss_level--;
		}
		break;
	case PR_CPU_DAMAGE:
		if(forward) {
			cfg.cpu_damage = ((cfg.cpu_damage == 3) ? 0 : (cfg.cpu_damage + 1));
		} else {
			cfg.cpu_damage = ((cfg.cpu_damage == 0) ? 3 : (cfg.cpu_damage - 1));
		}
		break;
	case PR_STOCK:
		if(cfg.preset == PRACTICE_PRESET_STORY_NATIVE) {
			if(forward) {
				cfg.stock = ((cfg.stock == 4) ? 0 : (cfg.stock + 1));
			} else {
				cfg.stock = ((cfg.stock == 0) ? 4 : (cfg.stock - 1));
			}
			// A fresh credit starts at 2 stock; each prior extend adds one.
			if(
				(cfg.stock > CREDIT_LIVES) &&
				(cfg.extends_gained < (cfg.stock - CREDIT_LIVES))
			) {
				cfg.extends_gained = (cfg.stock - CREDIT_LIVES);
			}
		}
		break;
	case PR_EXTENDS:
		if(cfg.preset == PRACTICE_PRESET_STORY_NATIVE) {
			if(forward) {
				cfg.extends_gained = (
					(cfg.extends_gained == EXTENDS_MAX) ?
					0 : (cfg.extends_gained + 1)
				);
			} else {
				cfg.extends_gained = (
					(cfg.extends_gained == 0) ?
					EXTENDS_MAX : (cfg.extends_gained - 1)
				);
			}
			if(cfg.stock > (CREDIT_LIVES + cfg.extends_gained)) {
				cfg.stock = (CREDIT_LIVES + cfg.extends_gained);
			}
		}
		break;
	}
}

static bool practice_is_exact_vs_default(practice_menu_t __ss& cfg)
{
	return (
		(cfg.preset == PRACTICE_PRESET_VS_DEFAULT) &&
		(cfg.stage == 0) &&
		(cfg.round == 0) &&
		(cfg.cpu_timer == PRACTICE_CPU_TIMER_VS_DEFAULT) &&
		(cfg.round_speed == practice_default_round_speed(resident->rank, 0)) &&
		(cfg.bullet_speed == practice_default_bullet_speed(resident->rank)) &&
		(cfg.p1_spell == GBA_GAUGE_LEVEL_MIN) &&
		(cfg.cpu_spell == GBA_GAUGE_LEVEL_MIN) &&
		(cfg.boss_level == practice_default_boss_level(
			resident->rank, 0, 0
		)) &&
		(cfg.cpu_damage == 0) &&
		(cfg.stock == T3_PRACTICE_STOCK_VS_RULES) &&
		(cfg.extends_gained == 0)
	);
}

static void practice_config_store(practice_menu_t __ss& cfg)
{
	practice_resident_clear();
	practice_resident_u8_set(T3_PRACTICE_RES_MAGIC_0_INDEX, T3_PRACTICE_MAGIC_0);
	practice_resident_u8_set(T3_PRACTICE_RES_MAGIC_1_INDEX, T3_PRACTICE_MAGIC_1);
	practice_resident_u8_set(T3_PRACTICE_RES_VERSION_INDEX, T3_PRACTICE_VERSION);
	practice_resident_u8_set(T3_PRACTICE_RES_PRESET_INDEX, cfg.preset);
	practice_resident_u8_set(T3_PRACTICE_RES_STAGE_INDEX, cfg.stage);
	practice_resident_u8_set(T3_PRACTICE_RES_ROUND_INDEX, cfg.round);
	practice_resident_u8_set(T3_PRACTICE_RES_STOCK_INDEX, cfg.stock);
	practice_resident_u8_set(T3_PRACTICE_RES_CPU_TIMER_INDEX, cfg.cpu_timer);
	practice_resident_u8_set(T3_PRACTICE_RES_ROUND_SPEED_INDEX, cfg.round_speed);
	practice_resident_u8_set(T3_PRACTICE_RES_BULLET_SPEED_INDEX, cfg.bullet_speed);
	practice_resident_u8_set(T3_PRACTICE_RES_P1_SPELL_INDEX, cfg.p1_spell);
	practice_resident_u8_set(T3_PRACTICE_RES_CPU_SPELL_INDEX, cfg.cpu_spell);
	practice_resident_u8_set(T3_PRACTICE_RES_BOSS_LEVEL_INDEX, cfg.boss_level);
	practice_resident_u8_set(T3_PRACTICE_RES_CPU_DAMAGE_INDEX, cfg.cpu_damage);
	practice_resident_u8_set(
		T3_PRACTICE_RES_EXTENDS_INDEX, cfg.extends_gained
	);
	practice_resident_u8_set(T3_PRACTICE_RES_INITIAL_STAGE_INDEX, true);

	resident->story_stage = cfg.stage;
	resident->story_lives = (
		(cfg.stock == T3_PRACTICE_STOCK_VS_RULES) ? 0 : cfg.stock
	);
}

static void practice_heading_put(bool restore)
{
	char line[65];
	int at;

#define P(c) line[at++] = (c)

	practice_line_clear(line);
	at = 0;
	P('P'); P('R'); P('A'); P('C'); P('T'); P('I'); P('C'); P('E');
	P(' '); P('S'); P('E'); P('T'); P('U'); P('P');
	line[at] = '\0';
	if(menu_font) {
		if(restore) {
			menu_font_restore_rect(0, (2 * GLYPH_H), RES_X, GLYPH_H);
		}
		menu_font_put_centered(
			(RES_X / 2), (2 * GLYPH_H), line, PRACTICE_COLOR_HEADER
		);
	} else {
		text_putsa(31, 2, line, TX_WHITE);
	}

	practice_line_clear(line);
	at = 0;
	P('A'); P('r'); P('r'); P('o'); P('w'); P('s'); P(':'); P(' ');
	P('S'); P('e'); P('l'); P('e'); P('c'); P('t'); P(' '); P('/'); P(' ');
	P('C'); P('h'); P('a'); P('n'); P('g'); P('e'); P(' '); P(' '); P(' ');
	P('Z'); P(':'); P(' '); P('C'); P('o'); P('n'); P('f'); P('i'); P('r');
	P('m'); P(' '); P(' '); P(' '); P('E'); P('s'); P('c'); P(':'); P(' ');
	P('B'); P('a'); P('c'); P('k');
	line[at] = '\0';
	if(menu_font) {
		if(restore) {
			menu_font_restore_rect(0, (20 * GLYPH_H), RES_X, GLYPH_H);
		}
		menu_font_put_centered(
			(RES_X / 2), (20 * GLYPH_H), line, PRACTICE_COLOR_FOOTER
		);
	} else {
		text_putsa(12, 20, line, TX_WHITE);
	}

#undef P
}

static void practice_screen_put(practice_menu_t __ss& cfg, uint8_t selected)
{
	select_vs_cpu_practice_background_put();
	practice_heading_put(false);
	practice_rows_put(cfg, selected, false);
	select_vs_cpu_practice_frame_finish();
}

static void practice_screen_clear(void)
{
	text_clear();
	graph_accesspage(0);
	graph_clear();
	graph_accesspage(1);
	graph_clear();
	graph_showpage(0);
	graph_accesspage(0);
}

bool far practice_setup_menu(void)
{
	practice_menu_t cfg;
	uint8_t selected = 0;
	input_t input_prev;

	cfg.preset = PRACTICE_PRESET_STORY_NATIVE;
	cfg.stage = practice_first_stage();
	cfg.round = 0;
	practice_defaults_set(cfg);

	text_clear();
	palette_100();
	palette_set(PRACTICE_COLOR_SELECTED, 0x20, 0xE0, 0xFF);
	palette_show();

	input_mode_interface();
	input_prev = input_sp;
	while(1) {
		practice_screen_put(cfg, selected);
		input_mode_interface();
		if(input_prev == INPUT_NONE) {
			if(input_sp & INPUT_UP) {
				selected = ((selected == 0) ? (PR_COUNT - 1) : (selected - 1));
			} else if(input_sp & INPUT_DOWN) {
				selected = ((selected == (PR_COUNT - 1)) ? 0 : (selected + 1));
			} else if(input_sp & INPUT_LEFT) {
				practice_value_step(cfg, selected, false);
			} else if(input_sp & INPUT_RIGHT) {
				practice_value_step(cfg, selected, true);
			} else if(input_sp & (INPUT_OK | INPUT_SHOT)) {
				if(selected == PR_RESET) {
					practice_defaults_set(cfg);
				} else if(selected == PR_START) {
					if(practice_is_exact_vs_default(cfg)) {
						practice_resident_clear();
						resident->story_stage = 0;
						resident->story_lives = 0;
					} else {
						practice_config_store(cfg);
					}
					palette_black_out(1);
					practice_screen_clear();
					return false;
				}
			} else if(input_sp & INPUT_CANCEL) {
				practice_resident_clear();
				select_vs_cpu_practice_palette_restore();
				text_clear();
				select_vs_cpu_practice_background_put();
				select_vs_cpu_practice_frame_finish();
				return true;
			}
		}
		input_prev = input_sp;
	}
}

// Keep the compiler runtime segment at its accepted paragraph phase.
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
