/* TH02 schema-4 common-world checkpoint apply substrate.
 *
 * This file is included as T2CAPPLY_TEXT at the end of s1_actor.cpp. It is
 * deliberately not a live seek hook: a later coordinator owns actors, stage
 * callbacks, palette/tile state, redraw, and the presentation boundary.
 */

#include "platform.h"
#include "libs/master.lib/master.hpp"
#include "th01/rank.h"
#include "th02/common.h"
#include "th02/replay_format.hpp"
#include "th02/resident.hpp"
#include "th02/core/globals.hpp"
#include "th02/hardware/pages.hpp"
#include "th02/math/randring.hpp"
#include "th02/snd/snd.h"
#include "th02/main/frames.hpp"
#include "th02/main/main.hpp"
#include "th02/main/midboss/midboss.hpp"
#include "th02/main/playperf.hpp"
#include "th02/main/score.hpp"
#include "th02/main/scroll.hpp"
#include "th02/main/slowdown.hpp"
#include "th02/main/stage/stage.hpp"
#include "th02/main/player/player.hpp"
#include "th02/main/player/bomb.hpp"
#include "th02/main/player/shot.hpp"
// s1_actor.cpp already includes bullet.hpp before this tail. state.hpp is the
// guarded private ABI companion and intentionally relies on that vocabulary.
#include "th02/main/bullet/state.hpp"
#include "th02/main/replay.hpp"
#include "th02/main/laser.hpp"
#include "th02/main/enemy/enemy.hpp"
#include "th02/main/item/item.hpp"
#include "th02/main/item/shared.hpp"
#include "th02/main/spark.hpp"
#include "th02/main/pointnum/pointnum.hpp"
#include "th02/sprites/pointnum.h"
#include "th02/sprites/bombpart.h"
#include "th02/v_colors.hpp"
#include "th02/main/tile/tile.hpp"
#include "th02/main/stage/callback.hpp"
#include "th02/main/null.hpp"
#include "th02/formats/map.hpp"
#include "th02/main/checkpoint_apply.hpp"

extern "C" int spawn_row_cur;
extern "C" uint8_t bgm_show_timer;
extern "C" uint8_t bgm_title_id;
extern "C" uint8_t boss_bgm_title_id;
extern "C" void far enemies_invalidate(void);
extern "C" void far enemies_update_and_render(void);
extern "C" void far nullfunc_void_2(void);

enum t2checkpoint_callback_id_t {
	T2CCB_DISABLED = 0,
	T2CCB_LIVE = 1,
};

#define T2CAP_HEADER_TOTAL_SIZE 0x10
#define T2CAP_HEADER_SOURCE_FINGERPRINT 0x14
#define T2CAP_HEADER_SEMANTIC_DIGEST 0x18
#define T2CAP_HEADER_DECODED_SIZE 0x1C
#define T2CAP_HEADER_CONTAINER_CHECKSUM 0x20
#define T2CAP_HEADER_GROUP_MASK 0x24

#define T2CAP_GROUP_ID 0
#define T2CAP_GROUP_SCHEMA 1
#define T2CAP_GROUP_CODEC 2
#define T2CAP_GROUP_FLAGS 3
#define T2CAP_GROUP_OFFSET 4
#define T2CAP_GROUP_STORED_SIZE 8
#define T2CAP_GROUP_DECODED_SIZE 12
#define T2CAP_GROUP_CHECKSUM 16

static uint16_t near t2cap_u16(const uint8_t far *data, unsigned offset)
{
	return static_cast<uint16_t>(
		static_cast<uint16_t>(data[offset + 0]) |
		(static_cast<uint16_t>(data[offset + 1]) << 8)
	);
}

static uint32_t near t2cap_u32(const uint8_t far *data, unsigned offset)
{
	return (
		static_cast<uint32_t>(data[offset + 0]) |
		(static_cast<uint32_t>(data[offset + 1]) << 8) |
		(static_cast<uint32_t>(data[offset + 2]) << 16) |
		(static_cast<uint32_t>(data[offset + 3]) << 24)
	);
}

static unsigned near t2cap_group_size(uint8_t id)
{
	switch(id) {
	case T2RCGI_IDENTITY: return T2REPLAY_CHECKPOINT_IDENTITY_SIZE;
	case T2RCGI_RNG: return T2REPLAY_CHECKPOINT_RNG_SIZE;
	case T2RCGI_RUN: return T2REPLAY_CHECKPOINT_RUN_SIZE;
	case T2RCGI_FIELD: return T2REPLAY_CHECKPOINT_FIELD_SIZE;
	case T2RCGI_STAGE_VM: return T2REPLAY_CHECKPOINT_STAGE_VM_SIZE;
	case T2RCGI_PACING: return T2REPLAY_CHECKPOINT_PACING_SIZE;
	case T2RCGI_PLAYER: return T2REPLAY_CHECKPOINT_PLAYER_SIZE;
	case T2RCGI_BOMB: return T2REPLAY_CHECKPOINT_BOMB_SIZE;
	case T2RCGI_BULLET: return T2REPLAY_CHECKPOINT_BULLET_SIZE;
	case T2RCGI_LASER: return T2REPLAY_CHECKPOINT_LASER_SIZE;
	case T2RCGI_ENEMY: return T2REPLAY_CHECKPOINT_ENEMY_SIZE;
	case T2RCGI_EFFECT: return T2REPLAY_CHECKPOINT_EFFECT_SIZE;
	default: return 0;
	}
}

static unsigned near t2cap_payload_offset(uint8_t id)
{
	unsigned offset = (
		T2REPLAY_CHECKPOINT_HEADER_SIZE +
		(T2REPLAY_CHECKPOINT_GROUP_COUNT * T2REPLAY_CHECKPOINT_GROUP_SIZE)
	);
	uint8_t group_id;

	for(group_id = 0; group_id < id; group_id++) {
		offset += t2cap_group_size(group_id);
	}
	return offset;
}

static uint8_t near t2cap_take_u8(const uint8_t far *data, unsigned& offset)
{
	return data[offset++];
}

static uint16_t near t2cap_take_u16(const uint8_t far *data, unsigned& offset)
{
	uint16_t value = t2cap_u16(data, offset);
	offset += 2;
	return value;
}

static int16_t near t2cap_take_i16(const uint8_t far *data, unsigned& offset)
{
	return static_cast<int16_t>(t2cap_take_u16(data, offset));
}

static uint32_t near t2cap_take_u32(const uint8_t far *data, unsigned& offset)
{
	uint32_t value = t2cap_u32(data, offset);
	offset += 4;
	return value;
}

static void near t2cap_take_point(
	screen_point_t& point, const uint8_t far *data, unsigned& offset
)
{
	point.x = static_cast<screen_x_t>(t2cap_take_i16(data, offset));
	point.y = static_cast<screen_y_t>(t2cap_take_i16(data, offset));
}

static void near t2cap_take_spoint(
	SPPoint& point, const uint8_t far *data, unsigned& offset
)
{
	point.x.v = static_cast<subpixel_t>(t2cap_take_i16(data, offset));
	point.y.v = static_cast<subpixel_t>(t2cap_take_i16(data, offset));
}

static void near t2cap_apply_rng(const uint8_t far *data)
{
	unsigned i;

	random_seed = static_cast<long>(t2cap_u32(data, 0));
	for(i = 0; i < RANDRING_SIZE; i++) {
		randring[i] = data[4 + i];
	}
	randring_p = data[260];
}

static void near t2cap_apply_run(const uint8_t far *data)
{
	resident->frame = static_cast<long>(t2cap_u32(data, 0));
	resident->score = static_cast<score_t>(t2cap_u32(data, 4));
	resident->score_highest = t2cap_u32(data, 8);
	resident->continues_used = t2cap_u16(data, 12);
	resident->skill = static_cast<int>(static_cast<int16_t>(t2cap_u16(data, 14)));
	resident->rem_lives = static_cast<int8_t>(data[16]);
	resident->rem_bombs = static_cast<int8_t>(data[17]);
	resident->start_lives = data[18];
	resident->start_bombs = data[19];
	resident->start_power = static_cast<int8_t>(data[20]);
	resident->bgm_mode = data[21];
	resident->debug = static_cast<char>(data[22]);
	resident->op_main_retval = data[23];
	resident->demo_num = data[24];
	score = static_cast<score_t>(t2cap_u32(data, 25));
	hiscore = static_cast<score_t>(t2cap_u32(data, 29));
	hiscore_continues = data[33];
	score_delta = static_cast<score_t>(t2cap_u32(data, 34));
	score_delta_transferred_prev = t2cap_u16(data, 38);
	extends_gained = t2cap_u16(data, 40);
	lives = static_cast<int8_t>(data[42]);
	bombs = static_cast<int8_t>(data[43]);
	power = data[44];
	quit = (data[45] != 0);
}

static void near t2cap_apply_field(const uint8_t far *data)
{
	page_front = data[0];
	page_back = data[1];
	scroll_done = (data[2] != 0);
	tile_mode = static_cast<tile_mode_t>(data[3]);
	scroll_line = static_cast<vram_y_t>(t2cap_u16(data, 4));
	scroll_speed = static_cast<pixel_length_8_t>(static_cast<int8_t>(data[6]));
	scroll_cycle = data[7];
	scroll_interval = data[8];
	scroll_delta = static_cast<pixel_length_8_t>(static_cast<int8_t>(data[9]));
	scroll_step = static_cast<int>(static_cast<int16_t>(t2cap_u16(data, 10)));
	scroll_step_advanced = static_cast<bool16>(t2cap_u16(data, 12));
	replay_scroll_page_line_set(0, static_cast<int16_t>(t2cap_u16(data, 14)));
	replay_scroll_page_line_set(1, static_cast<int16_t>(t2cap_u16(data, 16)));
	tiles_egc_render_all = (data[18] != 0);
}

static void near t2cap_apply_stage_vm(const uint8_t far *data)
{
	stage_id = static_cast<char>(data[0]);
	stage_progression = static_cast<stage_progression_t>(data[1]);
	midboss_active = (data[2] != 0);
	spawn_row_cur = static_cast<int>(static_cast<int16_t>(t2cap_u16(data, 4)));
	midboss_scroll_step = static_cast<int>(static_cast<int16_t>(t2cap_u16(data, 6)));
}

static void near t2cap_apply_pacing(const uint8_t far *data)
{
	stage_frame = t2cap_u32(data, 0);
	slowdown_factor = data[4];
	playperf = static_cast<int>(static_cast<int16_t>(t2cap_u16(data, 5)));
	playperf_max = data[7];
	bgm_show_timer = data[8];
	bgm_title_id = data[9];
	boss_bgm_title_id = data[10];
}

static void near t2cap_player_page_aliases_rebind(void)
{
	player_left_on_back_page = &player_left_on_page[page_back];
	player_top_on_back_page = &player_top_on_page[page_back];
	player_topleft.x = *player_left_on_back_page;
	player_topleft.y = *player_top_on_back_page;
	player_option_left_left_on_back_page =
		&player_option_left_topleft[page_back].x;
	player_option_left_top_on_back_page =
		&player_option_left_topleft[page_back].y;
}

static void near t2cap_apply_player(const uint8_t far *data)
{
	unsigned offset = 0;
	unsigned i;
	shot_t near *shot;

	for(i = 0; i < PAGE_COUNT; i++) {
		player_left_on_page[i] = static_cast<screen_x_t>(t2cap_take_i16(data, offset));
		player_top_on_page[i] = static_cast<screen_y_t>(t2cap_take_i16(data, offset));
	}
	for(i = 0; i < PAGE_COUNT; i++) {
		t2cap_take_point(player_option_left_topleft[i], data, offset);
	}
	player_option_patnum = t2cap_take_u8(data, offset);
	playchar_speed_aligned_x = static_cast<int8_t>(t2cap_take_u8(data, offset));
	playchar_speed_aligned_y = static_cast<int8_t>(t2cap_take_u8(data, offset));
	playchar_speed_diagonal_x = static_cast<int8_t>(t2cap_take_u8(data, offset));
	playchar_speed_diagonal_y = static_cast<int8_t>(t2cap_take_u8(data, offset));
	*reinterpret_cast<uint8_t near *>(&player_is_hit) = t2cap_take_u8(data, offset);
	player_invincibility_time = t2cap_take_u8(data, offset);
	player_invincible_via_bomb = (t2cap_take_u8(data, offset) != 0);
	miss_frame = t2cap_take_u8(data, offset);
	miss_active = (t2cap_take_u8(data, offset) != 0);
	power = t2cap_take_u8(data, offset);
	power_overflow = static_cast<int>(t2cap_take_i16(data, offset));
	shot_level = t2cap_take_u8(data, offset);
	player_patnum = static_cast<int>(t2cap_take_u16(data, offset));
	shot_stream_a_phase = t2cap_take_u8(data, offset);
	shot_stream_b_phase = t2cap_take_u8(data, offset);
	shot_stream_a_cooldown_time = t2cap_take_u8(data, offset);
	shot_stream_b_cooldown_time = t2cap_take_u8(data, offset);
	shot_patnum = t2cap_take_u8(data, offset);
	shot_option_patnum = t2cap_take_u8(data, offset);
	shot_patnum_powered = t2cap_take_u8(data, offset);
	shot_option_patnum_powered = t2cap_take_u8(data, offset);
	shot_a_spread_angle_delta = static_cast<int8_t>(t2cap_take_u8(data, offset));
	option_shots_alive = t2cap_take_u8(data, offset);
	boss_pos_x = static_cast<int>(t2cap_take_i16(data, offset));
	boss_pos_y = static_cast<int>(t2cap_take_i16(data, offset));
	boss_pos_x_unused = static_cast<int>(t2cap_take_i16(data, offset));
	shot_c_cycle = t2cap_take_u8(data, offset);
	for(i = 0; i < SHOT_COUNT; i++) {
		shot_anim_frame[i] = t2cap_take_u8(data, offset);
	}
	shot_option_decay_interval = static_cast<int8_t>(t2cap_take_u8(data, offset));
	shot_slot_i = static_cast<int>(t2cap_take_i16(data, offset));
	shot_spawn_top = static_cast<subpixel_t>(t2cap_take_i16(data, offset));
	for(i = 0; i < SHOT_COUNT; i++) {
		shot = &shots[i];
		shot->flag = static_cast<entity_flag_t>(t2cap_take_u8(data, offset));
		if(shot->flag == F_FREE) {
			shot->decay_cel = 0;
			shot->pos_on_page[0].x.v = shot->pos_on_page[0].y.v = 0;
			shot->pos_on_page[1].x.v = shot->pos_on_page[1].y.v = 0;
			shot->velocity.x.v = shot->velocity.y.v = 0;
			shot->patnum = 0;
			shot->from_option = false;
			offset += 15;
			continue;
		}
		shot->decay_cel = t2cap_take_u8(data, offset);
		t2cap_take_spoint(shot->pos_on_page[0], data, offset);
		t2cap_take_spoint(shot->pos_on_page[1], data, offset);
		t2cap_take_spoint(shot->velocity, data, offset);
		shot->patnum = t2cap_take_u8(data, offset);
		shot->from_option = (t2cap_take_u8(data, offset) != 0);
	}
	t2cap_player_page_aliases_rebind();
}

static void near t2cap_apply_bomb(const uint8_t far *data)
{
	unsigned offset = 1;
	unsigned i;

	bombing = (data[0] != 0);
	if(bombing) {
		bomb_frame = static_cast<int>(t2cap_take_i16(data, offset));
		bomb_circle_center.x = static_cast<int>(t2cap_take_i16(data, offset));
		bomb_circle_center.y = static_cast<int>(t2cap_take_i16(data, offset));
		bomb_circle_frame = static_cast<int>(t2cap_take_i16(data, offset));
		bomb_circle_done = static_cast<bool16>(t2cap_take_u16(data, offset));
		for(i = 0; i < BOMB_PARTICLE_COUNT; i++) {
			bomb_particle_pos[i].x = static_cast<int>(t2cap_take_i16(data, offset));
			bomb_particle_pos[i].y = static_cast<int>(t2cap_take_i16(data, offset));
		}
		for(i = 0; i < BOMB_PARTICLE_COUNT; i++) {
			bomb_particle_cel[i] = t2cap_take_u8(data, offset);
		}
		for(i = 0; i < BOMB_SMEAR_COLUMNS; i++) {
			bomb_smears[i].bottom = static_cast<screen_y_t>(t2cap_take_i16(data, offset));
			bomb_smears[i].unused = 0;
		}
		tile_mode_before_bomb_a = static_cast<tile_mode_t>(t2cap_take_u8(data, offset));
		tile_mode_before_bomb_b = static_cast<tile_mode_t>(t2cap_take_u8(data, offset));
		tile_mode_before_bomb_c = static_cast<tile_mode_t>(t2cap_take_u8(data, offset));
		for(i = 0; i < COMPONENT_COUNT; i++) {
			col0_before_bomb_a.v[i] = t2cap_take_u8(data, offset);
		}
		for(i = 0; i < COMPONENT_COUNT; i++) {
			col3_before_bomb_a.v[i] = t2cap_take_u8(data, offset);
		}
		bomb_b_cel = t2cap_take_u8(data, offset);
	} else {
		bomb_frame = 0;
		bomb_circle_center.x = 0;
		bomb_circle_center.y = 0;
		bomb_circle_frame = 0;
		bomb_circle_done = false;
		for(i = 0; i < BOMB_PARTICLE_COUNT; i++) {
			bomb_particle_pos[i].x = 0;
			bomb_particle_pos[i].y = 0;
			bomb_particle_cel[i] = 0;
		}
		for(i = 0; i < BOMB_SMEAR_COLUMNS; i++) {
			bomb_smears[i].bottom = 0;
			bomb_smears[i].unused = 0;
		}
		tile_mode_before_bomb_a = TM_COL_0;
		tile_mode_before_bomb_b = TM_COL_0;
		tile_mode_before_bomb_c = TM_COL_0;
		for(i = 0; i < COMPONENT_COUNT; i++) {
			col0_before_bomb_a.v[i] = 0;
			col3_before_bomb_a.v[i] = 0;
		}
		bomb_b_cel = 0;
		offset = T2REPLAY_CHECKPOINT_BOMB_SIZE - 4;
	}
	stage_miss_count = t2cap_take_u8(data, offset);
	stage_bombs_used = t2cap_take_u8(data, offset);
	total_miss_count = t2cap_take_u8(data, offset);
	total_bombs_used = t2cap_take_u8(data, offset);
}

static void near t2cap_apply_bullet(const uint8_t far *data)
{
	unsigned offset = 0;
	unsigned i;
	bullet_t near *bullet;

	bullet_special.u1.chase_speed.v = static_cast<subpixel_t>(
		t2cap_take_i16(data, offset)
	);
	bullet_special.u2.homing_frames = t2cap_take_i16(data, offset);
	bullet_special.u3.turns_max = t2cap_take_i16(data, offset);
	rank_base_speed.v = static_cast<int8_t>(t2cap_take_u8(data, offset));
	rank_base_stack = t2cap_take_u8(data, offset);
	bullet_stack = t2cap_take_u8(data, offset);
	easy_slow_skip_cycle = static_cast<int8_t>(t2cap_take_u8(data, offset));
	for(i = 0; i < BULLET_COUNT; i++) {
		bullet = &bullets[i];
		bullet->flag = static_cast<int8_t>(t2cap_take_u8(data, offset));
		if(bullet->flag == F_FREE) {
			bullet->size_type = 0;
			bullet->screen_topleft[0].x.v = bullet->screen_topleft[0].y.v = 0;
			bullet->screen_topleft[1].x.v = bullet->screen_topleft[1].y.v = 0;
			bullet->velocity.x.v = bullet->velocity.y.v = 0;
			bullet->patnum = 0;
			bullet->group_or_special_motion = BG_NONE;
			bullet->angle = 0;
			bullet->speed.v = 0;
			bullet->u1.v = 0;
			bullet->padding = 0;
			offset += 18;
			continue;
		}
		bullet->size_type = static_cast<int8_t>(t2cap_take_u8(data, offset));
		t2cap_take_spoint(bullet->screen_topleft[0], data, offset);
		t2cap_take_spoint(bullet->screen_topleft[1], data, offset);
		t2cap_take_spoint(bullet->velocity, data, offset);
		bullet->patnum = t2cap_take_u8(data, offset);
		bullet->group_or_special_motion =
			static_cast<bullet_group_or_special_motion_t>(t2cap_take_u8(data, offset));
		bullet->angle = t2cap_take_u8(data, offset);
		bullet->speed.v = t2cap_take_u8(data, offset);
		bullet->u1.v = t2cap_take_u8(data, offset);
		bullet->padding = 0;
	}
}

static void near t2cap_apply_laser(const uint8_t far *data)
{
	unsigned offset = 0;
	unsigned i;
	laser_t near *laser;

	if(data[offset] == T2CCB_DISABLED) {
		lasers_invalidate_func = nullfunc_void;
		lasers_update_and_render_func = nullfunc_void;
	} else {
		lasers_invalidate_func = lasers_invalidate;
		lasers_update_and_render_func = lasers_update_and_render;
	}
	offset += 2;
	laser_wait_frames = t2cap_take_u8(data, offset);
	for(i = 0; i < LASER_COUNT; i++) {
		laser = &lasers[i];
		laser->flag = static_cast<entity_flag_t>(t2cap_take_u8(data, offset));
		if(laser->flag == F_FREE) {
			laser->phase = 0;
			laser->origin.x = 0;
			laser->origin.y = 0;
			laser->wait_frames = 0;
			laser->active_frames = 0;
			laser->charge_cel = 0;
			laser->patnum_base = 0;
			offset += 11;
			continue;
		}
		laser->phase = t2cap_take_u8(data, offset);
		t2cap_take_point(laser->origin, data, offset);
		laser->wait_frames = static_cast<int>(t2cap_take_i16(data, offset));
		laser->active_frames = static_cast<int>(t2cap_take_i16(data, offset));
		laser->charge_cel = t2cap_take_u8(data, offset);
		laser->patnum_base = t2cap_take_u8(data, offset);
	}
}

static void near t2cap_enemy_record_clear(enemy_t near *enemy)
{
	enemy->pos_on_page[0].x = enemy->pos_on_page[0].y = 0;
	enemy->pos_on_page[1].x = enemy->pos_on_page[1].y = 0;
	enemy->script_ip = 0;
	enemy->age = 0;
	enemy->template_id = 0;
	enemy->flag = F_FREE;
	enemy->anim_frame = 0;
	enemy->in_kill_anim = false;
	enemy->unused_1 = 0;
	enemy->patnum_delta = 0;
	enemy->render_as = 0;
	enemy->angle = 0;
	enemy->spawned_in_left_half = false;
	enemy->loop_i = 0;
	enemy->velocity_x = 0;
	enemy->velocity_y = 0;
	enemy->despawn_when_offscreen_vertically = false;
	enemy->unused_2 = 0;
	enemy->damage = 0;
	enemy->not_shootable = false;
	enemy->no_player_collision = false;
	enemy->pellet_group = BG_NONE;
	enemy->pellet_speed = 0;
}

static void near t2cap_apply_enemy(const uint8_t far *data)
{
	unsigned offset = 0;
	unsigned i;
	enemy_t near *enemy;

	enemy_scripts_used = t2cap_take_u8(data, offset);
	enemies_loop_bound = t2cap_take_u8(data, offset);
	if(data[offset] == T2CCB_DISABLED) {
		enemies_invalidate_func = nullfunc_void_2;
		enemies_update_and_render_func = nullfunc_void_2;
	} else {
		enemies_invalidate_func = enemies_invalidate;
		enemies_update_and_render_func = enemies_update_and_render;
	}
	offset += 2;
	for(i = 0; i < ENEMY_COUNT; i++) {
		enemy = &enemies[i];
		if(data[offset + 14] == F_FREE) {
			t2cap_enemy_record_clear(enemy);
			offset += 36;
			continue;
		}
		t2cap_take_point(enemy->pos_on_page[0], data, offset);
		t2cap_take_point(enemy->pos_on_page[1], data, offset);
		enemy->script_ip = static_cast<int>(t2cap_take_i16(data, offset));
		enemy->age = static_cast<int>(t2cap_take_i16(data, offset));
		enemy->template_id = static_cast<int>(t2cap_take_i16(data, offset));
		enemy->flag = static_cast<entity_flag_t>(t2cap_take_u8(data, offset));
		enemy->anim_frame = t2cap_take_u8(data, offset);
		enemy->in_kill_anim = (t2cap_take_u8(data, offset) != 0);
		enemy->unused_1 = 0;
		enemy->patnum_delta = static_cast<int>(t2cap_take_i16(data, offset));
		enemy->render_as = static_cast<int>(t2cap_take_i16(data, offset));
		enemy->angle = static_cast<int>(t2cap_take_i16(data, offset));
		enemy->spawned_in_left_half =
			static_cast<bool16>(t2cap_take_u16(data, offset));
		enemy->loop_i = static_cast<int>(t2cap_take_i16(data, offset));
		enemy->velocity_x = static_cast<pixel_delta_8_t>(
			static_cast<int8_t>(t2cap_take_u8(data, offset))
		);
		enemy->velocity_y = static_cast<pixel_delta_8_t>(
			static_cast<int8_t>(t2cap_take_u8(data, offset))
		);
		enemy->despawn_when_offscreen_vertically = (t2cap_take_u8(data, offset) != 0);
		enemy->unused_2 = 0;
		enemy->damage = static_cast<int>(t2cap_take_i16(data, offset));
		enemy->not_shootable = (t2cap_take_u8(data, offset) != 0);
		enemy->no_player_collision = (t2cap_take_u8(data, offset) != 0);
		enemy->pellet_group = t2cap_take_u8(data, offset);
		enemy->pellet_speed = t2cap_take_u8(data, offset);
	}
}

static void near t2cap_item_record_clear(item_t near *item)
{
	unsigned page;

	item->flag = F_FREE;
	item->type = IT_POWER;
	for(page = 0; page < PAGE_COUNT; page++) {
		item->pos[page].screen_left = 0;
		item->pos[page].screen_top.v = 0;
	}
	item->velocity_y.v = 0;
	item->velocity_x_during_bounce = 0;
	item->age = 0;
}

static void near t2cap_spark_record_clear(spark_t near *spark)
{
	spark->flag = F_FREE;
	spark->age = 0;
	spark->screen_topleft[0].x.v = spark->screen_topleft[0].y.v = 0;
	spark->screen_topleft[1].x.v = spark->screen_topleft[1].y.v = 0;
	spark->velocity.x.v = spark->velocity.y.v = 0;
	spark->render_as = SRA_DOT;
	spark->unused_1 = 0;
	spark->angle = 0;
	spark->speed_base.v = 0;
	spark->unused_2 = 0;
	spark->default_render_as = SRA_DOT;
}

static void near t2cap_apply_effect(const uint8_t far *data)
{
	unsigned offset = 0;
	unsigned i;
	unsigned page;
	item_t near *item;
	spark_t near *spark;

	item_bigpower_override = t2cap_take_u16(data, offset);
	items_miss_add_gameover = (t2cap_take_u8(data, offset) != 0);
	item_semirandom_ring_p = t2cap_take_u8(data, offset);
	item_semirandom_cycle = t2cap_take_u8(data, offset);
	item_drop_cycle = t2cap_take_u8(data, offset);
	item_collect_skill = t2cap_take_u8(data, offset);
	item_score_this_frame = static_cast<score_t>(t2cap_take_u32(data, offset));
	item_skill = static_cast<int>(t2cap_take_i16(data, offset));
	point_items_collected = static_cast<int>(t2cap_take_i16(data, offset));
	for(i = 0; i < ITEM_COUNT; i++) {
		item = &items[i];
		if(data[offset] == F_FREE) {
			t2cap_item_record_clear(item);
			offset += 16;
			continue;
		}
		item->flag = static_cast<entity_flag_t>(t2cap_take_u8(data, offset));
		item->type = static_cast<item_type_t>(t2cap_take_u8(data, offset));
		for(page = 0; page < PAGE_COUNT; page++) {
			item->pos[page].screen_left =
				static_cast<screen_x_t>(t2cap_take_i16(data, offset));
			item->pos[page].screen_top.v =
				static_cast<subpixel_t>(t2cap_take_i16(data, offset));
		}
		item->velocity_y.v = static_cast<subpixel_t>(t2cap_take_i16(data, offset));
		item->velocity_x_during_bounce =
			static_cast<pixel_t>(t2cap_take_i16(data, offset));
		item->age = static_cast<int>(t2cap_take_i16(data, offset));
	}
	for(i = 0; i < SPARK_COUNT; i++) {
		spark = &sparks[i];
		if(data[offset] == F_FREE) {
			t2cap_spark_record_clear(spark);
			offset += 18;
			continue;
		}
		spark->flag = static_cast<entity_flag_t>(t2cap_take_u8(data, offset));
		spark->age = t2cap_take_u8(data, offset);
		t2cap_take_spoint(spark->screen_topleft[0], data, offset);
		t2cap_take_spoint(spark->screen_topleft[1], data, offset);
		t2cap_take_spoint(spark->velocity, data, offset);
		spark->render_as = static_cast<spark_render_as_t>(t2cap_take_u8(data, offset));
		spark->angle = t2cap_take_u8(data, offset);
		spark->speed_base.v = t2cap_take_u8(data, offset);
		spark->unused_1 = 0;
		spark->unused_2 = 0;
		spark->default_render_as =
			static_cast<spark_render_as_t>(t2cap_take_u8(data, offset));
	}
	spark_ring_i = t2cap_take_u16(data, offset);
	spark_sprite_interval = t2cap_take_u8(data, offset);
	spark_age_max = t2cap_take_u8(data, offset);
	spark_accel_x.v = static_cast<subpixel_t>(t2cap_take_i16(data, offset));
	pointnums.col = static_cast<vc_t>(t2cap_take_u8(data, offset));
	pointnums.unused = 0;
	for(i = 0; i < POINTNUM_COUNT; i++) {
		if(data[offset + 8] == F_FREE) {
			pointnums.left[i] = 0;
			pointnums.top[i][0] = 0;
			pointnums.top[i][1] = 0;
			pointnums.points[i] = 0;
			pointnums.flag[i] = F_FREE;
			pointnums.age[i] = 0;
			offset += 10;
			continue;
		}
		pointnums.left[i] = static_cast<screen_x_t>(t2cap_take_i16(data, offset));
		pointnums.top[i][0] = static_cast<screen_y_t>(t2cap_take_i16(data, offset));
		pointnums.top[i][1] = static_cast<screen_y_t>(t2cap_take_i16(data, offset));
		pointnums.points[i] = t2cap_take_u16(data, offset);
		pointnums.flag[i] = static_cast<entity_flag_t>(t2cap_take_u8(data, offset));
		pointnums.age[i] = t2cap_take_u8(data, offset);
	}
	pointnums.op = static_cast<int8_t>(t2cap_take_u8(data, offset));
	pointnums.operand = static_cast<int8_t>(t2cap_take_u8(data, offset));
}

static void near t2cap_reject_set(
	enum t2checkpoint_common_reject_t *reason,
	enum t2checkpoint_common_reject_t value
)
{
	if(reason != NULL) {
		*reason = value;
	}
}

static bool16 near t2cap_boundary_valid(
	const struct t2checkpoint_common_boundary_t *boundary
)
{
	return (
		(boundary != NULL) &&
		(boundary->at_ordinary_stage_loop_top != 0) &&
		(boundary->stage_init_complete != 0) &&
		(boundary->input_sampled == 0) &&
		(boundary->pause_or_debounce_active == 0) &&
		(boundary->blocking_presentation_active == 0) &&
		(boundary->restore_or_redraw_active == 0)
	);
}

static bool16 near t2cap_environment_valid(
	const uint8_t far *identity, const uint8_t far *stage_vm
)
{
	return (
		(resident != NULL) &&
		(identity[0] == stage_vm[0]) &&
		(static_cast<uint8_t>(stage_id) == identity[0]) &&
		(resident->shottype == identity[1]) &&
		(static_cast<uint8_t>(rank) == identity[2]) &&
		((reduce_effects ? 1 : 0) == identity[3])
	);
}

static bool16 near t2cap_plan_layout_valid(
	const struct t2checkpoint_common_plan_t *plan
)
{
	uint8_t id;
	const uint8_t far *expected;

	if(
		(plan == NULL) ||
		!replay_checkpoint_schema4_valid(
			plan->container, plan->container_size
		) ||
		(plan->semantic_digest !=
			t2cap_u32(plan->container, T2CAP_HEADER_SEMANTIC_DIGEST))
	) {
		return false;
	}
	for(id = 0; id < T2REPLAY_CHECKPOINT_GROUP_COUNT; id++) {
		expected = plan->container + t2cap_payload_offset(id);
		if(plan->group[id] != expected) {
			return false;
		}
	}
	return (
		t2cap_boundary_valid(&plan->boundary) &&
		(plan->page_front == plan->group[T2RCGI_FIELD][0]) &&
		(plan->page_back == plan->group[T2RCGI_FIELD][1]) &&
		(plan->laser_callback_id == plan->group[T2RCGI_LASER][0]) &&
		(plan->enemy_callback_id == plan->group[T2RCGI_ENEMY][2]) &&
		(plan->resource_id == plan->group[T2RCGI_IDENTITY][0]) &&
		(plan->palette_required == 1) && (plan->redraw_required == 1)
	);
}

bool16 far t2checkpoint_common_plan_prepare(
	struct t2checkpoint_common_plan_t *plan,
	const uint8_t far *container,
	uint32_t container_size,
	const struct t2checkpoint_common_boundary_t *boundary,
	enum t2checkpoint_common_reject_t *reason
)
{
	struct t2checkpoint_common_plan_t staged;
	uint8_t id;

	t2cap_reject_set(reason, T2CCAR_OK);
	if(plan == NULL) {
		t2cap_reject_set(reason, T2CCAR_NULL);
		return false;
	}
	if(container == NULL) {
		t2cap_reject_set(reason, T2CCAR_NULL);
		return false;
	}
	if(container_size != T2REPLAY_CHECKPOINT_CAPTURE_SIZE) {
		t2cap_reject_set(reason, T2CCAR_SIZE);
		return false;
	}
	if(!replay_checkpoint_schema4_valid(container, container_size)) {
		t2cap_reject_set(reason, T2CCAR_CONTAINER);
		return false;
	}
	if(!t2cap_boundary_valid(boundary)) {
		t2cap_reject_set(reason, T2CCAR_BOUNDARY);
		return false;
	}
	if(!t2cap_environment_valid(
		container + t2cap_payload_offset(T2RCGI_IDENTITY),
		container + t2cap_payload_offset(T2RCGI_STAGE_VM)
	)) {
		t2cap_reject_set(reason, T2CCAR_ENVIRONMENT);
		return false;
	}

	staged.container = container;
	staged.container_size = container_size;
	staged.semantic_digest = t2cap_u32(container, T2CAP_HEADER_SEMANTIC_DIGEST);
	staged.boundary = *boundary;
	for(id = 0; id < T2REPLAY_CHECKPOINT_GROUP_COUNT; id++) {
		staged.group[id] = container + t2cap_payload_offset(id);
	}
	staged.page_front = staged.group[T2RCGI_FIELD][0];
	staged.page_back = staged.group[T2RCGI_FIELD][1];
	staged.laser_callback_id = staged.group[T2RCGI_LASER][0];
	staged.enemy_callback_id = staged.group[T2RCGI_ENEMY][2];
	staged.resource_id = staged.group[T2RCGI_IDENTITY][0];
	staged.palette_required = 1;
	staged.redraw_required = 1;
	*plan = staged;
	return true;
}

bool16 far t2checkpoint_common_apply_prepared(
	const struct t2checkpoint_common_plan_t *plan
)
{
	if(
		!t2cap_plan_layout_valid(plan) ||
		!t2cap_environment_valid(
			plan->group[T2RCGI_IDENTITY], plan->group[T2RCGI_STAGE_VM]
		)
	) {
		return false;
	}

	// The caller has already checked every actor/palette/tile/callback/redraw
	// group needed to make this visible. This common-only commit never calls
	// graphics, allocators, DOS, or stage-specific code.
	t2cap_apply_rng(plan->group[T2RCGI_RNG]);
	t2cap_apply_run(plan->group[T2RCGI_RUN]);
	t2cap_apply_field(plan->group[T2RCGI_FIELD]);
	t2cap_apply_stage_vm(plan->group[T2RCGI_STAGE_VM]);
	t2cap_apply_pacing(plan->group[T2RCGI_PACING]);
	t2cap_apply_player(plan->group[T2RCGI_PLAYER]);
	t2cap_apply_bomb(plan->group[T2RCGI_BOMB]);
	t2cap_apply_bullet(plan->group[T2RCGI_BULLET]);
	t2cap_apply_laser(plan->group[T2RCGI_LASER]);
	t2cap_apply_enemy(plan->group[T2RCGI_ENEMY]);
	t2cap_apply_effect(plan->group[T2RCGI_EFFECT]);
	return true;
}

bool16 far t2checkpoint_common_apply(
	const uint8_t far *container,
	uint32_t container_size,
	const struct t2checkpoint_common_boundary_t *boundary,
	enum t2checkpoint_common_reject_t *reason
)
{
	struct t2checkpoint_common_plan_t plan;

	// Keep preparation immediately adjacent to the first possible mutation.
	// This is intentionally a fresh validation rather than a cached plan.
	if(!t2checkpoint_common_plan_prepare(
		&plan, container, container_size, boundary, reason
	)) {
		return false;
	}
	if(!t2checkpoint_common_apply_prepared(&plan)) {
		t2cap_reject_set(reason, T2CCAR_CONTAINER);
		return false;
	}
	t2cap_reject_set(reason, T2CCAR_OK);
	return true;
}
