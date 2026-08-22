#pragma option -zCREPLAY_TEXT

#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "libs/sprite16/sprite16.h"
#include "platform.h"
#include "platform/x86real/pc98/keyboard.hpp"
#include "th02/hardware/pages.hpp"
#include "th02/hardware/frmdelay.h"
#include "th02/math/randring.hpp"
#include "th03/hardware/palette.hpp"
#include "th03/main/defeat.hpp"
#include "th03/main/difficul.hpp"
#include "th03/main/enemy/enemy.hpp"
#include "th03/main/enemy/fireball.hpp"
#include "th03/main/hud/dynamic.hpp"
#include "th03/main/playfld.hpp"
#include "th03/main/player/bomb.hpp"
#include "th03/main/player/cpu.hpp"
#include "th03/main/player/chain.hpp"
#include "th03/main/player/combo.hpp"
#include "th03/main/player/exatt.hpp"
#include "th03/main/player/gba.hpp"
#include "th03/main/player/stuff.hpp"
#include "th03/main/replay.hpp"
#include "th03/main/sprite16.hpp"
#include "th03/language_main.hpp"
#include "th03/main/round.hpp"
#include "th03/main/score.hpp"
#include "th03/main/v_colors.hpp"
#include "th03/fast_forward.hpp"
#include "th03/keyconfig.hpp"
#include "th03/menu_font.hpp"
#include "th03/practice.hpp"
#include "th03/replay_build.hpp"
#include "th03/pixel_capture.hpp"
#include "th03/replay_format.hpp"
#include "th03/replay_handoff.hpp"
#include "th03/scorefile.hpp"
#include "th03/resident.hpp"
#include "th03/replay_protect.hpp"
#include "th03/snd/snd.h"
#include "x86real.h"

// Pack the pending packet size and stage-checkpoint state into RLE phase bits.
#define REPLAY_RLE_PHASE_MASK 0x03
#define REPLAY_RLE_PACKET_SIZE_SHIFT 2
#define REPLAY_RLE_PACKET_SIZE_MASK 0x3C
#define REPLAY_RLE_STAGE_CHECKPOINT_PENDING_MASK 0x40
#define REPLAY_RLE_STATE_OPEN 0x01
#define REPLAY_RLE_STATE_CHARGE_SHIFT 1
#define REPLAY_RLE_STATE_CHARGE_MASK 0x06
#define REPLAY_RECORD_BUFFER_SIZE ( \
	T3_REPLAY_WRITE_BUFFER_SIZE + T3R_STAGE_CKPT_PREFIX_SIZE \
)
#define REPLAY_SEEK_BUFFER_SIZE 4096
#define REPLAY_ACCEL_HASH_COUNT 4096
#define REPLAY_ACCEL_HASH_BYTES (REPLAY_ACCEL_HASH_COUNT * sizeof(uint16_t))
#define REPLAY_ACCEL_MATCH_MIN 3
#define REPLAY_ACCEL_MATCH_MAX 18

extern "C" const unsigned char aCOul[];
extern "C" int file_Handle;
#if defined(TH03_PIXEL_CAPTURE)
extern "C" Palette8 palette_1F2F4;
#endif

static char T3_REPLAY_CFG_FN[13];
static char T3_INPUT_FN[12];
static char T3_SPLIT_FN[12];
static char T3_DONE_FN[11];
static char T3_GUARD_DIAG_FN[12];
static char T3_USER_REPLAY_DIR[7];
static char T3_USER_REPLAY_INDEX_FN[16];
static char T3_USER_REPLAY_SLOT_FN[18];
static char T3_USER_REPLAY_FALLBACK_FN[12];
static char T3_ACCEL_TEMP_FN[10];
#if defined(TH03_REPLAY_DEVTOOLS)
static char T3_STATE_REFERENCE_FN[12];
static char T3_STATE_LIVE_FN[11];
#endif

enum replay_text_id_t {
	RTX_CRLF,
	RTX_ERROR,
	RTX_ERROR_SPLIT_OPEN,
	RTX_ERROR_INPUT_CREATE,
	RTX_ERROR_USER_CREATE,
	RTX_ERROR_USER_HEADER,
	RTX_ERROR_INPUT_HEADER,
	RTX_OK_USER_INPUT_END,
	RTX_OK_INPUT_END,
	RTX_ERROR_FRAME_IO,
	RTX_OK_MENU_RETURN,
	RTX_OK_MENU_RETURN_NOSAVE,
	RTX_OK_PARTIAL,
	RTX_OK_USER_PLAYBACK,
	RTX_OK,
};

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

struct replay_seek_reader_t {
	uint8_t far *buffer;
	uint16_t cursor;
	uint16_t size;
	uint32_t remaining;
};

static replay_mode_t replay_mode;
static replay_input_header_t replay_header;
static replay_user_header_t replay_user_header;
static replay_user_summary_ext_t replay_user_summary_ext;
static replay_user_identity_ext_t replay_user_identity_ext;
static replay_user_snapshot_t replay_user_snapshot;
static replay_user_round_state_t replay_user_round_state;
static replay_user_round_carry_t replay_user_round_carry;
static replay_user_index_header_t replay_user_index_header;
static replay_user_index_entry_t replay_user_index_entry;
static const char *replay_user_fn;
static uint8_t replay_user_slot;
static uint32_t replay_sample_count;
static uint32_t replay_global_frame;
static uint32_t replay_round_real_frames;
static uint16_t replay_round_vsync_last;
static uint32_t replay_input_byte_count;
static uint16_t replay_write_buffer_size;
static uint16_t replay_write_buffer_seg;
static uint8_t replay_last_route;
static uint8_t replay_rle_phase;
static uint8_t replay_rle_run;
static uint16_t replay_rle_input_mp_p1;
static uint16_t replay_rle_input_mp_p2;
static uint16_t replay_rle_input_sp;
static bool replay_done_written;
static bool replay_paths_initialized;
static bool replay_prompt_skip_queued;
static uint8_t replay_rle_packet_state;
static bool replay_user_discard_requested;
static bool replay_guard_diag_written;
static bool replay_restart_requested_flag;
static uint16_t replay_accel_raw_seg;
static uint8_t replay_accel_target_checkpoint;
static uint8_t replay_accel_group[17];
static replay_user_accel_footer_t replay_accel_footer;
static uint16_t replay_sum_flags;
static uint8_t replay_sum_route;
static uint8_t replay_sum_mode;
static uint8_t replay_sum_stage;
static uint8_t replay_sum_round;
static uint8_t replay_sum_winner;
static uint8_t replay_sum_lives;
static uint8_t replay_sum_misses;
static uint8_t replay_sum_stage_count;
static uint8_t replay_sum_stage_opps[T3_REPLAY_USER_STAGE_COUNT];
static uint8_t replay_sum_stage_scores[
	T3_REPLAY_USER_STAGE_COUNT
][T3_REPLAY_USER_PACKED_SCORE_SIZE];
#if defined(TH03_REPLAY_DEVTOOLS)
static bool replay_state_probe_pending;
static uint8_t replay_state_probe_checkpoint;
#endif
extern "C" unsigned char score[];
extern "C" unsigned char byte_220FC[PLAYER_COUNT];
extern "C" signed char byte_20E48;
extern "C" bool boss_panic_fired_in_current_combo[PLAYER_COUNT];
extern "C" uint8_t byte_23DF9;
extern "C" uint8_t defeat_combo_hits_max;
extern "C" uint8_t defeat_gauge_attacks_fired;
extern "C" uint8_t defeat_boss_attacks_fired;
extern "C" uint8_t defeat_boss_attacks_reversed;
extern "C" uint8_t defeat_boss_panics_fired;
extern uint8_t byte_23AF9;
extern uint8_t byte_23B00;
extern uint8_t randring_p;
extern uint8_t formation_p[PLAYER_COUNT];
extern uint8_t __seg *formation_type_ring;
extern uint8_t __seg *formation_pos_type_ring;
extern uint8_t __seg *enedat_2;
extern uint8_t __seg *enedat;
extern uint8_t __seg *formation_scripts;
extern uint8_t formation_count;
extern farfunc_t_near farfp_20F24;
extern farfunc_t_near farfp_20F20;
extern farfunc_t_near farfp_20F28;
extern farfunc_t_near callback_205CE[PLAYER_COUNT];
extern farfunc_t_near bomb_func[PLAYER_COUNT];
extern nearfunc_t_near fp_1FBC0;
extern "C" unsigned int TextShown;

extern "C" void pascal far sub_D1E7(void);
extern "C" void pascal far sub_D3F9(void);
extern "C" void pascal near sub_B4A3(void);
extern "C" void pascal near sub_B4A8(void);
extern "C" void pascal near sub_B60A(void);
extern "C" void pascal far SUB_CA3C(void);
extern "C" void near sub_C830(void);
extern "C" void near sub_C8C4(void);
extern "C" void pascal near SUB_D50E(void);
void near hitcircles_render(void);
void bullets_render(void);

struct replay_accel_pointer_state_t {
	uint16_t enedat_2_seg;
	uint16_t enedat_seg;
	uint16_t formation_scripts_seg;
	uint16_t formation_type_seg;
	uint16_t formation_pos_type_seg;
	farfunc_t_near gba_boss_update[PLAYER_COUNT];
	farfunc_t_near gba_boss_render[PLAYER_COUNT];
	exatt_funcs_t exatt_funcs[PLAYER_COUNT];
	farfunc_t_near chargeshot_update[PLAYER_COUNT];
	farfunc_t_near chargeshot_render[PLAYER_COUNT];
	chargeshot_hittest_func_t chargeshot_hittest[PLAYER_COUNT];
	farfunc_t_near gba_gauge_pattern_pellet[PLAYER_COUNT];
	farfunc_t_near gba_gauge_pattern_bullet[PLAYER_COUNT];
	farfunc_t_near callback_205CE[PLAYER_COUNT];
	farfunc_t_near bomb_func[PLAYER_COUNT];
	farfunc_t_near farfp_20F20;
	farfunc_t_near farfp_20F24;
	farfunc_t_near farfp_20F28;
	nearfunc_t_near hyper[PLAYER_COUNT];
	nearfunc_t_near hyper_func[PLAYER_COUNT];
	chargeshot_add_func_t chargeshot_add[PLAYER_COUNT];
};

static replay_accel_pointer_state_t replay_accel_pointers;

static replay_mode_t replay_cfg_mode(void);
static replay_mode_t replay_resident_mode(void);
static void replay_paths_init(void);
static void replay_write_text(replay_text_id_t text);
static void replay_handoff_cursor_store(void);
static void replay_resident_handoff_clear(void);
static void replay_user_sample_commit(void);
static bool replay_user_header_is_rle(void);
static bool replay_user_play_sample(void);
static bool replay_user_play_interstitial_sample(void);
static void replay_user_carry_chains_fill(void);
static void replay_user_carry_chains_restore(void);
static bool replay_user_snapshot_disk_read(void);
static bool replay_write_bytes_checked(const void far *buf, unsigned size);
static void replay_accel_temps_delete(void);
static bool replay_accel_capture(uint8_t checkpoint);
static bool replay_accel_prepare(uint8_t checkpoint);
static bool replay_accel_direct_start(void);
#if defined(TH03_REPLAY_DEVTOOLS)
static void replay_debug_transition_write(
	uint8_t code, uint8_t requested_phase
);
#endif

static uint8_t replay_user_background_phase(void)
{
	if(farfp_20F24 == sub_D1E7) {
		return T3R_BACKGROUND_TYPE_A_STEADY;
	}
	if(farfp_20F24 == sub_D3F9) {
		return T3R_BACKGROUND_TYPE_B_STEADY;
	}
	return T3R_BACKGROUND_INITIAL;
}

static uint8_t replay_user_result_phase(void)
{
	if(fp_1FBC0 == sub_B4A8) {
		return T3R_RESULT_OPENING;
	}
	if(fp_1FBC0 == sub_B60A) {
		return T3R_RESULT_CLOSING;
	}
	return T3R_RESULT_IDLE;
}

static void replay_memclear(void far *buf, unsigned size)
{
	uint8_t far *p = reinterpret_cast<uint8_t far *>(buf);
	while(size != 0) {
		*p++ = 0;
		size--;
	}
}

static void replay_copy_far(
	void far *dst, const void far *src, uint16_t size
)
{
	uint8_t far *d = reinterpret_cast<uint8_t far *>(dst);
	const uint8_t far *s = reinterpret_cast<const uint8_t far *>(src);
	while(size != 0) {
		*d++ = *s++;
		size--;
	}
}

static void replay_accel_temp_fn_set(uint8_t checkpoint)
{
	T3_ACCEL_TEMP_FN[4] = static_cast<char>(
		(checkpoint < 10) ? ('0' + checkpoint) : ('A' + checkpoint - 10)
	);
}

static void replay_accel_file_delete(const char far *fn)
{
	asm {
		push	ds
		lds 	dx, fn
		mov 	ah, 41h
		int 	21h
		pop 	ds
	}
}

static long replay_accel_file_size(void)
{
	long size;

	file_seek(0, SEEK_END);
	asm {
		mov 	bx, file_Handle
		mov 	ax, 4201h
		xor 	cx, cx
		xor 	dx, dx
		int 	21h
		mov 	word ptr size, ax
		mov 	word ptr size+2, dx
	}
	return size;
}

static void replay_accel_temps_delete(void)
{
	uint8_t checkpoint;

	for(checkpoint = 0; checkpoint < T3R_CKPT_COUNT_MAX; checkpoint++) {
		replay_accel_temp_fn_set(checkpoint);
		replay_accel_file_delete(T3_ACCEL_TEMP_FN);
	}
}

static uint32_t replay_accel_hash(const uint8_t far *raw)
{
	uint32_t hash = 5381;
	uint16_t i;

	for(i = 0; i < T3R_ACCEL_RAW_SIZE; i++) {
		hash = ((hash << 5) + hash) ^ raw[i];
	}
	return hash;
}

#if defined(TH03_PIXEL_CAPTURE)
static uint16_t replay_accel_bss_offset(void)
{
	// T3R accelerator images contain the original MAIN state beginning at
	// palette_1F2F4. T3PIX adds capture-only BSS ahead of that state, so bind
	// the image to the symbol rather than the release build's numeric offset.
	return FP_OFF(&palette_1F2F4);
}
#define REPLAY_ACCEL_BSS_OFFSET replay_accel_bss_offset()
#else
#define REPLAY_ACCEL_BSS_OFFSET T3R_ACCEL_BSS_OFFSET
#endif

static void replay_accel_raw_fill(uint8_t far *raw)
{
	uint16_t dgroup_seg = FP_SEG(&replay_mode);

	replay_copy_far(
		raw,
		MK_FP(dgroup_seg, REPLAY_ACCEL_BSS_OFFSET),
		T3R_ACCEL_BSS_SIZE
	);
	replay_copy_far(
		(raw + T3R_ACCEL_BSS_SIZE),
		formation_type_ring,
		T3_REPLAY_USER_FORMATION_RING_SIZE
	);
	replay_copy_far(
		(
			raw +
			T3R_ACCEL_BSS_SIZE +
			T3_REPLAY_USER_FORMATION_RING_SIZE
		),
		formation_pos_type_ring,
		T3_REPLAY_USER_FORMATION_RING_SIZE
	);
}

static void replay_accel_pointers_capture(void)
{
	int i;

	replay_accel_pointers.enedat_2_seg = reinterpret_cast<uint16_t>(enedat_2);
	replay_accel_pointers.enedat_seg = reinterpret_cast<uint16_t>(enedat);
	replay_accel_pointers.formation_scripts_seg = (
		reinterpret_cast<uint16_t>(formation_scripts)
	);
	replay_accel_pointers.formation_type_seg = (
		reinterpret_cast<uint16_t>(formation_type_ring)
	);
	replay_accel_pointers.formation_pos_type_seg = (
		reinterpret_cast<uint16_t>(formation_pos_type_ring)
	);
	for(i = 0; i < PLAYER_COUNT; i++) {
		replay_accel_pointers.gba_boss_update[i] = gba_boss_update[i];
		replay_accel_pointers.gba_boss_render[i] = gba_boss_render[i];
		replay_accel_pointers.exatt_funcs[i].add = exatt_funcs[i].add;
		replay_accel_pointers.exatt_funcs[i].update = exatt_funcs[i].update;
		replay_accel_pointers.exatt_funcs[i].render = exatt_funcs[i].render;
		replay_accel_pointers.chargeshot_update[i] = chargeshot_update[i];
		replay_accel_pointers.chargeshot_render[i] = chargeshot_render[i];
		replay_accel_pointers.chargeshot_hittest[i] = chargeshot_hittest[i];
		replay_accel_pointers.gba_gauge_pattern_pellet[i] = (
			gba_gauge_pattern_pellet[i]
		);
		replay_accel_pointers.gba_gauge_pattern_bullet[i] = (
			gba_gauge_pattern_bullet[i]
		);
		replay_accel_pointers.callback_205CE[i] = callback_205CE[i];
		replay_accel_pointers.bomb_func[i] = bomb_func[i];
		replay_accel_pointers.hyper[i] = players[i].hyper;
		replay_accel_pointers.hyper_func[i] = players[i].hyper_func;
		replay_accel_pointers.chargeshot_add[i] = players[i].chargeshot_add;
	}
	replay_accel_pointers.farfp_20F20 = farfp_20F20;
	replay_accel_pointers.farfp_20F24 = farfp_20F24;
	replay_accel_pointers.farfp_20F28 = farfp_20F28;
}

static void replay_accel_pointers_restore(void)
{
	int i;

	enedat_2 = reinterpret_cast<uint8_t __seg *>(
		replay_accel_pointers.enedat_2_seg
	);
	enedat = reinterpret_cast<uint8_t __seg *>(
		replay_accel_pointers.enedat_seg
	);
	formation_scripts = reinterpret_cast<uint8_t __seg *>(
		replay_accel_pointers.formation_scripts_seg
	);
	formation_type_ring = reinterpret_cast<uint8_t __seg *>(
		replay_accel_pointers.formation_type_seg
	);
	formation_pos_type_ring = reinterpret_cast<uint8_t __seg *>(
		replay_accel_pointers.formation_pos_type_seg
	);
	for(i = 0; i < PLAYER_COUNT; i++) {
		gba_boss_update[i] = replay_accel_pointers.gba_boss_update[i];
		gba_boss_render[i] = replay_accel_pointers.gba_boss_render[i];
		exatt_funcs[i].add = replay_accel_pointers.exatt_funcs[i].add;
		exatt_funcs[i].update = replay_accel_pointers.exatt_funcs[i].update;
		exatt_funcs[i].render = replay_accel_pointers.exatt_funcs[i].render;
		chargeshot_update[i] = replay_accel_pointers.chargeshot_update[i];
		chargeshot_render[i] = replay_accel_pointers.chargeshot_render[i];
		chargeshot_hittest[i] = replay_accel_pointers.chargeshot_hittest[i];
		gba_gauge_pattern_pellet[i] = (
			replay_accel_pointers.gba_gauge_pattern_pellet[i]
		);
		gba_gauge_pattern_bullet[i] = (
			replay_accel_pointers.gba_gauge_pattern_bullet[i]
		);
		callback_205CE[i] = replay_accel_pointers.callback_205CE[i];
		bomb_func[i] = replay_accel_pointers.bomb_func[i];
		players[i].hyper = replay_accel_pointers.hyper[i];
		players[i].hyper_func = replay_accel_pointers.hyper_func[i];
		players[i].chargeshot_add = replay_accel_pointers.chargeshot_add[i];
	}
	farfp_20F20 = replay_accel_pointers.farfp_20F20;
	farfp_20F24 = replay_accel_pointers.farfp_20F24;
	farfp_20F28 = replay_accel_pointers.farfp_20F28;
}

static void replay_accel_raw_apply(const uint8_t far *raw)
{
	uint16_t dgroup_seg = FP_SEG(&replay_mode);

	replay_accel_pointers_capture();
	replay_copy_far(
		MK_FP(dgroup_seg, REPLAY_ACCEL_BSS_OFFSET),
		raw,
		T3R_ACCEL_BSS_SIZE
	);
	replay_accel_pointers_restore();
	replay_copy_far(
		formation_type_ring,
		(raw + T3R_ACCEL_BSS_SIZE),
		T3_REPLAY_USER_FORMATION_RING_SIZE
	);
	replay_copy_far(
		formation_pos_type_ring,
		(
			raw +
			T3R_ACCEL_BSS_SIZE +
			T3_REPLAY_USER_FORMATION_RING_SIZE
		),
		T3_REPLAY_USER_FORMATION_RING_SIZE
	);
}

static bool replay_accel_capture(uint8_t checkpoint)
{
	replay_user_accel_temp_header_t header;
	uint16_t raw_seg;
	uint16_t hash_seg;
	uint8_t far *raw;
	uint16_t far *hash_heads;
	uint16_t pos = 0;
	uint16_t candidate;
	uint16_t distance;
	uint16_t match_len;
	uint16_t group_size;
	uint16_t packed_size = 0;
	uint16_t hash;
	uint8_t token;
	uint8_t flags;
	bool ok = false;

	raw_seg = reinterpret_cast<uint16_t>(hmem_allocbyte(T3R_ACCEL_RAW_SIZE));
	if(raw_seg == 0) {
		return false;
	}
	hash_seg = reinterpret_cast<uint16_t>(
		hmem_allocbyte(REPLAY_ACCEL_HASH_BYTES)
	);
	if(hash_seg == 0) {
		hmem_free(reinterpret_cast<void __seg *>(raw_seg));
		return false;
	}
	raw = reinterpret_cast<uint8_t far *>(MK_FP(raw_seg, 0));
	hash_heads = reinterpret_cast<uint16_t far *>(MK_FP(hash_seg, 0));
	replay_accel_raw_fill(raw);
	for(hash = 0; hash < REPLAY_ACCEL_HASH_COUNT; hash++) {
		hash_heads[hash] = 0xFFFF;
	}
	replay_memclear(&header, sizeof(header));
	header.magic[0] = 'T';
	header.magic[1] = '3';
	header.magic[2] = 'C';
	header.magic[3] = '1';
	header.checkpoint = checkpoint;
	header.codec = T3R_ACCEL_CODEC_LZSS4K;
	header.header_size = sizeof(header);
	header.raw_size = T3R_ACCEL_RAW_SIZE;
	header.state_hash = replay_accel_hash(raw);
	replay_accel_temp_fn_set(checkpoint);
	replay_accel_file_delete(T3_ACCEL_TEMP_FN);
	if(!file_create(T3_ACCEL_TEMP_FN)) {
		goto cleanup;
	}
	if(!replay_write_bytes_checked(&header, sizeof(header))) {
		goto close;
	}
	while(pos < T3R_ACCEL_RAW_SIZE) {
		flags = 0;
		group_size = 1;
		for(token = 0; (token < 8) && (pos < T3R_ACCEL_RAW_SIZE); token++) {
			match_len = 0;
			distance = 0;
			if((pos + 2) < T3R_ACCEL_RAW_SIZE) {
				hash = static_cast<uint16_t>(
					(
						(static_cast<uint16_t>(raw[pos]) * 251U) +
						raw[pos + 1]
					) * 251U + raw[pos + 2]
				) & (REPLAY_ACCEL_HASH_COUNT - 1);
				candidate = hash_heads[hash];
				hash_heads[hash] = pos;
				if(
					(candidate != 0xFFFF) &&
					((pos - candidate) <= 4095)
				) {
					while(
						(match_len < REPLAY_ACCEL_MATCH_MAX) &&
						((pos + match_len) < T3R_ACCEL_RAW_SIZE) &&
						(raw[candidate + match_len] ==
						 raw[pos + match_len])
					) {
						match_len++;
					}
					if(match_len >= REPLAY_ACCEL_MATCH_MIN) {
						distance = static_cast<uint16_t>(pos - candidate);
					} else {
						match_len = 0;
					}
				}
			}
			if(match_len != 0) {
				replay_accel_group[group_size++] = static_cast<uint8_t>(
					distance
				);
				replay_accel_group[group_size++] = static_cast<uint8_t>(
					((distance >> 8) << 4) |
					(match_len - REPLAY_ACCEL_MATCH_MIN)
				);
				pos += match_len;
			} else {
				flags |= (1 << token);
				replay_accel_group[group_size++] = raw[pos++];
			}
		}
		replay_accel_group[0] = flags;
		if(!replay_write_bytes_checked(replay_accel_group, group_size)) {
			goto close;
		}
		packed_size = static_cast<uint16_t>(packed_size + group_size);
	}
	header.packed_size = packed_size;
	file_seek(0, SEEK_SET);
	ok = replay_write_bytes_checked(&header, sizeof(header));
close:
	file_close();
	if(!ok) {
		replay_accel_file_delete(T3_ACCEL_TEMP_FN);
	}
cleanup:
	hmem_free(reinterpret_cast<void __seg *>(hash_seg));
	hmem_free(reinterpret_cast<void __seg *>(raw_seg));
	return ok;
}

static bool replay_accel_footer_valid(void)
{
	return (
		(replay_accel_footer.magic[0] == 'T') &&
		(replay_accel_footer.magic[1] == '3') &&
		(replay_accel_footer.magic[2] == 'R') &&
		(replay_accel_footer.magic[3] == 'A') &&
		(replay_accel_footer.magic[4] == 'C') &&
		(replay_accel_footer.magic[5] == 'C') &&
		(replay_accel_footer.magic[6] == '1') &&
		(replay_accel_footer.magic[7] == '\0') &&
		(replay_accel_footer.version == 1) &&
		(replay_accel_footer.footer_size == sizeof(replay_accel_footer)) &&
		(replay_accel_footer.reserved[0] == 0) &&
		(replay_accel_footer.reserved[1] == 0) &&
		(replay_accel_footer.reserved[2] == 0) &&
		(replay_accel_footer.count != 0) &&
		(replay_accel_footer.count <= T3R_ACCEL_COUNT_MAX)
	);
}

static bool replay_accel_decompress(
	const uint8_t far *packed,
	uint16_t packed_size,
	uint8_t far *raw
)
{
	uint16_t in = 0;
	uint16_t out = 0;
	uint16_t distance;
	uint16_t match_len;
	uint8_t flags;
	uint8_t token;
	uint8_t lo;
	uint8_t hi;

	while(out < T3R_ACCEL_RAW_SIZE) {
		if(in >= packed_size) {
			return false;
		}
		flags = packed[in++];
		for(
			token = 0;
			(token < 8) && (out < T3R_ACCEL_RAW_SIZE);
			token++
		) {
			if(flags & (1 << token)) {
				if(in >= packed_size) {
					return false;
				}
				raw[out++] = packed[in++];
				continue;
			}
			if((in + 1) >= packed_size) {
				return false;
			}
			lo = packed[in++];
			hi = packed[in++];
			distance = static_cast<uint16_t>(
				lo | ((static_cast<uint16_t>(hi) >> 4) << 8)
			);
			match_len = static_cast<uint16_t>(
				(hi & 0x0F) + REPLAY_ACCEL_MATCH_MIN
			);
			if(
				(distance == 0) ||
				(distance > out) ||
				((out + match_len) > T3R_ACCEL_RAW_SIZE)
			) {
				return false;
			}
			while(match_len != 0) {
				raw[out] = raw[out - distance];
				out++;
				match_len--;
			}
		}
	}
	return (in == packed_size);
}

static bool replay_accel_prepare(uint8_t checkpoint)
{
	replay_user_accel_desc_t far *desc = 0;
	uint16_t packed_seg = 0;
	uint8_t far *packed;
	uint8_t far *raw;
	uint8_t i;
	long file_bytes;
	long footer_offset;
	uint32_t logical_end;
	bool ok = false;

	if(
		(checkpoint == 0) ||
		resident->unused_3[T3R_RES_PREROLL_FORCE_INDEX]
	) {
		return false;
	}
	if(!file_ropen(replay_user_fn)) {
		return false;
	}
	file_bytes = replay_accel_file_size();
	if(file_bytes < static_cast<long>(sizeof(replay_accel_footer))) {
		goto close;
	}
	footer_offset = file_bytes - sizeof(replay_accel_footer);
	file_seek(footer_offset, SEEK_SET);
	if(
		(file_read(&replay_accel_footer, sizeof(replay_accel_footer)) !=
		 sizeof(replay_accel_footer)) ||
		!replay_accel_footer_valid()
	) {
		goto close;
	}
	for(i = 0; i < replay_accel_footer.count; i++) {
		if(replay_accel_footer.records[i].checkpoint == checkpoint) {
			desc = &replay_accel_footer.records[i];
			break;
		}
	}
	logical_end = (
		replay_user_header.input_offset + replay_user_header.input_size
	);
	if(
		(desc == 0) ||
		(desc->checkpoint >= replay_user_summary_ext.checkpoint_count) ||
		(
			(replay_user_summary_ext.checkpoint_stage_round[
				desc->checkpoint
			 ] >> 4) == 0
		) ||
		(desc->codec != T3R_ACCEL_CODEC_LZSS4K) ||
		(desc->raw_size != T3R_ACCEL_RAW_SIZE) ||
		(desc->packed_size == 0) ||
		(desc->packed_size > 0xFFF0UL) ||
		(desc->offset < logical_end) ||
		(desc->packed_size > static_cast<uint32_t>(footer_offset)) ||
		(desc->offset > (
			static_cast<uint32_t>(footer_offset) - desc->packed_size
		))
	) {
		goto close;
	}
	packed_seg = reinterpret_cast<uint16_t>(
		hmem_allocbyte(static_cast<uint16_t>(desc->packed_size))
	);
	replay_accel_raw_seg = reinterpret_cast<uint16_t>(
		hmem_allocbyte(T3R_ACCEL_RAW_SIZE)
	);
	if((packed_seg == 0) || (replay_accel_raw_seg == 0)) {
		goto close;
	}
	packed = reinterpret_cast<uint8_t far *>(MK_FP(packed_seg, 0));
	raw = reinterpret_cast<uint8_t far *>(MK_FP(replay_accel_raw_seg, 0));
	file_seek(desc->offset, SEEK_SET);
	if(
		file_read(packed, static_cast<uint16_t>(desc->packed_size)) !=
		static_cast<uint16_t>(desc->packed_size)
	) {
		goto close;
	}
	if(
		!replay_accel_decompress(
			packed, static_cast<uint16_t>(desc->packed_size), raw
		) ||
		(replay_accel_hash(raw) != desc->state_hash)
	) {
		goto close;
	}
	replay_accel_target_checkpoint = checkpoint;
	ok = true;
close:
	file_close();
	if(packed_seg != 0) {
		hmem_free(reinterpret_cast<void __seg *>(packed_seg));
	}
	if(!ok && (replay_accel_raw_seg != 0)) {
		hmem_free(reinterpret_cast<void __seg *>(replay_accel_raw_seg));
		replay_accel_raw_seg = 0;
	}
	return ok;
}

static uint8_t far *replay_main_01_entry(uint16_t offset)
{
	return reinterpret_cast<uint8_t far *>(
		MK_FP(FP_SEG(SUB_CA3C), offset)
	);
}

static bool replay_preroll_simulating(void)
{
	return (
		(replay_mode == REPLAY_USER_PLAYBACK) &&
		resident->unused_3[T3_REPLAY_RES_PREROLL_TARGET_INDEX] != 0
	);
}

static void replay_preroll_se_suppress(bool suppress)
{
	uint8_t far *se_driver_call = (
		reinterpret_cast<uint8_t far *>(snd_se_update) +
		T3_REPLAY_SND_SE_UPDATE_INT_OFFSET
	);

	se_driver_call[0] = (suppress ? 0x90 : 0xCD);
	se_driver_call[1] = (suppress ? 0x90 : PMD);
}

static void replay_preroll_audio_mask(bool mask)
{
	replay_preroll_se_suppress(mask);
	if(snd_active || snd_fm_possible) {
		asm { pushf; cli; }
		_AL = (mask ? 0xFF : 0);
		_AH = PMD_SET_VOLUME;
		geninterrupt(PMD);
		asm { popf; }
	}
}

static bool replay_preroll_startup_requested(void)
{
	return (
		(replay_resident_mode() == REPLAY_USER_PLAYBACK) &&
		(
			resident->unused_3[T3_REPLAY_RES_PREROLL_TARGET_INDEX] != 0
		)
	);
}

static void replay_preroll_hardware_hide(void)
{
	asm {
		mov	ah, 41h
		int	18h
		mov	ah, 0Dh
		int	18h
	}
	TextShown = false;
}

static void replay_preroll_hardware_show(void)
{
	asm {
		mov	ah, 40h
		int	18h
		mov	ah, 0Ch
		int	18h
	}
	TextShown = true;
}

void far replay_preroll_startup_mask(void)
{
	if(!replay_preroll_startup_requested()) {
		return;
	}
	replay_preroll_audio_mask(true);
	replay_preroll_hardware_hide();
}

static void replay_preroll_startup_unmask(void)
{
	if(!replay_preroll_startup_requested()) {
		return;
	}
	replay_preroll_hardware_show();
	replay_preroll_audio_mask(false);
}

static void replay_preroll_render_suppress(void)
{
	uint8_t far *put = reinterpret_cast<uint8_t far *>(sprite16_put);
	uint8_t far *putx = reinterpret_cast<uint8_t far *>(sprite16_putx);
	uint8_t far *noclip = reinterpret_cast<uint8_t far *>(sprite16_put_noclip);
	uint8_t far *enemy = reinterpret_cast<uint8_t far *>(enemies_render);
	uint8_t far *bullet = reinterpret_cast<uint8_t far *>(bullets_render);
	uint8_t far *points = reinterpret_cast<uint8_t far *>(
		hud_dynamic_5_digit_points_put
	);
	uint8_t far *player = replay_main_01_entry(FP_OFF(player_render));
	uint8_t far *overlay = replay_main_01_entry(FP_OFF(player_overlay_render));

	// Keep the global SPRITE16 vector valid even if MAIN crashes during preroll.
	// These process-local entry stubs are restored before the target round.
	// Parameterized Pascal callees must still remove their arguments.
	put[0] = 0xCA; put[1] = 6; put[2] = 0;
	putx[0] = 0xCA; putx[1] = 8; putx[2] = 0;
	noclip[0] = 0xCA; noclip[1] = 6; noclip[2] = 0;
	enemy[0] = 0xCB;
	bullet[0] = 0xCB;
	points[0] = 0xCA; points[1] = 8; points[2] = 0;
	replay_main_01_entry(FP_OFF(hitcircles_render))[0] = 0xC3;
	player[0] = 0xC2; player[1] = 2; player[2] = 0;
	overlay[0] = 0xC2; overlay[1] = 2; overlay[2] = 0;
	replay_main_01_entry(FP_OFF(sub_C830))[0] = 0xC3;
	replay_main_01_entry(FP_OFF(sub_C8C4))[0] = 0xC3;
	replay_main_01_entry(FP_OFF(SUB_D50E))[0] = 0xC3;
}

static void replay_preroll_render_restore(void)
{
	uint8_t far *put = reinterpret_cast<uint8_t far *>(sprite16_put);
	uint8_t far *putx = reinterpret_cast<uint8_t far *>(sprite16_putx);
	uint8_t far *noclip = reinterpret_cast<uint8_t far *>(sprite16_put_noclip);
	uint8_t far *enemy = reinterpret_cast<uint8_t far *>(enemies_render);
	uint8_t far *bullet = reinterpret_cast<uint8_t far *>(bullets_render);
	uint8_t far *points = reinterpret_cast<uint8_t far *>(
		hud_dynamic_5_digit_points_put
	);
	uint8_t far *player = replay_main_01_entry(FP_OFF(player_render));
	uint8_t far *overlay = replay_main_01_entry(FP_OFF(player_overlay_render));

	put[0] = 0x55; put[1] = 0x8B; put[2] = 0xEC;
	putx[0] = 0x55; putx[1] = 0x8B; putx[2] = 0xEC;
	noclip[0] = 0x55; noclip[1] = 0x8B; noclip[2] = 0xEC;
	enemy[0] = 0x55;
	bullet[0] = 0x55;
	points[0] = 0x55; points[1] = 0x8B; points[2] = 0xEC;
	replay_main_01_entry(FP_OFF(hitcircles_render))[0] = 0xC8;
	player[0] = 0x55; player[1] = 0x8B; player[2] = 0xEC;
	overlay[0] = 0x55; overlay[1] = 0x8B; overlay[2] = 0xEC;
	replay_main_01_entry(FP_OFF(sub_C830))[0] = 0x55;
	replay_main_01_entry(FP_OFF(sub_C8C4))[0] = 0xC8;
	replay_main_01_entry(FP_OFF(SUB_D50E))[0] = 0x56;
}

static void replay_preroll_display_hide(void)
{
	replay_preroll_render_suppress();
	replay_preroll_audio_mask(true);
	replay_preroll_hardware_hide();
}

static void replay_preroll_display_show(void)
{
	replay_preroll_render_restore();
	replay_preroll_hardware_show();
	replay_preroll_audio_mask(false);
}

static void replay_playfield_rows_fill_288(unsigned offset)
{
	_BX = offset;
	asm {
		push	di
		mov	di, bx
		mov	ax, 0A828h
		mov	es, ax
		db	66h, 31h, 0C0h	// XOR EAX, EAX
		db	66h, 0F7h, 0D0h	// NOT EAX
	replay_rows_fill_loop:
		mov	cx, 9
		db	0F3h, 66h, 0ABh	// REP STOSD
		sub	di, 74h
		jge	replay_rows_fill_loop
		pop	di
	}
}

void far replay_frame_publish(void)
{
	if(replay_preroll_simulating()) {
		graph_accesspage(page_front);
		page_front = _AL;
		page_back ^= 1;
		return;
	}
	if(palette_changed != false) {
		palette_show();
		palette_changed = false;
	}
	graph_accesspage(page_front);
	graph_showpage(page_back);
	page_front = _AL;
	page_back ^= 1;
	grcg_setcolor(GC_RMW, 0);
	replay_playfield_rows_fill_288(
		((183 * ROW_SIZE) + (16 / BYTE_DOTS))
	);
	grcg_setcolor(GC_RMW, 1);
	replay_playfield_rows_fill_288(
		((183 * ROW_SIZE) + (336 / BYTE_DOTS))
	);
	grcg_off();
}

void far replay_frame_delay(void)
{
	if(
		!replay_preroll_simulating() &&
		!(
			(replay_mode == REPLAY_USER_PLAYBACK) &&
			resident->unused_3[T3_REPLAY_RES_PAUSE_CANCEL_LATCH_INDEX]
		)
	) {
		frame_delay(1);
	}
	scorestat_process_sync();
}

void far replay_round_reset_seed_capture(void)
{
	replay_header.reserved_2 = random_seed;
}

static uint8_t replay_timing_flags(void)
{
	return static_cast<uint8_t>(
		resident->unused_3[T3_REPLAY_RES_TIMING_FLAGS_INDEX]
	);
}

static void replay_timing_flags_set(uint8_t flags)
{
	resident->unused_3[T3_REPLAY_RES_TIMING_FLAGS_INDEX] = flags;
}

static uint16_t replay_timing_baseline(void)
{
	return static_cast<uint16_t>(
		static_cast<uint8_t>(
			resident->unused_3[T3_REPLAY_RES_TIMING_BASELINE_INDEX]
		) |
		(
			static_cast<uint16_t>(static_cast<uint8_t>(
				resident->unused_3[T3_REPLAY_RES_TIMING_BASELINE_INDEX + 1]
			)) << 8
		)
	);
}

static void replay_timing_baseline_set(uint16_t baseline)
{
	resident->unused_3[T3_REPLAY_RES_TIMING_BASELINE_INDEX + 0] = (
		static_cast<uint8_t>(baseline)
	);
	resident->unused_3[T3_REPLAY_RES_TIMING_BASELINE_INDEX + 1] = (
		static_cast<uint8_t>(baseline >> 8)
	);
}

static void replay_timing_frame_begin(void)
{
	uint8_t flags = replay_timing_flags();

	flags &= static_cast<uint8_t>(~(
		T3_REPLAY_TIMING_SAMPLE_PENDING | T3_REPLAY_TIMING_PAUSE_OPENED
	));
	if(
		(replay_mode == REPLAY_USER_RECORD) &&
		(defeat_flag == DF_NONE)
	) {
		flags |= T3_REPLAY_TIMING_SAMPLE_PENDING;
	}
	replay_timing_flags_set(flags);
}

extern "C" void far replay_pause_request_poll(void)
{
	if(!(input_sp & INPUT_CANCEL)) {
		resident->unused_3[T3_REPLAY_RES_PAUSE_CANCEL_LATCH_INDEX] = false;
		asm { clc; }
		return;
	}
	if(resident->unused_3[T3_REPLAY_RES_PAUSE_CANCEL_LATCH_INDEX]) {
		asm { clc; }
		return;
	}
#if defined(TH03_REPLAY_DEVTOOLS)
	if(replay_mode == REPLAY_USER_PLAYBACK) {
		replay_debug_transition_write(0xE6, replay_rle_phase);
	}
#endif
	resident->unused_3[T3_REPLAY_RES_PAUSE_CANCEL_LATCH_INDEX] = true;
	replay_timing_flags_set(
		replay_timing_flags() | T3_REPLAY_TIMING_PAUSE_OPENED
	);
	asm { stc; }
}

static void replay_fast_forward_wait_skip(bool held)
{
	uint8_t phase;

	if(
		(replay_mode == REPLAY_USER_PLAYBACK) &&
		(resident->unused_3[T3_REPLAY_RES_PREROLL_TARGET_INDEX] != 0)
	) {
		resident->unused_3[T3_RES_FAST_FORWARD_REPLAY_PHASE_INDEX] = 0;
		vsync_Count1 = byte_23AF9;
		return;
	}
	if(
		!held ||
		(
			(replay_mode != REPLAY_PLAYBACK) &&
			(replay_mode != REPLAY_USER_PLAYBACK)
		)
	) {
		resident->unused_3[T3_RES_FAST_FORWARD_REPLAY_PHASE_INDEX] = 0;
		return;
	}

	phase = resident->unused_3[T3_RES_FAST_FORWARD_REPLAY_PHASE_INDEX];
	phase++;
	if(phase >= T3_REPLAY_FAST_FORWARD_RATE) {
		resident->unused_3[T3_RES_FAST_FORWARD_REPLAY_PHASE_INDEX] = 0;
		return;
	}
	resident->unused_3[T3_RES_FAST_FORWARD_REPLAY_PHASE_INDEX] = phase;
	vsync_Count1 = byte_23AF9;
}

static void replay_overlay_graph_fill(
	int left, int top, int right, int bottom, int color, int page
)
{
	grc_setclip(0, 0, (RES_X - 1), (SPRITE16_RES_Y - 1));
	grcg_setcolor(GC_RMW, color);
	if(page < 2) {
		graph_accesspage(page);
		grcg_boxfill(left, top, right, bottom);
	} else {
		graph_accesspage(0);
		grcg_boxfill(left, top, right, bottom);
		graph_accesspage(1);
		grcg_boxfill(left, top, right, bottom);
	}
	grcg_off();
	graph_accesspage(page_front);
}

#if defined(TH03_REPLAY_DEV_OVERLAY)
static char __ss *replay_debug_u16_put(
	char __ss *out, unsigned int value
)
{
	unsigned int digit;

#define REPLAY_DEBUG_DIGIT(place) \
	digit = 0; \
	while(value >= place) { \
		value -= place; \
		digit++; \
	} \
	*out++ = static_cast<char>('0' + digit)

	REPLAY_DEBUG_DIGIT(10000);
	REPLAY_DEBUG_DIGIT(1000);
	REPLAY_DEBUG_DIGIT(100);
	REPLAY_DEBUG_DIGIT(10);
	*out++ = static_cast<char>('0' + value);

#undef REPLAY_DEBUG_DIGIT
	return out;
}

static char __ss *replay_debug_u32_put(
	char __ss *out, unsigned long value
)
{
	unsigned int digit;

#define REPLAY_DEBUG_DIGIT(place) \
	digit = 0; \
	while(value >= place) { \
		value -= place; \
		digit++; \
	} \
	*out++ = static_cast<char>('0' + digit)

	REPLAY_DEBUG_DIGIT(1000000000UL);
	REPLAY_DEBUG_DIGIT(100000000UL);
	REPLAY_DEBUG_DIGIT(10000000UL);
	REPLAY_DEBUG_DIGIT(1000000UL);
	REPLAY_DEBUG_DIGIT(100000UL);
	REPLAY_DEBUG_DIGIT(10000UL);
	REPLAY_DEBUG_DIGIT(1000UL);
	REPLAY_DEBUG_DIGIT(100UL);
	REPLAY_DEBUG_DIGIT(10UL);
	*out++ = static_cast<char>('0' + value);

#undef REPLAY_DEBUG_DIGIT
	return out;
}

static void replay_debug_overlay_put(void)
{
	enum {
		TRAM_LEFT = 13,
		TRAM_W = 54,
		PIXEL_LEFT = (TRAM_LEFT * GLYPH_HALF_W),
		PIXEL_RIGHT = (((TRAM_LEFT + TRAM_W) * GLYPH_HALF_W) - 1),
		// MAIN uses doubled 200-line graphics under 16-pixel TRAM glyphs.
		PIXEL_BOTTOM = (GLYPH_HALF_H - 1),
	};
	char line[58];
	char __ss *out = line;

	*out++ = 'r';
	*out++ = 'o';
	*out++ = 'u';
	*out++ = 'n';
	*out++ = 'd';
	*out++ = '_';
	*out++ = 'f';
	*out++ = 'r';
	*out++ = 'a';
	*out++ = 'm';
	*out++ = 'e';
	*out++ = ' ';
	out = replay_debug_u32_put(out, round_frame);
	*out++ = ' ';
	*out++ = 'P';
	*out++ = '2';
	*out++ = ' ';
	*out++ = 'c';
	*out++ = 'p';
	*out++ = 'u';
	*out++ = '_';
	*out++ = 'f';
	*out++ = 'r';
	*out++ = 'a';
	*out++ = 'm';
	*out++ = 'e';
	*out++ = '/';
	*out++ = 's';
	*out++ = 'a';
	*out++ = 'f';
	*out++ = 'e';
	*out++ = 't';
	*out++ = 'y';
	*out++ = ' ';
	out = replay_debug_u16_put(out, players[1].cpu_frame);
	*out++ = '/';
	out = replay_debug_u16_put(out, players[1].cpu_safety_frames);
	*out = '\0';

	replay_overlay_graph_fill(
		PIXEL_LEFT, 0, PIXEL_RIGHT, PIXEL_BOTTOM, V_WHITE, 2
	);
	text_putsa(TRAM_LEFT, 0, line, (TX_BLACK | TX_REVERSE));
}
#endif

static void replay_slowdown_frame_sample(void)
{
	uint8_t flags = replay_timing_flags();
	uint16_t elapsed = vsync_Count1;
	bool eligible = true;

	if(flags & T3_REPLAY_TIMING_BASELINE_PENDING) {
		elapsed = static_cast<uint16_t>(elapsed - replay_timing_baseline());
		flags &= static_cast<uint8_t>(~T3_REPLAY_TIMING_BASELINE_PENDING);
	}
	if(replay_mode != REPLAY_USER_RECORD) {
		eligible = false;
	} else if(!(flags & T3_REPLAY_TIMING_SAMPLE_PENDING)) {
		eligible = false;
	} else if(defeat_flag != DF_NONE) {
		eligible = false;
	} else if(flags & T3_REPLAY_TIMING_PAUSE_OPENED) {
		eligible = false;
	}
	flags &= static_cast<uint8_t>(~(
		T3_REPLAY_TIMING_SAMPLE_PENDING | T3_REPLAY_TIMING_PAUSE_OPENED
	));
	replay_timing_flags_set(flags);
	if(!eligible) {
		return;
	}
	replay_user_summary_ext.timed_frames++;
	if(elapsed >= byte_23AF9) {
		replay_user_summary_ext.slow_frames++;
	}
}

void far replay_overlay_put(void)
{
	enum {
#if defined(TH03_REPLAY_DEV_OVERLAY)
		TRAM_LEFT = 75,
		TRAM_TOP = 0,
		TRAM_W = 3,
#else
		TRAM_LEFT = 37,
		TRAM_TOP = 0,
		TRAM_W = 6,
#endif
		PIXEL_LEFT = (TRAM_LEFT * GLYPH_HALF_W),
		PIXEL_TOP = (TRAM_TOP * GLYPH_HALF_H),
		PIXEL_RIGHT = (((TRAM_LEFT + TRAM_W) * GLYPH_HALF_W) - 1),
		PIXEL_BOTTOM = (PIXEL_TOP + GLYPH_HALF_H - 1),
	};
	char line[7];

	if(replay_preroll_simulating()) {
		return;
	}
	replay_slowdown_frame_sample();
#if defined(TH03_REPLAY_DEV_OVERLAY)
	replay_debug_overlay_put();
#endif
	if(
		(replay_mode != REPLAY_PLAYBACK) &&
		(replay_mode != REPLAY_USER_PLAYBACK)
	) {
		return;
	}
	line[0] = 'R';
#if defined(TH03_REPLAY_DEV_OVERLAY)
	line[1] = 'P';
	line[2] = 'Y';
	line[3] = '\0';
#else
	line[1] = 'E';
	line[2] = 'P';
	line[3] = 'L';
	line[4] = 'A';
	line[5] = 'Y';
	line[6] = '\0';
#endif

	replay_overlay_graph_fill(
		PIXEL_LEFT, PIXEL_TOP, PIXEL_RIGHT, PIXEL_BOTTOM, V_WHITE, 2
	);
	text_putsa(TRAM_LEFT, TRAM_TOP, line, (TX_BLACK | TX_REVERSE));
}

static void replay_autofire_apply_player(input_t near *input, uint8_t pid)
{
	if(resident->is_cpu[pid]) {
		return;
	}
	if(!(resident->autofire & (1 << pid))) {
		return;
	}
	if(resident->input_charge & (1 << pid)) {
		*input |= INPUT_SHOT;
		return;
	}
	if((*input & INPUT_SHOT) && (byte_220FC[pid] <= 3)) {
		*input &= ~INPUT_SHOT;
	}
}

static void replay_autofire_apply(void)
{
	replay_autofire_apply_player(&input_mp_p1, 0);
	replay_autofire_apply_player(&input_mp_p2, 1);
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

static void replay_write_bytes(const void far *buf, unsigned size)
{
	file_write(buf, size);
}

static bool replay_write_bytes_checked(const void far *buf, unsigned size)
{
	return (file_write(buf, size) != 0);
}

static void replay_write_char(char c)
{
	replay_write_bytes(&c, 1);
}

static void replay_paths_init(void)
{
	if(replay_paths_initialized) {
		return;
	}

	T3_REPLAY_CFG_FN[0] = 'T';
	T3_REPLAY_CFG_FN[1] = '3';
	T3_REPLAY_CFG_FN[2] = 'R';
	T3_REPLAY_CFG_FN[3] = 'E';
	T3_REPLAY_CFG_FN[4] = 'P';
	T3_REPLAY_CFG_FN[5] = 'L';
	T3_REPLAY_CFG_FN[6] = 'A';
	T3_REPLAY_CFG_FN[7] = 'Y';
	T3_REPLAY_CFG_FN[8] = '.';
	T3_REPLAY_CFG_FN[9] = 'C';
	T3_REPLAY_CFG_FN[10] = 'F';
	T3_REPLAY_CFG_FN[11] = 'G';
	T3_REPLAY_CFG_FN[12] = '\0';

	T3_INPUT_FN[0] = 'T';
	T3_INPUT_FN[1] = '3';
	T3_INPUT_FN[2] = 'I';
	T3_INPUT_FN[3] = 'N';
	T3_INPUT_FN[4] = 'P';
	T3_INPUT_FN[5] = 'U';
	T3_INPUT_FN[6] = 'T';
	T3_INPUT_FN[7] = '.';
	T3_INPUT_FN[8] = 'B';
	T3_INPUT_FN[9] = 'I';
	T3_INPUT_FN[10] = 'N';
	T3_INPUT_FN[11] = '\0';

	T3_SPLIT_FN[0] = 'T';
	T3_SPLIT_FN[1] = '3';
	T3_SPLIT_FN[2] = 'S';
	T3_SPLIT_FN[3] = 'P';
	T3_SPLIT_FN[4] = 'L';
	T3_SPLIT_FN[5] = 'I';
	T3_SPLIT_FN[6] = 'T';
	T3_SPLIT_FN[7] = '.';
	T3_SPLIT_FN[8] = 'B';
	T3_SPLIT_FN[9] = 'I';
	T3_SPLIT_FN[10] = 'N';
	T3_SPLIT_FN[11] = '\0';

	T3_DONE_FN[0] = 'T';
	T3_DONE_FN[1] = '3';
	T3_DONE_FN[2] = 'D';
	T3_DONE_FN[3] = 'O';
	T3_DONE_FN[4] = 'N';
	T3_DONE_FN[5] = 'E';
	T3_DONE_FN[6] = '.';
	T3_DONE_FN[7] = 'T';
	T3_DONE_FN[8] = 'X';
	T3_DONE_FN[9] = 'T';
	T3_DONE_FN[10] = '\0';

	T3_GUARD_DIAG_FN[0] = 'T';
	T3_GUARD_DIAG_FN[1] = '3';
	T3_GUARD_DIAG_FN[2] = 'G';
	T3_GUARD_DIAG_FN[3] = 'D';
	T3_GUARD_DIAG_FN[4] = 'I';
	T3_GUARD_DIAG_FN[5] = 'A';
	T3_GUARD_DIAG_FN[6] = 'G';
	T3_GUARD_DIAG_FN[7] = '.';
	T3_GUARD_DIAG_FN[8] = 'B';
	T3_GUARD_DIAG_FN[9] = 'I';
	T3_GUARD_DIAG_FN[10] = 'N';
	T3_GUARD_DIAG_FN[11] = '\0';

	T3_USER_REPLAY_DIR[0] = 'R';
	T3_USER_REPLAY_DIR[1] = 'E';
	T3_USER_REPLAY_DIR[2] = 'P';
	T3_USER_REPLAY_DIR[3] = 'L';
	T3_USER_REPLAY_DIR[4] = 'A';
	T3_USER_REPLAY_DIR[5] = 'Y';
	T3_USER_REPLAY_DIR[6] = '\0';

	T3_USER_REPLAY_INDEX_FN[0] = 'R';
	T3_USER_REPLAY_INDEX_FN[1] = 'E';
	T3_USER_REPLAY_INDEX_FN[2] = 'P';
	T3_USER_REPLAY_INDEX_FN[3] = 'L';
	T3_USER_REPLAY_INDEX_FN[4] = 'A';
	T3_USER_REPLAY_INDEX_FN[5] = 'Y';
	T3_USER_REPLAY_INDEX_FN[6] = '\\';
	T3_USER_REPLAY_INDEX_FN[7] = 'T';
	T3_USER_REPLAY_INDEX_FN[8] = 'H';
	T3_USER_REPLAY_INDEX_FN[9] = '3';
	T3_USER_REPLAY_INDEX_FN[10] = 'R';
	T3_USER_REPLAY_INDEX_FN[11] = '.';
	T3_USER_REPLAY_INDEX_FN[12] = 'I';
	T3_USER_REPLAY_INDEX_FN[13] = 'D';
	T3_USER_REPLAY_INDEX_FN[14] = 'X';
	T3_USER_REPLAY_INDEX_FN[15] = '\0';

	T3_USER_REPLAY_SLOT_FN[0] = 'R';
	T3_USER_REPLAY_SLOT_FN[1] = 'E';
	T3_USER_REPLAY_SLOT_FN[2] = 'P';
	T3_USER_REPLAY_SLOT_FN[3] = 'L';
	T3_USER_REPLAY_SLOT_FN[4] = 'A';
	T3_USER_REPLAY_SLOT_FN[5] = 'Y';
	T3_USER_REPLAY_SLOT_FN[6] = '\\';
	T3_USER_REPLAY_SLOT_FN[7] = 'T';
	T3_USER_REPLAY_SLOT_FN[8] = 'H';
	T3_USER_REPLAY_SLOT_FN[9] = '3';
	T3_USER_REPLAY_SLOT_FN[10] = 'R';
	T3_USER_REPLAY_SLOT_FN[11] = '0';
	T3_USER_REPLAY_SLOT_FN[12] = '0';
	T3_USER_REPLAY_SLOT_FN[13] = '.';
	T3_USER_REPLAY_SLOT_FN[14] = 'R';
	T3_USER_REPLAY_SLOT_FN[15] = 'P';
	T3_USER_REPLAY_SLOT_FN[16] = 'Y';
	T3_USER_REPLAY_SLOT_FN[17] = '\0';

	T3_USER_REPLAY_FALLBACK_FN[0] = 'T';
	T3_USER_REPLAY_FALLBACK_FN[1] = 'H';
	T3_USER_REPLAY_FALLBACK_FN[2] = '3';
	T3_USER_REPLAY_FALLBACK_FN[3] = 'L';
	T3_USER_REPLAY_FALLBACK_FN[4] = 'A';
	T3_USER_REPLAY_FALLBACK_FN[5] = 'S';
	T3_USER_REPLAY_FALLBACK_FN[6] = 'T';
	T3_USER_REPLAY_FALLBACK_FN[7] = '.';
	T3_USER_REPLAY_FALLBACK_FN[8] = 'R';
	T3_USER_REPLAY_FALLBACK_FN[9] = 'P';
	T3_USER_REPLAY_FALLBACK_FN[10] = 'Y';
	T3_USER_REPLAY_FALLBACK_FN[11] = '\0';

	T3_ACCEL_TEMP_FN[0] = 'T';
	T3_ACCEL_TEMP_FN[1] = 'H';
	T3_ACCEL_TEMP_FN[2] = '3';
	T3_ACCEL_TEMP_FN[3] = 'C';
	T3_ACCEL_TEMP_FN[4] = '0';
	T3_ACCEL_TEMP_FN[5] = '.';
	T3_ACCEL_TEMP_FN[6] = 'T';
	T3_ACCEL_TEMP_FN[7] = 'M';
	T3_ACCEL_TEMP_FN[8] = 'P';
	T3_ACCEL_TEMP_FN[9] = '\0';

#if defined(TH03_REPLAY_DEVTOOLS)
	T3_STATE_REFERENCE_FN[0] = 'T';
	T3_STATE_REFERENCE_FN[1] = '3';
	T3_STATE_REFERENCE_FN[2] = 'S';
	T3_STATE_REFERENCE_FN[3] = 'T';
	T3_STATE_REFERENCE_FN[4] = 'A';
	T3_STATE_REFERENCE_FN[5] = 'T';
	T3_STATE_REFERENCE_FN[6] = 'E';
	T3_STATE_REFERENCE_FN[7] = '.';
	T3_STATE_REFERENCE_FN[8] = 'B';
	T3_STATE_REFERENCE_FN[9] = 'I';
	T3_STATE_REFERENCE_FN[10] = 'N';
	T3_STATE_REFERENCE_FN[11] = '\0';

	T3_STATE_LIVE_FN[0] = 'T';
	T3_STATE_LIVE_FN[1] = '3';
	T3_STATE_LIVE_FN[2] = 'L';
	T3_STATE_LIVE_FN[3] = 'I';
	T3_STATE_LIVE_FN[4] = 'V';
	T3_STATE_LIVE_FN[5] = 'E';
	T3_STATE_LIVE_FN[6] = '.';
	T3_STATE_LIVE_FN[7] = 'B';
	T3_STATE_LIVE_FN[8] = 'I';
	T3_STATE_LIVE_FN[9] = 'N';
	T3_STATE_LIVE_FN[10] = '\0';
#endif

	replay_user_fn = T3_USER_REPLAY_FALLBACK_FN;
	replay_user_slot = T3_REPLAY_USER_SLOT_NONE;
	replay_paths_initialized = true;
}

static void replay_guard_diag_write(void)
{
	if(replay_guard_diag_written) {
		return;
	}
	replay_guard_diag_written = true;
	if(!file_create(T3_GUARD_DIAG_FN)) {
		return;
	}
	file_write(T3_GUARD_DIAG_FN, 4);
	file_write(
		&resident->unused_3[T3_REPLAY_RES_MODE_INDEX],
		(
			(T3_REPLAY_RES_MAINL_VSYNC_INDEX + 2) -
			T3_REPLAY_RES_MODE_INDEX
		)
	);
	file_close();
}

static void replay_write_text(replay_text_id_t text)
{
#define W(c) replay_write_char(c)
	switch(text) {
	case RTX_CRLF:
		W('\r');
		W('\n');
		break;
	case RTX_ERROR:
		W('e'); W('r'); W('r'); W('o'); W('r');
		break;
	case RTX_ERROR_SPLIT_OPEN:
		replay_write_text(RTX_ERROR);
		W(':'); W('s'); W('p'); W('l'); W('i'); W('t'); W('-'); W('o');
		W('p'); W('e'); W('n');
		break;
	case RTX_ERROR_INPUT_CREATE:
		replay_write_text(RTX_ERROR);
		W(':'); W('i'); W('n'); W('p'); W('u'); W('t'); W('-'); W('c');
		W('r'); W('e'); W('a'); W('t'); W('e');
		break;
	case RTX_ERROR_USER_CREATE:
		replay_write_text(RTX_ERROR);
		W(':'); W('u'); W('s'); W('e'); W('r'); W('-'); W('c'); W('r');
		W('e'); W('a'); W('t'); W('e');
		break;
	case RTX_ERROR_USER_HEADER:
		replay_write_text(RTX_ERROR);
		W(':'); W('u'); W('s'); W('e'); W('r'); W('-'); W('h'); W('e');
		W('a'); W('d'); W('e'); W('r');
		break;
	case RTX_ERROR_INPUT_HEADER:
		replay_write_text(RTX_ERROR);
		W(':'); W('i'); W('n'); W('p'); W('u'); W('t'); W('-'); W('h');
		W('e'); W('a'); W('d'); W('e'); W('r');
		break;
	case RTX_OK_USER_INPUT_END:
		replay_write_text(RTX_OK);
		W(':'); W('u'); W('s'); W('e'); W('r'); W('-'); W('i'); W('n');
		W('p'); W('u'); W('t'); W('-'); W('e'); W('n'); W('d');
		break;
	case RTX_OK_INPUT_END:
		replay_write_text(RTX_OK);
		W(':'); W('i'); W('n'); W('p'); W('u'); W('t'); W('-'); W('e');
		W('n'); W('d');
		break;
	case RTX_ERROR_FRAME_IO:
		replay_write_text(RTX_ERROR);
		W(':'); W('f'); W('r'); W('a'); W('m'); W('e'); W('-'); W('i');
		W('o');
		break;
	case RTX_OK_MENU_RETURN:
		replay_write_text(RTX_OK);
		W(':'); W('m'); W('e'); W('n'); W('u'); W('-'); W('r'); W('e');
		W('t'); W('u'); W('r'); W('n');
		break;
	case RTX_OK_MENU_RETURN_NOSAVE:
		replay_write_text(RTX_OK_MENU_RETURN);
		W('-'); W('n'); W('o'); W('s'); W('a'); W('v'); W('e');
		break;
	case RTX_OK_PARTIAL:
		replay_write_text(RTX_OK);
		W(':'); W('p'); W('a'); W('r'); W('t'); W('i'); W('a'); W('l');
		break;
	case RTX_OK_USER_PLAYBACK:
		replay_write_text(RTX_OK);
		W(':'); W('u'); W('s'); W('e'); W('r'); W('-'); W('p'); W('l');
		W('a'); W('y'); W('b'); W('a'); W('c'); W('k');
		break;
	case RTX_OK:
		W('o'); W('k');
		break;
	}
#undef W
}

static void replay_score_pack(
	uint8_t near *packed, const unsigned char near *digits
)
{
	int i;

	for(i = 0; i < T3_REPLAY_USER_PACKED_SCORE_SIZE; i++) {
		packed[i] = static_cast<uint8_t>(
			(digits[(i * 2) + 0] % 10) |
			((digits[(i * 2) + 1] % 10) << 4)
		);
	}
}

static void replay_u32_pack_score(uint8_t near *packed, uint32_t value)
{
	int i;

	for(i = 0; i < T3_REPLAY_USER_PACKED_SCORE_SIZE; i++) {
		packed[i] = static_cast<uint8_t>(
			(value % 10) | (((value / 10) % 10) << 4)
		);
		value /= 100;
	}
}

static void replay_user_summary_ext_init(void)
{
	replay_memclear(&replay_user_summary_ext, sizeof(replay_user_summary_ext));
	replay_user_summary_ext.flags = T3_REPLAY_USER_SUMMARY_CURRENT;
}

static uint8_t replay_user_summary_stage_round_pack(void)
{
	uint8_t stage;

	if((resident->game_mode == GM_STORY) || practice_game_active()) {
		stage = resident->story_stage;
		if(stage < T3_REPLAY_USER_STAGE_COUNT) {
			return static_cast<uint8_t>(
				((round_id & 0x0F) << 4) | stage
			);
		}
	}
	return static_cast<uint8_t>(
		((round_id & 0x0F) << 4) | T3_REPLAY_USER_ROUND_STAGE_VS
	);
}

static uint8_t replay_user_summary_route_winner_pack(uint8_t route)
{
	uint8_t winner = T3_REPLAY_USER_ROUND_VALUE_UNKNOWN;

	if(resident->pid_winner == 0) {
		winner = 0;
	} else if(resident->pid_winner == 1) {
		winner = 1;
	}
	return static_cast<uint8_t>(((route & 0x0F) << 4) | winner);
}

static void replay_round_real_frame_tick(void)
{
	uint16_t now = vsync_Count2;

	replay_round_real_frames += static_cast<uint16_t>(
		now - replay_round_vsync_last
	);
	replay_round_vsync_last = now;
}

static void replay_user_round_split_capture(uint8_t route)
{
	replay_user_round_split_t near *split;

	if(
		(replay_mode != REPLAY_USER_RECORD) ||
		(
			replay_user_summary_ext.round_reached_count >=
			T3_REPLAY_USER_ROUND_SPLIT_COUNT
		)
	) {
		return;
	}

	split = &replay_user_summary_ext.round_splits[
		replay_user_summary_ext.round_reached_count
	];
	split->stage_round = replay_user_summary_stage_round_pack();
	split->route_winner = replay_user_summary_route_winner_pack(route);
	replay_score_pack(split->score_p1, score);
	replay_score_pack(split->score_p2, (score + SCORE_DIGITS));
	split->real_frames = replay_round_real_frames;
	replay_user_summary_ext.round_reached_count++;
}

void far replay_clear_bonus_capture(void)
{
	replay_user_stage_clear_bonus_t near *bonus;
	uint32_t total;
	uint8_t stage;

	if(
		(replay_mode != REPLAY_USER_RECORD) ||
		(resident->pid_winner != 0) ||
		((resident->game_mode != GM_STORY) && !practice_game_active())
	) {
		return;
	}
	stage = resident->story_stage;
	if(stage >= T3_REPLAY_USER_STAGE_COUNT) {
		return;
	}
	bonus = &replay_user_summary_ext.stage_clear_bonuses[stage];
	bonus->max_combo = defeat_combo_hits_max;
	bonus->gauge_attacks = defeat_gauge_attacks_fired;
	bonus->boss_attacks = defeat_boss_attacks_fired;
	bonus->boss_reversals = defeat_boss_attacks_reversed;
	bonus->boss_panics = defeat_boss_panics_fired;
	bonus->remaining_lives = (
		(stage == STAGE_YUMEMI) ? byte_23DF9 : 0
	);
	total = (static_cast<uint32_t>(bonus->max_combo) * 1000UL);
	total += (static_cast<uint32_t>(bonus->gauge_attacks) * 10000UL);
	total += (static_cast<uint32_t>(bonus->boss_attacks) * 15000UL);
	total += (static_cast<uint32_t>(bonus->boss_reversals) * 20000UL);
	total += (static_cast<uint32_t>(bonus->boss_panics) * 30000UL);
	total += (static_cast<uint32_t>(bonus->remaining_lives) * 100000UL);
	replay_u32_pack_score(bonus->total, total);
}

static void replay_user_summary_init_from_snapshot(void)
{
	int i;
	int j;

	replay_sum_flags = T3_REPLAY_USER_SUMMARY_CURRENT;
	replay_sum_route = T3_REPLAY_USER_SUMMARY_UNKNOWN;
	replay_sum_mode = replay_user_snapshot.game_mode;
	replay_sum_stage = replay_user_snapshot.story_stage;
	replay_sum_round = T3_REPLAY_USER_SUMMARY_UNKNOWN;
	replay_sum_winner = T3_REPLAY_USER_SUMMARY_UNKNOWN;
	replay_sum_lives = replay_user_snapshot.story_lives;
	replay_sum_misses = T3_REPLAY_USER_SUMMARY_UNKNOWN;
	replay_sum_stage_count = 0;
	for(i = 0; i < T3_REPLAY_USER_STAGE_COUNT; i++) {
		replay_sum_stage_opps[i] = (
			replay_user_snapshot.story_opponents[i]
		);
		for(j = 0; j < T3_REPLAY_USER_PACKED_SCORE_SIZE; j++) {
			replay_sum_stage_scores[i][j] = 0;
		}
	}
}

static void replay_user_summary_load_from_header(void)
{
	int i;
	int j;

	replay_user_summary_init_from_snapshot();
	if(
		(replay_user_header.summary_flags & T3_REPLAY_USER_SUMMARY_VALID) == 0
	) {
		return;
	}
	replay_sum_flags = replay_user_header.summary_flags;
	replay_sum_route = replay_user_header.final_route;
	replay_sum_mode = replay_user_header.final_game_mode;
	replay_sum_stage = replay_user_header.final_story_stage;
	replay_sum_round = replay_user_header.final_round_id;
	replay_sum_winner = replay_user_header.final_winner;
	replay_sum_lives = replay_user_header.final_story_lives;
	replay_sum_misses = replay_user_header.final_misses;
	replay_sum_stage_count = (
		replay_user_header.stage_reached_count
	);
	if((replay_user_header.flags & T3_REPLAY_USER_FLAG_PRACTICE) == 0) {
		for(i = 0; i < T3_REPLAY_USER_STAGE_COUNT; i++) {
			replay_sum_stage_opps[i] = (
				replay_user_header.scenario.story.stage_opponents[i]
			);
			for(j = 0; j < T3_REPLAY_USER_PACKED_SCORE_SIZE; j++) {
				replay_sum_stage_scores[i][j] = (
					replay_user_header.scenario.story.stage_scores[i][j]
				);
			}
		}
	}
}

static void replay_user_summary_capture(uint8_t route)
{
	uint8_t stage = resident->story_stage;

	if(replay_mode != REPLAY_USER_RECORD) {
		return;
	}

	replay_sum_flags = T3_REPLAY_USER_SUMMARY_CURRENT;
	replay_sum_route = route;
	replay_sum_mode = resident->game_mode;
	replay_sum_stage = resident->story_stage;
	replay_sum_round = round_id;
	replay_sum_winner = static_cast<uint8_t>(resident->pid_winner);
	replay_sum_lives = resident->story_lives;
	replay_sum_misses = T3_REPLAY_USER_SUMMARY_UNKNOWN;

	if((resident->game_mode == GM_STORY) && (stage < T3_REPLAY_USER_STAGE_COUNT)) {
		replay_sum_stage_opps[stage] = (
			resident->story_opponents[stage].v
		);
		replay_score_pack(replay_sum_stage_scores[stage], score);
		if(replay_sum_stage_count <= stage) {
			replay_sum_stage_count = (stage + 1);
		}
	}
}

static void replay_user_summary_copy_to_header(void)
{
	int i;
	int j;

	replay_user_header.summary_flags = replay_sum_flags;
	replay_user_header.final_route = replay_sum_route;
	replay_user_header.final_game_mode = replay_sum_mode;
	replay_user_header.final_story_stage = replay_sum_stage;
	replay_user_header.final_round_id = replay_sum_round;
	replay_user_header.final_winner = replay_sum_winner;
	replay_user_header.final_story_lives = replay_sum_lives;
	replay_user_header.final_misses = replay_sum_misses;
	replay_user_header.stage_reached_count = replay_sum_stage_count;
	if((replay_user_header.flags & T3_REPLAY_USER_FLAG_PRACTICE) == 0) {
		for(i = 0; i < T3_REPLAY_USER_STAGE_COUNT; i++) {
			replay_user_header.scenario.story.stage_opponents[i] = (
				replay_sum_stage_opps[i]
			);
			for(j = 0; j < T3_REPLAY_USER_PACKED_SCORE_SIZE; j++) {
				replay_user_header.scenario.story.stage_scores[i][j] = (
					replay_sum_stage_scores[i][j]
				);
			}
		}
	}
	replay_score_pack(replay_user_header.final_score, score);
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

static void replay_user_guard_fn_set(char far *fn)
{
	fn[0] = '\\';
	if(replay_user_slot < T3_REPLAY_USER_SLOT_COUNT) {
		fn[1] = 'T';
		fn[2] = 'H';
		fn[3] = '3';
		fn[4] = 'G';
		fn[5] = static_cast<char>('0' + (replay_user_slot / 10));
		fn[6] = static_cast<char>('0' + (replay_user_slot % 10));
		fn[7] = '.';
		fn[8] = 'T';
		fn[9] = 'M';
		fn[10] = 'P';
		fn[11] = '\0';
	} else {
		fn[1] = 'T';
		fn[2] = 'H';
		fn[3] = '3';
		fn[4] = 'L';
		fn[5] = 'A';
		fn[6] = 'S';
		fn[7] = 'T';
		fn[8] = '.';
		fn[9] = 'G';
		fn[10] = 'R';
		fn[11] = 'D';
		fn[12] = '\0';
	}
}

static bool replay_user_guard_create(void)
{
	char guard_fn[13];

	replay_user_guard_fn_set(guard_fn);
	replay_protect_file_delete_commit(guard_fn);
	return replay_protect_guard_create(guard_fn);
}

static bool replay_user_guard_checkpoint(void)
{
	char guard_fn[13];

	replay_user_guard_fn_set(guard_fn);
	if(!replay_protect_checkpoint(guard_fn)) {
		if(replay_protect_invalid()) {
			replay_protect_guard_marker_set(guard_fn);
		}
		return false;
	}
	return true;
}

static void replay_user_guard_delete(void)
{
	char guard_fn[13];

	replay_user_guard_fn_set(guard_fn);
	replay_protect_file_delete_commit(guard_fn);
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
	replay_user_index_header.magic[6] = '9';
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
	int i;

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
	for(i = 0; i < T3_REPLAY_USER_NAME_LEN; i++) {
		replay_user_index_entry.name[i] = replay_user_header.name[i];
	}
	replay_user_index_entry.dos_date = replay_user_header.dos_date;
	replay_user_index_entry.autofire = replay_user_header.autofire;
	replay_user_index_entry.replay_flags = static_cast<uint8_t>(
		replay_user_header.flags & T3_REPLAY_USER_FLAG_PRACTICE
	);
	replay_user_index_entry.summary_flags = replay_user_header.summary_flags;
	replay_user_index_entry.final_route = replay_user_header.final_route;
	replay_user_index_entry.final_story_stage = (
		replay_user_header.final_story_stage
	);
	replay_user_index_entry.final_story_lives = (
		replay_user_header.final_story_lives
	);
	replay_user_index_entry.final_misses = replay_user_header.final_misses;
	replay_user_index_entry.stage_reached_count = (
		replay_user_header.stage_reached_count
	);
	for(i = 0; i < T3_REPLAY_USER_PACKED_SCORE_SIZE; i++) {
		replay_user_index_entry.final_score[i] = replay_user_header.final_score[i];
	}
	if((replay_user_header.flags & T3_REPLAY_USER_FLAG_PRACTICE) == 0) {
		for(i = 0; i < T3_REPLAY_USER_STAGE_COUNT; i++) {
			replay_user_index_entry.stage_opponents[i] = (
				replay_user_header.scenario.story.stage_opponents[i]
			);
		}
	}
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

static bool replay_user_index_slot_clear(void)
{
	uint32_t offset;
	bool ret;

	if(replay_user_slot >= T3_REPLAY_USER_SLOT_COUNT) {
		return false;
	}

	replay_dir_create();
	if(!file_append(T3_USER_REPLAY_INDEX_FN)) {
		return false;
	}

	replay_memclear(&replay_user_index_entry, sizeof(replay_user_index_entry));
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
	hash = replay_hash_u8(hash, player->cpu_dodge_strategy);
	hash = replay_hash_u16(hash, player->human_movement_last);
	hash = replay_hash_u8(hash, player->spell_ready_frames);
	hash = replay_hash_u16(hash, player->combo_bonus_max);
	hash = replay_hash_u8(hash, player->combo_hits_max);
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
	hash = replay_hash_u8(hash, randring_p);
	hash = replay_hash_u8(hash, byte_220FC[0]);
	hash = replay_hash_u8(hash, byte_220FC[1]);
	hash = replay_hash_u8(hash, byte_20E48);
	hash = replay_hash_u8(hash, boss_panic_fired_in_current_combo[0]);
	hash = replay_hash_u8(hash, boss_panic_fired_in_current_combo[1]);
	hash = replay_hash_u8(hash, replay_user_background_phase());
	hash = replay_hash_u8(hash, replay_user_result_phase());
	hash = replay_hash_bytes(hash, combos, sizeof(combos));
	hash = replay_hash_bytes(hash, chain_ring_p, sizeof(chain_ring_p));
	hash = replay_hash_bytes(hash, &chains, sizeof(chains));
	hash = replay_hash_u32(hash, random_seed);
	return hash;
}

#if defined(TH03_REPLAY_DEVTOOLS)
enum replay_state_probe_constants_t {
	REPLAY_STATE_PROBE_VERSION = 1,
	REPLAY_STATE_PROBE_DGROUP_DATA_OFFSET = 0x0090,
	REPLAY_STATE_PROBE_DGROUP_DATA_SIZE = (0x0BEE - 0x0090),
	REPLAY_STATE_PROBE_DGROUP_BSS_OFFSET = 0x1182,
	REPLAY_STATE_PROBE_DGROUP_BSS_SIZE = (0x8DFA - 0x1182),
	REPLAY_STATE_PROBE_FORMATION_SIZE = T3_REPLAY_USER_FORMATION_RING_SIZE,
	REPLAY_STATE_PROBE_BLOCK_COUNT = 5,
	REPLAY_STATE_PROBE_KIND_REFERENCE = 1,
	REPLAY_STATE_PROBE_KIND_LIVE = 2,
};

struct replay_state_probe_file_header_t {
	char magic[8];
	uint16_t version;
	uint16_t header_size;
	uint16_t record_header_size;
	uint16_t record_size;
	uint16_t dgroup_data_offset;
	uint16_t dgroup_data_size;
	uint16_t dgroup_bss_offset;
	uint16_t dgroup_bss_size;
	uint16_t resident_size;
	uint16_t formation_size;
	uint16_t block_count;
	uint16_t reserved;
};

struct replay_state_probe_record_header_t {
	uint16_t header_size;
	uint16_t checkpoint;
	uint16_t stage_round;
	uint16_t game_mode;
	uint32_t sample_count;
	uint32_t global_frame;
	uint32_t input_byte_count;
	uint32_t curated_hash;
	uint16_t dgroup_seg;
	uint16_t resident_seg;
	uint16_t formation_type_seg;
	uint16_t formation_pos_seg;
	uint16_t kind;
	uint16_t reserved;
};

typedef char rsp_file_header_size_check[
	(sizeof(replay_state_probe_file_header_t) == 32) ? 1 : -1
];
typedef char rsp_record_header_size_check[
	(sizeof(replay_state_probe_record_header_t) == 36) ? 1 : -1
];

static bool replay_state_probe_file_header_write(void)
{
	replay_state_probe_file_header_t header;

	replay_memclear(&header, sizeof(header));
	header.magic[0] = 'T';
	header.magic[1] = '3';
	header.magic[2] = 'S';
	header.magic[3] = 'T';
	header.magic[4] = 'A';
	header.magic[5] = 'T';
	header.magic[6] = 'E';
	header.magic[7] = '1';
	header.version = REPLAY_STATE_PROBE_VERSION;
	header.header_size = sizeof(header);
	header.record_header_size = sizeof(replay_state_probe_record_header_t);
	header.record_size = static_cast<uint16_t>(
		sizeof(replay_state_probe_record_header_t) +
		REPLAY_STATE_PROBE_DGROUP_DATA_SIZE +
		REPLAY_STATE_PROBE_DGROUP_BSS_SIZE +
		sizeof(resident_t) +
		(REPLAY_STATE_PROBE_FORMATION_SIZE * 2)
	);
	header.dgroup_data_offset = REPLAY_STATE_PROBE_DGROUP_DATA_OFFSET;
	header.dgroup_data_size = REPLAY_STATE_PROBE_DGROUP_DATA_SIZE;
	header.dgroup_bss_offset = REPLAY_STATE_PROBE_DGROUP_BSS_OFFSET;
	header.dgroup_bss_size = REPLAY_STATE_PROBE_DGROUP_BSS_SIZE;
	header.resident_size = sizeof(resident_t);
	header.formation_size = REPLAY_STATE_PROBE_FORMATION_SIZE;
	header.block_count = REPLAY_STATE_PROBE_BLOCK_COUNT;
	return replay_write_bytes_checked(&header, sizeof(header));
}

static bool replay_state_probe_record_write(
	const char near *fn, uint8_t checkpoint, uint8_t kind
)
{
	replay_state_probe_record_header_t record;
	uint16_t dgroup_seg = FP_SEG(&replay_mode);
	bool ok;

	replay_memclear(&record, sizeof(record));
	record.header_size = sizeof(record);
	record.checkpoint = checkpoint;
	record.stage_round = (
		replay_user_summary_ext.checkpoint_stage_round[checkpoint]
	);
	record.game_mode = resident->game_mode;
	record.sample_count = replay_sample_count;
	record.global_frame = replay_global_frame;
	record.input_byte_count = replay_input_byte_count;
	record.curated_hash = replay_state_hash();
	record.dgroup_seg = dgroup_seg;
	record.resident_seg = FP_SEG(resident);
	record.formation_type_seg = reinterpret_cast<uint16_t>(
		formation_type_ring
	);
	record.formation_pos_seg = reinterpret_cast<uint16_t>(
		formation_pos_type_ring
	);
	record.kind = kind;

	if(kind == REPLAY_STATE_PROBE_KIND_LIVE) {
		ok = file_create(fn);
	} else if(checkpoint == 0) {
		ok = file_create(fn);
	} else {
		ok = file_append(fn);
	}
	if(!ok) {
		return false;
	}
	if((kind == REPLAY_STATE_PROBE_KIND_LIVE) || (checkpoint == 0)) {
		ok = replay_state_probe_file_header_write();
	} else {
		ok = true;
	}
	ok = (
		ok &&
		replay_write_bytes_checked(&record, sizeof(record)) &&
		replay_write_bytes_checked(
			MK_FP(dgroup_seg, REPLAY_STATE_PROBE_DGROUP_DATA_OFFSET),
			REPLAY_STATE_PROBE_DGROUP_DATA_SIZE
		) &&
		replay_write_bytes_checked(
			MK_FP(dgroup_seg, REPLAY_STATE_PROBE_DGROUP_BSS_OFFSET),
			REPLAY_STATE_PROBE_DGROUP_BSS_SIZE
		) &&
		replay_write_bytes_checked(resident, sizeof(resident_t)) &&
		replay_write_bytes_checked(
			formation_type_ring, REPLAY_STATE_PROBE_FORMATION_SIZE
		) &&
		replay_write_bytes_checked(
			formation_pos_type_ring, REPLAY_STATE_PROBE_FORMATION_SIZE
		)
	);
	file_close();
	return ok;
}
#endif

static void replay_split_write_header(void)
{
	replay_split_header_t header;

	replay_memclear(&header, sizeof(header));
	header.magic[0] = 'T';
	header.magic[1] = '3';
	header.magic[2] = 'S';
	header.magic[3] = 'P';
	header.magic[4] = 'L';
	header.magic[5] = 'T';
	header.magic[6] = '1';
	header.version = T3_REPLAY_SPLIT_VERSION;
	header.header_size = sizeof(header);
	header.row_size = sizeof(replay_split_row_t);
	if(!file_create(T3_SPLIT_FN)) {
		return;
	}
	file_write(&header, sizeof(header));
	file_close();
}

static void replay_done_write(replay_text_id_t status)
{
	if(replay_done_written) {
		return;
	}
	if(file_create(T3_DONE_FN)) {
		replay_write_text(status);
		replay_write_text(RTX_CRLF);
		file_close();
	}
	replay_done_written = true;
}

static void replay_split_row(replay_split_event_t event, uint8_t route)
{
	replay_split_row_t row;
	int i;

	if(
		(replay_mode == REPLAY_DISABLED) ||
		(replay_mode == REPLAY_ERROR) ||
		(replay_mode == REPLAY_USER_RECORD)
	) {
		return;
	}
	if(!file_append(T3_SPLIT_FN)) {
		replay_mode = REPLAY_ERROR;
		replay_done_write(RTX_ERROR_SPLIT_OPEN);
		return;
	}
	replay_memclear(&row, sizeof(row));
	row.event = event;
	row.route = route;
	row.game_mode = resident->game_mode;
	row.story_stage = resident->story_stage;
	row.round_id = round_id;
	row.winner = resident->pid_winner;
	row.round_speed = round_speed;
	row.global_frame = replay_global_frame;
	row.round_frame = round_frame;
	row.round_or_result_frame = round_or_result_frame;
	for(i = 0; i < T3_REPLAY_USER_PACKED_SCORE_SIZE; i++) {
		row.score_p1[i] = static_cast<uint8_t>(
			(score[(i * 2) + 0] % 10) |
			((score[(i * 2) + 1] % 10) << 4)
		);
		row.score_p2[i] = static_cast<uint8_t>(
			(score[SCORE_DIGITS + (i * 2) + 0] % 10) |
			((score[SCORE_DIGITS + (i * 2) + 1] % 10) << 4)
		);
	}
	row.resident_rand = resident->rand;
	row.state_hash = replay_state_hash();
	if(file_write(&row, sizeof(row)) == 0) {
		replay_mode = REPLAY_ERROR;
	}
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
	replay_user_snapshot.autofire = resident->autofire;

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
	for(i = 0; i < PLAYER_COUNT; i++) {
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

static void replay_user_compact_pack(void)
{
	int i;
	replay_user_snapshot_compact_t near *compact = (
		reinterpret_cast<replay_user_snapshot_compact_t near *>(
			&replay_user_snapshot
		)
	);
	uint8_t near *dst = reinterpret_cast<uint8_t near *>(
		&compact->player_center_x[0]
	);
	uint8_t near *src = reinterpret_cast<uint8_t near *>(
		&replay_user_snapshot.player_center_x[0]
	);

	compact->autofire = replay_user_snapshot.autofire;
	for(i = 0; i < T3R_SNAPSHOT_PLAYER_RUNTIME_SIZE; i++) {
		dst[i] = src[i];
	}
	compact->round_reset_seed = replay_header.reserved_2;
	compact->formation_first = formation_type_ring[0];
}

static void replay_user_compact_unpack(void)
{
	int i;
	replay_user_snapshot_compact_t near *compact = (
		reinterpret_cast<replay_user_snapshot_compact_t near *>(
			&replay_user_snapshot
		)
	);
	uint8_t near *dst = reinterpret_cast<uint8_t near *>(
		&replay_user_snapshot.player_center_x[0]
	);
	uint8_t near *src = reinterpret_cast<uint8_t near *>(
		&compact->player_center_x[0]
	);

	for(i = 0; i < T3R_SNAPSHOT_PLAYER_RUNTIME_SIZE; i++) {
		dst[i] = src[i];
	}
	replay_header.reserved_2 = compact->round_reset_seed;
	replay_header.reserved_3 = compact->formation_first;
	replay_user_snapshot.autofire = compact->autofire;
}

static void replay_user_round_state_fill(void)
{
	int i;
	int digit;

	replay_memclear(&replay_user_round_state, sizeof(replay_user_round_state));
	replay_user_round_state.round_id = round_id;
	for(i = 0; i < PLAYER_COUNT; i++) {
		replay_user_round_state.rounds_won[i] = players[i].rounds_won;
		for(digit = 0; digit < SCORE_DIGITS; digit++) {
			replay_user_round_state.score[i][digit] = (
				score[(i * SCORE_DIGITS) + digit]
			);
		}
		replay_user_round_state.spell_rank[i] = gba_gauge_level[i];
		replay_user_round_state.cpu_safety_frames[i] = (
			players[i].cpu_safety_frames
		);
	}
	replay_user_round_state.round_speed = round_speed;
	replay_user_round_state.bullet_speed = bullet_base_speed.v;
	replay_user_round_state.boss_rank = gba_boss_level;
	replay_user_round_state.cpu_damage = cpu_hit_damage_additional;
	replay_user_round_state.extends_gained = extends_gained;
}

static void replay_user_round_carry_fill(void)
{
	int i;

	replay_memclear(&replay_user_round_carry, sizeof(replay_user_round_carry));
	for(i = 0; i < PLAYER_COUNT; i++) {
		replay_user_round_carry.cpu_dodge_strategy[i] = (
			players[i].cpu_dodge_strategy
		);
		replay_user_round_carry.human_movement_last[i] = (
			players[i].human_movement_last
		);
		replay_user_round_carry.spell_ready_frames[i] = (
			players[i].spell_ready_frames
		);
		replay_user_round_carry.combo_bonus_max[i] = players[i].combo_bonus_max;
		replay_user_round_carry.combo_hits_max[i] = players[i].combo_hits_max;
		replay_user_round_carry.shot_cycle[i] = byte_220FC[i];
		replay_user_round_carry.boss_panic_fired[i] = (
			boss_panic_fired_in_current_combo[i]
		);
		replay_user_round_carry.gba_flag_next[i] = gba_flag_next[i];
	}
	replay_user_round_carry.cpu_shot_decision = byte_20E48;
	replay_user_round_carry.background_phase = (
		replay_user_background_phase()
	);
	replay_user_round_carry.randring_p = randring_p;
	replay_user_round_carry.result_phase = replay_user_result_phase();
	replay_user_carry_chains_fill();
}

static void replay_user_header_fill(
	replay_user_status_t status, replay_user_end_reason_t end_reason
)
{
	if(replay_user_header.version != T3_REPLAY_USER_VERSION) {
		replay_memclear(&replay_user_header, sizeof(replay_user_header));
		replay_memclear(
			&replay_user_identity_ext, sizeof(replay_user_identity_ext)
		);
		replay_user_header.magic[0] = 'T';
		replay_user_header.magic[1] = '3';
		replay_user_header.magic[2] = 'R';
		replay_user_header.magic[3] = 'P';
		replay_user_header.magic[4] = 'L';
		replay_user_header.magic[5] = 'Y';
		replay_user_header.magic[6] = '1';
		replay_user_header.magic[7] = '4';
		replay_user_header.version = T3_REPLAY_USER_VERSION;
		replay_user_header.header_size = replay_user_header_size(
			T3_REPLAY_USER_VERSION
		);
		replay_user_header.sample_size = T3_REPLAY_USER_SAMPLE_SIZE_RLE;
		replay_user_header.flags = (
			T3_REPLAY_USER_FLAG_RLE_INPUT |
			T3_REPLAY_USER_FLAG_CHARGE_INPUT
		);
		if(practice_game_active()) {
			replay_user_header.flags |= T3_REPLAY_USER_FLAG_PRACTICE;
			practice_replay_config_capture(
				replay_user_header.scenario.practice.config,
				players[0].cpu_safety_frames
			);
		}
		replay_user_header.game_mode = replay_user_snapshot.game_mode;
		replay_user_header.rank = replay_user_snapshot.rank;
		replay_user_header.key_mode = replay_user_snapshot.key_mode;
		replay_user_header.playchar_p1 = (
			replay_user_snapshot.playchar_paletted[0]
		);
		replay_user_header.playchar_p2 = (
			replay_user_snapshot.playchar_paletted[1]
		);
		replay_user_header.story_stage = replay_user_snapshot.story_stage;
		replay_user_header.is_cpu_p1 = replay_user_snapshot.is_cpu[0];
		replay_user_header.is_cpu_p2 = replay_user_snapshot.is_cpu[1];
		replay_user_header.resident_rand = replay_user_snapshot.resident_rand;
		replay_user_header.random_seed_snapshot = (
			replay_user_snapshot.random_seed_snapshot
		);
		replay_user_header.snapshot_offset = replay_user_header.header_size;
		replay_user_header.snapshot_size = T3R_STAGE_CKPT_SIZE;
		replay_user_header.input_offset = replay_user_input_offset(
			T3_REPLAY_USER_VERSION,
			replay_user_snapshot.game_mode,
			replay_user_header.flags
		);
		replay_user_header.autofire = replay_user_snapshot.autofire;
		replay_user_identity_ext.ruleset = T3R_RULESET_STOCK;
		replay_user_identity_ext.recorder_source = T3R_RECORDER_SOURCE_PC98;
	}
	replay_user_header.status = status;
	replay_user_header.end_reason = end_reason;
	replay_user_header.sample_count = replay_sample_count;
	replay_user_header.final_frame_count = replay_global_frame;
	replay_user_header.input_size = replay_input_byte_count;
	replay_user_summary_copy_to_header();
}

static uint32_t far *replay_user_checkpoint_cursor(void)
{
	return reinterpret_cast<uint32_t far *>(MK_FP(
		replay_write_buffer_seg, T3_REPLAY_WRITE_BUFFER_SIZE
	));
}

static void replay_user_checkpoint_cursor_capture(void)
{
	if(replay_write_buffer_seg == 0) {
		return;
	}
	uint32_t far *cursor = replay_user_checkpoint_cursor();

	// The checkpoint byte cursor must begin a new, self-contained RLE packet.
	// No disk write is needed; the next sample emits all four input fields.
	replay_rle_packet_state &= ~REPLAY_RLE_STATE_OPEN;
	cursor[0] = replay_sample_count;
	cursor[1] = replay_global_frame;
	cursor[2] = replay_input_byte_count;
}

static bool replay_user_checkpoint_write(void)
{
	uint32_t far *cursor = replay_user_checkpoint_cursor();
	uint8_t checkpoint = static_cast<uint8_t>(
		replay_user_summary_ext.checkpoint_count - 1
	);
	uint32_t offset = (
		replay_user_header.snapshot_offset +
		(
			static_cast<uint32_t>(checkpoint) *
			static_cast<uint32_t>(T3R_STAGE_CKPT_SIZE)
		)
	);

	file_seek(offset, SEEK_SET);
	return (
		replay_write_bytes_checked(&cursor[0], sizeof(cursor[0])) &&
		replay_write_bytes_checked(&cursor[1], sizeof(cursor[1])) &&
		replay_write_bytes_checked(&cursor[2], sizeof(cursor[2])) &&
		replay_write_bytes_checked(
			&replay_user_snapshot,
			sizeof(replay_user_snapshot_compact_t)
		) &&
		replay_write_bytes_checked(
			&replay_user_round_state, sizeof(replay_user_round_state)
		) &&
		replay_write_bytes_checked(
			&replay_user_round_carry, sizeof(replay_user_round_carry)
		)
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
	uint32_t input_offset;
	bool checkpoint_pending = (
		(replay_rle_phase & REPLAY_RLE_STAGE_CHECKPOINT_PENDING_MASK) != 0
	);
	uint8_t far *write_buffer = reinterpret_cast<uint8_t far *>(
		MK_FP(replay_write_buffer_seg, 0)
	);

	if(replay_protect_blocked()) {
		return false;
	}
	if(!replay_user_guard_checkpoint()) {
		return false;
	}
	replay_user_summary_capture(replay_last_route);
	replay_user_header_fill(status, end_reason);
	if(!file_append(replay_user_fn)) {
		replay_protect_detector_error_set();
		return false;
	}
	file_seek(0, SEEK_SET);
	if(!replay_write_bytes_checked(&replay_user_header, sizeof(replay_user_header))) {
		file_close();
		replay_protect_detector_error_set();
		return false;
	}
	if(!replay_write_bytes_checked(
		&replay_user_summary_ext, sizeof(replay_user_summary_ext)
	)) {
		file_close();
		replay_protect_detector_error_set();
		return false;
	}
	if(!replay_write_bytes_checked(
		&replay_user_identity_ext, sizeof(replay_user_identity_ext)
	)) {
		file_close();
		replay_protect_detector_error_set();
		return false;
	}
	if(checkpoint_pending) {
		if(!replay_user_checkpoint_write()) {
			file_close();
			replay_protect_detector_error_set();
			return false;
		}
	}
	if(replay_write_buffer_size != 0) {
		input_offset = (
			replay_user_header.input_offset +
			replay_input_byte_count - replay_write_buffer_size
		);
		file_seek(input_offset, SEEK_SET);
		if(!replay_write_bytes_checked(
			write_buffer, replay_write_buffer_size
		)) {
			file_close();
			replay_protect_detector_error_set();
			return false;
		}
	}
	if(!replay_protect_close_current_file(RPD_COMMIT_FLUSH)) {
		return false;
	}
	replay_rle_phase &= ~REPLAY_RLE_STAGE_CHECKPOINT_PENDING_MASK;
	replay_write_buffer_size = 0;
	replay_rle_packet_state &= ~REPLAY_RLE_STATE_OPEN;
	replay_user_index_slot_write(status, end_reason);
	return true;
}

static bool replay_user_periodic_flush(void)
{
	return replay_user_header_write(RUS_RECORDING, RUER_PARTIAL);
}

static bool replay_user_header_is_rle(void)
{
	return (
		(replay_user_header.magic[6] == '1') &&
		(
			replay_user_header.magic[7] == static_cast<char>(
				'0' + (replay_user_header.version - 10)
			)
		) &&
		replay_user_version_supported(replay_user_header.version) &&
		(replay_user_header.sample_size == T3_REPLAY_USER_SAMPLE_SIZE_RLE) &&
		((replay_user_header.flags & T3_REPLAY_USER_FLAG_RLE_INPUT) != 0) &&
		((replay_user_header.flags & T3_REPLAY_USER_FLAG_CHARGE_INPUT) != 0)
	);
}

static bool replay_user_snapshot_disk_read(void)
{
	if(replay_user_header.version == T3_REPLAY_USER_VERSION_LEGACY) {
		return (
			file_read(&replay_user_snapshot, sizeof(replay_user_snapshot)) ==
			sizeof(replay_user_snapshot)
		);
	}
	if(
		file_read(
			&replay_user_snapshot,
			sizeof(replay_user_snapshot_compact_t)
		) != sizeof(replay_user_snapshot_compact_t)
	) {
		return false;
	}
	replay_user_compact_unpack();
	return true;
}

static bool replay_user_header_practice_valid(void)
{
	if(replay_user_header.flags & T3_REPLAY_USER_FLAG_PRACTICE) {
		return (
			(replay_user_header.game_mode == GM_VS_1P_CPU) &&
			practice_replay_config_valid(
				replay_user_header.scenario.practice.config
			)
		);
	}
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
		replay_user_header_is_rle() &&
		replay_user_header_practice_valid() &&
		(replay_user_header.header_size == replay_user_header_size(
			replay_user_header.version
		)) &&
		(replay_user_header.snapshot_offset == replay_user_header.header_size) &&
		(replay_user_header.snapshot_size == replay_user_checkpoint_size(
			replay_user_header.version
		)) &&
		(replay_user_header.autofire <= 0x03) &&
		(
			(replay_user_header.version == T3_REPLAY_USER_VERSION_LEGACY) ||
			(
				(replay_user_header.summary_flags &
				 T3_REPLAY_USER_SUMMARY_CURRENT) ==
				T3_REPLAY_USER_SUMMARY_CURRENT
			)
		) &&
		(replay_user_header.input_offset == replay_user_input_offset(
			replay_user_header.version,
			replay_user_header.game_mode,
			replay_user_header.flags
		)) &&
		(replay_user_header.sample_count != 0)
	);
}

static bool replay_user_identity_ext_valid(void)
{
	int i;
	bool netplay = (
		(replay_user_identity_ext.recording_flags &
		 T3R_RECORDING_FLAG_NETPLAY) != 0
	);
	if(
		(replay_user_identity_ext.ruleset != T3R_RULESET_STOCK) ||
		((replay_user_identity_ext.recording_flags &
		  ~T3R_RECORDING_FLAGS_KNOWN) != 0) ||
		(replay_user_identity_ext.recorder_role > T3R_RECORDER_ROLE_P2) ||
		(replay_user_identity_ext.recorder_source >
		 T3R_RECORDER_SOURCE_IMPORTED) ||
		(netplay != (replay_user_identity_ext.recorder_role !=
		 T3R_RECORDER_ROLE_UNKNOWN)) ||
		(netplay && (replay_user_header.game_mode != GM_VS_1P_2P))
	) {
		return false;
	}
	for(i = 0; i < T3R_IDENTITY_RESERVED_SIZE; i++) {
		if(replay_user_identity_ext.reserved[i] != 0) {
			return false;
		}
	}
	return true;
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
	replay_user_summary_ext_init();
	if(!replay_user_header_valid()) {
		file_close();
		return false;
	}
	uint16_t summary_size = replay_user_summary_ext_size(
		replay_user_header.version
	);
	if(file_read(&replay_user_summary_ext, summary_size) != summary_size) {
		file_close();
		return false;
	}
	if(
		(file_read(
			&replay_user_identity_ext, sizeof(replay_user_identity_ext)
		) != sizeof(replay_user_identity_ext)) ||
		!replay_user_identity_ext_valid()
	) {
		file_close();
		return false;
	}
	if(
		replay_user_summary_ext.round_reached_count >
		T3_REPLAY_USER_ROUND_SPLIT_COUNT
	) {
		file_close();
		return false;
	}
	if(
		replay_user_version_has_round_state(replay_user_header.version) &&
		(
			(
				(replay_user_summary_ext.flags &
				 T3_REPLAY_USER_SUMMARY_CURRENT) !=
				T3_REPLAY_USER_SUMMARY_CURRENT
			) ||
			(replay_user_summary_ext.checkpoint_count == 0) ||
			(replay_user_summary_ext.checkpoint_count >
			 replay_user_checkpoint_capacity(
				replay_user_header.game_mode, replay_user_header.flags
			 ))
		)
	) {
		file_close();
		return false;
	}
	file_seek((replay_user_header.snapshot_offset + (
		static_cast<uint32_t>(
			(replay_user_header.version == T3_REPLAY_USER_VERSION_LEGACY) ?
				(
					(replay_user_header.game_mode == GM_STORY) ?
						replay_user_header.story_stage : 0
				) : 0
		) * static_cast<uint32_t>(
			replay_user_checkpoint_size(replay_user_header.version)
		)
	) + T3R_STAGE_CKPT_PREFIX_SIZE), SEEK_SET);
	if(!replay_user_snapshot_disk_read()) {
		file_close();
		return false;
	}
	if(replay_user_snapshot.autofire != replay_user_header.autofire) {
		file_close();
		return false;
	}
	if(replay_user_version_has_round_state(replay_user_header.version)) {
		if(
			file_read(
				&replay_user_round_state, sizeof(replay_user_round_state)
			) != sizeof(replay_user_round_state)
		) {
			file_close();
			return false;
		}
	}
	replay_memclear(&replay_user_round_carry, sizeof(replay_user_round_carry));
	if(replay_user_version_has_round_carry(replay_user_header.version)) {
		if(
			file_read(
				&replay_user_round_carry, sizeof(replay_user_round_carry)
			) != sizeof(replay_user_round_carry)
		) {
			file_close();
			return false;
		}
		if(
			replay_user_round_carry.background_phase >
			T3R_BACKGROUND_TYPE_B_STEADY
		) {
			file_close();
			return false;
		}
		if(replay_user_round_carry.result_phase > T3R_RESULT_CLOSING) {
			file_close();
			return false;
		}
	}
	file_close();
	replay_user_summary_load_from_header();
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

static bool replay_user_checkpoint_snapshot_read(uint8_t checkpoint)
{
	uint32_t offset;
	uint8_t stage_round = 0;
	uint8_t stage;

	if(replay_user_version_has_round_state(replay_user_header.version)) {
		if(checkpoint >= replay_user_summary_ext.checkpoint_count) {
			return false;
		}
		stage_round = replay_user_summary_ext.checkpoint_stage_round[checkpoint];
	} else {
		if(
			(replay_user_header.game_mode != GM_STORY) ||
			(checkpoint >= T3_REPLAY_USER_STAGE_COUNT) ||
			(checkpoint >= replay_user_header.stage_reached_count)
		) {
			return false;
		}
		stage_round = checkpoint;
	}
	stage = (stage_round & 0x0F);
	offset = (
		replay_user_header.snapshot_offset +
		(
			static_cast<uint32_t>(checkpoint) *
			static_cast<uint32_t>(
				replay_user_checkpoint_size(replay_user_header.version)
			)
		) +
		T3R_STAGE_CKPT_PREFIX_SIZE
	);
	if(!file_ropen(replay_user_fn)) {
		return false;
	}
	file_seek(offset, SEEK_SET);
	if(!replay_user_snapshot_disk_read()) {
		file_close();
		return false;
	}
	if(replay_user_version_has_round_state(replay_user_header.version)) {
		if(
			file_read(
				&replay_user_round_state, sizeof(replay_user_round_state)
			) != sizeof(replay_user_round_state)
		) {
			file_close();
			return false;
		}
	}
	replay_memclear(&replay_user_round_carry, sizeof(replay_user_round_carry));
	if(replay_user_version_has_round_carry(replay_user_header.version)) {
		if(
			file_read(
				&replay_user_round_carry, sizeof(replay_user_round_carry)
			) != sizeof(replay_user_round_carry)
		) {
			file_close();
			return false;
		}
		if(
			replay_user_round_carry.background_phase >
			T3R_BACKGROUND_TYPE_B_STEADY
		) {
			file_close();
			return false;
		}
		if(replay_user_round_carry.result_phase > T3R_RESULT_CLOSING) {
			file_close();
			return false;
		}
	}
	file_close();
	// Early V12 recorders replaced each new stage's reset seed while reopening
	// checkpoint 0. resident_rand independently preserves that stage-start seed.
	if((stage_round & 0xF0) == 0) {
		replay_header.reserved_2 = replay_user_snapshot.resident_rand;
	}
	if(
		(replay_user_snapshot.game_mode != replay_user_header.game_mode) ||
		(replay_user_snapshot.autofire != replay_user_header.autofire)
	) {
		return false;
	}
	if(replay_user_header.game_mode == GM_STORY) {
		return (replay_user_snapshot.story_stage == stage);
	}
	if(replay_user_version_has_round_state(replay_user_header.version)) {
		return (replay_user_round_state.round_id == (stage_round >> 4));
	}
	return true;
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
	resident->autofire = replay_user_snapshot.autofire;

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

static bool replay_user_random_tables_restore(void)
{
	int i;
	int pid;
	uint8_t next;
	uint8_t prev;

	if(
		(formation_count == 0) ||
		(static_cast<uint8_t>(replay_header.reserved_3) >= formation_count)
	) {
		return false;
	}
	random_seed = replay_header.reserved_2;
	for(i = (RANDRING_SIZE - 1); i >= 0; i--) {
		randring[i] = irand();
	}
	randring_p = 0;
	for(pid = 0; pid < PLAYER_COUNT; pid++) {
		for(i = 0; i < CHARGE_AT_AVAIL_RING_SIZE; i++) {
			players[pid].cpu_charge_at_avail_ring[i] = (
				((irand() & 3) << 6) + 0x3F
			);
		}
		players[pid].cpu_charge_at_avail_ring_p = 0;
	}

	// The first recorded formation determines whether the original
	// uninitialized [prev] byte rejected the first RNG candidate.
	prev = 0xFF;
	for(i = 0; i < T3_REPLAY_USER_FORMATION_RING_SIZE; i++) {
		if(i == 0) {
			next = (irand() % formation_count);
			if(next != static_cast<uint8_t>(replay_header.reserved_3)) {
				prev = next;
				do {
					next = (irand() % formation_count);
				} while(next == prev);
			}
			if(next != static_cast<uint8_t>(replay_header.reserved_3)) {
				return false;
			}
		} else {
			do {
				next = (irand() % formation_count);
			} while(next == prev);
		}
		formation_type_ring[i] = next;
		prev = next;
		formation_pos_type_ring[i] = ((irand() & 1) << 7);
	}
	formation_p[0] = 0;
	formation_p[1] = 0;
	random_seed = replay_user_snapshot.random_seed_snapshot;
	return true;
}

static bool replay_user_snapshot_restore_runtime(void)
{
	int i;
	int digit;

	if(replay_user_header.version == T3_REPLAY_USER_VERSION_LEGACY) {
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
		}
	} else if(!replay_user_random_tables_restore()) {
		return false;
	}
	for(i = 0; i < PLAYER_COUNT; i++) {
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
	return true;
}

static void replay_user_round_state_restore(void)
{
	int i;
	int digit;

	round_id = replay_user_round_state.round_id;
	for(i = 0; i < PLAYER_COUNT; i++) {
		players[i].rounds_won = replay_user_round_state.rounds_won[i];
		for(digit = 0; digit < SCORE_DIGITS; digit++) {
			score[(i * SCORE_DIGITS) + digit] = (
				replay_user_round_state.score[i][digit]
			);
		}
		gba_gauge_level[i] = replay_user_round_state.spell_rank[i];
		players[i].cpu_safety_frames = (
			replay_user_round_state.cpu_safety_frames[i]
		);
	}
	round_speed = replay_user_round_state.round_speed;
	bullet_base_speed.v = replay_user_round_state.bullet_speed;
	gba_boss_level = replay_user_round_state.boss_rank;
	cpu_hit_damage_additional = replay_user_round_state.cpu_damage;
	extends_gained = replay_user_round_state.extends_gained;
}

static void replay_user_round_carry_restore(void)
{
	int i;

	for(i = 0; i < PLAYER_COUNT; i++) {
		players[i].cpu_dodge_strategy = (
			replay_user_round_carry.cpu_dodge_strategy[i]
		);
		players[i].human_movement_last = static_cast<input_t>(
			replay_user_round_carry.human_movement_last[i]
		);
		players[i].spell_ready_frames = (
			replay_user_round_carry.spell_ready_frames[i]
		);
		players[i].combo_bonus_max = replay_user_round_carry.combo_bonus_max[i];
		players[i].combo_hits_max = replay_user_round_carry.combo_hits_max[i];
		byte_220FC[i] = replay_user_round_carry.shot_cycle[i];
		boss_panic_fired_in_current_combo[i] = (
			replay_user_round_carry.boss_panic_fired[i]
		);
		gba_flag_next[i] = static_cast<gba_flag_t>(
			replay_user_round_carry.gba_flag_next[i]
		);
	}
	byte_20E48 = replay_user_round_carry.cpu_shot_decision;
	randring_p = replay_user_round_carry.randring_p;
	switch(replay_user_round_carry.background_phase) {
	case T3R_BACKGROUND_TYPE_A_STEADY:
		farfp_20F24 = sub_D1E7;
		break;
	case T3R_BACKGROUND_TYPE_B_STEADY:
		farfp_20F24 = sub_D3F9;
		break;
	}
	switch(replay_user_round_carry.result_phase) {
	case T3R_RESULT_OPENING:
		fp_1FBC0 = sub_B4A8;
		break;
	case T3R_RESULT_CLOSING:
		fp_1FBC0 = sub_B60A;
		break;
	default:
		fp_1FBC0 = sub_B4A3;
		break;
	}
	replay_user_carry_chains_restore();
}

static bool replay_user_create(void)
{
	uint8_t slot = replay_resident_slot();

	replay_user_snapshot_fill();
	replay_user_summary_init_from_snapshot();
	replay_user_summary_ext_init();
	replay_user_header_fill(RUS_RECORDING, RUER_PARTIAL);
	replay_user_slot_fn_set(slot);
	if(replay_protect_blocked()) {
		return true;
	}
	if(replay_protect_located()) {
		return true;
	}
	if(slot < T3_REPLAY_USER_SLOT_COUNT) {
		replay_dir_create();
	}
	if(!file_create(replay_user_fn)) {
		replay_user_slot_fn_set(T3_REPLAY_USER_SLOT_NONE);
		if(!file_create(replay_user_fn)) {
			return false;
		}
	}
	if(!replay_protect_close_current_file(RPD_COMMIT_FLUSH)) {
		return false;
	}
	if(!replay_user_guard_create()) {
		replay_guard_diag_write();
		replay_user_index_slot_clear();
		return true;
	}
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
	if(!replay_user_guard_checkpoint()) {
		return true;
	}
	if(!file_append(replay_user_fn)) {
		replay_protect_detector_error_set();
		return true;
	}
	file_seek(offset, SEEK_SET);
	if(!replay_write_bytes_checked(&sample, sizeof(sample))) {
		file_close();
		replay_protect_detector_error_set();
		return true;
	}
	file_close();
	replay_sample_count++;
	return true;
}

static bool replay_user_record_interstitial_sample(void)
{
	replay_user_sample_t sample;
	uint32_t offset;

	sample.frame_index = replay_global_frame;
	sample.input_mp_p1 = input_mp_p1;
	sample.input_mp_p2 = input_mp_p2;
	sample.input_sp = input_sp;
	sample.round_or_result_frame = T3_REPLAY_INTERSTITIAL_ROUND_OR_RESULT_FRAME;
	sample.round_frame = T3_REPLAY_INTERSTITIAL_ROUND_FRAME;

	offset = (
		replay_user_header.input_offset +
		(replay_sample_count * static_cast<uint32_t>(sizeof(sample)))
	);
	if(!replay_user_guard_checkpoint()) {
		return true;
	}
	if(!file_append(replay_user_fn)) {
		replay_protect_detector_error_set();
		return true;
	}
	file_seek(offset, SEEK_SET);
	if(!replay_write_bytes_checked(&sample, sizeof(sample))) {
		file_close();
		replay_protect_detector_error_set();
		return true;
	}
	file_close();
	replay_sample_count++;
	return true;
}

static uint8_t replay_user_rle_tag(uint8_t phase, uint8_t run)
{
	return static_cast<uint8_t>(
		(phase << T3_REPLAY_PACKET_PHASE_SHIFT) | (run - 1)
	);
}

static bool replay_user_buffer_u8(uint8_t value)
{
	uint8_t far *write_buffer = reinterpret_cast<uint8_t far *>(
		MK_FP(replay_write_buffer_seg, 0)
	);

	if(replay_write_buffer_size >= T3_REPLAY_WRITE_BUFFER_SIZE) {
		return false;
	}
	write_buffer[replay_write_buffer_size++] = value;
	replay_input_byte_count++;
	return true;
}

static bool replay_user_buffer_u16(uint16_t value)
{
	uint8_t far *write_buffer = reinterpret_cast<uint8_t far *>(
		MK_FP(replay_write_buffer_seg, 0)
	);

	if(
		(replay_write_buffer_size + sizeof(value)) >
		T3_REPLAY_WRITE_BUFFER_SIZE
	) {
		return false;
	}
	write_buffer[replay_write_buffer_size++] = static_cast<uint8_t>(value);
	write_buffer[replay_write_buffer_size++] = static_cast<uint8_t>(
		value >> 8
	);
	replay_input_byte_count += sizeof(value);
	return true;
}

static bool replay_user_control_write(uint8_t control)
{
	uint8_t tag = static_cast<uint8_t>(
		(T3_REPLAY_PACKET_PHASE_CONTROL << T3_REPLAY_PACKET_PHASE_SHIFT) |
		control
	);
	uint8_t marker = T3_REPLAY_PACKET_CONTROL_MARKER;
	if(replay_protect_blocked()) {
		return true;
	}
	if(
		((replay_write_buffer_size + 2) > T3_REPLAY_WRITE_BUFFER_SIZE) &&
		!replay_user_periodic_flush()
	) {
		return false;
	}
	if(!replay_user_buffer_u8(tag) || !replay_user_buffer_u8(marker)) {
		replay_protect_detector_error_set();
		return false;
	}
	replay_rle_packet_state &= ~REPLAY_RLE_STATE_OPEN;
	return true;
}

static bool replay_user_control_consume(uint8_t control)
{
	uint8_t tag;
	uint8_t marker;
	uint32_t offset = (replay_user_header.input_offset + replay_input_byte_count);

	if(
		(replay_rle_run != 0) ||
		((replay_input_byte_count + 2) > replay_user_header.input_size)
	) {
		return false;
	}
	if(!file_ropen(replay_user_fn)) {
		return false;
	}
	file_seek(offset, SEEK_SET);
	if(
		(file_read(&tag, sizeof(tag)) != sizeof(tag)) ||
		(file_read(&marker, sizeof(marker)) != sizeof(marker))
	) {
		file_close();
		return false;
	}
	file_close();
	if(
		(tag != static_cast<uint8_t>(
			(T3_REPLAY_PACKET_PHASE_CONTROL << T3_REPLAY_PACKET_PHASE_SHIFT) |
			control
		)) ||
		(marker != T3_REPLAY_PACKET_CONTROL_MARKER)
	) {
		return false;
	}
	replay_input_byte_count += 2;
	return true;
}

static bool replay_user_record_rle_sample(uint8_t phase, uint8_t shot_bits)
{
	uint16_t input_p1 = input_mp_p1;
	uint16_t input_p2 = input_mp_p2;
	uint16_t input_single = input_sp;
	uint8_t input_charge = (resident->input_charge & 0x03);
	uint8_t previous_charge = static_cast<uint8_t>(
		(replay_rle_packet_state & REPLAY_RLE_STATE_CHARGE_MASK) >>
		REPLAY_RLE_STATE_CHARGE_SHIFT
	);
	uint8_t packet_size = static_cast<uint8_t>(
		(replay_rle_phase & REPLAY_RLE_PACKET_SIZE_MASK) >>
		REPLAY_RLE_PACKET_SIZE_SHIFT
	);
	uint8_t far *write_buffer = reinterpret_cast<uint8_t far *>(
		MK_FP(replay_write_buffer_seg, 0)
	);
	uint8_t change = 0;
	uint8_t tag;

	if(phase == T3_REPLAY_PACKET_PHASE_GAMEPLAY) {
		input_p1 &= ~INPUT_SHOT;
		input_p2 &= ~INPUT_SHOT;
		if(shot_bits & 0x01) {
			input_p1 |= INPUT_SHOT;
		}
		if(shot_bits & 0x02) {
			input_p2 |= INPUT_SHOT;
		}
	}

	if(replay_protect_blocked()) {
		return true;
	}
	if(
		(replay_write_buffer_size >
		 (T3_REPLAY_WRITE_BUFFER_SIZE - T3_REPLAY_PACKET_SIZE_MAX)) &&
		!replay_user_periodic_flush()
	) {
		return true;
	}

	if(
		(replay_rle_packet_state & REPLAY_RLE_STATE_OPEN) &&
		((replay_rle_phase & REPLAY_RLE_PHASE_MASK) == phase) &&
		(replay_rle_input_mp_p1 == input_p1) &&
		(replay_rle_input_mp_p2 == input_p2) &&
		(replay_rle_input_sp == input_single) &&
		(previous_charge == input_charge) &&
		(replay_rle_run < T3_REPLAY_PACKET_RUN_MAX)
	) {
		tag = replay_user_rle_tag(phase, replay_rle_run + 1);
		replay_rle_run++;
		write_buffer[replay_write_buffer_size - packet_size] = tag;
		replay_sample_count++;
		return true;
	}

	if(!(replay_rle_packet_state & REPLAY_RLE_STATE_OPEN)) {
		change = (
			T3_REPLAY_PACKET_CHANGE_P1 |
			T3_REPLAY_PACKET_CHANGE_P2 |
			T3_REPLAY_PACKET_CHANGE_SP |
			T3_REPLAY_PACKET_CHANGE_CHARGE
		);
	} else {
		if(input_p1 != replay_rle_input_mp_p1) {
			change |= T3_REPLAY_PACKET_CHANGE_P1;
		}
		if(input_p2 != replay_rle_input_mp_p2) {
			change |= T3_REPLAY_PACKET_CHANGE_P2;
		}
		if(input_single != replay_rle_input_sp) {
			change |= T3_REPLAY_PACKET_CHANGE_SP;
		}
		if(input_charge != previous_charge) {
			change |= T3_REPLAY_PACKET_CHANGE_CHARGE;
		}
	}

	tag = replay_user_rle_tag(phase, 1);
	packet_size = replay_write_buffer_size;
	if(!replay_user_buffer_u8(tag) || !replay_user_buffer_u8(change)) {
		replay_protect_detector_error_set();
		return true;
	}
	if(change & T3_REPLAY_PACKET_CHANGE_P1) {
		if(!replay_user_buffer_u16(input_p1)) {
			replay_protect_detector_error_set();
			return true;
		}
	}
	if(change & T3_REPLAY_PACKET_CHANGE_P2) {
		if(!replay_user_buffer_u16(input_p2)) {
			replay_protect_detector_error_set();
			return true;
		}
	}
	if(change & T3_REPLAY_PACKET_CHANGE_SP) {
		if(!replay_user_buffer_u16(input_single)) {
			replay_protect_detector_error_set();
			return true;
		}
	}
	if(change & T3_REPLAY_PACKET_CHANGE_CHARGE) {
		if(!replay_user_buffer_u8(input_charge)) {
			replay_protect_detector_error_set();
			return true;
		}
	}
	packet_size = static_cast<uint8_t>(replay_write_buffer_size - packet_size);
	replay_rle_phase = static_cast<uint8_t>(
		(replay_rle_phase & REPLAY_RLE_STAGE_CHECKPOINT_PENDING_MASK) |
		phase |
		(packet_size << REPLAY_RLE_PACKET_SIZE_SHIFT)
	);
	replay_rle_packet_state = static_cast<uint8_t>(
		REPLAY_RLE_STATE_OPEN |
		(input_charge << REPLAY_RLE_STATE_CHARGE_SHIFT)
	);
	replay_rle_run = 1;
	replay_rle_input_mp_p1 = input_p1;
	replay_rle_input_mp_p2 = input_p2;
	replay_rle_input_sp = input_single;
	replay_sample_count++;
	return true;
}

static bool replay_user_record_logical_sample(uint8_t phase, uint8_t shot_bits)
{
	if(replay_user_header_is_rle()) {
		return replay_user_record_rle_sample(phase, shot_bits);
	}
	if(phase == T3_REPLAY_PACKET_PHASE_INTERSTITIAL) {
		return replay_user_record_interstitial_sample();
	}
	return replay_user_record_sample();
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
		(sample.round_frame == T3_REPLAY_INTERSTITIAL_ROUND_FRAME) ||
		(
			sample.round_or_result_frame ==
			T3_REPLAY_INTERSTITIAL_ROUND_OR_RESULT_FRAME
		) ||
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
		// Backward compatibility with recordings made before prompt samples.
		input_sp = 0;
		byte_23B00 = 1;
		replay_prompt_skip_queued = true;
	}
	return true;
}

static bool replay_user_read_rle_packet(void)
{
	uint8_t tag;
	uint8_t change;
	uint8_t phase;
	uint8_t charge_input = static_cast<uint8_t>(
		(replay_rle_packet_state & REPLAY_RLE_STATE_CHARGE_MASK) >>
		REPLAY_RLE_STATE_CHARGE_SHIFT
	);
	uint32_t offset = (replay_user_header.input_offset + replay_input_byte_count);

	if((replay_input_byte_count + 2) > replay_user_header.input_size) {
		return false;
	}
	if(!file_ropen(replay_user_fn)) {
		return false;
	}
	file_seek(offset, SEEK_SET);
	if(
		(file_read(&tag, sizeof(tag)) != sizeof(tag)) ||
		(file_read(&change, sizeof(change)) != sizeof(change))
	) {
		file_close();
		return false;
	}
	replay_input_byte_count += 2;
	phase = static_cast<uint8_t>(tag >> T3_REPLAY_PACKET_PHASE_SHIFT);
	replay_rle_phase = phase;
	replay_rle_run = static_cast<uint8_t>(
		(tag & T3_REPLAY_PACKET_RUN_MASK) + 1
	);
	if(phase > T3_REPLAY_PACKET_PHASE_INTERSTITIAL) {
		file_close();
		return false;
	}
	if(change & T3_REPLAY_PACKET_CHANGE_P1) {
		if(
			(replay_input_byte_count + sizeof(replay_rle_input_mp_p1)) >
			replay_user_header.input_size
		) {
			file_close();
			return false;
		}
		if(
			file_read(
				&replay_rle_input_mp_p1, sizeof(replay_rle_input_mp_p1)
			) != sizeof(replay_rle_input_mp_p1)
		) {
			file_close();
			return false;
		}
		replay_input_byte_count += sizeof(replay_rle_input_mp_p1);
	}
	if(change & T3_REPLAY_PACKET_CHANGE_P2) {
		if(
			(replay_input_byte_count + sizeof(replay_rle_input_mp_p2)) >
			replay_user_header.input_size
		) {
			file_close();
			return false;
		}
		if(
			file_read(
				&replay_rle_input_mp_p2, sizeof(replay_rle_input_mp_p2)
			) != sizeof(replay_rle_input_mp_p2)
		) {
			file_close();
			return false;
		}
		replay_input_byte_count += sizeof(replay_rle_input_mp_p2);
	}
	if(change & T3_REPLAY_PACKET_CHANGE_SP) {
		if(
			(replay_input_byte_count + sizeof(replay_rle_input_sp)) >
			replay_user_header.input_size
		) {
			file_close();
			return false;
		}
		if(
			file_read(&replay_rle_input_sp, sizeof(replay_rle_input_sp)) !=
			sizeof(replay_rle_input_sp)
		) {
			file_close();
			return false;
		}
		replay_input_byte_count += sizeof(replay_rle_input_sp);
	}
	if(change & T3_REPLAY_PACKET_CHANGE_CHARGE) {
		if(
			(replay_input_byte_count + sizeof(charge_input)) >
			replay_user_header.input_size
		) {
			file_close();
			return false;
		}
		if(
			file_read(
				&charge_input, sizeof(charge_input)
			) != sizeof(charge_input)
		) {
			file_close();
			return false;
		}
		replay_input_byte_count += sizeof(charge_input);
	}
	if(charge_input > 0x03) {
		file_close();
		return false;
	}
	replay_rle_phase = phase;
	replay_rle_packet_state = static_cast<uint8_t>(
		(replay_rle_packet_state & REPLAY_RLE_STATE_OPEN) |
		(charge_input << REPLAY_RLE_STATE_CHARGE_SHIFT)
	);
	if(change & ~(
		T3_REPLAY_PACKET_CHANGE_P1 |
		T3_REPLAY_PACKET_CHANGE_P2 |
		T3_REPLAY_PACKET_CHANGE_SP |
		T3_REPLAY_PACKET_CHANGE_CHARGE
	)) {
		file_close();
		return false;
	}
	file_close();
	return true;
}

static bool replay_seek_read_u8(
	replay_seek_reader_t _ss *reader, uint8_t _ss *value
)
{
	unsigned read_size;

	if(reader->cursor >= reader->size) {
		if(reader->remaining == 0) {
			return false;
		}
		read_size = static_cast<unsigned>(
			(reader->remaining > REPLAY_SEEK_BUFFER_SIZE) ?
				REPLAY_SEEK_BUFFER_SIZE : reader->remaining
		);
		if(file_read(reader->buffer, read_size) != read_size) {
			return false;
		}
		reader->remaining -= read_size;
		reader->cursor = 0;
		reader->size = read_size;
	}
	*value = reader->buffer[reader->cursor++];
	return true;
}

static bool replay_seek_read_u16(
	replay_seek_reader_t _ss *reader, uint16_t _ss *value
)
{
	uint8_t lo;
	uint8_t hi;

	if(!replay_seek_read_u8(reader, &lo) || !replay_seek_read_u8(reader, &hi)) {
		return false;
	}
	*value = static_cast<uint16_t>(lo | (hi << 8));
	return true;
}

static bool replay_user_decoder_seek(
	uint32_t target_sample, uint32_t target_byte
)
{
	replay_seek_reader_t reader;
	uint16_t buffer_seg;
	uint32_t consumed = 0;
	uint32_t decoded = 0;
	uint32_t packet_sample_start = 0;
	uint16_t input_p1 = 0;
	uint16_t input_p2 = 0;
	uint16_t input_single = 0;
	uint8_t input_charge = 0;
	uint8_t last_phase = T3_REPLAY_PACKET_PHASE_GAMEPLAY;
	uint8_t tag;
	uint8_t change;
	uint8_t phase;
	uint8_t run;
	bool last_packet_is_input = false;
	bool ok = false;

	if((target_sample > replay_user_header.sample_count) ||
		(target_byte > replay_user_header.input_size)) {
		return false;
	}
	if(target_byte == 0) {
		return (target_sample == 0);
	}

	buffer_seg = reinterpret_cast<uint16_t>(
		hmem_allocbyte(REPLAY_SEEK_BUFFER_SIZE)
	);
	if(buffer_seg == 0) {
		return false;
	}
	reader.buffer = reinterpret_cast<uint8_t far *>(MK_FP(buffer_seg, 0));
	reader.cursor = 0;
	reader.size = 0;
	reader.remaining = target_byte;

	if(!file_ropen(replay_user_fn)) {
		goto cleanup;
	}
	file_seek(replay_user_header.input_offset, SEEK_SET);
	while(consumed < target_byte) {
		if(!replay_seek_read_u8(&reader, &tag) ||
			!replay_seek_read_u8(&reader, &change)) {
			goto close;
		}
		consumed += 2;
		phase = static_cast<uint8_t>(tag >> T3_REPLAY_PACKET_PHASE_SHIFT);
		if(phase == T3_REPLAY_PACKET_PHASE_CONTROL) {
			if((change != T3_REPLAY_PACKET_CONTROL_MARKER) ||
				((tag & T3_REPLAY_PACKET_RUN_MASK) >
				 T3_REPLAY_PACKET_CONTROL_MAINL_END)) {
				goto close;
			}
			last_packet_is_input = false;
			continue;
		}
		if((phase > T3_REPLAY_PACKET_PHASE_INTERSTITIAL) ||
			(change & ~(T3_REPLAY_PACKET_CHANGE_P1 |
			 T3_REPLAY_PACKET_CHANGE_P2 |
			 T3_REPLAY_PACKET_CHANGE_SP |
			 T3_REPLAY_PACKET_CHANGE_CHARGE))) {
			goto close;
		}
		if(change & T3_REPLAY_PACKET_CHANGE_P1) {
			if(!replay_seek_read_u16(&reader, &input_p1)) {
				goto close;
			}
			consumed += sizeof(input_p1);
		}
		if(change & T3_REPLAY_PACKET_CHANGE_P2) {
			if(!replay_seek_read_u16(&reader, &input_p2)) {
				goto close;
			}
			consumed += sizeof(input_p2);
		}
		if(change & T3_REPLAY_PACKET_CHANGE_SP) {
			if(!replay_seek_read_u16(&reader, &input_single)) {
				goto close;
			}
			consumed += sizeof(input_single);
		}
		if(change & T3_REPLAY_PACKET_CHANGE_CHARGE) {
			if(!replay_seek_read_u8(&reader, &input_charge)) {
				goto close;
			}
			consumed++;
			if(input_charge > 0x03) {
				goto close;
			}
		}
		packet_sample_start = decoded;
		run = static_cast<uint8_t>((tag & T3_REPLAY_PACKET_RUN_MASK) + 1);
		decoded += run;
		last_phase = phase;
		last_packet_is_input = true;
	}

	if(consumed != target_byte) {
		goto close;
	}
	if(last_packet_is_input) {
		if((target_sample < packet_sample_start) || (target_sample > decoded)) {
			goto close;
		}
		replay_rle_run = static_cast<uint8_t>(decoded - target_sample);
	} else {
		if(target_sample != decoded) {
			goto close;
		}
		replay_rle_run = 0;
	}
	replay_rle_phase = last_phase;
	replay_rle_input_mp_p1 = input_p1;
	replay_rle_input_mp_p2 = input_p2;
	replay_rle_input_sp = input_single;
	replay_rle_packet_state = static_cast<uint8_t>(
		input_charge << REPLAY_RLE_STATE_CHARGE_SHIFT
	);
	replay_input_byte_count = target_byte;
	ok = true;

close:
	file_close();
cleanup:
	hmem_free(reinterpret_cast<void __seg *>(buffer_seg));
	return ok;
}

static void replay_user_decoder_inputs_restore(void)
{
	// At an exact packet boundary, seek leaves the most recently completed
	// packet in the decoder fields even though no packet is active.
	if(replay_rle_run == 0) {
		input_mp_p1 = INPUT_NONE;
		input_mp_p2 = INPUT_NONE;
		input_sp = INPUT_NONE;
		resident->input_charge = 0;
		return;
	}
	input_mp_p1 = replay_rle_input_mp_p1;
	input_mp_p2 = replay_rle_input_mp_p2;
	input_sp = replay_rle_input_sp;
	resident->input_charge = static_cast<uint8_t>(
		(replay_rle_packet_state & REPLAY_RLE_STATE_CHARGE_MASK) >>
		REPLAY_RLE_STATE_CHARGE_SHIFT
	);
}

static bool replay_accel_checkpoint_cursor_read(
	uint8_t checkpoint,
	uint32_t _ss *sample,
	uint32_t _ss *global_frame,
	uint32_t _ss *input_byte
)
{
	uint32_t offset = (
		replay_user_header.snapshot_offset +
		(
			static_cast<uint32_t>(checkpoint) *
			static_cast<uint32_t>(
				replay_user_checkpoint_size(replay_user_header.version)
			)
		)
	);
	bool ok;

	if(!file_ropen(replay_user_fn)) {
		return false;
	}
	file_seek(offset, SEEK_SET);
	ok = (
		(file_read(sample, sizeof(*sample)) == sizeof(*sample)) &&
		(file_read(global_frame, sizeof(*global_frame)) ==
		 sizeof(*global_frame)) &&
		(file_read(input_byte, sizeof(*input_byte)) == sizeof(*input_byte))
	);
	file_close();
	return ok;
}

static bool replay_accel_direct_start(void)
{
	uint32_t sample;
	uint32_t global_frame;
	uint32_t input_byte;
	uint8_t checkpoint = replay_accel_target_checkpoint;
	uint8_t far *raw = reinterpret_cast<uint8_t far *>(
		MK_FP(replay_accel_raw_seg, 0)
	);

	if(
		(replay_accel_raw_seg == 0) ||
		!replay_accel_checkpoint_cursor_read(
			checkpoint, &sample, &global_frame, &input_byte
		) ||
		!replay_user_checkpoint_snapshot_read(checkpoint) ||
		!replay_user_decoder_seek(sample, input_byte)
	) {
		return false;
	}
	replay_sample_count = sample;
	replay_global_frame = global_frame;
	replay_user_decoder_inputs_restore();
	replay_user_snapshot_restore_resident();
	if(!replay_user_snapshot_restore_runtime()) {
		return false;
	}
	replay_user_round_state_restore();
	replay_user_round_carry_restore();
	replay_accel_raw_apply(raw);

	// The broad image contains recording-process far pointers. Restore the
	// normalized portable phases after preserving this process's pointers.
	replay_user_round_carry_restore();
	replay_user_decoder_inputs_restore();
	hmem_free(reinterpret_cast<void __seg *>(replay_accel_raw_seg));
	replay_accel_raw_seg = 0;
	replay_accel_target_checkpoint = 0;
	resident->unused_3[T3_REPLAY_RES_PREROLL_TARGET_INDEX] = 0;
	resident->unused_3[T3R_RES_PREROLL_FORCE_INDEX] = 0;
	resident->unused_3[T3_RES_FAST_FORWARD_REPLAY_PHASE_INDEX] = 0;
#if defined(TH03_REPLAY_DEVTOOLS)
	replay_state_probe_checkpoint = checkpoint;
	replay_state_probe_pending = true;
#endif
	replay_preroll_display_show();
	return true;
}

static bool replay_user_play_rle_sample(uint8_t phase)
{
	if(replay_sample_count >= replay_user_header.sample_count) {
#if defined(TH03_REPLAY_DEVTOOLS)
		replay_debug_transition_write(0xE4, phase);
#endif
		return false;
	}
	if(replay_rle_run == 0) {
		if(!replay_user_read_rle_packet()) {
#if defined(TH03_REPLAY_DEVTOOLS)
			replay_debug_transition_write(0xE1, phase);
#endif
			return false;
		}
	}
	if((replay_rle_phase & REPLAY_RLE_PHASE_MASK) != phase) {
#if defined(TH03_REPLAY_DEVTOOLS)
		replay_debug_transition_write(0xE2, phase);
#endif
		return false;
	}
	input_mp_p1 = replay_rle_input_mp_p1;
	input_mp_p2 = replay_rle_input_mp_p2;
	input_sp = replay_rle_input_sp;
	resident->input_charge = static_cast<uint8_t>(
		(replay_rle_packet_state & REPLAY_RLE_STATE_CHARGE_MASK) >>
		REPLAY_RLE_STATE_CHARGE_SHIFT
	);
	replay_rle_run--;
	replay_sample_count++;
	return true;
}

static bool replay_user_play_logical_sample(uint8_t phase)
{
	bool ok;

	if(replay_user_header_is_rle()) {
		ok = replay_user_play_rle_sample(phase);
	} else if(phase == T3_REPLAY_PACKET_PHASE_INTERSTITIAL) {
		ok = replay_user_play_interstitial_sample();
	} else {
		ok = replay_user_play_sample();
	}
	if(ok && (phase == T3_REPLAY_PACKET_PHASE_GAMEPLAY)) {
		replay_autofire_apply();
	}
	return ok;
}

static bool replay_user_playback_cancel(void)
{
	if(input_sp & INPUT_CANCEL) {
#if defined(TH03_REPLAY_DEVTOOLS)
		replay_debug_transition_write(
			0xE0, T3_REPLAY_PACKET_PHASE_GAMEPLAY
		);
#endif
		input_sp = INPUT_NONE;
		byte_23B00 = 1;
		replay_prompt_skip_queued = true;
		return true;
	}
	return false;
}

static void replay_user_playback_error_finish(void)
{
#if defined(TH03_REPLAY_DEVTOOLS)
	replay_debug_transition_write(0xE3, replay_rle_phase);
#endif
	replay_split_row(RSE_ERROR, replay_last_route);
	replay_done_write(RTX_ERROR_FRAME_IO);
	replay_resident_handoff_clear();
	resident->game_mode = GM_NONE;
	input_mp_p1 = INPUT_NONE;
	input_mp_p2 = INPUT_NONE;
	input_sp = INPUT_NONE;
	byte_23B00 = 1;
	replay_prompt_skip_queued = true;
	replay_mode = REPLAY_DISABLED;
}

static bool replay_user_play_interstitial_sample(void)
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
		(sample.round_frame != T3_REPLAY_INTERSTITIAL_ROUND_FRAME) ||
		(
			sample.round_or_result_frame !=
			T3_REPLAY_INTERSTITIAL_ROUND_OR_RESULT_FRAME
		)
	) {
		return false;
	}

	input_mp_p1 = sample.input_mp_p1;
	input_mp_p2 = sample.input_mp_p2;
	input_sp = sample.input_sp;
	replay_sample_count++;
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
	unsigned i;
	char mode = '\0';

	replay_memclear(cfg, sizeof(cfg));
	if(!file_ropen(T3_REPLAY_CFG_FN)) {
		return REPLAY_DISABLED;
	}
	read_len = file_read(cfg, (sizeof(cfg) - 1));
	file_close();

	for(i = 0; i < read_len; i++) {
		if(
			(cfg[i] != ' ') &&
			(cfg[i] != '\t') &&
			(cfg[i] != '\r') &&
			(cfg[i] != '\n')
		) {
			mode = cfg[i];
			break;
		}
	}

	if(replay_char_ieq(mode, 'r')) {
		return REPLAY_RECORD;
	}
	if(replay_char_ieq(mode, 'p')) {
		return REPLAY_PLAYBACK;
	}
	if(replay_char_ieq(mode, 'v')) {
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

static uint8_t replay_handoff_u8(unsigned index)
{
	return static_cast<uint8_t>(resident->unused_3[index]);
}

static uint32_t replay_handoff_u32_read(unsigned index)
{
	return (
		static_cast<uint32_t>(replay_handoff_u8(index)) |
		(static_cast<uint32_t>(replay_handoff_u8(index + 1)) << 8) |
		(static_cast<uint32_t>(replay_handoff_u8(index + 2)) << 16) |
		(static_cast<uint32_t>(replay_handoff_u8(index + 3)) << 24)
	);
}

static void replay_handoff_u32_write(unsigned index, uint32_t value)
{
	resident->unused_3[index + 0] = static_cast<uint8_t>(value);
	resident->unused_3[index + 1] = static_cast<uint8_t>(value >> 8);
	resident->unused_3[index + 2] = static_cast<uint8_t>(value >> 16);
	resident->unused_3[index + 3] = static_cast<uint8_t>(value >> 24);
}

static void replay_handoff_cursor_store(void)
{
	if(
		(replay_mode != REPLAY_USER_RECORD) &&
		(replay_mode != REPLAY_USER_PLAYBACK)
	) {
		return;
	}
	replay_handoff_u32_write(
		T3_REPLAY_RES_SAMPLE_COUNT_INDEX, replay_sample_count
	);
	replay_handoff_u32_write(
		T3_REPLAY_RES_GLOBAL_FRAME_INDEX, replay_global_frame
	);
	replay_handoff_u32_write(
		T3_REPLAY_RES_INPUT_SIZE_INDEX, replay_input_byte_count
	);
}

static void replay_user_sample_commit(void)
{
	if(
		(replay_global_frame & (T3_REPLAY_DISK_INTERVAL_SAMPLES - 1)) ==
		(T3_REPLAY_DISK_INTERVAL_SAMPLES - 1)
	) {
		replay_split_row(RSE_CHECKPOINT, replay_last_route);
		if(replay_mode == REPLAY_USER_RECORD) {
			replay_user_periodic_flush();
			replay_handoff_cursor_store();
		}
	}
	replay_global_frame++;
	t3pix_logical_identity_set(
		replay_sample_count, replay_global_frame, round_frame
	);
	replay_handoff_cursor_store();
}

static void replay_resident_handoff_clear(void)
{
	int i;

	if(replay_preroll_simulating()) {
		replay_preroll_display_show();
	}
	resident->unused_3[0] = 0;
	resident->unused_3[1] = 0;
	resident->unused_3[2] = 0;
	resident->unused_3[3] = 0;
	resident->unused_3[T3_REPLAY_RES_MODE_INDEX] = 0;
	resident->unused_3[T3_REPLAY_RES_SLOT_INDEX] = T3_REPLAY_USER_SLOT_NONE;
	for(
		i = T3_REPLAY_RES_SAMPLE_COUNT_INDEX;
		i < T3_REPLAY_RES_CURSOR_END_INDEX;
		i++
	) {
		resident->unused_3[i] = 0;
	}
}

static void replay_resident_handoff_mode_set(uint8_t mode)
{
	replay_resident_handoff_clear();
	resident->unused_3[0] = T3_REPLAY_RES_MAGIC_0;
	resident->unused_3[1] = T3_REPLAY_RES_MAGIC_1;
	resident->unused_3[2] = T3_REPLAY_RES_MAGIC_2;
	resident->unused_3[3] = T3_REPLAY_RES_MAGIC_3;
	resident->unused_3[T3_REPLAY_RES_MODE_INDEX] = mode;
}

static void replay_user_record_error_disable(
	uint8_t diag_code, replay_text_id_t done_text
)
{
	if(replay_handoff_u8(T3R_DIAG_CODE_INDEX) == RPD_NONE) {
		replay_protect_diag_code_set(diag_code);
	}
	replay_protect_detector_error_set();
	replay_guard_diag_write();
	replay_done_write(done_text);
	replay_protect_local_free();
	replay_resident_handoff_clear();
	replay_mode = REPLAY_DISABLED;
}

#define PAUSE_GAIJI_FILE_SIZE 1312
#define PAUSE_GAIJI_HEADER_SIZE 32
#define PAUSE_GAIJI_TILE_SIZE 32
#define PAUSE_GAIJI_TILE_COUNT 40
#define PAUSE_GAIJI_FIRST_START 0x30
#define PAUSE_GAIJI_FIRST_COUNT 32
#define PAUSE_GAIJI_SECOND_START 0xD3
#define PAUSE_GAIJI_SECOND_COUNT 8

static uint16_t replay_pause_gaiji_u16(const uint8_t far *p)
{
	return (p[0] | (p[1] << 8));
}

static bool16 replay_pause_gaiji_validate(const uint8_t far *data)
{
	unsigned i;
	uint16_t checksum = 0;

	if(
		(data[0] != 'T') ||
		(data[1] != '3') ||
		(data[2] != 'P') ||
		(data[3] != 'G') ||
		(data[4] != 'J') ||
		(data[5] != '1') ||
		(data[6] != 0) ||
		(data[7] != 0) ||
		(replay_pause_gaiji_u16(&data[8]) != PAUSE_GAIJI_FILE_SIZE) ||
		(replay_pause_gaiji_u16(&data[10]) != PAUSE_GAIJI_HEADER_SIZE) ||
		(replay_pause_gaiji_u16(&data[12]) != PAUSE_GAIJI_TILE_COUNT) ||
		(data[14] != PAUSE_GAIJI_FIRST_START) ||
		(data[15] != PAUSE_GAIJI_FIRST_COUNT) ||
		(data[16] != PAUSE_GAIJI_SECOND_START) ||
		(data[17] != PAUSE_GAIJI_SECOND_COUNT) ||
		(data[18] != 16) ||
		(data[19] != 16) ||
		(replay_pause_gaiji_u16(&data[20]) != PAUSE_GAIJI_HEADER_SIZE)
	) {
		return false;
	}
	for(i = 24; i < PAUSE_GAIJI_HEADER_SIZE; i++) {
		if(data[i] != 0) {
			return false;
		}
	}
	for(i = PAUSE_GAIJI_HEADER_SIZE; i < PAUSE_GAIJI_FILE_SIZE; i++) {
		checksum += data[i];
	}
	return (checksum == replay_pause_gaiji_u16(&data[22]));
}

static bool16 replay_pause_gaiji_load(void)
{
	menu_font_t loaded = 0;
	bool16 valid = false;
	uint8_t extra;
	unsigned i;
	unsigned gaiji;
	char archive_fn[10];
	char gaiji_fn[10];

	if(!menu_font) {
		return false;
	}
	archive_fn[0] = 'A'; archive_fn[1] = 'Z'; archive_fn[2] = 'I';
	archive_fn[3] = 'N'; archive_fn[4] = 'N'; archive_fn[5] = '.';
	archive_fn[6] = 'D'; archive_fn[7] = 'A'; archive_fn[8] = 'T';
	archive_fn[9] = '\0';
	gaiji_fn[0] = 'P'; gaiji_fn[1] = 'A'; gaiji_fn[2] = 'U';
	gaiji_fn[3] = 'S'; gaiji_fn[4] = 'E'; gaiji_fn[5] = '.';
	gaiji_fn[6] = 'G'; gaiji_fn[7] = 'F'; gaiji_fn[8] = 'T';
	gaiji_fn[9] = '\0';

	pfend();
	pfstart(reinterpret_cast<const unsigned char far *>(archive_fn));
	if(file_ropen(gaiji_fn)) {
		loaded = reinterpret_cast<menu_font_t>(
			hmem_allocbyte(PAUSE_GAIJI_FILE_SIZE)
		);
		if(loaded &&
			(file_read(loaded, PAUSE_GAIJI_FILE_SIZE) == PAUSE_GAIJI_FILE_SIZE) &&
			(file_read(&extra, 1) == 0)) {
			valid = replay_pause_gaiji_validate(
				reinterpret_cast<const uint8_t far *>(loaded)
			);
		}
		file_close();
	}
	pfend();
	pfstart(aCOul);

	if(!valid) {
		if(loaded) {
			hmem_free(loaded);
		}
		return false;
	}
	for(i = 0; i < PAUSE_GAIJI_TILE_COUNT; i++) {
		gaiji = (
			(i < PAUSE_GAIJI_FIRST_COUNT) ?
			(PAUSE_GAIJI_FIRST_START + i) :
			(PAUSE_GAIJI_SECOND_START + (i - PAUSE_GAIJI_FIRST_COUNT))
		);
		gaiji_write(gaiji, &loaded[
			PAUSE_GAIJI_HEADER_SIZE + (i * PAUSE_GAIJI_TILE_SIZE)
		]);
	}
	hmem_free(loaded);
	return true;
}

#undef PAUSE_GAIJI_FILE_SIZE
#undef PAUSE_GAIJI_HEADER_SIZE
#undef PAUSE_GAIJI_TILE_SIZE
#undef PAUSE_GAIJI_TILE_COUNT
#undef PAUSE_GAIJI_FIRST_START
#undef PAUSE_GAIJI_FIRST_COUNT
#undef PAUSE_GAIJI_SECOND_START
#undef PAUSE_GAIJI_SECOND_COUNT

void far replay_session_start(void)
{
	uint8_t playback_stage;

	scorestat_process_enter();
	replay_preroll_startup_mask();
	language_main_apply();
	if(menu_font_load(aCOul) && !replay_pause_gaiji_load()) {
		menu_font_free();
	}
	replay_paths_init();
	replay_protect_local_reset();

	replay_mode = replay_resident_mode();
	if(replay_mode == REPLAY_DISABLED) {
		replay_mode = replay_cfg_mode();
	}
	replay_sample_count = 0;
	replay_global_frame = 0;
	t3pix_scene_set(T3PIX_SCENE_GAMEPLAY);
	t3pix_logical_identity_set(0, 0, round_frame);
	replay_input_byte_count = 0;
	replay_write_buffer_size = 0;
	replay_write_buffer_seg = 0;
	replay_last_route = 0;
	replay_rle_phase = T3_REPLAY_PACKET_PHASE_GAMEPLAY;
	replay_rle_run = 0;
	replay_rle_input_mp_p1 = 0;
	replay_rle_input_mp_p2 = 0;
	replay_rle_input_sp = 0;
	resident->unused_3[T3_RES_FAST_FORWARD_REPLAY_PHASE_INDEX] = 0;
	resident->unused_3[T3_REPLAY_RES_PAUSE_CANCEL_LATCH_INDEX] = false;
	replay_timing_flags_set(0);
	replay_timing_baseline_set(0);
	resident->unused_3[T3_REPLAY_RES_GUARD_FRESH_INDEX] = 0;
	resident->input_charge = 0;
	replay_done_written = false;
	replay_user_slot_fn_set(T3_REPLAY_USER_SLOT_NONE);
	replay_prompt_skip_queued = false;
	replay_rle_packet_state = 0;
	replay_user_discard_requested = false;
	replay_guard_diag_written = false;
	replay_restart_requested_flag = false;
	replay_accel_raw_seg = 0;
	replay_accel_target_checkpoint = 0;
#if defined(TH03_REPLAY_DEVTOOLS)
	replay_state_probe_pending = false;
	replay_state_probe_checkpoint = 0;
#endif

	if(replay_mode == REPLAY_DISABLED) {
		return;
	}
	if(
		(replay_mode == REPLAY_USER_RECORD) &&
		replay_protect_blocked()
	) {
		replay_guard_diag_write();
	}
	if(
		(replay_mode == REPLAY_USER_RECORD) ||
		(replay_mode == REPLAY_USER_PLAYBACK)
	) {
		replay_sample_count = replay_handoff_u32_read(
			T3_REPLAY_RES_SAMPLE_COUNT_INDEX
		);
		replay_global_frame = replay_handoff_u32_read(
			T3_REPLAY_RES_GLOBAL_FRAME_INDEX
		);
		replay_input_byte_count = replay_handoff_u32_read(
			T3_REPLAY_RES_INPUT_SIZE_INDEX
		);
	}
	if(replay_mode == REPLAY_USER_RECORD) {
		replay_write_buffer_seg = reinterpret_cast<uint16_t>(
			hmem_allocbyte(REPLAY_RECORD_BUFFER_SIZE)
		);
		if(replay_write_buffer_seg == 0) {
			replay_protect_detector_error_set();
		}
	}

	if(replay_mode != REPLAY_USER_RECORD) {
		file_create(T3_DONE_FN);
		file_close();
		replay_split_write_header();
	}

	if(replay_mode == REPLAY_RECORD) {
		replay_header_fill();
		if(file_create(T3_INPUT_FN)) {
			replay_write_bytes(&replay_header, sizeof(replay_header));
			file_close();
		} else {
			replay_mode = REPLAY_ERROR;
			replay_done_write(RTX_ERROR_INPUT_CREATE);
			return;
		}
	} else if(replay_mode == REPLAY_USER_RECORD) {
		if(replay_sample_count == 0) {
			replay_accel_temps_delete();
			if(!replay_user_create()) {
				replay_mode = REPLAY_ERROR;
				replay_done_write(RTX_ERROR_USER_CREATE);
				return;
			}
		} else if(!replay_user_read()) {
			replay_user_record_error_disable(
				RPD_MAIN_USER_READ, RTX_ERROR_USER_CREATE
			);
			return;
		} else {
			// replay_user_read() loaded checkpoint 0's compact reset seed.
			// Restore the seed captured by this new MAIN process instead.
			replay_header.reserved_2 = resident->rand;
			if(!replay_user_guard_checkpoint()) {
				replay_guard_diag_write();
			} else {
				resident->unused_3[
					T3_REPLAY_RES_GUARD_FRESH_INDEX
				] = 2;
			}
		}
	} else if(replay_mode == REPLAY_USER_PLAYBACK) {
		if(!replay_user_read()) {
			replay_preroll_startup_unmask();
			replay_mode = REPLAY_ERROR;
			replay_done_write(RTX_ERROR_USER_HEADER);
			return;
		}
		if(
			resident->unused_3[T3_REPLAY_RES_PREROLL_TARGET_INDEX] != 0
		) {
			(void)replay_accel_prepare(static_cast<uint8_t>(
				resident->unused_3[T3_REPLAY_RES_PREROLL_TARGET_INDEX] - 1
			));
		}
		if(
			(replay_user_header.flags & T3_REPLAY_USER_FLAG_PRACTICE) &&
			(players[0].cpu_safety_frames !=
			 replay_user_header.scenario.practice.config.initial_cpu_safety_frames)
		) {
			replay_preroll_startup_unmask();
			replay_mode = REPLAY_ERROR;
			replay_done_write(RTX_ERROR_USER_HEADER);
			return;
		}
		playback_stage = replay_handoff_u8(
			T3_REPLAY_RES_PLAYBACK_CHECKPOINT_INDEX
		);
		if(playback_stage != 0) {
			playback_stage--;
			if(
				!replay_user_decoder_seek(
					replay_sample_count, replay_input_byte_count
				) ||
				!replay_user_checkpoint_snapshot_read(playback_stage)
			) {
				replay_preroll_startup_unmask();
				replay_mode = REPLAY_ERROR;
				replay_done_write(RTX_ERROR_USER_HEADER);
				return;
			}
			replay_user_decoder_inputs_restore();
			replay_user_snapshot_restore_resident();
			if(!replay_user_snapshot_restore_runtime()) {
				replay_preroll_startup_unmask();
				replay_mode = REPLAY_ERROR;
				replay_done_write(RTX_ERROR_USER_HEADER);
				return;
			}
			if(replay_user_version_has_round_state(replay_user_header.version)) {
				replay_user_round_state_restore();
			}
			if(replay_user_version_has_round_carry(replay_user_header.version)) {
				replay_user_round_carry_restore();
			}
#if defined(TH03_REPLAY_DEVTOOLS)
			replay_state_probe_checkpoint = playback_stage;
			replay_state_probe_pending = true;
#endif
			resident->unused_3[T3_REPLAY_RES_PLAYBACK_CHECKPOINT_INDEX] = 0;
			if(
				(resident->unused_3[T3_REPLAY_RES_PREROLL_TARGET_INDEX] != 0) &&
				(
					(resident->unused_3[T3_REPLAY_RES_PREROLL_TARGET_INDEX] >
					 replay_user_summary_ext.checkpoint_count) ||
					(resident->unused_3[T3_REPLAY_RES_PREROLL_TARGET_INDEX] <=
					 (playback_stage + 1))
				)
			) {
				resident->unused_3[T3_REPLAY_RES_PREROLL_TARGET_INDEX] = 0;
			}
			if(
				resident->unused_3[T3_REPLAY_RES_PREROLL_TARGET_INDEX] != 0
			) {
				replay_preroll_display_hide();
			} else {
				replay_preroll_hardware_show();
				replay_preroll_audio_mask(false);
			}
		} else if(replay_sample_count == 0) {
			replay_user_snapshot_restore_resident();
			if(!replay_user_snapshot_restore_runtime()) {
				replay_mode = REPLAY_ERROR;
				replay_done_write(RTX_ERROR_USER_HEADER);
				return;
			}
			if(replay_user_version_has_round_state(replay_user_header.version)) {
				replay_user_round_state_restore();
			}
			if(replay_user_version_has_round_carry(replay_user_header.version)) {
				replay_user_round_carry_restore();
			}
		}
	} else if(!replay_header_read()) {
		replay_mode = REPLAY_ERROR;
		replay_done_write(RTX_ERROR_INPUT_HEADER);
		return;
	}

	replay_split_row(RSE_START, 0);
}

void far replay_round_start(void)
{
	replay_round_real_frames = 0;
	replay_round_vsync_last = vsync_Count2;
	if(replay_mode == REPLAY_USER_RECORD) {
		replay_timing_baseline_set(vsync_Count1);
		replay_timing_flags_set(T3_REPLAY_TIMING_BASELINE_PENDING);
	} else {
		replay_timing_flags_set(0);
	}
	if(
		(replay_mode == REPLAY_USER_PLAYBACK) &&
		(replay_accel_raw_seg != 0)
	) {
		if(!replay_accel_direct_start()) {
			hmem_free(reinterpret_cast<void __seg *>(replay_accel_raw_seg));
			replay_accel_raw_seg = 0;
			replay_accel_target_checkpoint = 0;
			resident->unused_3[T3_REPLAY_RES_PREROLL_TARGET_INDEX] = 0;
			resident->unused_3[T3R_RES_PREROLL_FORCE_INDEX] = 0;
			replay_preroll_display_show();
			replay_mode = REPLAY_ERROR;
			replay_done_write(RTX_ERROR_USER_HEADER);
			return;
		}
	}
	if(
		(replay_mode == REPLAY_USER_PLAYBACK) &&
		(resident->unused_3[T3_REPLAY_RES_PREROLL_TARGET_INDEX] != 0) &&
		(
			replay_user_summary_ext.checkpoint_stage_round[
				resident->unused_3[T3_REPLAY_RES_PREROLL_TARGET_INDEX] - 1
			] == replay_user_summary_stage_round_pack()
		)
	) {
		resident->unused_3[T3_REPLAY_RES_PREROLL_TARGET_INDEX] = 0;
		resident->unused_3[T3R_RES_PREROLL_FORCE_INDEX] = 0;
		resident->unused_3[T3_RES_FAST_FORWARD_REPLAY_PHASE_INDEX] = 0;
		replay_preroll_display_show();
	}
	if(replay_mode == REPLAY_USER_RECORD) {
		if(
			(replay_rle_phase & REPLAY_RLE_STAGE_CHECKPOINT_PENDING_MASK) &&
			!replay_user_header_write(RUS_RECORDING, RUER_PARTIAL)
		) {
			replay_guard_diag_write();
		}
		if(
			!replay_protect_invalid() &&
			(replay_user_summary_ext.checkpoint_count <
			 replay_user_checkpoint_capacity(
				replay_user_header.game_mode, replay_user_header.flags
			 ))
		) {
			replay_user_snapshot_fill();
			replay_user_compact_pack();
			replay_user_round_state_fill();
			replay_user_round_carry_fill();
			replay_user_checkpoint_cursor_capture();
			replay_user_summary_ext.checkpoint_stage_round[
				replay_user_summary_ext.checkpoint_count
			] = replay_user_summary_stage_round_pack();
			replay_user_summary_ext.checkpoint_count++;
			replay_rle_phase |= REPLAY_RLE_STAGE_CHECKPOINT_PENDING_MASK;
			if(
				(replay_user_summary_ext.checkpoint_stage_round[
					replay_user_summary_ext.checkpoint_count - 1
				 ] >> 4) != 0
			) {
				(void)replay_accel_capture(static_cast<uint8_t>(
					replay_user_summary_ext.checkpoint_count - 1
				));
			}
#if defined(TH03_REPLAY_DEVTOOLS)
			replay_state_probe_record_write(
				T3_STATE_REFERENCE_FN,
				static_cast<uint8_t>(
					replay_user_summary_ext.checkpoint_count - 1
				),
				REPLAY_STATE_PROBE_KIND_REFERENCE
			);
#endif
		}
	}
#if defined(TH03_REPLAY_DEVTOOLS)
	if(
		(replay_mode == REPLAY_USER_PLAYBACK) &&
		replay_state_probe_pending
	) {
		replay_state_probe_pending = false;
		replay_state_probe_record_write(
			T3_STATE_LIVE_FN,
			replay_state_probe_checkpoint,
			REPLAY_STATE_PROBE_KIND_LIVE
		);
	}
#endif
	replay_split_row(RSE_ROUND_START, replay_last_route);
}

static bool replay_fast_forward_key_held(void);

void far replay_frame_io(void)
{
	t3pix_scene_set(T3PIX_SCENE_GAMEPLAY);
	t3pix_logical_identity_set(
		replay_sample_count, replay_global_frame, round_frame
	);
	bool ok = true;
	bool fast_forward_held = false;
	uint8_t shot_bits;

	replay_timing_frame_begin();
	if(
		(replay_mode == REPLAY_USER_RECORD) &&
		(defeat_flag == DF_NONE)
	) {
		replay_round_real_frame_tick();
	}
	scorestat_process_sync();
	keyconfig_charge_mask_human();

	if(replay_mode == REPLAY_DISABLED) {
		replay_autofire_apply();
		return;
	}
	if(replay_mode == REPLAY_ERROR) {
		replay_user_playback_error_finish();
		return;
	}

	if(replay_mode == REPLAY_RECORD) {
		replay_autofire_apply();
		ok = replay_record_sample();
	} else if(replay_mode == REPLAY_USER_RECORD) {
		shot_bits = (
			((input_mp_p1 & INPUT_SHOT) ? 0x01 : 0x00) |
			((input_mp_p2 & INPUT_SHOT) ? 0x02 : 0x00)
		);
		replay_autofire_apply();
		ok = replay_user_record_logical_sample(
			T3_REPLAY_PACKET_PHASE_GAMEPLAY, shot_bits
		);
	} else if(replay_mode == REPLAY_USER_PLAYBACK) {
		fast_forward_held = replay_fast_forward_key_held();
		if(replay_user_playback_cancel()) {
			replay_fast_forward_wait_skip(false);
			return;
		}
		if(replay_sample_count >= replay_user_header.sample_count) {
#if defined(TH03_REPLAY_DEVTOOLS)
			replay_debug_transition_write(
				0xE4, T3_REPLAY_PACKET_PHASE_GAMEPLAY
			);
#endif
			replay_split_row(RSE_INPUT_END, replay_last_route);
			input_sp |= INPUT_CANCEL;
			replay_done_write(RTX_OK_USER_INPUT_END);
			replay_resident_handoff_clear();
			resident->game_mode = GM_NONE;
			replay_mode = REPLAY_DISABLED;
			replay_fast_forward_wait_skip(false);
			return;
		}
		ok = replay_user_play_logical_sample(T3_REPLAY_PACKET_PHASE_GAMEPLAY);
	} else if(replay_mode == REPLAY_PLAYBACK) {
		fast_forward_held = replay_fast_forward_key_held();
		if(replay_sample_count >= replay_header.sample_count) {
			replay_split_row(RSE_INPUT_END, replay_last_route);
			input_sp |= INPUT_CANCEL;
			replay_done_write(RTX_OK_INPUT_END);
			replay_mode = REPLAY_DISABLED;
			replay_fast_forward_wait_skip(false);
			return;
		}
		ok = replay_play_sample();
	}

	if(!ok) {
		if(replay_mode == REPLAY_USER_RECORD) {
			replay_user_record_error_disable(
				RPD_MAIN_RECORD_IO, RTX_ERROR_FRAME_IO
			);
			return;
		}
		replay_fast_forward_wait_skip(false);
		if(replay_mode == REPLAY_USER_PLAYBACK) {
			replay_user_playback_error_finish();
			return;
		}
		replay_split_row(RSE_ERROR, replay_last_route);
		replay_mode = REPLAY_ERROR;
		input_sp |= INPUT_CANCEL;
		replay_done_write(RTX_ERROR_FRAME_IO);
		return;
	}

	replay_fast_forward_wait_skip(fast_forward_held);

	if(
		(replay_global_frame & (T3_REPLAY_DISK_INTERVAL_SAMPLES - 1)) ==
		(T3_REPLAY_DISK_INTERVAL_SAMPLES - 1)
	) {
		replay_split_row(RSE_CHECKPOINT, replay_last_route);
		if(replay_mode == REPLAY_RECORD) {
			replay_header_write();
		} else if(replay_mode == REPLAY_USER_RECORD) {
			replay_user_periodic_flush();
			replay_handoff_cursor_store();
		}
	}
	replay_global_frame++;
	replay_handoff_cursor_store();
	if(
		(replay_mode == REPLAY_USER_RECORD) &&
		(resident->unused_3[T3_REPLAY_RES_GUARD_FRESH_INDEX] != 0)
	) {
		resident->unused_3[T3_REPLAY_RES_GUARD_FRESH_INDEX]--;
	}
}

void far replay_input_sense_held(void)
{
	bool ok = true;

	if(replay_mode == REPLAY_USER_PLAYBACK) {
		input_reset_sense_key_held();
		if(replay_user_playback_cancel()) {
			return;
		}
		ok = replay_user_play_logical_sample(
			T3_REPLAY_PACKET_PHASE_INTERSTITIAL
		);
	} else {
		input_reset_sense_key_held();
		keyconfig_charge_mask_human();
		if(replay_mode == REPLAY_USER_RECORD) {
			ok = replay_user_record_logical_sample(
				T3_REPLAY_PACKET_PHASE_INTERSTITIAL, 0
			);
		}
	}

	if(!ok) {
		if(replay_mode == REPLAY_USER_RECORD) {
			replay_user_record_error_disable(
				RPD_MAIN_RECORD_IO, RTX_ERROR_FRAME_IO
			);
			return;
		}
		if(replay_mode == REPLAY_USER_PLAYBACK) {
			replay_user_playback_error_finish();
			return;
		}
		replay_split_row(RSE_ERROR, replay_last_route);
		replay_mode = REPLAY_ERROR;
		input_sp |= INPUT_CANCEL;
		replay_done_write(RTX_ERROR_FRAME_IO);
		return;
	}
	if(
		(replay_mode == REPLAY_USER_RECORD) ||
		(replay_mode == REPLAY_USER_PLAYBACK)
	) {
		replay_user_sample_commit();
	}
}

#define REPLAY_PAUSE_LEFT 24
#define REPLAY_PAUSE_TOP 8
#define REPLAY_PAUSE_W 32
#define REPLAY_PAUSE_H 7
#define REPLAY_PAUSE_PIXEL_LEFT (REPLAY_PAUSE_LEFT * GLYPH_HALF_W)
#define REPLAY_PAUSE_PIXEL_TOP (REPLAY_PAUSE_TOP * GLYPH_HALF_H)
#define REPLAY_PAUSE_PIXEL_RIGHT ( \
	REPLAY_PAUSE_PIXEL_LEFT + (REPLAY_PAUSE_W * GLYPH_HALF_W) - 1 \
)
#define REPLAY_PAUSE_PIXEL_BOTTOM ( \
	REPLAY_PAUSE_PIXEL_TOP + (REPLAY_PAUSE_H * GLYPH_HALF_H) - 1 \
)
#define REPLAY_PAUSE_TEXT_LEFT (REPLAY_PAUSE_LEFT + 5)
#define REPLAY_PAUSE_CHOICE_MARK_LEFT (REPLAY_PAUSE_LEFT + 2)
#define REPLAY_PAUSE_BG_ATRB TX_BLACK
#define REPLAY_PAUSE_TITLE_ATRB TX_CYAN
#define REPLAY_PAUSE_CHOICE_ATRB TX_WHITE
#define REPLAY_PAUSE_SELECTED_ATRB TX_YELLOW
#define REPLAY_PAUSE_DISABLED_ATRB TX_BLUE
#define REPLAY_PAUSE_CHOICE_COLOR_W ( \
	(REPLAY_PAUSE_TEXT_LEFT + 20) - REPLAY_PAUSE_CHOICE_MARK_LEFT \
)
#define REPLAY_PAUSE_GAIJI_TEXT_LEFT (REPLAY_PAUSE_LEFT + 4)
#define REPLAY_PAUSE_GAIJI_CHOICE_COLOR_W ( \
	(REPLAY_PAUSE_LEFT + REPLAY_PAUSE_W - 1) - \
	REPLAY_PAUSE_CHOICE_MARK_LEFT \
)

enum replay_pause_vram_color_t {
	REPLAY_PAUSE_VRAM_BLUE = 9,
	REPLAY_PAUSE_VRAM_CYAN = 10,
};

enum replay_pause_gaiji_t {
	REPLAY_PAUSE_GAIJI_TITLE = 0,
	REPLAY_PAUSE_GAIJI_RESUME = 5,
	REPLAY_PAUSE_GAIJI_RESTART = 10,
	REPLAY_PAUSE_GAIJI_SAVE_EXIT = 15,
	REPLAY_PAUSE_GAIJI_DISCARD_EXIT = 28,
	REPLAY_PAUSE_GAIJI_FIRST_COUNT = 32,
	REPLAY_PAUSE_GAIJI_CURSOR = 39,
};

static unsigned replay_pause_gaiji_id(unsigned tile)
{
	return (
		(tile < REPLAY_PAUSE_GAIJI_FIRST_COUNT) ?
		(0x30 + tile) :
		(0xD3 + (tile - REPLAY_PAUSE_GAIJI_FIRST_COUNT))
	);
}

static void replay_pause_gaiji_put(
	unsigned x, unsigned y, unsigned tile, unsigned count
)
{
	while(count != 0) {
		gaiji_putca(
			x, y, replay_pause_gaiji_id(tile), (TX_BLACK | TX_REVERSE)
		);
		x += GAIJI_TRAM_W;
		tile++;
		count--;
	}
}

static void replay_text_putca(unsigned x, unsigned y, int ch, unsigned atrb)
{
	char str[2];

	str[0] = static_cast<char>(ch);
	str[1] = '\0';
	(void)atrb;
	text_putsa(x, y, str, (TX_BLACK | TX_REVERSE));
}

static void replay_text_putca_raw(
	unsigned x, unsigned y, int ch, unsigned atrb
)
{
	char str[2];

	str[0] = static_cast<char>(ch);
	str[1] = '\0';
	text_putsa(x, y, str, atrb);
}

static unsigned replay_pause_clear_atrb(unsigned x)
{
	if(
		(
			(x >= playfield_tram_x(0, 0)) &&
			(x < playfield_tram_x(0, PLAYFIELD_W))
		) ||
		(
			(x >= playfield_tram_x(1, 0)) &&
			(x < playfield_tram_x(1, PLAYFIELD_W))
		)
	) {
		return TX_WHITE;
	}
	return (TX_BLACK | TX_REVERSE);
}

static void replay_pause_put_graph_backing(void)
{
	graph_copy_page(page_back);
	replay_overlay_graph_fill(
		REPLAY_PAUSE_PIXEL_LEFT, REPLAY_PAUSE_PIXEL_TOP,
		REPLAY_PAUSE_PIXEL_RIGHT, REPLAY_PAUSE_PIXEL_BOTTOM,
		V_WHITE, page_front
	);
}

static int replay_pause_vram_color(unsigned atrb)
{
	if(atrb == REPLAY_PAUSE_TITLE_ATRB) {
		return REPLAY_PAUSE_VRAM_CYAN;
	}
	if(atrb == REPLAY_PAUSE_SELECTED_ATRB) {
		return V_YELLOW_BRIGHT;
	}
	if(atrb == REPLAY_PAUSE_DISABLED_ATRB) {
		return REPLAY_PAUSE_VRAM_BLUE;
	}
	return V_WHITE;
}

static void replay_pause_put_color_backing(
	unsigned left, unsigned top, unsigned width, unsigned atrb
)
{
	replay_overlay_graph_fill(
		(left * GLYPH_HALF_W), (top * GLYPH_HALF_H),
		((left + width) * GLYPH_HALF_W) - 1,
		((top + 1) * GLYPH_HALF_H) - 1,
		replay_pause_vram_color(atrb), page_front
	);
}

static bool replay_fast_forward_key_held(void)
{
	return ((peekb(0, KEYGROUP_5) & K5_Z) != 0);
}

static void replay_pause_put_tram_backing(void)
{
	int x;
	int y;

	for(y = 0; y < REPLAY_PAUSE_H; y++) {
		for(x = 0; x < REPLAY_PAUSE_W; x++) {
			replay_text_putca(
				(REPLAY_PAUSE_LEFT + x), (REPLAY_PAUSE_TOP + y),
				' ', REPLAY_PAUSE_BG_ATRB
			);
		}
	}
}

static void replay_pause_put_title(void)
{
	unsigned x = (REPLAY_PAUSE_LEFT + 13);
	unsigned y = (REPLAY_PAUSE_TOP + 1);

	replay_pause_put_color_backing(x, y, 6, REPLAY_PAUSE_TITLE_ATRB);
#define P(c) replay_text_putca(x++, y, c, REPLAY_PAUSE_TITLE_ATRB)
	P('P'); P('A'); P('U'); P('S'); P('E'); P('D');
#undef P
}

static void replay_pause_put_resume(unsigned y, unsigned atrb)
{
	unsigned x = REPLAY_PAUSE_TEXT_LEFT;

#define P(c) replay_text_putca(x++, y, c, atrb)
	P('R'); P('e'); P('s'); P('u'); P('m'); P('e');
#undef P
}

static void replay_pause_put_restart(unsigned y, unsigned atrb)
{
	unsigned x = REPLAY_PAUSE_TEXT_LEFT;

#define P(c) replay_text_putca(x++, y, c, atrb)
	P('R'); P('e'); P('s'); P('t'); P('a'); P('r'); P('t');
#undef P
}

static void replay_pause_put_save_exit(unsigned y, unsigned atrb)
{
	unsigned x = REPLAY_PAUSE_TEXT_LEFT;

#define P(c) replay_text_putca(x++, y, c, atrb)
	P('S'); P('a'); P('v'); P('e'); P(' '); P('R'); P('e'); P('p');
	P('l'); P('a'); P('y'); P(' '); P('a'); P('n'); P('d'); P(' ');
	P('E'); P('x'); P('i'); P('t');
#undef P
}

static void replay_pause_put_discard_exit(unsigned y, unsigned atrb)
{
	unsigned x = REPLAY_PAUSE_TEXT_LEFT;

#define P(c) replay_text_putca(x++, y, c, atrb)
	P('E'); P('x'); P('i'); P('t'); P(' '); P('W'); P('i'); P('t');
	P('h'); P('o'); P('u'); P('t'); P(' '); P('S'); P('a'); P('v');
	P('i'); P('n'); P('g');
#undef P
}

static bool replay_pause_save_disabled(void)
{
	if(replay_mode == REPLAY_USER_PLAYBACK) {
		return false;
	}
	if(replay_mode != REPLAY_USER_RECORD) {
		return true;
	}
	return replay_protect_blocked();
}

static void replay_pause_save_refresh(void)
{
	snd_kaja_func(PMD_SONG_PAUSE, 0);
	if(replay_mode == REPLAY_USER_PLAYBACK) {
		return;
	}
	if(replay_mode != REPLAY_USER_RECORD) {
		return;
	}
	if(!replay_protect_blocked()) {
		if(
			resident->unused_3[T3_REPLAY_RES_GUARD_FRESH_INDEX] != 0
		) {
			resident->unused_3[T3_REPLAY_RES_GUARD_FRESH_INDEX] = 0;
			return;
		}
		if(!replay_user_periodic_flush()) {
			if(replay_handoff_u8(T3R_DIAG_CODE_INDEX) == RPD_NONE) {
				replay_protect_diag_code_set(RPD_MAIN_PAUSE_FLUSH);
			}
		}
	}
	if(replay_protect_blocked()) {
		replay_guard_diag_write();
	}
}

static uint8_t replay_pause_next_choice(uint8_t sel)
{
	sel++;
	if(sel > REPLAY_PAUSE_DISCARD_EXIT) {
		sel = REPLAY_PAUSE_RESUME;
	}
	if((sel == REPLAY_PAUSE_SAVE_EXIT) && replay_pause_save_disabled()) {
		sel = REPLAY_PAUSE_DISCARD_EXIT;
	}
	return sel;
}

static uint8_t replay_pause_prev_choice(uint8_t sel)
{
	if(sel == REPLAY_PAUSE_RESUME) {
		sel = REPLAY_PAUSE_DISCARD_EXIT;
	} else {
		sel--;
	}
	if((sel == REPLAY_PAUSE_SAVE_EXIT) && replay_pause_save_disabled()) {
		sel = REPLAY_PAUSE_RESTART;
	}
	return sel;
}

static uint8_t replay_pause_validate_choice(uint8_t sel)
{
	if((sel == REPLAY_PAUSE_SAVE_EXIT) && replay_pause_save_disabled()) {
		return REPLAY_PAUSE_DISCARD_EXIT;
	}
	return sel;
}

static void replay_pause_put_choices(uint8_t sel)
{
	unsigned y;
	unsigned atrb;

	y = (REPLAY_PAUSE_TOP + 2);
	atrb = (
		(sel == REPLAY_PAUSE_RESUME) ?
		REPLAY_PAUSE_SELECTED_ATRB :
		REPLAY_PAUSE_CHOICE_ATRB
	);
	replay_pause_put_color_backing(
		REPLAY_PAUSE_CHOICE_MARK_LEFT, y,
		REPLAY_PAUSE_CHOICE_COLOR_W, atrb
	);
	replay_text_putca(REPLAY_PAUSE_CHOICE_MARK_LEFT, y, (
		(sel == REPLAY_PAUSE_RESUME) ? '>' : ' '
	), atrb);
	replay_pause_put_resume(y, atrb);

	y++;
	atrb = (
		(sel == REPLAY_PAUSE_RESTART) ?
		REPLAY_PAUSE_SELECTED_ATRB :
		REPLAY_PAUSE_CHOICE_ATRB
	);
	replay_pause_put_color_backing(
		REPLAY_PAUSE_CHOICE_MARK_LEFT, y,
		REPLAY_PAUSE_CHOICE_COLOR_W, atrb
	);
	replay_text_putca(REPLAY_PAUSE_CHOICE_MARK_LEFT, y, (
		(sel == REPLAY_PAUSE_RESTART) ? '>' : ' '
	), atrb);
	replay_pause_put_restart(y, atrb);

	y++;
	if(replay_pause_save_disabled()) {
		atrb = REPLAY_PAUSE_DISABLED_ATRB;
	} else {
		atrb = (
			(sel == REPLAY_PAUSE_SAVE_EXIT) ?
			REPLAY_PAUSE_SELECTED_ATRB :
			REPLAY_PAUSE_CHOICE_ATRB
		);
	}
	replay_pause_put_color_backing(
		REPLAY_PAUSE_CHOICE_MARK_LEFT, y,
		REPLAY_PAUSE_CHOICE_COLOR_W, atrb
	);
	replay_text_putca(REPLAY_PAUSE_CHOICE_MARK_LEFT, y, (
		(sel == REPLAY_PAUSE_SAVE_EXIT) ? '>' : ' '
	), atrb);
	replay_pause_put_save_exit(y, atrb);

	y++;
	atrb = (
		(sel == REPLAY_PAUSE_DISCARD_EXIT) ?
		REPLAY_PAUSE_SELECTED_ATRB :
		REPLAY_PAUSE_CHOICE_ATRB
	);
	replay_pause_put_color_backing(
		REPLAY_PAUSE_CHOICE_MARK_LEFT, y,
		REPLAY_PAUSE_CHOICE_COLOR_W, atrb
	);
	replay_text_putca(REPLAY_PAUSE_CHOICE_MARK_LEFT, y, (
		(sel == REPLAY_PAUSE_DISCARD_EXIT) ? '>' : ' '
	), atrb);
	replay_pause_put_discard_exit(y, atrb);
}

static void replay_pause_clear(void)
{
	int x;
	int y;

	for(y = 0; y < REPLAY_PAUSE_H; y++) {
		for(x = 0; x < REPLAY_PAUSE_W; x++) {
			replay_text_putca_raw(
				(REPLAY_PAUSE_LEFT + x), (REPLAY_PAUSE_TOP + y),
				' ', replay_pause_clear_atrb(REPLAY_PAUSE_LEFT + x)
			);
		}
	}
}

static void replay_pause_restore_graphics(void)
{
	graph_copy_page(page_front);
	graph_accesspage(page_back);
	snd_kaja_func(PMD_SONG_RESUME, 0);
}

static void replay_pause_wait_release(void)
{
	goto release_test;

release_wait:
	replay_input_sense_held();
	replay_frame_delay();

release_test:
	if(
		(input_sp != INPUT_NONE) ||
		(input_mp_p1 != INPUT_NONE) ||
		(input_mp_p2 != INPUT_NONE)
	) {
		goto release_wait;
	}
}

static void replay_pause_beep(void)
{
	snd_se_reset();
	snd_se_play(21);
	snd_se_update();
}

static void replay_pause_font_put_title(void)
{
	unsigned x = (REPLAY_PAUSE_LEFT + 11);
	unsigned y = (REPLAY_PAUSE_TOP + 1);

	replay_pause_put_color_backing(x, y, 10, REPLAY_PAUSE_TITLE_ATRB);
	replay_pause_gaiji_put(
		x, y, REPLAY_PAUSE_GAIJI_TITLE, 5
	);
}

static void replay_pause_font_choice_put(uint8_t choice, uint8_t sel)
{
	unsigned tile;
	unsigned count;
	unsigned atrb;
	unsigned y = (REPLAY_PAUSE_TOP + choice + 2);

	if(
		(choice == REPLAY_PAUSE_SAVE_EXIT) &&
		replay_pause_save_disabled()
	) {
		atrb = REPLAY_PAUSE_DISABLED_ATRB;
	} else if(choice == sel) {
		atrb = REPLAY_PAUSE_SELECTED_ATRB;
	} else {
		atrb = REPLAY_PAUSE_CHOICE_ATRB;
	}

	replay_pause_put_color_backing(
		REPLAY_PAUSE_CHOICE_MARK_LEFT, y,
		REPLAY_PAUSE_GAIJI_CHOICE_COLOR_W, atrb
	);
	if(choice == REPLAY_PAUSE_RESUME) {
		tile = REPLAY_PAUSE_GAIJI_RESUME;
		count = 5;
	} else if(choice == REPLAY_PAUSE_RESTART) {
		tile = REPLAY_PAUSE_GAIJI_RESTART;
		count = 5;
	} else if(choice == REPLAY_PAUSE_SAVE_EXIT) {
		tile = REPLAY_PAUSE_GAIJI_SAVE_EXIT;
		count = 13;
	} else {
		tile = REPLAY_PAUSE_GAIJI_DISCARD_EXIT;
		count = 11;
	}
	if(choice == sel) {
		replay_pause_gaiji_put(
			REPLAY_PAUSE_CHOICE_MARK_LEFT, y,
			REPLAY_PAUSE_GAIJI_CURSOR, 1
		);
	} else {
		replay_text_putca(REPLAY_PAUSE_CHOICE_MARK_LEFT, y, ' ', atrb);
		replay_text_putca((REPLAY_PAUSE_CHOICE_MARK_LEFT + 1), y, ' ', atrb);
	}
	replay_pause_gaiji_put(
		REPLAY_PAUSE_GAIJI_TEXT_LEFT, y, tile, count
	);
}

static void replay_pause_font_put_choices(uint8_t sel)
{
	uint8_t choice;

	for(
		choice = REPLAY_PAUSE_RESUME;
		choice <= REPLAY_PAUSE_DISCARD_EXIT;
		choice++
	) {
		replay_pause_font_choice_put(choice, sel);
	}
}

static void replay_pause_choices_redraw(uint8_t old_sel, uint8_t sel)
{
	(void)old_sel;
	if(menu_font) {
		replay_pause_font_put_choices(sel);
	} else {
		replay_pause_put_choices(sel);
	}
}

#if defined(TH03_REPLAY_DEVTOOLS)
#pragma codestring "\x90\x90"
#endif

// Keep the pause and following replay functions at their accepted offsets.
uint8_t far replay_pause_menu(void)
{
	t3pix_scene_set(T3PIX_SCENE_PAUSE);
	uint8_t sel = REPLAY_PAUSE_RESUME;
	uint8_t old_sel;

#if defined(TH03_REPLAY_DEVTOOLS)
	if(
		(replay_user_slot < T3_REPLAY_USER_SLOT_COUNT) &&
		(replay_mode != REPLAY_USER_PLAYBACK)
	) {
		replay_debug_transition_write(0xE5, 0xFF);
	}
#endif
	replay_pause_save_refresh();
	replay_pause_beep();
	replay_pause_wait_release();
	if(replay_prompt_skip()) {
		return REPLAY_PAUSE_DISCARD_EXIT;
	}
	// Gameplay uses doubled 200-line graphics, while TRAM remains 400-line and
	// owns the center HUD. Keep this menu on TRAM so it retains native height,
	// true black backing, and priority over that HUD.
	replay_pause_put_graph_backing();
	replay_pause_put_tram_backing();
	sel = replay_pause_validate_choice(sel);
	if(menu_font) {
		replay_pause_font_put_title();
		replay_pause_font_put_choices(sel);
	} else {
		replay_pause_put_title();
		replay_pause_put_choices(sel);
	}
	t3pix_publish(T3PIX_EVENT_SOURCE_MUTATION, T3PIX_BOUNDARY_STATE);

input_wait:
	replay_input_sense_held();
	if(replay_prompt_skip()) {
		return REPLAY_PAUSE_DISCARD_EXIT;
	}
	if(input_sp & INPUT_Q) {
		return REPLAY_PAUSE_DISCARD_EXIT;
	}
	asm {
		call far ptr keyconfig_restart_request_poll
		jnc restart_not_requested
	}
	return REPLAY_PAUSE_RESTART;
restart_not_requested:
	if(input_sp & INPUT_CANCEL) {
		goto resume;
	}
	if(input_sp & INPUT_UP) {
		replay_pause_save_refresh();
		old_sel = sel;
		sel = replay_pause_validate_choice(sel);
		sel = replay_pause_prev_choice(sel);
		replay_pause_choices_redraw(old_sel, sel);
		replay_pause_wait_release();
		goto input_wait;
	}
	if(input_sp & INPUT_DOWN) {
		replay_pause_save_refresh();
		old_sel = sel;
		sel = replay_pause_validate_choice(sel);
		sel = replay_pause_next_choice(sel);
		replay_pause_choices_redraw(old_sel, sel);
		replay_pause_wait_release();
		goto input_wait;
	}
	if(input_sp & (INPUT_OK | INPUT_SHOT)) {
		if(sel == REPLAY_PAUSE_SAVE_EXIT) {
			replay_pause_save_refresh();
			if(replay_pause_save_disabled()) {
				old_sel = sel;
				sel = REPLAY_PAUSE_DISCARD_EXIT;
				replay_pause_choices_redraw(old_sel, sel);
				replay_pause_beep();
				replay_pause_wait_release();
				goto input_wait;
			}
		}
		if((sel == REPLAY_PAUSE_SAVE_EXIT) && replay_pause_save_disabled()) {
			old_sel = sel;
			sel = REPLAY_PAUSE_DISCARD_EXIT;
			replay_pause_choices_redraw(old_sel, sel);
			replay_pause_beep();
			replay_pause_wait_release();
			goto input_wait;
		}
		if(sel == REPLAY_PAUSE_RESUME) {
			goto resume;
		}
		return sel;
	}
	replay_frame_delay();
	goto input_wait;

resume:
	replay_pause_wait_release();
	if(replay_prompt_skip()) {
		return REPLAY_PAUSE_DISCARD_EXIT;
	}
	// The frame-local timing marker now owns slowdown exclusion. Once Pause has
	// consumed the release, this debounce latch must not suppress the resumed
	// frame's VSync wait or the next eligible Esc press.
	resident->unused_3[T3_REPLAY_RES_PAUSE_CANCEL_LATCH_INDEX] = false;
	replay_pause_restore_graphics();
	replay_pause_clear();
	replay_pause_beep();
	return REPLAY_PAUSE_RESUME;
}

#undef REPLAY_PAUSE_LEFT
#undef REPLAY_PAUSE_TOP
#undef REPLAY_PAUSE_W
#undef REPLAY_PAUSE_H
#undef REPLAY_PAUSE_PIXEL_LEFT
#undef REPLAY_PAUSE_PIXEL_TOP
#undef REPLAY_PAUSE_PIXEL_RIGHT
#undef REPLAY_PAUSE_PIXEL_BOTTOM
#undef REPLAY_PAUSE_TEXT_LEFT
#undef REPLAY_PAUSE_CHOICE_MARK_LEFT
#undef REPLAY_PAUSE_BG_ATRB
#undef REPLAY_PAUSE_TITLE_ATRB
#undef REPLAY_PAUSE_CHOICE_ATRB
#undef REPLAY_PAUSE_SELECTED_ATRB
#undef REPLAY_PAUSE_DISABLED_ATRB
#undef REPLAY_PAUSE_CHOICE_COLOR_W
#undef REPLAY_PAUSE_GAIJI_TEXT_LEFT
#undef REPLAY_PAUSE_GAIJI_CHOICE_COLOR_W

bool far replay_prompt_skip(void)
{
	return replay_prompt_skip_queued;
}

void far replay_user_record_discard_on_exit(void)
{
	if(replay_mode == REPLAY_USER_RECORD) {
		replay_user_discard_requested = true;
	}
}

void far replay_restart_request(void)
{
	replay_restart_requested_flag = true;
	replay_user_record_discard_on_exit();
}

void far replay_route(uint8_t route)
{
	replay_last_route = route;
	replay_user_round_split_capture(route);
	replay_user_summary_capture(route);
	replay_split_row(RSE_ROUTE, route);
}

void far replay_finish(uint8_t route)
{
	bool finish_error = false;
	bool save_pending = false;
	bool control_ok = true;

	if(replay_accel_raw_seg != 0) {
		hmem_free(reinterpret_cast<void __seg *>(replay_accel_raw_seg));
		replay_accel_raw_seg = 0;
		replay_accel_target_checkpoint = 0;
	}

	scorestat_exit_checkpoint();
	if(route != 0) {
		replay_clear_bonus_capture();
	}
	replay_split_row(RSE_FINISH, route);
	if(replay_mode == REPLAY_USER_RECORD) {
		control_ok = replay_user_control_write(
			T3_REPLAY_PACKET_CONTROL_MAIN_END
		);
	} else if(
		(replay_mode == REPLAY_USER_PLAYBACK) &&
		!replay_prompt_skip_queued
	) {
		control_ok = replay_user_control_consume(
			T3_REPLAY_PACKET_CONTROL_MAIN_END
		);
	}
	if(!control_ok) {
		replay_split_row(RSE_ERROR, route);
		if(replay_mode == REPLAY_USER_PLAYBACK) {
			resident->game_mode = GM_NONE;
		}
		replay_done_write(RTX_ERROR_FRAME_IO);
		replay_protect_local_free();
		replay_resident_handoff_clear();
		replay_mode = REPLAY_DISABLED;
		return;
	}
	if(
		(route != 0) &&
		(
			(replay_mode == REPLAY_USER_RECORD) ||
			(replay_mode == REPLAY_USER_PLAYBACK)
		)
	) {
		if(replay_mode == REPLAY_USER_RECORD) {
			if(!replay_user_header_write(RUS_RECORDING, RUER_PARTIAL)) {
				replay_guard_diag_write();
			}
		} else {
			resident->unused_3[T3_REPLAY_RES_MODE_INDEX] =
				((resident->pid_winner == 0)
					? T3_REPLAY_RES_MODE_USER_PLAYBACK
					: T3R_RES_MODE_USER_GAME_OVER);
		}
		replay_handoff_cursor_store();
		replay_protect_local_free();
		replay_mode = REPLAY_DISABLED;
		return;
	}

	if(replay_mode == REPLAY_RECORD) {
		replay_header_write();
	} else if(replay_mode == REPLAY_USER_RECORD) {
		if(
			(route == 0) &&
			(replay_user_discard_requested || replay_protect_invalid())
		) {
			if(replay_protect_invalid()) {
				replay_guard_diag_write();
			}
			replay_protect_file_delete_commit(replay_user_fn);
			replay_user_index_slot_clear();
			replay_user_guard_delete();
			replay_accel_temps_delete();
		} else {
			if(!replay_user_header_write(
				((route == 0) ? RUS_FINALIZED : RUS_PARTIAL),
				((route == 0) ? RUER_MENU_RETURN : RUER_PARTIAL)
			)) {
				finish_error = true;
				replay_guard_diag_write();
			} else if(route == 0) {
				replay_user_guard_delete();
				save_pending = true;
			}
		}
		replay_protect_local_free();
		replay_resident_handoff_clear();
	} else if(replay_mode == REPLAY_USER_PLAYBACK) {
		resident->game_mode = GM_NONE;
		replay_protect_local_free();
		replay_resident_handoff_clear();
	}
	if(replay_mode != REPLAY_DISABLED) {
		if(replay_mode == REPLAY_USER_RECORD) {
			if(finish_error) {
				replay_done_write(RTX_ERROR_USER_HEADER);
			} else if(
				(route == 0) &&
				(replay_user_discard_requested || replay_protect_invalid())
			) {
				replay_done_write(RTX_OK_MENU_RETURN_NOSAVE);
			} else {
				replay_done_write(
					(route == 0) ? RTX_OK_MENU_RETURN : RTX_OK_PARTIAL
				);
			}
		} else if(replay_mode == REPLAY_USER_PLAYBACK) {
			replay_done_write(RTX_OK_USER_PLAYBACK);
		} else {
			replay_done_write(RTX_OK);
		}
	}
	if(replay_restart_requested_flag && (route == 0)) {
		resident->rand = random_seed;
		replay_resident_handoff_mode_set(T3_REPLAY_RES_MODE_RESTART);
	} else if(save_pending) {
		replay_resident_handoff_mode_set(T3_REPLAY_RES_MODE_SAVE_DIRECT);
	}
	replay_mode = REPLAY_DISABLED;
}

// These loops live after all offset-sensitive replay entry points. They copy
// state that ZUN's native retry path leaves live between rounds.
static void replay_user_carry_chains_fill(void)
{
	int i;
	int slot;

	replay_user_round_carry.fireball_generation_prev = generation_prev;
	for(i = 0; i < PLAYER_COUNT; i++) {
		replay_user_round_carry.combo_time[i] = combos[i].time;
		replay_user_round_carry.combo_hits_highest[i] = (
			combos[i].hits_highest
		);
		replay_user_round_carry.combo_bonus_total[i] = combos[i].bonus_total;
		replay_user_round_carry.chain_ring_p[i] = chain_ring_p[i];
		for(slot = 0; slot < CHAIN_RING_SIZE; slot++) {
			replay_user_round_carry.chain_hits[i][slot] = (
				chains.hits[i][slot]
			);
			replay_user_round_carry.chain_pellet_and_fireball_value[i][slot] = (
				chains.pellet_and_fireball_value[i][slot]
			);
			replay_user_round_carry.chain_charge_fireball[i][slot] = (
				chains.charge_fireball[i][slot]
			);
			replay_user_round_carry.chain_charge_exatt[i][slot] = (
				chains.charge_exatt[i][slot]
			);
		}
	}
}

static void replay_user_carry_chains_restore(void)
{
	int i;
	int slot;

	generation_prev = replay_user_round_carry.fireball_generation_prev;
	for(i = 0; i < PLAYER_COUNT; i++) {
		combos[i].time = replay_user_round_carry.combo_time[i];
		combos[i].hits_highest = (
			replay_user_round_carry.combo_hits_highest[i]
		);
		combos[i].bonus_total = replay_user_round_carry.combo_bonus_total[i];
		chain_ring_p[i] = replay_user_round_carry.chain_ring_p[i];
		for(slot = 0; slot < CHAIN_RING_SIZE; slot++) {
			chains.hits[i][slot] = (
				replay_user_round_carry.chain_hits[i][slot]
			);
			chains.pellet_and_fireball_value[i][slot] = (
				replay_user_round_carry.chain_pellet_and_fireball_value[i][slot]
			);
			chains.charge_fireball[i][slot] = (
				replay_user_round_carry.chain_charge_fireball[i][slot]
			);
			chains.charge_exatt[i][slot] = (
				replay_user_round_carry.chain_charge_exatt[i][slot]
			);
		}
	}
}

#if defined(TH03_REPLAY_DEVTOOLS)
static void replay_debug_transition_write(
	uint8_t code, uint8_t requested_phase
)
{
	uint16_t input;

	if(replay_guard_diag_written) {
		return;
	}
	replay_handoff_cursor_store();
	resident->unused_3[T3R_DIAG_CODE_INDEX] = code;
	resident->unused_3[T3R_DIAG_DRIVE_INDEX] = replay_mode;
	resident->unused_3[T3R_DIAG_DOS_AX_INDEX + 0] = requested_phase;
	resident->unused_3[T3R_DIAG_DOS_AX_INDEX + 1] = replay_rle_phase;
	input = input_sp;
	resident->unused_3[T3R_DIAG_BYTES_SECTOR_INDEX + 0] = input;
	resident->unused_3[T3R_DIAG_BYTES_SECTOR_INDEX + 1] = (input >> 8);
	input = replay_rle_input_sp;
	resident->unused_3[T3R_DIAG_ROOT_ENTRIES_INDEX + 0] = input;
	resident->unused_3[T3R_DIAG_ROOT_ENTRIES_INDEX + 1] = (input >> 8);
	resident->unused_3[T3R_DIAG_ROOT_SECTORS_INDEX + 0] = replay_rle_run;
	resident->unused_3[T3R_DIAG_ROOT_SECTORS_INDEX + 1] = (
		replay_rle_packet_state
	);
	resident->unused_3[T3R_DIAG_OFFSET_INDEX + 0] = byte_23B00;
	resident->unused_3[T3R_DIAG_OFFSET_INDEX + 1] = (
		resident->unused_3[T3_REPLAY_RES_PAUSE_CANCEL_LATCH_INDEX]
	);
	resident->unused_3[T3R_DIAG_INT25_FLAGS_INDEX + 0] = (
		replay_prompt_skip_queued
	);
	replay_guard_diag_write();
}
#endif

// Keep the following C runtime segment at its accepted paragraph phase.
#if !defined(TH03_REPLAY_DEVTOOLS)
#if defined(TH03_REPLAY_DEV_OVERLAY)
#pragma codestring "\x90\x90\x90\x90"
#else
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#endif
#endif
#pragma codestring "\x90\x90\x90\x90"
#if defined(TH03_REPLAY_DEV_OVERLAY)
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#else
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#endif
