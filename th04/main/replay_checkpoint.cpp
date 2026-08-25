#pragma option -zCREPLAY_CK_TEXT

// Portable checkpoint field codecs shared by TH04 and TH05. No native
// structure image, pointer, segment, padding byte, or renderer list enters the
// stream. Decode is deliberately split into validate and apply passes so a
// malformed group cannot partially mutate live gameplay state.

#include "platform.h"
#include "x86real.h"
#include "libs/master.lib/master.hpp"
#include "th04/common.h"
#include "th04/main/frames.h"
#include "th04/main/player/move.hpp"
#include "th04/main/player/shot.hpp"
#include "th04/main/quit.hpp"
#include "th04/main/replay_checkpoint.hpp"
#include "th04/replay_format.hpp"
#include "th04/score.h"
#if (GAME == 5)
	#include "th05/main/player/bomb.hpp"
	#include "th05/main/player/bombanim.hpp"
	#include "th05/playchar.h"
	#include "th05/resident.hpp"
#else
	#include "th04/main/player/bomb.hpp"
	#include "th04/playchar.h"
	#include "th04/resident.hpp"
#endif

#define RCK_SHOT_LEVEL_COUNT 9
#define RCK_BOMB_STAR_COUNT 48

extern uint16_t randring_p;
extern uint8_t stage_id;
extern uint8_t rank;
extern uint8_t playperf;
extern uint8_t extends_gained;
extern uint32_t score_delta;
extern uint32_t score_delta_frame;
extern uint16_t stage_graze;
extern uint8_t continues_used;
extern uint8_t power;

extern bool player_is_hit;
extern uint8_t player_invincibility_time;
extern uint8_t miss_time;
extern "C" uint8_t miss_move_lock_time;
extern uint8_t shot_level;
extern nearfunc_t_near near *playchar_shot_funcs;
extern nearfunc_t_near playchar_shot_func;

#if (GAME == 5)
	extern uint8_t lives;
	extern uint8_t bombs;
	extern uint8_t dream;
	extern "C" input_t word_2CE9E;
	extern "C" uint8_t shot_hit_spark_parity;
	extern uint16_t hitshot_next_free_id;
	extern "C" nearfunc_t_near SHOT_FUNCS[PLAYCHAR_COUNT][10];
#else
	extern "C" input_t word_2598C;
	extern "C" uint8_t byte_25980;
	extern uint8_t shot_reimu_cycle;
	extern nearfunc_t_near shot_funcs_reimu_a[];
	extern nearfunc_t_near shot_funcs_reimu_b[];
	extern nearfunc_t_near shot_funcs_marisa_a[];
	extern nearfunc_t_near shot_funcs_marisa_b[];

	struct replay_bomb_star_t {
		SPPoint center;
		uint8_t angle;
		SubpixelLength8 speed;
	};
	extern replay_bomb_star_t bomb_stars[RCK_BOMB_STAR_COUNT];
#endif

static bool rck_reading(const replay_ck_stream_t far *stream)
{
	return (stream->mode != RCK_ENCODE);
}

static bool rck_applying(const replay_ck_stream_t far *stream)
{
	return (stream->mode == RCK_APPLY);
}

static void rck_stream_init(
	replay_ck_stream_t far *stream,
	void far *data,
	uint32_t size,
	replay_ck_mode_t mode
)
{
	stream->data = reinterpret_cast<uint8_t far *>(data);
	stream->limit = size;
	stream->pos = 0;
	stream->checksum = REPLAY_FNV1A_BASIS;
	stream->mode = mode;
	stream->failed = false;
}

void replay_ck_measure_init(replay_ck_stream_t far *stream)
{
	rck_stream_init(stream, 0, 0xFFFFFFFFUL, RCK_ENCODE);
}

void replay_ck_encode_init(
	replay_ck_stream_t far *stream, void far *data, uint32_t size
)
{
	rck_stream_init(stream, data, size, RCK_ENCODE);
}

void replay_ck_validate_init(
	replay_ck_stream_t far *stream, const void far *data, uint32_t size
)
{
	rck_stream_init(
		stream, const_cast<void far *>(data), size, RCK_VALIDATE
	);
}

void replay_ck_apply_init(
	replay_ck_stream_t far *stream, const void far *data, uint32_t size
)
{
	rck_stream_init(stream, const_cast<void far *>(data), size, RCK_APPLY);
}

bool replay_ck_finish(const replay_ck_stream_t far *stream)
{
	if(stream->failed) {
		return false;
	}
	if((stream->data != 0) && (stream->pos != stream->limit)) {
		return false;
	}
	return true;
}

static bool rck_u8(replay_ck_stream_t far *stream, uint8_t *value)
{
	uint8_t encoded;

	if(stream->failed || (stream->pos >= stream->limit)) {
		stream->failed = true;
		return false;
	}
	if(rck_reading(stream)) {
		encoded = stream->data[static_cast<uint16_t>(stream->pos)];
		*value = encoded;
	} else {
		encoded = *value;
		if(stream->data != 0) {
			stream->data[static_cast<uint16_t>(stream->pos)] = encoded;
		}
	}
	stream->checksum ^= static_cast<uint32_t>(encoded);
	stream->checksum *= REPLAY_FNV1A_PRIME;
	stream->pos++;
	return true;
}

static bool rck_u16(replay_ck_stream_t far *stream, uint16_t *value)
{
	uint8_t byte;
	uint16_t encoded = *value;

	byte = static_cast<uint8_t>(encoded);
	if(!rck_u8(stream, &byte)) {
		return false;
	}
	if(rck_reading(stream)) {
		encoded = byte;
	}
	byte = static_cast<uint8_t>(encoded >> 8);
	if(!rck_u8(stream, &byte)) {
		return false;
	}
	if(rck_reading(stream)) {
		encoded |= (static_cast<uint16_t>(byte) << 8);
		*value = encoded;
	}
	return true;
}

static bool rck_u32(replay_ck_stream_t far *stream, uint32_t *value)
{
	uint8_t byte;
	uint32_t encoded = *value;
	int shift;

	for(shift = 0; shift < 32; shift += 8) {
		byte = static_cast<uint8_t>(encoded >> shift);
		if(!rck_u8(stream, &byte)) {
			return false;
		}
		if(rck_reading(stream)) {
			if(shift == 0) {
				encoded = byte;
			} else {
				encoded |= (static_cast<uint32_t>(byte) << shift);
			}
		}
	}
	if(rck_reading(stream)) {
		*value = encoded;
	}
	return true;
}

#define RCK_U8(field) do { \
	uint8_t rck_v = static_cast<uint8_t>(field); \
	if(!rck_u8(stream, &rck_v)) { return false; } \
	if(rck_applying(stream)) { field = rck_v; } \
} while(0)

#define RCK_U8_RANGE(field, minimum, maximum) do { \
	uint8_t rck_v = static_cast<uint8_t>(field); \
	if( \
		!rck_u8(stream, &rck_v) || \
		(rck_v < (minimum)) || (rck_v > (maximum)) \
	) { return false; } \
	if(rck_applying(stream)) { field = rck_v; } \
} while(0)

#define RCK_U8_MAX(field, maximum) do { \
	uint8_t rck_v = static_cast<uint8_t>(field); \
	if(!rck_u8(stream, &rck_v) || (rck_v > (maximum))) { return false; } \
	if(rck_applying(stream)) { field = rck_v; } \
} while(0)

#define RCK_S8(field) do { \
	uint8_t rck_v = static_cast<uint8_t>(field); \
	if(!rck_u8(stream, &rck_v)) { return false; } \
	if(rck_applying(stream)) { field = static_cast<int8_t>(rck_v); } \
} while(0)

#define RCK_U16(field) do { \
	uint16_t rck_v = static_cast<uint16_t>(field); \
	if(!rck_u16(stream, &rck_v)) { return false; } \
	if(rck_applying(stream)) { field = rck_v; } \
} while(0)

#define RCK_U16_MAX(field, maximum) do { \
	uint16_t rck_v = static_cast<uint16_t>(field); \
	if(!rck_u16(stream, &rck_v) || (rck_v > (maximum))) { return false; } \
	if(rck_applying(stream)) { field = rck_v; } \
} while(0)

#define RCK_S16(field) do { \
	uint16_t rck_v = static_cast<uint16_t>(field); \
	if(!rck_u16(stream, &rck_v)) { return false; } \
	if(rck_applying(stream)) { field = static_cast<int16_t>(rck_v); } \
} while(0)

#define RCK_U32(field) do { \
	uint32_t rck_v = static_cast<uint32_t>(field); \
	if(!rck_u32(stream, &rck_v)) { return false; } \
	if(rck_applying(stream)) { field = rck_v; } \
} while(0)

#define RCK_S32(field) do { \
	uint32_t rck_v = static_cast<uint32_t>(field); \
	if(!rck_u32(stream, &rck_v)) { return false; } \
	if(rck_applying(stream)) { field = static_cast<int32_t>(rck_v); } \
} while(0)

#define RCK_BOOL(field) do { \
	uint8_t rck_v = ((field) ? 1 : 0); \
	if(!rck_u8(stream, &rck_v) || (rck_v > 1)) { return false; } \
	if(rck_applying(stream)) { field = (rck_v != 0); } \
} while(0)

static bool rck_sppoint(
	replay_ck_stream_t far *stream, SPPoint far *point
)
{
	RCK_S16(point->x.v);
	RCK_S16(point->y.v);
	return true;
}

static bool rck_pfpoint(
	replay_ck_stream_t far *stream, PlayfieldPoint far *point
)
{
	RCK_S16(point->x.v);
	RCK_S16(point->y.v);
	return true;
}

static bool rck_motion(
	replay_ck_stream_t far *stream, PlayfieldMotion far *motion
)
{
	if(!rck_pfpoint(stream, &motion->cur)) {
		return false;
	}
	if(!rck_pfpoint(stream, &motion->prev)) {
		return false;
	}
	return rck_pfpoint(stream, &motion->velocity);
}

static bool rck_group_rng(replay_ck_stream_t far *stream)
{
	int i;

	RCK_S32(random_seed);
	RCK_S32(resident->rand);
	for(i = 0; i < 256; i++) {
		RCK_U8(randring[i]);
	}
	RCK_U16(randring_p);
	return true;
}

static bool rck_group_run(replay_ck_stream_t far *stream)
{
	int i;
	uint8_t current_lives;
	uint8_t current_bombs;
	uint8_t quit_value = static_cast<uint8_t>(quit);

	RCK_U8_MAX(stage_id, STAGE_EXTRA);
	RCK_U8_MAX(rank, 4);
	RCK_U16(stage_frame);
	RCK_U8_MAX(stage_frame_mod2, 1);
	RCK_U8_MAX(stage_frame_mod4, 3);
	RCK_U8_MAX(stage_frame_mod8, 7);
	RCK_U8_MAX(stage_frame_mod16, 15);
	RCK_U32(total_frames);
	RCK_U16(total_std_frames);
	RCK_U32(frames_unused);

	RCK_U16(resident->std_frames);
	RCK_U16(resident->items_spawned);
	RCK_U16(resident->items_collected);
	RCK_U16(resident->point_items_collected);
	RCK_U16(resident->max_valued_point_items_collected);
	RCK_U16(resident->enemies_gone);
	RCK_U16(resident->enemies_killed);
	RCK_U16(resident->graze);
	RCK_U8(resident->miss_count);
	RCK_U8(resident->bombs_used);
	RCK_U8(resident->end_sequence);
	RCK_U32(resident->frames);
	RCK_U8_RANGE(resident->credit_lives, 1, 6);
	RCK_U8_MAX(resident->credit_bombs, ((GAME == 5) ? 3 : 2));
	RCK_U8_RANGE(resident->cfg_lives, 1, 6);
	RCK_U8_MAX(resident->cfg_bombs, ((GAME == 5) ? 3 : 2));
	RCK_U8_MAX(resident->stage, STAGE_EXTRA);
	for(i = 0; i < SCORE_DIGITS; i++) {
		RCK_U8_MAX(resident->score_last.digits[i], 9);
	}

#if (GAME == 5)
	RCK_U8_MAX(resident->playchar, PLAYCHAR_COUNT - 1);
	current_lives = lives;
	current_bombs = bombs;
#else
	{
		uint8_t playchar_ascii = resident->playchar_ascii;
		if(
			!rck_u8(stream, &playchar_ascii) ||
			(
				(playchar_ascii != ('0' + PLAYCHAR_REIMU)) &&
				(playchar_ascii != ('0' + PLAYCHAR_MARISA))
			)
		) {
			return false;
		}
		if(rck_applying(stream)) {
			resident->playchar_ascii = playchar_ascii;
		}
	}
	RCK_U8_MAX(resident->shottype, 1);
	current_lives = resident->rem_lives;
	current_bombs = resident->rem_bombs;
#endif
	if(!rck_u8(stream, &current_lives) || !rck_u8(stream, &current_bombs)) {
		return false;
	}
	if(rck_applying(stream)) {
#if (GAME == 5)
		lives = current_lives;
		bombs = current_bombs;
#else
		resident->rem_lives = current_lives;
		resident->rem_bombs = current_bombs;
#endif
	}

	RCK_U8_MAX(continues_used, 9);
	RCK_U8(playperf);
	RCK_U16(stage_graze);
	RCK_U8(extends_gained);
	RCK_U32(score_delta);
	RCK_U32(score_delta_frame);
	if(!rck_u8(stream, &quit_value) || (quit_value != Q_KEEP_RUNNING)) {
		return false;
	}
	if(rck_applying(stream)) {
		quit = Q_KEEP_RUNNING;
	}

	if((current_lives > 9) || (current_bombs > 9)) {
		return false;
	}
	return true;
}

static uint8_t rck_playchar(void)
{
#if (GAME == 5)
	return resident->playchar;
#else
	if(resident->playchar_ascii == ('0' + PLAYCHAR_REIMU)) {
		return PLAYCHAR_REIMU;
	}
	if(resident->playchar_ascii == ('0' + PLAYCHAR_MARISA)) {
		return PLAYCHAR_MARISA;
	}
	return 0xFF;
#endif
}

static nearfunc_t_near near *rck_shot_table(uint8_t id)
{
#if (GAME == 5)
	if(id < PLAYCHAR_COUNT) {
		return SHOT_FUNCS[id];
	}
#else
	switch(id) {
	case 0:
		return reinterpret_cast<nearfunc_t_near near *>(
			FP_OFF(shot_funcs_reimu_a)
		);
	case 1:
		return reinterpret_cast<nearfunc_t_near near *>(
			FP_OFF(shot_funcs_reimu_b)
		);
	case 2:
		return reinterpret_cast<nearfunc_t_near near *>(
			FP_OFF(shot_funcs_marisa_a)
		);
	case 3:
		return reinterpret_cast<nearfunc_t_near near *>(
			FP_OFF(shot_funcs_marisa_b)
		);
	}
#endif
	return 0;
}

static uint8_t rck_shot_table_id(void)
{
	uint8_t count;
	uint8_t id;

#if (GAME == 5)
	count = PLAYCHAR_COUNT;
#else
	count = 4;
#endif
	for(id = 0; id < count; id++) {
		if(playchar_shot_funcs == rck_shot_table(id)) {
			return id;
		}
	}
	return 0xFF;
}

static uint8_t rck_shot_func_id(void)
{
	int i;

	if(playchar_shot_funcs == 0) {
		return 0xFF;
	}
	for(i = 0; i < RCK_SHOT_LEVEL_COUNT; i++) {
		if(playchar_shot_func == playchar_shot_funcs[i]) {
			return static_cast<uint8_t>(i);
		}
	}
	return 0xFF;
}

static uint8_t rck_expected_shot_table(void)
{
	uint8_t playchar_id = rck_playchar();

#if (GAME == 5)
	return playchar_id;
#else
	if((playchar_id > PLAYCHAR_MARISA) || (resident->shottype > 1)) {
		return 0xFF;
	}
	return static_cast<uint8_t>((playchar_id * 2) + resident->shottype);
#endif
}

static uint8_t rck_bomb_id(void)
{
	if(playchar_bomb_func == bomb_reimu) {
		return 1;
	}
	if(playchar_bomb_func == bomb_marisa) {
		return 2;
	}
#if (GAME == 5)
	if(playchar_bomb_func == bomb_mima) {
		return 3;
	}
	if(playchar_bomb_func == bomb_yuuka) {
		return 4;
	}
#endif
	return ((playchar_bomb_func == 0) ? 0 : 0xFF);
}

static bool rck_bomb_id_apply(uint8_t id)
{
	switch(id) {
	case 0: playchar_bomb_func = 0; return true;
	case 1: playchar_bomb_func = bomb_reimu; return true;
	case 2: playchar_bomb_func = bomb_marisa; return true;
#if (GAME == 5)
	case 3: playchar_bomb_func = bomb_mima; return true;
	case 4: playchar_bomb_func = bomb_yuuka; return true;
#endif
	}
	return false;
}

static bool rck_shot(
	replay_ck_stream_t far *stream, Shot near *shot
)
{
	uint8_t flag = static_cast<uint8_t>(shot->flag);

	if(!rck_u8(stream, &flag)) {
		return false;
	}
#if (GAME == 5)
	if(flag > F_REMOVE) {
		return false;
	}
#else
	if(flag > SF_REMOVE) {
		return false;
	}
#endif
	if(rck_applying(stream)) {
#if (GAME == 5)
		shot->flag = static_cast<entity_flag_t>(flag);
#else
		shot->flag = static_cast<shot_flag_th04_t>(flag);
#endif
	}
	RCK_S8(shot->age);
	if(!rck_motion(stream, &shot->pos)) {
		return false;
	}
#if (GAME == 5)
	RCK_S8(shot->patnum_base);
	RCK_S8(shot->type);
#else
	RCK_S16(shot->patnum_base);
#endif
	RCK_S8(shot->damage);
	RCK_S8(shot->angle);
	return true;
}

#if (GAME == 5)
static bool rck_bomb_anim(replay_ck_stream_t far *stream, uint8_t variant)
{
	int i;
	int j;

	switch(variant) {
	case PLAYCHAR_REIMU:
		for(i = 0; i < REIMU_STAR_TRAILS; i++) {
			for(j = 0; j < REIMU_STAR_NODE_COUNT; j++) {
				RCK_S16(bomb_anim.reimu[i][j].topleft.screen_x.v);
				RCK_S16(bomb_anim.reimu[i][j].topleft.screen_y.v);
				if(!rck_sppoint(stream, &bomb_anim.reimu[i][j].velocity)) {
					return false;
				}
				RCK_U8(bomb_anim.reimu[i][j].angle);
			}
		}
		return true;

	case PLAYCHAR_MARISA:
		for(i = 0; i < MARISA_LASER_COUNT; i++) {
			RCK_S16(bomb_anim.marisa[i].center.x.v);
			RCK_S16(bomb_anim.marisa[i].center.y.v);
			RCK_S16(bomb_anim.marisa[i].radius);
		}
		return true;

	case PLAYCHAR_MIMA:
		for(i = 0; i < MIMA_CIRCLE_COUNT; i++) {
			RCK_S16(bomb_anim.mima[i].center.x);
			RCK_S16(bomb_anim.mima[i].center.y);
			RCK_S16(bomb_anim.mima[i].distance);
			RCK_U8(bomb_anim.mima[i].angle);
		}
		return true;

	case PLAYCHAR_YUUKA:
		RCK_S16(bomb_anim.yuuka.topleft.x);
		RCK_S16(bomb_anim.yuuka.topleft.y);
		RCK_S16(bomb_anim.yuuka.distance);
		RCK_U8(bomb_anim.yuuka.angle);
		return true;
	}
	return false;
}
#else
static bool rck_bomb_anim(replay_ck_stream_t far *stream, uint8_t variant)
{
	int i;

	if(variant > PLAYCHAR_MARISA) {
		return false;
	}
	for(i = 0; i < RCK_BOMB_STAR_COUNT; i++) {
		if(!rck_sppoint(stream, &bomb_stars[i].center)) {
			return false;
		}
		RCK_U8(bomb_stars[i].angle);
		RCK_U8(bomb_stars[i].speed.v);
	}
	return true;
}
#endif

static uint16_t rck_shot_ptr_id(void)
{
	int i;

	if(shot_ptr == 0) {
		return 0xFFFFu;
	}
	for(i = 0; i < SHOT_COUNT; i++) {
		if(shot_ptr == &shots[i]) {
			return static_cast<uint16_t>(i);
		}
	}
	return 0xFFFEu;
}

static bool rck_group_player(replay_ck_stream_t far *stream)
{
	int i;
	uint8_t shot_table = rck_shot_table_id();
	uint8_t shot_func = rck_shot_func_id();
	uint8_t bomb_id = rck_bomb_id();
	uint8_t variant = rck_playchar();
	uint16_t shot_ptr_id = rck_shot_ptr_id();
	uint16_t prev_move;
	uint8_t hit_spark;

	if(!rck_motion(stream, &player_pos)) {
		return false;
	}
	if(
		!rck_pfpoint(stream, &player_option_pos_cur) ||
		!rck_pfpoint(stream, &player_option_pos_prev)
	) {
		return false;
	}
#if (GAME == 4)
	RCK_S16(player_option_patnum);
#else
	{
		uint16_t absent = 0;
		if(!rck_u16(stream, &absent) || absent != 0) {
			return false;
		}
	}
#endif
	RCK_BOOL(player_is_hit);
	RCK_U8(player_invincibility_time);
	RCK_U8(miss_time);
	RCK_U8(miss_move_lock_time);
	RCK_U8_RANGE(power, 1, POWER_MAX);
	RCK_S16(power_overflow);
	RCK_U8_MAX(shot_level, RCK_SHOT_LEVEL_COUNT - 1);
	RCK_U8(shot_time);
	RCK_BOOL(bombing);
	RCK_BOOL(bombing_disabled);
	RCK_U8(bomb_frame);

#if (GAME == 5)
	prev_move = word_2CE9E;
	hit_spark = shot_hit_spark_parity;
#else
	prev_move = word_2598C;
	hit_spark = byte_25980;
#endif
	if(!rck_u16(stream, &prev_move) || !rck_u8(stream, &hit_spark)) {
		return false;
	}
	if(rck_applying(stream)) {
#if (GAME == 5)
		word_2CE9E = prev_move;
		shot_hit_spark_parity = hit_spark;
#else
		word_2598C = prev_move;
		byte_25980 = hit_spark;
#endif
	}

#if (GAME == 4)
	RCK_U8(shot_reimu_cycle);
#else
	{
		uint8_t absent = 0;
		if(!rck_u8(stream, &absent) || absent != 0) {
			return false;
		}
	}
#endif

	if(
		!rck_u8(stream, &shot_table) ||
		!rck_u8(stream, &shot_func) ||
		!rck_u8(stream, &bomb_id) ||
		!rck_u8(stream, &variant)
	) {
		return false;
	}
	if(
		(shot_table != rck_expected_shot_table()) ||
		(shot_func >= RCK_SHOT_LEVEL_COUNT) ||
		(variant != rck_playchar()) ||
		(bomb_id != static_cast<uint8_t>(variant + 1))
	) {
		return false;
	}
	if(rck_applying(stream)) {
		playchar_shot_funcs = rck_shot_table(shot_table);
		if(playchar_shot_funcs == 0) {
			return false;
		}
		playchar_shot_func = playchar_shot_funcs[shot_func];
		if(!rck_bomb_id_apply(bomb_id)) {
			return false;
		}
	}

#if (GAME == 5)
	RCK_S16(playchar_speed_aligned);
	RCK_S16(playchar_speed_diagonal);
#else
	{
		uint16_t aligned = playchar_speed_aligned;
		uint16_t diagonal = playchar_speed_diagonal;
		if(
			!rck_u16(stream, &aligned) ||
			!rck_u16(stream, &diagonal) ||
			(aligned != playchar_speed_aligned) ||
			(diagonal != playchar_speed_diagonal)
		) {
			return false;
		}
	}
#endif

#if (GAME == 4)
	{
		uint8_t player_bomb_id = (
			(player_bomb_func == player_bomb) ? 1 :
			(player_bomb_func == 0) ? 0 : 0xFF
		);
		if(!rck_u8(stream, &player_bomb_id) || (player_bomb_id != 1)) {
			return false;
		}
		if(rck_applying(stream)) {
			player_bomb_func = player_bomb;
		}
	}
#else
	{
		uint8_t absent = 0;
		if(!rck_u8(stream, &absent) || absent != 0) {
			return false;
		}
	}
#endif

	{
		uint8_t last_id = static_cast<uint8_t>(shot_last_id);
		if(
			!rck_u8(stream, &last_id) ||
			((last_id != 0xFF) && (last_id >= SHOT_COUNT))
		) {
			return false;
		}
		if(rck_applying(stream)) {
			shot_last_id = static_cast<int8_t>(last_id);
		}
	}
	if(!rck_u16(stream, &shot_ptr_id)) {
		return false;
	}
	if((shot_ptr_id != 0xFFFFu) && (shot_ptr_id >= SHOT_COUNT)) {
		return false;
	}
	if(rck_applying(stream)) {
		shot_ptr = (
			(shot_ptr_id == 0xFFFFu)
				? 0
				: reinterpret_cast<Shot near *>(FP_OFF(&shots[shot_ptr_id]))
		);
	}
	RCK_U16_MAX(shots_alive_count, SHOT_COUNT);
	RCK_BOOL(shots_hittest_against_boss);
	if(
		!rck_sppoint(stream, &shot_hitbox_center) ||
		!rck_sppoint(stream, &shot_hitbox_radius)
	) {
		return false;
	}

#if (GAME == 4)
	RCK_U16(shot_laser_time);
	{
		uint8_t style = static_cast<uint8_t>(shot_laser_style);
		if(!rck_u8(stream, &style) || (style >= SLS_8 + 1)) {
			return false;
		}
		if(rck_applying(stream)) {
			shot_laser_style = static_cast<shot_laser_style_t>(style);
		}
	}
	RCK_U8(shot_laser_ring_cycle);
	if(!rck_motion(stream, &shot_laser_bottomcenter)) {
		return false;
	}
#else
	{
		uint16_t absent16 = 0;
		uint8_t absent8 = 0;
		PlayfieldMotion absent_motion;

		absent_motion.cur.x.v = 0; absent_motion.cur.y.v = 0;
		absent_motion.prev.x.v = 0; absent_motion.prev.y.v = 0;
		absent_motion.velocity.x.v = 0; absent_motion.velocity.y.v = 0;
		if(
			!rck_u16(stream, &absent16) || absent16 ||
			!rck_u8(stream, &absent8) || absent8 ||
			!rck_u8(stream, &absent8) || absent8 ||
			!rck_motion(stream, &absent_motion)
		) {
			return false;
		}
	}
#endif

	for(i = 0; i < SHOT_COUNT; i++) {
		if(!rck_shot(stream, &shots[i])) {
			return false;
		}
	}

#if (GAME == 5)
	RCK_U16_MAX(hitshot_next_free_id, HITSHOT_COUNT - 1);
	for(i = 0; i < HITSHOT_COUNT; i++) {
		RCK_U8(hitshots[i].age);
		RCK_U8(hitshots[i].patnum);
		if(!rck_motion(stream, &hitshots[i].pos)) {
			return false;
		}
	}
#else
	{
		uint16_t absent = 0;
		if(!rck_u16(stream, &absent) || absent != 0) {
			return false;
		}
	}
#endif

	if(!rck_bomb_anim(stream, variant)) {
		return false;
	}

	return true;
}

bool replay_ck_group_codec(
	uint8_t group_id, replay_ck_stream_t far *stream
)
{
	if(stream == 0 || stream->failed) {
		return false;
	}
	switch(group_id) {
	case RCGI_RNG:
		return rck_group_rng(stream);
	case RCGI_RUN:
		return rck_group_run(stream);
	case RCGI_PLAYER:
		return rck_group_player(stream);
	}
	stream->failed = true;
	return false;
}

#undef RCK_U8
#undef RCK_U8_RANGE
#undef RCK_U8_MAX
#undef RCK_S8
#undef RCK_U16
#undef RCK_U16_MAX
#undef RCK_S16
#undef RCK_U32
#undef RCK_S32
#undef RCK_BOOL
