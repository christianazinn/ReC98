#pragma option -zCREPLAY_CK_TEXT

// Portable checkpoint field codecs shared by TH04 and TH05. No native
// structure image, pointer, segment, padding byte, or renderer list enters the
// stream. Decode is deliberately split into validate and apply passes so a
// malformed group cannot partially mutate live gameplay state.

#include "platform.h"

#if (GAME == 5)
	// Turbo C++ frames near code addresses from the first declaration seen in
	// a translation unit. These callbacks live outside REPLAY_CK_TEXT, so their
	// real segments must be declared before the plain shared-header declarations.
	#pragma codeseg PLAYFLD_TEXT
	extern "C" void pascal near cheetos_render(void);
	#pragma codeseg

	#pragma codeseg MIDBOSSX_TEXT
	void pascal near midboss1_render(void);
	void pascal near midboss2_render(void);
	void pascal near midboss3_render(void);
	void pascal near midboss4_render(void);
	void pascal near midbossx_render(void);
	void pascal near sara_fg_render(void);
	void pascal near louise_fg_render(void);
	void pascal near alice_fg_render(void);
	void pascal near mai_yuki_fg_render(void);
	void pascal near yumeko_fg_render(void);
	void pascal near shinki_fg_render(void);
	extern "C" void pascal near b4balls_render(void);
	extern "C" void pascal near b4_solo_fg_render(void);
	extern "C" void pascal near swords_render(void);
	void pascal near shinki_custombullets_render(void);
	#pragma codeseg

	#pragma codeseg main_0_TEXT
	void pascal near midboss5_render(void);
	void pascal near exalice_fg_render(void);
	extern "C" void pascal near exalice_custombullets_render(void);
	#pragma codeseg

	#pragma codeseg PLAYER_B_TEXT
	extern "C" void pascal near nullfunc_near(void);
	#pragma codeseg

	#pragma codeseg BOSS_BG_TEXT main_01
	void pascal near sara_backdrop_colorfill(void);
	#pragma codeseg

	#pragma codeseg END_EXT_A_TEXT main_01
	void pascal near louise_backdrop_colorfill(void);
	void pascal near alice_backdrop_colorfill(void);
	void pascal near mai_yuki_backdrop_colorfill(void);
	#pragma codeseg

	#pragma codeseg YUMEKO_COLORFILL_TEXT main_01
	void pascal near yumeko_backdrop_colorfill(void);
	#pragma codeseg

	#pragma codeseg LASER_RH_TEXT main_01
	void pascal near shinki_stage_backdrop_colorfill(void);
	#pragma codeseg
#endif

#include "x86real.h"
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/hardware/pages.hpp"
#include "th02/main/scroll.hpp"
#include "th04/common.h"
#include "th04/formats/std.hpp"
#include "th04/main/bg.hpp"
#include "th04/main/bullet/bullet.hpp"
#include "th04/main/bullet/clearzap.hpp"
#include "th04/main/boss/explode.hpp"
#include "th04/main/circle.hpp"
#include "th04/main/custom.hpp"
#include "th04/main/dialog/dialog.hpp"
#include "th04/main/drawp.hpp"
#include "th04/main/frames.h"
#include "th04/main/gather.hpp"
#include "th04/main/item/splash.hpp"
#include "th04/main/midboss/midboss.hpp"
#include "th04/main/null.hpp"
#include "th04/main/pattern.hpp"
#include "th04/main/player/move.hpp"
#include "th04/main/player/shot.hpp"
#include "th04/main/pointnum/pointnum.hpp"
#include "th04/main/quit.hpp"
#include "th04/main/replay_checkpoint.hpp"
#include "th04/main/replay.hpp"
#include "th04/main/playperf.hpp"
#include "th04/main/playfld.hpp"
#include "th04/main/player/player.hpp"
#include "th04/main/score.hpp"
#include "th04/main/scroll.hpp"
#include "th04/main/slowdown.hpp"
#include "th04/main/spark.hpp"
#include "th04/main/stage/stage.hpp"
#include "th04/main/tile/tile.hpp"
#include "th04/replay_format.hpp"
#include "th04/replay_targets.hpp"
#include "th04/score.h"
#include "th04/snd/snd.h"

struct map_section_tiles_t;
extern map_section_tiles_t __seg* map_seg;
#if (GAME == 5)
	#include "th03/hardware/palette.hpp"
	#include "th05/main/boss/bosses.hpp"
	#include "th05/main/boss/b3puppet.hpp"
	#include "th04/main/boss/backdrop.hpp"
	#include "th05/main/bullet/b4ball.hpp"
	#include "th05/main/bullet/cheeto.hpp"
	#include "th05/main/bullet/laser.hpp"
	#include "th05/main/bullet/sword.hpp"
	#include "th05/main/enemy/enemy.hpp"
	#include "th05/main/player/bomb.hpp"
	#include "th05/main/player/bombanim.hpp"
	#include "th05/main/stage/stages.hpp"
	#include "th05/playchar.h"
	#include "th05/resident.hpp"
	#include "th05/formats/dialog.hpp"
#else
	#include "th04/formats/bb.h"
	#include "th04/formats/cdg.h"
	#include "th04/main/boss/boss.hpp"
	#include "th04/main/boss/backdrop.hpp"
	#include "th04/main/boss/b4m.hpp"
	#include "th04/main/boss/b4r.hpp"
	#include "th04/main/boss/bosses.hpp"
	#include "th04/main/bullet/laser_t.hpp"
	#include "th04/main/enemy/enemy.hpp"
	#include "th04/main/player/bomb.hpp"
	#include "th04/main/stage/stages.hpp"
	#include "th04/playchar.h"
	#include "th04/resident.hpp"
	#include "th04/sprites/main_cdg.h"
	#include "th04/sprites/main_pat.h"
#endif

#define RCK_SHOT_LEVEL_COUNT 10
#define RCK_BOMB_STAR_COUNT 48
#define RCK_SHAKE_ANIM_TIME_MAX 16
#define RCK_SPARK_SIZE 16
#define RCK_STD_OFFSET_NONE 0xFFFFu

// Far boss callbacks overwrite both long-lived automatic storage and the
// replay module's diagnostic scratch. Keep the constructor's immutable target
// contract in this separately linked, zero-initialized patch BSS.
static uint32_t rck_practice_boss_resident_rand;
static int32_t rck_practice_boss_random_seed;
static uint8_t rck_practice_boss_target_section;
static uint8_t rck_practice_boss_target_phase;

extern uint16_t randring_p;
extern uint8_t stage_id;
extern uint8_t rank;
extern uint8_t playperf;
extern uint8_t extends_gained;
extern uint32_t score_delta;
extern uint32_t score_delta_frame;
extern uint16_t stage_graze;
extern uint8_t continues_used;
extern uint8_t hiscore_popup_shown;
extern uint8_t power;
extern unsigned int total_max_valued_point_items;
extern unsigned char item_splash_last_id;
extern uint8_t midboss_defeat_angle;
extern bool (near* std_update)(void);
bool near std_update_done(void);
extern int8_t playfield_shake_redraw_time;

extern nearfunc_t_near overlay1;

extern bool player_is_hit;
extern uint8_t player_invincibility_time;
extern uint8_t miss_time;
extern "C" uint8_t miss_move_lock_time;
extern uint8_t shot_level;
extern nearfunc_t_near near *playchar_shot_funcs;
extern nearfunc_t_near playchar_shot_func;
extern SPPoint homing_target;

#if (GAME == 5)
	#define RCK_BOSS_PARTICLE_COUNT 64
	#define RCK_LINESET_LINE_COUNT 20
	#define RCK_LINESET_COUNT 4

	struct rck_boss_particle_t {
		PlayfieldPoint pos;
		PlayfieldPoint origin;
		SPPoint velocity;
		int age;
		uint8_t angle;
		uint8_t patnum;
	};

	struct rck_lineset_t {
		SPPoint center[RCK_LINESET_LINE_COUNT];
		Subpixel velocity_y;
		Subpixel radius[RCK_LINESET_LINE_COUNT];
		uint8_t angle[RCK_LINESET_LINE_COUNT];
	};

	extern rck_boss_particle_t boss_particles[RCK_BOSS_PARTICLE_COUNT];
	extern rck_lineset_t linesets[RCK_LINESET_COUNT];
	extern uint8_t shinki_bg_linesets_zoomed_out;
	extern int shinki_bg_type_a_particles_alive;
	extern bool shinki_bg_type_b_initialized;
	extern uint16_t shinki_bg_spinline_frame;
	extern bool shinki_bg_type_c_initialized;
	extern bool shinki_bg_type_d_initialized;

	struct rck_dialog_cursor_t {
		int16_t x;
		int16_t y;
	};
	extern rck_dialog_cursor_t dialog_cursor;
	extern int dialog_side;
	extern int8_t dialog_sequence_id;

	extern uint8_t byte_22274;
	extern uint8_t byte_22275;
	extern uint8_t lives;
	extern uint8_t bombs;
	extern uint8_t dream;
	extern "C" input_t word_2CE9E;
	extern "C" uint8_t shot_hit_spark_parity;
	extern uint16_t hitshot_next_free_id;
	extern "C" nearfunc_t_near SHOT_FUNCS[PLAYCHAR_COUNT][10];

	typedef bool (pascal near *near rck_puppet_func_t)(puppet_t near *puppet);

	struct near rck_firewave_t {
		bool alive;
		bool is_right;
		vram_y_t bottom;
		pixel_t amp;
	};

	extern pattern_loop_func_t sara_phase_2_3_pattern;
	extern const pattern_loop_func_t SARA_PATTERNS_PHASE_2_3[2][4];
	extern "C" SPPoint midboss2_center;
	extern "C" rck_puppet_func_t fp_2CE2A;
	extern "C" rck_puppet_func_t fp_2CE2C;
	extern "C" unsigned int alice_barrier_frame;
	extern "C" unsigned int alice_barrier_fire_frames;
	extern "C" pattern_loop_func_t fp_2CE32;
	extern "C" const rck_puppet_func_t ALICE_PUPPET_PATTERNS[4];
	extern "C" const pattern_loop_func_t off_22770[4][3];
	#pragma codeseg B4_UPDATE_TEXT main_03
	bool pascal near alice_puppet_pattern_19A84(puppet_t near *puppet);
	bool pascal near alice_puppet_pattern_19AE3(puppet_t near *puppet);
	bool pascal near alice_puppet_pattern_19AFB(puppet_t near *puppet);
	#pragma codeseg

	extern subpixel_t midboss4_warp_x;
	extern y_direction_t mai_flystep_random_next_y_direction;
	extern y_direction_t yuki_flystep_random_next_y_direction;
	extern "C" pattern_oneshot_func_t mai_pair_pattern;
	extern "C" pattern_oneshot_func_t yuki_pair_pattern;
	extern "C" pattern_oneshot_func_t mai_yuki_pattern;
	extern "C" const pattern_oneshot_func_t MAI_PAIR_PATTERNS_1[4];
	extern "C" const pattern_oneshot_func_t MAI_PAIR_PATTERNS_3[4];
	extern "C" const pattern_oneshot_func_t YUKI_PAIR_PATTERNS_1[4];
	extern "C" const pattern_oneshot_func_t YUKI_PAIR_PATTERNS_2[4];
	extern "C" const pattern_oneshot_func_t YUKI_PAIR_PATTERNS_3[4];
	extern "C" const pattern_oneshot_func_t MAI_PATTERNS_PHASE_3[2];
	extern "C" const pattern_oneshot_func_t MAI_PATTERNS_PHASE_7[2];
	extern "C" const pattern_oneshot_func_t MAI_PATTERNS_PHASE_9[2];
	extern "C" const pattern_oneshot_func_t YUKI_PATTERNS_PHASE_3[2];
	extern "C" const pattern_oneshot_func_t YUKI_PATTERNS_PHASE_5[2];
	extern "C" const pattern_oneshot_func_t YUKI_PATTERNS_PHASE_9[5];
	extern "C" const pattern_loop_func_t MAI_LASER_BULLET_PATTERNS[3];
	extern "C" int mai_laser_count;
	extern "C" int mai_laser_angle_speed;
	extern "C" int mai_laser_angle_progress;
	extern "C" pattern_loop_func_t mai_laser_bullet_pattern;
	#pragma codeseg main_035_TEXT main_03
	bool near mai_yuki_1A775(void);
	#pragma codeseg
	void pascal mai_update(void);
	extern "C" void pascal yuki_update(void);

	extern pattern_oneshot_func_t midboss5_phase_1_pattern;
	extern "C" const pattern_oneshot_func_t MIDBOSS5_PATTERNS_PHASE_1[3];
	extern "C" pattern_loop_func_t yumeko_pattern;
	extern "C" const pattern_loop_func_t YUMEKO_PATTERNS_PHASE_2[2];
	extern "C" const pattern_loop_func_t YUMEKO_PATTERNS_PHASE_5[2];
	#pragma codeseg main_035_TEXT main_03
	void near yumeko_1CB71(void);
	void near yumeko_1CED9(void);
	#pragma codeseg

	extern pattern_oneshot_func_t shinki_phase_2_3_pattern;
	extern pattern_loop_func_t shinki_wing_pattern;
	extern "C" const pattern_oneshot_func_t SHINKI_PATTERNS_PHASE_2_3[4];
	extern unsigned int shinki_devil_laser_grow_delay;
	extern unsigned char shinki_float_direction;
	#pragma codeseg B6_UPDATE_TEXT main_03
	void near pattern_random_rain_and_spreads_from_wings(void);
	void near pattern_cheetos_within_spread_walls(void);
	void near pattern_aimed_b6balls_and_symmetric_spreads(void);
	void near pattern_devil(void);
	#pragma codeseg

	extern pattern_oneshot_func_t midbossx_phase_1_pattern;
	extern "C" const pattern_oneshot_func_t MIDBOSSX_PATTERNS_PHASE_1[2][2];
	#pragma codeseg BX_UPDATE_TEXT main_03
	bool near pattern_wait(void);
	#pragma codeseg
	extern "C" pattern_oneshot_func_t exalice_pattern;
	extern "C" const pattern_oneshot_func_t EXALICE_PATTERNS[4][2];
	extern "C" unsigned char exalice_invincibility_frames;
	extern "C" PlayfieldPoint exalice_random_origin;
	extern "C" subpixel_t exalice_pattern_origin_x;
	extern "C" int exalice_laser_slot;
	extern "C" unsigned int exalice_overlay_patnum;
	extern rck_firewave_t firewaves[2];
	#pragma codeseg BX_TEXT main_03
	bool near pattern_spreads_and_firewaves(void);
	bool near pattern_bouncing_blue_rings(void);
	bool near pattern_pingpong_lasers(void);
	bool near pattern_mirrored_crosses(void);
	#pragma codeseg

	extern y_direction_t boss_flystep_random_next_y_direction;
	extern int s2particles_spawned;
	extern unsigned char stage2_bg_pulse;
	extern unsigned char stage2_flash_tone;
	extern int8_t stage2_bg_pulse_direction;
#else
	#define RCK_CHECKERBOARD_H 32
	#define RCK_CHECKERBOARD_SPEED 4
	#define RCK_CHECKERBOARD_OFF_MAX ((RCK_CHECKERBOARD_H - 1) * ROW_SIZE)

	struct rck_checkerboard_t {
		int16_t seg_bottom;
		uint16_t off_bottom;
		uint16_t off_top;
		uint8_t vo_x_of_dark;
		uint8_t loops;
	};
	extern rck_checkerboard_t checkerboard;

	extern uint8_t score_unused;
	extern vram_offset_t CARPET_TILE_IMAGE_VOS[3][TILES_X];
	extern uint8_t CARPET_LIGHTING_ANIM[8][TILES_X];
	extern int carpet_lighting_cel;
	extern uint8_t carpet_light_level;
	extern unsigned char dream_items_collected;
	extern "C" input_t word_2598C;
	extern "C" uint8_t byte_25980;
	extern uint8_t shot_reimu_cycle;
	extern nearfunc_t_near shot_funcs_reimu_a[];
	extern nearfunc_t_near shot_funcs_reimu_b[];
	extern nearfunc_t_near shot_funcs_marisa_a[];
	extern nearfunc_t_near shot_funcs_marisa_b[];
	extern "C" {
		extern uint8_t midboss1_25594;
		extern uint8_t midboss2_pattern;
		extern uint8_t midboss2_255B3;
		extern uint8_t midboss2_passes;
		extern uint8_t midboss3_pattern;
		extern uint8_t midboss3_25599;
		extern uint8_t midboss3_patterns_done;
		extern uint8_t midboss4_pattern;
		extern uint8_t midboss4_passes;
		extern uint8_t midboss4_22B9E;
		extern uint8_t midboss4_255C8;
		extern uint8_t kurumi_259F0;
		extern uint8_t elly_pattern_set;
		extern uint8_t elly_25A26;
		extern unsigned int elly_25A34;
		extern uint8_t elly_25A36;
		extern uint8_t elly_25A37;
		extern uint8_t elly_25A38;
		extern int elly_25A3A;
		extern uint8_t elly_boomerang_flag;
		extern PlayfieldMotion elly_boomerang_pos;
		extern uint8_t reimu_afterimage;
		extern uint8_t reimu_sweep_angle_delta;
		extern uint8_t marisa_pattern_prev;
		extern uint8_t marisa_bits_at_pattern_start;
		extern bool marisa_pulse_dimming;
		extern uint8_t marisa_25671;
		extern uint8_t marisa_patterns_without_bits;
		extern uint8_t marisa_explode_milestone;
		extern subpixel_t yuuka5_25662;
		extern uint8_t yuuka5_25664;
		extern uint8_t yuuka5_25665;
		extern uint8_t yuuka5_25666;
		extern uint8_t yuuka5_warp_phase;
		extern uint8_t yuuka6_bg_state;
		extern uint8_t yuuka6_bg_state_frame;
		extern bool yuuka6_bg_fade_done;
		extern int yuuka6_anim_frame;
		extern uint8_t yuuka6_sprite_flag;
		extern uint8_t yuuka6_phase2_fly_path;
		extern uint8_t yuuka6_25A02;
		extern uint8_t yuuka6_25A03;
		extern uint8_t yuuka6_25A04;
		extern uint8_t yuuka6_25A08;
		extern PlayfieldPoint yuuka6_25A0C;
		extern uint8_t yuuka6_25A1B;
		extern uint8_t yuuka6_25A1E;
		extern int mugetsu_gather_frame_offset;
		extern uint8_t (near *mugetsu_pose_func)(void);
		extern SPPoint mugetsu_gather_center;
		extern uint8_t mugetsu_phase2_mode;
		extern uint8_t mugetsu_damage_frames;
		extern uint8_t gengetsu_damage_frames;
		extern uint8_t extra_boss_bomb_immunity;
	}
	extern uint8_t gengetsu_wave_amp;
	extern Subpixel gengetsu_wave_target_x;
	uint8_t near mugetsu_180BB(void);
	uint8_t near mugetsu_1812A(void);
	uint8_t near mugetsu_1821E(void);

	struct bg_shape_t {
		SPPoint pos;
		uint8_t angle;
		SubpixelLength8 speed;
	};
	extern bg_shape_t bg_shapes[57];
	extern main_patnum_t bg_shape_patnum;
	extern Subpixel bg_shape_flyout_speed;
	extern void (near pascal *near bg_shape_clip)(bg_shape_t near& shape);
	void pascal near bg_shape_clip_and_respawn_in_cen(bg_shape_t near& shape);
	void pascal near bg_shape_clip_and_wrap(bg_shape_t near& shape);

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
	stream->io_func = 0;
	stream->io_context = 0;
	stream->window_capacity = 0;
	stream->window_pos = 0;
	stream->window_len = 0;
}

static void rck_stream_io_init(
	replay_ck_stream_t far *stream,
	void far *buffer,
	uint16_t buffer_size,
	uint32_t size,
	replay_ck_mode_t mode,
	replay_ck_io_func_t io_func,
	uint16_t context
)
{
	rck_stream_init(stream, buffer, size, mode);
	stream->io_func = io_func;
	stream->io_context = context;
	stream->window_capacity = buffer_size;
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

bool replay_ck_finish(replay_ck_stream_t far *stream)
{
	if(stream->failed) {
		return false;
	}
	if((stream->data != 0) && (stream->pos != stream->limit)) {
		return false;
	}
	if(
		(stream->io_func != 0) && !rck_reading(stream) &&
		(stream->window_pos != 0)
	) {
		if(!stream->io_func(
			stream->io_context, stream->data, stream->window_pos
		)) {
			stream->failed = true;
			return false;
		}
		stream->window_pos = 0;
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
		if(stream->io_func != 0) {
			if(stream->window_pos >= stream->window_len) {
				stream->window_len = static_cast<uint16_t>(
					((stream->limit - stream->pos) > stream->window_capacity)
					? stream->window_capacity
					: (stream->limit - stream->pos)
				);
				stream->window_pos = 0;
				if(
					(stream->window_len == 0) ||
					!stream->io_func(
						stream->io_context, stream->data,
						stream->window_len
					)
				) {
					stream->failed = true;
					return false;
				}
			}
			encoded = stream->data[stream->window_pos++];
		} else {
			encoded = stream->data[static_cast<uint16_t>(stream->pos)];
		}
		*value = encoded;
	} else {
		encoded = *value;
		if(stream->io_func != 0) {
			if(stream->window_pos >= stream->window_capacity) {
				if(!stream->io_func(
					stream->io_context, stream->data,
					stream->window_pos
				)) {
					stream->failed = true;
					return false;
				}
				stream->window_pos = 0;
			}
			stream->data[stream->window_pos++] = encoded;
		} else if(stream->data != 0) {
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

static bool rck_u8_field(
	replay_ck_stream_t far *stream, uint8_t far *field,
	uint8_t minimum, uint8_t maximum
)
{
	uint8_t value = *field;
	if(
		!rck_u8(stream, &value) ||
		(value < minimum) || (value > maximum)
	) {
		return false;
	}
	if(rck_applying(stream)) {
		*field = value;
	}
	return true;
}

static bool rck_u16_field(
	replay_ck_stream_t far *stream, uint16_t far *field, uint16_t maximum
)
{
	uint16_t value = *field;
	if(!rck_u16(stream, &value) || (value > maximum)) {
		return false;
	}
	if(rck_applying(stream)) {
		*field = value;
	}
	return true;
}

static bool rck_u32_field(
	replay_ck_stream_t far *stream, uint32_t far *field
)
{
	uint32_t value = *field;
	if(!rck_u32(stream, &value)) {
		return false;
	}
	if(rck_applying(stream)) {
		*field = value;
	}
	return true;
}

static bool rck_bool16_field(
	replay_ck_stream_t far *stream, bool16 far *field
)
{
	uint8_t value = (*field ? 1 : 0);
	if(!rck_u8(stream, &value) || (value > 1)) {
		return false;
	}
	if(rck_applying(stream)) {
		*field = (value != 0);
	}
	return true;
}

#define RCK_FIELD_PTR(type, field) \
	reinterpret_cast<type far *>(&(field))
#define RCK_REQUIRE_FIELD_SIZE(field, size) \
	(void)(1 / ((sizeof field == (size)) ? 1 : 0))

#define RCK_U8(field) do { \
	RCK_REQUIRE_FIELD_SIZE(field, 1); \
	if(!rck_u8_field(stream, RCK_FIELD_PTR(uint8_t, field), 0, 0xFF)) { \
		return false; \
	} \
} while(0)

#define RCK_U8_RANGE(field, minimum, maximum) do { \
	RCK_REQUIRE_FIELD_SIZE(field, 1); \
	if(!rck_u8_field( \
		stream, RCK_FIELD_PTR(uint8_t, field), (minimum), (maximum) \
	)) { \
		return false; \
	} \
} while(0)

#define RCK_U8_MAX(field, maximum) do { \
	RCK_REQUIRE_FIELD_SIZE(field, 1); \
	if(!rck_u8_field( \
		stream, RCK_FIELD_PTR(uint8_t, field), 0, (maximum) \
	)) { \
		return false; \
	} \
} while(0)

#define RCK_S8(field) RCK_U8(field)

#define RCK_U16(field) do { \
	RCK_REQUIRE_FIELD_SIZE(field, 2); \
	if(!rck_u16_field( \
		stream, RCK_FIELD_PTR(uint16_t, field), 0xFFFF \
	)) { \
		return false; \
	} \
} while(0)

#define RCK_U16_MAX(field, maximum) do { \
	RCK_REQUIRE_FIELD_SIZE(field, 2); \
	if(!rck_u16_field( \
		stream, RCK_FIELD_PTR(uint16_t, field), (maximum) \
	)) { \
		return false; \
	} \
} while(0)

#define RCK_S16(field) RCK_U16(field)

#define RCK_U32(field) do { \
	RCK_REQUIRE_FIELD_SIZE(field, 4); \
	if(!rck_u32_field(stream, RCK_FIELD_PTR(uint32_t, field))) { \
		return false; \
	} \
} while(0)

#define RCK_S32(field) RCK_U32(field)

#define RCK_BOOL(field) do { \
	RCK_REQUIRE_FIELD_SIZE(field, 1); \
	if(!rck_u8_field(stream, RCK_FIELD_PTR(uint8_t, field), 0, 1)) { \
		return false; \
	} \
} while(0)

#define RCK_BOOL16(field) do { \
	RCK_REQUIRE_FIELD_SIZE(field, 2); \
	if(!rck_bool16_field(stream, RCK_FIELD_PTR(bool16, field))) { \
		return false; \
	} \
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

	// The update switch has a no-op default, and ordinary TH04 bullets can
	// carry zero here. Every byte value is deterministic and memory-safe.
	if(!rck_u8(stream, &value)) {
		return false;
	}
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
	case BG_FORCESINGLE_AIMED:
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
	replay_ck_stream_t far *stream, BulletTemplate far *tmpl, bool validate
)
{
	uint8_t spawn_type = tmpl->spawn_type;
	uint8_t group = static_cast<uint8_t>(tmpl->group);

	if(
		!rck_u8(stream, &spawn_type) ||
		(validate && !rck_bullet_spawn_type_valid(spawn_type))
	) {
		return false;
	}
	RCK_U8(tmpl->patnum);
	if(!rck_pfpoint(stream, &tmpl->origin)) {
		return false;
	}
#if (GAME == 5)
	if(
		!rck_u8(stream, &group) ||
		(validate && !rck_bullet_group_valid(group))
	) {
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
	if(
		!rck_u8(stream, &group) ||
		(validate && !rck_bullet_group_valid(group))
	) {
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

static bool rck_group_scoring(replay_ck_stream_t far *stream)
{
	int i;

	for(i = 0; i < SCORE_DIGITS; i++) {
		RCK_U8_MAX(score.digits[i], 9);
	}
	for(i = 0; i < SCORE_DIGITS; i++) {
		RCK_U8_MAX(hiscore.digits[i], 9);
	}
#if (GAME == 4)
	RCK_U8(score_unused);
#endif
	RCK_BOOL(hiscore_popup_shown);
	RCK_U16(graze_score);
	RCK_U8(playperf_max);
	RCK_S8(playperf_min);
	RCK_U32(resident->slow_frames);
#if (GAME == 5)
	RCK_U8(byte_22274);
	RCK_U8(byte_22275);
#endif
	return true;
}

enum rck_stage_render_t {
	RCKSR_NULL = 0,
	RCKSR_STAGE_2_OR_4 = 1,
	RCKSR_STAGE_5 = 2,
};

enum rck_stage_invalidate_t {
	RCKSI_NULL = 0,
	RCKSI_STAGE_2_OR_5 = 1,
};

enum rck_bg_render_t {
	RCKBG_NULL = 0,
	RCKBG_TILES = 1,
	RCKBG_TILES_ALL = 2,
	RCKBG_TILES_ALL_TIMED = 3,
	RCKBG_STAGE_1 = 4,
	RCKBG_STAGE_2 = 5,
	RCKBG_STAGE_3 = 6,
	RCKBG_STAGE_4 = 7,
	RCKBG_STAGE_5 = 8,
	RCKBG_STAGE_6 = 9,
	RCKBG_STAGE_EXTRA = 10,
};

static uint8_t rck_stage_render_id(void)
{
	if(stage_render == nullfunc_near) {
		return RCKSR_NULL;
	}
#if (GAME == 5)
	if(stage_render == stage2_update) {
		return RCKSR_STAGE_2_OR_4;
	}
#else
	if(stage_render == stage4_render) {
		return RCKSR_STAGE_2_OR_4;
	}
	if(stage_render == stage5_render) {
		return RCKSR_STAGE_5;
	}
#endif
	return 0xFF;
}

static nearfunc_t_near rck_stage_render_func(uint8_t id)
{
	switch(id) {
	case RCKSR_NULL:
		return nullfunc_near;
	case RCKSR_STAGE_2_OR_4:
#if (GAME == 5)
		return stage2_update;
#else
		return stage4_render;
#endif
#if (GAME == 4)
	case RCKSR_STAGE_5:
		return stage5_render;
#endif
	}
	return 0;
}

static bool rck_stage_render_compatible(uint8_t id)
{
	if(id == RCKSR_NULL) {
		return true;
	}
#if (GAME == 5)
	return ((id == RCKSR_STAGE_2_OR_4) && (stage_id == 1));
#else
	return (
		((id == RCKSR_STAGE_2_OR_4) && (stage_id == 3)) ||
		((id == RCKSR_STAGE_5) && (stage_id == 4))
	);
#endif
}

static uint8_t rck_stage_invalidate_id(void)
{
	if(stage_invalidate == nullfunc_near) {
		return RCKSI_NULL;
	}
#if (GAME == 5)
	if(stage_invalidate == stage2_invalidate) {
#else
	if(stage_invalidate == stage5_invalidate) {
#endif
		return RCKSI_STAGE_2_OR_5;
	}
	return 0xFF;
}

static nearfunc_t_near rck_stage_invalidate_func(uint8_t id)
{
	switch(id) {
	case RCKSI_NULL:
		return nullfunc_near;
	case RCKSI_STAGE_2_OR_5:
#if (GAME == 5)
		return stage2_invalidate;
#else
		return stage5_invalidate;
#endif
	}
	return 0;
}

static bool rck_stage_invalidate_compatible(uint8_t id)
{
	if(id == RCKSI_NULL) {
		return true;
	}
	return (
		(id == RCKSI_STAGE_2_OR_5) &&
		(stage_id == ((GAME == 5) ? 1 : 4))
	);
}

static uint8_t rck_bg_render_id(nearfunc_t_near callback)
{
	if(callback == nullfunc_near) {
		return RCKBG_NULL;
	}
	if(callback == tiles_render) {
		return RCKBG_TILES;
	}
	if(callback == tiles_render_all) {
		return RCKBG_TILES_ALL;
	}
	if(callback == tiles_render_all_timed) {
		return RCKBG_TILES_ALL_TIMED;
	}
#if (GAME == 5)
	if(callback == sara_bg_render) { return RCKBG_STAGE_1; }
	if(callback == louise_bg_render) { return RCKBG_STAGE_2; }
	if(callback == alice_bg_render) { return RCKBG_STAGE_3; }
	if(callback == mai_yuki_bg_render) { return RCKBG_STAGE_4; }
	if(callback == yumeko_bg_render) { return RCKBG_STAGE_5; }
	if(callback == shinki_bg_render) { return RCKBG_STAGE_6; }
	if(callback == exalice_bg_render) { return RCKBG_STAGE_EXTRA; }
#else
	if(callback == orange_bg_render) { return RCKBG_STAGE_1; }
	if(callback == kurumi_bg_render) { return RCKBG_STAGE_2; }
	if(callback == elly_bg_render) { return RCKBG_STAGE_3; }
	if(callback == reimu_marisa_bg_render) { return RCKBG_STAGE_4; }
	if(callback == yuuka5_bg_render) { return RCKBG_STAGE_5; }
	if(callback == yuuka6_bg_render) { return RCKBG_STAGE_6; }
	if(callback == mugetsu_gengetsu_bg_render) {
		return RCKBG_STAGE_EXTRA;
	}
#endif
	return 0xFF;
}

static nearfunc_t_near rck_bg_render_func(uint8_t id)
{
	switch(id) {
	case RCKBG_NULL: return nullfunc_near;
	case RCKBG_TILES: return tiles_render;
	case RCKBG_TILES_ALL: return tiles_render_all;
	case RCKBG_TILES_ALL_TIMED: return tiles_render_all_timed;
#if (GAME == 5)
	case RCKBG_STAGE_1: return sara_bg_render;
	case RCKBG_STAGE_2: return louise_bg_render;
	case RCKBG_STAGE_3: return alice_bg_render;
	case RCKBG_STAGE_4: return mai_yuki_bg_render;
	case RCKBG_STAGE_5: return yumeko_bg_render;
	case RCKBG_STAGE_6: return shinki_bg_render;
	case RCKBG_STAGE_EXTRA: return exalice_bg_render;
#else
	case RCKBG_STAGE_1: return orange_bg_render;
	case RCKBG_STAGE_2: return kurumi_bg_render;
	case RCKBG_STAGE_3: return elly_bg_render;
	case RCKBG_STAGE_4: return reimu_marisa_bg_render;
	case RCKBG_STAGE_5: return yuuka5_bg_render;
	case RCKBG_STAGE_6: return yuuka6_bg_render;
	case RCKBG_STAGE_EXTRA: return mugetsu_gengetsu_bg_render;
#endif
	}
	return 0;
}

static bool rck_bg_render_compatible(uint8_t id)
{
	if(id < RCKBG_STAGE_1) {
		return (id <= RCKBG_TILES_ALL_TIMED);
	}
	return (id == (RCKBG_STAGE_1 + stage_id));
}

static bool rck_group_field(replay_ck_stream_t far *stream)
{
	int y;
	int x;
	int i;
	uint8_t stage_render_id = rck_stage_render_id();
	uint8_t stage_invalidate_id = rck_stage_invalidate_id();
	uint8_t bg_not_bombing_id = rck_bg_render_id(bg_render_not_bombing);
	uint8_t bg_bombing_id = rck_bg_render_id(bg_render_bombing);
	uint8_t bg_bombing_func_id = rck_bg_render_id(bg_render_bombing_func);
	uint8_t render_all_time = tile_render_all_time;

	replay_ck_failure_field_value = 0x1000;
	RCK_U8_MAX(scroll_subpixel_line.v, SUBPIXEL_FACTOR - 1);
	RCK_U8(scroll_speed.v);
	RCK_U16_MAX(scroll_line, RES_Y - 1);
	RCK_S16(scroll_last_delta.v);
	RCK_BOOL(scroll_active);
	for(i = 0; i < PAGE_COUNT; i++) {
		RCK_U16_MAX(scroll_line_on_page[i], RES_Y - 1);
	}
	RCK_U8(scroll_lines_pending);
	RCK_U8(scroll_lines_prev_frame);
	for(y = 0; y < TILES_Y; y++) {
		for(x = 0; x < TILES_MEMORY_X; x++) {
			RCK_U16(tile_ring[y][x]);
		}
	}
	RCK_S8(tile_row_in_section);
	RCK_S16(tile_ring_row_filled);
	if(!rck_u8(stream, &render_all_time) || (render_all_time > PAGE_COUNT)) {
		return false;
	}
	replay_ck_failure_field_value = 0x2000;
	if(
		!rck_u8(stream, &stage_render_id) ||
		!rck_u8(stream, &stage_invalidate_id) ||
		!rck_u8(stream, &bg_not_bombing_id) ||
		!rck_u8(stream, &bg_bombing_id) ||
		!rck_u8(stream, &bg_bombing_func_id)
	) {
		return false;
	}
	replay_ck_failure_field_value = 0x2100;
	if(!rck_stage_render_compatible(stage_render_id)) {
		return false;
	}
	replay_ck_failure_field_value = 0x2200;
	if(!rck_stage_invalidate_compatible(stage_invalidate_id)) {
		return false;
	}
	replay_ck_failure_field_value = 0x2300;
	if(
		!rck_bg_render_compatible(bg_not_bombing_id) ||
		(
			(bg_not_bombing_id == RCKBG_TILES_ALL_TIMED) &&
			(render_all_time == 0)
		)
	) {
		replay_ck_failure_field_value = static_cast<uint16_t>(
			0xA000 | ((render_all_time & 0xF) << 8) |
			((stage_id & 0xF) << 4) | (bg_not_bombing_id & 0xF)
		);
		return false;
	}
	replay_ck_failure_field_value = 0x2400;
	if(!rck_bg_render_compatible(bg_bombing_id)) {
		return false;
	}
	replay_ck_failure_field_value = 0x2500;
	if(
		!rck_bg_render_compatible(bg_bombing_func_id) ||
		(
			(bg_bombing_id != RCKBG_NULL) &&
			(bg_bombing_id != bg_bombing_func_id)
		)
	) {
		return false;
	}
	if(rck_applying(stream)) {
		tile_render_all_time = render_all_time;
		stage_render = rck_stage_render_func(stage_render_id);
		stage_invalidate = rck_stage_invalidate_func(stage_invalidate_id);
		bg_render_not_bombing = rck_bg_render_func(bg_not_bombing_id);
		bg_render_bombing = rck_bg_render_func(bg_bombing_id);
		bg_render_bombing_func = rck_bg_render_func(bg_bombing_func_id);
	}
#if (GAME == 5)
	if(stage_id == 1) {
		uint16_t particles_spawned = static_cast<uint16_t>(
			s2particles_spawned
		);
		uint8_t pulse_direction = static_cast<uint8_t>(
			stage2_bg_pulse_direction
		);
		if(
			!rck_u16(stream, &particles_spawned) ||
			(particles_spawned > 64)
		) {
			return false;
		}
		RCK_U8_MAX(stage2_bg_pulse, 0x3F);
		RCK_U8(stage2_flash_tone);
		if(
			!rck_u8(stream, &pulse_direction) ||
			(pulse_direction > 1)
		) {
			return false;
		}
		if(rck_applying(stream)) {
			s2particles_spawned = particles_spawned;
			stage2_bg_pulse_direction = pulse_direction;
		}
	}
#endif
	return true;
}

static bool rck_spark(
	replay_ck_stream_t far *stream, spark_t far *spark
)
{
	if(!rck_entity_flag(stream, &spark->flag)) {
		return false;
	}
	RCK_U8(spark->age);
	if(!rck_motion(stream, &spark->center)) {
		return false;
	}
	RCK_U16(spark->angle);
	return true;
}

static bool rck_circle(
	replay_ck_stream_t far *stream, circle_t far *circle
)
{
	if(!rck_entity_flag(stream, &circle->flag)) {
		return false;
	}
	RCK_U8(circle->age);
	RCK_S16(circle->center.x);
	RCK_S16(circle->center.y);
	RCK_S16(circle->radius_cur);
	RCK_S16(circle->radius_delta);
	return true;
}

#if (GAME == 5)
static bool rck_boss_particle(
	replay_ck_stream_t far *stream, rck_boss_particle_t far *particle
)
{
	if(
		!rck_pfpoint(stream, &particle->pos) ||
		!rck_pfpoint(stream, &particle->origin) ||
		!rck_sppoint(stream, &particle->velocity)
	) {
		return false;
	}
	RCK_S16(particle->age);
	RCK_U8(particle->angle);
	RCK_U8(particle->patnum);
	return true;
}

static bool rck_lineset(
	replay_ck_stream_t far *stream, rck_lineset_t far *set
)
{
	int i;

	for(i = 0; i < RCK_LINESET_LINE_COUNT; i++) {
		if(!rck_sppoint(stream, &set->center[i])) {
			return false;
		}
	}
	RCK_S16(set->velocity_y.v);
	for(i = 0; i < RCK_LINESET_LINE_COUNT; i++) {
		RCK_S16(set->radius[i].v);
	}
	for(i = 0; i < RCK_LINESET_LINE_COUNT; i++) {
		RCK_U8(set->angle[i]);
	}
	return true;
}
#else
static uint8_t rck_checkerboard_phase(void)
{
	uint16_t segment = static_cast<uint16_t>(checkerboard.seg_bottom);
	uint16_t segment_delta;
	uint16_t y;
	uint16_t bottom_delta;

	if(segment < SEG_PLANE_B) {
		return 0xFF;
	}
	segment_delta = (segment - SEG_PLANE_B);
	if((segment_delta % (ROW_SIZE / 16)) != 0) {
		return 0xFF;
	}
	y = (segment_delta / (ROW_SIZE / 16));
	if(y >= PLAYFIELD_BOTTOM) {
		return 0xFF;
	}
	bottom_delta = (PLAYFIELD_BOTTOM - y);
	if(
		(bottom_delta < RCK_CHECKERBOARD_SPEED) ||
		(bottom_delta > RCK_CHECKERBOARD_H) ||
		((bottom_delta % RCK_CHECKERBOARD_SPEED) != 0)
	) {
		return 0xFF;
	}
	return (bottom_delta / RCK_CHECKERBOARD_SPEED);
}

static bool rck_checkerboard(replay_ck_stream_t far *stream)
{
	uint8_t phase = rck_checkerboard_phase();
	uint16_t off_bottom = checkerboard.off_bottom;
	uint16_t off_top = checkerboard.off_top;
	uint8_t vo_x_of_dark = checkerboard.vo_x_of_dark;

	if(
		!rck_u8(stream, &phase) ||
		(phase < 1) ||
		(phase > (RCK_CHECKERBOARD_H / RCK_CHECKERBOARD_SPEED))
	) {
		return false;
	}
	if(rck_applying(stream)) {
		uint16_t y = (
			PLAYFIELD_BOTTOM - (phase * RCK_CHECKERBOARD_SPEED)
		);
		checkerboard.seg_bottom = static_cast<int16_t>(
			SEG_PLANE_B + ((y * ROW_SIZE) / 16)
		);
	}
	if(!rck_u16(stream, &off_bottom) || !rck_u16(stream, &off_top)) {
		return false;
	}
	if(
		(off_bottom > RCK_CHECKERBOARD_OFF_MAX) ||
		((off_bottom % ROW_SIZE) != 0) ||
		(off_top > RCK_CHECKERBOARD_OFF_MAX) ||
		((off_top % ROW_SIZE) != 0) ||
		!rck_u8(stream, &vo_x_of_dark) ||
		((vo_x_of_dark != PLAYFIELD_VRAM_LEFT) &&
		 (vo_x_of_dark != (
			PLAYFIELD_VRAM_LEFT + (RCK_CHECKERBOARD_H / BYTE_DOTS)
		)))
	) {
		return false;
	}
	if(rck_applying(stream)) {
		checkerboard.off_bottom = off_bottom;
		checkerboard.off_top = off_top;
		checkerboard.vo_x_of_dark = vo_x_of_dark;
		checkerboard.loops = 2;
	}
	return true;
}
#endif

static bool rck_group_effects(replay_ck_stream_t far *stream)
{
	int i;
	uint16_t ring_offset = spark_ring_offset;
	uint8_t shake_redraw_time = playfield_shake_redraw_time;

	for(i = 0; i < SPARK_COUNT; i++) {
		if(!rck_spark(stream, &sparks[i])) {
			return false;
		}
	}
	if(
		!rck_u16(stream, &ring_offset) ||
		(ring_offset >= (SPARK_COUNT * RCK_SPARK_SIZE)) ||
		((ring_offset % RCK_SPARK_SIZE) != 0)
	) {
		return false;
	}
	if(rck_applying(stream)) {
		spark_ring_offset = ring_offset;
	}
	for(i = 0; i < CIRCLE_COUNT; i++) {
		if(!rck_circle(stream, &circles[i])) {
			return false;
		}
	}
	RCK_U8_MAX(circles_color, 15);
	if(!rck_pfpoint(stream, &drawpoint)) {
		return false;
	}
	RCK_S16(miss_explosion_radius);
	RCK_U8(miss_explosion_angle);
	RCK_S16(playfield_shake_x);
	RCK_S16(playfield_shake_y);
	RCK_U16_MAX(playfield_shake_anim_time, RCK_SHAKE_ANIM_TIME_MAX);
	if(
		!rck_u8(stream, &shake_redraw_time) ||
		(shake_redraw_time > PAGE_COUNT)
	) {
		return false;
	}
	if(rck_applying(stream)) {
		playfield_shake_redraw_time = shake_redraw_time;
	}
#if (GAME == 5)
	for(i = 0; i < RCK_BOSS_PARTICLE_COUNT; i++) {
		if(!rck_boss_particle(stream, &boss_particles[i])) {
			return false;
		}
	}
	for(i = 0; i < RCK_LINESET_COUNT; i++) {
		if(!rck_lineset(stream, &linesets[i])) {
			return false;
		}
	}
	RCK_U8(shinki_bg_linesets_zoomed_out);
	{
		uint16_t particles_alive = shinki_bg_type_a_particles_alive;
		if(
			!rck_u16(stream, &particles_alive) ||
			((particles_alive > RCK_BOSS_PARTICLE_COUNT) &&
			 (particles_alive != 0xFF))
		) {
			return false;
		}
		if(rck_applying(stream)) {
			shinki_bg_type_a_particles_alive = particles_alive;
		}
	}
	RCK_BOOL(shinki_bg_type_b_initialized);
	RCK_U16(shinki_bg_spinline_frame);
	RCK_BOOL(shinki_bg_type_c_initialized);
	RCK_BOOL(shinki_bg_type_d_initialized);
#else
	if(!rck_checkerboard(stream)) {
		return false;
	}
#endif
	return true;
}

struct rck_std_source_t {
	uint16_t end;
	uint16_t ip_initial;
	uint16_t ip_terminal;
	uint8_t script_count;
	uint32_t checksum;
};

static uint8_t rck_std_source_u8(uint16_t offset)
{
	return *reinterpret_cast<uint8_t far *>(MK_FP(
		reinterpret_cast<uint16_t>(std_seg), offset
	));
}

static uint16_t rck_std_source_u16(uint16_t offset)
{
	return (
		static_cast<uint16_t>(rck_std_source_u8(offset)) |
		(static_cast<uint16_t>(rck_std_source_u8(offset + 1)) << 8)
	);
}

static uint16_t rck_std_near_offset(const void near *pointer)
{
	if(pointer == 0) {
		return RCK_STD_OFFSET_NONE;
	}
	return reinterpret_cast<uint16_t>(pointer);
}

static bool rck_std_source_info(rck_std_source_t *source)
{
	uint16_t script_offset;
	uint16_t script_size;
	uint32_t event_offset;
	uint32_t next_offset;
	uint32_t i;

	if(std_seg == 0 || std_enemy_scripts[0] == 0) {
		return false;
	}
	script_offset = rck_std_near_offset(std_enemy_scripts[0]);
	if(script_offset < 2) {
		return false;
	}
	source->script_count = rck_std_source_u8(script_offset - 2);
	if(
		(source->script_count < 1) ||
		(source->script_count > STD_ENEMY_SCRIPT_COUNT)
	) {
		return false;
	}
	for(i = 0; i < source->script_count; i++) {
		if(rck_std_near_offset(std_enemy_scripts[i]) != script_offset) {
			return false;
		}
		script_size = rck_std_source_u8(script_offset - 1);
		if(script_size == 0) {
			return false;
		}
		next_offset = (
			static_cast<uint32_t>(script_offset) + script_size + 1
		);
		if(next_offset > 0xFFFFUL) {
			return false;
		}
		script_offset = static_cast<uint16_t>(next_offset);
	}
	source->ip_initial = script_offset;
	event_offset = source->ip_initial;
	while(event_offset <= 0xFFFDUL) {
		uint16_t event_frame = rck_std_source_u16(
			static_cast<uint16_t>(event_offset)
		);
		uint8_t spawn_count;

		if(event_frame == 0) {
			source->ip_terminal = static_cast<uint16_t>(event_offset);
			source->end = static_cast<uint16_t>(event_offset + 2);
			source->checksum = REPLAY_FNV1A_BASIS;
			for(i = 0; i < source->end; i++) {
				source->checksum ^= rck_std_source_u8(
					static_cast<uint16_t>(i)
				);
				source->checksum *= REPLAY_FNV1A_PRIME;
			}
			return true;
		}
		if(event_offset > 0xFFFCUL) {
			return false;
		}
		spawn_count = rck_std_source_u8(
			static_cast<uint16_t>(event_offset + 2)
		);
		if(spawn_count == 0) {
			return false;
		}
		next_offset = (
			event_offset + 3 +
			(static_cast<uint32_t>(spawn_count) * STD_ENEMY_SPAWN_SIZE)
		);
		if((next_offset <= event_offset) || (next_offset > 0xFFFFUL)) {
			return false;
		}
		event_offset = next_offset;
	}
	return false;
}

static uint32_t rck_std_ip_offset(void)
{
	uint16_t base_segment = reinterpret_cast<uint16_t>(std_seg);
	uint16_t pointer_segment;

	if(std_ip == 0 || base_segment == 0) {
		return 0xFFFFFFFFUL;
	}
	pointer_segment = FP_SEG(std_ip);
	if(pointer_segment < base_segment) {
		return 0xFFFFFFFFUL;
	}
	return (
		(static_cast<uint32_t>(pointer_segment - base_segment) << 4) +
		FP_OFF(std_ip)
	);
}

static bool rck_std_ip_valid(
	const rck_std_source_t *source, uint32_t wanted
)
{
	uint32_t event_offset = source->ip_initial;

	while(event_offset <= source->ip_terminal) {
		uint8_t spawn_count;

		if(event_offset == wanted) {
			return true;
		}
		if(event_offset == source->ip_terminal) {
			return false;
		}
		spawn_count = rck_std_source_u8(
			static_cast<uint16_t>(event_offset + 2)
		);
		event_offset += (
			3 + (static_cast<uint32_t>(spawn_count) * STD_ENEMY_SPAWN_SIZE)
		);
	}
	return false;
}

enum rck_stage_vm_t {
	RCKSVM_NONE = 0,
	RCKSVM_RUN = 1,
};

static uint8_t rck_stage_vm_id(void)
{
	if(stage_vm == nullfunc_far) {
		return RCKSVM_NONE;
	}
	if(stage_vm == std_run) {
		return RCKSVM_RUN;
	}
	return 0xFF;
}

static func_t_near rck_stage_vm_func(uint8_t id)
{
	switch(id) {
	case RCKSVM_NONE: return nullfunc_far;
	case RCKSVM_RUN: return std_run;
	}
	return 0;
}

static bool rck_practice_map_row_fill(void)
{
	uint16_t map_list_offset;
	uint8_t map_section;
	uint16_t source_offset;
	const uint8_t far *source;
	int row = (scroll_line >> TILE_BITS_H);
	int x;

#if (GAME == 5)
	map_list_offset = static_cast<uint16_t>(std_map_section_p);
#else
	map_list_offset = static_cast<uint16_t>(std_map_section_id);
#endif
	map_section = rck_std_source_u8(map_list_offset);
#if (GAME == 5)
	if(map_section & 1) {
		return false;
	}
	map_section >>= 1;
#endif
	if((map_section >= 32) || (row >= TILES_Y)) {
		return false;
	}
	source_offset = static_cast<uint16_t>(
		TILE_SECTION_OFFSETS[map_section] +
		(tile_row_in_section * TILES_MEMORY_X * 2)
	);
	source = reinterpret_cast<const uint8_t far *>(MK_FP(
		reinterpret_cast<uint16_t>(map_seg), source_offset
	));
	for(x = 0; x < TILES_X; x++) {
		tile_ring[row][x] = *reinterpret_cast<const vram_offset_t far *>(
			source + (x * 2)
		);
	}
	return true;
}

static bool rck_practice_scroll_step(
	const rck_std_source_t *source
)
{
	uint8_t subpixel_line = static_cast<uint8_t>(
		scroll_subpixel_line.v + scroll_speed.v
	);
	uint8_t lines = 0;
	uint8_t lines_last_frame;
	int next_line;
	uint16_t offset;

	scroll_last_delta.v = 0;
	if(subpixel_line >= SUBPIXEL_FACTOR) {
		lines = static_cast<uint8_t>(subpixel_line >> SUBPIXEL_BITS);
		next_line = (scroll_line - lines);
		if(next_line < 0) {
			next_line += RES_Y;
		}
		scroll_line = next_line;
		scroll_lines_pending = lines;
		subpixel_line &= (SUBPIXEL_FACTOR - 1);
		scroll_last_delta.v = static_cast<subpixel_t>(
			lines << SUBPIXEL_BITS
		);
	}
	scroll_subpixel_line.v = subpixel_line;
	if((scroll_lines_pending == 0) && (scroll_lines_prev_frame == 0)) {
		return true;
	}
	if(scroll_speed.v == 0) {
		return true;
	}
	if((scroll_line >> TILE_BITS_H) != tile_ring_row_filled) {
		tile_ring_row_filled = (scroll_line >> TILE_BITS_H);
		tile_row_in_section--;
		if(tile_row_in_section < 0) {
			tile_row_in_section = 4;
#if (GAME == 5)
			std_map_section_p++;
			offset = static_cast<uint16_t>(std_map_section_p);
#else
			std_map_section_id++;
			offset = static_cast<uint16_t>(std_map_section_id);
#endif
			std_scroll_speed++;
			if(
				(offset >= source->end) ||
				(reinterpret_cast<uint16_t>(std_scroll_speed) >= source->end)
			) {
				replay_ck_failure_group_value = 0xE1;
				replay_ck_failure_field_value = offset;
				return false;
			}
			scroll_speed.v = rck_std_source_u8(
				reinterpret_cast<uint16_t>(std_scroll_speed)
			);
			if(scroll_speed.v == 0) {
				scroll_line = 0;
				scroll_lines_prev_frame = 0;
				scroll_lines_pending = 0;
				return true;
			}
		}
		if(!rck_practice_map_row_fill()) {
			replay_ck_failure_group_value = 0xE2;
			replay_ck_failure_field_value = static_cast<uint16_t>(
				#if (GAME == 5)
					static_cast<uint16_t>(std_map_section_p)
				#else
					std_map_section_id
				#endif
			);
			return false;
		}
	}
	lines_last_frame = scroll_lines_prev_frame;
	scroll_lines_prev_frame = scroll_lines_pending;
	scroll_lines_pending += lines_last_frame;
	// The skipped scroll is treated as already rendered on both pages.
	scroll_lines_pending = 0;
	return true;
}

#if (GAME == 5)
static void rck_practice_stage2_state(uint16_t target_frame)
{
	uint16_t frame;

	if(stage_id != 1) {
		return;
	}
	s2particles_spawned = 0;
	stage2_flash_tone = 0;
	stage2_bg_pulse = 0;
	stage2_bg_pulse_direction = 0;
	for(frame = 332; frame < target_frame; frame += 4) {
		if(stage2_bg_pulse_direction == 0) {
			stage2_bg_pulse++;
			if(stage2_bg_pulse >= 0x3F) {
				stage2_bg_pulse_direction = 1;
			}
		} else {
			stage2_bg_pulse--;
			if(stage2_bg_pulse <= 0x20) {
				stage2_bg_pulse_direction = 0;
			}
		}
	}
	PaletteTone = 100;
	Palettes[0].c.r = (stage2_bg_pulse * 2);
	Palettes[0].c.g = (stage2_bg_pulse * 2);
	Palettes[0].c.b = (stage2_bg_pulse * 4);
	palette_changed = true;
}
#else
static void rck_practice_stage4_state_step(uint16_t frame)
{
	int y;
	int x;
	uint8_t target_level;

	if(stage_id != 3) {
		return;
	}
	if(frame == 0) {
		carpet_lighting_cel = 0;
		carpet_light_level = 0;
	}
	y = (scroll_line >> TILE_BITS_H);
	for(x = 0; x < TILES_X; x++) {
		if(CARPET_LIGHTING_ANIM[carpet_lighting_cel][x] == 2) {
			tile_ring[y][x] = CARPET_TILE_IMAGE_VOS[
				carpet_light_level
			][x];
		}
	}
	if(frame <= 1) {
		for(y = 0; y < TILES_Y; y++) {
			for(x = 0; x < TILES_X; x++) {
				tile_ring[y][x] = CARPET_TILE_IMAGE_VOS[0][x];
			}
		}
		return;
	}
	if((carpet_light_level == 0) && (frame < 1664)) {
		return;
	}
	if(carpet_light_level >= 2) {
		stage_render = nullfunc_near;
		return;
	}
	target_level = static_cast<uint8_t>(carpet_light_level + 1);
	for(x = 0; x < TILES_X; x++) {
		if(CARPET_LIGHTING_ANIM[carpet_lighting_cel][x] == 1) {
			for(y = 0; y < TILES_Y; y++) {
				tile_ring[y][x] = CARPET_TILE_IMAGE_VOS[target_level][x];
			}
		}
	}
	if((frame & 3) == 0) {
		carpet_lighting_cel++;
		if(carpet_lighting_cel >= 8) {
			carpet_light_level++;
			carpet_lighting_cel = 0;
		}
	}
}

static void rck_practice_stage5_stars(uint16_t target_frame)
{
	uint16_t initial_y;
	int i;

	if(stage_id != 4) {
		return;
	}
	for(i = 0; i < STAGE5_STAR_COUNT; i++) {
		switch(i) {
		case 0: initial_y = 320; break;
		case 1: initial_y = 40; break;
		default: initial_y = 190; break;
		}
		stage5_star_center_y[i].v = static_cast<subpixel_t>(
			((initial_y + ((target_frame % 100) * 4)) % RES_Y) <<
			SUBPIXEL_BITS
		);
	}
}

static void rck_practice_stage4_midboss_rearm(uint16_t target_frame)
{
	if((stage_id != 3) || (target_frame < 3400)) {
		return;
	}
	midboss.frames_until = 5600;
	midboss.pos.cur.set(240, -32);
	midboss.pos.prev.set(240, -32);
	midboss.pos.velocity.set(-4, 2);
	midboss.hp = 1200;
	midboss.sprite = 0;
	midboss.phase = 0;
	midboss.phase_frame = 0;
	midboss.damage_this_frame = 0;
	midboss.angle = 0;
	midboss_active = false;
	midboss4_pattern = 0;
	midboss4_passes = 0;
	midboss4_22B9E = 1;
	midboss4_255C8 = 0;
}
#endif

#define RCK_PRACTICE_BOSS_CONSTRUCT_FRAME_MAX 60000UL

static void rck_practice_stage_frame_advance(void)
{
	stage_frame++;
	frames_unused = stage_frame;
	stage_frame_mod2 = static_cast<uint8_t>(stage_frame & 1);
	stage_frame_mod4 = static_cast<uint8_t>(stage_frame & 3);
	stage_frame_mod8 = static_cast<uint8_t>(stage_frame & 7);
	stage_frame_mod16 = static_cast<uint8_t>(stage_frame & 15);
}

static void rck_practice_randring_restore(void)
{
	int i = (RANDRING_SIZE - 1);

	resident->rand = rck_practice_boss_resident_rand;
	random_seed = rck_practice_boss_random_seed;
	do {
		randring[i] = irand();
	} while(--i >= 0);
	randring_p = 0;
}

static void rck_practice_bytes_clear(void near *dst, unsigned size)
{
	uint8_t near *p = static_cast<uint8_t near *>(dst);

	while(size != 0) {
		*p++ = 0;
		size--;
	}
}

static void rck_practice_boss_transients_clear(void)
{
	rck_practice_bytes_clear(shots, sizeof(shots));
	rck_practice_bytes_clear(enemies, sizeof(enemies));
	rck_practice_bytes_clear(sparks, sizeof(sparks));
	rck_practice_bytes_clear(bullets, sizeof(bullets));
	rck_practice_bytes_clear(circles, sizeof(circles));
	rck_practice_bytes_clear(items, sizeof(items));
	rck_practice_bytes_clear(pointnums, sizeof(pointnums));
	rck_practice_bytes_clear(gather_circles, sizeof(gather_circles));
#if (GAME == 5)
	rck_practice_bytes_clear(hitshots, sizeof(hitshots));
#endif
	explosions_small_reset();
	explosions_big.alive = false;
	bullet_zap.active = false;
	playfield_shake_x = 0;
	playfield_shake_y = 0;
	playfield_shake_redraw_time = 0;
	boss.damage_this_frame = 0;
	player_is_hit = false;
	score_delta = 0;
	score_delta_frame = 0;
}

#if (GAME == 5)
static void rck_practice_th05_stage4_solo_prepare(uint8_t section)
{
	rck_practice_bytes_clear(custom_entities, sizeof(custom_entities));
	rck_practice_bytes_clear(cheeto_trails, sizeof(cheeto_trails));
	boss_custombullets_render = nullfunc_near;
	if(section == RCS_TH05_YUKI) {
		boss.pos.cur = boss2.pos.cur;
		boss_update = yuki_update;
	} else {
		boss_update = mai_update;
	}
	boss.phase = PHASE_HP_FILL;
	boss.phase_frame = 0;
	boss_fg_render = b4_solo_fg_render;
	boss.hp = 7900;
}
#else
static void rck_practice_th04_gengetsu_prepare(void)
{
	char bg_fn[12];
	char bb_fn[9];

	bg_fn[0] = 's'; bg_fn[1] = 't'; bg_fn[2] = '0'; bg_fn[3] = '6';
	bg_fn[4] = 'b'; bg_fn[5] = 'k'; bg_fn[6] = '2'; bg_fn[7] = '.';
	bg_fn[8] = 'c'; bg_fn[9] = 'd'; bg_fn[10] = 'g'; bg_fn[11] = '\0';

	bb_fn[0] = 's'; bb_fn[1] = 't'; bb_fn[2] = '0'; bb_fn[3] = '6';
	bb_fn[4] = 'b'; bb_fn[5] = '.'; bb_fn[6] = 'b'; bb_fn[7] = 'b';
	bb_fn[8] = '\0';

	boss_statebyte[0] = true;
	boss_update = nullfunc_far;
	boss_fg_render = nullfunc_near;
	boss.phase = PHASE_HP_FILL;
	boss.mode = 0;
	boss.phase_state.patterns_seen = 0;
	boss.phase_frame = 0;
	boss.pos.velocity.set(0, 0);
	boss.damage_this_frame = 0;
	explosions_small_reset();
	boss_phase_timed_out = true;
	boss.pos.init((PLAYFIELD_W / 2), (playfield_fraction_y(6 / 23.0f)));
	mugetsu_pose_func = mugetsu_1821E;
	mugetsu_gather_frame_offset = -0x50;
	mugetsu_gather_center = boss.pos.cur;
	mugetsu_phase2_mode = 36;
	mugetsu_damage_frames = 0;
	gengetsu_damage_frames = 0;
	extra_boss_bomb_immunity = 0;
	gengetsu_wave_amp = 0;
	gengetsu_wave_target_x.v = TO_SP(PLAYFIELD_W / 2);
	bg_render_not_bombing = mugetsu_gengetsu_bg_render;
	boss_update = gengetsu_update;
	boss_fg_render = gengetsu_fg_render;
	boss.sprite = PAT_GENGETSU_TIPPING;
	boss_hitbox_radius.set((GENGETSU_W / 4), (GENGETSU_H / 2));
	cdg_free(CDG_BG_BOSS);
	bb_boss_free();
	cdg_load_single_noalpha(CDG_BG_BOSS, bg_fn, 0);
	file_ropen(bb_fn);
	bb_boss_seg = HMem<bb_tiles8_t>::alloc(BB_SIZE);
	file_read(bb_boss_seg, BB_SIZE);
	file_close();
	bombing_disabled = false;
}
#endif

static bool rck_practice_boss_target_reached(void)
{
	replay_ck_actor_probe_t probe;

	return (
		replay_ck_actor_probe(&probe) &&
		(probe.boss_section == rck_practice_boss_target_section) &&
		(probe.boss_phase == rck_practice_boss_target_phase)
	);
}

static bool rck_practice_boss_construct(
	const replay_start_config_t far *start
)
{
	unsigned char se_mode = snd_se_mode;
	uint32_t frames = 0;

	rck_practice_boss_resident_rand = start->resident_rand;
	rck_practice_boss_random_seed = start->random_seed;
	rck_practice_boss_target_section = start->section;
	rck_practice_boss_target_phase = start->phase;

	stage_vm = nullfunc_far;
	std_update = std_update_done;
	midboss_active = false;
	midboss_invalidate = nullfunc_near;
	midboss_update = nullfunc_far;
	midboss_render = nullfunc_near;
	bg_render_not_bombing = boss_bg_render_func;
	boss_update = boss_update_func;
	boss_fg_render = boss_fg_render_func;
	snd_se_mode = SND_SE_OFF;

#if (GAME == 4)
	if(
		(start->stage == STAGE_EXTRA) &&
		(start->section == RCS_TH04_GENGETSU)
	) {
		rck_practice_th04_gengetsu_prepare();
	}
#endif

	while(!rck_practice_boss_target_reached()) {
		if(frames++ >= RCK_PRACTICE_BOSS_CONSTRUCT_FRAME_MAX) {
			replay_ck_failure_group_value = rck_practice_boss_target_section;
			replay_ck_failure_field_value = static_cast<uint16_t>(
				(static_cast<uint16_t>(boss.phase) << 8) | boss.mode
			);
			snd_se_mode = se_mode;
			return false;
		}
#if (GAME == 5)
		if(
			(stage_id == 3) &&
			(rck_practice_boss_target_section != RCS_TH05_PAIR)
		) {
			if((boss_update == boss_update_func) &&
			   (boss.phase == PHASE_NONE)) {
				rck_practice_th05_stage4_solo_prepare(
					rck_practice_boss_target_section
				);
				continue;
			}
			if((boss_update == boss_update_func) && (boss.phase == 2)) {
				if(rck_practice_boss_target_section == RCS_TH05_MAI) {
					boss.hp = static_cast<int>(boss.phase_end_hp + 1);
					boss2.hp = boss2.phase_end_hp;
				} else {
					boss.hp = boss.phase_end_hp;
					boss2.hp = static_cast<int>(boss2.phase_end_hp + 1);
				}
			} else if(boss_update != boss_update_func) {
				boss.hp = boss.phase_end_hp;
			}
		} else {
			boss.hp = boss.phase_end_hp;
		}
#else
		boss.hp = boss.phase_end_hp;
#endif
		boss_update();
		rck_practice_stage_frame_advance();
		if(quit != Q_KEEP_RUNNING) {
			snd_se_mode = se_mode;
			return false;
		}
	}

	// Boss updates can temporarily replace live render callbacks while moving
	// between entrance and attack phases. A direct start begins after that
	// transition, so normalize the callbacks to the stage setup before the
	// checkpoint codec validates them and before either VRAM page is primed.
	bg_render_not_bombing = boss_bg_render_func;
	#if (GAME == 5)
		if(
			(stage_id != 3) ||
			(rck_practice_boss_target_section == RCS_TH05_PAIR)
		) {
			boss_update = boss_update_func;
			boss_fg_render = boss_fg_render_func;
		}
	#else
		boss_update = boss_update_func;
		boss_fg_render = boss_fg_render_func;
	#endif
	rck_practice_boss_transients_clear();
	rck_practice_randring_restore();
	snd_se_mode = se_mode;
	return true;
}

#undef RCK_PRACTICE_BOSS_CONSTRUCT_FRAME_MAX

bool replay_ck_practice_boss_direct_supported(
	const replay_start_config_t far *start
)
{
	if((start == 0) || (start->kind != RSK_BOSS_PHASE)) {
		return false;
	}
#if (GAME == 4)
	return true;
#else
	return true;
#endif
}

bool replay_ck_practice_direct_seek(
	const replay_start_config_t far *start
)
{
	rck_std_source_t source;
	uint32_t event_offset;
	uint16_t target_frame;
	uint16_t frame;
	bool boss_target = false;

	if((start == 0) || (start->stage != stage_id)) {
		return false;
	}
	if(start->kind == RSK_CHAPTER) {
		if(!replay_practice_chapter_valid(start->stage, start->section)) {
			return false;
		}
		target_frame = replay_practice_chapter_frame(
			start->stage, start->section
		);
	} else if(start->kind == RSK_MIDBOSS) {
		if(!replay_practice_midboss_valid(start->stage, start->section)) {
			return false;
		}
		target_frame = replay_practice_midboss_frame(
			start->stage, start->section
		);
	} else if(start->kind == RSK_BOSS_PHASE) {
		if(!replay_checkpoint_identity_valid(start)) {
			return false;
		}
		target_frame = 1;
		boss_target = true;
	} else {
		return false;
	}
	if((target_frame == 0) || !rck_std_source_info(&source)) {
		return false;
	}
	event_offset = source.ip_initial;
	while(event_offset < source.ip_terminal) {
		uint16_t event_frame = rck_std_source_u16(
			static_cast<uint16_t>(event_offset)
		);
		uint8_t spawn_count;

		if(!boss_target && (event_frame >= target_frame)) {
			break;
		}
		if(boss_target && (event_frame >= target_frame)) {
			target_frame = static_cast<uint16_t>(event_frame + 1);
		}
		spawn_count = rck_std_source_u8(
			static_cast<uint16_t>(event_offset + 2)
		);
		event_offset += (
			3 + (static_cast<uint32_t>(spawn_count) * STD_ENEMY_SPAWN_SIZE)
		);
	}
	if(event_offset > source.ip_terminal) {
		return false;
	}
	if(boss_target) {
		event_offset = source.ip_terminal;
	}
	std_ip = MK_FP(
		reinterpret_cast<uint16_t>(std_seg),
		static_cast<uint16_t>(event_offset)
	);
	stage_vm = ((event_offset == source.ip_terminal) ? nullfunc_far : std_run);

	frame = 0;
	while((frame < target_frame) || (boss_target && (scroll_speed.v != 0))) {
		if(frame == 0xFFFFu) {
			return false;
		}
#if (GAME == 5)
		if(stage_id == 5) {
			scroll_active = false;
		}
		if((stage_id == 1) && (frame < 304)) {
			scroll_active = false;
		} else if((stage_id == 1) && (frame < 306)) {
			int y;
			int x;
			for(y = 0; y < TILES_Y; y++) {
				for(x = 0; x < TILES_X; x++) {
					tile_ring[y][x] = TILE_AREA_VRAM_LEFT;
				}
			}
			scroll_active = true;
		}
#endif
	#if (GAME == 4)
		rck_practice_stage4_state_step(frame);
	#endif
		if(!rck_practice_scroll_step(&source)) {
			return false;
		}
		frame++;
	}
	target_frame = frame;
	stage_frame = target_frame;
	frames_unused = target_frame;
	stage_frame_mod2 = static_cast<uint8_t>(target_frame & 1);
	stage_frame_mod4 = static_cast<uint8_t>(target_frame & 3);
	stage_frame_mod8 = static_cast<uint8_t>(target_frame & 7);
	stage_frame_mod16 = static_cast<uint8_t>(target_frame & 15);
	scroll_line_on_page[0] = scroll_line;
	scroll_line_on_page[1] = scroll_line;
	scroll_lines_pending = 0;
	scroll_lines_prev_frame = 0;
#if (GAME == 5)
	rck_practice_stage2_state(target_frame);
#else
	rck_practice_stage5_stars(target_frame);
	rck_practice_stage4_midboss_rearm(target_frame);
#endif
	if(boss_target && !rck_practice_boss_construct(start)) {
		return false;
	}
	if(boss_target) {
		#if (GAME == 5)
		// Direct boss constructors install their own backdrop renderer. Prime
		// both VRAM pages before the stage loop unmasks the target; otherwise
		// one page still contains the pre-boss tile map and alternates into
		// view as persistent trails.
		graph_accesspage(page_front);
		bg_render_not_bombing();
		graph_accesspage(page_back);
		bg_render_not_bombing();
		#endif
	} else {
		tiles_activate_and_render_all_for_next_N_frames(PAGE_COUNT);
	}
	return true;
}

static bool rck_group_stage_vm(replay_ck_stream_t far *stream)
{
	bool loaded = (std_seg != 0);
	bool live_loaded = loaded;
	uint8_t stage_vm_id = rck_stage_vm_id();
	rck_std_source_t source;
	uint16_t source_end;
	uint32_t source_checksum;
	uint16_t map_offset;
	uint16_t scroll_offset;
	uint16_t script_offsets[STD_ENEMY_SCRIPT_COUNT];
	uint32_t ip_offset;
	int i;

	RCK_BOOL(loaded);
	if(loaded != live_loaded) {
		return false;
	}
	if(!loaded) {
		if(
			!rck_u8(stream, &stage_vm_id) ||
			(stage_vm_id != RCKSVM_NONE)
		) {
			return false;
		}
		if(rck_applying(stream)) {
			stage_vm = nullfunc_far;
		}
		return true;
	}
	if(!rck_std_source_info(&source)) {
		return false;
	}
	source_end = source.end;
	source_checksum = source.checksum;
	if(
		!rck_u16(stream, &source_end) ||
		!rck_u32(stream, &source_checksum) ||
		(source_end != source.end) ||
		(source_checksum != source.checksum)
	) {
		return false;
	}
#if (GAME == 5)
	map_offset = static_cast<uint16_t>(std_map_section_p);
#else
	map_offset = static_cast<uint16_t>(std_map_section_id);
#endif
	scroll_offset = rck_std_near_offset(std_scroll_speed);
	if(
		!rck_u16(stream, &map_offset) ||
		(map_offset >= source.end) ||
		!rck_u16(stream, &scroll_offset) ||
		((scroll_offset != RCK_STD_OFFSET_NONE) &&
		 (scroll_offset >= source.end))
	) {
		return false;
	}
	for(i = 0; i < STD_ENEMY_SCRIPT_COUNT; i++) {
		script_offsets[i] = rck_std_near_offset(std_enemy_scripts[i]);
		if(!rck_u16(stream, &script_offsets[i])) {
			return false;
		}
		if(
			((script_offsets[i] != RCK_STD_OFFSET_NONE) &&
			 (script_offsets[i] >= source.end)) ||
			((i < source.script_count) &&
			 (script_offsets[i] != rck_std_near_offset(std_enemy_scripts[i])))
		) {
			return false;
		}
	}
	ip_offset = rck_std_ip_offset();
	if(
		!rck_u32(stream, &ip_offset) ||
		!rck_std_ip_valid(&source, ip_offset) ||
		!rck_u8(stream, &stage_vm_id) ||
		(rck_stage_vm_func(stage_vm_id) == 0) ||
		((stage_vm_id == RCKSVM_RUN) &&
		 (ip_offset == source.ip_terminal))
	) {
		return false;
	}
	if(rck_applying(stream)) {
#if (GAME == 5)
		std_map_section_p = map_offset;
#else
		std_map_section_id = map_offset;
#endif
		std_scroll_speed = (
			(scroll_offset == RCK_STD_OFFSET_NONE)
				? 0
				: reinterpret_cast<SubpixelLength8 near *>(scroll_offset)
		);
		for(i = 0; i < STD_ENEMY_SCRIPT_COUNT; i++) {
			std_enemy_scripts[i] = (
				(script_offsets[i] == RCK_STD_OFFSET_NONE)
					? 0
					: reinterpret_cast<void near *>(script_offsets[i])
			);
		}
		std_ip = MK_FP(
			reinterpret_cast<uint16_t>(std_seg),
			static_cast<uint16_t>(ip_offset)
		);
		stage_vm = rck_stage_vm_func(stage_vm_id);
	}
	return true;
}

#if (GAME == 5)
#define RCK_DIALOG_OP_06_SIZE 6
#define RCK_DIALOG_CURSOR_X_MAX (RES_X / GLYPH_HALF_W)
#define RCK_DIALOG_CURSOR_Y_MAX (RES_Y / GLYPH_H)

struct rck_dialog_source_t {
	uint16_t end;
	uint32_t checksum;
};

static uint8_t rck_dialog_source_u8(uint16_t segment, uint16_t offset)
{
	return *reinterpret_cast<uint8_t far *>(MK_FP(segment, offset));
}

static bool rck_dialog_skip_string(uint16_t segment, uint32_t *cursor)
{
	while(*cursor <= 0xFFFFUL) {
		if(rck_dialog_source_u8(segment, static_cast<uint16_t>(*cursor)) == 0) {
			(*cursor)++;
			return (*cursor <= 0xFFFFUL);
		}
		(*cursor)++;
	}
	return false;
}

static bool rck_dialog_skip_operation(
	uint16_t segment,
	uint32_t *cursor,
	uint8_t operation,
	bool *allowed_in_box
)
{
	uint8_t parameter_size = 0;

	*allowed_in_box = false;
	switch(operation) {
	case 0x02:
		parameter_size = 1;
		break;
	case 0x03:
	case 0x05:
		return rck_dialog_skip_string(segment, cursor);
	case 0x06:
		parameter_size = RCK_DIALOG_OP_06_SIZE;
		break;
	case 0x09:
	case 0x0A:
		parameter_size = 1;
		break;
	case 0x0B:
	case 0x0D:
		*allowed_in_box = true;
		break;
	case 0x0C:
		parameter_size = 1;
		*allowed_in_box = true;
		break;
	}
	*cursor += parameter_size;
	return (*cursor <= 0xFFFFUL);
}

static bool rck_dialog_sequence_end(uint16_t segment, uint32_t *cursor)
{
	while(*cursor <= 0xFFFFUL) {
		uint8_t operation = rck_dialog_source_u8(
			segment, static_cast<uint16_t>(*cursor)
		);
		bool allowed_in_box;

		(*cursor)++;
		if(operation == 0xFF) {
			return true;
		}
		if(operation != 0x0D) {
			if(!rck_dialog_skip_operation(
				segment, cursor, operation, &allowed_in_box
			)) {
				return false;
			}
			continue;
		}

		while(*cursor <= 0xFFFFUL) {
			operation = rck_dialog_source_u8(
				segment, static_cast<uint16_t>(*cursor)
			);
			(*cursor)++;
			if(operation == 0xFF) {
				break;
			}
			if(!rck_dialog_skip_operation(
				segment, cursor, operation, &allowed_in_box
			)) {
				return false;
			}
			if(!allowed_in_box) {
				(*cursor)++;
				if(*cursor > 0xFFFFUL) {
					return false;
				}
			}
		}
	}
	return false;
}

static bool rck_dialog_source_info(
	uint16_t segment, rck_dialog_source_t *source
)
{
	uint32_t cursor = 0;
	uint8_t sequence_count = ((stage_id == 5) ? 1 : 2);
	uint32_t i;

	for(i = 0; i < sequence_count; i++) {
		if(!rck_dialog_sequence_end(segment, &cursor)) {
			return false;
		}
	}
	if((cursor == 0) || (cursor > 0xFFFFUL)) {
		return false;
	}
	source->end = static_cast<uint16_t>(cursor);
	source->checksum = REPLAY_FNV1A_BASIS;
	for(i = 0; i < source->end; i++) {
		source->checksum ^= rck_dialog_source_u8(
			segment, static_cast<uint16_t>(i)
		);
		source->checksum *= REPLAY_FNV1A_PRIME;
	}
	return true;
}

static bool rck_group_dialog(replay_ck_stream_t far *stream)
{
	bool loaded = (dialog_p != 0);
	bool live_loaded = loaded;
	uint16_t source_segment = (loaded ? FP_SEG(dialog_p) : 0);
	uint16_t cursor_offset = (loaded ? FP_OFF(dialog_p) : 0);
	rck_dialog_source_t source;
	uint16_t source_end;
	uint32_t source_checksum;
	uint16_t cursor_x = static_cast<uint16_t>(dialog_cursor.x);
	uint16_t cursor_y = static_cast<uint16_t>(dialog_cursor.y);
	uint8_t side = static_cast<uint8_t>(dialog_side);
	uint8_t sequence_id = static_cast<uint8_t>(dialog_sequence_id);

	RCK_BOOL(loaded);
	if(loaded != live_loaded) {
		return false;
	}
	if(loaded) {
		if(!rck_dialog_source_info(source_segment, &source)) {
			return false;
		}
		source_end = source.end;
		source_checksum = source.checksum;
		if(
			!rck_u16(stream, &source_end) ||
			!rck_u32(stream, &source_checksum) ||
			(source_end != source.end) ||
			(source_checksum != source.checksum) ||
			!rck_u16(stream, &cursor_offset) ||
			(cursor_offset > source.end)
		) {
			return false;
		}
	}
	if(!rck_u16(stream, &cursor_x) || !rck_u16(stream, &cursor_y)) {
		return false;
	}
	if(
		(static_cast<int16_t>(cursor_x) < 0) ||
		(static_cast<int16_t>(cursor_x) > RCK_DIALOG_CURSOR_X_MAX) ||
		(static_cast<int16_t>(cursor_y) < 0) ||
		(static_cast<int16_t>(cursor_y) > RCK_DIALOG_CURSOR_Y_MAX) ||
		!rck_u8(stream, &side) ||
		(side > 1) ||
		!rck_u8(stream, &sequence_id) ||
		(sequence_id > 1)
	) {
		return false;
	}
	if(rck_applying(stream)) {
		if(loaded) {
			dialog_p = reinterpret_cast<uint8_t far *>(MK_FP(
				source_segment, cursor_offset
			));
		}
		dialog_cursor.x = static_cast<int16_t>(cursor_x);
		dialog_cursor.y = static_cast<int16_t>(cursor_y);
		dialog_side = side;
		dialog_sequence_id = sequence_id;
	}
	return true;
}
#endif

enum rck_std_update_t {
	RCKSU_DIALOG = 0,
	RCKSU_DONE = 1,
};

static uint8_t rck_std_update_id(void)
{
	if(std_update ==
		std_update_frames_then_animate_dialog_and_activate_boss_if_done
	) {
		return RCKSU_DIALOG;
	}
	if(std_update == std_update_done) {
		return RCKSU_DONE;
	}
	return 0xFF;
}

static bool (near* rck_std_update_func(uint8_t id))(void)
{
	switch(id) {
	case RCKSU_DIALOG:
		return std_update_frames_then_animate_dialog_and_activate_boss_if_done;
	case RCKSU_DONE:
		return std_update_done;
	}
	return 0;
}

static bool rck_group_pacing(replay_ck_stream_t far *stream)
{
	uint16_t factor = slowdown_factor;
	uint8_t std_update_id = rck_std_update_id();

	RCK_BOOL(turbo_mode);
	RCK_BOOL(resident->turbo_mode);
	if(
		!rck_u16(stream, &factor) ||
		(factor < 1) || (factor > 2)
	) {
		return false;
	}
	if(rck_applying(stream)) {
		slowdown_factor = factor;
	}
#if (GAME == 5)
	RCK_BOOL(slowdown_caused_by_bullets);
#endif
	if(
		!rck_u8(stream, &std_update_id) ||
		(rck_std_update_func(std_update_id) == 0)
	) {
		return false;
	}
	if(rck_applying(stream)) {
		std_update = rck_std_update_func(std_update_id);
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

	replay_ck_failure_field_value = 0x1000;
	if(!rck_sppoint(stream, &homing_target)) {
		return false;
	}

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
	replay_ck_failure_field_value = 0x2000;
	RCK_BOOL(player_is_hit);
	RCK_U8(player_invincibility_time);
	RCK_U8(miss_time);
	RCK_U8(miss_move_lock_time);
	replay_ck_failure_field_value = 0x2100;
	RCK_U8_RANGE(power, 1, POWER_MAX);
	RCK_S16(power_overflow);
	replay_ck_failure_field_value = static_cast<uint16_t>(0x2200 | shot_level);
	RCK_U8_MAX(shot_level, RCK_SHOT_LEVEL_COUNT - 1);
	RCK_U8(shot_time);
	replay_ck_failure_field_value = 0x2300;
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
	replay_ck_failure_field_value = 0x3000;
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
	replay_ck_failure_field_value = 0x4000;
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
	replay_ck_failure_field_value = 0x5000;
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
		replay_ck_failure_field_value = 0x6000;
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
	replay_ck_failure_field_value = 0x7000;
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
		replay_ck_failure_field_value = static_cast<uint16_t>(0x8000 | i);
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

	replay_ck_failure_field_value = 0x9000;
	if(!rck_bomb_anim(stream, variant)) {
		return false;
	}

	return true;
}

static bool rck_bullet(
	replay_ck_stream_t far *stream, bullet_t near *bullet
)
{
	uint16_t index = (replay_ck_failure_field_value & 0x0FFF);
	bool active;
	uint8_t spawn_flag = static_cast<uint8_t>(bullet->spawn_flag);
	uint8_t move_flag = static_cast<uint8_t>(bullet->move_flag);

	replay_ck_failure_field_value = (0x1000 | index);
	if(!rck_entity_flag(stream, &bullet->flag)) {
		return false;
	}
	active = (bullet->flag != F_FREE);
	RCK_S8(bullet->age);
	replay_ck_failure_field_value = (0x2000 | index);
	if(!rck_motion(stream, &bullet->pos)) {
		return false;
	}
	RCK_U8(bullet->from_group);
	RCK_S8(bullet->unused);
	RCK_U8(bullet->speed_cur.v);
	RCK_U8(bullet->angle);
	replay_ck_failure_field_value = (0x3000 | index);
	if(!rck_u8(stream, &spawn_flag) || !rck_u8(stream, &move_flag)) {
		return false;
	}
	if(active &&
		((spawn_flag > BSF_CLOUD_END) || (move_flag > BMF_DECAY_END))
	) {
		return false;
	}
	replay_ck_failure_field_value = (0x4000 | index);
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
		replay_ck_failure_field_value = static_cast<uint16_t>(0x1000 + i);
		if(!rck_bullet(stream, &bullets[i])) {
			return false;
		}
	}
	RCK_U8(bullet_special.turns_max);
	RCK_S8(bullet_template_special_angle.v);
	replay_ck_failure_field_value = 0x2000;
	if(!rck_bullet_template(stream, &bullet_template, true)) {
		return false;
	}
	replay_ck_failure_field_value = 0x2100;
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

		replay_ck_failure_field_value = 0x2200;
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
		replay_ck_failure_field_value = static_cast<uint16_t>(0x3000 + i);
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
	replay_ck_failure_field_value = 0x3F00;
	if(!rck_thicklaser(stream, &thicklaser_template)) {
		return false;
	}
	for(i = 0; i < THICKLASER_COUNT; i++) {
		replay_ck_failure_field_value = static_cast<uint16_t>(0x4000 + i);
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
	// The update loop keeps each of the three kill cels for four frames.
	// EF_KILL_ANIM_last is unused and does not describe that live range.
	if(
		(value > EF_ALIVE_FIRST_FRAME) &&
		((value < EF_KILL_ANIM) ||
		 (value >= (EF_KILL_ANIM + (ENEMY_KILL_CELS * 4))))
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
	uint16_t index = (replay_ck_failure_field_value & 0x00FF);
	uint8_t script_id = rck_enemy_script_id(enemy->script);
	uint8_t item = static_cast<uint8_t>(enemy->item);
	bool running;

	replay_ck_failure_field_value = (0x1000 | index);
	if(!rck_enemy_flag(stream, &enemy->flag)) {
		return false;
	}
	running = (
		(enemy->flag == EF_ALIVE) ||
		(enemy->flag == EF_ALIVE_FIRST_FRAME)
	);
	if(!running && !rck_applying(stream)) {
		script_id = 0xFF;
	}
	RCK_U8(enemy->age);
	replay_ck_failure_field_value = (0x2000 | index);
	if(!rck_motion(stream, &enemy->pos)) {
		return false;
	}
	RCK_S16(enemy->hp);
	RCK_S16(enemy->score);
	replay_ck_failure_field_value = (0x3000 | index);
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
	replay_ck_failure_field_value = (0x4000 | index);
	if(
		!rck_u8(stream, &item) ||
		(running && !rck_item_type_valid(item))
	) {
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
	replay_ck_failure_field_value = (0x5000 | index);
	if(!rck_bullet_template(stream, &enemy->bullet_template, running)) {
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
		replay_ck_failure_field_value = static_cast<uint16_t>(i);
		if(!rck_enemy(stream, &enemies[i])) {
			return false;
		}
	}
	replay_ck_failure_field_value = 0x6000;
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
	RCK_BOOL16(item->pulled_to_player);
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
	if(!rck_bullet_template(
		stream, &gather->bullet_template, (gather->flag != F_FREE)
	)) {
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

static bool rck_midboss_record(
	replay_ck_stream_t far *stream, midboss_stuff_t far *actor
)
{
	if(!rck_motion(stream, &actor->pos)) {
		return false;
	}
	RCK_U16(actor->frames_until);
	RCK_S16(actor->hp);
	RCK_U8(actor->sprite);
	RCK_U8(actor->phase);
	RCK_S16(actor->phase_frame);
	RCK_U8(actor->damage_this_frame);
	RCK_U8(actor->angle);
	return true;
}

static bool rck_boss_record(
	replay_ck_stream_t far *stream, boss_stuff_t far *actor
)
{
	if(!rck_motion(stream, &actor->pos)) {
		return false;
	}
	RCK_S16(actor->hp);
	RCK_U8(actor->sprite);
	RCK_U8(actor->phase);
	RCK_S16(actor->phase_frame);
	RCK_U8(actor->damage_this_frame);
	RCK_U8(actor->mode);
	RCK_U8(actor->angle);
	RCK_U8(actor->phase_state.patterns_seen);
	RCK_S16(actor->phase_end_hp);
	return true;
}

static bool rck_explosion(
	replay_ck_stream_t far *stream, Explosion far *explosion
)
{
	RCK_BOOL(explosion->alive);
	RCK_U8(explosion->age);
	if(
		!rck_sppoint(stream, &explosion->center) ||
		!rck_sppoint(stream, &explosion->radius_cur) ||
		!rck_sppoint(stream, &explosion->radius_delta)
	) {
		return false;
	}
	RCK_U8(explosion->angle_offset);
	return true;
}

// Canonical actor storage shared by both games. Each game combines this prefix
// with a closed callback registry and stage-local program below.
static bool rck_group_actors_common(replay_ck_stream_t far *stream)
{
	int i;

	if(!rck_midboss_record(stream, &midboss)) {
		return false;
	}
	RCK_BOOL(midboss_active);
	RCK_U8(midboss_defeat_angle);

	if(!rck_boss_record(stream, &boss)) {
		return false;
	}
	for(i = 0; i < 16; i++) {
		RCK_U8(boss_statebyte[i]);
	}
	if(!rck_sppoint(stream, &boss_hitbox_radius)) {
		return false;
	}
	RCK_BOOL(boss_phase_timed_out);

#if (GAME == 5)
	if(!rck_boss_record(stream, &boss2)) {
		return false;
	}
	RCK_U16(boss_sprite_left);
	RCK_U16(boss_sprite_right);
	RCK_U16(boss_sprite_stay);
	RCK_S16(boss_flystep_random_clamp.left.v);
	RCK_S16(boss_flystep_random_clamp.right.v);
	RCK_S16(boss_flystep_random_clamp.top.v);
	RCK_S16(boss_flystep_random_clamp.bottom.v);
#endif

	for(i = 0; i < EXPLOSION_SMALL_COUNT; i++) {
		if(!rck_explosion(stream, &explosions_small[i])) {
			return false;
		}
	}
	return rck_explosion(stream, &explosions_big);
}

#if (GAME == 4)
enum rck_th04_midboss_setup_t {
	RCK4MS_NONE = 0,
	RCK4MS_STAGE1,
	RCK4MS_STAGE2,
	RCK4MS_STAGE3,
	RCK4MS_STAGE4,
	RCK4MS_EXTRA,
};

enum rck_th04_boss_setup_t {
	RCK4BS_ORANGE = 1,
	RCK4BS_KURUMI,
	RCK4BS_ELLY,
	RCK4BS_REIMU,
	RCK4BS_MARISA,
	RCK4BS_YUUKA5,
	RCK4BS_YUUKA6,
	RCK4BS_EXTRA,
};

enum rck_th04_backdrop_t {
	RCK4BD_ORANGE = 1,
	RCK4BD_KURUMI,
	RCK4BD_ELLY,
	RCK4BD_REIMU_MARISA,
	RCK4BD_YUUKA5,
	RCK4BD_EXTRA,
};

static uint8_t rck_th04_midboss_setup_id(void)
{
	if(
		(midboss_update_func == nullfunc_far) &&
		(midboss_render_func == nullfunc_near)
	) {
		return RCK4MS_NONE;
	}
	if(
		(midboss_update_func == midboss1_update) &&
		(midboss_render_func == midboss1_render)
	) {
		return RCK4MS_STAGE1;
	}
	if(
		(midboss_update_func == midboss2_update) &&
		(midboss_render_func == midboss2_render)
	) {
		return RCK4MS_STAGE2;
	}
	if(
		(midboss_update_func == midboss3_update) &&
		(midboss_render_func == midboss3_render)
	) {
		return RCK4MS_STAGE3;
	}
	if(
		(midboss_update_func == midboss4_update) &&
		(midboss_render_func == midboss4_render)
	) {
		return RCK4MS_STAGE4;
	}
	if(
		(midboss_update_func == midbossx_update) &&
		(midboss_render_func == midbossx_render)
	) {
		return RCK4MS_EXTRA;
	}
	return 0xFF;
}

static bool rck_th04_midboss_setup_apply(uint8_t id)
{
	switch(id) {
	case RCK4MS_NONE:
		midboss_update_func = nullfunc_far;
		midboss_render_func = nullfunc_near;
		break;
	case RCK4MS_STAGE1:
		midboss_update_func = midboss1_update;
		midboss_render_func = midboss1_render;
		break;
	case RCK4MS_STAGE2:
		midboss_update_func = midboss2_update;
		midboss_render_func = midboss2_render;
		break;
	case RCK4MS_STAGE3:
		midboss_update_func = midboss3_update;
		midboss_render_func = midboss3_render;
		break;
	case RCK4MS_STAGE4:
		midboss_update_func = midboss4_update;
		midboss_render_func = midboss4_render;
		break;
	case RCK4MS_EXTRA:
		midboss_update_func = midbossx_update;
		midboss_render_func = midbossx_render;
		break;
	default:
		return false;
	}
	return true;
}

static uint8_t rck_th04_midboss_live_id(void)
{
	if(
		(midboss_invalidate == nullfunc_near) &&
		(midboss_update == nullfunc_far) &&
		(midboss_render == nullfunc_near)
	) {
		return 0;
	}
	if(
		(midboss_invalidate == midboss_invalidate_func) &&
		(midboss_update == midboss_update_func) &&
		(midboss_render == midboss_render_func)
	) {
		return 1;
	}
	return 0xFF;
}

static bool rck_th04_midboss_stage_valid(uint8_t id)
{
	switch(stage_id) {
	case 0: return (id == RCK4MS_STAGE1);
	case 1: return (id == RCK4MS_STAGE2);
	case 2: return (id == RCK4MS_STAGE3);
	case 3: return (id == RCK4MS_STAGE4);
	case 4:
	case 5: return (id == RCK4MS_NONE);
	case 6: return (id == RCK4MS_EXTRA);
	}
	return false;
}

#define RCK4_BOSS_SETUP_CASE(id, bg, update, fg) \
	if( \
		(boss_bg_render_func == bg) && \
		(boss_update_func == update) && \
		(boss_fg_render_func == fg) \
	) { return id; }

static uint8_t rck_th04_boss_setup_id(void)
{
	RCK4_BOSS_SETUP_CASE(
		RCK4BS_ORANGE, orange_bg_render, orange_update, orange_fg_render
	);
	RCK4_BOSS_SETUP_CASE(
		RCK4BS_KURUMI, kurumi_bg_render, kurumi_update, kurumi_fg_render
	);
	RCK4_BOSS_SETUP_CASE(
		RCK4BS_ELLY, elly_bg_render, elly_update, elly_fg_render
	);
	RCK4_BOSS_SETUP_CASE(
		RCK4BS_REIMU, reimu_marisa_bg_render, reimu_update, reimu_fg_render
	);
	RCK4_BOSS_SETUP_CASE(
		RCK4BS_MARISA, reimu_marisa_bg_render, marisa_update, marisa_fg_render
	);
	RCK4_BOSS_SETUP_CASE(
		RCK4BS_YUUKA5, yuuka5_bg_render, yuuka5_update, yuuka5_fg_render
	);
	RCK4_BOSS_SETUP_CASE(
		RCK4BS_YUUKA6, yuuka6_bg_render, yuuka6_update, yuuka6_fg_render
	);
	RCK4_BOSS_SETUP_CASE(
		RCK4BS_EXTRA, mugetsu_gengetsu_bg_render,
		mugetsu_update, mugetsu_fg_render
	);
	return 0xFF;
}

#undef RCK4_BOSS_SETUP_CASE

static bool rck_th04_boss_setup_apply(uint8_t id)
{
	switch(id) {
	case RCK4BS_ORANGE:
		boss_bg_render_func = orange_bg_render;
		boss_update_func = orange_update;
		boss_fg_render_func = orange_fg_render;
		break;
	case RCK4BS_KURUMI:
		boss_bg_render_func = kurumi_bg_render;
		boss_update_func = kurumi_update;
		boss_fg_render_func = kurumi_fg_render;
		break;
	case RCK4BS_ELLY:
		boss_bg_render_func = elly_bg_render;
		boss_update_func = elly_update;
		boss_fg_render_func = elly_fg_render;
		break;
	case RCK4BS_REIMU:
		boss_bg_render_func = reimu_marisa_bg_render;
		boss_update_func = reimu_update;
		boss_fg_render_func = reimu_fg_render;
		break;
	case RCK4BS_MARISA:
		boss_bg_render_func = reimu_marisa_bg_render;
		boss_update_func = marisa_update;
		boss_fg_render_func = marisa_fg_render;
		break;
	case RCK4BS_YUUKA5:
		boss_bg_render_func = yuuka5_bg_render;
		boss_update_func = yuuka5_update;
		boss_fg_render_func = yuuka5_fg_render;
		break;
	case RCK4BS_YUUKA6:
		boss_bg_render_func = yuuka6_bg_render;
		boss_update_func = yuuka6_update;
		boss_fg_render_func = yuuka6_fg_render;
		break;
	case RCK4BS_EXTRA:
		boss_bg_render_func = mugetsu_gengetsu_bg_render;
		boss_update_func = mugetsu_update;
		boss_fg_render_func = mugetsu_fg_render;
		break;
	default:
		return false;
	}
	return true;
}

static bool rck_th04_boss_stage_valid(uint8_t id)
{
	switch(stage_id) {
	case 0: return (id == RCK4BS_ORANGE);
	case 1: return (id == RCK4BS_KURUMI);
	case 2: return (id == RCK4BS_ELLY);
	case 3:
		return (
			((playchar == PLAYCHAR_REIMU) && (id == RCK4BS_MARISA)) ||
			((playchar == PLAYCHAR_MARISA) && (id == RCK4BS_REIMU))
		);
	case 4: return (id == RCK4BS_YUUKA5);
	case 5: return (id == RCK4BS_YUUKA6);
	case 6: return (id == RCK4BS_EXTRA);
	}
	return false;
}

static uint8_t rck_th04_boss_update_id(void)
{
	if(boss_update == nullfunc_far) {
		return 0;
	}
	if(boss_update == boss_update_func) {
		return 1;
	}
	if(boss_update == gengetsu_update) {
		return 2;
	}
	return 0xFF;
}

static uint8_t rck_th04_boss_fg_id(void)
{
	if(boss_fg_render == nullfunc_near) {
		return 0;
	}
	if(boss_fg_render == boss_fg_render_func) {
		return 1;
	}
	if(boss_fg_render == gengetsu_fg_render) {
		return 2;
	}
	return 0xFF;
}

static uint8_t rck_th04_backdrop_id(void)
{
	if(boss_backdrop_colorfill == orange_backdrop_colorfill) {
		return RCK4BD_ORANGE;
	}
	if(boss_backdrop_colorfill == kurumi_backdrop_colorfill) {
		return RCK4BD_KURUMI;
	}
	if(boss_backdrop_colorfill == elly_backdrop_colorfill) {
		return RCK4BD_ELLY;
	}
	if(boss_backdrop_colorfill == reimu_marisa_backdrop_colorfill) {
		return RCK4BD_REIMU_MARISA;
	}
	if(boss_backdrop_colorfill == yuuka5_backdrop_colorfill) {
		return RCK4BD_YUUKA5;
	}
	if(boss_backdrop_colorfill == mugetsu_gengetsu_backdrop_colorfill) {
		return RCK4BD_EXTRA;
	}
	return 0xFF;
}

static bool rck_th04_backdrop_apply(uint8_t id)
{
	switch(id) {
	case RCK4BD_ORANGE:
		boss_backdrop_colorfill = orange_backdrop_colorfill;
		break;
	case RCK4BD_KURUMI:
		boss_backdrop_colorfill = kurumi_backdrop_colorfill;
		break;
	case RCK4BD_ELLY:
		boss_backdrop_colorfill = elly_backdrop_colorfill;
		break;
	case RCK4BD_REIMU_MARISA:
		boss_backdrop_colorfill = reimu_marisa_backdrop_colorfill;
		break;
	case RCK4BD_YUUKA5:
		boss_backdrop_colorfill = yuuka5_backdrop_colorfill;
		break;
	case RCK4BD_EXTRA:
		boss_backdrop_colorfill = mugetsu_gengetsu_backdrop_colorfill;
		break;
	default:
		return false;
	}
	return true;
}

static bool rck_th04_backdrop_stage_valid(uint8_t id)
{
	switch(stage_id) {
	case 0: return (id == RCK4BD_ORANGE);
	case 1: return (id == RCK4BD_KURUMI);
	case 2: return (id == RCK4BD_ELLY);
	case 3: return (id == RCK4BD_REIMU_MARISA);
	case 4:
	case 5: return (id == RCK4BD_YUUKA5);
	case 6: return (id == RCK4BD_EXTRA);
	}
	return false;
}

static bool rck_th04_actor_callbacks(replay_ck_stream_t far *stream)
{
	uint8_t midboss_setup = rck_th04_midboss_setup_id();
	uint8_t midboss_live = rck_th04_midboss_live_id();
	uint8_t boss_setup = rck_th04_boss_setup_id();
	uint8_t boss_update_id = rck_th04_boss_update_id();
	uint8_t boss_fg_id = rck_th04_boss_fg_id();
	uint8_t backdrop = rck_th04_backdrop_id();

	if(
		!rck_u8(stream, &midboss_setup) ||
		!rck_th04_midboss_stage_valid(midboss_setup) ||
		!rck_u8(stream, &midboss_live) || (midboss_live > 1) ||
		((midboss_setup == RCK4MS_NONE) && midboss_live) ||
		!rck_u8(stream, &boss_setup) ||
		!rck_th04_boss_stage_valid(boss_setup) ||
		!rck_u8(stream, &boss_update_id) || (boss_update_id > 2) ||
		((boss_update_id == 2) && (boss_setup != RCK4BS_EXTRA)) ||
		!rck_u8(stream, &boss_fg_id) || (boss_fg_id > 2) ||
		((boss_fg_id == 2) && (boss_setup != RCK4BS_EXTRA)) ||
		!rck_u8(stream, &backdrop) ||
		!rck_th04_backdrop_stage_valid(backdrop)
	) {
		return false;
	}
	if(!rck_applying(stream)) {
		return true;
	}
	if(
		!rck_th04_midboss_setup_apply(midboss_setup) ||
		!rck_th04_boss_setup_apply(boss_setup) ||
		!rck_th04_backdrop_apply(backdrop)
	) {
		return false;
	}
	midboss_invalidate = midboss_live
		? midboss_invalidate_func : nullfunc_near;
	midboss_update = midboss_live ? midboss_update_func : nullfunc_far;
	midboss_render = midboss_live ? midboss_render_func : nullfunc_near;
	boss_update = (boss_update_id == 0)
		? nullfunc_far
		: ((boss_update_id == 1) ? boss_update_func : gengetsu_update);
	boss_fg_render = (boss_fg_id == 0)
		? nullfunc_near
		: ((boss_fg_id == 1) ? boss_fg_render_func : gengetsu_fg_render);
	return true;
}

static bool rck_th04_pattern4(
	replay_ck_stream_t far *stream, uint8_t *pattern
)
{
	uint8_t value = *pattern;

	if(
		!rck_u8(stream, &value) ||
		((value > 3) && (value != 0xFF))
	) {
		return false;
	}
	if(rck_applying(stream)) {
		*pattern = value;
	}
	return true;
}

static bool rck_th04_elly_steering(
	replay_ck_stream_t far *stream, uint8_t *steering
)
{
	uint8_t value = *steering;

	if(!rck_u8(stream, &value)) {
		return false;
	}
	if((value != 0) && (value != 1) && (value != 0xFF)) {
		return false;
	}
	if(rck_applying(stream)) {
		*steering = value;
	}
	return true;
}

static bool rck_th04_elly_orbit(
	replay_ck_stream_t far *stream, int *orbit_frame
)
{
	uint16_t encoded = static_cast<uint16_t>(*orbit_frame);
	int value;

	if(!rck_u16(stream, &encoded)) {
		return false;
	}
	value = static_cast<int16_t>(encoded);
	if((value < 0) || (value > 768)) {
		return false;
	}
	if(rck_applying(stream)) {
		*orbit_frame = value;
	}
	return true;
}

static bool rck_th04_actor_stages_1_to_3(
	replay_ck_stream_t far *stream
)
{
	switch(stage_id) {
	case 0:
		RCK_U8(midboss1_25594);
		break;

	case 1:
		if(!rck_th04_pattern4(stream, &midboss2_pattern)) {
			return false;
		}
		RCK_U8_MAX(midboss2_255B3, 2);
		RCK_U8_MAX(midboss2_passes, 17);
		RCK_U8(kurumi_259F0);
		break;

	case 2:
		if(!rck_th04_pattern4(stream, &midboss3_pattern)) {
			return false;
		}
		RCK_BOOL(midboss3_25599);
		RCK_U8_MAX(midboss3_patterns_done, 12);
		RCK_U8_MAX(elly_pattern_set, 4);
		RCK_U8_MAX(elly_25A26, 8);
		RCK_U16(elly_25A34);
		RCK_U8(elly_25A36);
		RCK_U8(elly_25A37);
		if(!rck_th04_elly_steering(stream, &elly_25A38)) {
			return false;
		}
		if(!rck_th04_elly_orbit(stream, &elly_25A3A)) {
			return false;
		}
		RCK_U8_MAX(elly_boomerang_flag, 2);
		if(!rck_motion(stream, &elly_boomerang_pos)) {
			return false;
		}
		break;

	default:
		return false;
	}
	return true;
}

static uint8_t rck_th04_bit_fire_id(void)
{
	if(bit_fire == 0) {
		return 0;
	}
	if(bit_fire == marisa_bit_fire_16F24) {
		return 1;
	}
	if(bit_fire == marisa_bit_fire_17061) {
		return 2;
	}
	return 0xFF;
}

static bool rck_th04_bit_fire_apply(uint8_t id)
{
	switch(id) {
	case 0: bit_fire = 0; break;
	case 1: bit_fire = marisa_bit_fire_16F24; break;
	case 2: bit_fire = marisa_bit_fire_17061; break;
	default: return false;
	}
	return true;
}

static bool rck_th04_orb_patnum(
	replay_ck_stream_t far *stream, uint8_t *patnum
)
{
	uint8_t value = *patnum;

	if(!rck_u8(stream, &value)) {
		return false;
	}
	if(
		(value != 0) &&
		(value != PAT_REIMU_ORB_BLUE) &&
		(value != PAT_REIMU_ORB_YELLOW)
	) {
		return false;
	}
	if(rck_applying(stream)) {
		*patnum = value;
	}
	return true;
}

static bool rck_th04_orb_template(
	replay_ck_stream_t far *stream, reimu_orb_t far *orb
)
{
	uint8_t flag = static_cast<uint8_t>(orb->flag);

	if(!rck_u8(stream, &flag) || (flag > OF_MOVE)) {
		return false;
	}
	if(rck_applying(stream)) {
		orb->flag = static_cast<reimu_orb_flag_t>(flag);
	}
	RCK_U8(orb->angle);
	if(
		!rck_pfpoint(stream, &orb->center) ||
		!rck_pfpoint(stream, &orb->origin) ||
		!rck_pfpoint(stream, &orb->velocity)
	) {
		return false;
	}
	RCK_U16(orb->spin_time);
	RCK_S16(orb->distance.v);
	RCK_S16(orb->unknown);
	RCK_U8(orb->move_speed.v);
	RCK_U8(orb->angle_speed);
	return true;
}

static bool rck_th04_marisa_spin(
	replay_ck_stream_t far *stream, uint8_t *spin
)
{
	uint8_t value = *spin;

	if(!rck_u8(stream, &value)) {
		return false;
	}
	if((value != 0) && (value != 2) && (value != 0xFE)) {
		return false;
	}
	if(rck_applying(stream)) {
		*spin = value;
	}
	return true;
}

static bool rck_th04_actor_stage4(replay_ck_stream_t far *stream)
{
	uint8_t bit_fire_id;
	int i;

	if(stage_id != 3) {
		return false;
	}
	if(!rck_th04_pattern4(stream, &midboss4_pattern)) {
		return false;
	}
	RCK_U8_MAX(midboss4_passes, 8);
	RCK_U8(midboss4_22B9E);

	if(playchar == PLAYCHAR_MARISA) {
		RCK_BOOL(reimu_afterimage);
		RCK_U8(reimu_sweep_angle_delta);
		if(!rck_th04_orb_patnum(stream, &orb_patnum_base)) {
			return false;
		}
		RCK_U8(reimu_pattern8_angle);
		RCK_BOOL(reimu_bg_pulse_direction);
		return rck_th04_orb_template(stream, &orb_template);
	}
	if(playchar != PLAYCHAR_REIMU) {
		return false;
	}

	RCK_U8_MAX(bits_alive, BIT_COUNT);
	bit_fire_id = rck_th04_bit_fire_id();
	if(!rck_u8(stream, &bit_fire_id) || (bit_fire_id > 2)) {
		return false;
	}
	for(i = 0; i < BIT_COUNT; i++) {
		RCK_S16(bit_center_x[i]);
		RCK_S16(bit_center_y[i]);
	}
	RCK_U8(marisa_pattern_prev);
	RCK_U8_MAX(marisa_bits_at_pattern_start, BIT_COUNT);
	RCK_BOOL(marisa_pulse_dimming);
	if(!rck_th04_marisa_spin(stream, &marisa_25671)) {
		return false;
	}
	RCK_BOOL(marisa_patterns_without_bits);
	RCK_U8_MAX(marisa_explode_milestone, 3);
	if(rck_applying(stream)) {
		return rck_th04_bit_fire_apply(bit_fire_id);
	}
	return true;
}

static bool rck_th04_actor_stage5(replay_ck_stream_t far *stream)
{
	if(stage_id != 4) {
		return false;
	}
	RCK_U8_MAX(yuuka5_warp_phase, 3);
	RCK_S16(yuuka5_25662);
	RCK_U8(yuuka5_25664);
	RCK_U8(yuuka5_25665);
	RCK_U8(yuuka5_25666);
	return true;
}

static uint8_t rck_th04_bg_clip_id(void)
{
	if(bg_shape_clip == 0) {
		return 0;
	}
	if(bg_shape_clip == bg_shape_clip_and_wrap) {
		return 1;
	}
	if(bg_shape_clip == bg_shape_clip_and_respawn_in_cen) {
		return 2;
	}
	return 0xFF;
}

static bool rck_th04_bg_clip_compatible(
	uint8_t id, uint8_t state, uint8_t state_frame
)
{
	if(id == 0) {
		return (
			(state == 0) &&
			(boss.phase <= PHASE_BOSS_ENTRANCE_BB)
		);
	}
	if(id == 1) {
		return (
			(state <= 5) ||
			(state == 8) || (state == 9) ||
			(state >= 0xC) ||
			(((state == 6) || (state == 0xA)) && (state_frame == 0))
		);
	}
	if(id == 2) {
		return (
			(state == 6) || (state == 7) ||
			(state == 0xA) || (state == 0xB) ||
			(((state == 8) || (state == 0xC)) && (state_frame == 0))
		);
	}
	return false;
}

static bool rck_th04_bg_clip_apply(uint8_t id)
{
	switch(id) {
	case 0: bg_shape_clip = 0; break;
	case 1: bg_shape_clip = bg_shape_clip_and_wrap; break;
	case 2: bg_shape_clip = bg_shape_clip_and_respawn_in_cen; break;
	default: return false;
	}
	return true;
}

static bool rck_th04_bg_patnum(
	replay_ck_stream_t far *stream, uint8_t state
)
{
	uint16_t encoded = static_cast<uint16_t>(bg_shape_patnum);
	bool valid;

	if(!rck_u16(stream, &encoded)) {
		return false;
	}
	if((encoded == 0) && (state == 0)) {
		valid = (boss.phase <= PHASE_BOSS_ENTRANCE_BB);
	} else if(state <= 3) {
		valid = (encoded == (120 + state));
	} else if(state <= 0xF) {
		valid = (encoded == 120);
	} else if(state == 0x10) {
		valid = (encoded == 124);
	} else {
		valid = (encoded == 125);
	}
	if(!valid) {
		return false;
	}
	if(rck_applying(stream)) {
		bg_shape_patnum = static_cast<main_patnum_t>(encoded);
	}
	return true;
}

static bool rck_th04_yuuka6_sprite_flag(
	replay_ck_stream_t far *stream
)
{
	uint8_t value = yuuka6_sprite_flag;

	if(!rck_u8(stream, &value)) {
		return false;
	}
	switch(value) {
	case 0: case 1: case 2: case 3: case 4: case 8:
		break;
	default:
		return false;
	}
	if(rck_applying(stream)) {
		yuuka6_sprite_flag = value;
	}
	return true;
}

static bool rck_th04_yuuka6_anim_frame(
	replay_ck_stream_t far *stream
)
{
	uint16_t encoded = static_cast<uint16_t>(yuuka6_anim_frame);
	int value;

	if(!rck_u16(stream, &encoded)) {
		return false;
	}
	value = static_cast<int16_t>(encoded);
	if((value < 0) || (value > 39)) {
		return false;
	}
	if(rck_applying(stream)) {
		yuuka6_anim_frame = value;
	}
	return true;
}

static bool rck_th04_yuuka6_pattern_prev(
	replay_ck_stream_t far *stream
)
{
	uint8_t value = yuuka6_25A02;

	if(!rck_u8(stream, &value)) {
		return false;
	}
	if((value > 2) && (value != 0xFF)) {
		return false;
	}
	if(rck_applying(stream)) {
		yuuka6_25A02 = value;
	}
	return true;
}

static bool rck_th04_actor_stage6(replay_ck_stream_t far *stream)
{
	uint8_t clip_id;
	int i;

	if(stage_id != 5) {
		return false;
	}
	RCK_U8_MAX(yuuka6_bg_state, 0x11);
	RCK_U8(yuuka6_bg_state_frame);
	RCK_BOOL(yuuka6_bg_fade_done);
	if(!rck_th04_bg_patnum(stream, yuuka6_bg_state)) {
		return false;
	}
	RCK_S16(bg_shape_flyout_speed.v);
	clip_id = rck_th04_bg_clip_id();
	if(
		!rck_u8(stream, &clip_id) ||
		!rck_th04_bg_clip_compatible(
			clip_id, yuuka6_bg_state, yuuka6_bg_state_frame
		)
	) {
		return false;
	}
	for(i = 0; i < 56; i++) {
		if(!rck_sppoint(stream, &bg_shapes[i].pos)) {
			return false;
		}
		RCK_U8(bg_shapes[i].angle);
		RCK_U8(bg_shapes[i].speed.v);
	}
	if(
		!rck_th04_yuuka6_anim_frame(stream) ||
		!rck_th04_yuuka6_sprite_flag(stream)
	) {
		return false;
	}
	RCK_U8_MAX(yuuka6_phase2_fly_path, 1);
	if(!rck_th04_yuuka6_pattern_prev(stream)) {
		return false;
	}
	RCK_U8(yuuka6_25A03);
	RCK_U8(yuuka6_25A04);
	RCK_BOOL(yuuka6_25A08);
	if(!rck_pfpoint(stream, &yuuka6_25A0C)) {
		return false;
	}
	RCK_U8_MAX(yuuka6_25A1B, 2);
	RCK_U8(yuuka6_25A1E);
	if(rck_applying(stream)) {
		return rck_th04_bg_clip_apply(clip_id);
	}
	return true;
}

static uint8_t rck_th04_mugetsu_pose_id(void)
{
	if(mugetsu_pose_func == 0) {
		return 0;
	}
	if(mugetsu_pose_func == mugetsu_180BB) {
		return 1;
	}
	if(mugetsu_pose_func == mugetsu_1812A) {
		return 2;
	}
	if(mugetsu_pose_func == mugetsu_1821E) {
		return 3;
	}
	return 0xFF;
}

static bool rck_th04_mugetsu_pose_apply(uint8_t id)
{
	switch(id) {
	case 0: mugetsu_pose_func = 0; break;
	case 1: mugetsu_pose_func = mugetsu_180BB; break;
	case 2: mugetsu_pose_func = mugetsu_1812A; break;
	case 3: mugetsu_pose_func = mugetsu_1821E; break;
	default: return false;
	}
	return true;
}

static bool rck_th04_mugetsu_offset(
	replay_ck_stream_t far *stream, int *value_out
)
{
	uint16_t encoded = static_cast<uint16_t>(mugetsu_gather_frame_offset);
	int value;

	if(!rck_u16(stream, &encoded)) {
		return false;
	}
	value = static_cast<int16_t>(encoded);
	if((value != 0x10) && (value != 0) && (value != -0x50)) {
		return false;
	}
	if(rck_applying(stream)) {
		mugetsu_gather_frame_offset = value;
	}
	*value_out = value;
	return true;
}

static bool rck_th04_mugetsu_pose_compatible(
	uint8_t id, int offset
)
{
	uint8_t update_id = rck_th04_boss_update_id();

	if(update_id == 0) {
		return (id <= 3);
	}
	if(update_id == 2) {
		return (id != 0) && (id <= 3);
	}
	if(update_id != 1) {
		return false;
	}
	if(boss.phase <= 1) {
		return (id == 1) && (offset == 0x10);
	}
	if((boss.phase == 2) || (boss.phase == 3)) {
		if((boss.phase == 2) && (boss.phase_frame == 0)) {
			return (
				((id == 1) || (id == 2)) &&
				((offset == 0x10) || (offset == 0))
			);
		}
		return (
			((id == 1) && (offset == 0x10)) ||
			((id == 2) && (offset == 0))
		);
	}
	if((boss.phase == 4) && (boss.phase_frame == 0)) {
		return (
			(id == 3) &&
			((offset == 0x10) || (offset == 0) || (offset == -0x50))
		);
	}
	return (id == 3) && (offset == -0x50);
}

static bool rck_th04_actor_extra(replay_ck_stream_t far *stream)
{
	uint8_t pose_id;
	int gather_offset;

	if(stage_id != STAGE_EXTRA) {
		return false;
	}
	pose_id = rck_th04_mugetsu_pose_id();
	if(
		!rck_u8(stream, &pose_id) ||
		!rck_th04_mugetsu_offset(stream, &gather_offset) ||
		!rck_th04_mugetsu_pose_compatible(pose_id, gather_offset)
	) {
		return false;
	}
	if(!rck_sppoint(stream, &mugetsu_gather_center)) {
		return false;
	}
	RCK_U8_MAX(mugetsu_phase2_mode, 36);
	RCK_U8(mugetsu_damage_frames);
	RCK_U8(gengetsu_damage_frames);
	RCK_U8_MAX(extra_boss_bomb_immunity, 32);
	RCK_U8(gengetsu_wave_amp);
	RCK_S16(gengetsu_wave_target_x.v);
	if(rck_applying(stream)) {
		return rck_th04_mugetsu_pose_apply(pose_id);
	}
	return true;
}

static bool rck_group_actors(replay_ck_stream_t far *stream)
{
	if(
		!rck_group_actors_common(stream) ||
		!rck_th04_actor_callbacks(stream)
	) {
		return false;
	}
	switch(stage_id) {
	case 0: case 1: case 2:
		return rck_th04_actor_stages_1_to_3(stream);
	case 3:
		return rck_th04_actor_stage4(stream);
	case 4:
		return rck_th04_actor_stage5(stream);
	case 5:
		return rck_th04_actor_stage6(stream);
	case STAGE_EXTRA:
		return rck_th04_actor_extra(stream);
	}
	return false;
}
#else
enum rck_th05_midboss_setup_t {
	RCK5MS_NONE = 0,
	RCK5MS_STAGE1,
	RCK5MS_STAGE2,
	RCK5MS_STAGE3,
	RCK5MS_STAGE4,
	RCK5MS_STAGE5,
	RCK5MS_EXTRA,
};

enum rck_th05_boss_setup_t {
	RCK5BS_SARA = 1,
	RCK5BS_LOUISE,
	RCK5BS_ALICE,
	RCK5BS_MAI_YUKI,
	RCK5BS_YUMEKO,
	RCK5BS_SHINKI,
	RCK5BS_EXALICE,
};

enum rck_th05_boss_live_t {
	RCK5BL_NONE = 0,
	RCK5BL_SETUP,
	RCK5BL_YUKI,
	RCK5BL_MAI,
};

enum rck_th05_custom_render_t {
	RCK5CR_NONE = 0,
	RCK5CR_CHEETOS,
	RCK5CR_B4BALLS,
	RCK5CR_SWORDS,
	RCK5CR_SHINKI,
	RCK5CR_EXALICE,
};

static uint8_t rck_th05_midboss_setup_id(void)
{
	if(
		(midboss_update_func == nullfunc_far) &&
		(midboss_render_func == nullfunc_near)
	) {
		return RCK5MS_NONE;
	}
	if((midboss_update_func == midboss1_update) &&
	   (midboss_render_func == midboss1_render)) {
		return RCK5MS_STAGE1;
	}
	if((midboss_update_func == midboss2_update) &&
	   (midboss_render_func == midboss2_render)) {
		return RCK5MS_STAGE2;
	}
	if((midboss_update_func == midboss3_update) &&
	   (midboss_render_func == midboss3_render)) {
		return RCK5MS_STAGE3;
	}
	if((midboss_update_func == midboss4_update) &&
	   (midboss_render_func == midboss4_render)) {
		return RCK5MS_STAGE4;
	}
	if((midboss_update_func == midboss5_update) &&
	   (midboss_render_func == midboss5_render)) {
		return RCK5MS_STAGE5;
	}
	if((midboss_update_func == midbossx_update) &&
	   (midboss_render_func == midbossx_render)) {
		return RCK5MS_EXTRA;
	}
	return 0xFF;
}

static bool rck_th05_midboss_stage_valid(uint8_t id)
{
	switch(stage_id) {
	case 0: return (id == RCK5MS_STAGE1);
	case 1: return (id == RCK5MS_STAGE2);
	case 2: return (id == RCK5MS_STAGE3);
	case 3: return (id == RCK5MS_STAGE4);
	case 4: return (id == RCK5MS_STAGE5);
	case 5: return (id == RCK5MS_NONE);
	case STAGE_EXTRA: return (id == RCK5MS_EXTRA);
	}
	return false;
}

static bool rck_th05_midboss_setup_apply(uint8_t id)
{
	switch(id) {
	case RCK5MS_NONE:
		midboss_update_func = nullfunc_far;
		midboss_render_func = nullfunc_near;
		break;
	case RCK5MS_STAGE1:
		midboss_update_func = midboss1_update;
		midboss_render_func = midboss1_render;
		break;
	case RCK5MS_STAGE2:
		midboss_update_func = midboss2_update;
		midboss_render_func = midboss2_render;
		break;
	case RCK5MS_STAGE3:
		midboss_update_func = midboss3_update;
		midboss_render_func = midboss3_render;
		break;
	case RCK5MS_STAGE4:
		midboss_update_func = midboss4_update;
		midboss_render_func = midboss4_render;
		break;
	case RCK5MS_STAGE5:
		midboss_update_func = midboss5_update;
		midboss_render_func = midboss5_render;
		break;
	case RCK5MS_EXTRA:
		midboss_update_func = midbossx_update;
		midboss_render_func = midbossx_render;
		break;
	default:
		return false;
	}
	return true;
}

#define RCK5_BOSS_SETUP_CASE(id, bg, update, fg, backdrop) \
	if( \
		(boss_bg_render_func == bg) && \
		(boss_update_func == update) && \
		(boss_fg_render_func == fg) && \
		(boss_backdrop_colorfill == backdrop) \
	) { return id; }

static uint8_t rck_th05_boss_setup_id(void)
{
	RCK5_BOSS_SETUP_CASE(
		RCK5BS_SARA, sara_bg_render, sara_update, sara_fg_render,
		sara_backdrop_colorfill
	);
	RCK5_BOSS_SETUP_CASE(
		RCK5BS_LOUISE, louise_bg_render, louise_update, louise_fg_render,
		louise_backdrop_colorfill
	);
	RCK5_BOSS_SETUP_CASE(
		RCK5BS_ALICE, alice_bg_render, alice_update, alice_fg_render,
		alice_backdrop_colorfill
	);
	RCK5_BOSS_SETUP_CASE(
		RCK5BS_MAI_YUKI, mai_yuki_bg_render, mai_yuki_update,
		mai_yuki_fg_render, mai_yuki_backdrop_colorfill
	);
	RCK5_BOSS_SETUP_CASE(
		RCK5BS_YUMEKO, yumeko_bg_render, yumeko_update, yumeko_fg_render,
		yumeko_backdrop_colorfill
	);
	RCK5_BOSS_SETUP_CASE(
		RCK5BS_SHINKI, shinki_bg_render, shinki_update, shinki_fg_render,
		shinki_stage_backdrop_colorfill
	);
	RCK5_BOSS_SETUP_CASE(
		RCK5BS_EXALICE, exalice_bg_render, exalice_update,
		exalice_fg_render, shinki_stage_backdrop_colorfill
	);
	return 0xFF;
}

#undef RCK5_BOSS_SETUP_CASE

static bool rck_th05_boss_stage_valid(uint8_t id)
{
	return (id == (RCK5BS_SARA + stage_id));
}

static bool rck_th05_boss_setup_apply(uint8_t id)
{
	switch(id) {
	case RCK5BS_SARA:
		boss_bg_render_func = sara_bg_render;
		boss_update_func = sara_update;
		boss_fg_render_func = sara_fg_render;
		boss_backdrop_colorfill = sara_backdrop_colorfill;
		break;
	case RCK5BS_LOUISE:
		boss_bg_render_func = louise_bg_render;
		boss_update_func = louise_update;
		boss_fg_render_func = louise_fg_render;
		boss_backdrop_colorfill = louise_backdrop_colorfill;
		break;
	case RCK5BS_ALICE:
		boss_bg_render_func = alice_bg_render;
		boss_update_func = alice_update;
		boss_fg_render_func = alice_fg_render;
		boss_backdrop_colorfill = alice_backdrop_colorfill;
		break;
	case RCK5BS_MAI_YUKI:
		boss_bg_render_func = mai_yuki_bg_render;
		boss_update_func = mai_yuki_update;
		boss_fg_render_func = mai_yuki_fg_render;
		boss_backdrop_colorfill = mai_yuki_backdrop_colorfill;
		break;
	case RCK5BS_YUMEKO:
		boss_bg_render_func = yumeko_bg_render;
		boss_update_func = yumeko_update;
		boss_fg_render_func = yumeko_fg_render;
		boss_backdrop_colorfill = yumeko_backdrop_colorfill;
		break;
	case RCK5BS_SHINKI:
		boss_bg_render_func = shinki_bg_render;
		boss_update_func = shinki_update;
		boss_fg_render_func = shinki_fg_render;
		boss_backdrop_colorfill = shinki_stage_backdrop_colorfill;
		break;
	case RCK5BS_EXALICE:
		boss_bg_render_func = exalice_bg_render;
		boss_update_func = exalice_update;
		boss_fg_render_func = exalice_fg_render;
		boss_backdrop_colorfill = shinki_stage_backdrop_colorfill;
		break;
	default:
		return false;
	}
	return true;
}

static uint8_t rck_th05_midboss_live_id(void)
{
	if((midboss_invalidate == nullfunc_near) &&
	   (midboss_update == nullfunc_far) &&
	   (midboss_render == nullfunc_near)) {
		return 0;
	}
	if((midboss_invalidate == midboss_invalidate_func) &&
	   (midboss_update == midboss_update_func) &&
	   (midboss_render == midboss_render_func)) {
		return 1;
	}
	return 0xFF;
}

static uint8_t rck_th05_boss_live_id(void)
{
	if((boss_update == nullfunc_far) && (boss_fg_render == nullfunc_near)) {
		return RCK5BL_NONE;
	}
	if((boss_update == boss_update_func) &&
	   (boss_fg_render == boss_fg_render_func)) {
		return RCK5BL_SETUP;
	}
	if((boss_update == yuki_update) &&
	   (boss_fg_render == b4_solo_fg_render)) {
		return RCK5BL_YUKI;
	}
	if((boss_update == mai_update) &&
	   (boss_fg_render == b4_solo_fg_render)) {
		return RCK5BL_MAI;
	}
	return 0xFF;
}

static uint8_t rck_th05_custom_render_id(void)
{
	if(boss_custombullets_render == nullfunc_near) { return RCK5CR_NONE; }
	if(boss_custombullets_render == cheetos_render) { return RCK5CR_CHEETOS; }
	if(boss_custombullets_render == b4balls_render) { return RCK5CR_B4BALLS; }
	if(boss_custombullets_render == swords_render) { return RCK5CR_SWORDS; }
	if(boss_custombullets_render == shinki_custombullets_render) {
		return RCK5CR_SHINKI;
	}
	if(boss_custombullets_render == exalice_custombullets_render) {
		return RCK5CR_EXALICE;
	}
	return 0xFF;
}

static bool rck_th05_custom_render_compatible(uint8_t id)
{
	switch(stage_id) {
	case 0: case 1: case 2:
		return (id == RCK5CR_NONE);
	case 3:
		return (
			(id == RCK5CR_NONE) || (id == RCK5CR_CHEETOS) ||
			(id == RCK5CR_B4BALLS)
		);
	case 4:
		return ((id == RCK5CR_NONE) || (id == RCK5CR_SWORDS));
	case 5:
		return ((id == RCK5CR_NONE) || (id == RCK5CR_SHINKI));
	case STAGE_EXTRA:
		return ((id == RCK5CR_NONE) || (id == RCK5CR_EXALICE));
	}
	return false;
}

static bool rck_th05_custom_render_apply(uint8_t id)
{
	switch(id) {
	case RCK5CR_NONE: boss_custombullets_render = nullfunc_near; break;
	case RCK5CR_CHEETOS: boss_custombullets_render = cheetos_render; break;
	case RCK5CR_B4BALLS: boss_custombullets_render = b4balls_render; break;
	case RCK5CR_SWORDS: boss_custombullets_render = swords_render; break;
	case RCK5CR_SHINKI:
		boss_custombullets_render = shinki_custombullets_render;
		break;
	case RCK5CR_EXALICE:
		boss_custombullets_render = exalice_custombullets_render;
		break;
	default: return false;
	}
	return true;
}

static bool rck_th05_actor_callbacks(replay_ck_stream_t far *stream)
{
	uint8_t midboss_setup = rck_th05_midboss_setup_id();
	uint8_t midboss_live = rck_th05_midboss_live_id();
	uint8_t boss_setup = rck_th05_boss_setup_id();
	uint8_t boss_live = rck_th05_boss_live_id();
	uint8_t custom_render = rck_th05_custom_render_id();
	uint8_t random_y = static_cast<uint8_t>(
		boss_flystep_random_next_y_direction
	);

	if(
		!rck_u8(stream, &midboss_setup) ||
		!rck_th05_midboss_stage_valid(midboss_setup) ||
		!rck_u8(stream, &midboss_live) || (midboss_live > 1) ||
		((midboss_setup == RCK5MS_NONE) && midboss_live) ||
		!rck_u8(stream, &boss_setup) ||
		!rck_th05_boss_stage_valid(boss_setup) ||
		!rck_u8(stream, &boss_live) || (boss_live > RCK5BL_MAI) ||
		((boss_live >= RCK5BL_YUKI) && (stage_id != 3)) ||
		!rck_u8(stream, &custom_render) ||
		!rck_th05_custom_render_compatible(custom_render) ||
		!rck_u8(stream, &random_y) || (random_y > Y_DOWN)
	) {
		return false;
	}
	if(!rck_applying(stream)) {
		return true;
	}
	if(
		!rck_th05_midboss_setup_apply(midboss_setup) ||
		!rck_th05_boss_setup_apply(boss_setup) ||
		!rck_th05_custom_render_apply(custom_render)
	) {
		return false;
	}
	midboss_invalidate = midboss_live
		? midboss_invalidate_func : nullfunc_near;
	midboss_update = midboss_live ? midboss_update_func : nullfunc_far;
	midboss_render = midboss_live ? midboss_render_func : nullfunc_near;
	switch(boss_live) {
	case RCK5BL_NONE:
		boss_update = nullfunc_far;
		boss_fg_render = nullfunc_near;
		break;
	case RCK5BL_SETUP:
		boss_update = boss_update_func;
		boss_fg_render = boss_fg_render_func;
		break;
	case RCK5BL_YUKI:
		boss_update = yuki_update;
		boss_fg_render = b4_solo_fg_render;
		break;
	case RCK5BL_MAI:
		boss_update = mai_update;
		boss_fg_render = b4_solo_fg_render;
		break;
	}
	boss_flystep_random_next_y_direction = static_cast<y_direction_t>(random_y);
	return true;
}

static uint8_t rck_th05_oneshot_table_id(
	pattern_oneshot_func_t value,
	const pattern_oneshot_func_t near *table,
	uint8_t count
)
{
	uint8_t i;
	if(value == 0) {
		return 0;
	}
	for(i = 0; i < count; i++) {
		if(value == table[i]) {
			return (i + 1);
		}
	}
	return 0xFF;
}

static pattern_oneshot_func_t rck_th05_oneshot_table_func(
	uint8_t id,
	const pattern_oneshot_func_t near *table,
	uint8_t count
)
{
	if(id == 0) {
		return 0;
	}
	if(id > count) {
		return 0;
	}
	return table[id - 1];
}

static bool rck_th05_oneshot_table_codec(
	replay_ck_stream_t far *stream,
	pattern_oneshot_func_t near *value,
	const pattern_oneshot_func_t near *table,
	uint8_t count
)
{
	uint8_t id = rck_th05_oneshot_table_id(*value, table, count);
	pattern_oneshot_func_t decoded;
	if(!rck_u8(stream, &id)) {
		return false;
	}
	decoded = rck_th05_oneshot_table_func(id, table, count);
	if(rck_th05_oneshot_table_id(decoded, table, count) != id) {
		return false;
	}
	if(rck_applying(stream)) {
		*value = decoded;
	}
	return true;
}

static uint8_t rck_th05_loop_table_id(
	pattern_loop_func_t value,
	const pattern_loop_func_t near *table,
	uint8_t count
)
{
	uint8_t i;
	if(value == 0) {
		return 0;
	}
	for(i = 0; i < count; i++) {
		if(value == table[i]) {
			return (i + 1);
		}
	}
	return 0xFF;
}

static pattern_loop_func_t rck_th05_loop_table_func(
	uint8_t id,
	const pattern_loop_func_t near *table,
	uint8_t count
)
{
	if(id == 0) {
		return 0;
	}
	if(id > count) {
		return 0;
	}
	return table[id - 1];
}

static bool rck_th05_loop_table_codec(
	replay_ck_stream_t far *stream,
	pattern_loop_func_t near *value,
	const pattern_loop_func_t near *table,
	uint8_t count
)
{
	uint8_t id = rck_th05_loop_table_id(*value, table, count);
	pattern_loop_func_t decoded;
	if(!rck_u8(stream, &id)) {
		return false;
	}
	decoded = rck_th05_loop_table_func(id, table, count);
	if(rck_th05_loop_table_id(decoded, table, count) != id) {
		return false;
	}
	if(rck_applying(stream)) {
		*value = decoded;
	}
	return true;
}

static uint8_t rck_th05_puppet_id(rck_puppet_func_t value)
{
	uint8_t i;
	if(value == 0) { return 0; }
	for(i = 0; i < 4; i++) {
		if(value == ALICE_PUPPET_PATTERNS[i]) {
			return (i + 1);
		}
	}
	if(value == alice_puppet_pattern_19A84) { return 5; }
	if(value == alice_puppet_pattern_19AE3) { return 6; }
	if(value == alice_puppet_pattern_19AFB) { return 7; }
	return 0xFF;
}

static rck_puppet_func_t rck_th05_puppet_func(uint8_t id)
{
	if(id == 0) { return 0; }
	if((id >= 1) && (id <= 4)) {
		return ALICE_PUPPET_PATTERNS[id - 1];
	}
	switch(id) {
	case 5: return alice_puppet_pattern_19A84;
	case 6: return alice_puppet_pattern_19AE3;
	case 7: return alice_puppet_pattern_19AFB;
	}
	return 0;
}

static bool rck_th05_puppet_codec(
	replay_ck_stream_t far *stream, rck_puppet_func_t near *value
)
{
	uint8_t id = rck_th05_puppet_id(*value);
	rck_puppet_func_t decoded;
	if(!rck_u8(stream, &id)) {
		return false;
	}
	decoded = rck_th05_puppet_func(id);
	if(rck_th05_puppet_id(decoded) != id) {
		return false;
	}
	if(rck_applying(stream)) {
		*value = decoded;
	}
	return true;
}

static bool rck_th05_actor_stage1(replay_ck_stream_t far *stream)
{
	return rck_th05_loop_table_codec(
		stream,
		&sara_phase_2_3_pattern,
		&SARA_PATTERNS_PHASE_2_3[0][0],
		8
	);
}

static bool rck_th05_actor_stage2(replay_ck_stream_t far *stream)
{
	return rck_sppoint(stream, &midboss2_center);
}

static bool rck_th05_actor_stage3(replay_ck_stream_t far *stream)
{
	if(
		!rck_th05_puppet_codec(stream, &fp_2CE2A) ||
		!rck_th05_puppet_codec(stream, &fp_2CE2C)
	) {
		return false;
	}
	RCK_U16(alice_barrier_frame);
	RCK_U16(alice_barrier_fire_frames);
	return rck_th05_loop_table_codec(
		stream, &fp_2CE32, &off_22770[0][0], 12
	);
}

static uint8_t rck_th05_mai_pair_id(pattern_oneshot_func_t value)
{
	uint8_t id;
	if(value == 0) { return 0; }
	id = rck_th05_oneshot_table_id(value, MAI_PAIR_PATTERNS_1, 4);
	if(id != 0xFF) { return id; }
	if(value == mai_yuki_1A775) { return 5; }
	id = rck_th05_oneshot_table_id(value, MAI_PAIR_PATTERNS_3, 4);
	return ((id == 0xFF) ? 0xFF : static_cast<uint8_t>(id + 5));
}

static pattern_oneshot_func_t rck_th05_mai_pair_func(uint8_t id)
{
	if(id == 0) { return 0; }
	if(id <= 4) { return MAI_PAIR_PATTERNS_1[id - 1]; }
	if(id == 5) { return mai_yuki_1A775; }
	if(id <= 9) { return MAI_PAIR_PATTERNS_3[id - 6]; }
	return 0;
}

static uint8_t rck_th05_yuki_pair_id(pattern_oneshot_func_t value)
{
	uint8_t id;
	if(value == 0) { return 0; }
	id = rck_th05_oneshot_table_id(value, YUKI_PAIR_PATTERNS_1, 4);
	if(id != 0xFF) { return id; }
	id = rck_th05_oneshot_table_id(value, YUKI_PAIR_PATTERNS_2, 4);
	if(id != 0xFF) { return (id + 4); }
	id = rck_th05_oneshot_table_id(value, YUKI_PAIR_PATTERNS_3, 4);
	return ((id == 0xFF) ? 0xFF : static_cast<uint8_t>(id + 8));
}

static pattern_oneshot_func_t rck_th05_yuki_pair_func(uint8_t id)
{
	if(id == 0) { return 0; }
	if(id <= 4) { return YUKI_PAIR_PATTERNS_1[id - 1]; }
	if(id <= 8) { return YUKI_PAIR_PATTERNS_2[id - 5]; }
	if(id <= 12) { return YUKI_PAIR_PATTERNS_3[id - 9]; }
	return 0;
}

static uint8_t rck_th05_solo_id(pattern_oneshot_func_t value)
{
	uint8_t id;
	if(value == 0) { return 0; }
	id = rck_th05_oneshot_table_id(value, MAI_PATTERNS_PHASE_3, 2);
	if(id != 0xFF) { return id; }
	id = rck_th05_oneshot_table_id(value, MAI_PATTERNS_PHASE_7, 2);
	if(id != 0xFF) { return (id + 2); }
	id = rck_th05_oneshot_table_id(value, MAI_PATTERNS_PHASE_9, 2);
	if(id != 0xFF) { return (id + 4); }
	id = rck_th05_oneshot_table_id(value, YUKI_PATTERNS_PHASE_3, 2);
	if(id != 0xFF) { return (id + 6); }
	id = rck_th05_oneshot_table_id(value, YUKI_PATTERNS_PHASE_5, 2);
	if(id != 0xFF) { return (id + 8); }
	id = rck_th05_oneshot_table_id(value, YUKI_PATTERNS_PHASE_9, 5);
	return ((id == 0xFF) ? 0xFF : static_cast<uint8_t>(id + 10));
}

static pattern_oneshot_func_t rck_th05_solo_func(uint8_t id)
{
	if(id == 0) { return 0; }
	if(id <= 2) { return MAI_PATTERNS_PHASE_3[id - 1]; }
	if(id <= 4) { return MAI_PATTERNS_PHASE_7[id - 3]; }
	if(id <= 6) { return MAI_PATTERNS_PHASE_9[id - 5]; }
	if(id <= 8) { return YUKI_PATTERNS_PHASE_3[id - 7]; }
	if(id <= 10) { return YUKI_PATTERNS_PHASE_5[id - 9]; }
	if(id <= 15) { return YUKI_PATTERNS_PHASE_9[id - 11]; }
	return 0;
}

static bool rck_th05_direction_codec(
	replay_ck_stream_t far *stream, y_direction_t near *direction
)
{
	uint8_t value = static_cast<uint8_t>(*direction);
	if(!rck_u8(stream, &value) || (value > Y_DOWN)) {
		return false;
	}
	if(rck_applying(stream)) {
		*direction = static_cast<y_direction_t>(value);
	}
	return true;
}

static bool rck_th05_actor_stage4(replay_ck_stream_t far *stream)
{
	uint8_t mai_id = rck_th05_mai_pair_id(mai_pair_pattern);
	uint8_t yuki_id = rck_th05_yuki_pair_id(yuki_pair_pattern);
	uint8_t solo_id = rck_th05_solo_id(mai_yuki_pattern);
	uint8_t laser_id = rck_th05_loop_table_id(
		mai_laser_bullet_pattern, MAI_LASER_BULLET_PATTERNS, 3
	);
	pattern_oneshot_func_t decoded_oneshot;
	pattern_loop_func_t decoded_loop;

	RCK_S16(midboss4_warp_x);
	if(
		!rck_th05_direction_codec(
			stream, &mai_flystep_random_next_y_direction
		) ||
		!rck_th05_direction_codec(
			stream, &yuki_flystep_random_next_y_direction
		) ||
		!rck_u8(stream, &mai_id)
	) {
		return false;
	}
	decoded_oneshot = rck_th05_mai_pair_func(mai_id);
	if(rck_th05_mai_pair_id(decoded_oneshot) != mai_id) {
		return false;
	}
	if(rck_applying(stream)) { mai_pair_pattern = decoded_oneshot; }
	if(!rck_u8(stream, &yuki_id)) { return false; }
	decoded_oneshot = rck_th05_yuki_pair_func(yuki_id);
	if(rck_th05_yuki_pair_id(decoded_oneshot) != yuki_id) {
		return false;
	}
	if(rck_applying(stream)) { yuki_pair_pattern = decoded_oneshot; }
	if(!rck_u8(stream, &solo_id)) { return false; }
	decoded_oneshot = rck_th05_solo_func(solo_id);
	if(rck_th05_solo_id(decoded_oneshot) != solo_id) {
		return false;
	}
	if(rck_applying(stream)) { mai_yuki_pattern = decoded_oneshot; }
	RCK_U16_MAX(mai_laser_count, 10);
	RCK_U16_MAX(mai_laser_angle_speed, 0x80);
	RCK_U16_MAX(mai_laser_angle_progress, 0x0F);
	if(!rck_u8(stream, &laser_id)) { return false; }
	decoded_loop = rck_th05_loop_table_func(
		laser_id, MAI_LASER_BULLET_PATTERNS, 3
	);
	if(
		rck_th05_loop_table_id(
			decoded_loop, MAI_LASER_BULLET_PATTERNS, 3
		) != laser_id
	) {
		return false;
	}
	if(rck_applying(stream)) { mai_laser_bullet_pattern = decoded_loop; }
	return true;
}

static uint8_t rck_th05_yumeko_id(pattern_loop_func_t value)
{
	uint8_t id;
	if(value == 0) { return 0; }
	id = rck_th05_loop_table_id(value, YUMEKO_PATTERNS_PHASE_2, 2);
	if(id != 0xFF) { return id; }
	id = rck_th05_loop_table_id(value, YUMEKO_PATTERNS_PHASE_5, 2);
	if(id != 0xFF) { return (id + 2); }
	if(value == yumeko_1CB71) { return 5; }
	if(value == yumeko_1CED9) { return 6; }
	return 0xFF;
}

static pattern_loop_func_t rck_th05_yumeko_func(uint8_t id)
{
	if(id == 0) { return 0; }
	if(id <= 2) { return YUMEKO_PATTERNS_PHASE_2[id - 1]; }
	if(id <= 4) { return YUMEKO_PATTERNS_PHASE_5[id - 3]; }
	if(id == 5) { return yumeko_1CB71; }
	if(id == 6) { return yumeko_1CED9; }
	return 0;
}

static bool rck_th05_actor_stage5(replay_ck_stream_t far *stream)
{
	uint8_t yumeko_id = rck_th05_yumeko_id(yumeko_pattern);
	pattern_loop_func_t decoded;
	if(!rck_th05_oneshot_table_codec(
		stream,
		&midboss5_phase_1_pattern,
		MIDBOSS5_PATTERNS_PHASE_1,
		3
	)) {
		return false;
	}
	if(!rck_u8(stream, &yumeko_id)) {
		return false;
	}
	decoded = rck_th05_yumeko_func(yumeko_id);
	if(rck_th05_yumeko_id(decoded) != yumeko_id) {
		return false;
	}
	if(rck_applying(stream)) {
		yumeko_pattern = decoded;
	}
	return true;
}

static uint8_t rck_th05_wing_id(pattern_loop_func_t value)
{
	if(value == 0) { return 0; }
	if(value == pattern_random_rain_and_spreads_from_wings) { return 1; }
	if(value == pattern_cheetos_within_spread_walls) { return 2; }
	if(value == pattern_aimed_b6balls_and_symmetric_spreads) { return 3; }
	if(value == pattern_devil) { return 4; }
	return 0xFF;
}

static pattern_loop_func_t rck_th05_wing_func(uint8_t id)
{
	switch(id) {
	case 0: return 0;
	case 1: return pattern_random_rain_and_spreads_from_wings;
	case 2: return pattern_cheetos_within_spread_walls;
	case 3: return pattern_aimed_b6balls_and_symmetric_spreads;
	case 4: return pattern_devil;
	}
	return 0;
}

static bool rck_th05_actor_stage6(replay_ck_stream_t far *stream)
{
	uint8_t wing_id = rck_th05_wing_id(shinki_wing_pattern);
	pattern_loop_func_t decoded;
	if(!rck_th05_oneshot_table_codec(
		stream,
		&shinki_phase_2_3_pattern,
		SHINKI_PATTERNS_PHASE_2_3,
		4
	)) {
		return false;
	}
	if(!rck_u8(stream, &wing_id)) {
		return false;
	}
	decoded = rck_th05_wing_func(wing_id);
	if(rck_th05_wing_id(decoded) != wing_id) {
		return false;
	}
	if(rck_applying(stream)) {
		shinki_wing_pattern = decoded;
	}
	RCK_U16(shinki_devil_laser_grow_delay);
	RCK_BOOL(shinki_float_direction);
	return true;
}

static uint8_t rck_th05_midbossx_id(pattern_oneshot_func_t value)
{
	uint8_t id;
	if(value == 0) { return 0; }
	if(value == pattern_wait) { return 1; }
	id = rck_th05_oneshot_table_id(
		value, &MIDBOSSX_PATTERNS_PHASE_1[0][0], 4
	);
	return ((id == 0xFF) ? 0xFF : static_cast<uint8_t>(id + 1));
}

static pattern_oneshot_func_t rck_th05_midbossx_func(uint8_t id)
{
	if(id == 0) { return 0; }
	if(id == 1) { return pattern_wait; }
	if(id <= 5) { return (&MIDBOSSX_PATTERNS_PHASE_1[0][0])[id - 2]; }
	return 0;
}

static uint8_t rck_th05_exalice_id(pattern_oneshot_func_t value)
{
	uint8_t id;
	if(value == 0) { return 0; }
	id = rck_th05_oneshot_table_id(value, &EXALICE_PATTERNS[0][0], 8);
	if(id != 0xFF) { return id; }
	if(value == pattern_spreads_and_firewaves) { return 9; }
	if(value == pattern_bouncing_blue_rings) { return 10; }
	if(value == pattern_pingpong_lasers) { return 11; }
	if(value == pattern_mirrored_crosses) { return 12; }
	return 0xFF;
}

static pattern_oneshot_func_t rck_th05_exalice_func(uint8_t id)
{
	if(id == 0) { return 0; }
	if(id <= 8) { return (&EXALICE_PATTERNS[0][0])[id - 1]; }
	switch(id) {
	case 9: return pattern_spreads_and_firewaves;
	case 10: return pattern_bouncing_blue_rings;
	case 11: return pattern_pingpong_lasers;
	case 12: return pattern_mirrored_crosses;
	}
	return 0;
}

static bool rck_th05_firewave(
	replay_ck_stream_t far *stream, rck_firewave_t far *firewave
)
{
	RCK_BOOL(firewave->alive);
	RCK_BOOL(firewave->is_right);
	RCK_S16(firewave->bottom);
	RCK_S16(firewave->amp);
	return true;
}

static bool rck_th05_actor_extra(replay_ck_stream_t far *stream)
{
	uint8_t midboss_id = rck_th05_midbossx_id(midbossx_phase_1_pattern);
	uint8_t exalice_id = rck_th05_exalice_id(exalice_pattern);
	pattern_oneshot_func_t decoded;
	int i;

	if(!rck_u8(stream, &midboss_id)) { return false; }
	decoded = rck_th05_midbossx_func(midboss_id);
	if(rck_th05_midbossx_id(decoded) != midboss_id) { return false; }
	if(rck_applying(stream)) { midbossx_phase_1_pattern = decoded; }
	if(!rck_u8(stream, &exalice_id)) { return false; }
	decoded = rck_th05_exalice_func(exalice_id);
	if(rck_th05_exalice_id(decoded) != exalice_id) { return false; }
	if(rck_applying(stream)) { exalice_pattern = decoded; }
	RCK_U8_MAX(exalice_invincibility_frames, 39);
	if(!rck_pfpoint(stream, &exalice_random_origin)) { return false; }
	RCK_S16(exalice_pattern_origin_x);
	RCK_U16_MAX(exalice_laser_slot, 15);
	for(i = 0; i < 2; i++) {
		if(!rck_th05_firewave(stream, &firewaves[i])) {
			return false;
		}
	}
	RCK_U16(exalice_overlay_patnum);
	return true;
}

static bool rck_group_actors(replay_ck_stream_t far *stream)
{
	if(
		!rck_group_actors_common(stream) ||
		!rck_th05_actor_callbacks(stream)
	) {
		return false;
	}
	switch(stage_id) {
	case 0: return rck_th05_actor_stage1(stream);
	case 1: return rck_th05_actor_stage2(stream);
	case 2: return rck_th05_actor_stage3(stream);
	case 3: return rck_th05_actor_stage4(stream);
	case 4: return rck_th05_actor_stage5(stream);
	case 5: return rck_th05_actor_stage6(stream);
	case STAGE_EXTRA: return rck_th05_actor_extra(stream);
	}
	return false;
}
#endif

bool replay_ck_actor_probe(replay_ck_actor_probe_t far *probe)
{
	uint8_t live_id;

	if(probe == 0) {
		return false;
	}
	probe->midboss_active = midboss_active;
	probe->midboss_finished = (midboss.phase == PHASE_NONE);
	probe->boss_phase = boss.phase;
	if(boss.phase == PHASE_NONE) {
		probe->boss_section = REPLAY_CK_BOSS_SECTION_NONE;
		return true;
	}
	#if (GAME == 5)
		live_id = rck_th05_boss_live_id();
		switch(live_id) {
		case RCK5BL_NONE:
			probe->boss_section = REPLAY_CK_BOSS_SECTION_NONE;
			break;
		case RCK5BL_SETUP:
			probe->boss_section = RCS_TH05_PAIR;
			break;
		case RCK5BL_MAI:
			probe->boss_section = RCS_TH05_MAI;
			break;
		case RCK5BL_YUKI:
			probe->boss_section = RCS_TH05_YUKI;
			break;
		default:
			return false;
		}
	#else
		live_id = rck_th04_boss_update_id();
		if(live_id == 0) {
			probe->boss_section = REPLAY_CK_BOSS_SECTION_NONE;
		} else if(live_id == 1) {
			probe->boss_section = RCS_TH04_MUGETSU;
		} else if(live_id == 2) {
			probe->boss_section = RCS_TH04_GENGETSU;
		} else {
			return false;
		}
	#endif
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
	case RCGI_ACTORS:
		return rck_group_actors(stream);
	case RCGI_ITEMS:
		return rck_group_items(stream);
	case RCGI_SCORING:
		return rck_group_scoring(stream);
	case RCGI_FIELD:
		return rck_group_field(stream);
	case RCGI_EFFECTS:
		return rck_group_effects(stream);
	case RCGI_STAGE_VM:
		return rck_group_stage_vm(stream);
	case RCGI_PACING:
		return rck_group_pacing(stream);
#if (GAME == 5)
	case RCGI_DIALOG:
		return rck_group_dialog(stream);
#endif
	}
	stream->failed = true;
	return false;
}

#define RCK_CONTAINER_SIZE_MAX REPLAY_CHECKPOINT_SIZE_MAX
#define RCK_HEADER_CHECKSUM_OFFSET 0x24u
#define RCK_GROUP_ID_OFFSET 0x00u
#define RCK_GROUP_SCHEMA_OFFSET 0x01u
#define RCK_GROUP_CODEC_OFFSET 0x02u
#define RCK_GROUP_FLAGS_OFFSET 0x03u
#define RCK_GROUP_DATA_OFFSET 0x04u
#define RCK_GROUP_STORED_SIZE_OFFSET 0x08u
#define RCK_GROUP_DECODED_SIZE_OFFSET 0x0Cu
#define RCK_GROUP_CHECKSUM_OFFSET 0x10u

static uint8_t rck_group_count(void)
{
	#if (GAME == 5)
		return REPLAY_CKPT_GROUPS_TH05;
	#else
		return REPLAY_CKPT_GROUPS_TH04;
	#endif
}

static bool rck_identity_valid(const replay_ck_identity_t far *identity)
{
	replay_start_config_t start;

	if(
		(identity == 0) ||
		(identity->stage > STAGE_EXTRA) ||
		(identity->source_fingerprint == 0)
	) {
		return false;
	}
	start.kind = identity->start_kind;
	start.stage = identity->stage;
	start.section = identity->section;
	start.phase = identity->phase;
	start.playchar = playchar;
	return replay_checkpoint_identity_valid(&start);
}

static uint16_t rck_directory_offset(uint8_t group_id)
{
	return static_cast<uint16_t>(
		REPLAY_CHECKPOINT_HEADER_SIZE +
		(static_cast<uint16_t>(group_id) * REPLAY_CHECKPOINT_GROUP_SIZE)
	);
}

static uint8_t rck_get_u8(const uint8_t far *data, uint16_t offset)
{
	return data[offset];
}

static uint16_t rck_get_u16(const uint8_t far *data, uint16_t offset)
{
	return static_cast<uint16_t>(
		static_cast<uint16_t>(data[offset]) |
		(static_cast<uint16_t>(data[offset + 1]) << 8)
	);
}

static uint32_t rck_get_u32(const uint8_t far *data, uint16_t offset)
{
	return (
		static_cast<uint32_t>(data[offset]) |
		(static_cast<uint32_t>(data[offset + 1]) << 8) |
		(static_cast<uint32_t>(data[offset + 2]) << 16) |
		(static_cast<uint32_t>(data[offset + 3]) << 24)
	);
}

static void rck_put_u8(uint8_t far *data, uint16_t offset, uint8_t value)
{
	data[offset] = value;
}

static void rck_put_u16(uint8_t far *data, uint16_t offset, uint16_t value)
{
	data[offset] = static_cast<uint8_t>(value);
	data[offset + 1] = static_cast<uint8_t>(value >> 8);
}

static void rck_put_u32(uint8_t far *data, uint16_t offset, uint32_t value)
{
	int shift;

	for(shift = 0; shift < 32; shift += 8) {
		data[offset++] = static_cast<uint8_t>(value >> shift);
	}
}

static uint32_t rck_hash_u8(uint32_t hash, uint8_t value)
{
	hash ^= static_cast<uint32_t>(value);
	return (hash * REPLAY_FNV1A_PRIME);
}

static uint32_t rck_hash_u32(uint32_t hash, uint32_t value)
{
	int shift;

	for(shift = 0; shift < 32; shift += 8) {
		hash = rck_hash_u8(hash, static_cast<uint8_t>(value >> shift));
	}
	return hash;
}

uint8_t replay_ck_failure_group(void)
{
	return replay_ck_failure_group_value;
}

uint16_t replay_ck_failure_field(void)
{
	return replay_ck_failure_field_value;
}

static bool rck_groups_measure(
	uint16_t far *sizes,
	uint32_t far *checksums,
	uint32_t far *decoded_size,
	uint32_t far *state_digest
)
{
	replay_ck_stream_t stream;
	uint32_t decoded = 0;
	uint32_t digest = REPLAY_FNV1A_BASIS;
	uint8_t count = rck_group_count();
	uint8_t group_id;

	replay_ck_failure_group_value = 0xFF;
	replay_ck_failure_field_value = 0;
	for(group_id = 0; group_id < count; group_id++) {
		replay_ck_failure_group_value = group_id;
		replay_ck_failure_field_value = 0;
		replay_ck_measure_init(&stream);
		if(
			!replay_ck_group_codec(group_id, &stream) ||
			!replay_ck_finish(&stream) ||
			(stream.pos == 0) ||
			(stream.pos > RCK_CONTAINER_SIZE_MAX)
		) {
			return false;
		}
		sizes[group_id] = static_cast<uint16_t>(stream.pos);
		checksums[group_id] = stream.checksum;
		decoded += stream.pos;
		digest = rck_hash_u8(digest, group_id);
		digest = rck_hash_u8(digest, REPLAY_CHECKPOINT_GROUP_SCHEMA);
		digest = rck_hash_u32(digest, stream.pos);

		// Re-run the field codec with the digest as its initial FNV state. This
		// hashes live semantic bytes without allocating a second container.
		replay_ck_measure_init(&stream);
		stream.checksum = digest;
		if(
			!replay_ck_group_codec(group_id, &stream) ||
			!replay_ck_finish(&stream) ||
			(stream.pos != sizes[group_id])
		) {
			return false;
		}
		digest = stream.checksum;
	}
	replay_ck_failure_group_value = 0xFF;
	*decoded_size = decoded;
	*state_digest = digest;
	return true;
}

bool replay_ck_container_measure(
	const replay_ck_identity_t far *identity,
	uint16_t far *total_size,
	uint32_t far *state_digest
)
{
	uint16_t sizes[REPLAY_CKPT_GROUPS_MAX];
	uint32_t checksums[REPLAY_CKPT_GROUPS_MAX];
	uint32_t decoded_size;
	uint32_t total;

	if(
		(total_size == 0) || (state_digest == 0) ||
		!rck_identity_valid(identity) || (identity->stage != stage_id)
	) {
		return false;
	}
	if(!rck_groups_measure(sizes, checksums, &decoded_size, state_digest)) {
		return false;
	}
	total = (
		REPLAY_CHECKPOINT_HEADER_SIZE +
		(static_cast<uint32_t>(rck_group_count()) * REPLAY_CHECKPOINT_GROUP_SIZE) +
		decoded_size
	);
	if(total > RCK_CONTAINER_SIZE_MAX) {
		return false;
	}
	*total_size = static_cast<uint16_t>(total);
	return true;
}

bool replay_ck_container_plan(
	const replay_ck_identity_t far *identity,
	replay_ck_plan_t far *plan
)
{
	uint32_t total;
	uint8_t count = rck_group_count();
	uint8_t group_id;

	if(
		(plan == 0) || !rck_identity_valid(identity) ||
		(identity->stage != stage_id)
	) {
		return false;
	}
	for(group_id = 0; group_id < REPLAY_CKPT_GROUPS_MAX; group_id++) {
		plan->group_sizes[group_id] = 0;
		plan->group_checksums[group_id] = 0;
	}
	if(!rck_groups_measure(
		plan->group_sizes, plan->group_checksums,
		&plan->decoded_size, &plan->state_digest
	)) {
		return false;
	}
	plan->prefix_size = static_cast<uint16_t>(
		REPLAY_CHECKPOINT_HEADER_SIZE +
		(static_cast<uint16_t>(count) * REPLAY_CHECKPOINT_GROUP_SIZE)
	);
	total = static_cast<uint32_t>(plan->prefix_size) + plan->decoded_size;
	if(total > RCK_CONTAINER_SIZE_MAX) {
		return false;
	}
	plan->total_size = static_cast<uint16_t>(total);
	plan->container_checksum = 0;
	return true;
}

bool replay_ck_container_prefix_encode(
	const replay_ck_identity_t far *identity,
	const replay_ck_plan_t far *plan,
	void far *data_in,
	uint16_t capacity
)
{
	uint8_t far *data = reinterpret_cast<uint8_t far *>(data_in);
	uint16_t expected_prefix = static_cast<uint16_t>(
		REPLAY_CHECKPOINT_HEADER_SIZE +
		(static_cast<uint16_t>(rck_group_count()) *
		 REPLAY_CHECKPOINT_GROUP_SIZE)
	);
	uint16_t offset;
	uint16_t entry;
	uint8_t group_id;

	if(
		(data == 0) || (plan == 0) || !rck_identity_valid(identity) ||
		(identity->stage != stage_id) ||
		(plan->prefix_size != expected_prefix) ||
		(capacity < plan->prefix_size) ||
		(static_cast<uint32_t>(plan->prefix_size) + plan->decoded_size !=
		 plan->total_size)
	) {
		return false;
	}
	for(offset = 0; offset < plan->prefix_size; offset++) {
		data[offset] = 0;
	}
	data[0] = 'T'; data[1] = ('0' + GAME); data[2] = 'C'; data[3] = 'K';
	data[4] = 'P'; data[5] = '1'; data[6] = '\0'; data[7] = '\0';
	rck_put_u16(data, 0x08, REPLAY_CHECKPOINT_SCHEMA);
	rck_put_u16(data, 0x0A, REPLAY_CHECKPOINT_HEADER_SIZE);
	rck_put_u8(data, 0x0C, GAME);
	rck_put_u8(data, 0x0D, identity->start_kind);
	rck_put_u8(data, 0x0E, identity->stage);
	rck_put_u8(data, 0x0F, identity->section);
	rck_put_u8(data, 0x10, identity->phase);
	rck_put_u8(data, 0x11, rck_group_count());
	rck_put_u32(data, 0x14, plan->total_size);
	rck_put_u32(data, 0x18, identity->source_fingerprint);
	rck_put_u32(data, 0x1C, plan->state_digest);
	rck_put_u32(data, 0x20, plan->decoded_size);
	rck_put_u32(data, RCK_HEADER_CHECKSUM_OFFSET, 0);

	offset = plan->prefix_size;
	for(group_id = 0; group_id < rck_group_count(); group_id++) {
		entry = rck_directory_offset(group_id);
		if(
			(plan->group_sizes[group_id] == 0) ||
			(static_cast<uint32_t>(offset) + plan->group_sizes[group_id] >
			 plan->total_size)
		) {
			return false;
		}
		rck_put_u8(data, entry + RCK_GROUP_ID_OFFSET, group_id);
		rck_put_u8(
			data, entry + RCK_GROUP_SCHEMA_OFFSET,
			REPLAY_CHECKPOINT_GROUP_SCHEMA
		);
		rck_put_u8(data, entry + RCK_GROUP_CODEC_OFFSET, RCC_RAW);
		rck_put_u32(data, entry + RCK_GROUP_DATA_OFFSET, offset);
		rck_put_u32(
			data, entry + RCK_GROUP_STORED_SIZE_OFFSET,
			plan->group_sizes[group_id]
		);
		rck_put_u32(
			data, entry + RCK_GROUP_DECODED_SIZE_OFFSET,
			plan->group_sizes[group_id]
		);
		rck_put_u32(
			data, entry + RCK_GROUP_CHECKSUM_OFFSET,
			plan->group_checksums[group_id]
		);
		offset = static_cast<uint16_t>(
			offset + plan->group_sizes[group_id]
		);
	}
	return (offset == plan->total_size);
}

void replay_ck_container_prefix_checksum_set(
	void far *data, uint32_t checksum
)
{
	if(data != 0) {
		rck_put_u32(
			reinterpret_cast<uint8_t far *>(data),
			RCK_HEADER_CHECKSUM_OFFSET, checksum
		);
	}
}

static bool rck_container_prefix_valid(
	const replay_ck_identity_t far *identity,
	const uint8_t far *data,
	uint16_t total_size
)
{
	return (
		(data != 0) && rck_identity_valid(identity) &&
		(total_size <= RCK_CONTAINER_SIZE_MAX) &&
		(total_size >= (REPLAY_CHECKPOINT_HEADER_SIZE +
		 (rck_group_count() * REPLAY_CHECKPOINT_GROUP_SIZE))) &&
		(data[0] == 'T') && (data[1] == ('0' + GAME)) &&
		(data[2] == 'C') && (data[3] == 'K') &&
		(data[4] == 'P') && (data[5] == '1') &&
		(data[6] == 0) && (data[7] == 0) &&
		(rck_get_u16(data, 0x08) == REPLAY_CHECKPOINT_SCHEMA) &&
		(rck_get_u16(data, 0x0A) == REPLAY_CHECKPOINT_HEADER_SIZE) &&
		(rck_get_u8(data, 0x0C) == GAME) &&
		(rck_get_u8(data, 0x0D) == identity->start_kind) &&
		(rck_get_u8(data, 0x0E) == identity->stage) &&
		(rck_get_u8(data, 0x0F) == identity->section) &&
		(rck_get_u8(data, 0x10) == identity->phase) &&
		(rck_get_u8(data, 0x11) == rck_group_count()) &&
		(rck_get_u16(data, 0x12) == 0) &&
		(rck_get_u32(data, 0x14) == total_size) &&
		(rck_get_u32(data, 0x18) == identity->source_fingerprint)
	);
}

bool replay_ck_container_prefix_validate(
	const replay_ck_identity_t far *identity,
	const void far *data_in,
	uint16_t prefix_size,
	uint16_t total_size,
	replay_ck_plan_t far *plan
)
{
	const uint8_t far *data = reinterpret_cast<const uint8_t far *>(data_in);
	uint16_t expected_prefix = static_cast<uint16_t>(
		REPLAY_CHECKPOINT_HEADER_SIZE +
		(static_cast<uint16_t>(rck_group_count()) *
		 REPLAY_CHECKPOINT_GROUP_SIZE)
	);
	uint32_t decoded_size = 0;
	uint32_t stored_size;
	uint16_t next_offset;
	uint16_t entry;
	uint8_t group_id;

	if(
		(plan == 0) || (prefix_size != expected_prefix) ||
		!rck_container_prefix_valid(identity, data, total_size)
	) {
		return false;
	}
	for(group_id = 0; group_id < REPLAY_CKPT_GROUPS_MAX; group_id++) {
		plan->group_sizes[group_id] = 0;
		plan->group_checksums[group_id] = 0;
	}
	plan->total_size = total_size;
	plan->prefix_size = prefix_size;
	plan->decoded_size = rck_get_u32(data, 0x20);
	plan->state_digest = rck_get_u32(data, 0x1C);
	plan->container_checksum = rck_get_u32(
		data, RCK_HEADER_CHECKSUM_OFFSET
	);
	next_offset = prefix_size;
	for(group_id = 0; group_id < rck_group_count(); group_id++) {
		entry = rck_directory_offset(group_id);
		stored_size = rck_get_u32(data, entry + RCK_GROUP_STORED_SIZE_OFFSET);
		if(
			(rck_get_u8(data, entry + RCK_GROUP_ID_OFFSET) != group_id) ||
			(rck_get_u8(data, entry + RCK_GROUP_SCHEMA_OFFSET) !=
			 REPLAY_CHECKPOINT_GROUP_SCHEMA) ||
			(rck_get_u8(data, entry + RCK_GROUP_CODEC_OFFSET) != RCC_RAW) ||
			(rck_get_u8(data, entry + RCK_GROUP_FLAGS_OFFSET) != 0) ||
			(stored_size == 0) || (stored_size > RCK_CONTAINER_SIZE_MAX) ||
			(rck_get_u32(data, entry + RCK_GROUP_DECODED_SIZE_OFFSET) !=
			 stored_size) ||
			(rck_get_u32(data, entry + RCK_GROUP_DATA_OFFSET) != next_offset) ||
			((static_cast<uint32_t>(next_offset) + stored_size) > total_size)
		) {
			return false;
		}
		plan->group_sizes[group_id] = static_cast<uint16_t>(stored_size);
		plan->group_checksums[group_id] = rck_get_u32(
			data, entry + RCK_GROUP_CHECKSUM_OFFSET
		);
		decoded_size += stored_size;
		next_offset = static_cast<uint16_t>(next_offset + stored_size);
	}
	return (
		(next_offset == total_size) &&
		(decoded_size == plan->decoded_size)
	);
}

static bool rck_group_stream(
	uint8_t group_id,
	void far *buffer,
	uint16_t buffer_size,
	uint16_t group_size,
	uint32_t expected_checksum,
	replay_ck_mode_t mode,
	replay_ck_io_func_t io_func,
	uint16_t context
)
{
	replay_ck_stream_t stream;

	if(
		(buffer == 0) || (buffer_size == 0) || (group_size == 0) ||
		(group_id >= rck_group_count()) || (io_func == 0)
	) {
		return false;
	}
	replay_ck_failure_group_value = group_id;
	replay_ck_failure_field_value = 0;
	rck_stream_io_init(
		&stream, buffer, buffer_size, group_size, mode, io_func, context
	);
	if(!replay_ck_group_codec(group_id, &stream)) {
		return false;
	}
	if(!replay_ck_finish(&stream)) {
		replay_ck_failure_field_value = 0xF001;
		return false;
	}
	if(stream.checksum != expected_checksum) {
		replay_ck_failure_field_value = 0xF002;
		return false;
	}
	replay_ck_failure_group_value = 0xFF;
	return true;
}

bool replay_ck_group_encode_stream(
	uint8_t group_id,
	void far *buffer,
	uint16_t buffer_size,
	uint16_t group_size,
	uint32_t expected_checksum,
	replay_ck_io_func_t write_func,
	uint16_t context
)
{
	return rck_group_stream(
		group_id, buffer, buffer_size, group_size, expected_checksum,
		RCK_ENCODE, write_func, context
	);
}

bool replay_ck_group_validate_stream(
	uint8_t group_id,
	void far *buffer,
	uint16_t buffer_size,
	uint16_t group_size,
	uint32_t expected_checksum,
	replay_ck_io_func_t read_func,
	uint16_t context
)
{
	return rck_group_stream(
		group_id, buffer, buffer_size, group_size, expected_checksum,
		RCK_VALIDATE, read_func, context
	);
}

bool replay_ck_group_apply_stream(
	uint8_t group_id,
	void far *buffer,
	uint16_t buffer_size,
	uint16_t group_size,
	uint32_t expected_checksum,
	replay_ck_io_func_t read_func,
	uint16_t context
)
{
	return rck_group_stream(
		group_id, buffer, buffer_size, group_size, expected_checksum,
		RCK_APPLY, read_func, context
	);
}

uint32_t replay_ck_group_digest_begin(
	uint32_t digest, uint8_t group_id, uint16_t size
)
{
	if((size == 0) || (group_id >= rck_group_count())) {
		return 0;
	}
	digest = rck_hash_u8(digest, group_id);
	digest = rck_hash_u8(digest, REPLAY_CHECKPOINT_GROUP_SCHEMA);
	return rck_hash_u32(digest, size);
}

#undef RCK_CONTAINER_SIZE_MAX
#undef RCK_HEADER_CHECKSUM_OFFSET
#undef RCK_GROUP_ID_OFFSET
#undef RCK_GROUP_SCHEMA_OFFSET
#undef RCK_GROUP_CODEC_OFFSET
#undef RCK_GROUP_FLAGS_OFFSET
#undef RCK_GROUP_DATA_OFFSET
#undef RCK_GROUP_STORED_SIZE_OFFSET
#undef RCK_GROUP_DECODED_SIZE_OFFSET
#undef RCK_GROUP_CHECKSUM_OFFSET

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
#undef RCK_BOOL16
#undef RCK_REQUIRE_FIELD_SIZE
#undef RCK_FIELD_PTR

// Keep the Borland runtime code that follows the replay segments on its stock
// paragraph phase. This value is derived from the complete MAIN map, not from
// the source length of this translation unit in isolation.
#if (GAME == 4)
	#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
	#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
	#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
	#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
	#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
	#pragma codestring "\x90"
	#pragma codestring "\x90\x90\x90\x90\x90\x90\x90"
#else
	#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
	#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
	#pragma codestring "\x90\x90\x90\x90\x90"
	#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
	#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
	#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90"
	#pragma codestring "\x90\x90\x90\x90\x90\x90\x90"
#endif
