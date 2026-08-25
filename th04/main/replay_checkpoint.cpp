#pragma option -zCREPLAY_CK_TEXT

// Portable checkpoint field codecs shared by TH04 and TH05. No native
// structure image, pointer, segment, padding byte, or renderer list enters the
// stream. Decode is deliberately split into validate and apply passes so a
// malformed group cannot partially mutate live gameplay state.

#include "platform.h"
#include "x86real.h"
#include "libs/master.lib/master.hpp"
#include "th04/common.h"
#include "th04/formats/std.hpp"
#include "th04/main/bullet/bullet.hpp"
#include "th04/main/bullet/clearzap.hpp"
#include "th04/main/custom.hpp"
#include "th04/main/frames.h"
#include "th04/main/gather.hpp"
#include "th04/main/item/splash.hpp"
#include "th04/main/player/move.hpp"
#include "th04/main/player/shot.hpp"
#include "th04/main/pointnum/pointnum.hpp"
#include "th04/main/quit.hpp"
#include "th04/main/replay_checkpoint.hpp"
#include "th04/replay_format.hpp"
#include "th04/score.h"
#if (GAME == 5)
	#include "th05/main/bullet/cheeto.hpp"
	#include "th05/main/bullet/laser.hpp"
	#include "th05/main/enemy/enemy.hpp"
	#include "th05/main/player/bomb.hpp"
	#include "th05/main/player/bombanim.hpp"
	#include "th05/playchar.h"
	#include "th05/resident.hpp"
#else
	#include "th04/main/bullet/laser_t.hpp"
	#include "th04/main/enemy/enemy.hpp"
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
extern unsigned int total_max_valued_point_items;
extern unsigned char item_splash_last_id;

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
	extern unsigned char dream_items_collected;
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

static bool rck_entity_flag(
	replay_ck_stream_t far *stream, entity_flag_t far *flag
)
{
	uint8_t value = static_cast<uint8_t>(*flag);

	if(!rck_u8(stream, &value) || (value > F_REMOVE)) {
		return false;
	}
	if(rck_applying(stream)) {
		*flag = static_cast<entity_flag_t>(value);
	}
	return true;
}

static bool rck_bullet_special_motion(
	replay_ck_stream_t far *stream, bullet_special_motion_t far *motion
)
{
	uint8_t value = static_cast<uint8_t>(*motion);

	if(!rck_u8(stream, &value)) {
		return false;
	}
#if (GAME == 5)
	if((value > BSM_EXACT_LINEAR) && (value != BSM_NONE)) {
		return false;
	}
#else
	if(
		((value < BSM_DECELERATE_THEN_TURN_AIMED) ||
		(value > BSM_GRAVITY)) &&
		(value != BSM_NONE)
	) {
		return false;
	}
#endif
	if(rck_applying(stream)) {
		*motion = static_cast<bullet_special_motion_t>(value);
	}
	return true;
}

static bool rck_bullet_group_valid(uint8_t value)
{
	switch(value) {
	case BG_SINGLE:
	case BG_SINGLE_AIMED:
	case BG_RANDOM_ANGLE:
	case BG_RANDOM_ANGLE_AND_SPEED:
	case BG_SPREAD:
	case BG_SPREAD_AIMED:
	case BG_RING:
	case BG_RING_AIMED:
	case BG_STACK:
	case BG_STACK_AIMED:
	case BG_FORCESINGLE:
#if (GAME == 5)
	case BG_SPREAD_STACK:
	case BG_SPREAD_STACK_AIMED:
	case BG_RING_STACK:
	case BG_RING_STACK_AIMED:
	case BG_FORCESINGLE_AIMED:
#else
	case BG_RANDOM_CONSTRAINED_ANGLE_AIMED:
	case BG_FORCESINGLE_RANDOM_ANGLE:
#endif
		return true;
	}
	return false;
}

static bool rck_bullet_spawn_type_valid(uint8_t value)
{
#if (GAME == 5)
	switch(value) {
	case BST_NORMAL:
	case BST_GATHER_PELLET:
	case BST_CLOUD_FORWARDS:
	case BST_CLOUD_BACKWARDS:
	case (BST_NORMAL | BST_NO_DECELERATE):
	case (BST_GATHER_PELLET | BST_NO_DECELERATE):
	case (BST_CLOUD_FORWARDS | BST_NO_DECELERATE):
	case (BST_CLOUD_BACKWARDS | BST_NO_DECELERATE):
	case BST_GATHER_NORMAL_SPECIAL_MOVE:
	case BST_GATHER_ONLY:
		return true;
	}
#else
	if(value <= BST_BULLET16_CLOUD_BACKWARDS) {
		return true;
	}
#endif
	return false;
}

static bool rck_bullet_template(
	replay_ck_stream_t far *stream, BulletTemplate far *tmpl
)
{
	uint8_t spawn_type = tmpl->spawn_type;
	uint8_t group = static_cast<uint8_t>(tmpl->group);

	if(
		!rck_u8(stream, &spawn_type) ||
		!rck_bullet_spawn_type_valid(spawn_type)
	) {
		return false;
	}
	RCK_U8(tmpl->patnum);
	if(!rck_pfpoint(stream, &tmpl->origin)) {
		return false;
	}
#if (GAME == 5)
	if(!rck_u8(stream, &group) || !rck_bullet_group_valid(group)) {
		return false;
	}
	if(!rck_bullet_special_motion(stream, &tmpl->special_motion)) {
		return false;
	}
	RCK_U8(tmpl->spread);
	RCK_U8(tmpl->spread_angle_delta);
	RCK_U8(tmpl->stack);
	RCK_U8(tmpl->stack_speed_delta.v);
	RCK_U8(tmpl->angle);
	RCK_U8(tmpl->speed.v);
#else
	if(!rck_pfpoint(stream, &tmpl->velocity)) {
		return false;
	}
	if(!rck_u8(stream, &group) || !rck_bullet_group_valid(group)) {
		return false;
	}
	RCK_U8(tmpl->angle);
	RCK_U8(tmpl->speed.v);
	RCK_U8(tmpl->count);
	RCK_U8(tmpl->delta.spread_angle);
	RCK_U8(tmpl->unused_1);
	if(!rck_bullet_special_motion(stream, &tmpl->special_motion)) {
		return false;
	}
	RCK_U8(tmpl->unused_2);
#endif
	if(rck_applying(stream)) {
		tmpl->spawn_type = spawn_type;
		tmpl->group = static_cast<bullet_group_t>(group);
	}
	return true;
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

static bool rck_bullet(
	replay_ck_stream_t far *stream, bullet_t near *bullet
)
{
	uint8_t spawn_flag = static_cast<uint8_t>(bullet->spawn_flag);
	uint8_t move_flag = static_cast<uint8_t>(bullet->move_flag);

	if(!rck_entity_flag(stream, &bullet->flag)) {
		return false;
	}
	RCK_S8(bullet->age);
	if(!rck_motion(stream, &bullet->pos)) {
		return false;
	}
	RCK_U8(bullet->from_group);
	RCK_S8(bullet->unused);
	RCK_U8(bullet->speed_cur.v);
	RCK_U8(bullet->angle);
	if(
		!rck_u8(stream, &spawn_flag) || (spawn_flag > BSF_CLOUD_END) ||
		!rck_u8(stream, &move_flag) || (move_flag > BMF_DECAY_END)
	) {
		return false;
	}
	if(!rck_bullet_special_motion(stream, &bullet->special_motion)) {
		return false;
	}
	RCK_U8(bullet->speed_final.v);
	RCK_U8(bullet->u1.decelerate_time);
	RCK_U8(bullet->u2.decelerate_speed_delta.v);
	RCK_S16(bullet->patnum);
#if (GAME == 5)
	if(!rck_sppoint(stream, &bullet->origin)) {
		return false;
	}
	RCK_S16(bullet->distance.v);
#endif
	if(rck_applying(stream)) {
		bullet->spawn_flag = static_cast<bullet_spawn_flag_t>(spawn_flag);
		bullet->move_flag = static_cast<bullet_move_flag_t>(move_flag);
	}
	return true;
}

static uint8_t rck_bullet_tune_id(void)
{
	if(bullet_template_tune == bullet_template_tune_easy) {
		return 0;
	}
	if(bullet_template_tune == bullet_template_tune_normal) {
		return 1;
	}
	if(bullet_template_tune == bullet_template_tune_hard) {
		return 2;
	}
	if(bullet_template_tune == bullet_template_tune_lunatic) {
		return 3;
	}
	return 0xFF;
}

static nearfunc_t_near rck_bullet_tune_func(uint8_t id)
{
	switch(id) {
	case 0: return bullet_template_tune_easy;
	case 1: return bullet_template_tune_normal;
	case 2: return bullet_template_tune_hard;
	case 3: return bullet_template_tune_lunatic;
	}
	return 0;
}

#if (GAME == 4)
static uint8_t rck_bullet_add_id(nearfunc_t_near func, bool special)
{
	if(special) {
		if(func == bullets_add_special_easy) {
			return 0;
		}
		if(func == bullets_add_special_normal) {
			return 1;
		}
		if(func == bullets_add_special_hard_lunatic) {
			return 2;
		}
	} else {
		if(func == bullets_add_regular_easy) {
			return 0;
		}
		if(func == bullets_add_regular_normal) {
			return 1;
		}
		if(func == bullets_add_regular_hard_lunatic) {
			return 2;
		}
	}
	return 0xFF;
}

static nearfunc_t_near rck_bullet_add_func(uint8_t id, bool special)
{
	if(special) {
		switch(id) {
		case 0: return bullets_add_special_easy;
		case 1: return bullets_add_special_normal;
		case 2: return bullets_add_special_hard_lunatic;
		}
	} else {
		switch(id) {
		case 0: return bullets_add_regular_easy;
		case 1: return bullets_add_regular_normal;
		case 2: return bullets_add_regular_hard_lunatic;
		}
	}
	return 0;
}
#endif

static bool rck_custom(
	replay_ck_stream_t far *stream, custom_t near *custom
)
{
	RCK_U8(custom->flag);
	RCK_U8(custom->angle);
#if (GAME == 5)
	if(!rck_motion(stream, &custom->pos)) {
		return false;
	}
	RCK_U16(custom->val1);
	RCK_U16(custom->val2);
	RCK_S16(custom->sprite);
	RCK_S16(custom->val3);
	RCK_S16(custom->damage);
	RCK_U8(custom->speed.v);
	RCK_S8(custom->padding);
#else
	RCK_S16(custom->center.x);
	RCK_S16(custom->center.y);
	RCK_S16(custom->val1);
	RCK_S16(custom->origin_y.v);
	if(!rck_pfpoint(stream, &custom->velocity)) {
		return false;
	}
	RCK_U16(custom->val2);
	RCK_S16(custom->distance);
	RCK_S16(custom->val3);
	RCK_S16(custom->hp);
	RCK_S16(custom->damage_this_frame);
	RCK_U8(custom->val4);
	RCK_U8(custom->angle_speed);
#endif
	return true;
}

#if (GAME == 5)
static bool rck_laser(
	replay_ck_stream_t far *stream, Laser near *laser
)
{
	uint8_t flag = static_cast<uint8_t>(laser->flag);
	uint8_t width = laser->coords.width.nonshrink;

	if(!rck_u8(stream, &flag) || (flag > LF_SHOOTOUT_DECAY)) {
		return false;
	}
	RCK_U8_MAX(laser->col, 15);
	if(!rck_pfpoint(stream, &laser->coords.origin)) {
		return false;
	}
	RCK_S16(laser->coords.starts_at_distance.v);
	RCK_S16(laser->coords.ends_at_distance.v);
	RCK_U8(laser->coords.angle);
	if(!rck_u8(stream, &width)) {
		return false;
	}
	RCK_S16(laser->shootout_speed.v);
	RCK_S16(laser->age);
	RCK_S16(laser->active_at_age.grow);
	RCK_S16(laser->shrink_at_age);
	RCK_U8(laser->grow_to_width);
	if(rck_applying(stream)) {
		laser->flag = static_cast<laser_flag_t>(flag);
		laser->coords.width.nonshrink = width;
	}
	return true;
}

static bool rck_cheeto_trail(
	replay_ck_stream_t far *stream, cheeto_trail_t near *trail
)
{
	int i;
	uint8_t flag = static_cast<uint8_t>(trail->flag);

	if(!rck_u8(stream, &flag) || (flag > CF_SPEEDUP)) {
		return false;
	}
	RCK_S8(trail->col);
	for(i = 0; i < CHEETO_TRAIL_NODE_COUNT; i++) {
		if(!rck_pfpoint(stream, &trail->node_pos[i])) {
			return false;
		}
		RCK_U8(trail->node_sprite[i]);
	}
	if(rck_applying(stream)) {
		trail->flag = static_cast<cheeto_flag_t>(flag);
	}
	return true;
}
#else
static bool rck_thicklaser(
	replay_ck_stream_t far *stream, thicklaser_t near *laser
)
{
	uint8_t flag = static_cast<uint8_t>(laser->flag);

	if(!rck_u8(stream, &flag) || (flag > TF_SHRINK)) {
		return false;
	}
	if(!rck_sppoint(stream, &laser->origin)) {
		return false;
	}
	RCK_S16(laser->cur_flag_frame);
	RCK_S16(laser->line_frames);
	RCK_S16(laser->static_frames);
	RCK_U8_MAX(laser->col_outline, 15);
	RCK_S16(laser->radius_max);
	RCK_S16(laser->radius_cur);
	RCK_S16(laser->radius_speed);
	if(rck_applying(stream)) {
		laser->flag = static_cast<thicklaser_flag_t>(flag);
	}
	return true;
}
#endif

static bool rck_group_bullets(replay_ck_stream_t far *stream)
{
	int i;
	uint8_t tune_id = rck_bullet_tune_id();

	for(i = 0; i < BULLET_COUNT; i++) {
		if(!rck_bullet(stream, &bullets[i])) {
			return false;
		}
	}
	RCK_U8(bullet_special.turns_max);
	RCK_S8(bullet_template_special_angle.v);
	if(!rck_bullet_template(stream, &bullet_template)) {
		return false;
	}
	if(!rck_u8(stream, &tune_id) || (tune_id > 3)) {
		return false;
	}
	if(rck_applying(stream)) {
		bullet_template_tune = rck_bullet_tune_func(tune_id);
	}
#if (GAME == 5)
	RCK_BOOL(bullet_zap_drop_point_items);
#else
	{
		uint8_t regular_id = rck_bullet_add_id(bullets_add_regular, false);
		uint8_t special_id = rck_bullet_add_id(bullets_add_special, true);

		if(
			!rck_u8(stream, &regular_id) || (regular_id > 2) ||
			!rck_u8(stream, &special_id) || (special_id > 2)
		) {
			return false;
		}
		if(rck_applying(stream)) {
			bullets_add_regular = rck_bullet_add_func(regular_id, false);
			bullets_add_special = rck_bullet_add_func(special_id, true);
		}
	}
#endif
	RCK_U8_MAX(bullet_zap.frame, BULLET_ZAP_FRAMES);
	RCK_U8(bullet_clear_time);

	for(i = 0; i < CUSTOM_COUNT; i++) {
		if(!rck_custom(stream, &custom_entities[i])) {
			return false;
		}
	}

#if (GAME == 5)
	if(!rck_laser(stream, &laser_template)) {
		return false;
	}
	for(i = 0; i < LASER_COUNT; i++) {
		if(!rck_laser(stream, &lasers[i])) {
			return false;
		}
	}
	for(i = 0; i < (CHEETO_COUNT + 1); i++) {
		if(!rck_cheeto_trail(stream, &cheeto_trails[i])) {
			return false;
		}
	}
#else
	if(!rck_thicklaser(stream, &thicklaser_template)) {
		return false;
	}
	for(i = 0; i < THICKLASER_COUNT; i++) {
		if(!rck_thicklaser(stream, &thicklasers[i])) {
			return false;
		}
	}
#endif
	return true;
}

static bool rck_enemy_flag(
	replay_ck_stream_t far *stream, unsigned char far *flag
)
{
	uint8_t value = *flag;

	if(!rck_u8(stream, &value)) {
		return false;
	}
	if(
		(value > EF_ALIVE_FIRST_FRAME) &&
		((value < EF_KILL_ANIM) || (value > EF_KILL_ANIM_last))
	) {
		return false;
	}
	if(rck_applying(stream)) {
		*flag = value;
	}
	return true;
}

static bool rck_item_type_valid(uint8_t value)
{
	if(value <= IT_FULLPOWER || value == static_cast<uint8_t>(IT_ENEMY_DROP_NEXT)) {
		return true;
	}
#if (GAME == 5)
	if(value == static_cast<uint8_t>(IT_NONE)) {
		return true;
	}
#endif
	return false;
}

static uint8_t rck_enemy_script_id(const unsigned char near *script)
{
	int i;

	if(script == 0) {
		return 0xFF;
	}
	for(i = 0; i < STD_ENEMY_SCRIPT_COUNT; i++) {
		if(
			script == reinterpret_cast<unsigned char near *>(
				std_enemy_scripts[i]
			)
		) {
			return static_cast<uint8_t>(i);
		}
	}
	return 0xFE;
}

static uint16_t rck_enemy_cur_id(void)
{
	int i;

	if(enemy_cur == 0) {
		return 0xFFFFu;
	}
	for(i = 0; i < ENEMY_COUNT; i++) {
		if(enemy_cur == &enemies[i]) {
			return static_cast<uint16_t>(i);
		}
	}
	return 0xFFFEu;
}

static bool rck_enemy(
	replay_ck_stream_t far *stream, enemy_t near *enemy
)
{
	uint8_t script_id = rck_enemy_script_id(enemy->script);
	uint8_t item = static_cast<uint8_t>(enemy->item);

	if(!rck_enemy_flag(stream, &enemy->flag)) {
		return false;
	}
	RCK_U8(enemy->age);
	if(!rck_motion(stream, &enemy->pos)) {
		return false;
	}
	RCK_S16(enemy->hp);
	RCK_S16(enemy->score);
	if(
		!rck_u8(stream, &script_id) ||
		((script_id != 0xFF) && (script_id >= STD_ENEMY_SCRIPT_COUNT))
	) {
		return false;
	}
	RCK_S16(enemy->script_ip);
	RCK_U8(enemy->cur_instr_frame);
	RCK_U8(enemy->loop_i);
#if (GAME == 5)
	RCK_U8(enemy->speed.v);
#else
	RCK_S16(enemy->speed.v);
#endif
	RCK_U8(enemy->angle);
	RCK_U8(enemy->angle_delta);
	RCK_U8(enemy->patnum_base);
	RCK_U8(enemy->anim_cels);
	RCK_U8(enemy->anim_frames_per_cel);
	RCK_U8(enemy->anim_cur_cel);
#if (GAME == 5)
	{
		bool clip_x = ((enemy->clip & ENEMY_CLIP_X) != 0);
		bool clip_y = ((enemy->clip & ENEMY_CLIP_Y) != 0);

		RCK_BOOL(clip_x);
		RCK_BOOL(clip_y);
		if(rck_applying(stream)) {
			enemy->clip = (
				(clip_x ? ENEMY_CLIP_X : 0) |
				(clip_y ? ENEMY_CLIP_Y : 0)
			);
		}
	}
#else
	RCK_BOOL(enemy->clip_x);
	RCK_BOOL(enemy->clip_y);
#endif
	if(!rck_u8(stream, &item) || !rck_item_type_valid(item)) {
		return false;
	}
	RCK_BOOL(enemy->damaged_this_frame);
	RCK_BOOL(enemy->can_be_damaged);
	RCK_BOOL(enemy->autofire);
	RCK_BOOL(enemy->kills_player_on_collision);
	RCK_BOOL(enemy->spawned_in_left_half);
	RCK_U8(enemy->autofire_cur_frame);
	RCK_U8(enemy->autofire_interval);
#if (GAME == 5)
	RCK_U8(enemy->subtype);
#endif
	if(!rck_bullet_template(stream, &enemy->bullet_template)) {
		return false;
	}
	if(rck_applying(stream)) {
		enemy->script = (
			(script_id == 0xFF)
				? 0
				: reinterpret_cast<unsigned char near *>(
					std_enemy_scripts[script_id]
				)
		);
		enemy->item = static_cast<item_type_t>(item);
	}
	return true;
}

static bool rck_group_enemies(replay_ck_stream_t far *stream)
{
	int i;
	uint16_t enemy_cur_id = rck_enemy_cur_id();

	for(i = 0; i < ENEMY_COUNT; i++) {
		if(!rck_enemy(stream, &enemies[i])) {
			return false;
		}
	}
	if(!rck_u16(stream, &enemy_cur_id)) {
		return false;
	}
	if((enemy_cur_id != 0xFFFFu) && (enemy_cur_id >= ENEMY_COUNT)) {
		return false;
	}
	if(rck_applying(stream)) {
		enemy_cur = (
			(enemy_cur_id == 0xFFFFu)
				? 0
				: reinterpret_cast<enemy_t near *>(
					FP_OFF(&enemies[enemy_cur_id])
				)
		);
	}
	return true;
}

static bool rck_item(
	replay_ck_stream_t far *stream, item_t near *item
)
{
	uint8_t type = item->type;

	if(!rck_entity_flag(stream, &item->flag)) {
		return false;
	}
	if(!rck_motion(stream, &item->pos)) {
		return false;
	}
	if(!rck_u8(stream, &type) || (type > IT_FULLPOWER)) {
		return false;
	}
	RCK_S8(item->unknown);
	RCK_S16(item->patnum);
	RCK_BOOL(item->pulled_to_player);
	if(rck_applying(stream)) {
		item->type = type;
	}
	return true;
}

static bool rck_item_splash(
	replay_ck_stream_t far *stream, item_splash_t near *splash
)
{
	if(!rck_entity_flag(stream, &splash->flag)) {
		return false;
	}
	RCK_S8(splash->time);
	if(!rck_sppoint(stream, &splash->center)) {
		return false;
	}
	RCK_S16(splash->radius_cur.v);
	RCK_S16(splash->radius_prev.v);
	return true;
}

static bool rck_gather(
	replay_ck_stream_t far *stream, gather_t near *gather
)
{
	if(!rck_entity_flag(stream, &gather->flag)) {
		return false;
	}
	RCK_U8_MAX(gather->col, 15);
	if(!rck_motion(stream, &gather->center)) {
		return false;
	}
	RCK_S16(gather->radius_cur.v);
	RCK_S16(gather->ring_points);
	RCK_U8(gather->angle_cur);
	RCK_U8(gather->angle_delta);
	if(!rck_bullet_template(stream, &gather->bullet_template)) {
		return false;
	}
	RCK_S16(gather->radius_prev.v);
	RCK_S16(gather->radius_delta.v);
	return true;
}

static bool rck_gather_template(
	replay_ck_stream_t far *stream, gather_template_t far *tmpl
)
{
	if(
		!rck_pfpoint(stream, &tmpl->center) ||
		!rck_pfpoint(stream, &tmpl->velocity)
	) {
		return false;
	}
	RCK_S16(tmpl->radius.v);
	RCK_S16(tmpl->ring_points);
	RCK_U8_MAX(tmpl->col, 15);
	RCK_U8(tmpl->angle_delta);
	return true;
}

static bool rck_pointnum(
	replay_ck_stream_t far *stream, pointnum_t near *pointnum
)
{
	int digit;
	uint8_t flag = static_cast<uint8_t>(pointnum->flag);

	if(!rck_u8(stream, &flag) || (flag > F_REMOVE)) {
		return false;
	}
	RCK_U8(pointnum->age);
	if(!rck_sppoint(stream, &pointnum->center_cur)) {
		return false;
	}
	RCK_S16(pointnum->center_prev_y.v);
	RCK_U16(pointnum->width);
	for(digit = 0; digit < POINTNUM_DIGITS; digit++) {
		RCK_U8_MAX(pointnum->digits_lebcd[digit], 9);
	}
#if (GAME == 4)
	RCK_BOOL(pointnum->times_2);
#endif
	if(rck_applying(stream)) {
		pointnum->flag = static_cast<char>(flag);
	}
	return true;
}

static bool rck_group_items(replay_ck_stream_t far *stream)
{
	int i;

	for(i = 0; i < ITEM_COUNT; i++) {
		if(!rck_item(stream, &items[i])) {
			return false;
		}
	}
	for(i = 0; i < ITEM_SPLASH_COUNT; i++) {
		if(!rck_item_splash(stream, &item_splashes[i])) {
			return false;
		}
	}
	RCK_U8_MAX(item_splash_last_id, ITEM_SPLASH_COUNT - 1);
	RCK_U8(enemy_drop_ring_p);
	RCK_U8(item_playperf_raise);
	RCK_U8(item_playperf_lower);
	RCK_BOOL(items_pull_to_player);
	RCK_U16(items_spawned);
	RCK_U16(items_collected);
	RCK_U16(total_point_items_collected);
	RCK_U16(total_max_valued_point_items);
#if (GAME == 5)
	RCK_U16(stage_point_items_collected);
	RCK_U16(extend_point_items_collected);
	RCK_U16(item_point_score_at_full_dream);
	RCK_U8(dream);
#else
	RCK_U8(stage_point_items_collected);
	RCK_U16(dream_score);
	RCK_U8(dream_items_collected);
#endif

	for(i = 0; i < GATHER_COUNT; i++) {
		if(!rck_gather(stream, &gather_circles[i])) {
			return false;
		}
	}
	if(!rck_gather_template(stream, &gather_template)) {
		return false;
	}

	for(i = 0; i < POINTNUM_COUNT; i++) {
		if(!rck_pointnum(stream, &pointnums[i])) {
			return false;
		}
	}
	RCK_U8_MAX(pointnum_yellow_p, POINTNUM_YELLOW_COUNT - 1);
	RCK_U8_MAX(pointnum_white_p, POINTNUM_WHITE_COUNT - 1);
	RCK_BOOL(pointnum_times_2);
	if(rck_applying(stream)) {
		pointnums_alive[0] = 0;
		pointnum_first_yellow_alive = 0;
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
	case RCGI_BULLETS:
		return rck_group_bullets(stream);
	case RCGI_ENEMIES:
		return rck_group_enemies(stream);
	case RCGI_ITEMS:
		return rck_group_items(stream);
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
