/* ReC98
 * -----
 * Code segment #1 of TH03's OP.EXE
 */

#pragma option -zPgroup_01

#include "libs/master.lib/pc98_gfx.hpp"
#include "th01/rank.h"
#include "th01/math/clamp.hpp"
#include "th01/core/initexit.hpp"
#include "th01/hardware/grppsafx.h"
#include "th02/v_colors.hpp"
#include "th02/hardware/frmdelay.h"
#include "th02/formats/pi.h"
#include "th02/gaiji/str.hpp"
#include "th02/op/menu.hpp"
#include "th02/op/m_music.hpp"
#include "th03/common.h"
#include "th03/resident.hpp"
#include "th03/hardware/input.h"
#include "th03/keyconfig.hpp"
#include "th03/language.hpp"
#include "th03/lngop.hpp"
#include "th03/photosensitivity.hpp"
#include "th03/pixel_capture.hpp"
#include "th03/menu_font.hpp"
#include "th03/formats/cfg_impl.hpp"
#include "th03/formats/cdg.h"
#include "th03/core/initexit.h"
#include "th03/gaiji/gaiji.h"
#include "th03/op_patch.hpp"
#include "th03/replay_build.hpp"
#include "th03/replay_format.hpp"
#include "th03/replay_handoff.hpp"
#include "th03/snd/snd.h"
#include "th03/shiftjis/fns.hpp"
#include "th03/shiftjis/main.hpp"
#include "th03/op/m_main.hpp"
#include "th03/op/m_select.hpp"
#include "th03/opfont.hpp"
#include "th03/practice.hpp"
#include "th03/rpyfont.hpp"
#include "th03/sprites/regi.h"
#include "th03/snd/options.hpp"
#include "platform/x86real/pc98/keyboard.hpp"
#include "platform/x86real/flags.hpp"
#include "planar.h"
#include <conio.h>
#include <mem.h>
#include <stddef.h>
#include <process.h>

enum main_choice_t {
	MC_STORY,
	MC_VS,
	MC_MUSICROOM,
	MC_REGIST_VIEW,
	MC_OPTION,
	MC_REPLAY,
	MC_QUIT,
	MC_COUNT,
};

enum option_choice_t {
	OC_RANK,
	OC_BGM,
	OC_SFX,
	OC_LANGUAGE,
	OC_KEY_MODE,
	OC_PHOTOSENSITIVITY,
	OC_QUIT,
	OC_COUNT,
};

// Proportional gaiji strings
// --------------------------

enum gaiji_th03_mikoft_t {
	gp_Start = 0x30,
	gp_Start_last = ((gp_Start + 3) - 1),
	gp_VS_Start,
	gp_VS_Start_last = ((gp_VS_Start + 6) - 1),
	gp_Replay,
	gp_Replay_last = ((gp_Replay + 4) - 1),
	gp_Replay_y_right = 0x7F,
	gp_on_clean_left = 0x87,
	gp_Option = 0x3D,
	gp_Option_last = ((gp_Option + 4) - 1),
	gp_on = 0x3F,
	gp_on_last = ((gp_on + 2) - 1),
	gp_Music_room,
	gp_Music_room_last = ((gp_Music_room + 7) - 1),
	gp_Quit,
	gp_Quit_last = ((gp_Quit + 3) - 1),
	gp_Music,
	gp_Music_last = ((gp_Music + 4) - 1),
	gp_FM_86_,
	gp_FM_86__last = ((gp_FM_86_ + 4) - 1),
	gp_Autofire,
	gp_Autofire_last = ((gp_Autofire + 7) - 1),
	gp_off,
	gp_off_last = ((gp_off + 2) - 1),
	gp_KeyConfig,
	gp_KeyConfig_last = ((gp_KeyConfig + 6) - 1),
	gp_Type,
	gp_Type_last = ((gp_Type + 3) - 1),
	gp_1,
	gp_2,
	gp_3,
	gp_Key,
	gp_Key_last = ((gp_Key + 2) - 1),
	gp_Joy,
	gp_Joy_last = ((gp_Joy + 2) - 1),
	gp_vs,
	gp_vs_last = ((gp_vs + 2) - 1),
	gp_Rank,
	gp_Rank_last = ((gp_Rank + 3) - 1),
	gp_Easy,
	gp_Easy_last = ((gp_Easy + 3) - 1),
	gp_Normal,
	gp_Normal_last = ((gp_Normal + 4) - 1),
	gp_Hard,
	gp_Hard_last = ((gp_Hard + 3) - 1),
	gp_Lunatic,
	gp_Lunatic_last = ((gp_Lunatic + 4) - 1),

	gp_HiScore = 0x82,
	gp_HiScore_last = ((gp_HiScore + 5) - 1),
	gp_1P_vs = 0x88,
	gp_1P_vs_last = ((gp_1P_vs + 4) - 1),
	gp__CPU,
	gp__CPU_last = ((gp__CPU + 4) - 1),
	gp_CPU_vs = 0x92,
	gp_CPU_vs_last = ((gp_CPU_vs + 4) - 1),
	gp__2P,
	gp__2P_last = ((gp__2P + 4) - 1),
};

// Constructs a VS choice string out of its two halves.
#define g_str_vs(first, second) { g_str_4(first), g_str_4(second), '\0' }
// --------------------------

// Cached before applying the independent BGM and SFX preferences.
bool snd_sel_disabled = false;
static char REPLAY_BINARY_MAINL[] = "mainl";
static char REPLAY_BINARY_MAIN[] = "main";
static const char REPLAY_DIR[] = "REPLAY";
static const char REPLAY_INDEX_FN[] = "REPLAY\\TH3R.IDX";
static char REPLAY_SLOT_FN[] = "REPLAY\\TH3R00.RPY";
static const char REPLAY_FALLBACK_FN[] = "TH3LAST.RPY";
#if defined(TH03_PIXEL_CAPTURE)
static uint8_t replay_cfg_checkpoint;
#endif

replay_user_header_t replay_user_menu_header;
replay_user_menu_summary_ext_t replay_user_menu_summary_ext;
replay_user_snapshot_t replay_user_menu_snapshot;
typedef char replay_user_menu_identity_scratch_size_check[
	(sizeof(replay_user_menu_snapshot) >= sizeof(replay_user_identity_ext_t)) ?
		1 : -1
];
static replay_user_index_header_t replay_user_menu_index_header;
extern uint32_t far replay_user_menu_round_real_frames[
	T3_REPLAY_USER_ROUND_SPLIT_COUNT
];
extern replay_user_stage_clear_bonus_t far
	replay_user_menu_stage_clear_bonuses[T3_REPLAY_USER_STAGE_COUNT];
extern uint32_t far replay_user_menu_timed_frames;
extern uint32_t far replay_user_menu_slow_frames;

/// YUME.CFG loading and saving
/// ---------------------------

void near cfg_load(void)
{
	cfg_t cfg;

	cfg_load_and_set_resident(cfg, CFG_FN_CAPS);

	th03_snd_cfg_unpack(cfg.opts.bgm_mode);
	th03_snd_process_init();
	snd_sel_disabled = false;
	if(
		!snd_active &&
		(resident->bgm_mode != SND_BGM_MIDI)
	) {
		resident->bgm_mode = SND_BGM_OFF;
		snd_sel_disabled = true;
	} else if(resident->bgm_mode == SND_BGM_OFF) {
		snd_active = false;
	}

	resident->key_mode = cfg.opts.key_mode;
	resident->rank = cfg.opts.rank;
	resident->autofire = (cfg.opts.autofire == true);
	photosensitivity_enabled_set(
		(cfg.opts.language & T3_CFG_PHOTOSENSITIVITY) != 0
	);
	cfg.opts.language &= T3_CFG_LANGUAGE_MASK;
	if(cfg.opts.language >= LANGUAGE_COUNT) {
		cfg.opts.language = LANGUAGE_JAPANESE;
	}
	language_resident_set(static_cast<language_t>(cfg.opts.language));
	if(language_is_english() && !language_overlay_available()) {
		language_resident_set(LANGUAGE_JAPANESE);
	}
	language_op_apply();
	keyconfig_load(resident->autofire != 0);
}

inline void cfg_save_bytes(cfg_t &cfg, size_t bytes) {
	file_append(CFG_FN_CAPS);
	file_seek(0, SEEK_SET);

	cfg.opts.bgm_mode = th03_snd_cfg_pack();
	cfg.opts.key_mode = resident->key_mode;
	cfg.opts.rank = resident->rank;
	cfg.opts.autofire = (resident->autofire != 0);
	cfg.opts.language = static_cast<uint8_t>(
		language_resident() |
		(photosensitivity_enabled() ? T3_CFG_PHOTOSENSITIVITY : 0)
	);

	file_write(&cfg.opts, bytes);
	file_close();
}

void near cfg_save(void)
{
	cfg_t cfg;
	cfg_save_bytes(cfg, sizeof(cfg.opts));
}

void near cfg_save_exit(void)
{
	cfg_t cfg = { 0 };
	cfg_save_bytes(cfg, sizeof(cfg));
}
/// ---------------------------

static char replay_cfg_mode(void)
{
	static const char fn[] = "T3REPLAY.CFG";
	char cfg[64];
	unsigned read_len;

	for(unsigned i = 0; i < sizeof(cfg); i++) {
		cfg[i] = '\0';
	}
	if(!file_ropen(fn)) {
		return 0;
	}
	read_len = file_read(cfg, (sizeof(cfg) - 1));
	file_close();
	(void)read_len;
	if((cfg[0] == 'r') || (cfg[0] == 'R')) {
		return T3_REPLAY_RES_MODE_RECORD;
	}
	if((cfg[0] == 'p') || (cfg[0] == 'P')) {
		return T3_REPLAY_RES_MODE_PLAYBACK;
	}
	if((cfg[0] == 'v') || (cfg[0] == 'V')) {
#if defined(TH03_PIXEL_CAPTURE)
		// Capture-only suffix: v10 selects checkpoint 10 through the same
		// anchor/target handoff as the in-game Replay browser.
		replay_cfg_checkpoint = 0;
		if((cfg[1] >= '0') && (cfg[1] <= '9')) {
			replay_cfg_checkpoint = static_cast<uint8_t>(cfg[1] - '0');
			if((cfg[2] >= '0') && (cfg[2] <= '9')) {
				replay_cfg_checkpoint = static_cast<uint8_t>(
					(replay_cfg_checkpoint * 10) + (cfg[2] - '0')
				);
			}
		}
#endif
		return T3_REPLAY_RES_MODE_USER_PLAYBACK;
	}
	return 0;
}

static void replay_cfg_load_resident_only(void)
{
	cfg_t cfg;

	cfg_load_and_set_resident(cfg, CFG_FN_CAPS);
	th03_snd_cfg_unpack(cfg.opts.bgm_mode);
	resident->key_mode = cfg.opts.key_mode;
	resident->rank = cfg.opts.rank;
	resident->autofire = (cfg.opts.autofire == true);
	photosensitivity_enabled_set(
		(cfg.opts.language & T3_CFG_PHOTOSENSITIVITY) != 0
	);
	cfg.opts.language &= T3_CFG_LANGUAGE_MASK;
	if(cfg.opts.language >= LANGUAGE_COUNT) {
		cfg.opts.language = LANGUAGE_JAPANESE;
	}
	language_resident_set(static_cast<language_t>(cfg.opts.language));
	if(language_is_english() && !language_overlay_available()) {
		language_resident_set(LANGUAGE_JAPANESE);
	}
	language_op_apply();
	keyconfig_load(resident->autofire != 0);
}

static void replay_resident_handoff_set(char mode)
{
	int i;

	resident->unused_3[0] = T3_REPLAY_RES_MAGIC_0;
	resident->unused_3[1] = T3_REPLAY_RES_MAGIC_1;
	resident->unused_3[2] = T3_REPLAY_RES_MAGIC_2;
	resident->unused_3[3] = T3_REPLAY_RES_MAGIC_3;
	resident->unused_3[T3_REPLAY_RES_MODE_INDEX] = mode;
	resident->unused_3[T3_REPLAY_RES_SLOT_INDEX] = T3_REPLAY_USER_SLOT_NONE;
#if defined(TH03_REPLAY_DEV_STAGE_SELECT)
	resident->unused_3[T3_REPLAY_RES_DEBUG_STAGE_START_INDEX] = 0;
#endif
	for(
		i = T3_REPLAY_RES_SAMPLE_COUNT_INDEX;
		i < T3_REPLAY_RES_CURSOR_END_INDEX;
		i++
	) {
		resident->unused_3[i] = 0;
	}
}

static void replay_resident_handoff_slot_set(uint8_t slot)
{
	resident->unused_3[T3_REPLAY_RES_SLOT_INDEX] = slot;
}

static void replay_resident_handoff_u32_set(unsigned int index, uint32_t value)
{
	resident->unused_3[index + 0] = static_cast<uint8_t>(value);
	resident->unused_3[index + 1] = static_cast<uint8_t>(value >> 8);
	resident->unused_3[index + 2] = static_cast<uint8_t>(value >> 16);
	resident->unused_3[index + 3] = static_cast<uint8_t>(value >> 24);
}

static bool replay_resident_handoff_is(char mode)
{
	return (
		(resident->unused_3[0] == T3_REPLAY_RES_MAGIC_0) &&
		(resident->unused_3[1] == T3_REPLAY_RES_MAGIC_1) &&
		(resident->unused_3[2] == T3_REPLAY_RES_MAGIC_2) &&
		(resident->unused_3[3] == T3_REPLAY_RES_MAGIC_3) &&
		(resident->unused_3[T3_REPLAY_RES_MODE_INDEX] == mode)
	);
}

static void replay_resident_handoff_clear(void)
{
	int i;

	resident->unused_3[0] = 0;
	resident->unused_3[1] = 0;
	resident->unused_3[2] = 0;
	resident->unused_3[3] = 0;
	resident->unused_3[T3_REPLAY_RES_MODE_INDEX] = 0;
	resident->unused_3[T3_REPLAY_RES_SLOT_INDEX] = T3_REPLAY_USER_SLOT_NONE;
#if defined(TH03_REPLAY_DEV_STAGE_SELECT)
	resident->unused_3[T3_REPLAY_RES_DEBUG_STAGE_START_INDEX] = 0;
#endif
	for(
		i = T3_REPLAY_RES_SAMPLE_COUNT_INDEX;
		i < T3_REPLAY_RES_CURSOR_END_INDEX;
		i++
	) {
		resident->unused_3[i] = 0;
	}
}

static void replay_dir_create(void)
{
	dos_axdx(0x3900, REPLAY_DIR);
}

static void replay_user_slot_fn_set(uint8_t slot)
{
	REPLAY_SLOT_FN[11] = static_cast<char>('0' + (slot / 10));
	REPLAY_SLOT_FN[12] = static_cast<char>('0' + (slot % 10));
}

static void replay_memclear(void far *buf, unsigned size);

static void replay_user_summary_ext_init(void)
{
	int i;
	int j;

	replay_user_menu_summary_ext.flags = 0;
	replay_user_menu_summary_ext.round_reached_count = 0;
	for(i = 0; i < T3_REPLAY_USER_ROUND_SPLIT_COUNT; i++) {
		replay_user_menu_summary_ext.round_splits[i].stage_round = 0;
		replay_user_menu_summary_ext.round_splits[i].route_winner = 0;
		replay_user_menu_round_real_frames[i] = 0;
		for(j = 0; j < T3_REPLAY_USER_PACKED_SCORE_SIZE; j++) {
			replay_user_menu_summary_ext.round_splits[i].score_p1[j] = 0;
			replay_user_menu_summary_ext.round_splits[i].score_p2[j] = 0;
		}
	}
	for(i = 0; i < T3_REPLAY_USER_STAGE_COUNT; i++) {
		replay_memclear(
			&replay_user_menu_stage_clear_bonuses[i],
			sizeof(replay_user_stage_clear_bonus_t)
		);
	}
	replay_user_menu_timed_frames = 0;
	replay_user_menu_slow_frames = 0;
	replay_user_menu_summary_ext.checkpoint_count = 0;
	for(i = 0; i < T3R_CKPT_COUNT_MAX; i++) {
		replay_user_menu_summary_ext.checkpoint_stage_round[i] = 0;
	}
}

static bool replay_user_summary_ext_disk_read(void)
{
	int i;
	int j;
	replay_user_round_split_t split;

	if(
		file_read(&replay_user_menu_summary_ext, 2) != 2
	) {
		return false;
	}
	for(i = 0; i < T3_REPLAY_USER_ROUND_SPLIT_COUNT; i++) {
		if(file_read(&split, sizeof(split)) != sizeof(split)) {
			return false;
		}
		replay_user_menu_summary_ext.round_splits[i].stage_round = (
			split.stage_round
		);
		replay_user_menu_summary_ext.round_splits[i].route_winner = (
			split.route_winner
		);
		for(j = 0; j < T3_REPLAY_USER_PACKED_SCORE_SIZE; j++) {
			replay_user_menu_summary_ext.round_splits[i].score_p1[j] = (
				split.score_p1[j]
			);
			replay_user_menu_summary_ext.round_splits[i].score_p2[j] = (
				split.score_p2[j]
			);
		}
		replay_user_menu_round_real_frames[i] = split.real_frames;
	}
	if(
		file_read(
			replay_user_menu_stage_clear_bonuses,
			sizeof(replay_user_menu_stage_clear_bonuses)
		) != sizeof(replay_user_menu_stage_clear_bonuses)
	) {
		return false;
	}
	if(
		(file_read(&replay_user_menu_timed_frames, sizeof(uint32_t)) !=
		 sizeof(uint32_t)) ||
		(file_read(&replay_user_menu_slow_frames, sizeof(uint32_t)) !=
		 sizeof(uint32_t))
	) {
		return false;
	}
	return (
		file_read(
			&replay_user_menu_summary_ext.checkpoint_count,
			(1 + T3R_CKPT_COUNT_MAX)
		) == (1 + T3R_CKPT_COUNT_MAX)
	);
}

static void replay_memclear(void far *buf, unsigned size)
{
	uint8_t far *p = reinterpret_cast<uint8_t far *>(buf);

	while(size != 0) {
		*p++ = 0;
		size--;
	}
}

static bool replay_user_menu_snapshot_disk_read(void)
{
	replay_user_snapshot_compact_t near *compact = (
		reinterpret_cast<replay_user_snapshot_compact_t near *>(
			&replay_user_menu_snapshot
		)
	);

	if(replay_user_menu_header.version == T3_REPLAY_USER_VERSION_LEGACY) {
		return (
			file_read(
				&replay_user_menu_snapshot,
				sizeof(replay_user_menu_snapshot)
			) == sizeof(replay_user_menu_snapshot)
		);
	}
	if(
		file_read(compact, sizeof(*compact)) != sizeof(*compact)
	) {
		return false;
	}
	replay_user_menu_snapshot.autofire = compact->autofire;
	return true;
}

static uint16_t replay_file_current_psp(void)
{
	uint16_t psp = 0;

	asm {
		push	bp
		push	si
		push	di
		push	ds
		push	es
		mov 	ah, 51h
		int 	21h
		push	bx
		pop 	cx
		pop 	es
		pop 	ds
		pop 	di
		pop 	si
		pop 	bp
		mov 	psp, cx
	}
	return psp;
}

static bool replay_file_commit_process(void)
{
	uint16_t dpl[11];
	uint16_t dos_flags = 1;
	int i;

	for(i = 0; i < 11; i++) {
		dpl[i] = 0;
	}
	dpl[10] = replay_file_current_psp();
	asm {
		push	bp
		push	si
		push	di
		push	es
		push	ds
		push	ss
		pop 	ds
		lea 	dx, dpl
		mov 	ax, 5D01h
		int 	21h
		pushf
		pop 	ax
		pop 	ds
		pop 	es
		pop 	di
		pop 	si
		pop 	bp
		mov 	dos_flags, ax
	}
	return ((dos_flags & 1) == 0);
}

static void replay_file_delete_commit(const char far *fn)
{
	asm {
		push	ds
		lds 	dx, fn
		mov 	ah, 41h
		int 	21h
		pop 	ds
	}
	(void)replay_file_commit_process();
}

static bool replay_file_rename(
	const char far *old_fn, const char far *new_fn
)
{
	asm {
		push	ds
		push	es
		push	di
		lds 	dx, old_fn
		les 	di, new_fn
		mov 	ax, 5600h
		int 	21h
		pop 	di
		pop 	es
		pop 	ds
	}
	return !FLAGS_CARRY;
}

static bool replay_user_header_valid(const replay_user_header_t near& header)
{
	return (
		(header.magic[0] == 'T') &&
		(header.magic[1] == '3') &&
		(header.magic[2] == 'R') &&
		(header.magic[3] == 'P') &&
		(header.magic[4] == 'L') &&
		(header.magic[5] == 'Y') &&
		(header.magic[6] == '1') &&
		(
			header.magic[7] == static_cast<char>(
				'0' + (header.version - 10)
			)
		) &&
		replay_user_version_supported(header.version) &&
		(header.sample_size == T3_REPLAY_USER_SAMPLE_SIZE_RLE) &&
		((header.flags & T3_REPLAY_USER_FLAG_RLE_INPUT) != 0) &&
		((header.flags & T3_REPLAY_USER_FLAG_CHARGE_INPUT) != 0) &&
		(
			(
				(header.flags & T3_REPLAY_USER_FLAG_PRACTICE) &&
				(header.game_mode == GM_VS_1P_CPU) &&
				practice_replay_config_valid(
					header.scenario.practice.config
				)
			) ||
			((header.flags & T3_REPLAY_USER_FLAG_PRACTICE) == 0)
		) &&
		(header.autofire <= 0x03) &&
		(
			(header.version == T3_REPLAY_USER_VERSION_LEGACY) ||
			(
				(header.summary_flags & T3_REPLAY_USER_SUMMARY_CURRENT) ==
				T3_REPLAY_USER_SUMMARY_CURRENT
			)
		) &&
		(header.header_size == replay_user_header_size(header.version)) &&
		(header.snapshot_offset == header.header_size) &&
		(header.snapshot_size == replay_user_checkpoint_size(header.version)) &&
		(header.input_offset == replay_user_input_offset(
			header.version, header.game_mode, header.flags
		)) &&
		(header.sample_count != 0)
	);
}

static bool replay_user_index_header_valid(
	const replay_user_index_header_t near& header
)
{
	return (
		(header.magic[0] == 'T') &&
		(header.magic[1] == '3') &&
		(header.magic[2] == 'R') &&
		(header.magic[3] == 'I') &&
		(header.magic[4] == 'D') &&
		(header.magic[5] == 'X') &&
		(header.magic[6] == '9') &&
		(header.version == T3_REPLAY_USER_INDEX_VERSION) &&
		(header.header_size == sizeof(replay_user_index_header_t)) &&
		(header.entry_size == sizeof(replay_user_index_entry_t)) &&
		(header.slot_count == T3_REPLAY_USER_SLOT_COUNT) &&
		(header.next_slot < T3_REPLAY_USER_SLOT_COUNT)
	);
}

static bool replay_user_read_for_menu(const char *fn)
{
	replay_user_round_state_t round_state;
	replay_user_identity_ext_t near *identity =
		reinterpret_cast<replay_user_identity_ext_t near *>(
			&replay_user_menu_snapshot
		);

	if(!file_ropen(fn)) {
		return false;
	}
	if(
		file_read(&replay_user_menu_header, sizeof(replay_user_menu_header)) !=
		sizeof(replay_user_menu_header)
	) {
		file_close();
		return false;
	}
	if(!replay_user_header_valid(replay_user_menu_header)) {
		file_close();
		return false;
	}
	replay_user_summary_ext_init();
	if(!replay_user_summary_ext_disk_read()) {
		file_close();
		return false;
	}
	if(
		(file_read(
			identity, sizeof(*identity)
		) != sizeof(*identity)) ||
		!replay_user_identity_valid(
			*identity, replay_user_menu_header.game_mode
		)
	) {
		file_close();
		return false;
	}
	if(
		replay_user_menu_summary_ext.round_reached_count >
		T3_REPLAY_USER_ROUND_SPLIT_COUNT
	) {
		file_close();
		return false;
	}
	if(
		replay_user_version_has_round_state(replay_user_menu_header.version) &&
		(
			(
				(replay_user_menu_summary_ext.flags &
				 T3_REPLAY_USER_SUMMARY_CURRENT) !=
				T3_REPLAY_USER_SUMMARY_CURRENT
			) ||
			(replay_user_menu_summary_ext.checkpoint_count == 0) ||
			(replay_user_menu_summary_ext.checkpoint_count >
			 replay_user_checkpoint_capacity(
				replay_user_menu_header.game_mode,
				replay_user_menu_header.flags
			 ))
		)
	) {
		file_close();
		return false;
	}
	file_seek(
		(replay_user_menu_header.snapshot_offset +
		 T3R_STAGE_CKPT_PREFIX_SIZE),
		SEEK_SET
	);
	if(!replay_user_menu_snapshot_disk_read()) {
		file_close();
		return false;
	}
	if(replay_user_version_has_round_state(replay_user_menu_header.version)) {
		if(
			file_read(
				&round_state, sizeof(round_state)
			) != sizeof(round_state)
		) {
			file_close();
			return false;
		}
	}
	if(replay_user_menu_snapshot.autofire != replay_user_menu_header.autofire) {
		file_close();
		return false;
	}
	file_close();
	return true;
}

static bool replay_user_checkpoint_read_for_menu(
	uint8_t checkpoint,
	uint32_t _ss *sample_count,
	uint32_t _ss *global_frame,
	uint32_t _ss *input_size
)
{
	uint32_t offset;
	uint8_t stage_round;
	uint8_t stage;
	replay_user_round_state_t round_state;

	if(replay_user_version_has_round_state(replay_user_menu_header.version)) {
		if(checkpoint >= replay_user_menu_summary_ext.checkpoint_count) {
			return false;
		}
		stage_round = (
			replay_user_menu_summary_ext.checkpoint_stage_round[checkpoint]
		);
	} else {
		if(
			(replay_user_menu_header.game_mode != GM_STORY) ||
			(checkpoint >= T3_REPLAY_USER_STAGE_COUNT) ||
			(checkpoint >= replay_user_menu_header.stage_reached_count)
		) {
			return false;
		}
		stage_round = checkpoint;
	}
	stage = (stage_round & 0x0F);
	offset = (
		replay_user_menu_header.snapshot_offset +
		(
			static_cast<uint32_t>(checkpoint) *
			static_cast<uint32_t>(
				replay_user_checkpoint_size(replay_user_menu_header.version)
			)
		)
	);
#if defined(TH03_PIXEL_CAPTURE)
	if(!file_ropen(REPLAY_FALLBACK_FN)) {
#else
	if(!file_ropen(REPLAY_SLOT_FN)) {
#endif
		return false;
	}
	file_seek(offset, SEEK_SET);
	if(
		(file_read(sample_count, sizeof(*sample_count)) != sizeof(*sample_count)) ||
		(file_read(global_frame, sizeof(*global_frame)) != sizeof(*global_frame)) ||
		(file_read(input_size, sizeof(*input_size)) != sizeof(*input_size)) ||
		!replay_user_menu_snapshot_disk_read()
	) {
		file_close();
		return false;
	}
	if(replay_user_version_has_round_state(replay_user_menu_header.version)) {
		if(
			file_read(
				&round_state, sizeof(round_state)
			) != sizeof(round_state)
		) {
			file_close();
			return false;
		}
	}
	file_close();
	if(
		(replay_user_menu_snapshot.game_mode != replay_user_menu_header.game_mode) ||
		(replay_user_menu_snapshot.autofire != replay_user_menu_header.autofire) ||
		(*sample_count > replay_user_menu_header.sample_count) ||
		(*global_frame > replay_user_menu_header.final_frame_count) ||
		(*input_size > replay_user_menu_header.input_size)
	) {
		return false;
	}
	if(replay_user_menu_header.game_mode == GM_STORY) {
		return (replay_user_menu_snapshot.story_stage == stage);
	}
	if(replay_user_version_has_round_state(replay_user_menu_header.version)) {
		return (round_state.round_id == (stage_round >> 4));
	}
	return true;
}

static bool replay_user_read_slot_for_menu(uint8_t slot)
{
	if(slot >= T3_REPLAY_USER_SLOT_COUNT) {
		return false;
	}
	replay_user_slot_fn_set(slot);
	return replay_user_read_for_menu(REPLAY_SLOT_FN);
}

static bool replay_user_read_for_menu(void)
{
	if(replay_user_read_for_menu(REPLAY_FALLBACK_FN)) {
		return true;
	}
	return replay_user_read_slot_for_menu(0);
}

static uint8_t replay_user_checkpoint_count_for_menu(void)
{
	uint8_t count;

	if(replay_user_version_has_round_state(replay_user_menu_header.version)) {
		return replay_user_menu_summary_ext.checkpoint_count;
	}
	if(replay_user_menu_header.game_mode == GM_STORY) {
		count = replay_user_menu_header.stage_reached_count;
		return (
			(count > T3_REPLAY_USER_STAGE_COUNT) ?
				T3_REPLAY_USER_STAGE_COUNT : count
		);
	}
	return 1;
}

static uint8_t replay_user_first_used_slot(void)
{
	uint8_t slot;

	for(slot = 0; slot < T3_REPLAY_USER_SLOT_COUNT; slot++) {
		if(replay_user_read_slot_for_menu(slot)) {
			return slot;
		}
	}
	return 0;
}

static void replay_user_index_header_fill(uint8_t next_slot)
{
	replay_memclear(
		&replay_user_menu_index_header, sizeof(replay_user_menu_index_header)
	);
	replay_user_menu_index_header.magic[0] = 'T';
	replay_user_menu_index_header.magic[1] = '3';
	replay_user_menu_index_header.magic[2] = 'R';
	replay_user_menu_index_header.magic[3] = 'I';
	replay_user_menu_index_header.magic[4] = 'D';
	replay_user_menu_index_header.magic[5] = 'X';
	replay_user_menu_index_header.magic[6] = '9';
	replay_user_menu_index_header.magic[7] = '\0';
	replay_user_menu_index_header.version = T3_REPLAY_USER_INDEX_VERSION;
	replay_user_menu_index_header.header_size = (
		sizeof(replay_user_menu_index_header)
	);
	replay_user_menu_index_header.entry_size = sizeof(replay_user_index_entry_t);
	replay_user_menu_index_header.slot_count = T3_REPLAY_USER_SLOT_COUNT;
	replay_user_menu_index_header.next_slot = next_slot;
}

static bool replay_user_index_create(uint8_t next_slot)
{
	replay_user_index_entry_t entry;
	int slot;

	replay_user_index_header_fill(next_slot);
	replay_memclear(&entry, sizeof(entry));
	if(!file_create(REPLAY_INDEX_FN)) {
		return false;
	}
	if(
		file_write(
			&replay_user_menu_index_header,
			sizeof(replay_user_menu_index_header)
		) == 0
	) {
		file_close();
		return false;
	}
	for(slot = 0; slot < T3_REPLAY_USER_SLOT_COUNT; slot++) {
		if(file_write(&entry, sizeof(entry)) == 0) {
			file_close();
			return false;
		}
	}
	file_close();
	return replay_file_commit_process();
}

static void replay_user_index_entry_fill(
	replay_user_index_entry_t far& entry, uint8_t slot
)
{
	int i;

	replay_memclear(&entry, sizeof(entry));
	entry.used = true;
	entry.slot_id = slot;
	entry.status = replay_user_menu_header.status;
	entry.end_reason = replay_user_menu_header.end_reason;
	entry.game_mode = replay_user_menu_header.game_mode;
	entry.rank = replay_user_menu_header.rank;
	entry.key_mode = replay_user_menu_header.key_mode;
	entry.playchar_p1 = replay_user_menu_header.playchar_p1;
	entry.playchar_p2 = replay_user_menu_header.playchar_p2;
	entry.story_stage = replay_user_menu_header.story_stage;
	entry.is_cpu_p1 = replay_user_menu_header.is_cpu_p1;
	entry.is_cpu_p2 = replay_user_menu_header.is_cpu_p2;
	entry.sample_count = replay_user_menu_header.sample_count;
	entry.final_frame_count = replay_user_menu_header.final_frame_count;
	for(i = 0; i < T3_REPLAY_USER_NAME_LEN; i++) {
		entry.name[i] = replay_user_menu_header.name[i];
	}
	entry.dos_date = replay_user_menu_header.dos_date;
	entry.autofire = replay_user_menu_header.autofire;
	entry.replay_flags = static_cast<uint8_t>(
		replay_user_menu_header.flags & T3_REPLAY_USER_FLAG_PRACTICE
	);
	entry.summary_flags = replay_user_menu_header.summary_flags;
	entry.final_route = replay_user_menu_header.final_route;
	entry.final_story_stage = replay_user_menu_header.final_story_stage;
	entry.final_story_lives = replay_user_menu_header.final_story_lives;
	entry.final_misses = replay_user_menu_header.final_misses;
	entry.stage_reached_count = replay_user_menu_header.stage_reached_count;
	for(i = 0; i < T3_REPLAY_USER_PACKED_SCORE_SIZE; i++) {
		entry.final_score[i] = replay_user_menu_header.final_score[i];
	}
	if((replay_user_menu_header.flags & T3_REPLAY_USER_FLAG_PRACTICE) == 0) {
		for(i = 0; i < T3_REPLAY_USER_STAGE_COUNT; i++) {
			entry.stage_opponents[i] = (
				replay_user_menu_header.scenario.story.stage_opponents[i]
			);
		}
	}
}

static bool replay_user_index_slot_write(uint8_t slot)
{
	replay_user_index_entry_t entry;
	uint32_t offset;
	bool valid = false;

	if(file_ropen(REPLAY_INDEX_FN)) {
		valid = (
			file_read(
				&replay_user_menu_index_header,
				sizeof(replay_user_menu_index_header)
			) == sizeof(replay_user_menu_index_header)
		);
		file_close();
		if(valid) {
			valid = replay_user_index_header_valid(
				replay_user_menu_index_header
			);
		}
	}
	if(!valid) {
		if(!replay_user_index_create(
			((slot + 1) % T3_REPLAY_USER_SLOT_COUNT)
		)) {
			return false;
		}
	}
	if(!file_append(REPLAY_INDEX_FN)) {
		return false;
	}
	replay_user_menu_index_header.next_slot = (
		(slot + 1) % T3_REPLAY_USER_SLOT_COUNT
	);
	file_seek(0, SEEK_SET);
	if(
		file_write(
			&replay_user_menu_index_header,
			sizeof(replay_user_menu_index_header)
		) == 0
	) {
		file_close();
		return false;
	}
	replay_user_index_entry_fill(entry, slot);
	offset = (
		static_cast<uint32_t>(sizeof(replay_user_menu_index_header)) +
		(
			static_cast<uint32_t>(slot) *
			static_cast<uint32_t>(sizeof(entry))
		)
	);
	file_seek(offset, SEEK_SET);
	if(file_write(&entry, sizeof(entry)) == 0) {
		file_close();
		return false;
	}
	file_close();
	return replay_file_commit_process();
}

static void replay_user_restore_resident_from_menu(void)
{
	int i;
	int digit;

	practice_resident_clear();
	// The process-entry seed is stored independently from the compact
	// checkpoint seed, including in early V12 files with bad stage seeds.
	resident->rand = replay_user_menu_snapshot.resident_rand;
	resident->rank = replay_user_menu_snapshot.rank;
	resident->key_mode = replay_user_menu_snapshot.key_mode;
	resident->game_mode = replay_user_menu_snapshot.game_mode;
	resident->story_stage = replay_user_menu_snapshot.story_stage;
	resident->story_lives = replay_user_menu_snapshot.story_lives;
	resident->rem_credits = replay_user_menu_snapshot.rem_credits;
	resident->skill = replay_user_menu_snapshot.skill;
	resident->demo_num = replay_user_menu_snapshot.demo_num;
	resident->pid_winner = replay_user_menu_snapshot.pid_winner;
	resident->show_score_menu = replay_user_menu_snapshot.show_score_menu;
	resident->op_animation_fast = replay_user_menu_snapshot.op_animation_fast;

	for(i = 0; i < PLAYER_COUNT; i++) {
		resident->is_cpu[i] = replay_user_menu_snapshot.is_cpu[i];
		resident->playchar_paletted[i].v = (
			replay_user_menu_snapshot.playchar_paletted[i]
		);
		for(digit = 0; digit < SCORE_DIGITS; digit++) {
			resident->score_last[i].digits[digit] = (
				replay_user_menu_snapshot.score_last[i][digit]
			);
		}
	}
	for(i = 0; i < STAGE_COUNT; i++) {
		resident->story_opponents[i].v = (
			replay_user_menu_snapshot.story_opponents[i]
		);
	}
	if(replay_user_menu_header.flags & T3_REPLAY_USER_FLAG_PRACTICE) {
		practice_replay_config_restore(
			replay_user_menu_header.scenario.practice.config
		);
	}
}

#define resident_reset_scores(i) { \
	/* ZUN bloat: Very unsafe. */ \
	for(i = 0; i < (PLAYER_COUNT * SCORE_DIGITS); i++) { \
		resident->score_last[0].digits[i] = 0; \
	} \
}

inline bool switch_to_mainl(bool opwin_free) {
	cfg_save();

	// ZUN landmine: The system's previous gaiji should be restored *after*
	// TRAM gets cleared in game_exit(), not before while we're still showing
	// menu text.
	gaiji_restore();

	snd_kaja_func(KAJA_SONG_STOP, 0);
	if(opwin_free) {
		super_free(); // ZUN bloat: Process termination will do this anyway.
	}

	// ZUN landmine: The screen clearing done in this function will almost
	// certainly not run within VBLANK.
	game_exit();

	execl(BINARY_MAINL, BINARY_MAINL, nullptr);
	return false;
}

inline bool switch_to_mainl_preserve_bgm(bool opwin_free) {
	cfg_save();
	gaiji_restore();
	if(opwin_free) {
		super_free();
	}
	game_exit();
	execl(BINARY_MAINL, BINARY_MAINL, nullptr);
	return false;
}

#if defined(TH03_REPLAY_DEV_STAGE_SELECT)
static int near replay_dev_story_stage_menu(void);
#endif

playchar_t far practice_stage7_opponent(playchar_t playchar)
{
	// ACTUAL TYPE: playchar_t
	static const uint8_t STAGE7_OPPONENT_FOR[PLAYCHAR_COUNT] = {
		PLAYCHAR_MIMA, // for Reimu
		PLAYCHAR_REIMU, // for Mima
		PLAYCHAR_REIMU, // for Marisa
		PLAYCHAR_MARISA, // for Ellen
		PLAYCHAR_REIMU, // for Kotohime
		PLAYCHAR_ELLEN, // for Kana
		PLAYCHAR_KANA, // for Rikako
		PLAYCHAR_KOTOHIME, // for Chiyuri
		PLAYCHAR_RIKAKO, // for Yumemi
	};

	return static_cast<playchar_t>(STAGE7_OPPONENT_FOR[playchar]);
}

static bool near story_start(bool select_character)
{
	enum {
		RANDOM_OPPONENT_MIN = PLAYCHAR_REIMU,
		RANDOM_OPPONENT_MAX = PLAYCHAR_RIKAKO,
		RANDOM_OPPONENT_COUNT = (
			(RANDOM_OPPONENT_MAX - RANDOM_OPPONENT_MIN) + 1
		),
	};

	static bool opponent_seen[RANDOM_OPPONENT_COUNT] = { false };

	int stage;
	int candidate;
#if defined(TH03_REPLAY_DEV_STAGE_SELECT)
	int replay_dev_story_stage = 0;
#endif

	practice_resident_clear();
	resident->demo_num = 0;
	resident->pid_winner = 0;
	resident->story_stage = 0;
	resident->is_cpu[0] = false;
	resident->is_cpu[1] = true;
	resident->game_mode = GM_STORY;
	resident->story_lives = CREDIT_LIVES;
	resident->show_score_menu = false;
	resident->playchar_paletted[1].v = -1;

	if(select_character && select_story_menu()) {
		return true;
	}
#if defined(TH03_REPLAY_DEV_STAGE_SELECT)
	if(select_character) {
		replay_dev_story_stage = replay_dev_story_stage_menu();
		if(replay_dev_story_stage == STAGE_NONE) {
			return true;
		}
	}
#endif

retry_opponent_selection:
	// ACTUAL TYPE: playchar_t
	int stage7_opponent = practice_stage7_opponent(
		resident->playchar_paletted[0].char_id_16()
	);
	irand_init(resident->rand);

	for(stage = 0; stage < 6; stage++) {
		// Confirmed to terminate for all 2³² possible seeds and all original
		// combinations of player character and Stage 7 opponent.
		do {
			candidate = (
				RANDOM_OPPONENT_MIN + (irand() % RANDOM_OPPONENT_COUNT)
			);
		} while(opponent_seen[candidate] || (stage7_opponent == candidate));
		opponent_seen[candidate] = true;

		// ZUN bloat: Should not change types.
		#define candidate_paletted candidate
		candidate_paletted = TO_OPTIONAL_PALETTED(candidate);
		resident->story_opponents[stage].v = candidate_paletted;

		// ZUN bloat: All of these palette swaps could have been done in a
		// single loop at the end.
		if(candidate_paletted == resident->playchar_paletted[0].v) {
			resident->story_opponents[stage].v = (candidate_paletted + 1);
		}
		#undef candidate_paletted
	}

	resident->playchar_paletted[1] = resident->story_opponents[0];
	resident->story_opponents[6].v = TO_OPTIONAL_PALETTED(stage7_opponent);

	// ZUN bloat: Palette swaps...
	resident->story_opponents[7].set(PLAYCHAR_CHIYURI);
	if(
		resident->playchar_paletted[0].v ==
		TO_OPTIONAL_PALETTED(PLAYCHAR_CHIYURI)
	) {
		resident->story_opponents[7].v++;
	}
	resident->story_opponents[8].set(PLAYCHAR_YUMEMI);
	if(
		resident->playchar_paletted[0].v ==
		TO_OPTIONAL_PALETTED(PLAYCHAR_YUMEMI)
	) {
		resident->story_opponents[8].v++;
	}

	// ZUN bloat: This can never happen.
	for(stage = 0; stage < STAGE_COUNT; stage++) {
		if(resident->story_opponents[stage].char_id_16() >= PLAYCHAR_COUNT) {
			goto retry_opponent_selection;
		}
	}

#if defined(TH03_REPLAY_DEV_STAGE_SELECT)
	resident->story_stage = replay_dev_story_stage;
	resident->playchar_paletted[1] = resident->story_opponents[
		replay_dev_story_stage
	];
#endif
	resident_reset_scores(stage);
	resident->rem_credits = 3;
	resident->op_animation_fast = false;
	resident->skill = (70 + (resident->rank * 25));
	replay_resident_handoff_set(T3_REPLAY_RES_MODE_USER_RECORD);
	replay_resident_handoff_slot_set(T3_REPLAY_USER_SLOT_NONE);
#if defined(TH03_REPLAY_DEV_STAGE_SELECT)
	resident->unused_3[T3_REPLAY_RES_DEBUG_STAGE_START_INDEX] = (
		replay_dev_story_stage
	);
#endif
	return switch_to_mainl(false);
}

bool near story_menu(void)
{
	t3pix_scene_set(T3PIX_SCENE_CHARACTER_SELECT);
	return story_start(true);
}

inline tram_y_t choice_tram_y(unsigned int line) {
	return ((BOX_TOP / GLYPH_H) + 1 + line);
}

static void near title_menu_graphics_unput(screen_x_t box_right)
{
	if(menu_font) {
		title_box_interior_restore(
			(choice_tram_y(0) * GLYPH_H), (7 * GLYPH_H), box_right
		);
	}
}

static void near title_choice_graphics_unput(
	unsigned line, screen_x_t box_right
)
{
	if(menu_font) {
		title_box_interior_restore(
			(choice_tram_y(line) * GLYPH_H), GLYPH_H, box_right
		);
	}
}

void pascal near vs_choice_put(int sel, tram_atrb2 atrb)
{
	enum {
		W = (8 * GAIJI_W),
		TRAM_LEFT = (
			(BOX_SUBMENU_CENTER_X - (W / 2) + GLYPH_HALF_W) /
			GLYPH_HALF_W
		),
	};
	if(sel == VS_1P_CPU) {
		static const char STR[] = g_str_vs(gp_1P_vs, gp__CPU);
		gaiji_putsa(TRAM_LEFT, choice_tram_y(2), STR, atrb);
	} else if(sel == VS_1P_2P) {
		static const char STR[] = g_str_vs(gp_1P_vs, gp__2P);
		gaiji_putsa(TRAM_LEFT, choice_tram_y(3), STR, atrb);
	} else /* if (sel == VS_CPU_CPU) */ {
		static const char STR[] = g_str_vs(gp_CPU_vs, gp__CPU);
		gaiji_putsa(TRAM_LEFT, choice_tram_y(4), STR, atrb);
	}
}

static bool near vs_start(bool select_characters)
{
	int sel;

	// ZUN quirk: This assignment causes any initially held inputs to be
	// processed immediately, just like in the Main menu at startup, but unlike
	// after a later switch between the Main and Option menu.
	input_t input_prev = INPUT_NONE;

	// After a match, we come back here, skip the menu, and launch into
	// character selection.
	if(resident->game_mode < GM_VS) {
		text_clear();
		title_menu_graphics_unput(BOX_MAIN_RIGHT);
		title_credit_graphics_unput();
		box_main_to_submenu_animate(BOX_SUBMENU_RIGHT);

		sel = VS_1P_CPU;
		vs_choice_put(VS_1P_CPU, TX_WHITE);
		vs_choice_put(VS_1P_2P, TX_BLACK);
		vs_choice_put(VS_CPU_CPU, TX_BLACK);

		while(1) {
			input_mode_interface();
			if(input_prev == INPUT_NONE) {
				if(input_sp & INPUT_UP) {
					vs_choice_put(sel, TX_BLACK);
					ring_dec(sel, VS_CPU_CPU);
					vs_choice_put(sel, TX_WHITE);
				}
				if(input_sp & INPUT_DOWN) {
					vs_choice_put(sel, TX_BLACK);
					ring_inc(sel, VS_CPU_CPU);
					vs_choice_put(sel, TX_WHITE);
				}
				if((input_sp & INPUT_SHOT) || (input_sp & INPUT_OK)) {
					break;
				}
				if(input_sp & INPUT_CANCEL) {
					title_menu_graphics_unput(BOX_SUBMENU_RIGHT);
					return false;
				}
			}
			input_prev = input_sp;
			frame_delay(1);
		}
	} else {
		sel = (resident->game_mode - GM_VS);
	}

	if(select_characters) {
		practice_resident_clear();
	}
	resident->is_cpu[0] = ((sel == VS_CPU_CPU) ? true : false);
	resident->is_cpu[1] = ((sel != VS_1P_2P) ? true : false);
	resident->demo_num = 0;
	resident->pid_winner = 0;
	resident->story_stage = (
		practice_resident_active() ?
		practice_resident_u8(T3_PRACTICE_RES_STAGE_INDEX) : 0
	);
	resident->story_lives = (
		practice_resident_uses_stock() ?
		practice_resident_u8(T3_PRACTICE_RES_STOCK_INDEX) : 0
	);
	resident->game_mode = (GM_VS + sel);
	resident->show_score_menu = false;
	if(practice_resident_active()) {
		practice_resident_u8_set(T3_PRACTICE_RES_INITIAL_STAGE_INDEX, true);
	}

	// ZUN bloat: Could be compressed into a single branch.
	if(select_characters) {
		if(sel == VS_1P_2P) {
			if(select_1p_vs_2p_menu()) {
				resident->game_mode = GM_NONE;
				return true;
			}
		} else {
			bool resume = false;
			while(1) {
				if(select_vs_cpu_menu(resume)) {
					resident->game_mode = GM_NONE;
					return true;
				}
				if(sel != VS_1P_CPU) {
					break;
				}
				if(!practice_setup_menu()) {
					break;
				}
				// Keep select.m and the selection graphics across this return.
				while(input_sp != INPUT_NONE) {
					frame_delay(1);
					input_mode_interface();
				}
				palette_100();
				resume = true;
			}
			select_free();
		}
	}

	if(sel == VS_CPU_CPU) {
		replay_resident_handoff_clear();
	} else {
		replay_resident_handoff_set(T3_REPLAY_RES_MODE_USER_RECORD);
		replay_resident_handoff_slot_set(T3_REPLAY_USER_SLOT_NONE);
	}
	resident_reset_scores(sel);
	return switch_to_mainl(false);
}

bool near vs_menu(void)
{
	t3pix_scene_set(T3PIX_SCENE_CHARACTER_SELECT);
	return vs_start(true);
}

static void replay_demo_resident_set(void)
{
	static const int8_t PAIRINGS[DEMO_COUNT * PLAYER_COUNT] = {
		TO_OPTIONAL_PALETTED(PLAYCHAR_MIMA),
		TO_OPTIONAL_PALETTED(PLAYCHAR_REIMU),

		TO_OPTIONAL_PALETTED(PLAYCHAR_MARISA),
		TO_OPTIONAL_PALETTED(PLAYCHAR_RIKAKO),

		TO_OPTIONAL_PALETTED(PLAYCHAR_ELLEN),
		TO_OPTIONAL_PALETTED(PLAYCHAR_KANA),

		TO_OPTIONAL_PALETTED(PLAYCHAR_KOTOHIME),
		TO_OPTIONAL_PALETTED(PLAYCHAR_MARISA),
	};
	static const int32_t RAND[DEMO_COUNT] = { 600, 1000, 3200, 500 };

	practice_resident_clear();
	resident->is_cpu[0] = true;
	resident->is_cpu[1] = true;
	ring_inc_range(resident->demo_num, 1, DEMO_COUNT);
	resident->pid_winner = 0;

	// Critically important to guarantee deterministic demos!
	resident->story_stage = 0;

	resident->game_mode = GM_DEMO;
	resident->show_score_menu = false;

	// ZUN bloat: A two-dimensional array would have been more readable and
	// would have generated better code.
	resident->playchar_paletted[0].v = (
		PAIRINGS[((resident->demo_num - 1) * PLAYER_COUNT) + 0]
	);
	resident->playchar_paletted[1].v = (
		PAIRINGS[((resident->demo_num - 1) * PLAYER_COUNT) + 1]
	);

	resident->rand = RAND[resident->demo_num - 1];
	int i;
	resident_reset_scores(i);
}

static void replay_start_demo_headless(char mode)
{
#if defined(TH03_PIXEL_CAPTURE)
	uint8_t checkpoint;
	uint8_t anchor;
	uint32_t sample_count;
	uint32_t global_frame;
	uint32_t input_size;
#endif

	replay_cfg_load_resident_only();
	replay_resident_handoff_set(mode);
	if(mode == T3_REPLAY_RES_MODE_USER_PLAYBACK) {
		if(!replay_user_read_for_menu()) {
			return;
		}
#if defined(TH03_PIXEL_CAPTURE)
		checkpoint = replay_cfg_checkpoint;
		if(checkpoint != 0) {
			if(checkpoint >= replay_user_checkpoint_count_for_menu()) {
				return;
			}
			// Match the Replay browser's ordinary checkpoint selection. MAINL
			// applies a broad accelerator when the selected anchor provides one.
			replay_checkpoint_force_preroll_set(false);
			anchor = replay_checkpoint_anchor_for_menu(checkpoint);
			if(!replay_user_checkpoint_read_for_menu(
				anchor, &sample_count, &global_frame, &input_size
			)) {
				return;
			}
			replay_resident_handoff_u32_set(
				T3_REPLAY_RES_SAMPLE_COUNT_INDEX, sample_count
			);
			replay_resident_handoff_u32_set(
				T3_REPLAY_RES_GLOBAL_FRAME_INDEX, global_frame
			);
			replay_resident_handoff_u32_set(
				T3_REPLAY_RES_INPUT_SIZE_INDEX, input_size
			);
			replay_checkpoint_handoff_set(anchor);
		}
#endif
		replay_user_restore_resident_from_menu();
		execl(REPLAY_BINARY_MAINL, REPLAY_BINARY_MAINL, nullptr);
		return;
	} else {
		replay_demo_resident_set();
	}
	execl(REPLAY_BINARY_MAIN, REPLAY_BINARY_MAIN, nullptr);
}

void near start_demo(void)
{
	replay_demo_resident_set();
	palette_black_out(1);

	switch_to_mainl(false);
}

void near wait_for_input_or_start_demo_then_box_to_main_animate(void)
{
	{
		input_sp = INPUT_NONE;
		int frame = 0;
		while(input_sp == INPUT_NONE) {
			input_mode_interface();
			resident->rand++;
			frame++;
			if(frame > 520) {
				start_demo();
			}
			frame_delay(1);
		}
	}

	super_put(BOX_LEFT, BOX_TOP, OPWIN_LEFT);

	// ZUN bloat: Should maybe be merged with the two others in `m_main.cpp`.
	{for(
		screen_x_t right_left = (BOX_LEFT + OPWIN_W);
		right_left < (BOX_MAIN_RIGHT - OPWIN_STEP_W);
		right_left += OPWIN_STEP_W
	) {
		box_column16_unput(right_left);
		super_put(right_left, BOX_TOP, OPWIN_RIGHT);
		frame_delay(1);
	}}
}

bool near score_menu(void)
{
	t3pix_scene_set(T3PIX_SCENE_HISCORE);
	resident->story_stage = STAGE_NONE;
	resident->show_score_menu = true;
	resident->game_mode = GM_NONE;
	int i;
	resident_reset_scores(i);
	return switch_to_mainl(true);
}

enum {
	REPLAY_MENU_LIST_LEFT = 4,
	REPLAY_MENU_LIST_W = 72,
	REPLAY_MENU_DETAIL_LEFT = 4,
	REPLAY_MENU_DETAIL_W = 72,
	REPLAY_MENU_HEAD_Y = 5,
	REPLAY_MENU_LIST_Y = 6,
	REPLAY_MENU_DETAIL_Y = 4,
	REPLAY_MENU_FOOT_Y = 24,
	REPLAY_SAVE_COMPLETE_Y = 22,
	REPLAY_MENU_VISIBLE = 10,

	REPLAY_REGI_GLYPH_W = 32,
	REPLAY_REGI_GLYPH_H = 32,
	REPLAY_REGI_ROW_SPACING = 24,
	REPLAY_REGI_GLYPHS_PER_ROW = 16,
	REPLAY_REGI_GRID_LEFT = 64,
	REPLAY_REGI_GRID_TOP = 256,
	REPLAY_REGI_PLANE_SIZE = (
		(REPLAY_REGI_GLYPH_W / 8) * REPLAY_REGI_GLYPH_H
	),
	REPLAY_REGI_DASH_TOP = 13,
	REPLAY_REGI_DASH_BOTTOM = 18,

	REPLAY_NAME_ROW_LEFT = 80,
	REPLAY_NAME_ROW_RIGHT = 560,
	REPLAY_NAME_ROW_W = (REPLAY_NAME_ROW_RIGHT - REPLAY_NAME_ROW_LEFT),

	REPLAY_NAME_LINE1_TOP = 96,
	REPLAY_NAME_GLYPH_SPACING = 24,
	REPLAY_NAME_GLYPH_LEFT = REPLAY_NAME_ROW_LEFT,
	REPLAY_NAME_SCORE_SPACING = 16,
	REPLAY_NAME_SCORE_W = (
		((T3_REPLAY_USER_SCORE_DISPLAY_DIGITS - 1) *
		 REPLAY_NAME_SCORE_SPACING) +
		REPLAY_REGI_GLYPH_W
	),
	REPLAY_NAME_SCORE_LEFT = (
		REPLAY_NAME_ROW_RIGHT - REPLAY_NAME_SCORE_W
	),

	REPLAY_NAME_LINE_GAP = 32,
	REPLAY_NAME_LINE2_TOP = (
		REPLAY_NAME_LINE1_TOP +
		REPLAY_REGI_GLYPH_H +
		REPLAY_NAME_LINE_GAP
	),
	REPLAY_NAME_DATE_LEFT = REPLAY_NAME_ROW_LEFT,
	REPLAY_NAME_DATE_SPACING = 20,
	REPLAY_NAME_STAGE_SPACING = 24,
	REPLAY_NAME_STAGE_MAX_W = (
		REPLAY_NAME_STAGE_SPACING + REPLAY_REGI_GLYPH_W
	),
	REPLAY_NAME_STAGE_LEFT = (
		REPLAY_NAME_ROW_RIGHT - REPLAY_NAME_STAGE_MAX_W
	),
	REPLAY_NAME_PLAYCHAR_W = (6 * GLYPH_FULL_W),
	REPLAY_NAME_PLAYCHAR_LEFT = (
		REPLAY_NAME_ROW_RIGHT - REPLAY_REGI_GLYPH_W - 16 -
		REPLAY_NAME_PLAYCHAR_W
	),
	REPLAY_NAME_PLAYCHAR_TOP = (
		REPLAY_NAME_LINE2_TOP +
		((REPLAY_REGI_GLYPH_H - GLYPH_H) / 2)
	),
	REPLAY_NAME_RANK_LEFT = 344,

	REPLAY_SAVE_DIALOG_LEFT = 23,
	REPLAY_SAVE_DIALOG_TOP = 10,
	REPLAY_SAVE_DIALOG_W = 34,
	REPLAY_SAVE_DIALOG_H = 5,
	REPLAY_SAVE_DIALOG_QUESTION_Y = (REPLAY_SAVE_DIALOG_TOP + 1),
	REPLAY_SAVE_DIALOG_CHOICE_Y = (REPLAY_SAVE_DIALOG_TOP + 3),
	REPLAY_SAVE_DIALOG_YES_LEFT = (REPLAY_SAVE_DIALOG_LEFT + 7),
	REPLAY_SAVE_DIALOG_NO_LEFT = (REPLAY_SAVE_DIALOG_LEFT + 19),
	REPLAY_SAVE_DIALOG_PIXEL_LEFT = (
		REPLAY_SAVE_DIALOG_LEFT * GLYPH_HALF_W
	),
	REPLAY_SAVE_DIALOG_PIXEL_TOP = (REPLAY_SAVE_DIALOG_TOP * GLYPH_H),
	REPLAY_SAVE_DIALOG_PIXEL_RIGHT = (
		((REPLAY_SAVE_DIALOG_LEFT + REPLAY_SAVE_DIALOG_W) * GLYPH_HALF_W) - 1
	),
	REPLAY_SAVE_DIALOG_PIXEL_BOTTOM = (
		((REPLAY_SAVE_DIALOG_TOP + REPLAY_SAVE_DIALOG_H) * GLYPH_H) - 1
	),
	REPLAY_SAVE_DIALOG_PIXEL_CENTER = (
		REPLAY_SAVE_DIALOG_PIXEL_LEFT +
		((REPLAY_SAVE_DIALOG_W * GLYPH_HALF_W) / 2)
	),
};

char replay_menu_line[81];
// Keeps OP DGROUP offsets stable across replay-browser text rewrites.
static const char REPLAY_REGI2_BFT[] = "regi2.bft";
static const char REPLAY_REGI1_BFT[] = "regi1.bft";
static const unsigned char REPLAY_ASSET_PF_FN[] = "azinn.dat";
static char REPLAY_BG_PI[10] = "slb1.pi";
struct replay_menu_state_t {
	bool palette_pending;
	uint8_t page_shown;
	bool list_active;
	char offset_pad[9];
};
static replay_menu_state_t replay_menu_state = { false, 0, false, { 1 } };

enum replay_background_t {
	REPLAY_BG_LIST,
	REPLAY_BG_NAME,
};

static void replay_menu_palette_apply(replay_background_t bg);

enum {
	REPLAY_MENU_SELECTED_PALETTE = 12,
	REPLAY_MENU_SELECTED_RED = 0xFF,
	REPLAY_MENU_SELECTED_GREEN = 0xFF,
	REPLAY_MENU_SELECTED_BLUE = 0x00,
};

static char *replay_line_append_cstr(char *p, const char *str)
{
	while(*str != '\0') {
		*p++ = *str++;
	}
	return p;
}

enum replay_practice_text_t {
	RPT_PRACTICE,
	RPT_FINAL,
	RPT_RANK,
	RPT_PRESET,
	RPT_VS_DEFAULT,
	RPT_STORY_NATIVE,
	RPT_P1,
	RPT_CPU,
	RPT_START_STAGE,
	RPT_ROUND,
	RPT_STOCK,
	RPT_VS_RULES,
	RPT_EXTENDS,
	RPT_DASHES,
	RPT_TIMER,
	RPT_INFINITE,
	RPT_SAFETY,
	RPT_ROUND_SPEED,
	RPT_BULLET,
	RPT_SPELL_P1,
	RPT_BOSS,
	RPT_CPU_DAMAGE,
	RPT_ROUND_HEADING,
};

static char *replay_line_append_practice_text(
	char *p, replay_practice_text_t id
)
{
#define P(c) *p++ = (c)
	switch(id) {
	case RPT_PRACTICE:
		P('P'); P('r'); P('a'); P('c'); P('t'); P('i'); P('c'); P('e');
		break;
	case RPT_FINAL:
		P('F'); P('i'); P('n'); P('a'); P('l'); P(':'); P(' ');
		break;
	case RPT_RANK:
		P(' '); P(' '); P('R'); P('a'); P('n'); P('k'); P(':'); P(' ');
		break;
	case RPT_PRESET:
		P('P'); P('r'); P('e'); P('s'); P('e'); P('t'); P(':'); P(' ');
		break;
	case RPT_VS_DEFAULT:
		P('V'); P('S'); P(' '); P('D'); P('e'); P('f'); P('a'); P('u');
		P('l'); P('t');
		break;
	case RPT_STORY_NATIVE:
		P('S'); P('t'); P('o'); P('r'); P('y'); P(' '); P('N'); P('a');
		P('t'); P('i'); P('v'); P('e');
		break;
	case RPT_P1:
		P('P'); P('1'); P(':'); P(' ');
		break;
	case RPT_CPU:
		P(' '); P(' '); P('C'); P('P'); P('U'); P(':'); P(' ');
		break;
	case RPT_START_STAGE:
		P('S'); P('t'); P('a'); P('r'); P('t'); P(':'); P(' '); P('S');
		P('t'); P('a'); P('g'); P('e'); P(' ');
		break;
	case RPT_ROUND:
		P(' '); P(' '); P('R'); P('o'); P('u'); P('n'); P('d'); P(' ');
		break;
	case RPT_STOCK:
		P('S'); P('t'); P('o'); P('c'); P('k'); P(':'); P(' ');
		break;
	case RPT_VS_RULES:
		P('V'); P('S'); P(' '); P('R'); P('u'); P('l'); P('e'); P('s');
		break;
	case RPT_EXTENDS:
		P(' '); P(' '); P('E'); P('x'); P('t'); P('e'); P('n'); P('d');
		P('s'); P(':'); P(' ');
		break;
	case RPT_DASHES:
		P('-'); P('-');
		break;
	case RPT_TIMER:
		P('T'); P('i'); P('m'); P('e'); P('r'); P(':'); P(' ');
		break;
	case RPT_INFINITE:
		P('I'); P('n'); P('f'); P('i'); P('n'); P('i'); P('t'); P('e');
		break;
	case RPT_SAFETY:
		P(' '); P(' '); P('S'); P('a'); P('f'); P('e'); P('t'); P('y');
		P(':'); P(' ');
		break;
	case RPT_ROUND_SPEED:
		P('R'); P('o'); P('u'); P('n'); P('d'); P('S'); P('p'); P(':');
		P(' ');
		break;
	case RPT_BULLET:
		P(' '); P(' '); P('B'); P('u'); P('l'); P('l'); P('e'); P('t');
		P(':'); P(' ');
		break;
	case RPT_SPELL_P1:
		P('S'); P('p'); P('e'); P('l'); P('l'); P(' '); P('P'); P('1');
		P(':'); P(' ');
		break;
	case RPT_BOSS:
		P('B'); P('o'); P('s'); P('s'); P(':'); P(' ');
		break;
	case RPT_CPU_DAMAGE:
		P(' '); P(' '); P('C'); P('P'); P('U'); P(' '); P('D'); P('a');
		P('m'); P('a'); P('g'); P('e'); P(':'); P(' ');
		break;
	case RPT_ROUND_HEADING:
		P('R'); P('d'); P(' '); P('P'); P('1'); P(' '); P('S'); P('c');
		P('o'); P('r'); P('e'); P(' '); P(' '); P('P'); P('2'); P(' ');
		P('S'); P('c'); P('o'); P('r'); P('e'); P(' '); P(' '); P('W');
		break;
	}
	return p;
#undef P
}

static char *replay_line_append_u8_2(char *p, uint8_t value)
{
	*p++ = static_cast<char>('0' + (value / 10));
	*p++ = static_cast<char>('0' + (value % 10));
	return p;
}

static char *replay_line_append_u32(char *p, uint32_t value)
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
		*p++ = digits[i];
	}
	return p;
}

static char *replay_line_append_q4(char *p, uint8_t value)
{
	uint16_t fraction = static_cast<uint16_t>((value & 0x0F) * 625);

	p = replay_line_append_u32(p, (value >> 4));
	*p++ = '.';
	*p++ = static_cast<char>('0' + ((fraction / 1000) % 10));
	*p++ = static_cast<char>('0' + ((fraction / 100) % 10));
	*p++ = static_cast<char>('0' + ((fraction / 10) % 10));
	*p++ = static_cast<char>('0' + (fraction % 10));
	return p;
}

static char *replay_line_append_name(char *p, const char near *name)
{
	char c;

	for(int i = 0; i < T3_REPLAY_USER_NAME_LEN; i++) {
		c = name[i];
		if((c == '\0') || (c == ' ')) {
			c = ' ';
		}
		*p++ = c;
	}
	return p;
}

static char *replay_line_append_date(char *p, uint16_t dos_date)
{
	uint16_t year = static_cast<uint16_t>(1980 + (dos_date >> 9));
	uint8_t month = static_cast<uint8_t>((dos_date >> 5) & 0x0F);
	uint8_t day = static_cast<uint8_t>(dos_date & 0x1F);

	if(dos_date == 0) {
		*p++ = '-';
		return p;
	}
	p = replay_line_append_u8_2(p, month);
	*p++ = '-';
	p = replay_line_append_u8_2(p, day);
	*p++ = '-';
	return replay_line_append_u32(p, year);
}

static char *replay_line_append_unknown_score(char *p)
{
	for(int i = 0; i < T3_REPLAY_USER_SCORE_DISPLAY_DIGITS; i++) {
		*p++ = '-';
	}
	return p;
}

static char *replay_line_append_packed_score(
	char *p, const uint8_t near *score
)
{
	uint8_t value;
	bool seen = false;
	int digit;

	for(digit = (T3_REPLAY_USER_SCORE_DIGITS - 1); digit >= 0; digit--) {
		value = score[digit / 2];
		if(digit & 1) {
			value >>= 4;
		}
		value &= 0x0F;
		if(value < 10) {
			if((value != 0) || seen) {
				seen = true;
				*p++ = static_cast<char>('0' + value);
			} else {
				*p++ = ' ';
			}
		} else {
			seen = true;
			*p++ = '?';
		}
	}
	*p++ = '0';
	return p;
}

static int replay_packed_score_cmp(
	const uint8_t near *left, const uint8_t near *right
)
{
	uint8_t l;
	uint8_t r;
	int digit;

	for(digit = (T3_REPLAY_USER_SCORE_DIGITS - 1); digit >= 0; digit--) {
		l = left[digit / 2];
		r = right[digit / 2];
		if(digit & 1) {
			l >>= 4;
			r >>= 4;
		}
		l &= 0x0F;
		r &= 0x0F;
		if(l != r) {
			return (l - r);
		}
	}
	return 0;
}

static bool replay_menu_summary_valid(void)
{
	return (
		(replay_user_menu_header.summary_flags & T3_REPLAY_USER_SUMMARY_VALID) !=
		0
	);
}

static bool replay_menu_vs(void)
{
	return (replay_user_menu_header.game_mode >= GM_VS);
}

static bool replay_menu_practice(void)
{
	return (
		(replay_user_menu_header.flags & T3_REPLAY_USER_FLAG_PRACTICE) != 0
	);
}

static bool replay_menu_round_summary_valid(void)
{
	return (
		(replay_user_menu_summary_ext.flags & T3_REPLAY_USER_SUMMARY_VALID) !=
		0
	);
}

static replay_user_menu_round_split_t near *replay_menu_round_split(
	uint8_t round
)
{
	uint8_t i;
	uint8_t stage = (
		replay_menu_practice() ?
			replay_user_menu_header.scenario.practice.config.stage :
			T3_REPLAY_USER_ROUND_STAGE_VS
	);
	replay_user_menu_round_split_t near *split;

	if(!replay_menu_round_summary_valid()) {
		return NULL;
	}
	for(i = 0; i < replay_user_menu_summary_ext.round_reached_count; i++) {
		split = &replay_user_menu_summary_ext.round_splits[i];
		if(
			((split->stage_round & 0x0F) == stage) &&
			((split->stage_round >> 4) == round)
		) {
			return split;
		}
	}
	return NULL;
}

static const uint8_t near *replay_menu_list_score(void)
{
	replay_user_menu_round_split_t near *split;
	uint8_t round;

	if(replay_menu_practice()) {
		return replay_user_menu_header.final_score;
	}
	if(replay_menu_vs()) {
		for(round = 2; round != 0xFF; round--) {
			split = replay_menu_round_split(round);
			if(split != NULL) {
				if(replay_user_menu_header.game_mode == GM_VS_1P_CPU) {
					return split->score_p1;
				}
				if(replay_user_menu_header.game_mode == GM_VS_1P_2P) {
					if(replay_user_menu_header.final_winner == 0) {
						return split->score_p1;
					}
					if(replay_user_menu_header.final_winner == 1) {
						return split->score_p2;
					}
				}
				if(replay_packed_score_cmp(split->score_p2, split->score_p1) > 0) {
					return split->score_p2;
				}
				return split->score_p1;
			}
		}
	}
	return replay_user_menu_header.final_score;
}

static uint8_t replay_menu_stage_opponent(uint8_t stage)
{
	if(
		replay_menu_summary_valid() &&
		(replay_user_menu_header.scenario.story.stage_opponents[stage] != 0)
	) {
		return replay_user_menu_header.scenario.story.stage_opponents[stage];
	}
	return replay_user_menu_snapshot.story_opponents[stage];
}

static char *replay_line_append_story_lives(char *p)
{
	if(
		replay_menu_summary_valid() &&
		(replay_user_menu_header.final_story_lives != T3_REPLAY_USER_SUMMARY_UNKNOWN)
	) {
		return replay_line_append_u32(p, replay_user_menu_header.final_story_lives);
	}
	*p++ = '-';
	*p++ = '-';
	return p;
}

static const char *replay_user_end_reason_name(uint8_t end_reason)
{
	if(replay_menu_vs()) {
		return "Vs Mode";
	}
	switch(end_reason) {
	case RUER_COMPLETE:
		return "Clear";
	case RUER_GAME_OVER:
		return "Game Over";
	case RUER_MENU_RETURN:
		return "Menu Return";
	case RUER_INPUT_END:
		return "Input End";
	case RUER_PARTIAL:
		return "Partial";
	case RUER_ERROR:
		return "Error";
	default:
		return "None";
	}
}

static const char *replay_rank_name(uint8_t rank)
{
	switch(rank) {
	case RANK_EASY:
		return "Easy";
	case RANK_NORMAL:
		return "Normal";
	case RANK_HARD:
		return "Hard";
	case RANK_LUNATIC:
		return "Lunatic";
	default:
		return "?";
	}
}

static char replay_rank_initial(uint8_t rank)
{
	switch(rank) {
	case RANK_EASY:
		return 'E';
	case RANK_NORMAL:
		return 'N';
	case RANK_HARD:
		return 'H';
	case RANK_LUNATIC:
		return 'L';
	default:
		return '?';
	}
}

static const char *replay_game_mode_name(uint8_t game_mode)
{
	switch(game_mode) {
	case GM_STORY:
		return "Story";
	case GM_VS_1P_CPU:
		return "VS 1P-CPU";
	case GM_VS_1P_2P:
		return "VS 1P-2P";
	case GM_VS_CPU_CPU:
		return "VS CPU-CPU";
	default:
		return "Unknown";
	}
}

static char *replay_line_append_autofire(char *p, bool both_players)
{
	*p++ = 'A';
	*p++ = 'F';
	*p++ = ' ';
	*p++ = 'P';
	*p++ = '1';
	*p++ = ':';
	*p++ = ' ';
	if(replay_user_menu_header.autofire & 0x01) {
		*p++ = 'O';
		*p++ = 'n';
	} else {
		*p++ = 'O';
		*p++ = 'f';
		*p++ = 'f';
	}
	if(both_players) {
		*p++ = ' ';
		*p++ = ' ';
		*p++ = 'P';
		*p++ = '2';
		*p++ = ':';
		*p++ = ' ';
		if(replay_user_menu_header.autofire & 0x02) {
			*p++ = 'O';
			*p++ = 'n';
		} else {
			*p++ = 'O';
			*p++ = 'f';
			*p++ = 'f';
		}
	}
	return p;
}

static uint8_t replay_playchar_id(uint8_t paletted)
{
	if(paletted == 0) {
		return 0xFF;
	}
	return ((paletted - 1) / 2);
}

static const char *replay_playchar_name(uint8_t paletted)
{
	switch(replay_playchar_id(paletted)) {
	case PLAYCHAR_REIMU:
		return "Reimu";
	case PLAYCHAR_MIMA:
		return "Mima";
	case PLAYCHAR_MARISA:
		return "Marisa";
	case PLAYCHAR_ELLEN:
		return "Ellen";
	case PLAYCHAR_KOTOHIME:
		return "Kotohime";
	case PLAYCHAR_KANA:
		return "Kana";
	case PLAYCHAR_RIKAKO:
		return "Rikako";
	case PLAYCHAR_CHIYURI:
		return "Chiyuri";
	case PLAYCHAR_YUMEMI:
		return "Yumemi";
	default:
		return "?";
	}
}

static char *replay_line_append_playchar_pair(char *p, uint8_t paletted)
{
	char first = '?';
	char second = '?';

	switch(replay_playchar_id(paletted)) {
	case PLAYCHAR_REIMU:
		first = 'R';
		second = 'e';
		break;
	case PLAYCHAR_MIMA:
		first = 'M';
		second = 'i';
		break;
	case PLAYCHAR_MARISA:
		first = 'M';
		second = 'a';
		break;
	case PLAYCHAR_ELLEN:
		first = 'E';
		second = 'l';
		break;
	case PLAYCHAR_KOTOHIME:
		first = 'K';
		second = 'o';
		break;
	case PLAYCHAR_KANA:
		first = 'K';
		second = 'a';
		break;
	case PLAYCHAR_RIKAKO:
		first = 'R';
		second = 'i';
		break;
	case PLAYCHAR_CHIYURI:
		first = 'C';
		second = 'h';
		break;
	case PLAYCHAR_YUMEMI:
		first = 'Y';
		second = 'u';
		break;
	default:
		break;
	}
	*p++ = first;
	*p++ = second;
	return p;
}

static char *replay_line_append_stage(char *p, uint8_t stage)
{
	if(stage == STAGE_ALL) {
		return replay_line_append_cstr(p, "ALL");
	}
	if(stage == STAGE_NONE) {
		return replay_line_append_cstr(p, "--");
	}
	return replay_line_append_u32(p, (stage + 1));
}

static char *replay_line_append_final_stage_mark(char *p)
{
	uint8_t stage;

	if(replay_menu_practice()) {
		*p++ = 'P';
		return p;
	}
	if(replay_menu_vs()) {
		return replay_line_append_cstr(p, "VS");
	}
	if(replay_user_menu_header.end_reason == RUER_COMPLETE) {
		return replay_line_append_cstr(p, "ALL");
	} else if(
		replay_menu_summary_valid() &&
		(replay_user_menu_header.stage_reached_count != 0)
	) {
		stage = replay_user_menu_header.stage_reached_count;
		if(stage > T3_REPLAY_USER_STAGE_COUNT) {
			stage = T3_REPLAY_USER_STAGE_COUNT;
		}
		*p++ = static_cast<char>('0' + stage);
	} else {
		*p++ = '-';
	}
	return p;
}

static void replay_menu_slot_line_put(
	uint8_t slot, uint8_t sel, unsigned int y, bool active, bool clear
)
{
	char *p;
	bool has_replay;
	tram_atrb2 atrb;

	has_replay = replay_user_read_slot_for_menu(slot);
	atrb = (((slot == sel) && active) ? TX_YELLOW : TX_WHITE);
	if(clear) {
		replay_menu_span_clear(REPLAY_MENU_LIST_LEFT, y, REPLAY_MENU_LIST_W);
	}
	if(menu_font) {
		replay_font_slot_line_put(
			slot, sel, y, active, has_replay
		);
		return;
	}

	p = replay_menu_line;
	*p++ = (((slot == sel) && active) ? '>' : ' ');
	p = replay_line_append_u8_2(p, slot);
	*p++ = ' ';
	if(has_replay) {
		p = replay_line_append_playchar_pair(
			p, replay_user_menu_header.playchar_p1
		);
		*p++ = ' ';
		*p++ = replay_rank_initial(replay_user_menu_header.rank);
		*p++ = ' ';
		p = replay_line_append_name(p, replay_user_menu_header.name);
		*p++ = ' ';
		if(replay_menu_summary_valid()) {
			p = replay_line_append_packed_score(p, replay_menu_list_score());
		} else {
			p = replay_line_append_unknown_score(p);
		}
		*p++ = ' ';
		p = replay_line_append_final_stage_mark(p);
	} else {
		p = replay_line_append_cstr(p, "none");
	}
	*p = '\0';
	replay_menu_line_put(REPLAY_MENU_LIST_LEFT, y, atrb);
}

static void replay_menu_slot_line_put(uint8_t slot, uint8_t sel, unsigned int y)
{
	replay_menu_slot_line_put(slot, sel, y, true, true);
}

static void replay_menu_detail_line_put(unsigned int y, char *p)
{
	*p = '\0';
	replay_menu_line_put(REPLAY_MENU_DETAIL_LEFT, y, TX_WHITE);
}

static void replay_menu_detail_line_put(
	unsigned int y, char *p, tram_atrb2 atrb
)
{
	*p = '\0';
	replay_menu_line_put(REPLAY_MENU_DETAIL_LEFT, y, atrb);
}

#if defined(TH03_REPLAY_DEVTOOLS)
static void replay_menu_font_diagnostics_put(void)
{
	unsigned y = (replay_menu_practice() ? 14 : 12);

	replay_font_diagnostic_line_put(
		y, RFD_SAMPLES, replay_user_menu_header.sample_count
	);
	replay_font_diagnostic_line_put(
		(y + 1), RFD_FRAMES, replay_user_menu_header.final_frame_count
	);
	replay_font_diagnostic_line_put(
		(y + 2), RFD_BYTES, replay_user_menu_header.input_size
	);
	replay_font_diagnostic_line_put(
		(y + 3), RFD_RNG, replay_user_menu_header.resident_rand
	);
}
#endif

static char *replay_line_append_round_winner(char *p, uint8_t route_winner)
{
	switch(route_winner & 0x0F) {
	case 0:
		*p++ = '1';
		break;
	case 1:
		*p++ = '2';
		break;
	default:
		*p++ = '-';
		break;
	}
	return p;
}

static void replay_menu_detail_put_empty(uint8_t slot, bool clear)
{
	char *p;

	if(clear) {
		for(uint8_t y = REPLAY_MENU_DETAIL_Y; y < REPLAY_MENU_FOOT_Y; y++) {
			replay_menu_span_clear(
				REPLAY_MENU_DETAIL_LEFT, y, REPLAY_MENU_DETAIL_W
			);
		}
	}
	if(menu_font) {
		replay_font_detail_empty_put(slot);
		return;
	}

	p = replay_menu_line;
	p = replay_line_append_cstr(p, "Slot ");
	p = replay_line_append_u8_2(p, slot);
	p = replay_line_append_cstr(p, ": none");
	replay_menu_detail_line_put(REPLAY_MENU_DETAIL_Y, p);

	p = replay_menu_line;
	p = replay_line_append_cstr(p, "No replay header found.");
	replay_menu_detail_line_put(REPLAY_MENU_DETAIL_Y + 2, p);
}

static void replay_menu_detail_put_vs(void)
{
	char *p;
	uint8_t round;
	replay_user_menu_round_split_t near *split;

	p = replay_menu_line;
	p = replay_line_append_cstr(p, (
		(replay_user_menu_header.game_mode == GM_VS_1P_CPU) ? "P1: " : "Win: "
	));
	if(replay_menu_summary_valid()) {
		p = replay_line_append_packed_score(p, replay_menu_list_score());
	} else {
		p = replay_line_append_unknown_score(p);
	}
	replay_menu_detail_line_put(REPLAY_MENU_DETAIL_Y + 2, p);

	p = replay_menu_line;
	p = replay_line_append_cstr(p, "Mode: ");
	p = replay_line_append_cstr(p, replay_game_mode_name(
		replay_user_menu_header.game_mode
	));
	p = replay_line_append_cstr(p, "       Rank: ");
	p = replay_line_append_cstr(p, replay_rank_name(replay_user_menu_header.rank));
	replay_menu_detail_line_put(REPLAY_MENU_DETAIL_Y + 3, p);

	p = replay_menu_line;
	p = replay_line_append_autofire(p, true);
	replay_menu_detail_line_put(REPLAY_MENU_DETAIL_Y + 4, p);

	p = replay_menu_line;
	p = replay_line_append_cstr(p, "P1: ");
	p = replay_line_append_cstr(
		p, replay_playchar_name(replay_user_menu_header.playchar_p1)
	);
	if(replay_user_menu_header.is_cpu_p1) {
		p = replay_line_append_cstr(p, " CPU");
	}
	replay_menu_detail_line_put(REPLAY_MENU_DETAIL_Y + 5, p);

	p = replay_menu_line;
	p = replay_line_append_cstr(p, "P2: ");
	p = replay_line_append_cstr(
		p, replay_playchar_name(replay_user_menu_header.playchar_p2)
	);
	if(replay_user_menu_header.is_cpu_p2) {
		p = replay_line_append_cstr(p, " CPU");
	}
	replay_menu_detail_line_put(REPLAY_MENU_DETAIL_Y + 6, p);

	p = replay_menu_line;
	p = replay_line_append_cstr(p, "Rd P1 Score  P2 Score  W");
	replay_menu_detail_line_put(REPLAY_MENU_DETAIL_Y + 8, p);

	for(round = 0; round < 3; round++) {
		p = replay_menu_line;
		*p++ = ' ';
		*p++ = static_cast<char>('1' + round);
		*p++ = ' ';
		split = replay_menu_round_split(round);
		if(split != NULL) {
			p = replay_line_append_packed_score(p, split->score_p1);
			*p++ = ' ';
			p = replay_line_append_packed_score(p, split->score_p2);
			*p++ = ' ';
			p = replay_line_append_round_winner(p, split->route_winner);
		} else {
			p = replay_line_append_unknown_score(p);
			*p++ = ' ';
			p = replay_line_append_unknown_score(p);
			*p++ = ' ';
			*p++ = '-';
		}
		replay_menu_detail_line_put((REPLAY_MENU_DETAIL_Y + 9 + round), p);
	}
}

static void replay_menu_detail_put_practice(void)
{
	char *p;
	uint8_t attempt;
	uint8_t attempts = (
		(replay_user_menu_header.scenario.practice.config.stock ==
		 T3_PRACTICE_STOCK_VS_RULES) ?
			3 : (replay_user_menu_header.scenario.practice.config.stock + 1)
	);
	uint8_t round;
	replay_user_menu_round_split_t near *split;

	p = replay_menu_line;
	p = replay_line_append_practice_text(p, RPT_FINAL);
	if(replay_menu_summary_valid()) {
		p = replay_line_append_packed_score(
			p, replay_user_menu_header.final_score
		);
	} else {
		p = replay_line_append_unknown_score(p);
	}
	p = replay_line_append_practice_text(p, RPT_RANK);
	p = replay_line_append_cstr(p, replay_rank_name(replay_user_menu_header.rank));
	replay_menu_detail_line_put(REPLAY_MENU_DETAIL_Y + 2, p);

	p = replay_menu_line;
	p = replay_line_append_practice_text(p, RPT_PRESET);
	p = replay_line_append_practice_text(p, (
		(replay_user_menu_header.scenario.practice.config.preset ==
		 PRACTICE_PRESET_VS_DEFAULT) ?
			RPT_VS_DEFAULT : RPT_STORY_NATIVE
	));
	*p++ = ' ';
	*p++ = ' ';
	p = replay_line_append_autofire(p, true);
	replay_menu_detail_line_put(REPLAY_MENU_DETAIL_Y + 3, p);

	p = replay_menu_line;
	p = replay_line_append_practice_text(p, RPT_P1);
	p = replay_line_append_cstr(
		p, replay_playchar_name(replay_user_menu_header.playchar_p1)
	);
	p = replay_line_append_practice_text(p, RPT_CPU);
	p = replay_line_append_cstr(
		p, replay_playchar_name(replay_user_menu_header.playchar_p2)
	);
	replay_menu_detail_line_put(REPLAY_MENU_DETAIL_Y + 4, p);

	p = replay_menu_line;
	p = replay_line_append_practice_text(p, RPT_START_STAGE);
	p = replay_line_append_u32(
		p, (replay_user_menu_header.scenario.practice.config.stage + 1)
	);
	p = replay_line_append_practice_text(p, RPT_ROUND);
	p = replay_line_append_u32(
		p, (replay_user_menu_header.scenario.practice.config.round + 1)
	);
	if(replay_user_menu_header.scenario.practice.config.round == 5) {
		*p++ = '+';
	}
	replay_menu_detail_line_put(REPLAY_MENU_DETAIL_Y + 5, p);

	p = replay_menu_line;
	p = replay_line_append_practice_text(p, RPT_STOCK);
	if(
		replay_user_menu_header.scenario.practice.config.stock ==
		T3_PRACTICE_STOCK_VS_RULES
	) {
		p = replay_line_append_practice_text(p, RPT_VS_RULES);
	} else {
		p = replay_line_append_u32(
			p, replay_user_menu_header.scenario.practice.config.stock
		);
	}
	p = replay_line_append_practice_text(p, RPT_EXTENDS);
	if(
		replay_user_menu_header.scenario.practice.config.preset ==
		PRACTICE_PRESET_VS_DEFAULT
	) {
		p = replay_line_append_practice_text(p, RPT_DASHES);
	} else {
		p = replay_line_append_u32(
			p, replay_user_menu_header.scenario.practice.config.extends_gained
		);
	}
	replay_menu_detail_line_put(REPLAY_MENU_DETAIL_Y + 6, p);

	p = replay_menu_line;
	p = replay_line_append_practice_text(p, RPT_TIMER);
	if(
		replay_user_menu_header.scenario.practice.config.cpu_timer ==
		PRACTICE_CPU_TIMER_VS_DEFAULT
	) {
		p = replay_line_append_practice_text(p, RPT_VS_DEFAULT);
	} else if(
		replay_user_menu_header.scenario.practice.config.cpu_timer ==
		PRACTICE_CPU_TIMER_STORY_NATIVE
	) {
		p = replay_line_append_practice_text(p, RPT_STORY_NATIVE);
	} else {
		p = replay_line_append_practice_text(p, RPT_INFINITE);
	}
	p = replay_line_append_practice_text(p, RPT_SAFETY);
	p = replay_line_append_u32(
		p,
		replay_user_menu_header.scenario.practice.config.initial_cpu_safety_frames
	);
	replay_menu_detail_line_put(REPLAY_MENU_DETAIL_Y + 7, p);

	p = replay_menu_line;
	p = replay_line_append_practice_text(p, RPT_ROUND_SPEED);
	p = replay_line_append_q4(
		p, replay_user_menu_header.scenario.practice.config.round_speed
	);
	p = replay_line_append_practice_text(p, RPT_BULLET);
	p = replay_line_append_q4(
		p, replay_user_menu_header.scenario.practice.config.bullet_speed
	);
	replay_menu_detail_line_put(REPLAY_MENU_DETAIL_Y + 8, p);

	p = replay_menu_line;
	p = replay_line_append_practice_text(p, RPT_SPELL_P1);
	p = replay_line_append_u32(
		p, replay_user_menu_header.scenario.practice.config.p1_spell
	);
	p = replay_line_append_practice_text(p, RPT_CPU);
	p = replay_line_append_u32(
		p, replay_user_menu_header.scenario.practice.config.cpu_spell
	);
	replay_menu_detail_line_put(REPLAY_MENU_DETAIL_Y + 9, p);

	p = replay_menu_line;
	p = replay_line_append_practice_text(p, RPT_BOSS);
	p = replay_line_append_u32(
		p, (replay_user_menu_header.scenario.practice.config.boss_level + 1)
	);
	p = replay_line_append_practice_text(p, RPT_CPU_DAMAGE);
	p = replay_line_append_u32(
		p, replay_user_menu_header.scenario.practice.config.cpu_damage
	);
	replay_menu_detail_line_put(REPLAY_MENU_DETAIL_Y + 10, p);

	p = replay_menu_line;
	p = replay_line_append_practice_text(p, RPT_ROUND_HEADING);
	replay_menu_detail_line_put(REPLAY_MENU_DETAIL_Y + 11, p);

	for(attempt = 0; attempt < 5; attempt++) {
		round = static_cast<uint8_t>(
			replay_user_menu_header.scenario.practice.config.round + attempt
		);
		p = replay_menu_line;
		if(round < 9) {
			*p++ = ' ';
		}
		p = replay_line_append_u32(p, (round + 1));
		*p++ = ' ';
		split = replay_menu_round_split(round);
		if((attempt < attempts) && (split != NULL)) {
			p = replay_line_append_packed_score(p, split->score_p1);
			*p++ = ' ';
			p = replay_line_append_packed_score(p, split->score_p2);
			*p++ = ' ';
			p = replay_line_append_round_winner(p, split->route_winner);
		} else {
			p = replay_line_append_unknown_score(p);
			*p++ = ' ';
			p = replay_line_append_unknown_score(p);
			*p++ = ' ';
			*p++ = '-';
		}
		replay_menu_detail_line_put(
			(REPLAY_MENU_DETAIL_Y + 12 + attempt), p
		);
	}

	p = replay_menu_line;
	p = replay_line_append_cstr(p, "Smpl: ");
	p = replay_line_append_u32(p, replay_user_menu_header.sample_count);
	p = replay_line_append_cstr(p, " Frm: ");
	p = replay_line_append_u32(p, replay_user_menu_header.final_frame_count);
	replay_menu_detail_line_put(REPLAY_MENU_DETAIL_Y + 17, p);

	p = replay_menu_line;
	p = replay_line_append_cstr(p, "Bytes: ");
	p = replay_line_append_u32(p, replay_user_menu_header.input_size);
	p = replay_line_append_cstr(p, "  RNG: ");
	p = replay_line_append_u32(p, replay_user_menu_header.resident_rand);
	replay_menu_detail_line_put(REPLAY_MENU_DETAIL_Y + 18, p);
}

static void replay_menu_detail_put_story(uint8_t stage_sel, bool stage_focus)
{
	char *p;
	uint8_t stage;
	tram_atrb2 atrb;

	p = replay_menu_line;
	p = replay_line_append_cstr(p, "Final: ");
	if(replay_menu_summary_valid()) {
		p = replay_line_append_packed_score(p, replay_user_menu_header.final_score);
	} else {
		p = replay_line_append_unknown_score(p);
	}
	p = replay_line_append_cstr(p, "  Lives: ");
	p = replay_line_append_story_lives(p);
	replay_menu_detail_line_put(REPLAY_MENU_DETAIL_Y + 2, p);

	p = replay_menu_line;
	p = replay_line_append_cstr(p, "Mode: ");
	p = replay_line_append_cstr(p, replay_game_mode_name(
		replay_user_menu_header.game_mode
	));
	p = replay_line_append_cstr(p, "       Rank: ");
	p = replay_line_append_cstr(p, replay_rank_name(replay_user_menu_header.rank));
	replay_menu_detail_line_put(REPLAY_MENU_DETAIL_Y + 3, p);

	p = replay_menu_line;
	p = replay_line_append_cstr(p, "Start: S");
	p = replay_line_append_stage(p, replay_user_menu_header.story_stage);
	*p++ = ' ';
	*p++ = ' ';
	*p++ = ' ';
	p = replay_line_append_autofire(p, false);
	replay_menu_detail_line_put(REPLAY_MENU_DETAIL_Y + 4, p);

	p = replay_menu_line;
	p = replay_line_append_cstr(p, "Player: ");
	p = replay_line_append_cstr(
		p, replay_playchar_name(replay_user_menu_header.playchar_p1)
	);
	if(replay_user_menu_header.is_cpu_p1) {
		p = replay_line_append_cstr(p, " CPU");
	}
	replay_menu_detail_line_put(REPLAY_MENU_DETAIL_Y + 5, p);

	p = replay_menu_line;
	p = replay_line_append_cstr(p, "St Op Score");
	replay_menu_detail_line_put(REPLAY_MENU_DETAIL_Y + 7, p);

	for(stage = 0; stage < T3_REPLAY_USER_STAGE_COUNT; stage++) {
		p = replay_menu_line;
		atrb = (
			(stage_focus && (stage == stage_sel)) ? TX_YELLOW : TX_WHITE
		);
		*p++ = ((stage_focus && (stage == stage_sel)) ? '>' : ' ');
		*p++ = static_cast<char>('1' + stage);
		*p++ = ' ';
		#if defined(TH03_REPLAY_DEVTOOLS)
			p = replay_line_append_playchar_pair(
				p, replay_menu_stage_opponent(stage)
			);
		#else
			*p++ = '-';
		#endif
		*p++ = ' ';
		if(
			replay_menu_summary_valid() &&
			(stage < replay_user_menu_header.stage_reached_count)
		) {
			p = replay_line_append_packed_score(
				p, replay_user_menu_header.scenario.story.stage_scores[stage]
			);
		} else {
			p = replay_line_append_unknown_score(p);
		}
		replay_menu_detail_line_put(
			(REPLAY_MENU_DETAIL_Y + 8 + stage), p, atrb
		);
	}

	p = replay_menu_line;
	p = replay_line_append_cstr(p, "Smpl: ");
	p = replay_line_append_u32(p, replay_user_menu_header.sample_count);
	p = replay_line_append_cstr(p, " Frm: ");
	p = replay_line_append_u32(p, replay_user_menu_header.final_frame_count);
	replay_menu_detail_line_put(REPLAY_MENU_DETAIL_Y + 17, p);

	p = replay_menu_line;
	p = replay_line_append_cstr(p, "Bytes: ");
	p = replay_line_append_u32(p, replay_user_menu_header.input_size);
	p = replay_line_append_cstr(p, "  RNG: ");
	p = replay_line_append_u32(p, replay_user_menu_header.resident_rand);
	replay_menu_detail_line_put(REPLAY_MENU_DETAIL_Y + 18, p);
}

static void replay_menu_detail_put(
	uint8_t slot, uint8_t stage_sel, bool stage_focus,
	uint8_t detail_page, bool clear
)
{
	char *p;

	if(clear) {
		for(uint8_t y = REPLAY_MENU_DETAIL_Y; y < REPLAY_MENU_FOOT_Y; y++) {
			replay_menu_span_clear(
				REPLAY_MENU_DETAIL_LEFT, y, REPLAY_MENU_DETAIL_W
			);
		}
	}

	if(!replay_user_read_slot_for_menu(slot)) {
		replay_menu_detail_put_empty(slot, clear);
		return;
	}
	if(menu_font) {
		#if defined(TH03_REPLAY_DEVTOOLS)
			replay_font_detail_put(
				slot, stage_sel, stage_focus, detail_page, true
			);
			replay_menu_font_diagnostics_put();
		#else
			replay_font_detail_put(
				slot, stage_sel, stage_focus, detail_page, false
			);
		#endif
		return;
	}

	p = replay_menu_line;
	p = replay_line_append_cstr(p, "Slot ");
	p = replay_line_append_u8_2(p, slot);
	p = replay_line_append_cstr(p, "  ");
	if(replay_menu_practice()) {
		p = replay_line_append_practice_text(p, RPT_PRACTICE);
	} else {
		p = replay_line_append_cstr(
			p, replay_user_end_reason_name(replay_user_menu_header.end_reason)
		);
	}
	replay_menu_detail_line_put(REPLAY_MENU_DETAIL_Y, p);

	p = replay_menu_line;
	*p++ = 'N'; *p++ = 'a'; *p++ = 'm'; *p++ = 'e'; *p++ = ':'; *p++ = ' ';
	p = replay_line_append_name(p, replay_user_menu_header.name);
	*p++ = ' '; *p++ = ' '; *p++ = ' '; *p++ = ' ';
	*p++ = 'D'; *p++ = 'a'; *p++ = 't'; *p++ = 'e';
	*p++ = ':'; *p++ = ' ';
	p = replay_line_append_date(p, replay_user_menu_header.dos_date);
	replay_menu_detail_line_put(REPLAY_MENU_DETAIL_Y + 1, p);

	if(replay_menu_practice()) {
		replay_menu_detail_put_practice();
	} else if(replay_menu_vs()) {
		replay_menu_detail_put_vs();
	} else {
		replay_menu_detail_put_story(stage_sel, stage_focus);
	}
}

static uint8_t replay_menu_render_begin(void)
{
	uint8_t page_drawn = 0;

	#if defined(TH03_REPLAY_DEVTOOLS)
		text_clear();
	#endif
	if(menu_font) {
		page_drawn = (1 - replay_menu_state.page_shown);
	}
	graph_accesspage(page_drawn);
	pi_put_8(0, 0, 0);
	return page_drawn;
}

static void replay_menu_render_end(uint8_t page_drawn)
{
	if(menu_font) {
		vsync_wait();
	}
	if(replay_menu_state.palette_pending) {
		replay_menu_palette_apply(REPLAY_BG_LIST);
		replay_menu_state.palette_pending = false;
	}
	if(menu_font) {
		graph_showpage(page_drawn);
		replay_menu_state.page_shown = page_drawn;
	}
	graph_accesspage(page_drawn);
}

static void replay_menu_render(uint8_t sel, uint8_t top)
{
	uint8_t slot;
	uint8_t page_drawn;
	unsigned int line;
	bool clear = !menu_font;

	page_drawn = replay_menu_render_begin();
	if(clear) {
		for(line = REPLAY_MENU_DETAIL_Y; line < REPLAY_MENU_FOOT_Y; line++) {
			replay_menu_span_clear(
				REPLAY_MENU_DETAIL_LEFT, line, REPLAY_MENU_DETAIL_W
			);
		}
	}
	replay_font_columns_put(clear);
	for(line = 0; line < REPLAY_MENU_VISIBLE; line++) {
		slot = (top + line);
		replay_menu_slot_line_put(
			slot, sel,
			(REPLAY_MENU_LIST_Y + line), true, clear
		);
	}
	replay_font_page_put(sel, REPLAY_MENU_FOOT_Y);
	replay_menu_render_end(page_drawn);
}

// The proportional-font compositor redraws each hidden page from the PI and
// must not call the legacy page-1-to-page-0 row restorer for this footer.
// Preserve the protected OP contribution after removing those two calls.
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"

static void replay_menu_detail_render(
	uint8_t slot, uint8_t checkpoint_sel, bool checkpoint_focus,
	uint8_t detail_page
)
{
	uint8_t page_drawn;
	bool clear = !menu_font;

	page_drawn = replay_menu_render_begin();
	replay_menu_detail_put(
		slot, checkpoint_sel, checkpoint_focus, detail_page, clear
	);
	if(clear) {
		replay_menu_span_clear(0, REPLAY_MENU_FOOT_Y, text_width());
	}
	replay_menu_render_end(page_drawn);
}

static void replay_menu_practice_settings_show(
	uint8_t slot, uint8_t checkpoint_sel, uint8_t detail_page
)
{
	uint8_t page_drawn;
	bool clear = !menu_font;

	page_drawn = replay_menu_render_begin();
	replay_menu_detail_put(slot, checkpoint_sel, false, detail_page, clear);
	if(menu_font) {
		replay_font_practice_settings_modal_put();
	}
	replay_menu_render_end(page_drawn);
	do {
		input_mode_interface();
		frame_delay(1);
	} while(input_sp != INPUT_NONE);
	do {
		input_mode_interface();
		frame_delay(1);
	} while(input_sp == INPUT_NONE);
	replay_menu_detail_render(slot, checkpoint_sel, false, detail_page);
}

static void fullscreen_menu_resources_clear(void)
{
	for(int i = 0; i < CDG_SLOT_COUNT; i++) {
		cdg_free(i);
	}
	super_free();
	text_clear();
}

static void replay_menu_background_load(replay_background_t bg)
{
	if(bg == REPLAY_BG_NAME) {
		REPLAY_BG_PI[4] = 'b';
		REPLAY_BG_PI[5] = '.';
		REPLAY_BG_PI[6] = 'p';
		REPLAY_BG_PI[7] = 'i';
		REPLAY_BG_PI[8] = '\0';
	} else {
		REPLAY_BG_PI[4] = '.';
		REPLAY_BG_PI[5] = 'p';
		REPLAY_BG_PI[6] = 'i';
		REPLAY_BG_PI[7] = '\0';
	}
	pfend();
	pfstart(REPLAY_ASSET_PF_FN);
	pi_load(0, REPLAY_BG_PI);
	pfend();
	pfstart(reinterpret_cast<const unsigned char *>(OP_AND_END_PF_FN));
}

static void replay_menu_palette_apply(replay_background_t bg)
{
	if(bg == REPLAY_BG_LIST) {
		palette_set_all(pi_headers[0].palette);
		palette_set(
			REPLAY_MENU_SELECTED_PALETTE,
			REPLAY_MENU_SELECTED_RED,
			REPLAY_MENU_SELECTED_GREEN,
			REPLAY_MENU_SELECTED_BLUE
		);
		palette_show();
	} else {
		pi_palette_apply(0);
	}
}

static void replay_menu_background_put(
	replay_background_t bg, bool start_black, bool keep
)
{
	replay_menu_background_load(bg);
	PaletteTone = (start_black ? 0 : 100);
	replay_menu_palette_apply(bg);
	graph_accesspage(0);
	pi_put_8(0, 0, 0);
	graph_accesspage(1);
	pi_put_8(0, 0, 0);
	if(!keep) {
		pi_free(0);
	}
	graph_showpage(0);
	graph_accesspage(0);
	replay_menu_state.page_shown = 0;
}

static void replay_menu_browser_init(bool start_black)
{
	fullscreen_menu_resources_clear();
	replay_menu_background_load(REPLAY_BG_LIST);
	PaletteTone = (start_black ? 0 : 100);
	replay_menu_state.page_shown = 0;
	replay_menu_state.list_active = true;
	if(menu_font) {
		replay_menu_state.palette_pending = true;
	} else {
		replay_menu_palette_apply(REPLAY_BG_LIST);
		graph_showpage(0);
		replay_menu_state.palette_pending = false;
	}
	graph_accesspage(0);
}

static void replay_save_input_release(void)
{
	do {
		input_mode_interface();
		frame_delay(1);
	} while(input_sp != INPUT_NONE);
}

static void replay_save_date_set(void)
{
	uint16_t year;
	uint8_t month;
	uint8_t day;

	_asm {
		mov ah, 2Ah
		int 21h
		mov year, cx
		mov month, dh
		mov day, dl
	}
	if(
		(year < 1980) ||
		(month < 1) || (month > 12) ||
		(day < 1) || (day > 31)
	) {
		replay_user_menu_header.dos_date = 0;
		return;
	}
	replay_user_menu_header.dos_date = static_cast<uint16_t>(
		((year - 1980) << 9) |
		(static_cast<uint16_t>(month) << 5) |
		static_cast<uint16_t>(day)
	);
}

static bool replay_pending_header_write(void)
{
	if(!file_append(REPLAY_FALLBACK_FN)) {
		return false;
	}
	file_seek(0, SEEK_SET);
	if(
		file_write(
			&replay_user_menu_header, sizeof(replay_user_menu_header)
		) == 0
	) {
		file_close();
		return false;
	}
	file_close();
	return replay_file_commit_process();
}

static void replay_backup_fn_set(char far *fn)
{
	fn[0] = 'R'; fn[1] = 'E'; fn[2] = 'P'; fn[3] = 'L'; fn[4] = 'A';
	fn[5] = 'Y'; fn[6] = '\\'; fn[7] = 'T'; fn[8] = 'H'; fn[9] = '3';
	fn[10] = 'O'; fn[11] = 'L'; fn[12] = 'D'; fn[13] = '.';
	fn[14] = 'R'; fn[15] = 'P'; fn[16] = 'Y'; fn[17] = '\0';
}

static bool replay_save_to_slot(uint8_t slot, bool occupied)
{
	char backup_fn[18];

	if(!replay_user_read_for_menu(REPLAY_FALLBACK_FN)) {
		return false;
	}
	replay_dir_create();
	replay_user_slot_fn_set(slot);
	replay_backup_fn_set(backup_fn);
	if(occupied) {
		replay_file_delete_commit(backup_fn);
		if(!replay_file_rename(REPLAY_SLOT_FN, backup_fn)) {
			return false;
		}
		if(!replay_file_commit_process()) {
			replay_file_rename(backup_fn, REPLAY_SLOT_FN);
			(void)replay_file_commit_process();
			return false;
		}
	}
	if(!replay_file_rename(REPLAY_FALLBACK_FN, REPLAY_SLOT_FN)) {
		if(occupied) {
			replay_file_rename(backup_fn, REPLAY_SLOT_FN);
			(void)replay_file_commit_process();
		}
		return false;
	}
	if(!replay_file_commit_process()) {
		replay_file_rename(REPLAY_SLOT_FN, REPLAY_FALLBACK_FN);
		if(occupied) {
			replay_file_rename(backup_fn, REPLAY_SLOT_FN);
		}
		(void)replay_file_commit_process();
		return false;
	}
	if(occupied) {
		replay_file_delete_commit(backup_fn);
	}
	(void)replay_user_index_slot_write(slot);
	return true;
}

static void replay_save_dialog_frame_put(void)
{
	if(menu_font) {
		graph_copy_page(1 - replay_menu_state.page_shown);
	}
	graph_accesspage(replay_menu_state.page_shown);
	grcg_setcolor(GC_RMW, 0);
	grcg_boxfill(
		REPLAY_SAVE_DIALOG_PIXEL_LEFT, REPLAY_SAVE_DIALOG_PIXEL_TOP,
		REPLAY_SAVE_DIALOG_PIXEL_RIGHT, REPLAY_SAVE_DIALOG_PIXEL_BOTTOM
	);
	grcg_off();
	text_clear();
}

enum replay_save_question_t {
	RSQ_SAVE,
	RSQ_OVERWRITE,
	RSQ_QUIT,
};

enum replay_save_answer_t {
	RSA_NO,
	RSA_YES,
	RSA_CANCEL,
};

static replay_save_answer_t replay_save_yes_no(
	replay_save_question_t question, uint8_t slot, bool default_yes
)
{
	bool yes = default_yes;
	input_t input_prev;
	replay_save_answer_t answer;

	replay_save_dialog_frame_put();
	replay_font_save_dialog_put(question, slot, yes);
	replay_save_input_release();
	input_prev = INPUT_NONE;
	while(1) {
		input_mode_interface();
		if(input_prev == INPUT_NONE) {
			if(input_sp & (INPUT_UP | INPUT_DOWN | INPUT_LEFT | INPUT_RIGHT)) {
				yes = !yes;
				replay_font_save_dialog_put(question, slot, yes);
			}
			if(input_sp & (INPUT_OK | INPUT_SHOT)) {
				answer = (yes ? RSA_YES : RSA_NO);
				break;
			}
			if(input_sp & INPUT_CANCEL) {
				answer = RSA_CANCEL;
				break;
			}
		}
		input_prev = input_sp;
		frame_delay(1);
	}
	if(menu_font) {
		replay_menu_render_end(1 - replay_menu_state.page_shown);
	}
	return answer;
}

static bool replay_save_quit_confirm(void)
{
	return (
		replay_save_yes_no(RSQ_QUIT, 0, false) == RSA_YES
	);
}

static char replay_name_ascii(int regi)
{
	if((regi >= REGI_A) && (regi <= REGI_M)) {
		return static_cast<char>('A' + (regi - REGI_A));
	}
	if((regi >= REGI_N) && (regi <= REGI_Z)) {
		return static_cast<char>('N' + (regi - REGI_N));
	}
	if((regi >= REGI_0) && (regi <= REGI_9)) {
		return static_cast<char>('0' + (regi - REGI_0));
	}
	switch(regi) {
	case REGI_PERIOD:      return '.';
	case REGI_COMMA:       return ',';
	case REGI_EXCLAMATION: return '!';
	case REGI_QUESTION:    return '?';
	case REGI_SP:          return ' ';
	default:               return '\0';
	}
}

static int replay_name_regi(char c)
{
	if((c >= 'a') && (c <= 'z')) {
		c = static_cast<char>('A' + (c - 'a'));
	}
	if((c >= 'A') && (c <= 'M')) {
		return (REGI_A + (c - 'A'));
	}
	if((c >= 'N') && (c <= 'Z')) {
		return (REGI_N + (c - 'N'));
	}
	if((c >= '0') && (c <= '9')) {
		return (REGI_0 + (c - '0'));
	}
	switch(c) {
	case '.': return REGI_PERIOD;
	case ',': return REGI_COMMA;
	case '!': return REGI_EXCLAMATION;
	case '?': return REGI_QUESTION;
	case '-': return REGI_BLANK_1;
	default:  return -1;
	}
}

static void replay_name_regi_patterns_patch(void)
{
	uint8_t far *pattern;
	uint8_t far *mask;
	uint8_t far *blue;
	uint8_t far *red;
	uint8_t far *green;
	uint8_t far *intensity;
	uint8_t fill;
	int patnum;
	int i;
	int y;

	for(patnum = 0; patnum < REGI_COUNT; patnum++) {
		pattern = reinterpret_cast<uint8_t far *>(MK_FP(
			super_patdata[patnum], 0
		));
		blue = (pattern + (REPLAY_REGI_PLANE_SIZE * PATTERN_BLUE));
		red = (pattern + (REPLAY_REGI_PLANE_SIZE * PATTERN_RED));
		green = (pattern + (REPLAY_REGI_PLANE_SIZE * PATTERN_GREEN));
		intensity = (pattern + (REPLAY_REGI_PLANE_SIZE * PATTERN_INTEN));
		for(i = 0; i < REPLAY_REGI_PLANE_SIZE; i++) {
			fill = static_cast<uint8_t>(
				red[i] & intensity[i] & ~(blue[i] | green[i])
			);
			blue[i] |= fill;
			green[i] |= fill;
		}

		pattern = reinterpret_cast<uint8_t far *>(MK_FP(
			super_patdata[patnum + REGI_COUNT], 0
		));
		blue = (pattern + (REPLAY_REGI_PLANE_SIZE * PATTERN_BLUE));
		red = (pattern + (REPLAY_REGI_PLANE_SIZE * PATTERN_RED));
		green = (pattern + (REPLAY_REGI_PLANE_SIZE * PATTERN_GREEN));
		intensity = (pattern + (REPLAY_REGI_PLANE_SIZE * PATTERN_INTEN));
		for(i = 0; i < REPLAY_REGI_PLANE_SIZE; i++) {
			fill = static_cast<uint8_t>(
				blue[i] & red[i] & green[i] & intensity[i]
			);
			blue[i] &= ~fill;
			red[i] &= ~fill;
			green[i] &= ~fill;
			intensity[i] &= ~fill;
		}
	}

	pattern = reinterpret_cast<uint8_t far *>(MK_FP(
		super_patdata[REGI_BLANK_1], 0
	));
	mask = pattern;
	blue = (pattern + (REPLAY_REGI_PLANE_SIZE * PATTERN_BLUE));
	red = (pattern + (REPLAY_REGI_PLANE_SIZE * PATTERN_RED));
	green = (pattern + (REPLAY_REGI_PLANE_SIZE * PATTERN_GREEN));
	intensity = (pattern + (REPLAY_REGI_PLANE_SIZE * PATTERN_INTEN));
	for(y = REPLAY_REGI_DASH_TOP; y <= REPLAY_REGI_DASH_BOTTOM; y++) {
		fill = (
			((y == REPLAY_REGI_DASH_TOP) ||
			 (y == REPLAY_REGI_DASH_BOTTOM)) ? 3 : 15
		);
		i = ((y * (REPLAY_REGI_GLYPH_W / 8)) + 1);
		for(int x = 0; x < 2; x++, i++) {
			mask[i] = 0xFF;
			blue[i] = ((fill & 1) ? 0xFF : 0x00);
			red[i] = ((fill & 2) ? 0xFF : 0x00);
			green[i] = ((fill & 4) ? 0xFF : 0x00);
			intensity[i] = ((fill & 8) ? 0xFF : 0x00);
		}
	}
}

static void replay_name_summary_row_unput(
	screen_x_t left, screen_y_t top, pixel_t w
)
{
	vram_offset_t row_p = vram_offset_shift(left, top);
	pixel_t row;

	for(row = 0; row < REPLAY_REGI_GLYPH_H; row++) {
		vram_word_amount_t col;
		int p;
		for(
			col = 0, p = row_p;
			col < (w >> 4);
			col++, p += 2
		) {
			Planar<dots16_t> p16;
			graph_accesspage(1); VRAM_SNAP_PLANAR(p16, p, 16);
			graph_accesspage(0); VRAM_PUT_PLANAR(p, p16, 16);
		}
		row_p += ROW_SIZE;
	}
}

static void replay_name_summary_unput(void)
{
	replay_name_summary_row_unput(
		REPLAY_NAME_ROW_LEFT, REPLAY_NAME_LINE1_TOP,
		REPLAY_NAME_ROW_W
	);
	replay_name_summary_row_unput(
		REPLAY_NAME_ROW_LEFT, REPLAY_NAME_LINE2_TOP,
		REPLAY_NAME_ROW_W
	);
}

static void replay_name_glyphs_put(
	screen_x_t left, screen_y_t top, const char near *str,
	unsigned int len, pixel_t spacing
)
{
	int regi;

	for(unsigned int i = 0; i < len; i++) {
		regi = replay_name_regi(str[i]);
		if(regi >= 0) {
			super_put(left, top, regi);
		}
		left += spacing;
	}
}

static char *replay_line_append_shiftjis(
	char *p, shiftjis_kanji_t codepoint
)
{
	*p++ = static_cast<char>(codepoint >> 8);
	*p++ = static_cast<char>(codepoint);
	return p;
}

static void replay_name_playchar_put(void)
{
	char *p = replay_menu_line;
	pixel_t text_w;

	switch(replay_playchar_id(replay_user_menu_header.playchar_p1)) {
	case PLAYCHAR_REIMU:
		p = replay_line_append_shiftjis(p, 0xE8CB);
		p = replay_line_append_shiftjis(p, 0x96B2);
		text_w = (2 * GLYPH_FULL_W);
		break;
	case PLAYCHAR_MIMA:
		p = replay_line_append_shiftjis(p, 0x96A3);
		p = replay_line_append_shiftjis(p, 0x9682);
		text_w = (2 * GLYPH_FULL_W);
		break;
	case PLAYCHAR_MARISA:
		p = replay_line_append_shiftjis(p, 0x9682);
		p = replay_line_append_shiftjis(p, 0x979D);
		p = replay_line_append_shiftjis(p, 0x8DB9);
		text_w = (3 * GLYPH_FULL_W);
		break;
	case PLAYCHAR_ELLEN:
		p = replay_line_append_shiftjis(p, 0x8347);
		p = replay_line_append_shiftjis(p, 0x838C);
		p = replay_line_append_shiftjis(p, 0x8393);
		text_w = (3 * GLYPH_FULL_W);
		break;
	case PLAYCHAR_KOTOHIME:
		p = replay_line_append_shiftjis(p, 0x8FAC);
		p = replay_line_append_shiftjis(p, 0x9365);
		p = replay_line_append_shiftjis(p, 0x9550);
		text_w = (3 * GLYPH_FULL_W);
		break;
	case PLAYCHAR_KANA:
		p = replay_line_append_shiftjis(p, 0x834A);
		p = replay_line_append_shiftjis(p, 0x8369);
		text_w = (2 * GLYPH_FULL_W);
		break;
	case PLAYCHAR_RIKAKO:
		p = replay_line_append_shiftjis(p, 0x979D);
		p = replay_line_append_shiftjis(p, 0x8D81);
		p = replay_line_append_shiftjis(p, 0x8E71);
		text_w = (3 * GLYPH_FULL_W);
		break;
	case PLAYCHAR_CHIYURI:
		p = replay_line_append_shiftjis(p, 0x82BF);
		p = replay_line_append_shiftjis(p, 0x82E4);
		p = replay_line_append_shiftjis(p, 0x82E8);
		text_w = (3 * GLYPH_FULL_W);
		break;
	case PLAYCHAR_YUMEMI:
		p = replay_line_append_shiftjis(p, 0x96B2);
		p = replay_line_append_shiftjis(p, 0x94FC);
		text_w = (2 * GLYPH_FULL_W);
		break;
	default:
		*p++ = '?';
		text_w = GLYPH_HALF_W;
		break;
	}
	*p = '\0';
	graph_putsa_fx(
		(
			REPLAY_NAME_PLAYCHAR_LEFT +
			((REPLAY_NAME_PLAYCHAR_W - text_w) / 2)
		),
		REPLAY_NAME_PLAYCHAR_TOP,
		(V_WHITE | FX_WEIGHT_BOLD),
		reinterpret_cast<const shiftjis_t *>(replay_menu_line)
	);
}

static void replay_name_stage_put(void)
{
	char *p;
	unsigned int len;
	pixel_t w;

	if(
		!replay_menu_vs() &&
		(replay_user_menu_header.end_reason == RUER_COMPLETE)
	) {
		super_put(
			(REPLAY_NAME_ROW_RIGHT - REPLAY_REGI_GLYPH_W),
			REPLAY_NAME_LINE2_TOP,
			REGI_ALL
		);
		return;
	}

	p = replay_menu_line;
	p = replay_line_append_final_stage_mark(p);
	len = (p - replay_menu_line);
	w = (
		((len - 1) * REPLAY_NAME_STAGE_SPACING) +
		REPLAY_REGI_GLYPH_W
	);
	replay_name_glyphs_put(
		(REPLAY_NAME_ROW_RIGHT - w), REPLAY_NAME_LINE2_TOP,
		replay_menu_line, len, REPLAY_NAME_STAGE_SPACING
	);
}

static void replay_name_summary_put(void)
{
	char *p;

	replay_name_summary_unput();

	replay_name_glyphs_put(
		REPLAY_NAME_GLYPH_LEFT, REPLAY_NAME_LINE1_TOP,
		replay_user_menu_header.name, T3_REPLAY_USER_NAME_LEN,
		REPLAY_NAME_GLYPH_SPACING
	);

	p = replay_menu_line;
	if(replay_menu_summary_valid()) {
		p = replay_line_append_packed_score(p, replay_menu_list_score());
	} else {
		p = replay_line_append_unknown_score(p);
	}
	replay_name_glyphs_put(
		REPLAY_NAME_SCORE_LEFT, REPLAY_NAME_LINE1_TOP,
		replay_menu_line, (p - replay_menu_line),
		REPLAY_NAME_SCORE_SPACING
	);

	p = replay_menu_line;
	p = replay_line_append_date(p, replay_user_menu_header.dos_date);
	replay_name_glyphs_put(
		REPLAY_NAME_DATE_LEFT, REPLAY_NAME_LINE2_TOP,
		replay_menu_line, (p - replay_menu_line),
		REPLAY_NAME_DATE_SPACING
	);

	replay_menu_line[0] = replay_rank_initial(replay_user_menu_header.rank);
	replay_name_glyphs_put(
		REPLAY_NAME_RANK_LEFT, REPLAY_NAME_LINE2_TOP,
		replay_menu_line, 1, REPLAY_NAME_GLYPH_SPACING
	);

	replay_name_playchar_put();
	replay_name_stage_put();

}

static void replay_name_item_put(int regi, bool selected)
{
	int patnum = (selected ? (regi + REGI_COUNT) : regi);
	screen_x_t left = static_cast<screen_x_t>(
		REPLAY_REGI_GRID_LEFT +
		((regi % REPLAY_REGI_GLYPHS_PER_ROW) * REPLAY_REGI_GLYPH_W)
	);
	screen_y_t top = static_cast<screen_y_t>(
		REPLAY_REGI_GRID_TOP +
		((regi / REPLAY_REGI_GLYPHS_PER_ROW) * REPLAY_REGI_ROW_SPACING)
	);

	super_put(left, top, patnum);
	if((regi % REPLAY_REGI_GLYPHS_PER_ROW) == REGI_SP) {
		super_put((left + REPLAY_REGI_GLYPH_W), top, (patnum + 1));
	}
}

static void replay_name_grid_put(int selected)
{
	int regi;

	for(regi = 0; regi < REGI_ALL; regi++) {
		if(
			(regi == REGI_BLANK_1) || (regi == REGI_SP_last) ||
			(regi == REGI_BLANK_2) || (regi == REGI_BS_last) ||
			(regi == REGI_END_last)
		) {
			continue;
		}
		replay_name_item_put(regi, (regi == selected));
	}
}

static void replay_name_screen_put(int selected, bool fade_in)
{
	fullscreen_menu_resources_clear();
	replay_menu_state.list_active = false;
	super_entry_bfnt(REPLAY_REGI2_BFT);
	super_entry_bfnt(REPLAY_REGI1_BFT);
	replay_name_regi_patterns_patch();
	replay_menu_background_put(REPLAY_BG_NAME, fade_in, false);
	replay_name_summary_put();
	replay_name_grid_put(selected);
	if(fade_in) {
		palette_black_in(1);
	}
}

static void replay_name_backspace(uint8_t& name_len)
{
	if(name_len != 0) {
		name_len--;
		replay_user_menu_header.name[name_len] = ' ';
	}
}

static bool replay_name_menu(uint8_t& name_len, bool fade_in)
{
	int regi = REGI_A;
	int previous;
	char c;
	input_t input_prev;

	replay_name_screen_put(regi, fade_in);
	replay_save_input_release();
	input_prev = INPUT_NONE;
	while(1) {
		input_mode_interface();
		if(input_prev == INPUT_NONE) {
			previous = regi;
			if((input_sp & INPUT_UP) && (regi != REGI_QUESTION)) {
				regi -= 16;
				if(regi < 0) {
					regi += REGI_ALL;
				}
			}
			if((input_sp & INPUT_DOWN) && (regi != REGI_QUESTION)) {
				regi += 16;
				if(regi >= REGI_ALL) {
					regi -= REGI_ALL;
				}
			}
			if(input_sp & INPUT_LEFT) {
				if((regi % 16) == 0) {
					regi += REGI_SP;
				} else if((regi == REGI_BS) || (regi == REGI_SP)) {
					regi -= 2;
				} else {
					regi--;
				}
			}
			if(input_sp & INPUT_RIGHT) {
				if((regi % 16) == REGI_SP) {
					regi -= REGI_SP;
				} else if((regi == REGI_M) || (regi == REGI_Z)) {
					regi += 2;
				} else {
					regi++;
				}
			}
			if(previous != regi) {
				replay_name_item_put(previous, false);
				replay_name_item_put(regi, true);
			}
			if(input_sp & (INPUT_OK | INPUT_SHOT)) {
				if(regi == REGI_END) {
					return true;
				}
				if(regi == REGI_BS) {
					replay_name_backspace(name_len);
				} else if(name_len < T3_REPLAY_USER_NAME_LEN) {
					c = replay_name_ascii(regi);
					if(c != '\0') {
						replay_user_menu_header.name[name_len] = c;
						name_len++;
						if(name_len == T3_REPLAY_USER_NAME_LEN) {
							replay_name_item_put(regi, false);
							regi = REGI_END;
							replay_name_item_put(regi, true);
						}
					}
				}
				replay_name_summary_put();
			}
			if(input_sp & INPUT_BOMB) {
				replay_name_backspace(name_len);
				replay_name_summary_put();
			}
			if(input_sp & INPUT_CANCEL) {
				if(replay_save_quit_confirm()) {
					return false;
				}
				if(!menu_font) {
					replay_name_screen_put(regi, false);
				}
				replay_save_input_release();
				input_prev = INPUT_NONE;
				continue;
			}
		}
		input_prev = input_sp;
		frame_delay(1);
	}
}

static void replay_save_complete_put(void)
{
	replay_font_save_complete_put();
}

static void replay_save_slot_render(
	uint8_t sel, uint8_t top, bool complete
)
{
	uint8_t slot;
	uint8_t page_drawn;
	unsigned int line;
	bool clear = !menu_font;

	page_drawn = replay_menu_render_begin();
	replay_font_columns_put(clear);
	for(line = 0; line < REPLAY_MENU_VISIBLE; line++) {
		slot = (top + line);
		replay_menu_slot_line_put(
			slot, sel,
			(REPLAY_MENU_LIST_Y + line), true, clear
		);
	}
	replay_font_page_put(sel, REPLAY_MENU_FOOT_Y);
	if(complete) {
		replay_save_complete_put();
	}
	replay_menu_render_end(page_drawn);
}

static void replay_save_complete_wait(void)
{
	input_wait_for_change(0);
}

static void replay_save_screen_exit(bool free_background)
{
	PaletteTone = 0;
	palette_show();
	replay_menu_state.list_active = false;
	if(free_background) {
		pi_free(0);
	}
	fullscreen_menu_resources_clear();
	graph_accesspage(0);
	graph_clear();
	graph_accesspage(1);
	graph_clear();
	graph_showpage(0);
	graph_accesspage(0);
}

static bool replay_save_slot_menu(void)
{
	uint8_t sel = 0;
	uint8_t top = 0;
	bool occupied;
	input_t input_prev;
	replay_save_answer_t answer;

	replay_menu_browser_init(false);
	replay_save_slot_render(sel, top, false);
	replay_save_input_release();
	input_prev = INPUT_NONE;
	while(1) {
		input_mode_interface();
		if(input_prev == INPUT_NONE) {
			if(input_sp & INPUT_UP) {
				ring_dec_range(sel, 0, (T3_REPLAY_USER_SLOT_COUNT - 1));
				top = replay_font_page_top(sel);
				replay_save_slot_render(sel, top, false);
			}
			if(input_sp & INPUT_DOWN) {
				ring_inc_range(sel, 0, (T3_REPLAY_USER_SLOT_COUNT - 1));
				top = replay_font_page_top(sel);
				replay_save_slot_render(sel, top, false);
			}
			if(input_sp & INPUT_LEFT) {
				sel = replay_font_page_left(sel);
				top = replay_font_page_top(sel);
				replay_save_slot_render(sel, top, false);
			}
			if(input_sp & INPUT_RIGHT) {
				sel = replay_font_page_right(sel);
				top = replay_font_page_top(sel);
				replay_save_slot_render(sel, top, false);
			}
			if(input_sp & (INPUT_OK | INPUT_SHOT)) {
				replay_user_slot_fn_set(sel);
				occupied = (file_exist(REPLAY_SLOT_FN) != 0);
				if(occupied) {
					answer = replay_save_yes_no(RSQ_OVERWRITE, sel, false);
					if(answer == RSA_CANCEL) {
						if(replay_save_quit_confirm()) {
							return false;
						}
						answer = RSA_NO;
					}
					if(answer != RSA_YES) {
						replay_save_slot_render(sel, top, false);
						replay_save_input_release();
						input_prev = INPUT_NONE;
						continue;
					}
				}
				if(replay_save_to_slot(sel, occupied)) {
					replay_save_slot_render(sel, top, true);
					replay_save_complete_wait();
					return true;
				}
				replay_save_slot_render(sel, top, false);
				replay_save_input_release();
				input_prev = INPUT_NONE;
				continue;
			}
			if(input_sp & INPUT_CANCEL) {
				if(replay_save_quit_confirm()) {
					return false;
				}
				replay_save_slot_render(sel, top, false);
				replay_save_input_release();
				input_prev = INPUT_NONE;
				continue;
			}
		}
		input_prev = input_sp;
		frame_delay(1);
	}
}

static void replay_save_pending(bool prompt)
{
	t3pix_scene_set(T3PIX_SCENE_REPLAY_SAVE);
	uint8_t name_len = 0;
	int i;
	replay_save_answer_t answer;

	(void)replay_accel_pending_merge(REPLAY_FALLBACK_FN);
	if(!replay_user_read_for_menu(REPLAY_FALLBACK_FN)) {
		return;
	}
	if(prompt) {
		fullscreen_menu_resources_clear();
		replay_menu_state.list_active = false;
		replay_menu_background_put(REPLAY_BG_LIST, true, true);
		palette_black_in(1);
		while(1) {
			answer = replay_save_yes_no(RSQ_SAVE, 0, true);
			if(answer == RSA_YES) {
				break;
			}
			if(
				(answer == RSA_NO) ||
				((answer == RSA_CANCEL) && replay_save_quit_confirm())
			) {
				replay_file_delete_commit(REPLAY_FALLBACK_FN);
				replay_accel_temps_delete();
				replay_save_screen_exit(true);
				return;
			}
		}
		pi_free(0);
	}
	for(i = 0; i < T3_REPLAY_USER_NAME_LEN; i++) {
		replay_user_menu_header.name[i] = ' ';
	}
	replay_save_date_set();
	if(!replay_name_menu(name_len, !prompt)) {
		replay_file_delete_commit(REPLAY_FALLBACK_FN);
		replay_accel_temps_delete();
		replay_save_screen_exit(false);
		return;
	}
	if(!replay_pending_header_write()) {
		replay_save_screen_exit(false);
		return;
	}
	if(!replay_save_slot_menu()) {
		replay_file_delete_commit(REPLAY_FALLBACK_FN);
	}
	replay_accel_temps_delete();
	replay_save_screen_exit(true);
}

bool near replay_menu(void)
{
	t3pix_scene_set(T3PIX_SCENE_REPLAY_BROWSER);
	uint8_t sel = replay_user_first_used_slot();
	uint8_t top = replay_font_page_top(sel);
	uint8_t checkpoint_sel = 0;
	uint8_t checkpoint_count;
	uint8_t detail_page = RDP_SPLITS;
	bool detail_view = false;
	bool checkpoint_focus = true;
	uint32_t sample_count;
	uint32_t global_frame;
	uint32_t input_size;
	input_t input_prev = input_sp;

	replay_menu_browser_init(true);
	replay_menu_render(sel, top);
	palette_black_in(1);

	while(1) {
		input_mode_interface();
		if(input_prev == INPUT_NONE) {
			if(detail_view) {
				if(input_sp & (INPUT_CANCEL | INPUT_BOMB)) {
					detail_view = false;
					replay_menu_render(sel, top);
				} else if(input_sp & INPUT_LEFT) {
					if(replay_menu_practice() && !checkpoint_focus) {
						// The settings command is already the leftmost focus target.
					} else if(
						replay_menu_practice() &&
						(detail_page == RDP_SPLITS)
					) {
						checkpoint_focus = false;
						replay_menu_detail_render(
							sel, checkpoint_sel, checkpoint_focus, detail_page
						);
					} else if(detail_page > RDP_SPLITS) {
						detail_page--;
						replay_menu_detail_render(
							sel, checkpoint_sel, checkpoint_focus, detail_page
						);
					}
				} else if(input_sp & INPUT_RIGHT) {
					if(replay_menu_practice() && !checkpoint_focus) {
						checkpoint_focus = true;
						detail_page = RDP_SPLITS;
						replay_menu_detail_render(
							sel, checkpoint_sel, checkpoint_focus, detail_page
						);
					} else if(detail_page < RDP_TIMERS) {
						detail_page++;
						replay_menu_detail_render(
							sel, checkpoint_sel, checkpoint_focus, detail_page
						);
					}
				} else if(
					(input_sp & INPUT_UP) && checkpoint_focus &&
					(detail_page != RDP_CLEAR_BONUSES)
				) {
					if(checkpoint_count != 0) {
						if(checkpoint_sel == 0) {
							checkpoint_sel = (checkpoint_count - 1);
						} else {
							checkpoint_sel--;
						}
						replay_menu_detail_render(
							sel, checkpoint_sel, checkpoint_focus, detail_page
						);
					}
				} else if(
					(input_sp & INPUT_DOWN) && checkpoint_focus &&
					(detail_page != RDP_CLEAR_BONUSES)
				) {
					if(checkpoint_count != 0) {
						checkpoint_sel++;
						if(checkpoint_sel >= checkpoint_count) {
							checkpoint_sel = 0;
						}
						replay_menu_detail_render(
							sel, checkpoint_sel, checkpoint_focus, detail_page
						);
					}
				} else if(input_sp & (INPUT_OK | INPUT_SHOT)) {
					if(replay_menu_practice() && !checkpoint_focus) {
						replay_menu_practice_settings_show(
							sel, checkpoint_sel, detail_page
						);
					} else if(detail_page != RDP_CLEAR_BONUSES)
					if(replay_user_read_slot_for_menu(sel)) {
						if(
							replay_user_version_has_round_state(
								replay_user_menu_header.version
							) ||
							(!replay_menu_vs() && !replay_menu_practice())
						) {
#if defined(TH03_REPLAY_DEVTOOLS)
							replay_checkpoint_force_preroll_set(
								(
									*reinterpret_cast<uint8_t far *>(
										MK_FP(0, KEYGROUP_14)
									) & K14_SHIFT
								) != 0
							);
#else
							replay_checkpoint_force_preroll_set(false);
#endif
							checkpoint_sel = (
								replay_checkpoint_anchor_for_menu(checkpoint_sel)
							);
							if(replay_user_checkpoint_read_for_menu(
								checkpoint_sel,
								&sample_count, &global_frame, &input_size
							)) {
								replay_user_restore_resident_from_menu();
								replay_resident_handoff_set(
									T3_REPLAY_RES_MODE_USER_PLAYBACK
								);
								replay_resident_handoff_slot_set(sel);
								replay_resident_handoff_u32_set(
									T3_REPLAY_RES_SAMPLE_COUNT_INDEX, sample_count
								);
								replay_resident_handoff_u32_set(
									T3_REPLAY_RES_GLOBAL_FRAME_INDEX, global_frame
								);
								replay_resident_handoff_u32_set(
									T3_REPLAY_RES_INPUT_SIZE_INDEX, input_size
								);
								replay_checkpoint_handoff_set(checkpoint_sel);
								pi_free(0);
								return switch_to_mainl(false);
							}
						} else {
							replay_user_restore_resident_from_menu();
							replay_resident_handoff_set(
								T3_REPLAY_RES_MODE_USER_PLAYBACK
							);
							replay_resident_handoff_slot_set(sel);
							pi_free(0);
							return switch_to_mainl(false);
						}
					}
				}
			} else {
				if(input_sp & INPUT_UP) {
					ring_dec_range(sel, 0, (T3_REPLAY_USER_SLOT_COUNT - 1));
					top = replay_font_page_top(sel);
					replay_menu_render(sel, top);
				} else if(input_sp & INPUT_DOWN) {
					ring_inc_range(sel, 0, (T3_REPLAY_USER_SLOT_COUNT - 1));
					top = replay_font_page_top(sel);
					replay_menu_render(sel, top);
				} else if(input_sp & INPUT_LEFT) {
					sel = replay_font_page_left(sel);
					top = replay_font_page_top(sel);
					replay_menu_render(sel, top);
				} else if(input_sp & INPUT_RIGHT) {
					sel = replay_font_page_right(sel);
					top = replay_font_page_top(sel);
					replay_menu_render(sel, top);
				} else if(input_sp & (INPUT_OK | INPUT_SHOT)) {
					if(replay_user_read_slot_for_menu(sel)) {
						detail_view = true;
						checkpoint_sel = 0;
						checkpoint_focus = true;
						detail_page = RDP_SPLITS;
						checkpoint_count = (
							replay_user_checkpoint_count_for_menu()
						);
						replay_menu_detail_render(
							sel, checkpoint_sel, checkpoint_focus, detail_page
						);
					}
				} else if(input_sp & INPUT_CANCEL) {
					text_clear();
					pi_free(0);
					return true;
				}
			}
		}
		input_prev = input_sp;
		frame_delay(1);
	}
}

// Preserve all later OP_01_TEXT entry points after simplifying tab navigation.
#pragma codestring "\x90\x90\x90\x90\x90\x90"

/// The menu
/// --------

// Must be non-`const` for data ordering reasons. Declared at global scope
// because
// 1) the same [COMMAND_QUIT] string is used in both the main and Option menu,
//    and
// 2) some of those are unused, which points toward ZUN having declared them at
//    global scope as well.
char COMMAND_STORY[] = "Start";
char COMMAND_VS[] = "VS Start";
char COMMAND_MUSICROOM[] = "Music room";
char COMMAND_REGIST_VIEW[] = "HiScore";
char COMMAND_OPTION[] = "Option";
char COMMAND_REPLAY[] = "Replay";
char COMMAND_QUIT[] = "Quit";

char LABEL_RANK[] = "Rank";
char LABEL_MUSIC[6] = "Music";
char LABEL_SFX[8] = "SFX";
char LABEL_KEYCONFIG[] = "KeyConfig";

// ZUN bloat: Unused, but looks like a gaiji version of the space string below.
// Since that is the only call to text_putsa() in this binary, using this one
// would have also removed the need to link in that function.
char UNUSED_SPACES[5] = { g_SP, g_SP, g_SP, g_SP, '\0' };

char VALUE_EASY[] = "Easy";
char VALUE_NORMAL[] = "Normal";
char VALUE_HARD[] = "Hard";
char VALUE_LUNATIC[] = "Lunatic";

char VALUE_OFF[8] = "Off";
char VALUE_FM[8] = "FM (86)";
char VALUE_MIDI[13] = "MIDI (SC-88)";

// The initial names for the three input modes? Unused in the final game.
// Replay mod: Reuses the five-byte VALUE_TYPE_1 slot to keep the original
// data layout. Cell 87h is cell 3Fh without Option's stray i column.
char VALUE_ON[5] = "On";
char VALUE_TYPE_2[] = { g_str_3(gp_Type), gp_2, '\0' };
char VALUE_TYPE_3[] = { g_str_3(gp_Type), gp_3, '\0' };

char VALUE_KEY_KEY[] = "Key vs Key";
char VALUE_JOY_KEY[] = "Joy vs Key";
char VALUE_KEY_JOY[] = "Key vs Joy";

// Globals
// -------

int8_t menu_sel = 0;
bool quit = false;
bool in_main = true;

// ZUN bloat: Should be function-level statics.
bool main_input_allowed;
bool option_input_allowed;

int8_t in_option; // ACTUAL TYPE: bool
static int8_t padding; // ZUN bloat; reused for the initial Option language
menu_put_func_t menu_put;

static char title_credit_line[48];
// Keep OP's initialized-data offsets after removing the GDC error path.
static char gdc_frequency_error_offset_padding[
	(sizeof(
		ERROR_GDC_5MHZ_1 "\0" ERROR_GDC_5MHZ_2 "\0" ERROR_GDC_5MHZ_3
	) - 49)
] = { 0 };
// -------

#define TITLE_CREDIT_PAIR(index, left, right) \
	pairs[index] = static_cast<uint16_t>((left) | ((right) << 8))
#define TITLE_CREDIT_QUAD(index, value) \
	(*reinterpret_cast<uint32_t near *>( \
		&title_credit_line[(index) * sizeof(uint32_t)] \
	) = (value))

static void near title_credit_put(void)
{
	enum {
		TRAM_RIGHT = 80,
		LINE1_LEN = 24,
		LINE2_LEN = 39,
	};
	uint16_t near *pairs = reinterpret_cast<uint16_t near *>(title_credit_line);

	TITLE_CREDIT_PAIR( 0, 'T', 'h');
	TITLE_CREDIT_PAIR( 1, 'e', ' ');
	TITLE_CREDIT_PAIR( 2, 'P', 'o');
	TITLE_CREDIT_PAIR( 3, 'D', 'D');
	TITLE_CREDIT_PAIR( 4, ' ', 'A');
	TITLE_CREDIT_PAIR( 5, 'r', 'r');
	TITLE_CREDIT_PAIR( 6, 'a', 'n');
	TITLE_CREDIT_PAIR( 7, 'g', 'e');
	TITLE_CREDIT_PAIR( 8, ' ', 'P');
	TITLE_CREDIT_PAIR( 9, 'r', 'o');
	TITLE_CREDIT_PAIR(10, 'j', 'e');
	TITLE_CREDIT_PAIR(11, 'c', 't');
	title_credit_line[LINE1_LEN] = '\0';
	title_credit_line_put(title_credit_line, LINE1_LEN, 0);

	TITLE_CREDIT_QUAD(0, 0x6C706552UL); // "Repl"
	TITLE_CREDIT_QUAD(1, 0x50207961UL); // "ay P"
	TITLE_CREDIT_QUAD(2, 0x68637461UL); // "atch"
	TITLE_CREDIT_QUAD(3, 0x2E307620UL); // " v0."
	TITLE_CREDIT_QUAD(4, 0x38312E34UL); // "4.18"
	TITLE_CREDIT_QUAD(5, 0x20796220UL); // " by "
	TITLE_CREDIT_QUAD(6, 0x69726843UL); // "Chri"
	TITLE_CREDIT_QUAD(7, 0x61697473UL); // "stia"
	TITLE_CREDIT_QUAD(8, 0x7A41206EUL); // "n Az"
	TITLE_CREDIT_QUAD(9, 0x006E6E69UL); // "inn\0"
	TITLE_CREDIT_QUAD(10, 0x00000000UL);
	TITLE_CREDIT_QUAD(11, 0x00000000UL);
	title_credit_line_put(title_credit_line, LINE2_LEN, 1);
}

#undef TITLE_CREDIT_PAIR
#undef TITLE_CREDIT_QUAD

void pascal near main_choice_put(int sel, tram_atrb2 atrb)
{
	title_choice_graphics_unput(sel, BOX_MAIN_RIGHT);
	if(sel == MC_STORY) {
		choice_put_centered(BOX_MAIN_CENTER_X, 0, 0, COMMAND_STORY, atrb);
	} else if(sel == MC_VS) {
		choice_put_centered(BOX_MAIN_CENTER_X, 1, -1, COMMAND_VS, atrb);
	} else if(sel == MC_MUSICROOM) {
		choice_put_centered(BOX_MAIN_CENTER_X, 2, -1, COMMAND_MUSICROOM, atrb);
	} else if(sel == MC_REGIST_VIEW) {
		choice_put_centered(
			BOX_MAIN_CENTER_X, 3, -1, COMMAND_REGIST_VIEW, atrb
		);
	} else if(sel == MC_OPTION) {
		choice_put_centered(BOX_MAIN_CENTER_X, 4, -1, COMMAND_OPTION, atrb);
	} else if(sel == MC_REPLAY) {
		choice_put_centered(BOX_MAIN_CENTER_X, 5, -1, COMMAND_REPLAY, atrb);
	} else if(sel == MC_QUIT) {
		choice_put_centered(BOX_MAIN_CENTER_X, 6, -1, COMMAND_QUIT, atrb);
	}
}

#pragma option -a2

static void near option_choice_draw(int sel, tram_atrb2 atrb)
{
	enum {
		// ZUN quirk: Not the center of the left column.
		LABEL_CENTER_X = BOX_MAIN_CENTER_X,

		VALUE_LEFT = BOX_SUBMENU_CENTER_X,
		VALUE_W = (BOX_OPTION_RIGHT - VALUE_LEFT),
		VALUE_CENTER_X = (VALUE_LEFT + (VALUE_W / 2)),
	};

	if(sel == OC_RANK) {
		choice_put_centered(LABEL_CENTER_X, 0, 0, LABEL_RANK, atrb);
		switch(resident->rank) {
		case RANK_EASY:
			choice_put_centered(VALUE_CENTER_X, 0, 1, VALUE_EASY, atrb);
			break;
		case RANK_NORMAL:
			choice_put_centered(VALUE_CENTER_X, 0, 1, VALUE_NORMAL, atrb);
			break;
		case RANK_HARD:
			choice_put_centered(VALUE_CENTER_X, 0, 1, VALUE_HARD, atrb);
			break;
		case RANK_LUNATIC:
			choice_put_centered(VALUE_CENTER_X, 0, 1, VALUE_LUNATIC, atrb);
			break;
		}
	} else if(sel == OC_BGM) {
		choice_put_centered(LABEL_CENTER_X, 1, -1, LABEL_MUSIC, atrb);
		switch(resident->bgm_mode) {
		case SND_BGM_OFF:
			choice_put_centered(VALUE_CENTER_X, 1, 0, VALUE_OFF, atrb);
			break;
		case SND_BGM_FM:
			choice_put_centered(VALUE_CENTER_X, 1, 0, VALUE_FM, atrb);
			break;
		case SND_BGM_MIDI:
			choice_put_centered(VALUE_CENTER_X, 1, 0, VALUE_MIDI, atrb);
			break;
		}
	} else if(sel == OC_SFX) {
		choice_put_centered(LABEL_CENTER_X, 2, -1, LABEL_SFX, atrb);
		choice_put_centered(
			VALUE_CENTER_X, 2, 0,
			(th03_snd_se_enabled() ? VALUE_ON : VALUE_OFF), atrb
		);
	} else if(sel == OC_LANGUAGE) {
		title_credit_line[0] = 'L'; title_credit_line[1] = 'a';
		title_credit_line[2] = 'n'; title_credit_line[3] = 'g';
		title_credit_line[4] = 'u'; title_credit_line[5] = 'a';
		title_credit_line[6] = 'g'; title_credit_line[7] = 'e';
		title_credit_line[8] = '\0';
		choice_put_centered(
			LABEL_CENTER_X, 3, -1, title_credit_line, atrb
		);
		if(language_is_english()) {
			title_credit_line[0] = 'E'; title_credit_line[1] = 'n';
			title_credit_line[2] = 'g'; title_credit_line[3] = 'l';
			title_credit_line[4] = 'i'; title_credit_line[5] = 's';
			title_credit_line[6] = 'h'; title_credit_line[7] = '\0';
			choice_put_centered(
				VALUE_CENTER_X, 3, 0, title_credit_line, atrb
			);
		} else if(menu_font) {
			title_credit_line[0] = (char)0x93;
			title_credit_line[1] = (char)0xFA;
			title_credit_line[2] = (char)0x96;
			title_credit_line[3] = (char)0x7B;
			title_credit_line[4] = (char)0x8C;
			title_credit_line[5] = (char)0xEA;
			title_credit_line[6] = '\0';
			graph_putsa_fx(
				(VALUE_CENTER_X - ((3 * GLYPH_FULL_W) / 2)),
				(choice_tram_y(3) * GLYPH_H),
				((atrb == TX_BLACK) ? 0 : V_WHITE), title_credit_line
			);
		} else {
			title_credit_line[0] = (char)0x93;
			title_credit_line[1] = (char)0xFA;
			title_credit_line[2] = (char)0x96;
			title_credit_line[3] = (char)0x7B;
			title_credit_line[4] = (char)0x8C;
			title_credit_line[5] = (char)0xEA;
			title_credit_line[6] = '\0';
			text_putsa(
				((VALUE_CENTER_X / GLYPH_HALF_W) - 3),
				choice_tram_y(3), title_credit_line, atrb
			);
		}
	} else if(sel == OC_KEY_MODE) {
		choice_put_centered(LABEL_CENTER_X, 4, -1, LABEL_KEYCONFIG, atrb);
		switch(resident->key_mode) {
		case KM_KEY_KEY:
			choice_put_centered(VALUE_CENTER_X, 4, -1, VALUE_KEY_KEY, atrb);
			break;
		case KM_JOY_KEY:
			choice_put_centered(VALUE_CENTER_X, 4, -1, VALUE_JOY_KEY, atrb);
			break;
		case KM_KEY_JOY:
			choice_put_centered(VALUE_CENTER_X, 4, -1, VALUE_KEY_JOY, atrb);
			break;
		}
	} else if(sel == OC_PHOTOSENSITIVITY) {
		title_credit_line[0] = 'F'; title_credit_line[1] = 'l';
		title_credit_line[2] = 'a'; title_credit_line[3] = 's';
		title_credit_line[4] = 'h'; title_credit_line[5] = ' ';
		title_credit_line[6] = 'F'; title_credit_line[7] = 'X';
		title_credit_line[8] = '\0';
		choice_put_centered(
			LABEL_CENTER_X, 5, -1, title_credit_line, atrb
		);
		if(photosensitivity_enabled()) {
			title_credit_line[0] = 'R'; title_credit_line[1] = 'e';
			title_credit_line[2] = 'd'; title_credit_line[3] = 'u';
			title_credit_line[4] = 'c'; title_credit_line[5] = 'e';
			title_credit_line[6] = 'd'; title_credit_line[7] = '\0';
		} else {
			title_credit_line[0] = 'F'; title_credit_line[1] = 'u';
			title_credit_line[2] = 'l'; title_credit_line[3] = 'l';
			title_credit_line[4] = '\0';
		}
		choice_put_centered(
			VALUE_CENTER_X, 5, 0, title_credit_line, atrb
		);
	} else if(sel == OC_QUIT) {
		choice_put_centered(BOX_OPTION_CENTER_X, 6, 0, COMMAND_QUIT, atrb);
	}
}

void pascal near option_choice_put(int sel, tram_atrb2 atrb)
{
	unsigned line = ((sel == OC_QUIT) ? 6 : sel);

	title_choice_graphics_unput(line, BOX_OPTION_RIGHT);
	if(!menu_font && (sel != OC_QUIT)) {
		text_putsa(
			(BOX_SUBMENU_CENTER_X / GLYPH_HALF_W), choice_tram_y(line),
			"            ", TX_WHITE
		);
	}
	option_choice_draw(sel, atrb);
}

static void near option_language_title_refresh(void)
{
	// The regular title loaders load TL02.PI before the character-select CDGs.
	// Free those CDGs here as well: Keeping them allocated can make pi_load()
	// fail, after which master.lib leaves the freed slot pointer unchanged and
	// pi_put_8() would blit unrelated heap contents into VRAM.
	for(int i = 0; i < CDG_SLOT_COUNT; i++) {
		cdg_free(i);
	}
	if(language_pi_load_freed_slot(0, MENU_MAIN_BG_FN) != 0) {
		goto reload_select_cdgs;
	}
	pi_palette_apply(0);

	graph_accesspage(1);
	pi_put_8(0, 0, 0);
	graph_accesspage(0);
	// Keep the valid animated box on page 0. Copying only around it avoids
	// stacking OPWIN_RIGHT's transparent intermediate edges, while the clean
	// page-1 title remains available to the regular contraction animation.
	menu_font_restore_rect(0, 0, RES_X, BOX_TOP);
	menu_font_restore_rect(0, BOX_BOTTOM, RES_X, (RES_Y - BOX_BOTTOM));
	menu_font_restore_rect(0, BOX_TOP, BOX_LEFT, BOX_H);
	menu_font_restore_rect(
		BOX_OPTION_RIGHT, BOX_TOP, (RES_X - BOX_OPTION_RIGHT), BOX_H
	);
	title_credit_put();
	pi_free(0);

reload_select_cdgs:
	select_cdg_load_part1_of_4();
	select_cdg_load_part3_of_4();
	select_cdg_load_part2_of_4();
}

void pascal near menu_sel_update_and_render(int8_t max, int8_t direction)
{
	menu_put(menu_sel, TX_BLACK);
	menu_sel += direction;
	if(menu_sel < ring_min()) {
		menu_sel = max;
	}
	if(menu_sel > max) {
		menu_sel = 0;
	}
	menu_put(menu_sel, TX_WHITE);
}

#define menu_init(in_this_menu, input_allowed, choice_count, put) { \
	input_allowed = false; /* ZUN bloat: Redundant */ \
	for(int i = 0; i < choice_count; i++) { \
		put(i, ((menu_sel == i) ? TX_WHITE : TX_BLACK)); \
	} \
	menu_put = put; \
	in_this_menu = true; \
	input_allowed = false; \
}

inline void return_from_other_screen_to_main(
	bool& in_this_menu, bool& main_input_allowed, bool restart_bgm
) {
	op_fadein_animate(restart_bgm);
	wait_for_input_or_start_demo_then_box_to_main_animate();
	select_cdg_load_part2_of_4();
	in_this_menu = false;
	main_input_allowed = false;
	in_main = true;
}

// Sure, *maybe* these names should point out the possibility of a blocking
// box transition animation, but main_update_and_render() also directly enters
// the even more blocking character selection and Music Room screens.
void near main_update_and_render(void)
{
	t3pix_scene_set(T3PIX_SCENE_TITLE);
	#define input_allowed	main_input_allowed
	static bool in_this_menu = false;

	if(!in_this_menu) {
		// Preserve the accepted code phase after removing the title text scroll.
		__emit__(0x90, 0x90);
		text_clear();
		if(!in_main) {
			screen_x_t box_right = (
				(menu_sel == MC_OPTION) ? BOX_OPTION_RIGHT : BOX_SUBMENU_RIGHT
			);
			title_menu_graphics_unput(box_right);
			box_submenu_to_main_animate(box_right);
		}
		in_main = false; // ZUN bloat: Why is this set here, and now?
		menu_init(in_this_menu, input_allowed, MC_COUNT, main_choice_put);
		title_credit_put();
	}

	if(input_sp == INPUT_NONE) {
		input_allowed = true;
	}
	if(!input_allowed) {
		return;
	}
	title_extra_unlock_update();
	menu_update_vertical(input_sp, MC_COUNT);
	if((input_sp & INPUT_OK) || (input_sp & INPUT_SHOT)) {
		// Preserve the accepted control flow after removing its conditional reset.
		if(menu_sel != MC_OPTION) {
			__emit__(0x90, 0x90, 0x90, 0x90);
		}
		switch(menu_sel) {
		case MC_STORY:
			story_menu();
			return_from_other_screen_to_main(in_this_menu, input_allowed, true);
			return;
		case MC_VS:
			resident->playchar_paletted[0].set(PLAYCHAR_REIMU);
			resident->playchar_paletted[1].set(PLAYCHAR_REIMU);
			if(vs_menu()) {
				menu_sel = MC_VS;
				return_from_other_screen_to_main(
					in_this_menu, input_allowed, true
				);
			} else {
				in_this_menu = false;
				menu_sel = MC_VS;
			}
			return;
		case MC_MUSICROOM:
			t3pix_scene_set(T3PIX_SCENE_MUSIC_ROOM);
			/* TODO: Replace with the decompiled call
			* 	musicroom_menu();
			* once the segmentation allows us to, if ever */
			_asm { nop; push cs; call near ptr musicroom_menu; }

			return_from_other_screen_to_main(
				in_this_menu, input_allowed, (_AX == 0)
			);
			return;
		case MC_REGIST_VIEW:
			score_menu();
			break; // launches into MAINL.EXE
		case MC_OPTION:
			in_this_menu = false;
			in_option = true;
			menu_sel = OC_RANK;
			break;
		case MC_REPLAY:
			replay_menu();
			return_from_other_screen_to_main(in_this_menu, input_allowed, false);
			return;
		case MC_QUIT:
			in_this_menu = false; // We're quitting anyway...
			quit = true;
			break;
		}
	}
	if(input_sp & INPUT_CANCEL) {
		quit = true;
	}
	if(input_sp != INPUT_NONE) { // Covers all previous input cases too! Good!
		input_allowed = false;
	}

	#undef input_allowed
}

#define bgm_cycle(ring_direction) { \
	if(!snd_sel_disabled) { \
		snd_kaja_func(KAJA_SONG_STOP, 0); \
		ring_direction(resident->bgm_mode, SND_BGM_MIDI); \
		th03_snd_process_init(); \
		if(resident->bgm_mode != SND_BGM_OFF) { \
			if(snd_midi_active) { \
				snd_load("gminit.m", SND_LOAD_SONG); \
				snd_kaja_func(KAJA_SONG_PLAY, 0); \
				frame_delay(4); \
			} \
			snd_load(BGM_MENU_MAIN_FN, SND_LOAD_SONG); \
			snd_kaja_func(KAJA_SONG_PLAY, 0); \
		} \
		/* ZUN bloat: Already done at the call site. */ \
		option_choice_put(menu_sel, TX_WHITE); \
	} \
}

#define bgm_ring_dec(val, ring_end) \
	ring_dec_range(val, SND_BGM_OFF, ring_end)

#define sfx_flip() { \
	th03_snd_se_toggle(); \
}

#define photosensitivity_flip() { \
	photosensitivity_enabled_set(!photosensitivity_enabled()); \
}

inline void return_from_option_to_main(bool& option_initialized) {
	option_initialized = false;
	menu_sel = MC_OPTION;
	in_option = false;
}

static void near option_return_to_main(bool& option_initialized)
{
	if(padding != language_resident()) {
		option_language_title_refresh();
	}
	return_from_option_to_main(option_initialized);
}

void near option_update_and_render(void)
{
	t3pix_scene_set(T3PIX_SCENE_OPTIONS);
	#define input_allowed	option_input_allowed
	static bool in_this_menu = false;

	if(!in_this_menu) {
		padding = language_resident();
		text_clear();
		title_menu_graphics_unput(BOX_MAIN_RIGHT);
		box_main_to_submenu_animate(BOX_OPTION_RIGHT);
		menu_init(in_this_menu, input_allowed, OC_COUNT, option_choice_put);
	}

	if(input_sp == INPUT_NONE) {
		input_allowed = true;
	}
	if(!input_allowed) {
		return;
	}
	menu_update_vertical(input_sp, OC_COUNT);

	// ZUN bloat: Could have been deduplicated.
	if(input_sp & INPUT_RIGHT) {
		switch(menu_sel) {
		case OC_RANK:
			ring_inc_range(resident->rank, RANK_EASY, RANK_LUNATIC);
			break;
		case OC_BGM:
			bgm_cycle(ring_inc);
			break;
		case OC_SFX:
			sfx_flip();
			break;
		case OC_LANGUAGE:
			language_op_toggle();
			break;
		case OC_KEY_MODE:
			ring_inc_range(resident->key_mode, KM_KEY_KEY, KM_KEY_JOY);
			break;
		case OC_PHOTOSENSITIVITY:
			photosensitivity_flip();
			break;
		}
		option_choice_put(menu_sel, TX_WHITE);
	}
	if(input_sp & INPUT_LEFT) {
		switch(menu_sel) {
		case OC_RANK:
			ring_dec_range(resident->rank, RANK_EASY, RANK_LUNATIC);
			break;
		case OC_BGM:
			bgm_cycle(bgm_ring_dec);
			break;
		case OC_SFX:
			sfx_flip();
			break;
		case OC_LANGUAGE:
			language_op_toggle();
			break;
		case OC_KEY_MODE:
			ring_dec_range(resident->key_mode, KM_KEY_KEY, KM_KEY_JOY);
			break;
		case OC_PHOTOSENSITIVITY:
			photosensitivity_flip();
			break;
		}
		option_choice_put(menu_sel, TX_WHITE);
	}

	if((input_sp & INPUT_OK) || (input_sp & INPUT_SHOT)) {
		if(menu_sel == OC_KEY_MODE) {
			fullscreen_menu_resources_clear();
			keyconfig_menu();
			t3pix_scene_set(T3PIX_SCENE_OPTIONS);
			menu_sel = MC_OPTION;
			in_option = false;
			return_from_other_screen_to_main(
				in_this_menu, main_input_allowed, false
			);
			return;
		} else if(menu_sel == OC_LANGUAGE) {
			language_op_toggle();
			option_choice_put(menu_sel, TX_WHITE);
		} else if(menu_sel == OC_PHOTOSENSITIVITY) {
			photosensitivity_flip();
			option_choice_put(menu_sel, TX_WHITE);
		} else if(menu_sel == OC_QUIT) {
			option_return_to_main(in_this_menu);
		}
	}
	if(input_sp & INPUT_CANCEL) {
		option_return_to_main(in_this_menu);
	}
	if(input_sp != INPUT_NONE) { // Covers all previous input cases too! Good!
		input_allowed = false;
	}

	#undef input_allowed
}

#undef bgm_ring_dec

#undef photosensitivity_flip

void main(void)
{
	bool replay_restart_requested;
	bool replay_save_prompt;
	bool replay_save_direct;
	char replay_save_resume_mode;

	{
		char replay_mode = replay_cfg_mode();
		if(replay_mode != 0) {
			#if defined(TH03_PIXEL_CAPTURE)
			// Capture-profile state equivalence: The deterministic headless
			// handoff below bypasses ordinary OP startup, whereas selecting a
			// replay from the in-game browser has already called respal_create().
			// The raw oracle needs that same resident palette before MAINL/MAIN;
			// this does not alter any release or non-capture binary.
			respal_create();
			#endif
			replay_start_demo_headless(replay_mode);
			return;
		}
	}

	graph_400line();
	text_clear();
	respal_create();
	// The allocation-safe language refresh above now occupies the 32-byte
	// phase pad left by removing the GDC frequency check.

	if(game_init_op(OP_AND_END_PF_FN)) {
		dos_puts2(ERROR_OUT_OF_MEMORY);
		getch();
		return;
	}

	gaiji_backup();
	gaiji_entry_bfnt(GAIJI_FN);
	menu_font_load(
		reinterpret_cast<const unsigned char far *>(OP_AND_END_PF_FN)
	);
	cfg_load();
	if(snd_midi_active) {
		snd_load("gminit.m", SND_LOAD_SONG);
		snd_kaja_func(KAJA_SONG_PLAY, 0);
		frame_delay(4);
	}
	replay_restart_requested = replay_resident_handoff_is(
		T3_REPLAY_RES_MODE_RESTART
	);
	replay_save_prompt = replay_resident_handoff_is(
		T3_REPLAY_RES_MODE_SAVE_PROMPT
	);
	replay_save_direct = replay_resident_handoff_is(
		T3_REPLAY_RES_MODE_SAVE_DIRECT
	);
	replay_save_resume_mode = 0;
	if(replay_resident_handoff_is(T3_REPLAY_RES_MODE_SAVE_PROMPT_GAME_OVER)) {
		replay_save_prompt = true;
		replay_save_resume_mode = T3_REPLAY_RES_MODE_RESUME_GAME_OVER;
	} else if(replay_resident_handoff_is(T3_REPLAY_RES_MODE_SAVE_PROMPT_CLEAR)) {
		replay_save_prompt = true;
		replay_save_resume_mode = T3_REPLAY_RES_MODE_RESUME_CLEAR;
	}
	if(replay_resident_handoff_is(T3_REPLAY_RES_MODE_ACCEL_CLEANUP)) {
		replay_accel_temps_delete();
	}
	replay_resident_handoff_clear();
	if(replay_save_prompt || replay_save_direct) {
		replay_save_pending(replay_save_prompt);
	}
	if(replay_save_resume_mode != 0) {
		replay_resident_handoff_set(replay_save_resume_mode);
		if(replay_save_resume_mode == T3_REPLAY_RES_MODE_RESUME_GAME_OVER) {
			switch_to_mainl_preserve_bgm(false);
		} else {
			switch_to_mainl(false);
		}
		return;
	}
	if(replay_restart_requested) {
		if(resident->game_mode == GM_STORY) {
			story_start(false);
			return;
		}
		if(resident->game_mode >= GM_VS) {
			vs_start(false);
			return;
		}
	}
	if((resident->game_mode >= GM_VS) && (resident->demo_num == 0)) {
		select_cdg_load_part1_of_4();
		select_cdg_load_part3_of_4();
		select_cdg_load_part2_of_4();
		// Replay-save teardown leaves the global palette tone at black.
		PaletteTone = 100;
		vs_menu();
	}

	if(!resident->op_animation_fast) {
		op_animate();
		resident->op_animation_fast = true;
	} else {
		resident->op_animation_fast = false;
		op_fadein_animate(true);
	}
	wait_for_input_or_start_demo_then_box_to_main_animate();

	// Showing the menu options before loading part 2 is actually a pretty nice
	// idea to better hide potentially long loading times.
	//
	// ZUN quirk: Resetting [input_sp] regardless of the actually held keys
	// means that main_update_and_render() always returns with its instance of
	// [input_allowed] set to `true`. Thus, any initially held key is processed
	// instantly on the first call to the function in the loop below – contrary
	// to what you would expect from the whole input locking system, and
	// contrary to how the game behaves after switching the active menu later
	// on, where inputs *are* locked until the player releases all keys.
	in_option = false;
	input_sp = INPUT_NONE;
	main_update_and_render();

	select_cdg_load_part2_of_4();

	while(!quit) {
		input_mode_interface();
		switch(in_option) {
		case false:	main_update_and_render();  	break;
		case true: 	option_update_and_render();	break;
		}
		resident->rand++;
		frame_delay(1);
	}
	cfg_save_exit();

	// ZUN landmine: The system's previous gaiji should be restored *after*
	// clearing TRAM, not before while we're still showing menu text. Sending
	// ((8,192 × 2) + 512) bytes of data over I/O ports one byte at a time
	// takes a short while, so this can definitely be visible for a fraction of
	// a frame on real, not infinitely fast hardware. Especially since the CRT
	// beam is most certainly in the middle of a frame after the file I/O
	// immediately above.
	// Funnily enough, TH02 got the order correct right in the one place where
	// it mattered in that game.
	//
	// ZUN bloat: Also, game_exit_to_dos() already clears TRAM.
	gaiji_restore();
	text_clear();

	game_exit_to_dos();
	respal_free();
}

#if defined(TH03_REPLAY_DEV_STAGE_SELECT)
static int near replay_dev_story_stage_menu(void)
{
	int stage = 6;
	input_t input_prev;
	uint16_t near *pairs = reinterpret_cast<uint16_t near *>(title_credit_line);

	#define DEBUG_TEXT_PAIR(index, left, right) \
		pairs[index] = static_cast<uint16_t>((left) | ((right) << 8))

	text_clear();
	graph_accesspage(0);
	graph_clear();
	graph_accesspage(1);
	graph_clear();
	graph_showpage(0);
	graph_accesspage(0);

	DEBUG_TEXT_PAIR(0, 'D', 'E');
	DEBUG_TEXT_PAIR(1, 'B', 'U');
	DEBUG_TEXT_PAIR(2, 'G', ' ');
	DEBUG_TEXT_PAIR(3, 'S', 'T');
	DEBUG_TEXT_PAIR(4, 'O', 'R');
	DEBUG_TEXT_PAIR(5, 'Y', ' ');
	DEBUG_TEXT_PAIR(6, 'S', 'T');
	DEBUG_TEXT_PAIR(7, 'A', 'R');
	title_credit_line[16] = 'T';
	title_credit_line[17] = '\0';
	text_putsa(31, 8, title_credit_line, TX_WHITE);

	DEBUG_TEXT_PAIR(0, 'U', 'P');
	DEBUG_TEXT_PAIR(1, '/', 'D');
	DEBUG_TEXT_PAIR(2, 'N', ':');
	DEBUG_TEXT_PAIR(3, ' ', 'S');
	DEBUG_TEXT_PAIR(4, 'E', 'L');
	DEBUG_TEXT_PAIR(5, 'E', 'C');
	title_credit_line[12] = 'T';
	title_credit_line[13] = '\0';
	text_putsa(31, 12, title_credit_line, TX_WHITE);

	DEBUG_TEXT_PAIR(0, 'Z', '/');
	DEBUG_TEXT_PAIR(1, 'E', 'N');
	DEBUG_TEXT_PAIR(2, 'T', 'E');
	DEBUG_TEXT_PAIR(3, 'R', ':');
	DEBUG_TEXT_PAIR(4, ' ', 'S');
	DEBUG_TEXT_PAIR(5, 'T', 'A');
	DEBUG_TEXT_PAIR(6, 'R', 'T');
	title_credit_line[14] = '\0';
	text_putsa(29, 13, title_credit_line, TX_WHITE);

	DEBUG_TEXT_PAIR(0, 'E', 'S');
	DEBUG_TEXT_PAIR(1, 'C', ':');
	DEBUG_TEXT_PAIR(2, ' ', 'C');
	DEBUG_TEXT_PAIR(3, 'A', 'N');
	DEBUG_TEXT_PAIR(4, 'C', 'E');
	title_credit_line[10] = 'L';
	title_credit_line[11] = '\0';
	text_putsa(33, 14, title_credit_line, TX_WHITE);

	DEBUG_TEXT_PAIR(0, 'S', 'T');
	DEBUG_TEXT_PAIR(1, 'A', 'G');
	DEBUG_TEXT_PAIR(2, 'E', ' ');
	title_credit_line[6] = '7';
	title_credit_line[7] = '\0';
	text_putsa(36, 10, title_credit_line, TX_WHITE);

	input_mode_interface();
	input_prev = input_sp;
	while(1) {
		input_mode_interface();
		if(input_prev == INPUT_NONE) {
			if(input_sp & (INPUT_UP | INPUT_LEFT)) {
				stage = ((stage == 0) ? (STAGE_COUNT - 1) : (stage - 1));
			} else if(input_sp & (INPUT_DOWN | INPUT_RIGHT)) {
				stage = ((stage == (STAGE_COUNT - 1)) ? 0 : (stage + 1));
			} else if(input_sp & (INPUT_OK | INPUT_SHOT)) {
				text_clear();
				return stage;
			} else if(input_sp & INPUT_CANCEL) {
				text_clear();
				return STAGE_NONE;
			}
			title_credit_line[6] = ('1' + stage);
			text_putsa(36, 10, title_credit_line, TX_WHITE);
		}
		input_prev = input_sp;
		frame_delay(1);
	}

	#undef DEBUG_TEXT_PAIR
}

// Keep the following shared runtime segment at its accepted paragraph phase.
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90"
#endif
#if defined(TH03_REPLAY_DEV_STAGE_SELECT)
// Replaces the obsolete initial-playback stage-select handoff in this profile.
#pragma codestring "\x90\x90\x90\x90\x90\x90"
#if defined(TH03_REPLAY_DEVTOOLS)
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90"
#else
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90"
#endif
#endif
// Keep all following original OP contributions at their 0.2.13 offsets.
// Keep the browser's dense proportional layouts in patch-owned tail code.
#if defined(TH03_REPLAY_DEVTOOLS)
#elif defined(TH03_REPLAY_DEV_STAGE_SELECT)

#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90"
#endif
#if defined(TH03_REPLAY_DEVTOOLS)
// The debug-only Shift override is larger than the release handoff.
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90"
#endif
// Keep every following original/shared OP contribution at its accepted offset
// after moving the localized Replay UI into the patch-owned RPYFONT segment.
// The range-safe BGM decrement and unavailable-MIDI preservation reclaim
// eleven bytes from this inert pad.
// The 8.3-safe language header name corrects the far-call declarations and
// reclaims another 23 bytes, compensated here to retain the proven layout.
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90"
#if defined(TH03_REPLAY_DEVTOOLS)
// The debug profile has two fewer affected bytes than stage-select.
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#elif defined(TH03_REPLAY_DEV_STAGE_SELECT)
// This profile adds further calls covered by the corrected declaration.
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#endif
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
/// --------
