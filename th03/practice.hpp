#ifndef TH03_PRACTICE_HPP
#define TH03_PRACTICE_HPP

#include "platform.h"
#include "th03/fast_forward.hpp"
#include "th03/replay_format.hpp"
#include "th03/replay_handoff.hpp"
#include "th03/resident.hpp"

// Versioned practice configuration in resident->unused_3. Keeping this block
// outside the replay cursor and fast-forward ranges lets it survive every
// OP/MAIN/MAINL handoff, including Pause-menu Restart.
#define T3_PRACTICE_RES_START_INDEX 160
#define T3_PRACTICE_RES_MAGIC_0_INDEX 160
#define T3_PRACTICE_RES_MAGIC_1_INDEX 161
#define T3_PRACTICE_RES_VERSION_INDEX 162
#define T3_PRACTICE_RES_PRESET_INDEX 163
#define T3_PRACTICE_RES_STAGE_INDEX 164
#define T3_PRACTICE_RES_ROUND_INDEX 165
#define T3_PRACTICE_RES_STOCK_INDEX 166
#define T3_PRACTICE_RES_CPU_TIMER_INDEX 167
#define T3_PRACTICE_RES_ROUND_SPEED_INDEX 168
#define T3_PRACTICE_RES_BULLET_SPEED_INDEX 169
#define T3_PRACTICE_RES_P1_SPELL_INDEX 170
#define T3_PRACTICE_RES_CPU_SPELL_INDEX 171
#define T3_PRACTICE_RES_BOSS_LEVEL_INDEX 172
#define T3_PRACTICE_RES_CPU_DAMAGE_INDEX 173
#define T3_PRACTICE_RES_SAFETY_FRAMES_INDEX 174
#define T3_PRACTICE_RES_INITIAL_STAGE_INDEX 176
#define T3_PRACTICE_RES_EXTENDS_INDEX 177
#define T3_PRACTICE_RES_P1_GAUGE_INDEX 178
#define T3_PRACTICE_RES_CPU_GAUGE_INDEX 179
#define T3_PRACTICE_RES_END_INDEX 180

#if (T3_PRACTICE_RES_START_INDEX <= T3_REPLAY_RES_MAINL_VSYNC_INDEX)
#error Practice resident block overlaps replay handoff state
#endif
#if (T3_PRACTICE_RES_END_INDEX > T3_RES_FAST_FORWARD_REPLAY_PHASE_INDEX)
#error Practice resident block overlaps fast-forward state
#endif

#define T3_PRACTICE_MAGIC_0 'P'
#define T3_PRACTICE_MAGIC_1 'R'
#define T3_PRACTICE_VERSION 3

enum practice_preset_t {
	PRACTICE_PRESET_VS_DEFAULT = 0,
	PRACTICE_PRESET_STORY_NATIVE = 1,
};

enum practice_cpu_timer_t {
	PRACTICE_CPU_TIMER_VS_DEFAULT = 0,
	PRACTICE_CPU_TIMER_STORY_NATIVE = 1,
	PRACTICE_CPU_TIMER_INFINITE = 2,
};

#define T3_PRACTICE_STOCK_VS_RULES 0xFF
#define T3_PRACTICE_GAUGE_MIN 64

inline uint8_t practice_resident_u8(unsigned int index)
{
	return static_cast<uint8_t>(resident->unused_3[index]);
}

inline void practice_resident_u8_set(unsigned int index, uint8_t value)
{
	resident->unused_3[index] = value;
}

inline uint16_t practice_resident_u16(unsigned int index)
{
	return static_cast<uint16_t>(
		practice_resident_u8(index) |
		(practice_resident_u8(index + 1) << 8)
	);
}

inline void practice_resident_u16_set(unsigned int index, uint16_t value)
{
	resident->unused_3[index + 0] = static_cast<uint8_t>(value);
	resident->unused_3[index + 1] = static_cast<uint8_t>(value >> 8);
}

inline bool practice_resident_active(void)
{
	return (
		(practice_resident_u8(T3_PRACTICE_RES_MAGIC_0_INDEX) ==
		 T3_PRACTICE_MAGIC_0) &&
		(practice_resident_u8(T3_PRACTICE_RES_MAGIC_1_INDEX) ==
		 T3_PRACTICE_MAGIC_1) &&
		(practice_resident_u8(T3_PRACTICE_RES_VERSION_INDEX) ==
		 T3_PRACTICE_VERSION)
	);
}

#define practice_game_active() ( \
	(resident->game_mode == GM_VS_1P_CPU) && practice_resident_active() \
)

inline bool practice_resident_uses_stock(void)
{
	return (
		practice_resident_active() &&
		(practice_resident_u8(T3_PRACTICE_RES_STOCK_INDEX) !=
		 T3_PRACTICE_STOCK_VS_RULES)
	);
}

inline bool practice_initial_stage_take(void)
{
	if(
		practice_resident_active() &&
		practice_resident_u8(T3_PRACTICE_RES_INITIAL_STAGE_INDEX)
	) {
		practice_resident_u8_set(T3_PRACTICE_RES_INITIAL_STAGE_INDEX, false);
		return true;
	}
	return false;
}

void far practice_resident_clear(void);

inline bool practice_replay_config_valid(
	const replay_user_practice_t near& cfg
)
{
	if(
		(cfg.preset > PRACTICE_PRESET_STORY_NATIVE) ||
		(cfg.cpu_timer > PRACTICE_CPU_TIMER_INFINITE) ||
		(cfg.round_speed > 0x7F) ||
		(cfg.bullet_speed > 0x7F) ||
		(cfg.p1_spell < 1) || (cfg.p1_spell > 16) ||
		(cfg.cpu_spell < 1) || (cfg.cpu_spell > 16) ||
		(cfg.p1_gauge < T3_PRACTICE_GAUGE_MIN) ||
		(cfg.cpu_gauge < T3_PRACTICE_GAUGE_MIN) ||
		(cfg.boss_level > 15) ||
		(cfg.cpu_damage > 3)
	) {
		return false;
	}
	if(cfg.preset == PRACTICE_PRESET_VS_DEFAULT) {
		return (
			(cfg.stage == 0) &&
			(cfg.round == 0) &&
			(cfg.stock == T3_PRACTICE_STOCK_VS_RULES) &&
			(cfg.extends_gained == 0)
		);
	}
	return (
		(cfg.stage < T3_REPLAY_USER_STAGE_COUNT) &&
		(cfg.round <= 5) &&
		(cfg.stock <= 4) &&
		(cfg.extends_gained <= 2) &&
		(cfg.stock <= (2 + cfg.extends_gained))
	);
}

inline void practice_replay_config_capture(
	replay_user_practice_t near& cfg, uint16_t initial_cpu_safety_frames
)
{
	cfg.preset = practice_resident_u8(T3_PRACTICE_RES_PRESET_INDEX);
	cfg.stage = practice_resident_u8(T3_PRACTICE_RES_STAGE_INDEX);
	cfg.round = practice_resident_u8(T3_PRACTICE_RES_ROUND_INDEX);
	cfg.stock = practice_resident_u8(T3_PRACTICE_RES_STOCK_INDEX);
	cfg.extends_gained = practice_resident_u8(T3_PRACTICE_RES_EXTENDS_INDEX);
	cfg.cpu_timer = practice_resident_u8(T3_PRACTICE_RES_CPU_TIMER_INDEX);
	cfg.round_speed = practice_resident_u8(T3_PRACTICE_RES_ROUND_SPEED_INDEX);
	cfg.bullet_speed = practice_resident_u8(T3_PRACTICE_RES_BULLET_SPEED_INDEX);
	cfg.p1_spell = practice_resident_u8(T3_PRACTICE_RES_P1_SPELL_INDEX);
	cfg.cpu_spell = practice_resident_u8(T3_PRACTICE_RES_CPU_SPELL_INDEX);
	cfg.boss_level = practice_resident_u8(T3_PRACTICE_RES_BOSS_LEVEL_INDEX);
	cfg.cpu_damage = practice_resident_u8(T3_PRACTICE_RES_CPU_DAMAGE_INDEX);
	cfg.initial_cpu_safety_frames = initial_cpu_safety_frames;
	cfg.p1_gauge = practice_resident_u8(T3_PRACTICE_RES_P1_GAUGE_INDEX);
	cfg.cpu_gauge = practice_resident_u8(T3_PRACTICE_RES_CPU_GAUGE_INDEX);
}

inline void practice_replay_config_restore(
	const replay_user_practice_t near& cfg
)
{
	practice_resident_clear();
	practice_resident_u8_set(T3_PRACTICE_RES_MAGIC_0_INDEX, T3_PRACTICE_MAGIC_0);
	practice_resident_u8_set(T3_PRACTICE_RES_MAGIC_1_INDEX, T3_PRACTICE_MAGIC_1);
	practice_resident_u8_set(T3_PRACTICE_RES_VERSION_INDEX, T3_PRACTICE_VERSION);
	practice_resident_u8_set(T3_PRACTICE_RES_PRESET_INDEX, cfg.preset);
	practice_resident_u8_set(T3_PRACTICE_RES_STAGE_INDEX, cfg.stage);
	practice_resident_u8_set(T3_PRACTICE_RES_ROUND_INDEX, cfg.round);
	practice_resident_u8_set(T3_PRACTICE_RES_STOCK_INDEX, cfg.stock);
	practice_resident_u8_set(T3_PRACTICE_RES_EXTENDS_INDEX, cfg.extends_gained);
	practice_resident_u8_set(T3_PRACTICE_RES_CPU_TIMER_INDEX, cfg.cpu_timer);
	practice_resident_u8_set(T3_PRACTICE_RES_ROUND_SPEED_INDEX, cfg.round_speed);
	practice_resident_u8_set(T3_PRACTICE_RES_BULLET_SPEED_INDEX, cfg.bullet_speed);
	practice_resident_u8_set(T3_PRACTICE_RES_P1_SPELL_INDEX, cfg.p1_spell);
	practice_resident_u8_set(T3_PRACTICE_RES_CPU_SPELL_INDEX, cfg.cpu_spell);
	practice_resident_u8_set(T3_PRACTICE_RES_BOSS_LEVEL_INDEX, cfg.boss_level);
	practice_resident_u8_set(T3_PRACTICE_RES_CPU_DAMAGE_INDEX, cfg.cpu_damage);
	practice_resident_u8_set(T3_PRACTICE_RES_P1_GAUGE_INDEX, cfg.p1_gauge);
	practice_resident_u8_set(T3_PRACTICE_RES_CPU_GAUGE_INDEX, cfg.cpu_gauge);
	practice_resident_u16_set(
		T3_PRACTICE_RES_SAFETY_FRAMES_INDEX,
		cfg.initial_cpu_safety_frames
	);
	practice_resident_u8_set(T3_PRACTICE_RES_INITIAL_STAGE_INDEX, true);
}

#if (BINARY == 'O')
playchar_t far practice_stage7_opponent(playchar_t playchar);
bool far practice_setup_menu(void);
#elif (BINARY == 'M')
uint8_t far practice_initial_round(void);
void far practice_initial_apply(void);
void far practice_retry_apply(uint8_t p1_spell, uint8_t cpu_spell);
#endif

#endif /* TH03_PRACTICE_HPP */
